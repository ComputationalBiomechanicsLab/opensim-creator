cmake_minimum_required(VERSION 3.10)
project(osc-dependencies)

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)   # CMAKE_INSTALL_LIBDIR
find_package(Git REQUIRED)  # GIT_EXECUTABLE

set(OSCDEPS_BUILD_GLAD ON CACHE BOOL "enable building glad from source")
set(OSCDEPS_BUILD_GOOGLETEST ON CACHE BOOL "enable building googletest from source")
set(OSCDEPS_BUILD_IMGUI ON CACHE BOOL "enable building imgui from source")
set(OSCDEPS_BUILD_IMPLOT ON CACHE BOOL "enable building implot from source")
set(OSCDEPS_BUILD_LIBOSIM ON CACHE BOOL "enable building libosim from source")
set(OSCDEPS_BUILD_LUNASVG ON CACHE BOOL "enable building lunasvg from source")
set(OSCDEPS_BUILD_MDSPAN ON CACHE BOOL "enable building mdspan shim")
set(OSCDEPS_BUILD_OPENBLAS OFF CACHE BOOL "enable building OpenBLAS from source")  # default OFF because it's a big build and only necessary on Windows
set(OSCDEPS_BUILD_SDL ON CACHE BOOL "enable building SDL from source")
set(OSCDEPS_BUILD_SPDLOG ON CACHE BOOL "enable building spdlog from source")
set(OSCDEPS_BUILD_STB ON CACHE BOOL "enable building stb (headers)")
set(OSCDEPS_BUILD_TOMLPLUSPLUS ON CACHE BOOL "enable building tomlplusplus from source")
set(OSCDEPS_BUILD_UNORDERED_DENSE ON CACHE BOOL "enable building unordered-dense from source")

# set OSCDEPS_BUILD_ALWAYS
if(TRUE)
    set(OSCDEPS_BUILD_ALWAYS_DOCS "set BUILD_ALWAYS on all dependency targets, useful when editing dependencies")
    if(DEFINED ENV{OSCDEPS_BUILD_ALWAYS})
        set(OSCDEPS_BUILD_ALWAYS "$ENV{OSCDEPS_BUILD_ALWAYS}" CACHE BOOL "${OSCDEPS_BUILD_ALWAYS_DOCS}" FORCE)
    else()
        set(OSCDEPS_BUILD_ALWAYS OFF CACHE BOOL "${OSCDEPS_BUILD_ALWAYS_DOCS}")
    endif()
    unset(OSCDEPS_BUILD_ALWAYS_DOCS)
endif()

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
    foreach(OSCDEPS_FORWARDED_VAR IN LISTS OSCDEPS_FORWARDED_VARS)
        if (DEFINED ${OSCDEPS_FORWARDED_VAR})
            list(APPEND OSCDEPS_DEPENDENCY_CMAKE_ARGS -D${OSCDEPS_FORWARDED_VAR}:STRING=${${OSCDEPS_FORWARDED_VAR}})
        endif()
    endforeach()
    unset(OSCDEPS_FORWARDED_VARS)
endif()


if(${OSCDEPS_BUILD_GOOGLETEST})
    ExternalProject_Add(googletest
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_GMOCK:BOOL=OFF
            -Dgtest_force_shared_crt:BOOL=ON  # osc uses a shared runtime lib
    )
endif()

if(${OSCDEPS_BUILD_LUNASVG})
    ExternalProject_Add(lunasvg
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lunasvg
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_SHARED_LIBS:BOOL=OFF
            -DLUNASVG_BUILD_EXAMPLES:BOOL=OFF
    )
endif()

if(${OSCDEPS_BUILD_SDL})
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
            -DSDL_TEST_LIBRARY:BOOL=OFF
            -DSDL_PRESEED:BOOL=OFF  # https://github.com/libsdl-org/SDL/issues/11487#issuecomment-2487405649
    )
endif()

if(${OSCDEPS_BUILD_TOMLPLUSPLUS})
    ExternalProject_Add(tomlplusplus
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_BUILD_UNORDERED_DENSE})
    ExternalProject_Add(unordered_dense
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unordered_dense
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_BUILD_GLAD})
    ExternalProject_Add(glad
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if (${OSCDEPS_BUILD_IMGUI})
    file(GLOB OSCDEPS_IMGUI_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/patches/imgui/*.patch")
    ExternalProject_Add(imgui
        # PATCH_COMMAND ${GIT_EXECUTABLE} apply ${OSCDEPS_IMGUI_PATCHES}
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        SOURCE_SUBDIR ../cmake/imgui/
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
    unset(OSCDEPS_IMGUI_PATCHES)
endif()

if(${OSCDEPS_BUILD_IMPLOT})
    ExternalProject_Add(implot
        DEPENDS imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/implot
        SOURCE_SUBDIR ../cmake/implot/
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -Dimgui_DIR:PATH=${CMAKE_INSTALL_PREFIX}/lib/cmake/imgui
    )
endif()

if(${OSCDEPS_BUILD_OPENBLAS})
    ExternalProject_Add(OpenBLAS
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/OpenBLAS
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DC_LAPACK:BOOL=ON
            -DBUILD_STATIC_LIBS:BOOL=ON
    )
endif()

if(${OSCDEPS_BUILD_SPDLOG})

    ExternalProject_Add(spdlog
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DSPDLOG_BUILD_BENCH:BOOL=OFF
            -DSPDLOG_BUILD_TESTS:BOOL=OFF
            -DSPDLOG_BUILD_EXAMPLE:BOOL=OFF
            -DSPDLOG_BUILD_SHARED:BOOL=OFF
            -DCMAKE_CXX_FLAGS:STRING=${SPDLOG_CXX_FLAGS}
    )
endif()

if(${OSCDEPS_BUILD_LIBOSIM})
    if(${OSCDEPS_BUILD_OPENBLAS})
        list(APPEND OSCDEPS_LIBOSIM_DEPS OpenBLAS)
    endif()
    if(${OSCDEPS_BUILD_SPDLOG})
        list(APPEND OSCDEPS_LIBOSIM_DEPS spdlog)
    endif()

    file(GLOB OSCDEPS_LIBOSIM_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/patches/libosim/*.patch")
    ExternalProject_Add(libosim
        DEPENDS ${OSCDEPS_LIBOSIM_DEPS}
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libosim
        # PATCH_COMMAND ${GIT_EXECUTABLE} apply ${OSCDEPS_LIBOSIM_PATCHES}
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DOSIM_INSTALL_OPENSIM_HEADERS:BOOL=1
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
    )
    unset(OSCDEPS_LIBOSIM_PATCHES)
endif()

if (${OSCDEPS_BUILD_STB})
    ExternalProject_Add(stb
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_BUILD_MDSPAN})
    ExternalProject_Add(mdspan
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mdspan
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()
