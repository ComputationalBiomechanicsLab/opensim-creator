#include "MainEditorState.hpp"

#include "src/UI/Component3DViewer.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <utility>

using namespace osc;

static std::unique_ptr<Component3DViewer> create3DViewer() {
    return std::make_unique<osc::Component3DViewer>(osc::Component3DViewerFlags_Default | osc::Component3DViewerFlags_DrawFrames);
}

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
    viewers{create3DViewer(), nullptr, nullptr, nullptr} {
}

void osc::MainEditorState::setModel(std::unique_ptr<OpenSim::Model> newModel) {
    editedModel.setModel(std::move(newModel));
}
