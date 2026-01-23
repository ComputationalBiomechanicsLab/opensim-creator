cmake_minimum_required(VERSION 3.10)
project(osc-dependencies)

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)   # CMAKE_INSTALL_LIBDIR

option(OSCDEPS_BUILD_OPYNSIM_DEPS "Build opynsim's transitive dependencies" ON)
option(OSCDEPS_BUILD_OSCAR        "Build oscar (via opynsim)"               ON)
option(OSCDEPS_BUILD_OPYNSIM      "Build opynsim"                           ON)

# set OSCDEPS_BUILD_ALWAYS
if(TRUE)
    set(_oscdeps_build_always_docstring "set BUILD_ALWAYS on all dependency targets, useful when editing dependencies")
    if(DEFINED ENV{OSCDEPS_BUILD_ALWAYS})
        set(OSCDEPS_BUILD_ALWAYS "$ENV{OSCDEPS_BUILD_ALWAYS}" CACHE BOOL "${_oscdeps_build_always_docstring}" FORCE)
    else()
        set(OSCDEPS_BUILD_ALWAYS OFF CACHE BOOL "${_oscdeps_build_always_docstring}")
    endif()
    unset(_oscdeps_build_always_docstring)
endif()

# setup `OSCDEPS_DEPENDENCY_CMAKE_ARGS`
#
# these are cache args that should be forwarded to external dependency projects
if(TRUE)
    set(_oscdeps_forwarded_vars

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
    foreach(_forwarded_var IN LISTS _oscdeps_forwarded_vars)
        if (DEFINED ${_forwarded_var})
            list(APPEND _oscdeps_dependency_cmake_args -D${_forwarded_var}:STRING=${${_forwarded_var}})
        endif()
    endforeach()
    unset(_oscdeps_forwarded_vars)
endif()

if(OSCDEPS_BUILD_OPYNSIM_DEPS)
    ExternalProject_Add(opyndeps
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opynsim/third_party
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/opyndeps/  # make binary path shorter for Windows path length limit (250 chars)
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        INSTALL_COMMAND ""
        CMAKE_CACHE_ARGS
            ${_oscdeps_dependency_cmake_args}
            -DBUILD_TESTING:BOOL=OFF
            -DOPYNDEPS_BUILD_OSCAR:BOOL=${OSCDEPS_BUILD_OSCAR}
    )
endif()

if(OSCDEPS_BUILD_OPYNSIM)
    ExternalProject_Add(opynsim
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opynsim
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${_oscdeps_dependency_cmake_args}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
            -DBUILD_TESTING:BOOL=OFF
            -DOPYN_BUILD_PYTHON_BINDINGS:BOOL=OFF
            -DOPYN_BUILD_DOCS:BOOL=OFF
            -DOPYN_USE_STRICT_COMPILER_FLAGS:BOOL=OFF
            -DOPYN_PACKAGE:BOOL=OFF
    )
    if(OSCDEPS_BUILD_OPYNSIM_DEPS)
        add_dependencies(opynsim opyndeps)
    endif()
endif()

