add_library(oscar_demos STATIC

    CookiecutterScreen.cpp
    CookiecutterScreen.h
    CustomWidgetsTab.cpp
    CustomWidgetsTab.h
    FrustrumCullingTab.cpp
    FrustrumCullingTab.h
    HittestTab.cpp
    HittestTab.h
    ImGuiDemoTab.cpp
    ImGuiDemoTab.h
    ImGuizmoDemoTab.cpp
    ImGuizmoDemoTab.h
    ImPlotDemoTab.cpp
    ImPlotDemoTab.h
    MandelbrotTab.cpp
    MandelbrotTab.h
    MeshGenTestTab.cpp
    MeshGenTestTab.h
    OscarDemosTabRegistry.cpp
    OscarDemosTabRegistry.h
    OscarDemoTabs.h
    SubMeshTab.cpp
    SubMeshTab.h
)

target_include_directories(oscar_demos PUBLIC

    # so that `#include "oscar_demos/HEADER.h"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_demos PUBLIC
    oscar
)

set_target_properties(oscar_demos PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)