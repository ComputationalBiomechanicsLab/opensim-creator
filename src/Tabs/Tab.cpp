#include "Tab.hpp"

#include "src/Tabs/TabHost.hpp"
#include "src/Utils/CStringView.hpp"

#include <memory>
#include <utility>

osc::Tab::Tab(TabHost* parent) : m_Parent{ std::move(parent) }
{
}

void osc::Tab::onMount()
{
    implOnMount();
}

void osc::Tab::onUnmount()
{
    implOnUnmount();
}

bool osc::Tab::onEvent(SDL_Event const& e)
{
    return implOnEvent(e);
}

void osc::Tab::tick()
{
    implOnTick();
}

void osc::Tab::drawMainMenu()
{
    implDrawMainMenu();
}

void osc::Tab::draw()
{
    implOnDraw();
}

osc::CStringView osc::Tab::name()
{
    return implName();
}

void osc::Tab::addTab(std::unique_ptr<Tab> tab)
{
    m_Parent->addTab(std::move(tab));
}

void osc::Tab::selectTab(Tab* t)
{
    m_Parent->selectTab(std::move(t));
}

void osc::Tab::closeTab(Tab* t)
{
    m_Parent->closeTab(std::move(t));
}