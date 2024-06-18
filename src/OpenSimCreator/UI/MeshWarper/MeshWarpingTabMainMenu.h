#pragma once

#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabActionsMenu.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabEditMenu.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabFileMenu.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/WindowMenu.h>

#include <memory>

namespace osc
{
    // widget: the main menu (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class MeshWarpingTabMainMenu final {
    public:
        explicit MeshWarpingTabMainMenu(
            const std::shared_ptr<MeshWarpingTabSharedState>& tabState_,
            const std::shared_ptr<PanelManager>& panelManager_) :

            m_FileMenu{tabState_},
            m_EditMenu{tabState_},
            m_ActionsMenu{tabState_},
            m_WindowMenu{panelManager_}
        {
        }

        void onDraw()
        {
            m_FileMenu.onDraw();
            m_EditMenu.onDraw();
            m_ActionsMenu.onDraw();
            m_WindowMenu.on_draw();
            m_AboutTab.onDraw();
        }
    private:
        MeshWarpingTabFileMenu m_FileMenu;
        MeshWarpingTabEditMenu m_EditMenu;
        MeshWarpingTabActionsMenu m_ActionsMenu;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutTab;
    };
}
