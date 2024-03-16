#include "LOGLCSMTab.h"

#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/Utils/CStringView.h>

#include <SDL_events.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/CSM";
}

class osc::LOGLCSMTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
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

osc::CStringView osc::LOGLCSMTab::id()
{
    return c_TabStringID;
}

osc::LOGLCSMTab::LOGLCSMTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLCSMTab::LOGLCSMTab(LOGLCSMTab&&) noexcept = default;
osc::LOGLCSMTab& osc::LOGLCSMTab::operator=(LOGLCSMTab&&) noexcept = default;
osc::LOGLCSMTab::~LOGLCSMTab() noexcept = default;

UID osc::LOGLCSMTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLCSMTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLCSMTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLCSMTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLCSMTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLCSMTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLCSMTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLCSMTab::implOnDraw()
{
    m_Impl->onDraw();
}
