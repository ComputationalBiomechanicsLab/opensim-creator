cmake_minimum_required(VERSION 3.5)

project(osmv VERSION 0.0.1 LANGUAGES CXX)


# -------------- top-level configuration ---------------- #

# shim CMake for older versions:
#
#     - all shimming should be close to the variables set by this, for easier debugging
if((${CMAKE_MAJOR_VERSION} LESS 3) OR (${CMAKE_MINOR_VERSION} LESS 13))
    message(WARNING "you are using an older version of CMake. This is fine (for now), but you should upgrade")
    set(OSMV_USING_MODERN_CMAKE 0)
else()
    set(OSMV_USING_MODERN_CMAKE 1)
endif()

get_property(OSMV_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

set(OSMV_USE_LINTER OFF CACHE BOOL "enable/disable running `clang-tidy` (linter) while building")
set(OSMV_USE_IWYU OFF CACHE BOOL "enable/disable running `iwyu` (include-what-you-use) while building")
set(OSMV_IWYU_PATH "iwyu" CACHE STRING "path to the `iwyu` executable (if not on the PATH)")
set(OSMV_REPO_PROVIDER "https://github.com" CACHE STRING "URL (or local path, if you setup your mirrors properly) to a git repo provider for external projects")

if(${OSMV_USING_MODERN_CMAKE})
    cmake_policy(SET CMP0072 NEW)  # use newer OpenGL linking strategy
endif()
set(OpenGL_GL_PREFERENCE "GLVND")  # use newer OpenGL linking strategy

if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a UNIX system that may not be supported yet - beware!")
    endif()
endif()



# --------------- dependency management ----------------- #

include(ExternalProject)

# DEPENDENCY: OpenGL
#     transitively used by GLEW to load the OpenGL API
find_package(OpenGL REQUIRED)

# DEPENDENCY: GLEW
#     used to bootstrap the OpenGL API, load extensions, etc.
#
#     - always built from source and linked statically
if(TRUE)
    ExternalProject_Add(glew-project
        URL "${OSMV_REPO_PROVIDER}/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip"
        URL_HASH "SHA256=a9046a913774395a095edcc0b0ac2d81c3aacca61787b39839b941e9be14e0d4"
        PREFIX ""
        CONFIGURE_COMMAND cmake ../glew-project/build/cmake
        BUILD_COMMAND cmake --build . --target glew_s
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
    )
    ExternalProject_Get_Property(glew-project SOURCE_DIR)
    ExternalProject_Get_Property(glew-project BINARY_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

    add_library(osmv-glew STATIC IMPORTED)
    add_dependencies(osmv-glew glew-project-build)

    if(WIN32)
        set(LIBNAME "libglew32${CMAKE_STATIC_LIBRARY_SUFFIX}")
        set(DEBUG_LIBNAME "libglew32d${CMAKE_STATIC_LIBRARY_SUFFIX}")
    else()
        set(LIBNAME "${CMAKE_STATIC_LIBRARY_PREFIX}GLEW${CMAKE_STATIC_LIBRARY_SUFFIX}")
        set(DEBUG_LIGNAME "${CMAKE_STATIC_LIBRARY_PREFIX}GLEWd${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    set_target_properties(osmv-glew PROPERTIES
        IMPORTED_LOCATION ${BINARY_DIR}/lib/${LIBNAME}
        INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
    )
    if(${OSMV_GENERATOR_IS_MULTI_CONFIG})
        set_target_properties(osmv-glew PROPERTIES
            IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/lib/Debug/${DEBUG_LIBNAME}
            IMPORTED_LOCATION_RELWITHDEBINFO ${BINARY_DIR}/lib/RelWithDebInfo/${LIBNAME}
            IMPORTED_LOCATION_MINSIZEREL ${BINARY_DIR}/lib/MinSizeRel/${LIBNAME}
            IMPORTED_LOCATION_RELEASE ${BINARY_DIR}/lib/Release/${LIBNAME}
        )
    endif()

    unset(SOURCE_DIR)
    unset(BINARY_DIR)
    unset(LIBNAME)
    unset(DEBUG_LIBNAME)
endif()

# DEPENDENCY: SDL
#     used to provide a cross-platform API to the OS (window management, sound, clipboard, etc.)
if(LINUX)
    # on Linux, dynamically link to system-provided SDL library

    find_library(OSMV_SDL2_LOCATION SDL2 REQUIRED)
    add_library(osmv-sdl2 SHARED IMPORTED)
    set_target_properties(osmv-sdl2 PROPERTIES
        IMPORTED_LOCATION ${OSMV_SDL2_LOCATION}
        INTERFACE_INCLUDE_DIRECTORIES /usr/include/SDL2
    )
else()
    # on non-Linux, build SDL from source and package it with the install

    ExternalProject_Add(sdl2-project
        GIT_REPOSITORY "${OSMV_REPO_PROVIDER}/adamkewley/SDL2"
        GIT_TAG "0330c566b6d9d5fdf1d9bb6d0a8bd2ba2b4f9407"  # tag: SDL2-2.0.14
        GIT_SUBMODULES ""
        BUILD_COMMAND cmake --build . --target SDL2
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
    )
    ExternalProject_Get_Property(sdl2-project SOURCE_DIR)
    ExternalProject_Get_Property(sdl2-project BINARY_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

    add_library(osmv-sdl2 SHARED IMPORTED)
    add_dependencies(osmv-sdl2 sdl2-project-build)

    set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0d${CMAKE_SHARED_LIBRARY_SUFFIX})

    set_target_properties(osmv-sdl2 PROPERTIES
        IMPORTED_LOCATION ${BINARY_DIR}/${LIBNAME}
        INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
    )
    if(${OSMV_GENERATOR_IS_MULTI_CONFIG})
        set_target_properties(osmv-glew PROPERTIES
            IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}
            IMPORTED_LOCATION_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}
            IMPORTED_LOCATION_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}
            IMPORTED_LOCATION_RELEASE ${BINARY_DIR}/Release/${LIBNAME}
        )
    endif()

    unset(SOURCE_DIR)
    unset(BINARY_DIR)
    unset(LIBNAME)
    unset(DEBUG_LIBNAME)
endif()

# DEPENDENCY: glm
#     header-only library, used for OpenGL-friendly vector maths
if(TRUE)
    ExternalProject_Add(glm-project
        GIT_REPOSITORY "${OSMV_REPO_PROVIDER}/g-truc/glm"
        GIT_TAG "bf71a834948186f4097caa076cd2663c69a10e1e"  # tag: 0.9.9.8
        GIT_SUBMODULES ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
    )
    ExternalProject_Get_Property(glm-project SOURCE_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR})

    add_library(osmv-glm INTERFACE)
    add_dependencies(osmv-glm glm-project-build)  # so the headers are populated
    target_include_directories(osmv-glm INTERFACE ${SOURCE_DIR})

    unset(SOURCE_DIR)
endif()

# DEPENDENCY: imgui
#     small low-dependency library for rendering interactive panels (buttons, sliders, etc.)
#
#     - built from source with SDL2 + OpenGL backend
#     - in tree, to reduce some of the faffing around to deal with CMake3.5 bugs
if(TRUE)
    add_custom_target(imgui-configured
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h third_party/imgui-1.78/imconfig.h
    )
    add_library(osmv-imgui STATIC
        third_party/imgui-1.78/imgui.cpp
        third_party/imgui-1.78/imgui_draw.cpp
        third_party/imgui-1.78/imgui_widgets.cpp
        third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
        third_party/imgui-1.78/examples/imgui_impl_sdl.cpp
    )
    target_link_libraries(osmv-imgui PUBLIC osmv-sdl2 osmv-glew osmv-glm)
    target_include_directories(osmv-imgui PUBLIC third_party/imgui-1.78/)
endif()

# DEPENDENCY: stb_image
#     header-only library, used to read/write images
if(TRUE)
    ExternalProject_Add(stb-project
        GIT_REPOSITORY "${OSMV_REPO_PROVIDER}/nothings/stb"
        GIT_TAG "b42009b3b9d4ca35bc703f5310eedc74f584be58"
        GIT_SUBMODULES ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
    )
    ExternalProject_Get_Property(stb-project SOURCE_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR})

    add_library(osmv-stb-image INTERFACE)
    add_dependencies(osmv-stb-image stb-project-build)  # so the headers are populated
    target_include_directories(osmv-stb-image INTERFACE ${SOURCE_DIR})

    unset(SOURCE_DIR)
endif()

# DEPENDENCY: tomlplusplus
#     header-only library, used to parse toml config files
if(TRUE)
    ExternalProject_Add(tomlplusplus-project
        GIT_REPOSITORY "${OSMV_REPO_PROVIDER}/marzer/tomlplusplus"
        GIT_TAG "4face4d5bf16326aca0da1fb33876dbca63b6e2f"  # tag: v2.3.0
        GIT_SUBMODULES ""  # don't care about the submodules
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        STEP_TARGETS build
    )
    ExternalProject_Get_Property(tomlplusplus-project SOURCE_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR})

    add_library(osmv-tomlplusplus INTERFACE)
    add_dependencies(osmv-tomlplusplus tomlplusplus-project-build)  # so the headers are populated
    target_include_directories(osmv-tomlplusplus INTERFACE ${SOURCE_DIR})

    unset(SOURCE_DIR)
endif()

# DEPENDENCY: OpenSim
#     primary dependency for performing musculoskeletal simulations
#
#     - linked as a separate CMake dependency
#
#     - to customize this, set CMAKE_PREFIX_PATH to the appropriate cmake/ dir in
#       an OpenSim install (e.g. OpenSim4.1/cmake/, OpenSim4.1/lib/cmake/)
#
#     - you can build your own OpenSim install dir from source by checking out
#       OpenSim, building it (follow their instructions) and building the `install`
#       target
#
#     - by default, this `find_package` call will typically just find the installed
#       OpenSim on your system
find_package(OpenSim REQUIRED)
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


# ------------ osmv build target configuration ---------- #

# OSMV: osmv_config.hpp
#     configuration file that contains configuration-time vals (e.g. version, libs)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")

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
target_link_libraries(osmv PRIVATE
    # in gcc/clang the <filesystem> implementation must be explicitly linked
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>

    ${OPENGL_LIBRARIES}

    osmv-glew
    osmv-sdl2
    osmv-glm
    osmv-imgui
    osmv-stb-image
    osmv-tomlplusplus

    ${OSMV_OPENSIM_LIBS}
)
target_include_directories(osmv PRIVATE
    # so that "osmv_config.hpp" can be #included
    ${PROJECT_BINARY_DIR}
)
target_compile_options(osmv PRIVATE

    # msvc flags
    $<$<CXX_COMPILER_ID:MSVC>:
        /permissive-  # I like `and` and `not` keywords: sue me
        /wd4996  # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4455  # disable warning 4455: incorrectly flags std::string::literals::"s"
        /MP  # parallel-build
        /W3  # turn warning level up to "production grade" (note: /W4 explodes because of Simbody)
        /WX  # treat all warnings as errors
    >

    # gcc/clang flags
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
        -Wno-sign-compare  # disable sign comparison warning: broken by STB headers
    >

    # clang-only
    $<$<CXX_COMPILER_ID:Clang>:
        -Wno-cast-align
    >

    # gcc-only
    $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused  # disable no unused variable warning: broken by STB headers
    >

    # older CMakes do not support C++17 config
    $<$<NOT:$<BOOL:${OSMV_USING_MODERN_CMAKE}>>:-std=c++17>
)
set_target_properties(osmv PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED YES
    EXPORT_COMPILE_COMMANDS ON  # needed by clang-tidy
    CLANG_TIDY $<${OSMV_USE_LINTER}:"clang-tidy">
)
if(OSMV_USE_ISYU)
    set_target_properties(osmv PROPERTIES INCLUDE_WHAT_YOU_USE "${OSMV_IWYU_PATH}")
endif()
if(LINUX)
    # Linux RPATH:
    #     - at installation time, Linux install dir is:
    #         bin/
    #         lib/*.so
    #     - so set the install RPATH to reflect this
    set_target_properties(osmv PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
endif()

if(${OSMV_USING_MODERN_CMAKE})
    target_compile_features(osmv PUBLIC
        cxx_std_17
    )
endif()

if(MSVC)
    target_link_options(osmv PUBLIC
        # on Windows, open the application as a desktop application, rather than as
        # a CLI application
        $<$<CXX_COMPILER_ID:MSVC>:
            /SUBSYSTEM:windows  # open as a desktop app, not CLI
            /ENTRY:mainCRTStartup  # as above
            /ignore:4217  # ImGui uses locally defined symbols
        >
    )
endif()

# collect a list of all the libraries that need to be copied when developing/installing
# osmv
foreach(OPENSIM_LIB ${OSMV_OPENSIM_LIBS})
    list(APPEND OSMV_LIB_FILES_TO_COPY $<TARGET_FILE:${OPENSIM_LIB}>)
endforeach()
if(NOT LINUX)
    list(APPEND OSMV_LIB_FILES_TO_COPY $<TARGET_FILE:osmv-sdl2>)
endif()


# ------------ osmv development nice-to-haves ----------- #

# generate a dev-centric `osmv.toml`
#
#     - this causes osmv to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DEV_osmv.toml.in" "${CMAKE_BINARY_DIR}/osmv.toml")

# (on Windows): copy libraries into the same dir as osmv.exe
#
#     - this is so that devs can run osmv.exe from an IDE without having to
#       copy libraries around manually etc.
#
#     - this isn't necessary with Unix'es because CMake sets the RPATH up to
#       point to wherever the libraries are located
#
#     - at install time, the libraries are copied into the install location
if (WIN32)
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        add_custom_command(
            TARGET osmv
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_FILE_TO_COPY}" "$<TARGET_FILE_DIR:osmv>"
        )
    endforeach()
endif()



# ---------- installation / packaging (CPack) ----------- #

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${osmv_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${osmv_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_DESCRIPTION "A thin GUI for OpenSim - primarily for development use")

# install a user-centric `osmv.toml`
#
#     - in contrast to the dev-centric one, this loads resources from the installation dir,
#       which has a known path relative to the osmv executable (../resources)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL_osmv.toml" RENAME "osmv.toml" DESTINATION ".")

# install `resources/`
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

# Windows install/packaging:
#
#     - copy osmv.exe + all DLLs into the bin/ dir
#
#     - this creates a "fat" build in bin/, where all the necessary libraries are just
#       packaged with the install (so users only have to install one thing)
#
#     - see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
#
#     - packaging: uses NSIS.exe : get it from https://nsis.sourceforge.io/Download
if (WIN32)
    # install osmv.exe
    install(TARGETS osmv)

    # install DLLs
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION bin)
    endforeach()

    # set up NSIS packaging
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

# Linux install/packaging:
#
#     - copy osmv + libraries into a standard-layout dir
#           bin/  # executables
#           lib/  # libraries (.so)
#           resources/  # arch-independent runtime resources
#
#     - this dir is "done", and could be sent as a ZIP to an end-user
#
#     - **BUT** the way that OpenSim/osmv is built assumes the user has all the
#       necessary runtime libraries available
#
#     - so the packaging step creates a .deb file that declares which runtime dependencies
#       a client should have (or automatically acquire via apt)
#
#     - note: the package assumes the user has a compatible OpenGL driver installed, servers
#             can try to "fake" this by running:
#
#           apt-get install libopengl0 libglx0 libglu1-mesa
if (LINUX)
    # install osmv
    install(TARGETS osmv DESTINATION bin/)

    # install *bundled* SOs
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION lib/)
    endforeach()

    # set up DEB packaging
    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osmv)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libblas3, liblapack3, libstdc++6")  # TODO: this doesn't handle OSMV_USE_SYSTEM_SDL
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    configure_file("${PROJECT_SOURCE_DIR}/postinst.in" "postinst" @ONLY)  # create symlink /usr/local/bin/osmv --> /opt/osmv/bin/osmv
    configure_file("${PROJECT_SOURCE_DIR}/postrm.in" "postrm" @ONLY)  # destroy symlink created during install
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
