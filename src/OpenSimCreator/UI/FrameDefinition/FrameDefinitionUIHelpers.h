#pragma once

#include <oscar/Utils/ParentPtr.h>

#include <memory>

namespace OpenSim { class Model; }
namespace osc { class IMainUIStateAPI; }
namespace osc { class IModelStatePair; }
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
        const ParentPtr<IMainUIStateAPI>&,
        const OpenSim::Model&
    );
}
