#include "MainEditorState.hpp"

#include "src/UI/UiModelViewer.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <utility>

using namespace osc;

osc::MainEditorState::MainEditorState() :
    MainEditorState{std::make_unique<OpenSim::Model>()} {
}

osc::MainEditorState::MainEditorState(std::unique_ptr<OpenSim::Model> model) :
    MainEditorState{UndoableUiModel{std::move(model)}} {
}

osc::MainEditorState::MainEditorState(UndoableUiModel uim) :
    editedModel{uim},
    simulations{},
    focusedSimulation{-1},
    focusedSimulationScrubbingTime{-1.0f},
    desiredOutputs{},
    simParams{},
    viewers{std::make_unique<UiModelViewer>(), nullptr, nullptr, nullptr} {
}

void osc::MainEditorState::setModel(std::unique_ptr<OpenSim::Model> newModel) {
    editedModel.setModel(std::move(newModel));
}
