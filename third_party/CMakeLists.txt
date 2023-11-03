cmake_minimum_required(VERSION 3.10)
project(osc-dependencies)

include(ExternalProject)  # ExternalProject_Add
include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR

# setup `OSC_DEPENDENCY_CMAKE_ARGS`
#
# these are cache args that should be forwarded to external dependency projects
if(TRUE)
    set(OSC_FORWARDED_VARS

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
    foreach(OSC_FORWARDED_VAR IN LISTS OSC_FORWARDED_VARS)
        list(APPEND OSC_DEPENDENCY_CMAKE_ARGS -D${OSC_FORWARDED_VAR}:STRING=${${OSC_FORWARDED_VAR}})
    endforeach()
    unset(OSC_FORWARDED_VARS)
endif()

ExternalProject_Add(googletest
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DBUILD_GMOCK:BOOL=OFF
        -Dgtest_force_shared_crt:BOOL=ON  # osc uses a shared runtime lib
)

ExternalProject_Add(benchmark
    DEPENDS googletest
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/benchmark
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DGOOGLETEST_PATH:STRING=${CMAKE_CURRENT_SOURCE_DIR}/googletest
        -DBENCHMARK_ENABLE_TESTING:BOOL=OFF
        -DBENCHMARK_INSTALL_DOCS:BOOL=OFF
)

ExternalProject_Add(glm
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DBUILD_TESTING:BOOL=OFF
)

ExternalProject_Add(lunasvg
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lunasvg
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
)

ExternalProject_Add(sdl
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SDL
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DSDL_INSTALL_CMAKEDIR:STRING=${CMAKE_INSTALL_LIBDIR}/cmake/SDL2
        -DBUILD_SHARED_LIBS:BOOL=ON
        -DSDL_TEST:BOOL=OFF
)

ExternalProject_Add(tomlplusplus
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
)

ExternalProject_Add(unordered_dense
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unordered_dense
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
)

ExternalProject_Add(glew
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glew
    SOURCE_SUBDIR build/cmake
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DBUILD_UTILS:BOOL=OFF
        -DBUILD_SHARED_LIBS:BOOL=OFF
)

ExternalProject_Add(IconFontCppHeaders
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/IconFontCppHeaders

    # we provide our own cmake for this dependency
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IconFontCppHeaders.cmake" <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IconFontCppHeadersConfig.cmake.in" <SOURCE_DIR>/IconFontCppHeadersConfig.cmake.in

    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
)

ExternalProject_Add(stb
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb

    # we provide our own cmake for this dependency
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stb.cmake" <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stbConfig.cmake.in" <SOURCE_DIR>/stbConfig.cmake.in

    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
)

ExternalProject_Add(nativefiledialog
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog

    # we provide our own cmake for this dependency
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nativefiledialog.cmake" <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nativefiledialogConfig.cmake.in" <SOURCE_DIR>/nativefiledialogConfig.cmake.in

    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
)

ExternalProject_Add(imgui
    DEPENDS sdl glm
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui

    # we provide our own cmake for this dependency
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imgui.cmake" <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/imguiConfig.cmake.in" <SOURCE_DIR>/imguiConfig.cmake.in

    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(ImGuizmo
    DEPENDS imgui
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo

    # we provide our own cmake for this dependency
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImGuizmo.cmake" <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImGuizmoConfig.cmake.in" <SOURCE_DIR>/ImGuizmoConfig.cmake.in

    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(implot
    DEPENDS imgui
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/implot

    # we provide our own cmake for this dependency
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/implot.cmake" <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmake/implotConfig.cmake.in" <SOURCE_DIR>/implotConfig.cmake.in

    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
)

ExternalProject_Add(opensim-core-dependencies
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opensim-core/dependencies
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/opensim-core-dependencies-install
    INSTALL_COMMAND ""  # OpenSim's dependency build installs during its build step
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DOPENSIM_WITH_CASADI:BOOL=OFF
        -DOPENSIM_WITH_TROPTER:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>
        "-DSIMBODY_EXTRA_CMAKE_ARGS:STRING=-DBUILD_VISUALIZER:BOOL=OFF"
)

ExternalProject_Add(opensim-core
    DEPENDS opensim-core-dependencies
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opensim-core
    TEST_EXCLUDE_FROM_MAIN ON
    CMAKE_CACHE_ARGS
        ${OSC_DEPENDENCY_CMAKE_ARGS}
        -DOPENSIM_WITH_CASADI:BOOL=OFF
        -DOPENSIM_WITH_TROPTER:BOOL=OFF
        -DBUILD_API_ONLY:BOOL=ON
        -DOPENSIM_DISABLE_LOG_FILE:BOOL=ON
        -DOPENSIM_BUILD_INDIVIDUAL_APPS:BOOL=OFF
        -DOPENSIM_DEPENDENCIES_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}/opensim-core-dependencies-install
        -DBUILD_JAVA_WRAPPING:BOOL=OFF
        -DOPENSIM_INSTALL_UNIX_FHS:BOOL=ON
        -DOPENSIM_BUILD_INDIVIDUAL_APPS_DEFAULT:BOOL=OFF
        -DBUILD_TESTING:BOOL=OFF
        -DBUILD_API_EXAMPLES:BOOL=OFF
)
