#include "ModelWarperTab.h"

#include <OpenSimCreator/UI/ModelWarper/ChecklistPanel.h>
#include <OpenSimCreator/UI/ModelWarper/MainMenu.h>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperTabInitialPopup.h>
#include <OpenSimCreator/UI/ModelWarper/ResultModelViewerPanel.h>
#include <OpenSimCreator/UI/ModelWarper/SourceModelViewerPanel.h>
#include <OpenSimCreator/UI/ModelWarper/Toolbar.h>
#include <OpenSimCreator/UI/ModelWarper/UIState.h>

#include <oscar/Platform/App.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <string_view>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Model Warper (" OSC_ICON_MAGIC " experimental)";
}

class osc::mow::ModelWarperTab::Impl final : public TabPrivate {
public:
    explicit Impl(const ParentPtr<ITabHost>& tabHost) :
        TabPrivate{c_TabStringID},
        m_TabHost{tabHost}
    {
        m_PanelManager->register_toggleable_panel(
            "Checklist",
            [state = m_State](std::string_view panelName)
            {
                return std::make_shared<ChecklistPanel>(panelName, state);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Source Model",
            [state = m_State](std::string_view panelName)
            {
                return std::make_shared<SourceModelViewerPanel>(panelName, state);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Result Model",
            [state = m_State](std::string_view panelName)
            {
                return std::make_shared<ResultModelViewerPanel>(panelName, state);
            }
        );

        m_PanelManager->register_toggleable_panel(
            "Log",
            [](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );

        m_PopupManager.emplace_back<ModelWarperTabInitialPopup>("Model Warper Experimental Warning").open();
    }

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_PanelManager->on_mount();
        m_PopupManager.on_mount();
    }

    void on_unmount()
    {
        m_PanelManager->on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event&)
    {
        return false;
    }

    void on_tick()
    {
        m_PanelManager->on_tick();
    }

    void on_draw_main_menu()
    {
        m_MainMenu.onDraw();
    }

    void on_draw()
    {
        ui::enable_dockspace_over_main_viewport();

        m_Toolbar.onDraw();
        m_PanelManager->on_draw();
        m_PopupManager.on_draw();
    }

private:
    ParentPtr<ITabHost> m_TabHost;
    std::shared_ptr<UIState> m_State = std::make_shared<UIState>(m_TabHost);
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;
    MainMenu m_MainMenu{m_State, m_PanelManager};
    Toolbar m_Toolbar{"##ModelWarperToolbar", m_State};
};


CStringView osc::mow::ModelWarperTab::id() { return c_TabStringID; }

osc::mow::ModelWarperTab::ModelWarperTab(const ParentPtr<ITabHost>& tabHost) :
    Tab{std::make_unique<Impl>(tabHost)}
{}
void osc::mow::ModelWarperTab::impl_on_mount() { private_data().on_mount(); }
void osc::mow::ModelWarperTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::mow::ModelWarperTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::mow::ModelWarperTab::impl_on_tick() { private_data().on_tick(); }
void osc::mow::ModelWarperTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::mow::ModelWarperTab::impl_on_draw() { private_data().on_draw(); }
