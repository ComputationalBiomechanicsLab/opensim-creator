#include "ui_types.hpp"

#include "src/log.hpp"
#include "src/opensim_bindings/simulation.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/Component.h>
#include <SimTKcommon.h>

#include <chrono>
#include <utility>

using namespace osc;
using std::chrono_literals::operator""s;

namespace {
    // translate a pointer to a component in model A to a pointer to a component in model B
    //
    // returns nullptr if the pointer cannot be cleanly translated
    OpenSim::Component*
        relocate_component_pointer_to_new_model(OpenSim::Model const& model, OpenSim::Component* ptr) {

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

    struct Debounce_rv final {
        std::chrono::system_clock::time_point valid_at;
        bool should_undo;
    };

    Debounce_rv can_push_new_undo_state_with_debounce(Undoable_ui_model const& uim) {
        Debounce_rv rv;
        rv.valid_at = std::chrono::system_clock::now();
        rv.should_undo = uim.undo.empty() || uim.undo.back().timestamp + 5s <= rv.valid_at;
        return rv;
    }

    void do_debounced_undo_push(Undoable_ui_model& uim) {
        auto [valid_at, should_undo] = can_push_new_undo_state_with_debounce(uim);

        if (should_undo) {
            uim.undo.emplace_back(uim.current, valid_at);
            uim.redo.clear();
        }
    }

    void rollback_model_to_earlier_state(Undoable_ui_model& uim) {
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

    void carefully_try_init_system_and_realize_on_current(Undoable_ui_model& uim) {
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
            uim.current.on_model_modified();
            return;
        } catch (std::exception const& ex) {
            log::error("exception thrown when initializing updated model: %s", ex.what());
        }

        rollback_model_to_earlier_state(uim);
    }

    std::unique_ptr<fd::Simulation> create_fd_sim(OpenSim::Model const& m, SimTK::State const& s, fd::Params const& p) {
        auto model_copy = std::make_unique<OpenSim::Model>(m);
        auto state_copy = std::make_unique<SimTK::State>(s);

        model_copy->initSystem();
        model_copy->setPropertiesFromState(*state_copy);
        model_copy->realizePosition(*state_copy);
        model_copy->equilibrateMuscles(*state_copy);
        model_copy->realizeAcceleration(*state_copy);

        auto sim_input = std::make_unique<fd::Input>(std::move(model_copy), std::move(state_copy));
        sim_input->params = p;

        return std::make_unique<fd::Simulation>(std::move(sim_input));
    }

    std::unique_ptr<OpenSim::Model> create_initialized_model(OpenSim::Model const& m) {
        auto rv = std::make_unique<OpenSim::Model>(m);
        rv->finalizeFromProperties();
        rv->initSystem();
        return rv;
    }

    std::unique_ptr<fd::Report> create_dummy_report(OpenSim::Model const& m) {
        auto rv = std::make_unique<fd::Report>();
        rv->state = m.getWorkingState();
        rv->stats = {};
        m.realizeReport(rv->state);

        return rv;
    }

    std::unique_ptr<Component_3d_viewer> create_3dviewer() {
        return std::make_unique<osc::Component_3d_viewer>(osc::Component3DViewerFlags_Default | osc::Component3DViewerFlags_DrawFrames);
    }

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
    double extract_erased(OpenSim::AbstractOutput const& o, SimTK::State const& s) {
        return extract<>(dynamic_cast<OutputType const&>(o), s);
    }

    // type-erase a concrete *subfield* extractor function
    template<Subfield_ sf, typename OutputType>
    double extract_erased(OpenSim::AbstractOutput const& o, SimTK::State const& s) {
        return extract<sf>(dynamic_cast<OutputType const&>(o), s);
    }

    // helper function that wires the above together for the lookup
    template<Subfield_ sf, typename TValue>
    Plottable_output_subfield subfield(char const* name) {
        return {name, extract_erased<sf, OpenSim::Output<TValue>>, typeid(OpenSim::Output<TValue>).hash_code()};
    }

    // create a constant-time lookup for an OpenSim::Output<T>'s available subfields
    static std::unordered_map<size_t, std::vector<osc::Plottable_output_subfield>> create_subfield_lut() {
        std::unordered_map<size_t, std::vector<osc::Plottable_output_subfield>> rv;

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
    static extrator_fn_t extractor_fn_for(OpenSim::AbstractOutput const& ao) {
        auto const* dp = dynamic_cast<OpenSim::Output<double> const*>(&ao);
        if (dp) {
            return extract_erased<OpenSim::Output<double>>;
        } else {
            return nullptr;
        }
    }
}


// public API

osc::Ui_model::Ui_model(std::unique_ptr<OpenSim::Model> _model) :
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

osc::Ui_model::Ui_model(Ui_model const& other, std::chrono::system_clock::time_point t) :
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
    selected{relocate_component_pointer_to_new_model(*model, other.selected)},
    hovered{relocate_component_pointer_to_new_model(*model, other.hovered)},
    isolated{relocate_component_pointer_to_new_model(*model, other.isolated)},
    timestamp{t} {
}

osc::Ui_model::Ui_model(Ui_model const& other) : Ui_model{other, std::chrono::system_clock::now()} {
}

osc::Ui_model::Ui_model(Ui_model&&) noexcept = default;

osc::Ui_model::~Ui_model() noexcept = default;

osc::Ui_model& osc::Ui_model::operator=(Ui_model&&) = default;

void osc::Ui_model::on_model_modified() {
    *this->state = model->initSystem();
    model->equilibrateMuscles(*this->state);
    model->realizePosition(*this->state);
    this->timestamp = std::chrono::system_clock::now();
}


osc::Undoable_ui_model::Undoable_ui_model(std::unique_ptr<OpenSim::Model> model) :
    current{std::move(model)}, undo{}, redo{}, damaged{std::nullopt} {
}

void osc::Undoable_ui_model::do_undo() {
    if (!can_undo()) {
        return;
    }

    redo.emplace_back(std::move(current));
    current = undo.pop_back();
}

void osc::Undoable_ui_model::do_redo() {
    if (!can_redo()) {
        return;
    }

    undo.emplace_back(std::move(current));
    current = redo.pop_back();
}

void osc::Undoable_ui_model::set_model(std::unique_ptr<OpenSim::Model> new_model) {
    // care: this step can throw, because it initializes a system etc.
    //       so, do this *before* potentially breaking these sequecnes
    Ui_model new_current{std::move(new_model)};

    undo.emplace_back(std::move(current));
    redo.clear();
    current = std::move(new_current);
}

void osc::Undoable_ui_model::before_modifying_model() {
    osc::log::debug("starting model modification");
    do_debounced_undo_push(*this);
}

void osc::Undoable_ui_model::after_modifying_model() {
    osc::log::debug("ended model modification");
    carefully_try_init_system_and_realize_on_current(*this);
}

void osc::Undoable_ui_model::forcibly_rollback_to_earlier_state() {
    rollback_model_to_earlier_state(*this);
}

void osc::Undoable_ui_model::clear_any_damaged_models() {
    if (damaged) {
        log::error("destructing damaged model");
        damaged = std::nullopt;
    }
}

osc::Ui_simulation::Ui_simulation(OpenSim::Model const& m, SimTK::State const& s, fd::Params const& p) :
    simulation{create_fd_sim(m, s, p)},
    model{create_initialized_model(m)},
    spot_report{create_dummy_report(*this->model)},
    regular_reports{} {
}

osc::Ui_simulation::Ui_simulation(Ui_model const& uim, fd::Params const& p) :
    Ui_simulation{*uim.model, *uim.state, p} {
}

osc::Ui_simulation::Ui_simulation(Ui_simulation&&) noexcept = default;
osc::Ui_simulation::~Ui_simulation() noexcept = default;
osc::Ui_simulation& osc::Ui_simulation::operator=(Ui_simulation&&) noexcept = default;

std::vector<Plottable_output_subfield> const& osc::get_subfields(
        OpenSim::AbstractOutput const& ao) {

    static std::unordered_map<size_t, std::vector<osc::Plottable_output_subfield>> const g_SubfieldLut =
            subfield_magic::create_subfield_lut();
    static std::vector<osc::Plottable_output_subfield> const g_EmptyResponse = {};

    size_t arg_typeid = typeid(ao).hash_code();
    auto it = g_SubfieldLut.find(arg_typeid);

    if (it != g_SubfieldLut.end()) {
        return it->second;
    } else {
        return g_EmptyResponse;
    }
}

osc::Desired_output::Desired_output(
        OpenSim::Component const& c,
        OpenSim::AbstractOutput const& ao) :

    component_path{c.getAbsolutePathString()},
    output_name{ao.getName()},
    extractor{subfield_magic::extractor_fn_for(ao)},
    typehash{typeid(ao).hash_code()} {
}

osc::Desired_output::Desired_output(
        OpenSim::Component const& c,
        OpenSim::AbstractOutput const& ao,
        Plottable_output_subfield const& pls) :
    component_path{c.getAbsolutePathString()},
    output_name{ao.getName()},
    extractor{pls.extractor},
    typehash{typeid(ao).hash_code()} {

    if (pls.parent_typehash != typehash) {
        throw std::runtime_error{"output subfield mismatch: the provided Plottable_output_field does not match the provided AbstractOutput: this is a developer error"};
    }
}
