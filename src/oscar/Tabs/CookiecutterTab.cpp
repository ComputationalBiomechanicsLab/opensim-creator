#include "CookiecutterTab.hpp"

#include "oscar/Utils/CStringView.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "CookiecutterTab";
}

class osc::CookiecutterTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
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
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;
};


// public API (PIMPL)

osc::CStringView osc::CookiecutterTab::id() noexcept
{
    return c_TabStringID;
}

osc::CookiecutterTab::CookiecutterTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::CookiecutterTab::CookiecutterTab(CookiecutterTab&&) noexcept = default;
osc::CookiecutterTab& osc::CookiecutterTab::operator=(CookiecutterTab&&) noexcept = default;
osc::CookiecutterTab::~CookiecutterTab() noexcept = default;

osc::UID osc::CookiecutterTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::CookiecutterTab::implGetName() const
{
    return m_Impl->getName();
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
