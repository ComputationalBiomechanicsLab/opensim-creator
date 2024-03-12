#include "UIState.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Platform/RecentFiles.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

void osc::mow::UIState::actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
{
    if (!path) {
        path = PromptUserForFile("osim");
    }

    if (path) {
        App::singleton<RecentFiles>()->push_back(*path);
        m_Document = std::make_shared<ModelWarpDocument>(std::move(path).value());
    }
}

void osc::mow::UIState::actionWarpModelAndOpenInModelEditor()
{
    if (!canWarpModel()) {
        log_error("cannot warp the provided model: there are probably errors in the input model (missing warp information, etc.)");
        return;
    }

    auto api = DynamicParentCast<IMainUIStateAPI>(m_TabHost);
    if (!api) {
        log_error("cannot warp the provided model: I can't open a model editor tab (something has gone wrong internally)");
        return;
    }

    auto m = m_ModelWarper.warp(*m_Document);
    m_TabHost->addAndSelectTab<ModelEditorTab>(*api, std::make_unique<UndoableModelStatePair>(m->getModel()));
}
