#pragma once

#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabActionsMenu.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabEditMenu.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabFileMenu.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>

#include <liboscar/platform/Widget.h>
#include <liboscar/ui/panels/PanelManager.h>
#include <liboscar/ui/widgets/WindowMenu.h>

#include <memory>

namespace osc
{
    // widget: the main menu (contains multiple submenus: 'file', 'edit', 'about', etc.)
    class MeshWarpingTabMainMenu final : public Widget {
    public:
        explicit MeshWarpingTabMainMenu(
            Widget* parent,
            const std::shared_ptr<MeshWarpingTabSharedState>& tabState_,
            const std::shared_ptr<PanelManager>& panelManager_) :

            Widget{parent},
            m_FileMenu{tabState_},
            m_EditMenu{tabState_},
            m_ActionsMenu{tabState_},
            m_WindowMenu{this, panelManager_}
        {}

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
