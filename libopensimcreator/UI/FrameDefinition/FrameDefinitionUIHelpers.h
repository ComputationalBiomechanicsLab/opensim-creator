#pragma once

#include <memory>

namespace OpenSim { class Model; }
namespace osc { class IModelStatePair; }
namespace osc { class UndoableModelStatePair; }
namespace osc { class Widget; }

namespace osc::fd
{
    void ActionPromptUserToAddMeshFiles(
        std::shared_ptr<IModelStatePair>
    );

    std::unique_ptr<UndoableModelStatePair> MakeUndoableModelFromSceneModel(
        const OpenSim::Model& sceneModel
    );

    void ActionExportFrameDefinitionSceneModelToEditorTab(
        Widget&,
        const OpenSim::Model&
    );
}
