#pragma once

#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabActionsMenu.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabEditMenu.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabFileMenu.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>
#include <OpenSimCreator/UI/Shared/MainMenu.hpp>

#include <oscar/UI/Panels/PanelManager.hpp>
#include <oscar/UI/Widgets/WindowMenu.hpp>

#include <memory>

namespace osc
{
    // widget: the main menu (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class MeshWarpingTabMainMenu final {
    public:
        explicit MeshWarpingTabMainMenu(
            std::shared_ptr<MeshWarpingTabSharedState> const& tabState_,
            std::shared_ptr<PanelManager> const& panelManager_) :

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
            m_WindowMenu.onDraw();
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
