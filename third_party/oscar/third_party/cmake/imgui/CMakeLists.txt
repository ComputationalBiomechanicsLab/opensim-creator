cmake_minimum_required(VERSION 3.10)
project(imgui VERSION 1.91.9)

include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR
include(CMakePackageConfigHelpers)  # write_basic_package_version_file, configure_package_config_file

set(OSCARDEPS_IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui")

add_library(imgui STATIC
    ${OSCARDEPS_IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
    ${OSCARDEPS_IMGUI_ROOT}/misc/cpp/imgui_stdlib.h
    ${OSCARDEPS_IMGUI_ROOT}/imgui.cpp
    ${OSCARDEPS_IMGUI_ROOT}/imgui.h
    ${OSCARDEPS_IMGUI_ROOT}/imgui_draw.cpp
    ${OSCARDEPS_IMGUI_ROOT}/imgui_internal.h
    ${OSCARDEPS_IMGUI_ROOT}/imgui_widgets.cpp
    ${OSCARDEPS_IMGUI_ROOT}/imgui_tables.cpp
    ${OSCARDEPS_IMGUI_ROOT}/imgui_demo.cpp
)
target_include_directories(imgui
    PRIVATE ${OSCARDEPS_IMGUI_ROOT}
    INTERFACE $<INSTALL_INTERFACE:include/imgui>
)
target_compile_features(imgui
    PRIVATE cxx_std_17
)
install(
    TARGETS imgui
    EXPORT imguiTargets
)
install(
    FILES
        ${OSCARDEPS_IMGUI_ROOT}/imconfig.h
        ${OSCARDEPS_IMGUI_ROOT}/imgui.h
        ${OSCARDEPS_IMGUI_ROOT}/imgui_internal.h  # required by implot/opensim creator
#        ${OSCARDEPS_IMGUI_ROOT}/imstb_rectpack.h  # not required by downstream
#        ${OSCARDEPS_IMGUI_ROOT}/imstb_textedit.h  # not required by downstream
#        ${OSCARDEPS_IMGUI_ROOT}/imstb_truetype.h  # not required by downstream
    DESTINATION
        include/imgui
)
install(
    FILES
       ${OSCARDEPS_IMGUI_ROOT}/misc/cpp/imgui_stdlib.h
    DESTINATION
        include/imgui/misc/cpp
)
install(
    EXPORT imguiTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
write_basic_package_version_file(
    "imguiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/imguiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)

