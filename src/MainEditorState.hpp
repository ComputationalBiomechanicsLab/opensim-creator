#pragma once

#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/OpenSimBindings/UiSimulation.hpp"
#include "src/OpenSimBindings/PlottableOutputSubfield.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/UI/UiModelViewer.hpp"

#include <array>
#include <vector>
#include <memory>

namespace osc {

    // top-level UI state
    //
    // this is the main state that gets shared between the top-level editor
    // and simulation screens that the user is *typically* interacting with
    struct MainEditorState final {

        // the model that the user is currently editing
        UndoableUiModel editedModel;

        // running/finished simulations
        //
        // the models being simulated are separate from the model being edited
        std::vector<std::unique_ptr<UiSimulation>> simulations;

        // currently-focused simulation
        int focusedSimulation = -1;

        // simulation time the user is scrubbed to - if they have scrubbed to
        // a specific time
        //
        // if the scrubbing time doesn't fall within the currently-available
        // simulation states ("frames") then the implementation will just use
        // the latest available time
        float focusedSimulationScrubbingTime = -1.0f;

        // the model outputs that the user has expressed interest in
        //
        // e.g. the user may want to plot `force` from a model's muscle
        std::vector<DesiredOutput> desiredOutputs;

        // parameters used when launching a new simulation
        //
        // these are the params that are used whenever a user hits "simulate"
        FdParams simParams;

        // available 3D viewers
        //
        // the user can open a limited number of 3D viewers. They are kept on
        // this top-level state so that they, and their settings, can be
        // cached between screens
        //
        // the viewers can be null, which should be interpreted as "not yet initialized"
        std::array<std::unique_ptr<UiModelViewer>, 4> viewers;

        // which panels should be shown
        //
        // the user can enable/disable these in the "window" entry in the main menu
        struct {
            bool actions = true;
            bool hierarchy = true;
            bool log = true;
            bool outputs = true;
            bool propertyEditor = true;
            bool selectionDetails = true;
            bool simulations = true;
            bool simulationStats = true;
            bool coordinateEditor = true;
        } showing;

        MainEditorState();
        MainEditorState(std::unique_ptr<OpenSim::Model>);
        MainEditorState(UndoableUiModel);
        MainEditorState(MainEditorState const&) = delete;
        MainEditorState(MainEditorState&&) = delete;

        MainEditorState& operator=(MainEditorState const&) = delete;
        MainEditorState& operator=(MainEditorState&&) = delete;

        OpenSim::Model& updModel() noexcept {
            return editedModel.updModel();
        }

        OpenSim::Model const& getModel() const noexcept {
            return editedModel.getModel();
        }

        SimTK::State const& getState() const noexcept {
            return editedModel.getState();
        }

        SimTK::State& updState() noexcept {
            return editedModel.updState();
        }

        [[nodiscard]] bool canUndo() const noexcept {
            return editedModel.canUndo();
        }

        void doUndo() {
            editedModel.doUndo();
        }

        [[nodiscard]] bool canRedo() const noexcept {
            return editedModel.canRedo();
        }

        void doRedo() {
            editedModel.doRedo();
        }



        void setModel(std::unique_ptr<OpenSim::Model> new_model);

        void beforeModifyingModel() {
            editedModel.beforeModifyingModel();
        }

        void afterModifyingModel() {
            editedModel.afterModifyingModel();
        }


        OpenSim::Component const* getSelected() const noexcept {
            return editedModel.getSelected();
        }

        OpenSim::Component* updSelected() noexcept {
            return editedModel.updSelected();
        }

        void setSelected(OpenSim::Component const* c) {
            editedModel.setSelected(c);
        }


        OpenSim::Component const* getHovered() const noexcept {
            return editedModel.getHovered();
        }

        OpenSim::Component* updHovered() {
            return editedModel.updHovered();
        }

        void setHovered(OpenSim::Component const* c) {
            editedModel.setHovered(c);
        }


        OpenSim::Component const* getIsolated() const noexcept {
            return editedModel.getIsolated();
        }

        OpenSim::Component* updIsolated() {
            return editedModel.updIsolated();
        }

        void setIsolated(OpenSim::Component const* c) {
            editedModel.setIsolated(c);
        }


        void clearAnyDamagedModels() {
            editedModel.clearAnyDamagedModels();
        }

        void startSimulatingEditedModel() {
            int newFocus = static_cast<int>(simulations.size());
            simulations.emplace_back(new UiSimulation{editedModel.current, simParams});
            focusedSimulation = newFocus;
            focusedSimulationScrubbingTime = -1.0f;
        }

        [[nodiscard]] UiSimulation* getFocusedSim() noexcept {
            if (!(0 <= focusedSimulation && focusedSimulation < static_cast<int>(simulations.size()))) {
                return nullptr;
            } else {
                return simulations[static_cast<size_t>(focusedSimulation)].get();
            }
        }

        [[nodiscard]] UiSimulation const* getFocusedSim() const noexcept {
            return const_cast<MainEditorState*>(this)->getFocusedSim();
        }

        [[nodiscard]] bool hasSimulations() const noexcept {
            return !simulations.empty();
        }
    };
}
