#include "main_editor_state.hpp"

#include "src/log.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Common/Component.h>
#include <SimTKcommon.h>

#include <chrono>
#include <utility>

using std::chrono_literals::operator""s;

// translate a pointer to a component in model A to a pointer to a component in model B
//
// returns nullptr if the pointer cannot be cleanly translated
static OpenSim::Component*
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

// Ui_model impl

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
    this->selected = relocate_component_pointer_to_new_model(*model, selected);
    this->hovered = relocate_component_pointer_to_new_model(*model, hovered);
    this->isolated = relocate_component_pointer_to_new_model(*model, isolated);
    this->timestamp = std::chrono::system_clock::now();

    // note: expensive and potentially throwing
    //
    // this should be done last, so that the rest of the class is in a somewhat
    // valid state if this throws
    {
        *this->state = model->initSystem();
        model->realizePosition(*this->state);
    }
}

// Undoable_ui_model impl

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

struct Debounce_rv final {
    std::chrono::system_clock::time_point valid_at;
    bool should_undo;
};

static Debounce_rv can_push_new_undo_state_with_debounce(osc::Undoable_ui_model const& uim) {
    Debounce_rv rv;
    rv.valid_at = std::chrono::system_clock::now();
    rv.should_undo = uim.undo.empty() || uim.undo.back().timestamp + 5s <= rv.valid_at;
    return rv;
}

static void do_debounced_undo_push(osc::Undoable_ui_model& uim) {
    auto [valid_at, should_undo] = can_push_new_undo_state_with_debounce(uim);

    if (should_undo) {
        uim.undo.emplace_back(uim.current, valid_at);
        uim.redo.clear();
    }
}

static void carefully_try_init_system_and_realize_on_current(osc::Undoable_ui_model& uim) {
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
        osc::log::error("exception thrown when initializing updated model: %s", ex.what());
    }

    if (uim.undo.empty()) {
        osc::log::error("the model cannot be fixed: no earlier versions of the model exist, throwing an exception");
        throw std::runtime_error{
            "an OpenSim::Model was put into an invalid state: probably by a modification. We tried to recover from this error, but couldn't - view the logs"};
    }

    // otherwise, we have undo entries we can pop
    osc::log::error(
        "attempting to rollback to an earlier (pre-modification) of the model that was saved into the undo buffer");
    try {
        uim.damaged.emplace(std::move(uim.current));
        uim.current = uim.undo.pop_back();
        return;
    } catch (...) {
        osc::log::error(
            "error encountered when trying to rollback to an earlier version of the model, this will be thrown as an exception");
        throw;
    }
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

static osc::fd::Simulation create_fd_sim(OpenSim::Model const& m, SimTK::State const& s, osc::fd::Params const& p) {
    auto model_copy = std::make_unique<OpenSim::Model>(m);
    auto state_copy = std::make_unique<SimTK::State>(s);

    model_copy->initSystem();
    model_copy->setPropertiesFromState(*state_copy);
    model_copy->realizePosition(*state_copy);
    model_copy->equilibrateMuscles(*state_copy);
    model_copy->realizeAcceleration(*state_copy);

    auto sim_input = std::make_unique<osc::fd::Input>(std::move(model_copy), std::move(state_copy));
    sim_input->params = p;

    return osc::fd::Simulation{std::move(sim_input)};
}

static std::unique_ptr<OpenSim::Model> create_initialized_model(OpenSim::Model const& m) {
    auto rv = std::make_unique<OpenSim::Model>(m);
    rv->finalizeFromProperties();
    rv->initSystem();
    return rv;
}

static std::unique_ptr<osc::fd::Report> create_dummy_report(OpenSim::Model const& m) {
    auto rv = std::make_unique<osc::fd::Report>();
    rv->state = m.getWorkingState();
    rv->stats = {};
    m.realizeReport(rv->state);

    return rv;
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

osc::Main_editor_state::Main_editor_state() : 
    edited_model{std::make_unique<OpenSim::Model>()} {
}

osc::Main_editor_state::Main_editor_state(std::unique_ptr<OpenSim::Model> model) :
    edited_model{std::move(model)} {
}

void osc::Main_editor_state::set_model(std::unique_ptr<OpenSim::Model> new_model) {
    edited_model.set_model(std::move(new_model));
}
