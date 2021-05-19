#pragma once

#include "src/utils/circular_buffer.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/log.hpp"

#include <memory>
#include <chrono>
#include <optional>
#include <vector>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Model;
    class Component;
}

namespace osc {
    // a "UI-ready" OpenSim::Model with an associated (rendered) state
    //
    // this is what most of the components, screen elements, etc. are
    // accessing - usually indirectly
    struct Ui_model final {
        // the model, finalized from its properties
        std::unique_ptr<OpenSim::Model> model;

        // SimTK::State, in a renderable state (e.g. realized up to a relevant stage)
        std::unique_ptr<SimTK::State> state;

        // current selection, if any
        OpenSim::Component* selected;

        // current hover, if any
        OpenSim::Component* hovered;

        // current isolation, if any
        //
        // "isolation" here means that the user is only interested in this
        // particular subcomponent in the model
        OpenSim::Component* isolated;

        // timestamp, indicating construction/modification time
        std::chrono::system_clock::time_point timestamp;

        explicit Ui_model(std::unique_ptr<OpenSim::Model>);
        Ui_model(Ui_model const&);
        Ui_model(Ui_model const&, std::chrono::system_clock::time_point t);
        Ui_model(Ui_model&&) noexcept;
        ~Ui_model() noexcept;
        Ui_model& operator=(Ui_model const&) = delete;
        Ui_model& operator=(Ui_model&&);

        // this should be called whenever `model` is mutated
        //
        // This method updates the other members to reflect the modified model. It
        // can throw, because the modification may have put the model into an invalid
        // state that can't be used to initialize a new SimTK::MultiBodySystem or
        // SimTK::State
        void on_model_modified();
    };

    // a "UI-ready" OpenSim::Model with undo/redo and rollback support
    //
    // this is what the top-level editor screens are managing. As the user makes
    // edits to the model, the current/undo/redo states are being updated. This
    // class also has light support for handling "rollbacks", which is where the
    // implementation detects that the user modified the model into an invalid state
    // and the implementation tried to fix the problem by rolling back to an undo
    // state
    struct Undoable_ui_model final {
        Ui_model current;
        Circular_buffer<Ui_model, 32> undo;
        Circular_buffer<Ui_model, 32> redo;

        // holding space for a "damaged" model
        //
        // this is set whenever the implementation detects that the current
        // model was damaged by a modification (i.e. the model does not survive a
        // call to .initSystem with its modified properties). The implementation
        // will try to recover from damage by popping models from undo, and will
        // store the damaged model here for later cleanup
        //
        // the damaged model is kept here so that any pointers into the model are 
        // still valid. The reason this is important is because the damage may have
        // been done midway through frame draw and there may be local (stack-allocated)
        // pointers into components of the damaged model. It is *probably* safer to
        // let the drawcall finish with a damaged model than potentially segfault.
        //
        // users of this class are responsible for nuking `damaged` at an appropriate
        // time
        std::optional<Ui_model> damaged;

        explicit Undoable_ui_model(std::unique_ptr<OpenSim::Model> model);

        [[nodiscard]] constexpr bool can_undo() const noexcept {
            return !undo.empty();
        }

        void do_undo();

        [[nodiscard]] constexpr bool can_redo() const noexcept {
            return !redo.empty();
        }

        void do_redo();

        [[nodiscard]] OpenSim::Model& model() const noexcept {
            return *current.model;
        }

        void set_model(std::unique_ptr<OpenSim::Model> new_model);

        // this should be called before any modification is made to the current model
        // 
        // it gives the implementation a chance to save a known-to-be-undamaged
        // version of `current` before any potential damage happens
        void before_modifying_model();

        // this should be called after any modification is made to the current model
        //
        // it "commits" the modification by calling `on_model_modified` on the model
        // in a way that will "rollback" if comitting the change throws an exception
        void after_modifying_model();

        [[nodiscard]] OpenSim::Component* selection() noexcept {
            return current.selected;
        }

        void set_selection(OpenSim::Component* c) {
            current.selected = c;
        }

        [[nodiscard]] OpenSim::Component* hovered() noexcept {
            return current.hovered;
        }

        void set_hovered(OpenSim::Component* c) {
            current.hovered = c;
        }

        [[nodiscard]] OpenSim::Component* isolated() noexcept {
            return current.isolated;
        }

        void set_isolated(OpenSim::Component* c) {
            current.isolated = c;
        }

        [[nodiscard]] SimTK::State& state() noexcept {
            return *current.state;
        }

        void clear_any_damaged_models() {
            if (damaged) {
                log::error("destructing damaged model");
                damaged = std::nullopt;
            }            
        }
    };
    
    /*
    // a running/finished simulation in the UI
    struct Ui_simulation final {
        // sim-side: the simulation, running on a background thread
        fd::Simulation simulation;

        // UI-side: copy of the model being simulated
        std::unique_ptr<OpenSim::Model> model;

        // UI-side: spot report
        // 
        // latest (usually per-integration-step) report popped from `simulation`
        std::unique_ptr<fd::Report> spot_report;

        // UI-side: regular reports popped from the simulator
        //
        // the simulator is guaranteed to produce reports at some regular
        // interval (in simulation time).
        std::vector<std::unique_ptr<fd::Report>> regular_reports;
    };
    */

    // top-level UI state
    struct Main_editor_state final {
        // the model that the user is currently editing
        Undoable_ui_model edited_model;

        // construct with a blank OpenSim::Model
        Main_editor_state();

        // construct with an existing OpenSim::Model
        Main_editor_state(std::unique_ptr<OpenSim::Model>);

        // forward relevant methods from members (law of Demeter, or something...)

        OpenSim::Model& model() {
            return edited_model.model();
        }

        bool can_undo() {
            return edited_model.can_undo();
        }

        void do_undo() {
            edited_model.do_undo();
        }

        bool can_redo() {
            return edited_model.can_redo();
        }

        void do_redo() {
            edited_model.do_redo();
        }
        
        [[nodiscard]] OpenSim::Model& model() const noexcept {
            return edited_model.model();
        }

        void set_model(std::unique_ptr<OpenSim::Model> new_model) {
            edited_model.set_model(std::move(new_model));
        }

        void before_modifying_model() {
            edited_model.before_modifying_model();
        }

        void after_modifying_model() {
            edited_model.after_modifying_model();
        }

        [[nodiscard]] OpenSim::Component* selection() noexcept {
            return edited_model.selection();
        }

        void set_selection(OpenSim::Component* c) {
            edited_model.set_selection(c);
        }

        [[nodiscard]] OpenSim::Component* hovered() noexcept {
            return edited_model.hovered();
        }

        void set_hovered(OpenSim::Component* c) {
            edited_model.set_hovered(c);
        }

        [[nodiscard]] OpenSim::Component* isolated() noexcept {
            return edited_model.isolated();
        }

        void set_isolated(OpenSim::Component* c) {
            edited_model.set_isolated(c);
        }

        [[nodiscard]] SimTK::State& state() noexcept {
            return edited_model.state();
        }

        void clear_any_damaged_models() {
            edited_model.clear_any_damaged_models();
        }
    };
}