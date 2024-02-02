#include "FrameDefinitionUIHelpers.hpp"

#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.hpp>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <OpenSimCreator/UI/IMainUIStateAPI.hpp>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/UI/Tabs/ITabHost.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using osc::UndoableModelStatePair;

void osc::fd::ActionPromptUserToAddMeshFiles(UndoableModelStatePair& model)
{
    std::vector<std::filesystem::path> const meshPaths =
        PromptUserForFiles(GetCommaDelimitedListOfSupportedSimTKMeshFormats());
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
        log::error("Tried to export frame definition scene to an OpenSim model but there is no MainUIStateAPI data");
        return;
    }

    (*maybeMainUIStateAPI)->addAndSelectTab<ModelEditorTab>(
        *maybeMainUIStateAPI,
        MakeUndoableModelFromSceneModel(model)
    );
}
