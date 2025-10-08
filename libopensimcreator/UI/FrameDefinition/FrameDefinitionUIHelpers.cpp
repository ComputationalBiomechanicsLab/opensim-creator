#include "FrameDefinitionUIHelpers.h"

#include <libopensimcreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Graphics/SimTKMeshLoader.h>
#include <libopensimcreator/UI/ModelEditor/ModelEditorTab.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/UI/Events/OpenTabEvent.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

namespace
{
    void handleDialogResponse(
        const std::shared_ptr<IModelStatePair>& model,
        FileDialogResponse response)
    {
        if (model->isReadonly()) {
            return;
        }

        if (response.empty()) {
            return;  // user didn't select anything
        }

        // create a human-readable commit message
        const std::string commitMessage = [&response]()
        {
            if (response.size() == 1) {
                return fd::GenerateAddedSomethingCommitMessage(response.front().filename().string());
            }
            else {
                std::stringstream ss;
                ss << "added " << response.size() << " meshes";
                return std::move(ss).str();
            }
        }();

        // perform the model mutation
        OpenSim::Model& mutableModel = model->updModel();
        for (const std::filesystem::path& meshPath : response) {
            const std::string meshName = meshPath.filename().replace_extension().string();

            // add an offset frame that is connected to ground - this will become
            // the mesh's offset frame
            auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            meshPhysicalOffsetFrame->setParentFrame(model->getModel().getGround());
            meshPhysicalOffsetFrame->setName(meshName + "_offset");

            // attach the mesh to the frame
            {
                auto mesh = std::make_unique<OpenSim::Mesh>(meshPath.string());
                mesh->setName(meshName);
                AttachGeometry(*meshPhysicalOffsetFrame, std::move(mesh));
            }

            // add it to the model and select it (i.e. always select the last mesh)
            const OpenSim::PhysicalOffsetFrame& pofRef = AddModelComponent(mutableModel, std::move(meshPhysicalOffsetFrame));
            FinalizeConnections(mutableModel);
            model->setSelected(&pofRef);
        }

        model->commit(commitMessage);
        InitializeModel(mutableModel);
        InitializeState(mutableModel);
    }
}

void osc::fd::ActionPromptUserToAddMeshFiles(
    const std::shared_ptr<IModelStatePair>& model)
{
    if (model->isReadonly()) {
        return;
    }

    // Asynchronously handle the user's response
    App::upd().prompt_user_to_select_file_async(
        std::bind_front(handleDialogResponse, model),
        GetSupportedSimTKMeshFormatsAsFilters(),
        std::nullopt,
        true
    );
}

std::unique_ptr<UndoableModelStatePair> osc::fd::MakeUndoableModelFromSceneModel(
    const OpenSim::Model& sceneModel)
{
    auto modelCopy = std::make_unique<OpenSim::Model>(sceneModel);
    modelCopy->upd_ComponentSet().clearAndDestroy();
    return std::make_unique<UndoableModelStatePair>(std::move(modelCopy));
}

void osc::fd::ActionExportFrameDefinitionSceneModelToEditorTab(
    Widget& parent,
    const OpenSim::Model& model)
{
    auto tab = std::make_unique<ModelEditorTab>(&parent, MakeUndoableModelFromSceneModel(model));
    App::post_event<OpenTabEvent>(parent, std::move(tab));
}
