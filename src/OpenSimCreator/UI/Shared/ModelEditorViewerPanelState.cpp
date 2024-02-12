#include "ModelEditorViewerPanelState.h"

#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/ShaderCache.h>
#include <oscar/Platform/App.h>

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
