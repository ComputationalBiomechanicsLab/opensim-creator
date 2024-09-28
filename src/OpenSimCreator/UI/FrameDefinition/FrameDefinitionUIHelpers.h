#pragma once

#include <memory>

namespace OpenSim { class Model; }
namespace osc { class IModelStatePair; }
namespace osc { class MainUIScreen; }
namespace osc { class UndoableModelStatePair; }

namespace osc::fd
{
    void ActionPromptUserToAddMeshFiles(
        IModelStatePair&
    );

    std::unique_ptr<UndoableModelStatePair> MakeUndoableModelFromSceneModel(
        const OpenSim::Model& sceneModel
    );

    void ActionExportFrameDefinitionSceneModelToEditorTab(
        MainUIScreen&,
        const OpenSim::Model&
    );
}
