cmake_minimum_required(VERSION 3.10)
project(ImGuiColorTextEdit)

include(GNUInstallDirs) # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
include(CMakePackageConfigHelpers)  # configure_package_config_file

find_package(imgui REQUIRED)

add_library(ImGuiColorTextEdit STATIC
    TextEditor.cpp
    TextEditor.h
)
target_include_directories(ImGuiColorTextEdit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ImGuiColorTextEdit>
)
target_compile_features(ImGuiColorTextEdit PRIVATE cxx_std_17)
target_link_libraries(ImGuiColorTextEdit PUBLIC imgui)
install(
    TARGETS ImGuiColorTextEdit
    EXPORT ImGuiColorTextEditTargets
)
install(
    FILES TextEditor.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ImGuiColorTextEdit
)
install(
    EXPORT ImGuiColorTextEditTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGuiColorTextEdit
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ImGuiColorTextEditConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ImGuiColorTextEditConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGuiColorTextEdit
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ImGuiColorTextEditConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGuiColorTextEdit
)
