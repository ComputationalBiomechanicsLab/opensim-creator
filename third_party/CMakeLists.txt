cmake_minimum_required(VERSION 3.10)
project(opynsim-dependencies)

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR

# no nanobind: it's used via `add_submodule`
option(OPYNDEPS_BUILD_MDSPAN    "Build mdspan"                            ON)
option(OPYNDEPS_BUILD_OPENBLAS  "Build OpenBLAS"                          ${WIN32})
option(OPYNDEPS_BUILD_ROBIN_MAP "Build robin-map (required for nanobind)" ON)
option(OPYNDEPS_BUILD_SPDLOG    "Build spdlog"                            ON)

# required for `opynsim` python bindings (shared object)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set OPYNDEPS_BUILD_ALWAYS
if(TRUE)
    set(OPYNDEPS_BUILD_ALWAYS_DOCS "set BUILD_ALWAYS on all dependency targets, useful when editing dependencies")
    if(DEFINED ENV{OPYNDEPS_BUILD_ALWAYS})
        set(OPYNDEPS_BUILD_ALWAYS "$ENV{OPYNDEPS_BUILD_ALWAYS}" CACHE BOOL "${OPYNDEPS_BUILD_ALWAYS_DOCS}" FORCE)
    else()
        set(OPYNDEPS_BUILD_ALWAYS OFF CACHE BOOL "${OPYNDEPS_BUILD_ALWAYS_DOCS}")
    endif()
    unset(OPYNDEPS_BUILD_ALWAYS_DOCS)
endif()

# setup `OPYNDEPS_DEPENDENCY_CMAKE_ARGS`
#
# these are cache args that should be forwarded to external dependency projects
if(TRUE)
    set(OPYNDEPS_FORWARDED_VARS

        CMAKE_CXX_COMPILER
        CMAKE_C_COMPILER
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_VISIBILITY_PRESET
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_VISIBILITY_PRESET
        CMAKE_BUILD_TYPE
        CMAKE_INSTALL_PREFIX
        CMAKE_TOOLCHAIN_FILE
        CMAKE_OSX_SYSROOT
        CMAKE_SYSTEM_VERSION
        CMAKE_OSX_DEPLOYMENT_TARGET
        CMAKE_OSX_ARCHITECTURES
        CMAKE_POSITION_INDEPENDENT_CODE
    )
    foreach(OPYNDEPS_FORWARDED_VAR IN LISTS OPYNDEPS_FORWARDED_VARS)
        if (DEFINED ${OPYNDEPS_FORWARDED_VAR})
            list(APPEND OPYNDEPS_DEPENDENCY_CMAKE_ARGS -D${OPYNDEPS_FORWARDED_VAR}:STRING=${${OPYNDEPS_FORWARDED_VAR}})
        endif()
    endforeach()
    unset(OPYNDEPS_FORWARDED_VARS)
endif()

if(${OPYNDEPS_BUILD_MDSPAN})
    ExternalProject_Add(mdspan
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mdspan
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OPYNDEPS_BUILD_OPENBLAS})
    ExternalProject_Add(OpenBLAS
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/OpenBLAS
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DC_LAPACK:BOOL=ON
            -DBUILD_STATIC_LIBS:BOOL=ON
            -DNOFORTRAN:STRING="1"
            -DBUILD_TESTING:BOOL=OFF
            -BUILD_LAPACK_DEPRECATED:BOOL=OFF
    )
endif()

if(${OPYNDEPS_BUILD_ROBIN_MAP})
    ExternalProject_Add(robin-map
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/robin-map
	BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OPYNDEPS_BUILD_SPDLOG})
    ExternalProject_Add(spdlog
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DSPDLOG_BUILD_BENCH:BOOL=OFF
            -DSPDLOG_BUILD_TESTS:BOOL=OFF
            -DSPDLOG_BUILD_EXAMPLE:BOOL=OFF
            -DSPDLOG_BUILD_SHARED:BOOL=OFF
            -DCMAKE_CXX_FLAGS:STRING=${SPDLOG_CXX_FLAGS}
    )
endif()

