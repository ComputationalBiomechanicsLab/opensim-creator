#include "ImPlotDemoTab.hpp"

#include <IconsFontAwesome5.h>
#include <implot.h>

#include <memory>

class osc::ImPlotDemoTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_HAT_WIZARD " ImPlotDemo";
    }

    void onMount()
    {
        // ImPlot::CreateContext();  this is already done by MainUIScreen
    }

    void onUnmount()
    {
        // ImPlot::DestroyContext();  this is already done by MainUIScreen
    }

    void onDraw()
    {
        ImPlot::ShowDemoWindow();
    }

private:
    UID m_TabID;
};


// public API (PIMPL)

osc::CStringView osc::ImPlotDemoTab::id() noexcept
{
    return "Demos/ImPlot";
}

osc::ImPlotDemoTab::ImPlotDemoTab(TabHost*) :
    m_Impl{std::make_unique<Impl>()}
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

void osc::ImPlotDemoTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ImPlotDemoTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

void osc::ImPlotDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
