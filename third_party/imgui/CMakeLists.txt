cmake_minimum_required(VERSION 3.10)

project(imgui VERSION 1.89.4)

add_library(imgui STATIC
    misc/cpp/imgui_stdlib.cpp
    misc/cpp/imgui_stdlib.h
    imgui.cpp
    imgui.h
    imgui_draw.cpp
    imgui_internal.h
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
)
target_include_directories(imgui
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    INTERFACE $<INSTALL_INTERFACE:include/imgui>
)
target_compile_features(imgui
    PRIVATE cxx_std_17
)
target_compile_definitions(imgui PUBLIC
    # for opensim-creator
    #
    # this is necessary until we move away from our legacy `imgui.ini` and
    # `imgui_base_config.ini` files because existing users have already encoded
    # the legacy CRC checks into their local `imgui.ini` file (and we don't want
    # it to screw up when they update the software)
    IMGUI_USE_LEGACY_CRC32_ADLER
)

include(GNUInstallDirs)
install(
    TARGETS imgui
    EXPORT imguiTargets
)
install(
    FILES
        imconfig.h
        imgui.h
        imgui_internal.h
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
    DESTINATION
        include/imgui
)
install(
    FILES
        misc/cpp/imgui_stdlib.h
    DESTINATION
        include/imgui/misc/cpp
)
install(
    EXPORT imguiTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "imguiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imguiConfig.cmake.in"
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
