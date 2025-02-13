#pragma once

#include <libopensimcreator/UI/ModelWarper/FileMenu.h>
#include <libopensimcreator/UI/ModelWarper/UIState.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>

#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/Panels/PanelManager.h>
#include <liboscar/UI/Widgets/WindowMenu.h>

#include <memory>

namespace osc::mow
{
    class MainMenu final : public Widget {
    public:
        explicit MainMenu(
            Widget* parent,
            std::shared_ptr<UIState> state_,
            std::shared_ptr<PanelManager> panelManager_) :

            Widget{parent},
            m_FileMenu{state_},
            m_WindowMenu{this, panelManager_}
        {}
    private:
        void impl_on_draw() final
        {
            m_FileMenu.onDraw();
            m_WindowMenu.on_draw();
            m_AboutTab.onDraw();
        }

        FileMenu m_FileMenu;
        WindowMenu m_WindowMenu;
        MainMenuAboutTab m_AboutTab;
    };
}
