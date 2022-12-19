#include "ImPlotDemoTab.hpp"

#include "src/Platform/Log.hpp"

#include <SDL_events.h>
#include <IconsFontAwesome5.h>
#include <implot.h>

#include <string>
#include <utility>

class osc::ImPlotDemoTab::Impl final {
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
        ImPlot::CreateContext();
    }

    void onUnmount()
    {
        ImPlot::DestroyContext();
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
        ImPlot::ShowDemoWindow();
    }

private:
    UID m_ID;
    std::string m_Name = ICON_FA_HAT_WIZARD " ImPlotDemo";
    TabHost* m_Parent;
};


// public API (PIMPL)

osc::CStringView osc::ImPlotDemoTab::id() noexcept
{
    return "Demos/ImPlot";
}

osc::ImPlotDemoTab::ImPlotDemoTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::ImPlotDemoTab::ImPlotDemoTab(ImPlotDemoTab&&) noexcept = default;
osc::ImPlotDemoTab& osc::ImPlotDemoTab::operator=(ImPlotDemoTab&&) noexcept = default;
osc::ImPlotDemoTab::~ImPlotDemoTab() noexcept = default;

osc::UID osc::ImPlotDemoTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ImPlotDemoTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ImPlotDemoTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ImPlotDemoTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ImPlotDemoTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ImPlotDemoTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ImPlotDemoTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ImPlotDemoTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ImPlotDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
