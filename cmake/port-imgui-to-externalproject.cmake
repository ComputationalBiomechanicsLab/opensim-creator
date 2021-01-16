if(FALSE)  # broken in CMake3.5, but works in newer CMakes because of how it resolves libs
    ExternalProject_Add(imgui-project
        URL "https://github.com/ocornut/imgui/archive/v1.79.zip"
        URL_HASH "SHA256=dcba57bb13a5cd903d570909b97d8fb2aaac669144f5d3c2b3a5f9b1f5998264"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
    )
    ExternalProject_Get_Property(imgui-project SOURCE_DIR)

    # ImGui configuration: copy an osmv-centric configuration into ImgGui, so that glm vectors
    # can be implicitly converted into ImVecs
    add_custom_target(imgui-configured
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h ${SOURCE_DIR}/imconfig.h
    )
    add_dependencies(imgui-configured imgui-project-build)  # must be checked out first

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR})

    add_library(osmv-imgui STATIC
        ${SOURCE_DIR}/imgui.cpp
        ${SOURCE_DIR}/imgui_draw.cpp
        ${SOURCE_DIR}/imgui_widgets.cpp
        ${SOURCE_DIR}/examples/imgui_impl_opengl3.cpp
        ${SOURCE_DIR}/examples/imgui_impl_sdl.cpp
    )
    add_dependencies(osmv-imgui imgui-configured)
    target_link_libraries(osmv-imgui PUBLIC osmv-sdl2 osmv-glew osmv-glm)
    target_include_directories(osmv-imgui PUBLIC ${SOURCE_DIR})

    unset(SOURCE_DIR)
endif()
