#include "ImPlotDemoTab.hpp"

#include "oscar/Utils/CStringView.hpp"

#include <implot.h>

#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/ImPlot";
}

class osc::ImPlotDemoTab::Impl final {
public:

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
    return c_TabStringID;
}

osc::ImPlotDemoTab::ImPlotDemoTab(ParentPtr<TabHost> const&) :
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
