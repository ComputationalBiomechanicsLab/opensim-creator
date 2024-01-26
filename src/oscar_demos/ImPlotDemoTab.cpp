#include "ImPlotDemoTab.hpp"

#include <implot.h>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <memory>

using osc::CStringView;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/ImPlot";
}

class osc::ImPlotDemoTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        // ImPlot::CreateContext();  // presumed to already done by the screen
    }

private:
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

osc::ImPlotDemoTab::ImPlotDemoTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

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

void osc::ImPlotDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
