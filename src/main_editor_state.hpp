#pragma once

#include "src/utils/circular_buffer.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/simulation.hpp"

#include <memory>
#include <chrono>
#include <optional>
#include <string>

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
    // accessing - usually indirectly (e.g. via a reference to the Model)
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
        // particular subcomponent in the model, so visualizers etc. should
        // try to only show that component
        OpenSim::Component* isolated;

        // generic timestamp
        //
        // can indicate creation or latest modification, it's here to roughly
        // track how old/new the instance is
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
    // and the implementation tried to fix the problem by rolling back to an earlier
    // (hopefully, valid) undo state
    struct Undoable_ui_model final {
        Ui_model current;
        Circular_buffer<Ui_model, 32> undo;
        Circular_buffer<Ui_model, 32> redo;

        // holding space for a "damaged" model
        //
        // this is set whenever the implementation detects that the current
        // model was damaged by a modification (i.e. the model does not survive a
        // call to .initSystem with its modified properties).
        //
        // The implementation will try to recover from the damage by popping models
        // from the undo buffer and making them `current`. It will then store the
        // damaged model here for later cleanup (by the user of this class, which
        // should `std::move` out the damaged instance)
        //
        // the damaged model is kept "alive" so that any pointers into the model are
        // still valid. The reason this is important is because the damage may have
        // been done midway through a larger process (e.g. rendering) and there may
        // be local (stack-allocated) pointers into the damaged model's components.
        // In that case, it is *probably* safer to let that process finish with a
        // damaged model than potentially segfault.
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

        // tries to rollback the model to an earlier state, throwing an exception if
        // that isn't possible (e.g. because there are no earlier states)
        void forcibly_rollback_to_earlier_state();

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

        // declare the death of a component pointer
        //
        // this happens when we know that OpenSim has destructed a component in
        // the model indirectly (e.g. it was destructed by an OpenSim container)
        // and that we want to ensure the pointer isn't still held by this state
        void declare_death_of(OpenSim::Component const* c) noexcept {
            if (current.selected == c) {
                current.selected = nullptr;
            }

            if (current.hovered == c) {
                current.hovered = nullptr;
            }

            if (current.isolated == c) {
                current.isolated = nullptr;
            }
        }
    };

    // a forward-dynamic simulation
    //
    // the simulation's computation runs on a background thread, but
    // this struct also contains information that is kept UI-side for
    // UI feedback/interaction
    struct Ui_simulation final {

        // the simulation, running on a background thread
        fd::Simulation simulation;

        // copy of the model being simulated in the bg thread
        std::unique_ptr<OpenSim::Model> model;

        // current user selection, if any
        OpenSim::Component* selected = nullptr;

        // current user hover, if any
        OpenSim::Component* hovered = nullptr;

        // latest (usually per-integration-step) report popped from
        // the bg thread
        std::unique_ptr<fd::Report> spot_report;

        // regular reports that are popped from the simulator thread by
        // the (polling) UI thread
        std::vector<std::unique_ptr<fd::Report>> regular_reports;

        // start a new simulation by *copying* the provided OpenSim::Model and
        // SimTK::State pair
        Ui_simulation(OpenSim::Model const&, SimTK::State const&, fd::Params const&);

        // start a new simulation by *copying* the provided Ui_model
        Ui_simulation(Ui_model const&, fd::Params const&);
    };

    // the path + name of an OpenSim::AbstractOutput that the user has
    // expressed interest in
    struct Desired_output final {
        std::string component_path;
        std::string output_name;

        Desired_output(std::string cp, std::string on) :
            component_path{std::move(cp)},
            output_name{std::move(on)} {
        }
    };

    // top-level UI state
    //
    // this is the main state that gets shared between the top-level editor
    // and simulation screens that the user is *typically* interacting with
    struct Main_editor_state final {

        // the model that the user is currently editing
        Undoable_ui_model edited_model;

        // running/finished simulations
        //
        // the models being simulated are separate from the model being edited
        std::vector<std::unique_ptr<Ui_simulation>> simulations;

        // currently-focused simulation
        int focused_simulation = -1;

        // simulation time the user is scrubbed to - if they have scrubbed to
        // a specific time
        //
        // if the scrubbing time doesn't fall within the currently-available
        // simulation states ("frames") then the implementation will just use
        // the latest available time
        float focused_simulation_scrubbing_time = -1.0f;

        // the model outputs that the user has expressed interest in
        //
        // e.g. the user may want to plot `force` from a model's muscle
        std::vector<Desired_output> desired_outputs;

        // parameters used when launching a new simulation
        //
        // these are the params that are used whenever a user hits "simulate"
        fd::Params sim_params;

        // construct with a blank (new) OpenSim::Model
        Main_editor_state();

        // construct with an existing OpenSim::Model
        Main_editor_state(std::unique_ptr<OpenSim::Model>);


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

        void set_model(std::unique_ptr<OpenSim::Model> new_model);

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

        void start_simulating_edited_model() {
            int new_focus = static_cast<int>(simulations.size());
            simulations.emplace_back(new Ui_simulation{edited_model.current, sim_params});
            focused_simulation = new_focus;
            focused_simulation_scrubbing_time = -1.0f;
        }

        Ui_simulation* get_focused_sim() noexcept {
            if (!(0 <= focused_simulation && focused_simulation < static_cast<int>(simulations.size()))) {
                return nullptr;
            } else {
                return simulations[static_cast<size_t>(focused_simulation)].get();
            }
        }

        Ui_simulation const* get_focused_sim() const noexcept {
            return const_cast<Main_editor_state*>(this)->get_focused_sim();
        }
    };
}
