cmake_minimum_required(VERSION 3.13)

project(osmv VERSION 0.0.1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# vendored dependencies
#
# - this project deliberately uses C libraries that only transitively depend on
#   OS-level libs.
#
# - those deps are copied in-tree to reduce the amount of fucking around
#   required to build osmv
add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm-0.9.9.8 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)

# opensim: linked as a separate dependency
#
# - to customize this, set CMAKE_PREFIX_PATH to the appropriate cmake/ dir in
#   an OpenSim install (e.g. OpenSim4.1/cmake/, OpenSim4.1/lib/cmake/)
#
# - you can build your own OpenSim install dir from source by checking out
#   OpenSim, building it (follow their instructions) and building the `install`
#   target
#
# - by default, this `find_package` call will typically just find the installed
#   OpenSim on your system
find_package(OpenSim REQUIRED)

# source-code configuration
#
# - generate `osmv_config.hpp` using this particular build's parameters.
#   Contains per-config info (e.g. version number, build type)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/ RESOURCES_DIR)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/ SHADERS_DIR)
configure_file("${PROJECT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")

add_executable(osmv
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/gl.hpp
    src/gl.cpp
    src/gl_extensions.hpp
    src/gl_extensions.cpp
    src/sdl.hpp
    src/sdl.cpp
    src/imgui_extensions.hpp
    src/imgui_extensions.cpp
    src/shims.hpp
    src/os.hpp
    src/os.cpp

    src/application.hpp
    src/application.cpp
    src/screen.hpp
    src/show_model_screen.hpp
    src/show_model_screen.cpp
    src/loading_screen.hpp
    src/loading_screen.cpp
    src/opensim_wrapper.hpp
    src/opensim_wrapper.cpp
    src/3d_common.hpp
    src/3d_common.cpp
    src/fd_simulation.hpp

    src/osmv.cpp
)
target_include_directories(osmv PUBLIC
    third_party/SDL2-2.0.12/include/
    third_party/glm-0.9.9.8/
    third_party/glew-2.1.0/include/
    third_party/imgui-1.78/
    third_party/stb_image/
    third_party/tomlplusplus-2.3.0/
    ${PROJECT_BINARY_DIR}  # for osmv_config.hpp
)
set(OSMV_OPENSIM_LIBS
    osimCommon
    osimSimulation
    osimActuators
    osimAnalyses
    osimTools
    SimTKcommon
    SimTKmath
    SimTKsimbody
)
target_link_libraries(osmv
    SDL2-static
    glew_s

    ${OSMV_OPENSIM_LIBS}
)
target_compile_options(osmv PUBLIC
    # disable MSVC's (default) permissive mode
    # - This forces MSVC to obey ISO C++
    # - I do this mostly because I like the logic keywords (e.g. `x or b`)
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)
target_compile_features(osmv PUBLIC
    # use C++17
    #
    # OpenSim is a C++11 project, but this project is a CLI/GUI with no
    # external native API exposed downstream
    cxx_std_17
)
target_link_options(osmv PUBLIC
    # on Windows, open the application as a desktop application, rather than as
    # a CLI application
    $<$<CXX_COMPILER_ID:MSVC>:/SUBSYSTEM:windows>
    $<$<CXX_COMPILER_ID:MSVC>:/ENTRY:mainCRTStartup>
)
set_target_properties(osmv PROPERTIES
    CXX_EXTENSIONS OFF
)

# IDE hint: vendored code should be organized into a separate group in the UI
source_group(TREE third_party/)


# HACK: runtime resources are loaded differently in dev vs. install mode
#
# osmv tries to load a runtime configuration by searching from the exe's
# dir upwards until it finds a file called `osmv.toml`. The contents of that
# file depends on whether we're developing or installing osmv
#
# - dev:     `osmv.toml` contains a hard-coded absolute path to the `resources/`
#            dir in the source tree, so that developers can edit resources and
#            reload without having to copy files around or edit their run
#            args in the IDE etc.
#
# - install: `osmv.toml` defines a relative (to the config) path to the
#            resources/ dir, which is copied into the install directory
#            (which depends on where the user installs it, etc.)
#
# - none:   if there's no osmv.toml config, or 'resources' is not defined in
#           osmv.toml, then the implementation tries "$EXE/../resources" as a
#           last-ditch effort
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DEV_osmv.toml.in" "${CMAKE_BINARY_DIR}/osmv.toml")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL_osmv.toml" RENAME "osmv.toml" DESTINATION ".")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

if (WIN32)
    # Windows builds: copy all libraries into the exe dir
    #
    # this is because Windows specifically searches in the exe's dir for
    # libraries. Windows doesn't have the concept of RPATHs for debugging
    #
    # see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll

    foreach(OSL ${OSMV_OPENSIM_LIBS})
        add_custom_command(
            TARGET osmv
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OpenSim_ROOT_DIR}/bin/${OSL}.dll" "$<TARGET_FILE_DIR:osmv>")
    endforeach()

    # Windows install
    #
    # - copy exe + libraries into a single dir
    # - copy any other assets (shaders, icons, etc.)
    install(TARGETS osmv)
    foreach(OSL ${OSMV_OPENSIM_LIBS})
        install(FILES "${OpenSim_ROOT_DIR}/bin/${OSL}.dll" DESTINATION bin)
    endforeach()

endif()

# packaging: use CPack to create standard OS install packages
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${osmv_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${osmv_VERSION_MINOR}")
if (WIN32)
    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/social_youtube_icon_177144.ico")
    set(CPACK_NSIS_UNINSTALL_NAME "uninstall-osmv")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_HELP_LINK "https://github.com/adamkewley/osmv")
    set(CPACK_NSIS_CONTACT "contact@adamkewley.com")
    set(CPACK_NSIS_MODIFY_PATH ON)  # user can modify install locaton
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN osmv)
    set(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\osmv ${osmv_VERSION_MAJOR}.${osmv_VERSION_MINOR}.${osmv_VERSION_PATCH}\\\\osmv.lnk' '\$INSTDIR\\\\bin\\\\osmv.exe'")
endif()
include(CPack)
