#include "ModelEditorViewerPanelState.h"

#include <oscar/Platform/App.h>
#include <oscar/Scene/SceneCache.h>
#include <oscar/Scene/ShaderCache.h>

osc::ModelEditorViewerPanelState::ModelEditorViewerPanelState(
    std::string_view panelName_) :

    m_PanelName{panelName_},
    m_CachedModelRenderer
    {
        App::singleton<SceneCache>(),
        *App::singleton<ShaderCache>(App::resource_loader()),
    }
{
}
