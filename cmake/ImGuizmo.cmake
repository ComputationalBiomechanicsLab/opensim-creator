cmake_minimum_required(VERSION 3.10)

project(ImGuizmo)

find_package(imgui REQUIRED)

add_library(ImGuizmo STATIC
    ImGuizmo.cpp
)
target_include_directories(ImGuizmo
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    INTERFACE $<INSTALL_INTERFACE:include/ImGuizmo>
)
target_compile_features(ImGuizmo
    PRIVATE cxx_std_17
)
target_link_libraries(ImGuizmo
    PUBLIC imgui
)

include(GNUInstallDirs)
install(
    TARGETS ImGuizmo
    EXPORT ImGuizmoTargets
)
install(
    FILES
        ImGuizmo.h
    DESTINATION
        include/ImGuizmo
)
install(
    EXPORT ImGuizmoTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGuizmo
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ImGuizmoConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGuizmo
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ImGuizmoConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/ImGuizmo
)