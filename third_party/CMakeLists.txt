cmake_minimum_required(VERSION 3.10)
project(osc-dependencies)

# -------------- gather user-facing build cache vars ---------------- #

set(OSCDEPS_GET_GOOGLETEST ON CACHE BOOL "enable getting googletest")
set(OSCDEPS_GET_LUNASVG ON CACHE BOOL "enable getting lunasvg")
set(OSCDEPS_GET_SDL ON CACHE BOOL "enable getting SDL")
set(OSCDEPS_GET_TOMLPLUSPLUS ON CACHE BOOL "enable gettting tomlplusplus")
set(OSCDEPS_GET_UNORDERED_DENSE ON CACHE BOOL "enable getting unordered-dense")
set(OSCDEPS_GET_GLAD ON CACHE BOOL "enable getting glad")
set(OSCDEPS_GET_NATIVEFILEDIALOG ON CACHE BOOL "enable getting nativefiledialog")
set(OSCDEPS_OSIM_BUILD_OPENBLAS OFF CACHE BOOL "enable transitively getting+building OpenBLAS for osim")
set(OSCDEPS_GET_OPENSIM ON CACHE BOOL "enable getting OpenSim (+its dependencies)")
set(OSCDEPS_BUILD_ALWAYS OFF CACHE BOOL "set BUILD_ALWAYS on all dependency targets, useful when editing dependencies")

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR

# setup `OSCDEPS_DEPENDENCY_CMAKE_ARGS`
#
# these are cache args that should be forwarded to external dependency projects
if(TRUE)
    set(OSCDEPS_FORWARDED_VARS

        CMAKE_CXX_COMPILER
        CMAKE_C_COMPILER
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_BUILD_TYPE
        CMAKE_INSTALL_PREFIX
        CMAKE_TOOLCHAIN_FILE
        CMAKE_OSX_SYSROOT
        CMAKE_OSX_DEPLOYMENT_TARGET
        CMAKE_OSX_ARCHITECTURES
    )
    foreach(OSCDEPS_FORWARDED_VAR IN LISTS OSCDEPS_FORWARDED_VARS)
        list(APPEND OSCDEPS_DEPENDENCY_CMAKE_ARGS -D${OSCDEPS_FORWARDED_VAR}:STRING=${${OSCDEPS_FORWARDED_VAR}})
    endforeach()
    unset(OSCDEPS_FORWARDED_VARS)
endif()

if(${OSCDEPS_GET_GOOGLETEST})
    ExternalProject_Add(googletest
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_GMOCK:BOOL=OFF
            -Dgtest_force_shared_crt:BOOL=ON  # osc uses a shared runtime lib
    )
endif()

if(${OSCDEPS_GET_LUNASVG})
    ExternalProject_Add(lunasvg
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lunasvg
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_SHARED_LIBS:BOOL=OFF
    )
endif()

if(${OSCDEPS_GET_SDL})
    # Ensure that OpenGL is available before trying to configure SDL3, because
    # SDL3 will silently drop (i.e. not configure) 3D context support, which
    # won't surface as an issue until the downstream application's test suite
    # runs (#961)
    cmake_policy(SET CMP0072 NEW)
    find_package(OpenGL REQUIRED)
    ExternalProject_Add(sdl
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DSDL_INSTALL_CMAKEDIR:STRING=${CMAKE_INSTALL_LIBDIR}/cmake/SDL3
            -DSDL_SHARED:BOOL=OFF
            -DSDL_STATIC:BOOL=ON
            -DSDL_JOYSTICK:BOOL=OFF
            -DSDL_HAPTIC:BOOL=OFF
            -DSDL_TEST_LIBRARY:BOOL=OFF
    )
endif()

if(${OSCDEPS_GET_TOMLPLUSPLUS})
    ExternalProject_Add(tomlplusplus
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_GET_UNORDERED_DENSE})
    ExternalProject_Add(unordered_dense
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unordered_dense
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_GET_GLAD})
    ExternalProject_Add(glad
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_GET_NATIVEFILEDIALOG})
    ExternalProject_Add(nativefiledialog
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_GET_OPENSIM})
    # build osim's dependencies
    ExternalProject_Add(osim-third-party
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/osim/third_party
	INSTALL_COMMAND ""  # it's an ExternalProject that does some of its own building
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DOSIMDEPS_GET_OPENBLAS:BOOL=${OSCDEPS_OSIM_BUILD_OPENBLAS}
    )
    # build osim
    ExternalProject_Add(osim
        DEPENDS osim-third-party
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/osim
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DCMAKE_PREFIX_PATH:STRING=${CMAKE_INSTALL_PREFIX}
    )
endif()
