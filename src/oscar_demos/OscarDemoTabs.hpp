#pragma once

#include <oscar_demos/CustomWidgetsTab.hpp>
#include <oscar_demos/HittestTab.hpp>
#include <oscar_demos/ImGuiDemoTab.hpp>
#include <oscar_demos/ImGuizmoDemoTab.hpp>
#include <oscar_demos/ImPlotDemoTab.hpp>
#include <oscar_demos/MandelbrotTab.hpp>
#include <oscar_demos/MeshGenTestTab.hpp>
#include <oscar_demos/SubMeshTab.hpp>

#include <oscar/Utils/Typelist.hpp>

namespace osc
{
    using OscarDemoTabs = Typelist<
        CustomWidgetsTab,
        HittestTab,
        ImGuiDemoTab,
        ImPlotDemoTab,
        ImGuizmoDemoTab,
        MandelbrotTab,
        MeshGenTestTab,
        SubMeshTab
    >;
}
