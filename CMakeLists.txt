cmake_minimum_required(VERSION 3.13)

project(osmv VERSION 0.0.1)

if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    endif()

    # if it's a unix variant that is non-Linux, it isn't (currently)
    # supported. It *might* compile + work, though
endif()

# glew: build from source and statically link: works on all platforms
#       without introducing potentially breaking dependencies into the
#       runtime
add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)

if (LINUX)
    # on Linux specifically, use a system-provided SDL2
    #
    # SDL2 handles *a lot* of distro-specific bs (e.g. X vs Wayland), so
    # it's advantageous to just link to whatever build of SDL2 that distro
    # provides.
    #
    # I'd prefer to just statically build + link SDL2, because that would
    # result in a uniform build across all platforms, but SDL2 itself links
    # to potentially volatile OS libraries and statically linking it would
    # cause osmv to be dependent on libraries that change between distros.

    set(SDL2_LIB SDL2)
    set(SDL2_INCLUDE /usr/include/SDL2)
else()
    # on non-Linux, just build SDL2 from source and statically link
    # it
    #
    # on Windows, this is more convenient because it's a PITA to tell
    # every developer how to setup SDL2 on their system. The dependency
    # brittleness problem (that Linux has) is less of a problem under
    # Windows, which has a mostly stable OS API for GUIs.
    #
    # other Unixes (e.g. Apple) aren't (yet) packaged for. Users will
    # be able to build on OSX but the packages produced by static
    # linking might be unstable. Needs investigation.

    add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)
    set(SDL2_LIB SDL2-static)
    set(SDL2_INCLUDE third_party/SDL2-2.0.12/include/)
endif()

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
configure_file("${PROJECT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")

if(LINUX)
    # linux installs are arranged like this:
    #
    # <install_dir>/
    #     bin/
    #         osmv
    #     lib/
    #         *.so
    #
    # so set the installed RPATH to search in "../lib" for bundled libraries
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

add_executable(osmv
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/os.hpp
    $<$<BOOL:${WIN32}>:src/os_win32.cpp>
    $<$<BOOL:${LINUX}>:src/os_linux.cpp>

    src/gl.hpp
    src/gl.cpp
    src/gl_extensions.hpp
    src/gl_extensions.cpp
    src/sdl.hpp
    src/sdl.cpp
    src/imgui_extensions.hpp
    src/imgui_extensions.cpp
    src/shims.hpp
    src/globals.hpp
    src/globals.cpp

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
    ${SDL2_INCLUDE}
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
    osimLepton
    SimTKcommon
    SimTKmath
    SimTKsimbody
)
target_link_libraries(osmv
    # in gcc, the <filesystem> implementation must be explicitly linked separately
    $<$<CXX_COMPILER_ID:GNU>:stdc++fs>
    ${SDL2_LIB}
    glew_s
    ${OSMV_OPENSIM_LIBS}
)
target_compile_options(osmv PUBLIC
    # disable MSVC's (default) permissive mode
    #
    # - This forces MSVC to obey ISO C++
    # - I do this mostly because I like the logic keywords (e.g. `x or b`)
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)
target_compile_features(osmv PUBLIC
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
    CXX_STANDARD_REQUIRED YES
)

source_group(TREE third_party/)  # IDE hint: group third_party code together

# development configuration: generate a configuration that loads resources from the
# developer's source tree
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DEV_osmv.toml.in" "${CMAKE_BINARY_DIR}/osmv.toml")

if (WIN32)
    # Windows development: copy OpenSim libraries (if they change) after each build
    #
    # this is necessary so that developers can boot osmv.exe from their IDE without
    # having to manually copy DLLs around
    foreach(OSL ${OSMV_OPENSIM_LIBS})
        add_custom_command(
            TARGET osmv
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OpenSim_ROOT_DIR}/bin/${OSL}.dll" "$<TARGET_FILE_DIR:osmv>")
    endforeach()
endif()



# ----------------- installation / packaging (CPack) -----------------

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${osmv_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${osmv_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_DESCRIPTION "A thin GUI for OpenSim - primarily for development use")

# install the runtime configuration used by installed deployments
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL_osmv.toml" RENAME "osmv.toml" DESTINATION ".")

# install the runtime resources used by osmv.exe
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

if (WIN32)
    # Windows install: copy osmv.exe + libraries into the bin/ dir
    #
    # this creates a "fat" build in bin/, where the Windows dynamic linker can just
    # find everything in one place
    #
    # see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
    install(TARGETS osmv)
    foreach(OSL ${OSMV_OPENSIM_LIBS})
        install(FILES "${OpenSim_ROOT_DIR}/bin/${OSL}.dll" DESTINATION bin)
    endforeach()

    # windows packaging: use NSIS.exe to create a Windows installer
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

if (LINUX)
    # Linux install: copy everything into a standard layout dir:
    #
    # - bin/: executables
    # - lib/: libraries
    # - resources/: arch-independent runtime resources

    # osmv --> bin/osmv
    install(TARGETS osmv DESTINATION bin/)

    # (e.g.) osimCommon.so --> lib/osimCommon.so
    foreach(OSL ${OSMV_OPENSIM_LIBS})
        get_filename_component(OSL_REALPATH "${OpenSim_ROOT_DIR}/lib/lib${OSL}.so" REALPATH)
        install(PROGRAMS "${OSL_REALPATH}" DESTINATION lib/)
    endforeach()

    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osmv)
    # this assumes the user's computer already has OpenGL, GLX, and GLU, which tend to
    # be packaged with OpenGL drivers (usually... ;))
    #
    # software implementations of those APIs can be retrieved with:
    #     apt-get install libopengl0 libglx0 libglu1-mesa
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libblas3, liblapack3, libstdc++6")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # installation scripts that setup/teardown shortcuts, symlinks,
    # etc.
    configure_file("${PROJECT_SOURCE_DIR}/postinst.in" "postinst" @ONLY)
    configure_file("${PROJECT_SOURCE_DIR}/postrm.in" "postrm" @ONLY)
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
