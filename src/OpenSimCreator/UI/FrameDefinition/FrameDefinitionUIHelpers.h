#pragma once
#include <OpenSimCreator/UI/MainUIScreen.h>


#include <oscar/Utils/ParentPtr.h>

#include <memory>

namespace OpenSim { class Model; }
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
        const ParentPtr<MainUIScreen>&,
        const OpenSim::Model&
    );
}
