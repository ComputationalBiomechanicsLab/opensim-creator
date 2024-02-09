#pragma once

#include <oscar/Utils/ParentPtr.h>

#include <memory>

namespace osc { class ITabHost; }
namespace osc { class UndoableModelStatePair; }

namespace osc::fd
{
    void ActionPromptUserToAddMeshFiles(
        UndoableModelStatePair&
    );

    std::unique_ptr<UndoableModelStatePair> MakeUndoableModelFromSceneModel(
        UndoableModelStatePair const& sceneModel
    );

    void ActionExportFrameDefinitionSceneModelToEditorTab(
        ParentPtr<ITabHost> const&,
        UndoableModelStatePair const&
    );
}
