#include "UIState.h"

#include <OpenSimCreator/Platform/RecentFiles.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>

#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Events/OpenTabEvent.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

void osc::mow::UIState::actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
{
    if (not path) {
        path = prompt_user_to_select_file({"osim"});
    }

    if (path) {
        App::singleton<RecentFiles>()->push_back(*path);
        m_Document = std::make_shared<WarpableModel>(std::move(path).value());
    }
}

void osc::mow::UIState::actionWarpModelAndOpenInModelEditor()
{
    if (not canWarpModel()) {
        log_error("cannot warp the provided model: there are probably errors in the input model (missing warp information, etc.)");
        return;
    }

    // create a copy of the document so that we can apply export-specific
    // configuration changes to it
    WarpableModel copy{*m_Document};
    copy.setShouldWriteWarpedMeshesToDisk(true);  // required for OpenSim to be able to load the warped model correctly
    auto warpedModelStatePair = m_ModelWarper.warp(copy);

    auto editor = std::make_unique<ModelEditorTab>(*m_Parent, warpedModelStatePair->getModel());
    App::post_event<OpenTabEvent>(*m_Parent, std::move(editor));
}
