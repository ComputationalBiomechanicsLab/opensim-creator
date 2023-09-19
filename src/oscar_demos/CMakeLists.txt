add_library(oscar_demos STATIC

    Tabs/CustomWidgetsTab.cpp
    Tabs/CustomWidgetsTab.hpp
    Tabs/HittestTab.cpp
    Tabs/HittestTab.hpp
    Tabs/ImGuiDemoTab.cpp
    Tabs/ImGuiDemoTab.hpp
    Tabs/ImGuizmoDemoTab.cpp
    Tabs/ImGuizmoDemoTab.hpp
    Tabs/ImPlotDemoTab.cpp
    Tabs/ImPlotDemoTab.hpp
    Tabs/MeshGenTestTab.cpp
    Tabs/MeshGenTestTab.hpp

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
