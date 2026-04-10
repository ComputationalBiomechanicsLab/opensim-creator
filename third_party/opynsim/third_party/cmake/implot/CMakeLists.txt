cmake_minimum_required(VERSION 3.25)
project(implot LANGUAGES CXX)

include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR
include(CMakePackageConfigHelpers)  # configure_package_config_file

find_package(imgui REQUIRED)

set(_implot_root ${CMAKE_CURRENT_SOURCE_DIR}/../../implot)

add_library(implot STATIC
    ${_implot_root}/implot.cpp
    ${_implot_root}/implot.h
    ${_implot_root}/implot_demo.cpp
    ${_implot_root}/implot_internal.h
    ${_implot_root}/implot_items.cpp
)
target_include_directories(implot
    PRIVATE ${_implot_root}
    INTERFACE $<INSTALL_INTERFACE:include/implot>
)
target_compile_features(implot
    PRIVATE cxx_std_17
)
target_link_libraries(implot PUBLIC imgui)

install(
    TARGETS implot
    EXPORT implotTargets
)
install(
    FILES
        ${_implot_root}/implot.h
    DESTINATION
        include/implot
)
install(
    EXPORT implotTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/implot
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/implotConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/implotConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/implot
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/implotConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/implot
)
