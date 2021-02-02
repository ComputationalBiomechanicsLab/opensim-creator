# OSMV_GetDependencies
#
# Dependency management for OSMV. Key points:
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
#       target_link_library(osmv osmv-some-dependency)
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
# - Must work in CMake 3.5, because Ubuntu Xenial still packages that and
#   some team members still use Xenial
#
# - Must correctly list all the libraries that downstream must package (e.g.
#   when `install` or `package`ing) to produce a portable (within config params)
#   build

include(ExternalProject)


if(NOT OSMV_REPO_PROVIDER)
    set(OSMV_REPO_PROVIDER "https://github.com")
endif()


# Forward CMake arguments from this configuration to the external 
# sub-build dependencies
#
# each flag should be checked, because some sub builds screw up if you set
# a flag to an empty value (e.g. GLEW)

if(CMAKE_CXX_COMPILER)
    list(APPEND OSMV_DEPENDENCY_OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER})
endif()
if(CMAKE_C_COMPILER)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER})
endif()
if(CMAKE_CXX_FLAGS)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS})
endif()
if(CMAKE_CXX_FLAGS_DEBUG)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG})
endif()
if(CMAKE_CXX_FLAGS_MINSIZEREL)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL})
endif()
if(CMAKE_CXX_FLAGS_RELEASE)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE})
endif()
if(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()
if(CMAKE_C_FLAGS)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS})
endif()
if(CMAKE_C_FLAGS_DEBUG)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG})
endif()
if(CMAKE_C_FLAGS_MINSIZEREL)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL})
endif()
if(CMAKE_C_FLAGS_RELEASE)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE})
endif()
if(CMAKE_C_FLAGS_RELWITHDEBINFO)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO})
endif()
if(CMAKE_BUILD_TYPE)
    list(APPEND OSMV_DEPENDENCY_CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
endif()


# DEPENDENCY: OpenGL
#     transitively used by GLEW to load the OpenGL API
find_package(OpenGL REQUIRED)

# DEPENDENCY: GLEW
#     used to bootstrap the OpenGL API, load extensions, etc.
#
#     - always built from source and linked statically
if(TRUE)
    if((${CMAKE_MAJOR_VERSION} LESS 3) OR (${CMAKE_MINOR_VERSION} LESS 7))
        # HACK: if the CMake is < 3.7, then SOURCE_DIR isn't supported, so
        # we have to use an ExternalProject_Add that *may* forward fewer args
        #
        # older CMake also seems to break with UPDATE_DISCONNECTED *shrug*
        ExternalProject_Add(glew-project
            URL "${OSMV_REPO_PROVIDER}/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0.zip"
            PREFIX ""
            CONFIGURE_COMMAND cmake ${CMAKE_BINARY_DIR}/glew-project-prefix/src/glew-project/build/cmake ${OSMV_DEPENDENCY_CMAKE_ARGS} -G ${CMAKE_GENERATOR}
            INSTALL_COMMAND ""
            EXCLUDE_FROM_ALL TRUE
        )
    else()
        ExternalProject_Add(glew-project
            URL "${OSMV_REPO_PROVIDER}/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0.zip"
            PREFIX ""
            CMAKE_CACHE_ARGS ${OSMV_DEPENDENCY_CMAKE_ARGS}
            SOURCE_SUBDIR build/cmake
            BUILD_COMMAND ${CMAKE_COMMAND} --build . --target glew_s
            INSTALL_COMMAND ""
            EXCLUDE_FROM_ALL TRUE
            UPDATE_DISCONNECTED ON
        )
    endif()
    ExternalProject_Get_Property(glew-project SOURCE_DIR)
    ExternalProject_Get_Property(glew-project BINARY_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

    add_library(osmv-glew STATIC IMPORTED)
    add_dependencies(osmv-glew glew-project)

    if(WIN32)
        set(LIBNAME "libglew32${CMAKE_STATIC_LIBRARY_SUFFIX}")
        set(DEBUG_LIBNAME "libglew32d${CMAKE_STATIC_LIBRARY_SUFFIX}")
    else()
        set(LIBNAME "${CMAKE_STATIC_LIBRARY_PREFIX}GLEW${CMAKE_STATIC_LIBRARY_SUFFIX}")
        set(DEBUG_LIBNAME "${CMAKE_STATIC_LIBRARY_PREFIX}GLEWd${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    if(${OSMV_GENERATOR_IS_MULTI_CONFIG})
        set_target_properties(osmv-glew PROPERTIES
            IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/lib/Debug/${DEBUG_LIBNAME}
            IMPORTED_LOCATION_RELWITHDEBINFO ${BINARY_DIR}/lib/RelWithDebInfo/${LIBNAME}
            IMPORTED_LOCATION_MINSIZEREL ${BINARY_DIR}/lib/MinSizeRel/${LIBNAME}
            IMPORTED_LOCATION_RELEASE ${BINARY_DIR}/lib/Release/${LIBNAME}
        )
    else()
        set_target_properties(osmv-glew PROPERTIES
            IMPORTED_LOCATION ${BINARY_DIR}/lib/${LIBNAME}
        )
    endif()

    set_target_properties(osmv-glew PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
        INTERFACE_COMPILE_DEFINITIONS GLEW_STATIC  # https://github.com/nigels-com/glew/issues/161
    )

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
        CMAKE_CACHE_ARGS ${OSMV_DEPENDENCY_CMAKE_ARGS}
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        UPDATE_DISCONNECTED ON
    )
    ExternalProject_Get_Property(sdl2-project SOURCE_DIR)
    ExternalProject_Get_Property(sdl2-project BINARY_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

    add_library(osmv-sdl2 SHARED IMPORTED)
    add_dependencies(osmv-sdl2 sdl2-project)

    if(WIN32)
        set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2)
        set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2d)
    else()
        set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0)
        set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0d)
    endif()

    if(${OSMV_GENERATOR_IS_MULTI_CONFIG})
        set_target_properties(osmv-sdl2 PROPERTIES
            IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_RELEASE ${BINARY_DIR}/Release/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_RELEASE ${BINARY_DIR}/Release/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
        )
    else()
        set_target_properties(osmv-sdl2 PROPERTIES
            IMPORTED_LOCATION ${BINARY_DIR}/${LIBNAME}
            INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
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
        UPDATE_DISCONNECTED ON
    )
    ExternalProject_Get_Property(glm-project SOURCE_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR})

    add_library(osmv-glm INTERFACE)
    add_dependencies(osmv-glm glm-project)  # so the headers are populated
    target_include_directories(osmv-glm INTERFACE ${SOURCE_DIR})

    unset(SOURCE_DIR)
endif()

# DEPENDENCY: imgui
#     small low-dependency library for rendering interactive panels (buttons, sliders, etc.)
#
#     - built from source with SDL2 + OpenGL backend
#     - in tree, to reduce some of the faffing around to deal with CMake3.5 bugs
if(TRUE)
    add_library(osmv-imgui STATIC
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_demo.cpp  # useful for osmv devs to see what's available
        third_party/imgui/backends/imgui_impl_opengl3.cpp
        third_party/imgui/backends/imgui_impl_sdl.cpp
    )
    target_link_libraries(osmv-imgui PUBLIC osmv-sdl2 osmv-glew osmv-glm)
    target_include_directories(osmv-imgui PUBLIC third_party/ third_party/imgui/)
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
        UPDATE_DISCONNECTED ON
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
        GIT_SUBMODULES ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        UPDATE_DISCONNECTED ON
    )
    ExternalProject_Get_Property(tomlplusplus-project SOURCE_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
    file(MAKE_DIRECTORY ${SOURCE_DIR})

    add_library(osmv-tomlplusplus INTERFACE)
    add_dependencies(osmv-tomlplusplus tomlplusplus-project)  # so the headers are populated
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
if(TRUE)
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
endif()

# `osmv-all-dependencies`: all libraries osmv should link to
add_library(osmv-all-dependencies INTERFACE)
target_link_libraries(osmv-all-dependencies INTERFACE

    osmv-glew
    osmv-sdl2
    osmv-glm
    osmv-imgui
    osmv-stb-image
    osmv-tomlplusplus

    ${OSMV_OPENSIM_LIBS}
    ${OPENGL_LIBRARIES}
)

# `OSMV_LIB_FILES_TO_COPY`: all lib *files* that osmv should copy
# (i.e. they aren't supplied by the system)
foreach(OPENSIM_LIB ${OSMV_OPENSIM_LIBS})
    list(APPEND OSMV_LIB_FILES_TO_COPY $<TARGET_FILE:${OPENSIM_LIB}>)
endforeach()
if(NOT LINUX)
    list(APPEND OSMV_LIB_FILES_TO_COPY $<TARGET_FILE:osmv-sdl2>)
endif()
