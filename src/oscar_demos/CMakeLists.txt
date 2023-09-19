add_library(oscar_demos STATIC

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
    MeshGenTestTab.cpp
    MeshGenTestTab.hpp
    OscarDemosTabRegistry.cpp
    OscarDemosTabRegistry.hpp
)

target_include_directories(oscar_demos PUBLIC

    # so that `#include "oscar_demos/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_demos PUBLIC
    oscar
)
