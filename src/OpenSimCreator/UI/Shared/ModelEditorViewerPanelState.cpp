#include "ModelEditorViewerPanelState.h"

#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Platform/App.h>

osc::ModelEditorViewerPanelState::ModelEditorViewerPanelState(
    std::string_view panelName_) :

    panel_name_{panelName_},
    m_CachedModelRenderer
    {
        App::singleton<SceneCache>(App::resource_loader()),
    }
{
}
