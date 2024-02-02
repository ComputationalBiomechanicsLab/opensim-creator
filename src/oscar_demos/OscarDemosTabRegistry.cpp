#include "OscarDemosTabRegistry.hpp"

#include <oscar_demos/CustomWidgetsTab.hpp>
#include <oscar_demos/HittestTab.hpp>
#include <oscar_demos/ImGuiDemoTab.hpp>
#include <oscar_demos/ImGuizmoDemoTab.hpp>
#include <oscar_demos/ImPlotDemoTab.hpp>
#include <oscar_demos/MandelbrotTab.hpp>
#include <oscar_demos/MeshGenTestTab.hpp>
#include <oscar_demos/SubMeshTab.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>
#include <oscar/Utils/Typelist.hpp>

void osc::RegisterDemoTabs(TabRegistry& registry)
{
    using DemoTabs = Typelist<
        CustomWidgetsTab,
        HittestTab,
        ImGuiDemoTab,
        ImPlotDemoTab,
        ImGuizmoDemoTab,
        MandelbrotTab,
        MeshGenTestTab,
        SubMeshTab
    >;

    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(DemoTabs{});
}
