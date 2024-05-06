#include "SimulationTabMainMenu.h"

#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationTabMainMenu::Impl final {
public:
    Impl(
        ParentPtr<IMainUIStateAPI> parent,
        std::shared_ptr<Simulation> simulation,
        std::shared_ptr<PanelManager> panelManager) :

        m_Parent{std::move(parent)},
        m_Simulation{std::move(simulation)},
        m_PanelManager{std::move(panelManager)}
    {}

    void onDraw()
    {
        m_MainMenuFileTab.onDraw(m_Parent);
        drawActionsMenu();
        m_MainMenuWindowTab.on_draw();
        m_MainMenuAboutTab.onDraw();
    }

private:
    void drawActionsMenu()
    {
        if (not ui::BeginMenu("Actions")) {
            return;
        }

        if (ui::BeginMenu("Change End Time", m_Simulation->canChangeEndTime())) {
            if (ui::MenuItem("0.1x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.1 * dur));
            }
            if (ui::MenuItem("0.25x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.25 * dur));
            }
            if (ui::MenuItem("0.5x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (0.5 * dur));
            }
            if (ui::MenuItem("2x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (2 * dur));
            }
            if (ui::MenuItem("4x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (4 * dur));
            }
            if (ui::MenuItem("10x")) {
                auto dur = m_Simulation->getEndTime() - m_Simulation->getStartTime();
                m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + (10 * dur));
            }
            {
                auto count = m_NewCustomEndTime.count();
                if (ui::InputDouble("custom end time", &count, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                    m_Simulation->requestNewEndTime(m_Simulation->getStartTime() + SimulationClock::duration{count});
                }
            }


            ui::EndMenu();
        }

        ui::EndMenu();
    }

    ParentPtr<IMainUIStateAPI> m_Parent;
    std::shared_ptr<Simulation> m_Simulation;
    std::shared_ptr<PanelManager> m_PanelManager;

    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    WindowMenu m_MainMenuWindowTab{m_PanelManager};
    SimulationClock::duration m_NewCustomEndTime = 2 * (m_Simulation->getEndTime() - m_Simulation->getStartTime());
};

osc::SimulationTabMainMenu::SimulationTabMainMenu(
    ParentPtr<IMainUIStateAPI> parent,
    std::shared_ptr<Simulation> simulation,
    std::shared_ptr<PanelManager> panelManager) :

    m_Impl{std::make_unique<Impl>(std::move(parent), std::move(simulation), std::move(panelManager))}
{}
osc::SimulationTabMainMenu::SimulationTabMainMenu(SimulationTabMainMenu&&) noexcept = default;
osc::SimulationTabMainMenu& osc::SimulationTabMainMenu::operator=(SimulationTabMainMenu&&) noexcept = default;
osc::SimulationTabMainMenu::~SimulationTabMainMenu() noexcept = default;

void osc::SimulationTabMainMenu::onDraw()
{
    m_Impl->onDraw();
}
