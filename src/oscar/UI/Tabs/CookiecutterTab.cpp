#include "CookiecutterTab.hpp"

#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

using osc::CStringView;

namespace
{
    constexpr CStringView c_TabStringID = "CookiecutterTab";
}

class osc::CookiecutterTab::Impl final : public StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
    }

    void implOnUnmount() final
    {
    }

    bool implOnEvent(SDL_Event const&) final
    {
        return false;
    }

    void implOnTick() final
    {
    }

    void implOnDrawMainMenu() final
    {
    }

    void implOnDraw() final
    {
    }
};


// public API

osc::CStringView osc::CookiecutterTab::id()
{
    return c_TabStringID;
}

osc::CookiecutterTab::CookiecutterTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
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
