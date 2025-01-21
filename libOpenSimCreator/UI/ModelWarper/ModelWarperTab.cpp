#include "ModelWarperTab.h"

#include <libopensimcreator/UI/ModelWarper/ChecklistPanel.h>
#include <libopensimcreator/UI/ModelWarper/MainMenu.h>
#include <libopensimcreator/UI/ModelWarper/ModelWarperTabInitialPopup.h>
#include <libopensimcreator/UI/ModelWarper/ResultModelViewerPanel.h>
#include <libopensimcreator/UI/ModelWarper/SourceModelViewerPanel.h>
#include <libopensimcreator/UI/ModelWarper/Toolbar.h>
#include <libopensimcreator/UI/ModelWarper/UIState.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/UI/Panels/LogViewerPanel.h>
#include <liboscar/UI/Panels/PanelManager.h>
#include <liboscar/UI/Popups/PopupManager.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/CStringView.h>

#include <functional>
#include <string_view>

using namespace osc;

class osc::mow::ModelWarperTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "Model Warper (" OSC_ICON_MAGIC " experimental)"; }

    explicit Impl(ModelWarperTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()},
        m_State{std::make_shared<UIState>(parent)}
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
    std::shared_ptr<UIState> m_State;
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;
    MainMenu m_MainMenu{m_State, m_PanelManager};
    Toolbar m_Toolbar{"##ModelWarperToolbar", m_State};
};


CStringView osc::mow::ModelWarperTab::id() { return Impl::static_label(); }

osc::mow::ModelWarperTab::ModelWarperTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::mow::ModelWarperTab::impl_on_mount() { private_data().on_mount(); }
void osc::mow::ModelWarperTab::impl_on_unmount() { private_data().on_unmount(); }
void osc::mow::ModelWarperTab::impl_on_tick() { private_data().on_tick(); }
void osc::mow::ModelWarperTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::mow::ModelWarperTab::impl_on_draw() { private_data().on_draw(); }
