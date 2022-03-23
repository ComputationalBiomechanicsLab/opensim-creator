#pragma once

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/Output.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/OpenSimBindings/Simulation.hpp"

#include <memory>
#include <optional>

namespace osc
{
    class UiModelViewer;
}

namespace OpenSim
{
    class Model;
}

namespace osc
{
    // which panels should be shown?
    struct UserPanelPreferences final {
        bool actions = true;
        bool hierarchy = true;
        bool log = true;
        bool outputs = true;
        bool propertyEditor = true;
        bool selectionDetails = true;
        bool simulations = true;
        bool simulationStats = false;
        bool coordinateEditor = true;
        bool perfPanel = false;
    };

    // top-level UI state
    //
    // this is the main state that gets shared between the top-level editor
    // and simulation screens that the user is *typically* interacting with
    class MainEditorState final {
    public:
        MainEditorState();
        MainEditorState(std::unique_ptr<OpenSim::Model>);
        MainEditorState(UndoableUiModel);
        MainEditorState(MainEditorState const&) = delete;
        MainEditorState(MainEditorState&&);
        MainEditorState& operator=(MainEditorState const&) = delete;
        MainEditorState& operator=(MainEditorState&&);
        ~MainEditorState() noexcept;

        // edited model
        UndoableUiModel const& getEditedModel() const;
        UndoableUiModel& updEditedModel();
        std::shared_ptr<UndoableUiModel> updEditedModelPtr();

        // active simulations
        bool hasSimulations() const;
        int getNumSimulations() const;
        Simulation const& getSimulation(int) const;
        Simulation& updSimulation(int);
        void addSimulation(Simulation);
        void removeSimulation(int);
        int getFocusedSimulationIndex() const;
        Simulation const* getFocusedSimulation() const;
        Simulation* updFocusedSimulation();
        void setFocusedSimulation(int);

        // simulation params (for making new sims)
        ParamBlock const& getSimulationParams() const;
        ParamBlock& updSimulationParams();

        // output plots (user-enacted)
        int getNumUserDesiredOutputs() const;
        Output const& getUserDesiredOutput(int) const;
        void addUserDesiredOutput(Output);
        void removeUserDesiredOutput(int);

        // active ImGui panels
        UserPanelPreferences const& getUserPanelPrefs() const;
        UserPanelPreferences& updUserPanelPrefs();

        int getNumViewers() const;
        UiModelViewer& updViewer(int);
        UiModelViewer& addViewer();
        void removeViewer(int);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };

    void AutoFocusAllViewers(MainEditorState&);
    void StartSimulatingEditedModel(MainEditorState&);
    std::vector<osc::Output> GetAllUserDesiredOutputs(MainEditorState const&);
}
