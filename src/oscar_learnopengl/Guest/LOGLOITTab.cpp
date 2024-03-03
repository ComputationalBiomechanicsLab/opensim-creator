#include "LOGLOITTab.h"

#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/Utils/CStringView.h>

#include <SDL_events.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "LearnOpenGL/OIT";
}

class osc::LOGLOITTab::Impl final : public StandardTabImpl {
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

osc::CStringView osc::LOGLOITTab::id()
{
    return c_TabStringID;
}

osc::LOGLOITTab::LOGLOITTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLOITTab::LOGLOITTab(LOGLOITTab&&) noexcept = default;
osc::LOGLOITTab& osc::LOGLOITTab::operator=(LOGLOITTab&&) noexcept = default;
osc::LOGLOITTab::~LOGLOITTab() noexcept = default;

UID osc::LOGLOITTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::LOGLOITTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLOITTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLOITTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLOITTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLOITTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLOITTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLOITTab::implOnDraw()
{
    m_Impl->onDraw();
}
