#include "UiTypes.hpp"

#include "src/Log.hpp"
#include "src/OpenSimBindings/Simulation.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/Component.h>
#include <SimTKcommon.h>

#include <chrono>
#include <utility>

using namespace osc;
using std::chrono_literals::operator""s;

// translate a pointer to a component in model A to a pointer to a component in model B
//
// returns nullptr if the pointer cannot be cleanly translated
static OpenSim::Component* relocateComponentPointerToAnotherModel(OpenSim::Model const& model, OpenSim::Component* ptr) {
    if (!ptr) {
        return nullptr;
    }

    try {
        return const_cast<OpenSim::Component*>(model.findComponent(ptr->getAbsolutePath()));
    } catch (OpenSim::Exception const&) {
        // finding fails with exception when ambiguous (fml)
        return nullptr;
    }
}

namespace {
    struct DebounceRv final {
        std::chrono::system_clock::time_point validAt;
        bool shouldUndo;
    };
}

static DebounceRv canPushNewUndoStateWithDebounce(UndoableUiModel const& uim) {
    DebounceRv rv;
    rv.validAt = std::chrono::system_clock::now();
    rv.shouldUndo = uim.undo.empty() || uim.undo.back().timestamp + 5s <= rv.validAt;
    return rv;
}

static void doDebouncedUndoPush(UndoableUiModel& uim) {
    auto [validAt, shouldUndo] = canPushNewUndoStateWithDebounce(uim);

    if (shouldUndo) {
        uim.undo.emplace_back(uim.current, validAt);
        uim.redo.clear();
    }
}

static void rollbackModelToEarlierState(UndoableUiModel& uim) {
    if (uim.undo.empty()) {
        log::error("the model cannot be fixed: no earlier versions of the model exist, throwing an exception");
        throw std::runtime_error{
            "an OpenSim::Model was put into an invalid state: probably by a modification. We tried to recover from this error, but couldn't - view the logs"};
    }

    // otherwise, we have undo entries we can pop
    log::error(
        "attempting to rollback to an earlier (pre-modification) of the model that was saved into the undo buffer");
    try {
        uim.damaged.emplace(std::move(uim.current));
        uim.current = uim.undo.pop_back();
        return;
    } catch (...) {
        log::error(
            "error encountered when trying to rollback to an earlier version of the model, this will be thrown as an exception");
        throw;
    }
}

static void carefullyTryToInitSimTKSystemAndRealizeOnCurrentModel(UndoableUiModel& uim) {
    // this code is messy because it has to handle the messy situation where
    // the `current` model has been modified into an invalid state that OpenSim
    // refuses to initialize a system for
    //
    // this code has to balance being super-aggressive (i.e. immediately terminating with
    // a horrible error message) against letting the UI limp along with the broken
    // model *just* long enough for a recovery effort to complete. Typical end-users
    // are going to *strongly* prefer the latter, because they might have unsaved
    // changes in the UI that should not be lost by a crash.

    try {
        uim.current.onUiModelModified();
        return;
    } catch (std::exception const& ex) {
        log::error("exception thrown when initializing updated model: %s", ex.what());
    }

    rollbackModelToEarlierState(uim);
}

static std::unique_ptr<FdSimulation> createForwardDynamicSim(OpenSim::Model const& m, SimTK::State const& s, FdParams const& p) {
    auto modelCopy = std::make_unique<OpenSim::Model>(m);
    auto stateCopy = std::make_unique<SimTK::State>(s);

    modelCopy->initSystem();
    modelCopy->setPropertiesFromState(*stateCopy);
    modelCopy->realizePosition(*stateCopy);
    modelCopy->equilibrateMuscles(*stateCopy);
    modelCopy->realizeAcceleration(*stateCopy);

    auto simInput = std::make_unique<Input>(std::move(modelCopy), std::move(stateCopy));
    simInput->params = p;

    return std::make_unique<FdSimulation>(std::move(simInput));
}

static std::unique_ptr<OpenSim::Model> createInitializedModel(OpenSim::Model const& m) {
    auto rv = std::make_unique<OpenSim::Model>(m);
    rv->finalizeFromProperties();
    rv->initSystem();
    return rv;
}

static std::unique_ptr<Report> createDummySimulationReport(OpenSim::Model const& m) {
    auto rv = std::make_unique<Report>();
    rv->state = m.getWorkingState();
    rv->stats = {};
    m.realizeReport(rv->state);
    return rv;
}

// named namespace is due to an MSVC internal linkage compiler bug
namespace subfield_magic {

    enum Subfield_ {
        Subfield_x,
        Subfield_y,
        Subfield_z,
        Subfield_mag,
    };

    // top-level output extractor declaration
    template<typename ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // subfield output extractor declaration
    template<Subfield_ sf, typename ConcreteOutput>
    double extract(ConcreteOutput const&, SimTK::State const&);

    // extract a `double` from an `OpenSim::Property<double>`
    template<>
    double extract<>(OpenSim::Output<double> const& o, SimTK::State const& s) {
        return o.getValue(s);
    }

    // extract X from `SimTK::Vec3`
    template<>
    double extract<Subfield_x>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).get(0);
    }

    // extract Y from `SimTK::Vec3`
    template<>
    double extract<Subfield_y>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).get(1);
    }

    // extract Z from `SimTK::Vec3`
    template<>
    double extract<Subfield_z>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).get(2);
    }

    // extract magnitude from `SimTK::Vec3`
    template<>
    double extract<Subfield_mag>(OpenSim::Output<SimTK::Vec3> const& o, SimTK::State const& s) {
        return o.getValue(s).norm();
    }

    // type-erase a concrete extractor function
    template<typename OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s) {
        return extract<>(dynamic_cast<OutputType const&>(o), s);
    }

    // type-erase a concrete *subfield* extractor function
    template<Subfield_ sf, typename OutputType>
    double extractTypeErased(OpenSim::AbstractOutput const& o, SimTK::State const& s) {
        return extract<sf>(dynamic_cast<OutputType const&>(o), s);
    }

    // helper function that wires the above together for the lookup
    template<Subfield_ sf, typename TValue>
    PlottableOutputSubfield subfield(char const* name) {
        return {name, extractTypeErased<sf, OpenSim::Output<TValue>>, typeid(OpenSim::Output<TValue>).hash_code()};
    }

    // create a constant-time lookup for an OpenSim::Output<T>'s available subfields
    static std::unordered_map<size_t, std::vector<osc::PlottableOutputSubfield>> createSubfieldLookup() {
        std::unordered_map<size_t, std::vector<osc::PlottableOutputSubfield>> rv;

        // SimTK::Vec3
        rv[typeid(OpenSim::Output<SimTK::Vec3>).hash_code()] = {
            subfield<Subfield_x, SimTK::Vec3>("x"),
            subfield<Subfield_y, SimTK::Vec3>("y"),
            subfield<Subfield_z, SimTK::Vec3>("z"),
            subfield<Subfield_mag, SimTK::Vec3>("magnitude"),
        };

        return rv;
    }

    // returns top-level extractor function for an AO, or nullptr if it isn't plottable
    static extrator_fn_t extractorFunctionForOutput(OpenSim::AbstractOutput const& ao) {
        auto const* dp = dynamic_cast<OpenSim::Output<double> const*>(&ao);
        if (dp) {
            return extractTypeErased<OpenSim::Output<double>>;
        } else {
            return nullptr;
        }
    }
}


// public API

osc::UiModel::UiModel(std::unique_ptr<OpenSim::Model> _model) :
    model{[&_model]() {
        _model->finalizeFromProperties();
        _model->finalizeConnections();
        return std::unique_ptr<OpenSim::Model>{std::move(_model)};
    }()},
    state{[this]() {
        std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(model->initSystem());
        model->realizePosition(*rv);
        return rv;
    }()},
    selected{nullptr},
    hovered{nullptr},
    isolated{nullptr},
    timestamp{std::chrono::system_clock::now()} {
}

osc::UiModel::UiModel(UiModel const& other, std::chrono::system_clock::time_point t) :
    model{[&other]() {
        auto copy = std::make_unique<OpenSim::Model>(*other.model);
        copy->finalizeFromProperties();
        copy->finalizeConnections();
        return copy;
    }()},
    state{[this]() {
        std::unique_ptr<SimTK::State> rv = std::make_unique<SimTK::State>(model->initSystem());
        model->equilibrateMuscles(*this->state);
        model->realizePosition(*rv);
        return rv;
    }()},
    selected{relocateComponentPointerToAnotherModel(*model, other.selected)},
    hovered{relocateComponentPointerToAnotherModel(*model, other.hovered)},
    isolated{relocateComponentPointerToAnotherModel(*model, other.isolated)},
    timestamp{t} {
}

osc::UiModel::UiModel(UiModel const& other) : UiModel{other, std::chrono::system_clock::now()} {
}

osc::UiModel::UiModel(UiModel&&) noexcept = default;

osc::UiModel::~UiModel() noexcept = default;

osc::UiModel& osc::UiModel::operator=(UiModel&&) = default;

void osc::UiModel::onUiModelModified() {
    *this->state = model->initSystem();
    model->equilibrateMuscles(*this->state);
    model->realizePosition(*this->state);
    this->timestamp = std::chrono::system_clock::now();
}


osc::UndoableUiModel::UndoableUiModel(std::unique_ptr<OpenSim::Model> model) :
    current{std::move(model)}, undo{}, redo{}, damaged{std::nullopt} {
}

void osc::UndoableUiModel::doUndo() {
    if (!canUndo()) {
        return;
    }

    redo.emplace_back(std::move(current));
    current = undo.pop_back();
}

void osc::UndoableUiModel::doRedo() {
    if (!canRedo()) {
        return;
    }

    undo.emplace_back(std::move(current));
    current = redo.pop_back();
}

void osc::UndoableUiModel::setModel(std::unique_ptr<OpenSim::Model> newModel) {
    // care: this step can throw, because it initializes a system etc.
    //       so, do this *before* potentially breaking these sequecnes
    UiModel newCurrentModel{std::move(newModel)};

    undo.emplace_back(std::move(current));
    redo.clear();
    current = std::move(newCurrentModel);
}

void osc::UndoableUiModel::beforeModifyingModel() {
    osc::log::debug("starting model modification");
    doDebouncedUndoPush(*this);
}

void osc::UndoableUiModel::afterModifyingModel() {
    osc::log::debug("ended model modification");
    carefullyTryToInitSimTKSystemAndRealizeOnCurrentModel(*this);
}

void osc::UndoableUiModel::forciblyRollbackToEarlierState() {
    rollbackModelToEarlierState(*this);
}

void osc::UndoableUiModel::clearAnyDamagedModels() {
    if (damaged) {
        log::error("destructing damaged model");
        damaged = std::nullopt;
    }
}

osc::UiSimulation::UiSimulation(OpenSim::Model const& m, SimTK::State const& s, FdParams const& p) :
    simulation{createForwardDynamicSim(m, s, p)},
    model{createInitializedModel(m)},
    spotReport{createDummySimulationReport(*this->model)},
    regularReports{} {
}

osc::UiSimulation::UiSimulation(UiModel const& uim, FdParams const& p) :
    UiSimulation{*uim.model, *uim.state, p} {
}

osc::UiSimulation::UiSimulation(UiSimulation&&) noexcept = default;
osc::UiSimulation::~UiSimulation() noexcept = default;
osc::UiSimulation& osc::UiSimulation::operator=(UiSimulation&&) noexcept = default;

std::vector<PlottableOutputSubfield> const& osc::getOutputSubfields(
        OpenSim::AbstractOutput const& ao) {

    static std::unordered_map<size_t, std::vector<osc::PlottableOutputSubfield>> const g_SubfieldLut =
            subfield_magic::createSubfieldLookup();
    static std::vector<osc::PlottableOutputSubfield> const g_EmptyResponse = {};

    size_t argTypeid = typeid(ao).hash_code();
    auto it = g_SubfieldLut.find(argTypeid);

    if (it != g_SubfieldLut.end()) {
        return it->second;
    } else {
        return g_EmptyResponse;
    }
}

osc::DesiredOutput::DesiredOutput(
        OpenSim::Component const& c,
        OpenSim::AbstractOutput const& ao) :

    absoluteComponentPath{c.getAbsolutePathString()},
    outputName{ao.getName()},
    extractorFunc{subfield_magic::extractorFunctionForOutput(ao)},
    outputTypeHashcode{typeid(ao).hash_code()} {
}

osc::DesiredOutput::DesiredOutput(
        OpenSim::Component const& c,
        OpenSim::AbstractOutput const& ao,
        PlottableOutputSubfield const& pls) :
    absoluteComponentPath{c.getAbsolutePathString()},
    outputName{ao.getName()},
    extractorFunc{pls.extractor},
    outputTypeHashcode{typeid(ao).hash_code()} {

    if (pls.parentOutputTypeHashcode != outputTypeHashcode) {
        throw std::runtime_error{"output subfield mismatch: the provided Plottable_output_field does not match the provided AbstractOutput: this is a developer error"};
    }
}
