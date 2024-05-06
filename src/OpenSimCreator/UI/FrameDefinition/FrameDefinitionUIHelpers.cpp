#include "FrameDefinitionUIHelpers.h"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace osc;

void osc::fd::ActionPromptUserToAddMeshFiles(UndoableModelStatePair& model)
{
    std::vector<std::filesystem::path> const meshPaths =
        prompt_user_to_select_files(GetSupportedSimTKMeshFormats());
    if (meshPaths.empty())
    {
        return;  // user didn't select anything
    }

    // create a human-readable commit message
    std::string const commitMessage = [&meshPaths]()
    {
        if (meshPaths.size() == 1)
        {
            return GenerateAddedSomethingCommitMessage(meshPaths.front().filename().string());
        }
        else
        {
            std::stringstream ss;
            ss << "added " << meshPaths.size() << " meshes";
            return std::move(ss).str();
        }
    }();

    // perform the model mutation
    OpenSim::Model& mutableModel = model.updModel();
    for (std::filesystem::path const& meshPath : meshPaths)
    {
        std::string const meshName = meshPath.filename().replace_extension().string();

        // add an offset frame that is connected to ground - this will become
        // the mesh's offset frame
        auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysicalOffsetFrame->setParentFrame(model.getModel().getGround());
        meshPhysicalOffsetFrame->setName(meshName + "_offset");

        // attach the mesh to the frame
        {
            auto mesh = std::make_unique<OpenSim::Mesh>(meshPath.string());
            mesh->setName(meshName);
            AttachGeometry(*meshPhysicalOffsetFrame, std::move(mesh));
        }

        // add it to the model and select it (i.e. always select the last mesh)
        OpenSim::PhysicalOffsetFrame const& pofRef = AddModelComponent(mutableModel, std::move(meshPhysicalOffsetFrame));
        FinalizeConnections(mutableModel);
        model.setSelected(&pofRef);
    }

    model.commit(commitMessage);
    InitializeModel(mutableModel);
    InitializeState(mutableModel);
}

std::unique_ptr<UndoableModelStatePair> osc::fd::MakeUndoableModelFromSceneModel(
    UndoableModelStatePair const& sceneModel)
{
    auto modelCopy = std::make_unique<OpenSim::Model>(sceneModel.getModel());
    modelCopy->upd_ComponentSet().clearAndDestroy();
    return std::make_unique<UndoableModelStatePair>(std::move(modelCopy));
}

void osc::fd::ActionExportFrameDefinitionSceneModelToEditorTab(
    ParentPtr<ITabHost> const& tabHost,
    UndoableModelStatePair const& model)
{
    auto maybeMainUIStateAPI = DynamicParentCast<IMainUIStateAPI>(tabHost);
    if (!maybeMainUIStateAPI)
    {
        log_error("Tried to export frame definition scene to an OpenSim model but there is no MainUIStateAPI data");
        return;
    }

    (*maybeMainUIStateAPI)->add_and_select_tab<ModelEditorTab>(
        *maybeMainUIStateAPI,
        MakeUndoableModelFromSceneModel(model)
    );
}
