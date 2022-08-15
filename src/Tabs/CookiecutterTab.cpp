#include "CookiecutterTab.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

class osc::CookiecutterTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {

    }

    void onDraw()
    {

    }


private:
    UID m_ID;
    std::string m_Name = ICON_FA_COOKIE " CookiecutterTab";
    TabHost* m_Parent;
};


// public API

osc::CookiecutterTab::CookiecutterTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::CookiecutterTab::CookiecutterTab(CookiecutterTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::CookiecutterTab& osc::CookiecutterTab::operator=(CookiecutterTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::CookiecutterTab::~CookiecutterTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::CookiecutterTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::CookiecutterTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::CookiecutterTab::implParent() const
{
    return m_Impl->parent();
}

void osc::CookiecutterTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::CookiecutterTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::CookiecutterTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::CookiecutterTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::CookiecutterTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::CookiecutterTab::implOnDraw()
{
    m_Impl->onDraw();
}
