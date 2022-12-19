#include "RendererNormalMappingTab.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

class osc::RendererNormalMappingTab::Impl final {
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
    // tab state
    UID m_ID;
    std::string m_Name = ICON_FA_COOKIE " NormalMapping (LearnOpenGL)";
    TabHost* m_Parent;
};


// public API (PIMPL)

osc::CStringView osc::RendererNormalMappingTab::id() noexcept
{
    return "Renderer/NormalMapping";
}

osc::RendererNormalMappingTab::RendererNormalMappingTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::RendererNormalMappingTab::RendererNormalMappingTab(RendererNormalMappingTab&&) noexcept = default;
osc::RendererNormalMappingTab& osc::RendererNormalMappingTab::operator=(RendererNormalMappingTab&&) noexcept = default;
osc::RendererNormalMappingTab::~RendererNormalMappingTab() noexcept = default;

osc::UID osc::RendererNormalMappingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererNormalMappingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererNormalMappingTab::implParent() const
{
    return m_Impl->parent();
}

void osc::RendererNormalMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererNormalMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererNormalMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererNormalMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererNormalMappingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererNormalMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
