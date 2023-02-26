#include "RendererParallaxMappingTab.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

class osc::RendererParallaxMappingTab::Impl final {
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
        return ICON_FA_COOKIE " RendererParallaxMappingTab";
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

osc::CStringView osc::RendererParallaxMappingTab::id() noexcept
{
    return "Renderer/ParallaxMapping";
}

osc::RendererParallaxMappingTab::RendererParallaxMappingTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::RendererParallaxMappingTab::RendererParallaxMappingTab(RendererParallaxMappingTab&&) noexcept = default;
osc::RendererParallaxMappingTab& osc::RendererParallaxMappingTab::operator=(RendererParallaxMappingTab&&) noexcept = default;
osc::RendererParallaxMappingTab::~RendererParallaxMappingTab() noexcept = default;

osc::UID osc::RendererParallaxMappingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererParallaxMappingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererParallaxMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererParallaxMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererParallaxMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererParallaxMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererParallaxMappingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererParallaxMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
