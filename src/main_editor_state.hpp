#pragma once

#include "src/opensim_bindings/ui_types.hpp"
#include "src/opensim_bindings/simulation.hpp"
#include "src/ui/component_3d_viewer.hpp"

#include <array>
#include <vector>
#include <memory>

namespace osc {

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

        // available 3D viewers
        //
        // the user can open a limited number of 3D viewers. They are kept on
        // this top-level state so that they, and their settings, can be
        // cached between screens
        //
        // the viewers can be null, which should be interpreted as "not yet initialized"
        std::array<std::unique_ptr<Component_3d_viewer>, 4> viewers;

        // which panels should be shown
        //
        // the user can enable/disable these in the "window" entry in the main menu
        struct {
            bool actions = true;
            bool hierarchy = true;
            bool log = true;
            bool outputs = true;
            bool property_editor = true;
            bool selection_details = true;
            bool simulations = true;
            bool simulation_stats = true;
        } showing;

        // construct with a blank (new) OpenSim::Model
        Main_editor_state();

        // construct with an existing OpenSim::Model
        Main_editor_state(std::unique_ptr<OpenSim::Model>);

        Main_editor_state(Main_editor_state const&) = delete;
        Main_editor_state(Main_editor_state&&) = delete;

        Main_editor_state& operator=(Main_editor_state const&) = delete;
        Main_editor_state& operator=(Main_editor_state&&) = delete;

        [[nodiscard]] OpenSim::Model& model() noexcept {
            return edited_model.model();
        }

        [[nodiscard]] bool can_undo() const noexcept {
            return edited_model.can_undo();
        }

        void do_undo() {
            edited_model.do_undo();
        }

        [[nodiscard]] bool can_redo() const noexcept {
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

        [[nodiscard]] Ui_simulation* get_focused_sim() noexcept {
            if (!(0 <= focused_simulation && focused_simulation < static_cast<int>(simulations.size()))) {
                return nullptr;
            } else {
                return simulations[static_cast<size_t>(focused_simulation)].get();
            }
        }

        [[nodiscard]] Ui_simulation const* get_focused_sim() const noexcept {
            return const_cast<Main_editor_state*>(this)->get_focused_sim();
        }

        [[nodiscard]] bool has_simulations() const noexcept {
            return !simulations.empty();
        }
    };
}
