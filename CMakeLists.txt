cmake_minimum_required(VERSION 3.5)

project(osmv VERSION 0.0.1 LANGUAGES CXX)

set(OSMV_USE_LINTER OFF CACHE BOOL "enable/disable running clang-tidy while building")

if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a system that may not be supported yet - beware!")
    endif()
endif()

# needed by external linters (e.g. clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GLEW (OpenGL): build from source
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)

if (LINUX)
    # on Linux specifically, use a system-provided SDL2
    #
    # SDL2 handles *a lot* of distro-specific bs (e.g. X vs Wayland), so
    # it's advantageous to dynamically link to whatever build of SDL2 that distro
    # provides.

    set(OSMV_SDL2_LIB SDL2)
    set(OSMV_SDL2_INCLUDE /usr/include/SDL2)
else()
    # on non-Linux, just build SDL2 from source and statically link it.
    #
    # this results in a more brittle binary, but is easier to onboard developers
    # with, which is prio #1 for this (new) project

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)
    set(OSMV_SDL2_LIB SDL2-static)
    set(OSMV_SDL2_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2-2.0.12/include/)
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

# generate osmv_config.hpp: used by the codebase (e.g.) for the version string
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")

# imgui: build only the parts osmv needs as a static library
add_library(osmvimgui STATIC
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp
)
target_include_directories(osmvimgui PRIVATE
    ${OSMV_SDL2_INCLUDE}
    third_party/imgui-1.78/
    third_party/glm-0.9.9.8/
    third_party/glew-2.1.0/include/
)

add_executable(osmv
	${CMAKE_BINARY_DIR}/osmv_config.hpp

    src/os.hpp
    src/os.cpp
    src/sdl.hpp
    src/sdl.cpp
    src/gl.hpp
    src/gl.cpp
    src/imgui_extensions.hpp
    src/imgui_extensions.cpp
    src/shims.hpp
    src/cfg.hpp
    src/cfg.cpp
    src/opensim_wrapper.hpp
    src/opensim_wrapper.cpp
    src/3d_common.hpp
    src/3d_common.cpp
    src/fd_simulation.hpp
    src/fd_simulation.cpp
    src/renderer.hpp
    src/renderer.cpp

    src/application.hpp
    src/application.cpp

    src/screen.hpp
    src/show_model_screen.hpp
    src/show_model_screen.cpp
    src/loading_screen.hpp
    src/loading_screen.cpp
    src/splash_screen.hpp
    src/splash_screen.cpp

    src/osmv.cpp
)
target_include_directories(osmv PRIVATE
    ${OSMV_SDL2_INCLUDE}
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
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>
    ${OSMV_SDL2_LIB}
    glew_s
    osmvimgui
    ${OSMV_OPENSIM_LIBS}
)
target_compile_options(osmv PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
		/permissive-  # I like `and` and `not` keywords: sue me
		/wd4996  # simbody uses a deprecated iterator class
		/wd4455  # 's' is an allowed literal suffix because it's from the STL
		/MP  # parallelize MSVC builds
		/W3  # production-grade warnings (W4 explodes because of Simbody)
		/WX  # treat warnings like errors
	>

    # turn on warnings / errors
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall
        -Wextra
        -pedantic
        -Werror
        -Wuninitialized
        -Winit-self
        -Wshadow
        -Wcast-align
        -Wwrite-strings
        -Wdangling-else
        -Wdate-time
        -Wmissing-declarations
        -Wno-multichar
        -Wredundant-decls
        -Winline
        -Wvla
        -Wdisabled-optimization
        -Wpacked
        -Wmissing-noreturn
        -Wno-unused  # PITA to debug a prototype with this enabled
        -Wno-sign-compare  # broken by STB
    >
    $<$<CXX_COMPILER_ID:GNU>:
        #-Wcast-function-type
        #-Wint-in-bool-context
        #-Wlogical-op
        #-Wmaybe-uninitialized
        #-Wnormalized
        # -Wcast-qual  broken by STB
        # -Wconversion  broken by STB
    >
    $<$<CXX_COMPILER_ID:Clang>:
        -Wno-cast-align
    >

    # C++17 is set here for GNU/Clang because older versions of CMake
    # don't support `target_compile_features` and older versions again
    # don't support CXX_STANDARD 17, whereas quite old versions blindly
    # accept compiler flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -std=c++17
    >
)

set_target_properties(osmv PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED YES

    CLANG_TIDY $<OSMV_USE_LINTER:"clang-tidy">
)
if(LINUX)
    # at install, Linux install dir is:
    #
    #     bin/osmv
    #     lib/*.so
    set_target_properties(osmv PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
endif()

# these features only work in newer CMakes' than some devs have, so
# they might break on newer systems
if(MSVC)  # HACK: "newer systems" is Windows for now, where devs can install whatever
    target_compile_features(osmv PUBLIC
        cxx_std_17  # note: this is redundantly set above
    )
    target_link_options(osmv PUBLIC
        # on Windows, open the application as a desktop application, rather than as
        # a CLI application
        $<$<CXX_COMPILER_ID:MSVC>:
			/SUBSYSTEM:windows  # open as a desktop app, not CLI
			/ENTRY:mainCRTStartup  # as above
			/ignore:4217  # ImGui uses locally defined symbols
		>
    )
    source_group(TREE third_party/)  # IDE hint: group third_party source together
endif()

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
