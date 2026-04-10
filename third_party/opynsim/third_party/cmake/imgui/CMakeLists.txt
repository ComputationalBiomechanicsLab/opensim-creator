cmake_minimum_required(VERSION 3.25)
project(imgui LANGUAGES CXX)

include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR
include(CMakePackageConfigHelpers)  # configure_package_config_file

set(_imgui_root "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui")

add_library(imgui STATIC
    ${_imgui_root}/misc/cpp/imgui_stdlib.cpp
    ${_imgui_root}/misc/cpp/imgui_stdlib.h
    ${_imgui_root}/imgui.cpp
    ${_imgui_root}/imgui.h
    ${_imgui_root}/imgui_draw.cpp
    ${_imgui_root}/imgui_internal.h
    ${_imgui_root}/imgui_widgets.cpp
    ${_imgui_root}/imgui_tables.cpp
    ${_imgui_root}/imgui_demo.cpp
)
target_include_directories(imgui
    PRIVATE ${_imgui_root}
    INTERFACE $<INSTALL_INTERFACE:include/imgui>
)
target_compile_features(imgui
    PRIVATE cxx_std_17
)
target_compile_definitions(imgui
    # Aggressively avoid obsolete functions
    PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS

    # for opensim-creator
    #
    # this is necessary until we move away from our legacy `imgui.ini` and
    # `imgui_base_config.ini` files because existing users have already encoded
    # the legacy CRC checks into their local `imgui.ini` file (and we don't want
    # it to screw up when they update the software)
    PRIVATE IMGUI_USE_LEGACY_CRC32_ADLER
)
install(
    TARGETS imgui
    EXPORT imguiTargets
)
install(
    FILES
        ${_imgui_root}/imconfig.h
        ${_imgui_root}/imgui.h
        ${_imgui_root}/imgui_internal.h  # required by implot/oscimgui
#        ${_imgui_root}/imstb_rectpack.h  # not required by downstream
#        ${_imgui_root}/imstb_textedit.h  # not required by downstream
#        ${_imgui_root}/imstb_truetype.h  # not required by downstream
    DESTINATION
        include/imgui
)
install(
    FILES
       ${_imgui_root}/misc/cpp/imgui_stdlib.h
    DESTINATION
        include/imgui/misc/cpp
)
install(
    EXPORT imguiTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/imguiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)
