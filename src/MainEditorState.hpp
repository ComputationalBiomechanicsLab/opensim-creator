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
        bool simulationStats = true;
        bool coordinateEditor = true;
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

        UndoableUiModel const& getEditedModel() const;
        UndoableUiModel& updEditedModel();

        bool hasSimulations() const;
        int getNumSimulations() const;
        Simulation const& getSimulation(int) const;
        Simulation& updSimulation(int);
        void addSimulation(Simulation);
        void removeSimulation(int);
        Simulation* updFocusedSimulation();
        Simulation const* getFocusedSim() const;

        int getNumUserDesiredOutputs() const;
        Output const& getUserDesiredOutput(int);
        void addUserDesiredOutput(Output);

        ParamBlock const& getSimulationParams() const;
        ParamBlock& updSimulationParams();

        UserPanelPreferences const& getUserPanelPrefs() const;
        UserPanelPreferences& updUserPanelPrefs();

        std::optional<float> getUserSimulationScrubbingTime() const;
        void setUserSimulationScrubbingTime(float);
        void clearUserSimulationScrubbingTime();

        int getNumViewers() const;
        UiModelViewer& updViewer(int);
        UiModelViewer& addViewer();

        void startSimulatingEditedModel();

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
