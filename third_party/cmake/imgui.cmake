cmake_minimum_required(VERSION 3.10)

project(imgui VERSION 1.89.4)

find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_sdl2.cpp
)
target_include_directories(imgui
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    INTERFACE $<INSTALL_INTERFACE:include/imgui>
)
target_compile_features(imgui
    PRIVATE cxx_std_17
)
target_link_libraries(imgui
    PUBLIC SDL2::SDL2
    PUBLIC glm::glm
    PUBLIC ${CMAKE_DL_LIBS}  # imgui_impl_opengl3.cpp references `dlclose`
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
        backends/imgui_impl_opengl3.h
        backends/imgui_impl_sdl2.h
    DESTINATION
        include/imgui/backends
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
