cmake_minimum_required(VERSION 3.10)
project(implot)

include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR
include(CMakePackageConfigHelpers)  # configure_package_config_file

find_package(imgui REQUIRED)

set(OSCARDEPS_IMPLOT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../implot)

add_library(implot STATIC
    ${OSCARDEPS_IMPLOT_ROOT}/implot.cpp
    ${OSCARDEPS_IMPLOT_ROOT}/implot.h
    ${OSCARDEPS_IMPLOT_ROOT}/implot_demo.cpp
    ${OSCARDEPS_IMPLOT_ROOT}/implot_internal.h
    ${OSCARDEPS_IMPLOT_ROOT}/implot_items.cpp
)
target_include_directories(implot
    PRIVATE ${OSCARDEPS_IMPLOT_ROOT}
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
        ${OSCARDEPS_IMPLOT_ROOT}/implot.h
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
