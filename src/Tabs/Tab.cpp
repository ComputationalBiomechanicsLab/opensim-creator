#include "Tab.hpp"

#include "src/Utils/CStringView.hpp"

#include <memory>
#include <utility>

// the impl-forwarding methods are here so that it's easier to hook into
// the API (e.g. for debugging)

// public API

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

osc::TabHost* osc::Tab::parent()
{
    return implParent();
}
