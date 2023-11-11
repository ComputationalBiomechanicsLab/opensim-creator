#include "ModelEditorViewerPanelState.hpp"

#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Scene/SceneMeshCache.hpp>
#include <oscar/Platform/App.hpp>

osc::ModelEditorViewerPanelState::ModelEditorViewerPanelState(
    std::string_view panelName_) :

    m_PanelName{panelName_},
    m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<SceneMeshCache>(),
        *App::singleton<ShaderCache>(),
    }
{
}
