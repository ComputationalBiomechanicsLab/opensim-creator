add_library(oscar_demos STATIC

    CookiecutterScreen.cpp
    CookiecutterScreen.hpp
    CustomWidgetsTab.cpp
    CustomWidgetsTab.hpp
    HittestTab.cpp
    HittestTab.hpp
    ImGuiDemoTab.cpp
    ImGuiDemoTab.hpp
    ImGuizmoDemoTab.cpp
    ImGuizmoDemoTab.hpp
    ImPlotDemoTab.cpp
    ImPlotDemoTab.hpp
    MandelbrotTab.cpp
    MandelbrotTab.hpp
    MeshGenTestTab.cpp
    MeshGenTestTab.hpp
    OscarDemosTabRegistry.cpp
    OscarDemosTabRegistry.hpp
    SubMeshTab.cpp
    SubMeshTab.hpp
)

target_include_directories(oscar_demos PUBLIC

    # so that `#include "oscar_demos/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_demos PUBLIC
    oscar
)
