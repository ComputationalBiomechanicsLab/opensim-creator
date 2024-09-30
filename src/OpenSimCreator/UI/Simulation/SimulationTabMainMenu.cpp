#include "SimulationTabMainMenu.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <oscar/Platform/Widget.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/LifetimedPtr.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationTabMainMenu::Impl final {
public:
    Impl(
        Widget& parent,
        std::shared_ptr<Simulation> simulation,
        std::shared_ptr<PanelManager> panelManager) :

        m_Parent{parent.weak_ref()},
        m_Simulation{std::move(simulation)},
        m_PanelManager{std::move(panelManager)},
        m_MainMenuFileTab{parent}
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
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.1 * dur));
            }
            if (ui::draw_menu_item("0.25x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.25 * dur));
            }
            if (ui::draw_menu_item("0.5x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.5 * dur));
            }
            if (ui::draw_menu_item("2x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (2 * dur));
            }
            if (ui::draw_menu_item("4x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (4 * dur));
            }
            if (ui::draw_menu_item("10x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (10 * dur));
            }
            {
                auto count = m_NewCustomEndTime.count();
                if (ui::draw_double_input("custom end time", &count, 0.0, 0.0, "%.6f", ui::TextInputFlag::EnterReturnsTrue)) {
                    m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + SimulationClock::duration{count});
                }
            }


            ui::end_menu();
        }

        ui::end_menu();
    }

    LifetimedPtr<Widget> m_Parent;
    std::shared_ptr<Simulation> m_Simulation;
    std::shared_ptr<PanelManager> m_PanelManager;

    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    WindowMenu m_MainMenuWindowTab{m_PanelManager};
    SimulationClock::duration m_NewCustomEndTime = 2 * (m_Simulation->getEndTime() - m_Simulation->getStartTime());
};

osc::SimulationTabMainMenu::SimulationTabMainMenu(
    Widget& parent,
    std::shared_ptr<Simulation> simulation,
    std::shared_ptr<PanelManager> panelManager) :

    m_Impl{std::make_unique<Impl>(parent, std::move(simulation), std::move(panelManager))}
{}
osc::SimulationTabMainMenu::SimulationTabMainMenu(SimulationTabMainMenu&&) noexcept = default;
osc::SimulationTabMainMenu& osc::SimulationTabMainMenu::operator=(SimulationTabMainMenu&&) noexcept = default;
osc::SimulationTabMainMenu::~SimulationTabMainMenu() noexcept = default;

void osc::SimulationTabMainMenu::onDraw()
{
    m_Impl->onDraw();
}
