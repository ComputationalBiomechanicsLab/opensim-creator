# GetDependencies
#
# Dependency management for OpenSim Creator. Key points:
#
# - Must handle all dependencies as part of the main build, so that
#   this file can be `include`d from the main CMakeLists.txt to "magically"
#   handle all the dependencies in a sane way across all platforms without
#   developers having to fuck around with separate build steps or bootstrap
#   scripts
#
# - Export all dependencies as standard CMake targets with the correct
#   interfaces (includes, linking), such that the main build can just:
#
#       target_link_library(oscgui osc::depX)
#
#     and not have to handle include paths etc. downstream manually
#
# - Must correctly handle multi-configuration builds (looking at you, VS...)
#   so that developers can select "Debug" or whatever in their IDE and
#   their build switches over without having to rebuild parts of the system
#   (e.g. some upstream builds just normalize artifacts into one directory
#    without considering multi-config, which results in rebuilds when a dev
#    switches between two (possibly, already built) configurations)
#
# - Must correctly list all the libraries that downstream must package (e.g.
#   when `install` or `package`ing) to produce a portable (within config params)
#   build

include(ExternalProject)

# Forward CMake arguments from this configuration to the external
# sub-build dependencies
#
# each flag should be checked, because some sub builds screw up if you set
# a flag to an empty value (e.g. GLEW)
if(TRUE)
    if(CMAKE_CXX_COMPILER)
        list(APPEND OSC_DEPENDENCY_OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER})
    endif()
    if(CMAKE_C_COMPILER)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER})
    endif()
    if(CMAKE_CXX_FLAGS)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS})
    endif()
    if(CMAKE_CXX_FLAGS_DEBUG)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG})
    endif()
    if(CMAKE_CXX_FLAGS_MINSIZEREL)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL})
    endif()
    if(CMAKE_CXX_FLAGS_RELEASE)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE})
    endif()
    if(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    endif()
    if(CMAKE_C_FLAGS)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS})
    endif()
    if(CMAKE_C_FLAGS_DEBUG)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG})
    endif()
    if(CMAKE_C_FLAGS_MINSIZEREL)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL})
    endif()
    if(CMAKE_C_FLAGS_RELEASE)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE})
    endif()
    if(CMAKE_C_FLAGS_RELWITHDEBINFO)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO})
    endif()
    if(CMAKE_BUILD_TYPE)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
    endif()
endif()

# DEPENDENCY: OpenGL
#     transitively used by GLEW to load the OpenGL API
find_package(OpenGL REQUIRED)

# DEPENDENCY: GLEW
#     used to bootstrap the OpenGL API, load extensions, etc.
#
#     - always built from source and linked statically
if(TRUE)
    # note: this is a heavily edited version of the CMake file that ships with GLEW
    #
    #    - see build/cmake/CMakeLists.txt

    set(GLEW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glew)

    # get version from config/version
    file(STRINGS ${GLEW_DIR}/config/version GLEW_VERSION_MAJOR_STR REGEX "GLEW_MAJOR[ ]*=[ ]*[0-9]+.*")
    string(REGEX REPLACE "GLEW_MAJOR[ ]*=[ ]*([0-9]+)" "\\1" GLEW_VERSION_MAJOR ${GLEW_VERSION_MAJOR_STR})

    file(STRINGS ${GLEW_DIR}/config/version GLEW_VERSION_MINOR_STRING REGEX "GLEW_MINOR[ ]*=[ ]*[0-9]+.*")
    string(REGEX REPLACE "GLEW_MINOR[ ]*=[ ]*([0-9]+)" "\\1" GLEW_VERSION_MINOR ${GLEW_VERSION_MINOR_STRING})

    file(STRINGS ${GLEW_DIR}/config/version GLEW_VERSION_PATCH_STRING REGEX "GLEW_MICRO[ ]*=[ ]*[0-9]+.*")
    string(REGEX REPLACE "GLEW_MICRO[ ]*=[ ]*([0-9]+)" "\\1" GLEW_VERSION_PATCH ${GLEW_VERSION_PATCH_STRING})

    set(GLEW_VERSION ${GLEW_VERSION_MAJOR}.${GLEW_VERSION_MINOR}.${GLEW_VERSION_PATCH})

    set(GLEW_SRC_FILES ${GLEW_DIR}/src/glew.c)
    set(GLEW_PUBLIC_HEADER_FILES
        ${GLEW_DIR}/include/GL/wglew.h
        ${GLEW_DIR}/include/GL/glew.h
        ${GLEW_DIR}/include/GL/glxew.h
    )
    if(WIN32)
        list(APPEND GLEW_SRC_FILES ${GLEW_DIR}/build/glew.rc)
    endif()

    add_library(glew STATIC ${GLEW_PUBLIC_HEADER_FILES} ${GLEW_SRC_FILES})
    target_include_directories(glew PUBLIC ${GLEW_DIR}/include/)
    target_compile_definitions(glew PRIVATE -DGLEW_NO_GLU)
    target_link_libraries(glew PUBLIC OpenGL::GL)
    set_target_properties(glew PROPERTIES
        VERSION ${GLEW_VERSION}
        COMPILE_DEFINITIONS "GLEW_STATIC"
        INTERFACE_INCLUDE_DIRECTORIES ${GLEW_DIR}/include
        INTERFACE_COMPILE_DEFINITIONS GLEW_STATIC
        POSITION_INDEPENDENT_CODE ON
    )

    # kill security checks which are dependent on stdlib
    if(MSVC)
        target_compile_definitions(glew PRIVATE "GLEW_STATIC;VC_EXTRALEAN")
        target_compile_options(glew PRIVATE -GS-)
    elseif (WIN32 AND ((CMAKE_C_COMPILER_ID MATCHES "GNU") OR (CMAKE_C_COMPILER_ID MATCHES "Clang")))
        target_compile_options (glew PRIVATE -fno-builtin -fno-stack-protector)
    endif()

    unset(GLEW_VERSION)
    unset(GLEW_VERSION_MAJOR_STR)
    unset(GLEW_VERSION_MAJOR)
    unset(GLEW_VERSION_MINOR_STR)
    unset(GLEW_VERSION_MINOR)
    unset(GLEW_VERSION_PATCH_STR)
    unset(GLEW_VERSION_PATCH)
    unset(GLEW_DIR)
    unset(GLEW_SRC_FILES)
    unset(GLEW_PUBLIC_HEADER_FILES)
endif()

# DEPENDENCY: SDL
#     used to provide a cross-platform API to the OS (window management, sound, clipboard, etc.)
if(LINUX)
    # on Linux, dynamically link to system-provided SDL library

    find_library(OSC_SDL2_LOCATION SDL2 REQUIRED)
    add_library(sdl2 SHARED IMPORTED)
    set_target_properties(sdl2 PROPERTIES
        IMPORTED_LOCATION ${OSC_SDL2_LOCATION}
        INTERFACE_INCLUDE_DIRECTORIES /usr/include/SDL2
    )
else()
    # on not-Linux, build SDL from source using the in-tree sources

    # compute library name (e.g. libSDL2.dylib)
    if(WIN32)
        set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2)
        set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2d)
    elseif(APPLE)
        set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0)
        set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2)
    else()
        set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0)
        set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0d)
    endif()

    # compute possible build byproducts (needed for build systems that check
    # this, like ninja)
    if(TRUE)
        set(SDL2_BINDIR "sdl2-project-prefix/src/sdl2-project-build")
        list(APPEND SDL2_BUILD_BYPRODUCTS
            "${SDL2_BINDIR}/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}"
            "${SDL2_BINDIR}/${DEBUG_LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}"
            "${SDL2_BINDIR}/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${SDL2_BINDIR}/${DEBUG_LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
        unset(SDL2_BINDIR)
    endif()

    # on non-Linux, build SDL from source and package it with the install
    ExternalProject_Add(sdl2-project
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2
        CMAKE_CACHE_ARGS ${OSC_DEPENDENCY_CMAKE_ARGS}
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS "${SDL2_BUILD_BYPRODUCTS}"
    )
    ExternalProject_Get_Property(sdl2-project SOURCE_DIR)
    ExternalProject_Get_Property(sdl2-project BINARY_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

    add_library(sdl2 SHARED IMPORTED)
    add_dependencies(sdl2 sdl2-project)

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(SDL2_LIB_SUFFIX "d")
    else()
        set(SDL2_LIB_SUFFIX "")
    endif()

    set_target_properties(sdl2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
    )

    if(${OSC_GENERATOR_IS_MULTI_CONFIG})
        set_target_properties(sdl2 PROPERTIES
            IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_RELEASE ${BINARY_DIR}/Release/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_RELEASE ${BINARY_DIR}/Release/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
    else()
        set_target_properties(sdl2 PROPERTIES
            IMPORTED_LOCATION ${BINARY_DIR}/${LIBNAME}${SDL2_LIB_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB ${BINARY_DIR}/${LIBNAME}${SDL2_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
    endif()

    unset(SDL2_LIB_SUFFIX)
    unset(SOURCE_DIR)
    unset(BINARY_DIR)
    unset(LIBNAME)
    unset(DEBUG_LIBNAME)
    unset(SDL2_BUILD_BYPRODUCTS)
endif()

# DEPENDENCY: glm
#     header-only library, used for OpenGL-friendly vector maths
if(TRUE)
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)
endif()

# DEPENDENCY: imgui
#     small low-dependency library for rendering interactive panels (buttons, sliders, etc.)
#
#     - built from source with SDL2 + OpenGL backend
#     - in tree, to reduce some of the faffing around to deal with CMake3.5 bugs
if(TRUE)
    add_library(imgui STATIC
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_demo.cpp  # useful for osc devs to see what widgets are available
        third_party/imgui/backends/imgui_impl_opengl3.cpp
        third_party/imgui/backends/imgui_impl_sdl.cpp
    )
    target_link_libraries(imgui PUBLIC sdl2 glew glm)
    target_include_directories(imgui PUBLIC third_party/ third_party/imgui/)
endif()

# DEPENDENCY: stb
#     header-only library, used to read/write asset files (images, sounds)
if(TRUE)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
endif()

# DEPENDENCY: tomlplusplus
#     header-only library, used to parse toml config files
if(TRUE)
    add_library(tomlplusplus INTERFACE)
    target_include_directories(tomlplusplus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus)
endif()

# DEPENDENCY: span-lite
#     header-only library, shims std::span (C++20) into C++17
if(TRUE)
    add_library(span-lite INTERFACE)
    target_include_directories(span-lite INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/span-lite)
endif()

# DEPENDENCY: nativefiledialog
#     OS-dependent library for showing file/folder open/save dialogs
if(TRUE)
    if(LINUX)
        include(FindPkgConfig)

        add_library(nativefiledialog STATIC
            third_party/nativefiledialog/src/nfd_gtk.c
            third_party/nativefiledialog/src/nfd_common.c
        )

        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

        target_link_libraries(nativefiledialog INTERFACE ${GTK3_LIBRARIES})
        target_include_directories(nativefiledialog
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src/include
            PRIVATE ${GTK3_INCLUDE_DIRS}
            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src/include
        )
    elseif(WIN32)
        add_library(nativefiledialog STATIC
            third_party/nativefiledialog/src/nfd_win.cpp
            third_party/nativefiledialog/src/nfd_common.c
        )
        target_include_directories(nativefiledialog
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src/include
            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src/include
        )
    elseif(APPLE)
        add_library(nativefiledialog STATIC
            third_party/nativefiledialog/src/nfd_cocoa.m
            third_party/nativefiledialog/src/nfd_common.c
        )
        target_include_directories(nativefiledialog
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src/include
            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nativefiledialog/src/include
        )
        target_link_libraries(nativefiledialog INTERFACE "-framework Cocoa")
    else()
        message(FATAL_ERROR "no implementation of nfd.h available on this platform: required for native platform file dialogs")
    endif()

    set_target_properties(nativefiledialog PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# DEPENDENCY: OpenSim
#     primary dependency for performing musculoskeletal simulations
#
#     - linked as a separate CMake dependency
#
#     - to customize this, set CMAKE_PREFIX_PATH to the appropriate cmake/ dir in
#       an OpenSim install (e.g. OpenSim4.1/cmake/, OpenSim4.1/lib/cmake/,
#       C:\OpenSim 4.2\)
#
#     - you can build your own OpenSim install dir from source by checking out
#       OpenSim, building it (follow their instructions) and building the `install`
#       target
#
#     - search priority (stolen from: https://cmake.org/cmake/help/v3.0/command/find_package.html)
#
#         - CMAKE_PREFIX_PATH (cache var)
#         - CMAKE_FRAMEWORK_PATH (cache var)
#         - CMAKE_APPBUNDLE_PATH (cache var)
#         - OpenSim_DIR (env var)
#         - CMAKE_PREFIX_PATH (env var)
#         - CMAKE_FRAMEWORK_PATH (env var)
#         - CMAKE_APPBUNDLE_PATH (env var)
#         - HINTS
#         - PATH (env var)
#         - some b.s. to do with recently built projects in the CMake GUI
#         - user's package registry (see: https://cmake.org/cmake/help/v3.0/manual/cmake-packages.7.html#user-package-registry)
#         - CMAKE_SYSTEM_PREFIX_PATH (platform var)
#         - CMAKE_SYSTEM_FRAMEWORK_PATH (platform var)
#         - CMAKE_SYSTEM_APPBUNDLE_PATH (platform var)
#         - system package registry (see: https://cmake.org/cmake/help/v3.0/manual/cmake-packages.7.html#system-package-registry)
#         - PATHS (configured here: see below)
#
if(TRUE)
    find_package(OpenSim
        REQUIRED
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/opensim-install"  # generated by OSC's build script
            "C:\\OpenSim "
        PATH_SUFFIXES "4.2" "4.3" "4.4"
    )
    set(OSC_OPENSIM_LIBS
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
endif()

# `osc::all-deps`: all libraries osc should link to
add_library(osc-all-deps INTERFACE)
target_link_libraries(osc-all-deps INTERFACE

    # linking to the OSes OpenGL driver/loader
    OpenGL::GL

    # for loading the OpenGL API at runtime
    glew

    # for application matrix maths, etc. necessary to use the OpenGL API
    glm

    # OS integration (window creation, sounds, input)
    sdl2

    # OS integration for file dialogs (Open, Save As, etc.)
    nativefiledialog

    # GUI component rendering (text boxes, sliders, etc.)
    imgui

    # GUI asset parsing (image files, sound files, etc.)
    stb

    # config file parsing
    tomlplusplus

    # shim for C++20's std::span
    span-lite

    # OpenSim API
    ${OSC_OPENSIM_LIBS}
)

# ----- OSC_LIB_FILES_TO_COPY: variable that is set to all lib files to be copied -----

if(WIN32)
    # in Windows, copy all DLLs in the OpenSim install dir
    #
    # this is necessary because there's a bunch of transitive DLLs that
    # must be included (e.g. liblapack, libgfortran). On the other systems,
    # these might be provided by the base OS

    file(GLOB OPENSIM_DLLS LIST_DIRECTORIES false CONFIGURE_DEPENDS ${OpenSim_ROOT_DIR}/bin/*.dll)
    foreach(OPENSIM_DLL ${OPENSIM_DLLS})
        list(APPEND OSC_LIB_FILES_TO_COPY ${OPENSIM_DLL})
    endforeach()
    unset(OPENSIM_DLL)
    unset(OPENSIM_DLLS)
else()
    # on Linux/mac, only copy the direct dependencies (it's assumed that
    # the OSes provide the rest)
    foreach(OPENSIM_LIB ${OSC_OPENSIM_LIBS})
        list(APPEND OSC_LIB_FILES_TO_COPY $<TARGET_FILE:${OPENSIM_LIB}>)
    endforeach()
endif()

# copy SDL2 lib if on Windows/Mac
if(NOT LINUX)
    list(APPEND OSC_LIB_FILES_TO_COPY $<TARGET_FILE:sdl2>)
endif()
