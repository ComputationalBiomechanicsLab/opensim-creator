#include "SimulationTabMainMenu.h"

#include <libopensimcreator/Documents/Simulation/Simulation.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>

#include <liboscar/Platform/Widget.h>
#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/PanelManager.h>
#include <liboscar/UI/Widgets/WindowMenu.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationTabMainMenu::Impl final : public WidgetPrivate {
public:
    Impl(
        Widget& owner,
        Widget* parent,
        std::shared_ptr<Simulation> simulation,
        std::shared_ptr<PanelManager> panelManager) :

        WidgetPrivate{owner, parent},
        m_Simulation{std::move(simulation)},
        m_MainMenuFileTab{&owner},
        m_MainMenuWindowTab{&owner, std::move(panelManager)}
    {}

    void onDraw()
    {
        m_MainMenuFileTab.onDraw();
        drawActionsMenu();
        m_MainMenuWindowTab.on_draw();
        m_MainMenuAboutTab.onDraw();
    }

private:
    void drawActionsMenu()
    {
        if (not ui::begin_menu("Actions")) {
            return;
        }

        if (ui::begin_menu("Change End Time", m_Simulation->canChangeEndTime())) {
            if (ui::draw_menu_item("0.1x")) {
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.1 * m_Simulation->getDuration()));
            }
            if (ui::draw_menu_item("0.25x")) {
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.25 * m_Simulation->getDuration()));
            }
            if (ui::draw_menu_item("0.5x")) {
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.5 * m_Simulation->getDuration()));
            }
            if (ui::draw_menu_item("2x")) {
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (2 * m_Simulation->getDuration()));
            }
            if (ui::draw_menu_item("4x")) {
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (4 * m_Simulation->getDuration()));
            }
            if (ui::draw_menu_item("10x")) {
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (10 * m_Simulation->getDuration()));
            }
            {
                auto count = m_NewCustomEndTime.count();
                ui::draw_double_input("custom end time", &count);
                if (ui::should_save_last_drawn_item_value()) {
                    m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + SimulationClock::duration{count});
                }
            }


            ui::end_menu();
        }

        ui::end_menu();
    }

    std::shared_ptr<Simulation> m_Simulation;

    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    WindowMenu m_MainMenuWindowTab;
    SimulationClock::duration m_NewCustomEndTime = 2 * (m_Simulation->getEndTime() - m_Simulation->getStartTime());
};

osc::SimulationTabMainMenu::SimulationTabMainMenu(
    Widget* parent,
    std::shared_ptr<Simulation> simulation,
    std::shared_ptr<PanelManager> panelManager) :

    Widget{std::make_unique<Impl>(*this, parent, std::move(simulation), std::move(panelManager))}
{}
void osc::SimulationTabMainMenu::impl_on_draw() { private_data().onDraw(); }
