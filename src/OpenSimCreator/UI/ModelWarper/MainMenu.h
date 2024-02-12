#pragma once

#include <OpenSimCreator/UI/ModelWarper/FileMenu.h>
#include <OpenSimCreator/UI/ModelWarper/UIState.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/WindowMenu.h>

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
        {
        }

        void onDraw()
        {
            m_FileMenu.onDraw();
            m_WindowMenu.onDraw();
            m_AboutTab.onDraw();
        }
    private:
        FileMenu m_FileMenu;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutTab;
    };
}
