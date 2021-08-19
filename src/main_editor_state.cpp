#include "main_editor_state.hpp"

#include "src/log.hpp"
#include "src/ui/component_3d_viewer.hpp"

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

    fd::Simulation create_fd_sim(OpenSim::Model const& m, SimTK::State const& s, fd::Params const& p) {
        auto model_copy = std::make_unique<OpenSim::Model>(m);
        auto state_copy = std::make_unique<SimTK::State>(s);

        model_copy->initSystem();
        model_copy->setPropertiesFromState(*state_copy);
        model_copy->realizePosition(*state_copy);
        model_copy->equilibrateMuscles(*state_copy);
        model_copy->realizeAcceleration(*state_copy);

        auto sim_input = std::make_unique<fd::Input>(std::move(model_copy), std::move(state_copy));
        sim_input->params = p;

        return fd::Simulation{std::move(sim_input)};
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


osc::Main_editor_state::Main_editor_state() :
    Main_editor_state{std::make_unique<OpenSim::Model>()} {
}

osc::Main_editor_state::Main_editor_state(std::unique_ptr<OpenSim::Model> model) :
    edited_model{std::move(model)},
    simulations{},
    focused_simulation{-1},
    focused_simulation_scrubbing_time{-1.0f},
    desired_outputs{},
    sim_params{},
    viewers{create_3dviewer(), nullptr, nullptr, nullptr} {
}

void osc::Main_editor_state::set_model(std::unique_ptr<OpenSim::Model> new_model) {
    edited_model.set_model(std::move(new_model));
}
