cmake_minimum_required(VERSION 3.10)
project(osc-dependencies)

# -------------- gather user-facing build cache vars ---------------- #

set(OSCDEPS_GET_GOOGLETEST ON CACHE BOOL "enable getting googletest")
set(OSCDEPS_GET_LUNASVG ON CACHE BOOL "enable getting lunasvg")
set(OSCDEPS_GET_SDL ON CACHE BOOL "enable getting SDL")
set(OSCDEPS_GET_TOMLPLUSPLUS ON CACHE BOOL "enable gettting tomlplusplus")
set(OSCDEPS_GET_UNORDERED_DENSE ON CACHE BOOL "enable getting unordered-dense")
set(OSCDEPS_GET_GLEW ON CACHE BOOL "enable getting glew")
set(OSCDEPS_GET_STB ON CACHE BOOL "enable getting stb headers")
set(OSCDEPS_GET_NATIVEFILEDIALOG ON CACHE BOOL "enable getting nativefiledialog")
set(OSCDEPS_GET_IMGUI ON CACHE BOOL "enable getting imgui")
set(OSCDEPS_GET_IMPLOT ON CACHE BOOL "enable gettting implot")
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
    ExternalProject_Add(sdl
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DSDL_INSTALL_CMAKEDIR:STRING=${CMAKE_INSTALL_LIBDIR}/cmake/SDL2
            -DSDL_SHARED:BOOL=OFF
            -DSDL_STATIC:BOOL=ON
            -DSDL_TEST:BOOL=OFF
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

if(${OSCDEPS_GET_GLEW})
    ExternalProject_Add(glew
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glew
        SOURCE_SUBDIR build/cmake
        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_UTILS:BOOL=OFF
            -DBUILD_SHARED_LIBS:BOOL=OFF
    )
endif()

if (${OSCDEPS_GET_STB})
    ExternalProject_Add(stb
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb

        # we provide our own cmake for this dependency
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stb.cmake" <SOURCE_DIR>/CMakeLists.txt
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stbConfig.cmake.in" <SOURCE_DIR>/stbConfig.cmake.in

        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(${OSCDEPS_GET_NATIVEFILEDIALOG})
    ExternalProject_Add(nativefiledialog
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog

        # we provide our own cmake for this dependency
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nativefiledialog.cmake" <SOURCE_DIR>/CMakeLists.txt
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nativefiledialogConfig.cmake.in" <SOURCE_DIR>/nativefiledialogConfig.cmake.in

        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if (${OSCDEPS_GET_IMGUI})
    ExternalProject_Add(imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui

        # we provide our own cmake for this dependency
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imgui.cmake" <SOURCE_DIR>/CMakeLists.txt
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imguiConfig.cmake.in" <SOURCE_DIR>/imguiConfig.cmake.in

        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
    )
endif()

if(${OSCDEPS_GET_IMPLOT})
    ExternalProject_Add(implot
        DEPENDS imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/implot

        # we provide our own cmake for this dependency
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/implot.cmake" <SOURCE_DIR>/CMakeLists.txt
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/implotConfig.cmake.in" <SOURCE_DIR>/implotConfig.cmake.in

        BUILD_ALWAYS ${OSCDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OSCDEPS_DEPENDENCY_CMAKE_ARGS}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
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
