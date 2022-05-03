#pragma once

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/OpenSimBindings/Simulation.hpp"

#include <memory>
#include <optional>

namespace osc
{
    class UiModelViewer;
    class ModelMusclePlotPanel;
}

namespace OpenSim
{
    class Coordinate;
    class Model;
    class Muscle;
}

namespace osc
{
    // top-level UI state
    //
    // this is the main state that gets shared between the top-level editor
    // and simulation screens that the user is *typically* interacting with
    class MainEditorState final {
    public:
        MainEditorState();
        MainEditorState(std::unique_ptr<OpenSim::Model>);
        MainEditorState(UndoableModelStatePair);
        MainEditorState(MainEditorState const&) = delete;
        MainEditorState(MainEditorState&&);
        MainEditorState& operator=(MainEditorState const&) = delete;
        MainEditorState& operator=(MainEditorState&&);
        ~MainEditorState() noexcept;

        // model that the user is editing
        std::shared_ptr<UndoableModelStatePair> editedModel();

        // simulations
        int getNumSimulations() const;
        std::shared_ptr<Simulation> updSimulation(int);
        void addSimulation(Simulation);
        void removeSimulation(int);
        int getFocusedSimulationIndex() const;
        std::shared_ptr<Simulation> updFocusedSimulation();
        void setFocusedSimulation(int);

        // simulation params (for making new sims)
        ParamBlock const& getSimulationParams() const;
        ParamBlock& updSimulationParams();

        // output plots (user-enacted: e.g. plotting muscle activation)
        int getNumUserOutputExtractors() const;
        OutputExtractor const& getUserOutputExtractor(int) const;
        void addUserOutputExtractor(OutputExtractor);
        void removeUserOutputExtractor(int);

        int getNumViewers() const;
        UiModelViewer& updViewer(int);
        UiModelViewer& addViewer();
        void removeViewer(int);

        int getNumMusclePlots() const;
        ModelMusclePlotPanel const& getMusclePlot(int) const;
        ModelMusclePlotPanel& updMusclePlot(int);
        ModelMusclePlotPanel& addMusclePlot();
        ModelMusclePlotPanel& addMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&);
        void removeMusclePlot(int);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };

    void AutoFocusAllViewers(MainEditorState&);
    void StartSimulatingEditedModel(MainEditorState&);
    std::vector<osc::OutputExtractor> GetAllUserDesiredOutputs(MainEditorState const&);
}
