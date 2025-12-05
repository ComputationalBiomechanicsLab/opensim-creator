cmake_minimum_required(VERSION 3.10)
project(oscar-dependencies)

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)   # CMAKE_INSTALL_LIBDIR

option(OSCARDEPS_BUILD_GLAD            "Build glad"            ON)
option(OSCARDEPS_BUILD_GOOGLETEST      "Build googletest"      ON)
option(OSCARDEPS_BUILD_IMGUI           "Build imgui"           ON)
option(OSCARDEPS_BUILD_IMPLOT          "Build implot"          ON)
option(OSCARDEPS_BUILD_LUNASVG         "Build lunasvg"         ON)
option(OSCARDEPS_BUILD_SDL             "Build SDL"             ON)
option(OSCARDEPS_BUILD_STB             "Build stb"             ON)
option(OSCARDEPS_BUILD_TOMLPLUSPLUS    "Build tomlplusplus"    ON)
option(OSCARDEPS_BUILD_UNORDERED_DENSE "Build unordered-dense" ON)

# set OSCARDEPS_BUILD_ALWAYS
if(TRUE)
    set(OSCARDEPS_BUILD_ALWAYS_DOCS "set BUILD_ALWAYS on all dependency targets, useful when editing dependencies")
    if(DEFINED ENV{OSCARDEPS_BUILD_ALWAYS})
        set(OSCARDEPS_BUILD_ALWAYS "$ENV{OSCARDEPS_BUILD_ALWAYS}" CACHE BOOL "${OSCARDEPS_BUILD_ALWAYS_DOCS}" FORCE)
    else()
        set(OSCARDEPS_BUILD_ALWAYS OFF CACHE BOOL "${OSCARDEPS_BUILD_ALWAYS_DOCS}")
    endif()
    unset(OSCARDEPS_BUILD_ALWAYS_DOCS)
endif()

# setup `OSCARDEPS_DEPENDENCY_CMAKE_ARGS`
#
# these are cache args that should be forwarded to external dependency projects
if(TRUE)
    set(OSCARDEPS_FORWARDED_VARS

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
    foreach(OSCARDEPS_FORWARDED_VAR IN LISTS OSCARDEPS_FORWARDED_VARS)
        if (DEFINED ${OSCARDEPS_FORWARDED_VAR})
            list(APPEND OSCARDEPS_DEPENDENCY_CMAKE_ARGS -D${OSCARDEPS_FORWARDED_VAR}:STRING=${${OSCARDEPS_FORWARDED_VAR}})
        endif()
    endforeach()
    unset(OSCARDEPS_FORWARDED_VARS)
endif()

if(OSCARDEPS_BUILD_GLAD)
    ExternalProject_Add(glad
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OSCARDEPS_BUILD_GOOGLETEST)
    ExternalProject_Add(googletest
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_GMOCK:BOOL=OFF
            -Dgtest_force_shared_crt:BOOL=ON  # osc uses a shared runtime lib
    )
endif()

if (OSCARDEPS_BUILD_IMGUI)
    ExternalProject_Add(imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        SOURCE_SUBDIR ../cmake/imgui/
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OSCARDEPS_BUILD_IMPLOT)
    ExternalProject_Add(implot
        DEPENDS imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/implot
        SOURCE_SUBDIR ../cmake/implot/
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
            -Dimgui_DIR:PATH=${CMAKE_INSTALL_PREFIX}/lib/cmake/imgui
    )
endif()

if(OSCARDEPS_BUILD_LUNASVG)
    ExternalProject_Add(lunasvg
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lunasvg
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_SHARED_LIBS:BOOL=OFF
            -DLUNASVG_BUILD_EXAMPLES:BOOL=OFF
    )
endif()

if(OSCARDEPS_BUILD_SDL)
    # Ensure that OpenGL is available before trying to configure SDL3, because
    # SDL3 will silently drop (i.e. not configure) 3D context support, which
    # won't surface as an issue until the downstream application's test suite
    # runs (#961)
    cmake_policy(SET CMP0072 NEW)
    find_package(OpenGL REQUIRED)
    ExternalProject_Add(sdl
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
            -DSDL_INSTALL_CMAKEDIR_ROOT_DEFAULT:STRING=${CMAKE_INSTALL_LIBDIR}/cmake/SDL3
            -DSDL_SHARED:BOOL=OFF
            -DSDL_STATIC:BOOL=ON
            -DSDL_TEST_LIBRARY:BOOL=OFF
    )
endif()

if (OSCARDEPS_BUILD_STB)
    ExternalProject_Add(stb
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb
        SOURCE_SUBDIR ../cmake/stb/
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OSCARDEPS_BUILD_TOMLPLUSPLUS)
    ExternalProject_Add(tomlplusplus
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OSCARDEPS_BUILD_UNORDERED_DENSE)
    ExternalProject_Add(unordered_dense
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unordered_dense
        BUILD_ALWAYS ${OSCARDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCARDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

