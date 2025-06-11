cmake_minimum_required(VERSION 3.10)
project(imgui VERSION 1.91.9)

include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR
include(CMakePackageConfigHelpers)  # write_basic_package_version_file, configure_package_config_file

set(OSCDEPS_IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui")

add_library(imgui STATIC
    ${OSCDEPS_IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
    ${OSCDEPS_IMGUI_ROOT}/misc/cpp/imgui_stdlib.h
    ${OSCDEPS_IMGUI_ROOT}/imgui.cpp
    ${OSCDEPS_IMGUI_ROOT}/imgui.h
    ${OSCDEPS_IMGUI_ROOT}/imgui_draw.cpp
    ${OSCDEPS_IMGUI_ROOT}/imgui_internal.h
    ${OSCDEPS_IMGUI_ROOT}/imgui_widgets.cpp
    ${OSCDEPS_IMGUI_ROOT}/imgui_tables.cpp
    ${OSCDEPS_IMGUI_ROOT}/imgui_demo.cpp
)
target_include_directories(imgui
    PRIVATE ${OSCDEPS_IMGUI_ROOT}
    INTERFACE $<INSTALL_INTERFACE:include/imgui>
)
target_compile_features(imgui
    PRIVATE cxx_std_17
)
target_compile_definitions(imgui PRIVATE
    # for opensim-creator
    #
    # this is necessary until we move away from our legacy `imgui.ini` and
    # `imgui_base_config.ini` files because existing users have already encoded
    # the legacy CRC checks into their local `imgui.ini` file (and we don't want
    # it to screw up when they update the software)
    IMGUI_USE_LEGACY_CRC32_ADLER
)
install(
    TARGETS imgui
    EXPORT imguiTargets
)
install(
    FILES
        ${OSCDEPS_IMGUI_ROOT}/imconfig.h
        ${OSCDEPS_IMGUI_ROOT}/imgui.h
        ${OSCDEPS_IMGUI_ROOT}/imgui_internal.h  # required by implot/opensim creator
#        ${OSCDEPS_IMGUI_ROOT}/imstb_rectpack.h  # not required by downstream
#        ${OSCDEPS_IMGUI_ROOT}/imstb_textedit.h  # not required by downstream
#        ${OSCDEPS_IMGUI_ROOT}/imstb_truetype.h  # not required by downstream
    DESTINATION
        include/imgui
)
install(
    FILES
       ${OSCDEPS_IMGUI_ROOT}/misc/cpp/imgui_stdlib.h
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

