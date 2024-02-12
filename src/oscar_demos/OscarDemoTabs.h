#pragma once

#include <oscar_demos/CustomWidgetsTab.h>
#include <oscar_demos/HittestTab.h>
#include <oscar_demos/ImGuiDemoTab.h>
#include <oscar_demos/ImGuizmoDemoTab.h>
#include <oscar_demos/ImPlotDemoTab.h>
#include <oscar_demos/MandelbrotTab.h>
#include <oscar_demos/MeshGenTestTab.h>
#include <oscar_demos/SubMeshTab.h>

#include <oscar/Utils/Typelist.h>

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
