#include "UIState.h"

#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/UI/ModelEditor/ModelEditorTab.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/Documents/FileFilters.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/Events/OpenTabEvent.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

void osc::mow::UIState::actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
{
    if (not path) {
        App::upd().prompt_user_to_select_file_async(
            [document = m_Document](FileDialogResponse response)
            {
                if (response.size() != 1) {
                    return;  // Error, cancellation, or the user somehow selected >1 file.
                }
                App::singleton<RecentFiles>()->push_back(response.front());
                WarpableModel loadedModel{response.front()};
                *document = std::move(loadedModel);
            },
            GetModelFileFilters()
        );
    }
    else {
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

    auto editor = std::make_unique<ModelEditorTab>(m_Parent, warpedModelStatePair->getModel());
    App::post_event<OpenTabEvent>(*m_Parent, std::move(editor));
}
