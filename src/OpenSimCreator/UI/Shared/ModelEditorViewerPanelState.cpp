#include "ModelEditorViewerPanelState.hpp"

#include <oscar/Platform/App.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/ShaderCache.hpp>

osc::ModelEditorViewerPanelState::ModelEditorViewerPanelState(
    std::string_view panelName_) :

    m_PanelName{panelName_},
    m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<SceneCache>(),
        *App::singleton<ShaderCache>(),
    }
{
}
