cmake_minimum_required(VERSION 3.10)

project(implot)

find_package(imgui REQUIRED)

add_library(implot STATIC
    implot.cpp
    implot.h
    implot_demo.cpp
    implot_internal.h
    implot_items.cpp
)
target_include_directories(implot
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    INTERFACE $<INSTALL_INTERFACE:include/implot>
)
target_compile_features(implot
    PRIVATE cxx_std_17
)
target_link_libraries(implot PUBLIC imgui)

include(GNUInstallDirs)
install(
    TARGETS implot
    EXPORT implotTargets
)
install(
    FILES
        implot.h
    DESTINATION
        include/implot
)
install(
    EXPORT implotTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/implot
)

include(CMakePackageConfigHelpers)
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
