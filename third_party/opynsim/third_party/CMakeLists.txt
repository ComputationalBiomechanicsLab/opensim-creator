cmake_minimum_required(VERSION 3.25)
project(opynsim-dependencies LANGUAGES CXX C)

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR

if(APPLE)
    set(_build_openblas_default OFF)  # Accelerate framework includes BLAS+LAPACK
else()
    set(_build_openblas_default ON)
endif()

option(OPYNDEPS_BUILD_GLAD            "Build glad"                              ON)
option(OPYNDEPS_BUILD_GOOGLETEST      "Build googletest"                        ON)
option(OPYNDEPS_BUILD_IMGUI           "Build imgui"                             ON)
option(OPYNDEPS_BUILD_IMPLOT          "Build implot"                            ON)
option(OPYNDEPS_BUILD_LUNASVG         "Build lunasvg"                           ON)
option(OPYNDEPS_BUILD_MDSPAN          "Build mdspan"                            ON)
option(OPYNDEPS_BUILD_NANOBIND        "Build nanobind"                          ON)
option(OPYNDEPS_BUILD_OPENBLAS        "Build OpenBLAS"                          ${_build_openblas_default})
option(OPYNDEPS_BUILD_ROBIN_MAP       "Build robin-map (required for nanobind)" ON)
option(OPYNDEPS_BUILD_SDL             "Build SDL"                               ON)
option(OPYNDEPS_BUILD_SPDLOG          "Build spdlog"                            ON)
option(OPYNDEPS_BUILD_STB             "Build stb"                               ON)
option(OPYNDEPS_BUILD_TOMLPLUSPLUS    "Build tomlplusplus"                      ON)
option(OPYNDEPS_BUILD_UNORDERED_DENSE "Build unordered-dense"                   ON)

# set OPYNDEPS_BUILD_ALWAYS
set(_build_always_docstring "set BUILD_ALWAYS on all dependency targets, useful when editing dependencies")
if(DEFINED ENV{OPYNDEPS_BUILD_ALWAYS})
    set(OPYNDEPS_BUILD_ALWAYS "$ENV{OPYNDEPS_BUILD_ALWAYS}" CACHE BOOL "${_build_always_docstring}" FORCE)
else()
    set(OPYNDEPS_BUILD_ALWAYS OFF CACHE BOOL "${_build_always_docstring}")
endif()

# setup `OPYNDEPS_DEPENDENCY_CMAKE_ARGS`
#
# these are cache args that should be forwarded to external dependency projects
if(TRUE)
    set(_forwarded_cache_variables

        CMAKE_CXX_COMPILER
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_VISIBILITY_PRESET
        CMAKE_C_COMPILER
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_VISIBILITY_PRESET
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_EXE_LINKER_FLAGS_DEBUG
        CMAKE_EXE_LINKER_FLAGS_MINSIZEREL
        CMAKE_EXE_LINKER_FLAGS_RELEASE
        CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_SHARED_LINKER_FLAGS
        CMAKE_SHARED_LINKER_FLAGS_DEBUG
        CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL
        CMAKE_SHARED_LINKER_FLAGS_RELEASE
        CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_STATIC_LINKER_FLAGS
        CMAKE_STATIC_LINKER_FLAGS_DEBUG
        CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL
        CMAKE_STATIC_LINKER_FLAGS_RELEASE
        CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_MODULE_LINKER_FLAGS
        CMAKE_MODULE_LINKER_FLAGS_DEBUG
        CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL
        CMAKE_MODULE_LINKER_FLAGS_RELEASE
        CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_MSVC_RUNTIME_LIBRARY
        CMAKE_BUILD_TYPE
        CMAKE_INSTALL_PREFIX
        CMAKE_TOOLCHAIN_FILE
        CMAKE_OSX_SYSROOT
        CMAKE_SYSTEM_VERSION
        CMAKE_OSX_DEPLOYMENT_TARGET
        CMAKE_OSX_ARCHITECTURES
        CMAKE_POSITION_INDEPENDENT_CODE
    )
    foreach(_forwarded_var IN LISTS _forwarded_cache_variables)
        if (DEFINED ${_forwarded_var} AND NOT "${${_forwarded_var}}" STREQUAL "")
            list(APPEND OPYNDEPS_DEPENDENCY_CMAKE_ARGS -D${_forwarded_var}:STRING=${${_forwarded_var}})
        endif()
    endforeach()
endif()


if(OPYNDEPS_BUILD_GLAD)
    ExternalProject_Add(glad
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OPYNDEPS_BUILD_GOOGLETEST)
    ExternalProject_Add(googletest
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_GMOCK:BOOL=OFF
            -Dgtest_force_shared_crt:BOOL=ON  # assume Windows provides the uCRT shared lib
    )
endif()

if (OPYNDEPS_BUILD_IMGUI)
    ExternalProject_Add(imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        SOURCE_SUBDIR ../cmake/imgui/
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OPYNDEPS_BUILD_IMPLOT)
    ExternalProject_Add(implot
        DEPENDS imgui
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/implot
        SOURCE_SUBDIR ../cmake/implot/
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -Dimgui_DIR:PATH=${CMAKE_INSTALL_PREFIX}/lib/cmake/imgui
    )
endif()

if(OPYNDEPS_BUILD_LUNASVG)
    ExternalProject_Add(lunasvg
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lunasvg
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DBUILD_SHARED_LIBS:BOOL=OFF
            -DLUNASVG_BUILD_EXAMPLES:BOOL=OFF
    )
endif()

if(OPYNDEPS_BUILD_MDSPAN)
    ExternalProject_Add(mdspan
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mdspan
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OPYNDEPS_BUILD_NANOBIND)
    ExternalProject_Add(nanobind
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nanobind
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DNB_TEST:BOOL=OFF
            -DNB_USE_SUBMODULE_DEPS:BOOL=OFF  # we separately install robin-map, below
    )
endif()

if(OPYNDEPS_BUILD_OPENBLAS)
    ExternalProject_Add(OpenBLAS
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/OpenBLAS
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DC_LAPACK:BOOL=ON
            -DBUILD_STATIC_LIBS:BOOL=ON
            -DNOFORTRAN:BOOL=ON
            -DBUILD_TESTING:BOOL=OFF
            -DTARGET:STRING=NEHALEM
            -DDYNAMIC_ARCH:BOOL=OFF
            -DBUILD_LAPACK_DEPRECATED:BOOL=OFF
    )
endif()

if(OPYNDEPS_BUILD_ROBIN_MAP)
    ExternalProject_Add(robin-map
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/robin-map
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OPYNDEPS_BUILD_SDL)
    # Ensure that OpenGL is available before trying to configure SDL3, because
    # SDL3 will silently drop (i.e. not configure) 3D context support, which
    # won't surface as an issue until the downstream application's test suite
    # runs (#961)
    cmake_policy(SET CMP0072 NEW)
    find_package(OpenGL REQUIRED)
    ExternalProject_Add(sdl
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
            -DSDL_INSTALL_CMAKEDIR_ROOT_DEFAULT:STRING=${CMAKE_INSTALL_LIBDIR}/cmake/SDL3
            -DSDL_SHARED:BOOL=OFF
            -DSDL_STATIC:BOOL=ON
            -DSDL_TEST_LIBRARY:BOOL=OFF
    )
endif()

if(OPYNDEPS_BUILD_SPDLOG)
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

if (OPYNDEPS_BUILD_STB)
    ExternalProject_Add(stb
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb
        SOURCE_SUBDIR ../cmake/stb/
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OPYNDEPS_BUILD_TOMLPLUSPLUS)
    ExternalProject_Add(tomlplusplus
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()

if(OPYNDEPS_BUILD_UNORDERED_DENSE)
    ExternalProject_Add(unordered_dense
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unordered_dense
        BUILD_ALWAYS ${OPYNDEPS_BUILD_ALWAYS}
        CMAKE_CACHE_ARGS
            ${OPYNDEPS_DEPENDENCY_CMAKE_ARGS}
    )
endif()
