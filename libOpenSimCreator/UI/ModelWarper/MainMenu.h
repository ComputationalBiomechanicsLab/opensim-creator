#pragma once

#include <libopensimcreator/UI/ModelWarper/FileMenu.h>
#include <libopensimcreator/UI/ModelWarper/UIState.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>

#include <liboscar/UI/Panels/PanelManager.h>
#include <liboscar/UI/Widgets/WindowMenu.h>

#include <memory>

namespace osc::mow
{
    class MainMenu final {
    public:
        MainMenu(
            std::shared_ptr<UIState> state_,
            std::shared_ptr<PanelManager> panelManager_) :

            m_FileMenu{state_},
            m_WindowMenu{panelManager_}
        {}

        void onDraw()
        {
            m_FileMenu.onDraw();
            m_WindowMenu.on_draw();
            m_AboutTab.onDraw();
        }
    private:
        FileMenu m_FileMenu;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutTab;
    };
}
