#include "LOGLHDREquirectangularTab.hpp"

#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/HDREquirectangular";
}

class osc::LOGLHDREquirectangularTab::Impl final : public osc::StandardTabBase {
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

osc::CStringView osc::LOGLHDREquirectangularTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLHDREquirectangularTab::LOGLHDREquirectangularTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLHDREquirectangularTab::LOGLHDREquirectangularTab(LOGLHDREquirectangularTab&&) noexcept = default;
osc::LOGLHDREquirectangularTab& osc::LOGLHDREquirectangularTab::operator=(LOGLHDREquirectangularTab&&) noexcept = default;
osc::LOGLHDREquirectangularTab::~LOGLHDREquirectangularTab() noexcept = default;

osc::UID osc::LOGLHDREquirectangularTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLHDREquirectangularTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLHDREquirectangularTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLHDREquirectangularTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLHDREquirectangularTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLHDREquirectangularTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLHDREquirectangularTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLHDREquirectangularTab::implOnDraw()
{
    m_Impl->onDraw();
}
