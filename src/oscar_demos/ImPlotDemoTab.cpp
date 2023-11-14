#include "ImPlotDemoTab.hpp"

#include <implot.h>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/ImPlot";
}

class osc::ImPlotDemoTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
        // ImPlot::CreateContext();  this is already done by MainUIScreen
    }

    void implOnUnmount() final
    {
        // ImPlot::DestroyContext();  this is already done by MainUIScreen
    }

    void implOnDraw() final
    {
        ImPlot::ShowDemoWindow();
    }
};


// public API

osc::CStringView osc::ImPlotDemoTab::id()
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
