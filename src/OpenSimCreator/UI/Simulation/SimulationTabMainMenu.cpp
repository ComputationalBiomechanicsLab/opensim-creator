#include "SimulationTabMainMenu.h"

#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/UI/Widgets/WindowMenu.h>
#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationTabMainMenu::Impl final {
public:
    Impl(
        ParentPtr<IMainUIStateAPI> parent,
        std::shared_ptr<PanelManager> panelManager) :

        m_Parent{std::move(parent)},
        m_PanelManager{std::move(panelManager)}
    {}

    void onDraw()
    {
        m_MainMenuFileTab.onDraw(m_Parent);
        m_MainMenuWindowTab.onDraw();
        m_MainMenuAboutTab.onDraw();
    }

    ParentPtr<IMainUIStateAPI> m_Parent;
    std::shared_ptr<PanelManager> m_PanelManager;

    MainMenuFileTab m_MainMenuFileTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    WindowMenu m_MainMenuWindowTab{m_PanelManager};
};

osc::SimulationTabMainMenu::SimulationTabMainMenu(
    ParentPtr<IMainUIStateAPI> parent,
    std::shared_ptr<PanelManager> panelManager) :

    m_Impl{std::make_unique<Impl>(std::move(parent), std::move(panelManager))}
{}
osc::SimulationTabMainMenu::SimulationTabMainMenu(SimulationTabMainMenu&&) noexcept = default;
osc::SimulationTabMainMenu& osc::SimulationTabMainMenu::operator=(SimulationTabMainMenu&&) noexcept = default;
osc::SimulationTabMainMenu::~SimulationTabMainMenu() noexcept = default;

void osc::SimulationTabMainMenu::onDraw()
{
    m_Impl->onDraw();
}
