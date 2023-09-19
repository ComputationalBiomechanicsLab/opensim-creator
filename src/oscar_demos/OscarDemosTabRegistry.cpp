#include "OscarDemosTabRegistry.hpp"

#include "oscar_demos/Tabs/CustomWidgetsTab.hpp"
#include "oscar_demos/Tabs/HittestTab.hpp"
#include "oscar_demos/Tabs/ImGuiDemoTab.hpp"
#include "oscar_demos/Tabs/ImGuizmoDemoTab.hpp"
#include "oscar_demos/Tabs/ImPlotDemoTab.hpp"
#include "oscar_demos/Tabs/MeshGenTestTab.hpp"

#include <oscar/Tabs/TabRegistry.hpp>
#include <oscar/Tabs/TabRegistryEntry.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <memory>

namespace
{
    template<typename TabType>
    void RegisterTab(osc::TabRegistry& registry)
    {
        osc::TabRegistryEntry entry
        {
            TabType::id(),
            [](osc::ParentPtr<osc::TabHost> const& h) { return std::make_unique<TabType>(h); },
        };
        registry.registerTab(entry);
    }
}

void osc::RegisterDemoTabs(TabRegistry& registry)
{
    RegisterTab<CustomWidgetsTab>(registry);
    RegisterTab<HittestTab>(registry);
    RegisterTab<ImGuiDemoTab>(registry);
    RegisterTab<ImPlotDemoTab>(registry);
    RegisterTab<ImGuizmoDemoTab>(registry);
    RegisterTab<MeshGenTestTab>(registry);
}
