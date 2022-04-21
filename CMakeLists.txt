cmake_minimum_required(VERSION 3.10)

project(osc VERSION 0.1.5 LANGUAGES C CXX)

enable_testing()


# -------------- gather user-facing build config options ---------------- #

set(
    OSC_USE_CLANG_TIDY OFF
    CACHE BOOL
    "enable/disable running `clang-tidy` (linter) while building"
)
set(
    OSC_USE_IWYU OFF
    CACHE BOOL
    "enable/disable running `iwyu` (include-what-you-use) while building"
)
set(
    OSC_IWYU_PATH "include-what-you-use"
    CACHE STRING
    "path to the `iwyu` executable (if not on the PATH)"
)
set(
    OSC_FORCE_ASSERTS_ENABLED ON
    CACHE BOOL
    "enable OSC's runtime assertions - even if building a release build"
)
set(
    OSC_FORCE_UNDEFINE_NDEBUG OFF
    CACHE BOOL
    "force disable NDEBUG, even if running in Release mode. This enables *a lot* of runtime assertions (standard lib, SimTK, OpenSim)"
)
set(
    OSC_USE_ASAN OFF
    CACHE BOOL
    "enable address sanitizer (libASAN): useful for spotting memory access violations and leaks"
)
set(
    OSC_DEFAULT_USE_MULTI_VIEWPORT OFF
    CACHE BOOL
    "enable/disable osc using multi viewport when not explicitly specified a the user/config"
)
set(
    OSC_DEFAULT_RESOURCE_DIR "../resources"
    CACHE STRING
    "default location of runtime resources dir when not specified by user/config"
)
set(
    OSC_BUILD_DOCS OFF
    CACHE BOOL
    "whether to build the documentation or not (requires sphinx-build is available on the PATH)"
)


# -------------- top-level build configuration (vars etc.) ---------------- #

# set OSC_GENERATOR_IS_MULTI_CONFIG
#
# effectively, tests if the user is using a multi-config generator, like
# Visual Studio, which lets developers change their build configuration
# (Release, Debug) in a single configuration
#
# this can affect things like deciding which/where to link libraries
get_property(OSC_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# set LINUX
#
# this is based on whether the UNIX variant being used appears to be LINUX
if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a UNIX system that may not be supported yet - beware!")
    endif()
endif()

# set OSC_BUILD_ID
#
# this is so that errors in the binary can potentially be linked to the source
# that produced the binary later on (user errors, etc.)
if(DEFINED ENV{GITHUB_RUN_ID})

    # build is running inside GitHub Action CI - use that ID
    set(OSC_BUILD_ID "GITHUB_$ENV{GITHUB_RUN_ID}.$ENV{GITHUB_RUN_NUMBER}" CACHE STRING "build ID")
else()

    # build is probably running on a dev's machine - generate a random ID
    string(RANDOM RANDOM_ID)
    set(OSC_BUILD_ID "CUSTOM_${RANDOM_ID}" CACHE STRING "build ID")
    unset(RANDOM_ID)
endif()

# set OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL
#
# this is a C-like boolean that tells OSC whether to use a multi-viewport
# mode
if(${OSC_DEFAULT_USE_MULTI_VIEWPORT})
    set(OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL "true")
else()
    set(OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL "false")
endif()

# set CMAKE_CXX_INCLUDE_WHAT_YOU_USE
#
# this enables CMake's in-built support for iwyu
if(OSC_USE_IWYU)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${OSC_IWYU_PATH};-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;")
endif()

# set CMAKE_CXX_CLANG_TIDY
#
# this enables CMake's in-built support for clang-tidy
if(OSC_USE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
endif()

# handle OSC_FORCE_UNDEFINE_NDEBUG
#
# this unsets the relevant flags globally
if(OSC_FORCE_UNDEFINE_NDEBUG)
    # MSVC
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    # others
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()


# ------------------ configure dependencies -------------------- #

# all dependency management is handled from a separate CMake file that exports
# the dependencies as targets - along with some other useful vars (e.g. a list
# of all dependencies that should be copied into the install, etc.)
include(GetDependencies.cmake)


# ------------- configure configuration header ----------------- #

# osc_config.hpp:
#
#     configuration file that contains configuration-time vals (e.g. version, libs)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/osc_config.hpp.in" "osc_config.hpp")


# --------------- configure configuration target --------------- #

# osc_configuration:
#
#     an INTERFACE target that sets common options that are used by the real
#     targets (e.g. use C++17, use these flags, etc.)
add_library(osc_configuration INTERFACE)

# osc: compile-time includes
target_include_directories(osc_configuration INTERFACE

    # so `#include "src/<path>.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# osc_configuration: set general compiler options used by all targets
target_compile_options(osc_configuration INTERFACE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:
        # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4996

        # disable warning 4455: incorrectly flags std::string::literals::"s"
        /wd4455

        # turn warning level up to "production grade" (note: /W4 explodes because of Simbody)
        /W3

        # treat all warnings as errors
        /WX

        # keep frame pointers around
        /Oy-

        # newer MSVCs may also come with LIBASAN (experimental)
        $<$<BOOL:${OSC_USE_ASAN}>:/fsanitize=address>
    >

    # gcc AND clang flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall
        -Wextra
        -pedantic
        -Werror
        -Wuninitialized
        -Winit-self
        -Wshadow
        -Wcast-align
        -Wwrite-strings
        -Wdangling-else
        -Wdate-time
        -Wno-multichar
        -Wredundant-decls
        -Wvla
        -Wdisabled-optimization
        -Wpacked
        -Wno-sign-compare  # disable: broken by STB headers
        -Wformat-security

        # regardless of debug/release, pin the frame pointer register
        # so that stack traces are sane when debugging (even in Release).
        #
        # This adds some overhead (pins one register and requires callers
        # to setup their base pointers etc.) but makes debugging + profiling
        # the application much easier, even in release mode
        -fno-omit-frame-pointer

        # if address sanitizer is specified, use it
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >

    # clang flags
    $<$<CXX_COMPILER_ID:Clang>:
        -Wno-cast-align

        # required in earlier clangs. Just setting
        # -fno-omit-frame-pointer (above) is not enough
        #
        # see:
        #   - https://stackoverflow.com/questions/43864881/fno-omit-frame-pointer-equivalent-compiler-option-for-clang
        #   - fixed here: https://reviews.llvm.org/D64294
        -mno-omit-leaf-frame-pointer
    >

    # gcc flags
    $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused  # disable: broken by STB headers
    >
)

# osc_configuration: set general compiler features used by all targets
target_compile_features(osc_configuration INTERFACE cxx_std_17)

# osc_configuration: set libraries used by all targets
target_link_libraries(osc_configuration INTERFACE

    # in earlier gcc/clang the <filesystem> implementation must be explicitly linked
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>
)

# osc_configuration: handle OSC_FORCE_ASSERTS_ENABLED
if(OSC_FORCE_ASSERTS_ENABLED)
    target_compile_definitions(osc_configuration INTERFACE -DOSC_FORCE_ASSERTS_ENABLED=1)
endif()


#------------- configure core library target ------------- #

# [lib]osccore:
#
#     shared library where all the runtime code is actually hiding. This is what
#     the main executable, test code, etc. hooks into
add_library(osccore STATIC
    ${CMAKE_BINARY_DIR}/osc_config.hpp

    src/Bindings/GlmHelpers.cpp
    src/Bindings/GlmHelpers.hpp
    src/Bindings/ImGuiHelpers.cpp
    src/Bindings/ImGuiHelpers.hpp
    src/Bindings/SDL2Helpers.hpp
    src/Bindings/SimTKHelpers.cpp
    src/Bindings/SimTKHelpers.hpp

    src/Graphics/Shaders/ColormappedPlainTextureShader.cpp
    src/Graphics/Shaders/ColormappedPlainTextureShader.hpp
    src/Graphics/Shaders/EdgeDetectionShader.cpp
    src/Graphics/Shaders/EdgeDetectionShader.hpp
    src/Graphics/Shaders/GouraudMrtShader.cpp
    src/Graphics/Shaders/GouraudMrtShader.hpp
    src/Graphics/Shaders/GouraudShader.cpp
    src/Graphics/Shaders/GouraudShader.hpp
    src/Graphics/Shaders/InstancedGouraudColorShader.cpp
    src/Graphics/Shaders/InstancedGouraudColorShader.hpp
    src/Graphics/Shaders/InstancedSolidColorShader.cpp
    src/Graphics/Shaders/InstancedSolidColorShader.hpp
    src/Graphics/Shaders/NormalsShader.cpp
    src/Graphics/Shaders/NormalsShader.hpp
    src/Graphics/Shaders/PlainTextureShader.cpp
    src/Graphics/Shaders/PlainTextureShader.hpp
    src/Graphics/Shaders/SkipMSXAABlitterShader.cpp
    src/Graphics/Shaders/SkipMSXAABlitterShader.hpp
    src/Graphics/Shaders/SolidColorShader.cpp
    src/Graphics/Shaders/SolidColorShader.hpp

    src/Graphics/Color.cpp
    src/Graphics/Color.hpp
    src/Graphics/Gl.cpp
    src/Graphics/Gl.hpp
    src/Graphics/GlGlm.hpp
    src/Graphics/InstancedRenderer.cpp
    src/Graphics/InstancedRenderer.hpp
    src/Graphics/Mesh.cpp
    src/Graphics/Mesh.hpp
    src/Graphics/MeshCache.cpp
    src/Graphics/MeshCache.hpp
    src/Graphics/MeshData.cpp
    src/Graphics/MeshData.hpp
    src/Graphics/MeshGen.cpp
    src/Graphics/MeshGen.hpp
    src/Graphics/MeshTopography.hpp
    src/Graphics/Renderer.cpp
    src/Graphics/Renderer.hpp
    src/Graphics/Shader.hpp
    src/Graphics/ShaderCache.cpp
    src/Graphics/ShaderCache.hpp
    src/Graphics/ShaderLocationIndex.hpp
    src/Graphics/Texturing.cpp
    src/Graphics/Texturing.hpp

    src/Maths/AABB.cpp
    src/Maths/AABB.hpp
    src/Maths/BVH.cpp
    src/Maths/BVH.hpp
    src/Maths/Constants.hpp
    src/Maths/Disc.cpp
    src/Maths/Disc.hpp
    src/Maths/EulerPerspectiveCamera.cpp
    src/Maths/EulerPerspectiveCamera.hpp
    src/Maths/Geometry.cpp
    src/Maths/Geometry.hpp
    src/Maths/Line.cpp
    src/Maths/Line.hpp
    src/Maths/Plane.cpp
    src/Maths/Plane.hpp
    src/Maths/PolarPerspectiveCamera.cpp
    src/Maths/PolarPerspectiveCamera.hpp
    src/Maths/RayCollision.hpp
    src/Maths/Rect.cpp
    src/Maths/Rect.hpp
    src/Maths/Segment.cpp
    src/Maths/Segment.hpp
    src/Maths/Sphere.cpp
    src/Maths/Sphere.hpp
    src/Maths/Transform.cpp
    src/Maths/Transform.hpp

    src/OpenSimBindings/BasicModelStatePair.cpp
    src/OpenSimBindings/BasicModelStatePair.hpp
    src/OpenSimBindings/ComponentDecoration.cpp
    src/OpenSimBindings/ComponentDecoration.hpp
    src/OpenSimBindings/ComponentOutputExtractor.cpp
    src/OpenSimBindings/ComponentOutputExtractor.hpp
    src/OpenSimBindings/FdSimulation.cpp
    src/OpenSimBindings/FdSimulation.hpp
    src/OpenSimBindings/IntegratorMethod.cpp
    src/OpenSimBindings/IntegratorMethod.hpp
    src/OpenSimBindings/IntegratorOutputExtractor.cpp
    src/OpenSimBindings/IntegratorOutputExtractor.hpp
    src/OpenSimBindings/MainEditorState.cpp
    src/OpenSimBindings/MainEditorState.hpp
    src/OpenSimBindings/MultiBodySystemOutputExtractor.cpp
    src/OpenSimBindings/MultiBodySystemOutputExtractor.hpp
    src/OpenSimBindings/OpenSimApp.cpp
    src/OpenSimBindings/OpenSimApp.hpp
    src/OpenSimBindings/OpenSimHelpers.cpp
    src/OpenSimBindings/OpenSimHelpers.hpp
    src/OpenSimBindings/OutputExtractor.cpp
    src/OpenSimBindings/OutputExtractor.hpp
    src/OpenSimBindings/ParamBlock.hpp
    src/OpenSimBindings/ParamBlock.cpp
    src/OpenSimBindings/Simulation.hpp
    src/OpenSimBindings/SimulationClock.hpp
    src/OpenSimBindings/SimulationReport.cpp
    src/OpenSimBindings/SimulationReport.hpp
    src/OpenSimBindings/SimulationStatus.cpp
    src/OpenSimBindings/SimulationStatus.hpp
    src/OpenSimBindings/SimulatorModelStatePair.cpp
    src/OpenSimBindings/SimulatorModelStatePair.hpp
    src/OpenSimBindings/StateModifications.cpp
    src/OpenSimBindings/StateModifications.hpp
    src/OpenSimBindings/StoFileSimulation.hpp
    src/OpenSimBindings/StoFileSimulation.cpp
    src/OpenSimBindings/TypeRegistry.cpp
    src/OpenSimBindings/TypeRegistry.hpp
    src/OpenSimBindings/UiFdSimulation.cpp
    src/OpenSimBindings/UiFdSimulation.hpp
    src/OpenSimBindings/UiModel.cpp
    src/OpenSimBindings/UiModel.hpp
    src/OpenSimBindings/UndoableUiModel.cpp
    src/OpenSimBindings/UndoableUiModel.hpp
    src/OpenSimBindings/VirtualConstModelStatePair.hpp
    src/OpenSimBindings/VirtualModelStatePair.hpp
    src/OpenSimBindings/VirtualOutputExtractor.hpp
    src/OpenSimBindings/VirtualSimulation.hpp

    src/Platform/App.cpp
    src/Platform/App.hpp
    src/Platform/Config.cpp
    src/Platform/Config.hpp
    src/Platform/IoPoller.cpp
    src/Platform/IoPoller.hpp
    src/Platform/Log.cpp
    src/Platform/Log.hpp
    src/Platform/os.cpp
    src/Platform/os.hpp
    src/Platform/RecentFile.hpp
    src/Platform/Screen.hpp
    src/Platform/Styling.hpp

    src/Screens/CookiecutterScreen.cpp
    src/Screens/CookiecutterScreen.hpp
    src/Screens/ErrorScreen.cpp
    src/Screens/ErrorScreen.hpp
    src/Screens/ExperimentsScreen.cpp
    src/Screens/ExperimentsScreen.hpp
    src/Screens/HelloTriangleScreen.cpp
    src/Screens/HelloTriangleScreen.hpp
    src/Screens/HittestScreen.cpp
    src/Screens/HittestScreen.hpp
    src/Screens/ImGuiDemoScreen.cpp
    src/Screens/ImGuiDemoScreen.hpp
    src/Screens/ImGuizmoDemoScreen.cpp
    src/Screens/ImGuizmoDemoScreen.hpp
    src/Screens/InstancedRendererScreen.cpp
    src/Screens/InstancedRendererScreen.hpp
    src/Screens/LoadingScreen.cpp
    src/Screens/LoadingScreen.hpp
    src/Screens/MathExperimentsScreen.cpp
    src/Screens/MathExperimentsScreen.hpp
    src/Screens/MeshHittestScreen.cpp
    src/Screens/MeshHittestScreen.hpp
    src/Screens/MeshHittestWithBVHScreen.cpp
    src/Screens/MeshHittestWithBVHScreen.hpp
    src/Screens/MeshImporterScreen.cpp
    src/Screens/MeshImporterScreen.hpp
    src/Screens/MeshScreen.cpp
    src/Screens/MeshScreen.hpp
    src/Screens/ModelEditorScreen.cpp
    src/Screens/ModelEditorScreen.hpp
    src/Screens/RendererHelloTriangleScreen.cpp
    src/Screens/RendererHelloTriangleScreen.hpp
    src/Screens/SimulatorScreen.cpp
    src/Screens/SimulatorScreen.hpp
    src/Screens/SplashScreen.cpp
    src/Screens/SplashScreen.hpp
    src/Screens/UiModelViewerScreen.cpp
    src/Screens/UiModelViewerScreen.hpp

    src/Utils/Algorithms.cpp
    src/Utils/Algorithms.hpp
    src/Utils/Assertions.cpp
    src/Utils/Assertions.hpp
    src/Utils/CheckedIndex.hpp
    src/Utils/CircularBuffer.hpp
    src/Utils/ClonePtr.hpp
    src/Utils/Cpp20Shims.hpp
    src/Utils/CStringView.hpp
    src/Utils/Dag.hpp
    src/Utils/DedupedString.cpp
    src/Utils/DedupedString.hpp
    src/Utils/DefaultConstructOnCopy.hpp
    src/Utils/FileChangePoller.cpp
    src/Utils/FileChangePoller.hpp
    src/Utils/FilesystemHelpers.cpp
    src/Utils/FilesystemHelpers.hpp
    src/Utils/Macros.hpp
    src/Utils/MethodTestMacro.hpp
    src/Utils/Perf.cpp
    src/Utils/Perf.hpp
    src/Utils/ScopeGuard.hpp
    src/Utils/Spsc.hpp
    src/Utils/SynchronizedValue.hpp
    src/Utils/UID.cpp
    src/Utils/UID.hpp

    src/Widgets/AddBodyPopup.cpp
    src/Widgets/AddBodyPopup.hpp
    src/Widgets/AddComponentPopup.cpp
    src/Widgets/AddComponentPopup.hpp
    src/Widgets/ComponentDetails.cpp
    src/Widgets/ComponentDetails.hpp
    src/Widgets/ComponentHierarchy.cpp
    src/Widgets/ComponentHierarchy.hpp
    src/Widgets/CoordinateEditor.cpp
    src/Widgets/CoordinateEditor.hpp
    src/Widgets/LogViewer.cpp
    src/Widgets/LogViewer.hpp
    src/Widgets/MainMenu.cpp
    src/Widgets/MainMenu.hpp
    src/Widgets/ModelActionsMenuBar.cpp
    src/Widgets/ModelActionsMenuBar.hpp
    src/Widgets/ObjectPropertiesEditor.cpp
    src/Widgets/ObjectPropertiesEditor.hpp
    src/Widgets/ParamBlockEditorPopup.cpp
    src/Widgets/ParamBlockEditorPopup.hpp
    src/Widgets/PerfPanel.cpp
    src/Widgets/PerfPanel.hpp
    src/Widgets/ReassignSocketPopup.cpp
    src/Widgets/ReassignSocketPopup.hpp
    src/Widgets/SaveChangesPopup.cpp
    src/Widgets/SaveChangesPopup.hpp
    src/Widgets/Select1PFPopup.cpp
    src/Widgets/Select1PFPopup.hpp
    src/Widgets/Select2PFsPopup.cpp
    src/Widgets/Select2PFsPopup.hpp
    src/Widgets/SelectComponentPopup.hpp
    src/Widgets/SelectGeometryPopup.cpp
    src/Widgets/SelectGeometryPopup.hpp
    src/Widgets/StandardPopup.cpp
    src/Widgets/StandardPopup.hpp
    src/Widgets/UiModelViewer.cpp
    src/Widgets/UiModelViewer.hpp
)

# osccore: target properties
set_target_properties(osccore PROPERTIES

    # disable compiler-specific language extensions
    CXX_EXTENSIONS OFF

    # strictly require the requested C++ standard (e.g. C++17)
    CXX_STANDARD_REQUIRED YES

    # export compile commands, so clang-tidy works correctly
    EXPORT_COMPILE_COMMANDS ON
)


# osccore: compile-time includes
target_include_directories(osccore PUBLIC

    # so `#include "osc_config.hpp"` works
    ${PROJECT_BINARY_DIR}
)

# osccore: use all the stuff set by `osc_configuration`
target_link_libraries(osccore PUBLIC

    # so that it uses the standard flags etc.
    osc_configuration

    # see: GetDependencies.cmake
    osc-all-deps
)

# osccore: link options
target_link_options(osccore PUBLIC

    # MSVC (Windows)
    $<$<CXX_COMPILER_ID:MSVC>:

        # ignore error from ImGui, which uses locally-defined symbols
        /ignore:4217
    >

    # Linux /w GCC or Clang
    $<$<AND:$<BOOL:LINUX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:

        # make `ld` emit symbols for all functions - even if they aren't
        # externally used (with an exe, only `main` is used, really)
        #
        # this makes the binary a little bigger (~400 KB) but means
        # that the application can emit its own stack trace /w function
        # names - even in Release mode.
        -rdynamic
    >

    # GCC or Clang: handle OSC_USE_ASAN
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >
)


# ------------ configure test suite target ------------------ #

# osctests: main exe that links to `osccore` and exercises APIs etc.
add_executable(testosc

    tests/TestOsc.cpp
    tests/TestDedupedString.cpp
    tests/TestRenderer.cpp
    tests/TestRendererHelloTriangleScreen.cpp
)

# osctests: target properties
set_target_properties(testosc PROPERTIES

    # disable compiler-specific language extensions
    CXX_EXTENSIONS OFF

    # strictly require the requested C++ standard (e.g. C++17)
    CXX_STANDARD_REQUIRED YES

    # export compile commands, so clang-tidy works correctly
    EXPORT_COMPILE_COMMANDS ON
)

# osctests: link to `osccore` to exercise it
target_link_libraries(testosc PUBLIC

    # set compile options
    osc_configuration

    # link to the to-be-tested library
    osccore

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# osctests: tell CMake (+IDEs) to discover each test case
#
# this is how IDEs like Visual Studio integrate their test parser with
# the test framework
if(NOT APPLE)  # TODO: fix apple: it's trying to run this shit while installing, where the RPATH is set up in install-mode
    include(GoogleTest)
    gtest_discover_tests(testosc)
endif()


# ------------ configure top-level executable target ------------ #

# osc:
#
#     executable frontend to the `osccore` library
add_executable(osc

    # the CLI frontend
    src/osc.cpp

    # Windows: also link a resources file (rc)
    #
    # the resources file tells MSVC compiler how to compile non-source resources
    # into the output exe. Specifically, it's used to embed the application icon
    # into the `osc` exe
    $<$<CXX_COMPILER_ID:MSVC>:build_resources/resources.rc>
)

# osc: target properties
set_target_properties(osc PROPERTIES

    # disable compiler-specific language extensions
    CXX_EXTENSIONS OFF

    # strictly require the requested C++ standard (e.g. C++17)
    CXX_STANDARD_REQUIRED YES

    # export compile commands, so clang-tidy works correctly
    EXPORT_COMPILE_COMMANDS ON
)

# osc: link to necessary libraries
target_link_libraries(osc PUBLIC

    # so that it uses the standard flags etc.
    osc_configuration

    # so that it can boot into the main osc codebase
    osccore
)

# osc: link options
target_link_options(osc PUBLIC

    # MSVC (Windows)
    $<$<CXX_COMPILER_ID:MSVC>:

        # open as a desktop app, not CLI
        /SUBSYSTEM:windows

        # as above, call into `main`
        /ENTRY:mainCRTStartup
    >
)

# osc: handle installed library lookup locations
#
# - Windows: skip: just copy the DLLs into the same dir as the EXE at install time
# - Linux: setup RPATH with this presumed install dir layout:
#     bin/
#     lib/*.so
# - Mac: setup @executable_path with this presumed install dir layout:
#     lib/*.dylib
#     osc (exe)
if(LINUX)
    set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
elseif(APPLE)
    set_target_properties(osc PROPERTIES INSTALL_RPATH "@executable_path/lib")
endif()


# --------------- configure documentation target ------------------ #

# configure documentation generation
#
# if the user sets OSC_BUILD_DOCS then they *require* documentation
# to be generated. Documentation is written in `sphinx`, which should
# be tested here, and the documentation should be generated at
# install-time
if(OSC_BUILD_DOCS)

    # configure time: ensure the user's build system has 'sphinx-build'
    find_program(
        OSC_SPHINX_EXE
        "sphinx-build"
        DOC "Location of sphinx-build, which is used to compile the documentation"
        REQUIRED
    )

    # install time: run `sphinx-build` to build the docs
    install(
        CODE "execute_process(COMMAND ${OSC_SPHINX_EXE} -M html source build WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/docs\")"
    )

    # install time: copy the build docs to the appropriate packaging dir
    if(APPLE)

        # APPLE's `DOC` packaging location is (imho) wrong for DMG packaging.
        # It puts the docs outside the DMG in the user-facing drag and drop
        # GUI? So users are presented with 'osc' and 'share', rather than just
        # 'osc' (ewww)
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/build/html
            DESTINATION osc.app/Contents/MacOS/share/doc
        )

    else()
        # on other OSes, just use the CMake `DOC` default, which typically just
        # bundles it in $INSTALL_LOC/share/doc/html
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/build/html
            TYPE DOC
        )
    endif()
endif()


# -------------- development nice-to-haves ------------- #

# generate a dev-centric `osc.toml`
#
#     - this causes osc to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/build_resources/DEV_osc.toml.in" "${CMAKE_BINARY_DIR}/osc.toml")

# (on Windows): copy runtime DLLs into the `osccore` build dir
#
#     - this is so that devs can run `osc.exe` from an IDE without having to
#       copy libraries around manually etc.
#
#     - this isn't necessary with Unix'es because CMake sets the RPATH up to
#       point to wherever the libraries are located
#
#     - at install time, the libraries are copied into the install location
if (WIN32)
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        add_custom_command(
            TARGET osccore
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_FILE_TO_COPY}" "$<TARGET_FILE_DIR:osccore>"
        )
    endforeach()
endif()


# ------------- installation / packaging (CPack) -------------- #

# install-time: also package any required system libraries
include(InstallRequiredSystemLibraries)

# install-time: set necessary CPack variables
set(CPACK_PACKAGE_NAME "OpenSimCreator")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "Adam Kewley")
set(CPACK_PACKAGE_CONTACT "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ComputationalBiomechanicsLab/opensim-creator")
set(CPACK_PACKAGE_DESCRIPTION "A standalone GUI for building OpenSim models")
set(CPACK_PACKAGE_EXECUTABLES "osc;OpenSimCreator")

# Windows install/packaging:
#
#     - copy osc.exe + all DLLs into the bin/ dir
#
#     - this creates a "fat" build in bin/, where all the necessary libraries are just
#       packaged with the install (so users only have to install one thing)
#
#     - see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
#
#     - packaging: uses NSIS.exe : get it from https://nsis.sourceforge.io/Download
if (WIN32)

    # install-time: install osc.exe
    install(TARGETS osc)

    # install-time: install all dependency DLLs next to the exe
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION bin)
    endforeach()

    # install-time: install a user-facing `osc.toml` config file
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
        RENAME "osc.toml"
        DESTINATION "."
    )

    # install-time: copy `resources/` (assets) dir
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        DESTINATION "."
    )

    # packaging: use NSIS to package everything into a self-extracting installer
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenSimCreator")
    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.ico")

    # BROKE: set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    #
    # the reason it's broke is because CMake has changed something in more-recent
    # versions that breaks it. There's a PR about it here:
    #
    # - https://gitlab.kitware.com/cmake/cmake/-/issues/23001
    #
    # We disable it in OpenSim Creator also because it hits our users and these
    # early versions of OpenSim Creator can just overwrite-install. See issue:
    #
    # - https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/131

    set(CPACK_NSIS_HELP_LINK ${CPACK_PACKAGE_HOMEPAGE_URL})
    set(CPACK_NSIS_CONTACT "contact@adamkewley.com")
    set(CPACK_NSIS_MODIFY_PATH OFF)  # do not prompt the user to modify the PATH
    set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
    # set(CPACK_NSIS_MUI_FINISHPAGE_RUN osc)  # DISALLOWED: boots app with admin privs: see #95 (or inkscape's CMake file)
    # set(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\OpenSimCreator\\\\osc.lnk' '\$INSTDIR\\\\bin\\\\osc.exe'")
endif()

# Linux install/packaging:
#
#     - copy osc + libraries into a standard-layout dir
#           bin/  # executables
#           lib/  # libraries (.so)
#           resources/  # arch-independent runtime resources
#
#     - this dir is "done", and could be sent as a ZIP to an end-user
#
#     - **BUT** the way that OpenSim/osc is built assumes the user has all the
#       necessary runtime libraries available
#
#     - so the packaging step creates a .deb file that declares which runtime dependencies
#       a client should have (or automatically acquire via apt)
#
#     - note: the package assumes the user has a compatible OpenGL driver installed, servers
#             can try to "fake" this by running:
#
#           apt-get install libopengl0 libglx0 libglu1-mesa
if (LINUX)
    # install-time: install osc executable
    install(TARGETS osc DESTINATION bin/)

    # install-time: install all dependency `.so` files into `lib/`
    #
    # the RPATH stuff ensures that the exe can find these at runtime
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION lib/)
    endforeach()

    # install-time: install a user-facing `osc.toml` config file
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
        RENAME "osc.toml"
        DESTINATION "."
    )

    # install-time: copy `resources/` (assets) dir
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        DESTINATION
        "."
    )

    # packaging: package installation as a DEB
    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libblas3, liblapack3, libstdc++6")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # packaging: configure a script that creates a symlink /usr/local/bin/osc --> /opt/osc/bin/osc
    configure_file("${PROJECT_SOURCE_DIR}/build_resources/postinst.in" "postinst" @ONLY)

    # packaging: configure a script that destroys the above symlink on uninstall
    configure_file("${PROJECT_SOURCE_DIR}/build_resources/postrm.in" "postrm" @ONLY)

    # packaging: tell debian packager to use the scripts for postinst and postrm actions
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")
endif()

# Apple OSX packaging:
#
# - Create a DMG (archive) installer that packages the whole application +
#   libraries into a single directory tree that can be copied to
#   /Applications/osc
#
if (APPLE)

    # install-time: install osc into package root (Application/ dir requirement)
    install(TARGETS osc DESTINATION osc.app/Contents/MacOS/)

    # install-time: install bundled libraries into lib/
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION osc.app/Contents/MacOS/lib/)
    endforeach()

    # install-time: install a user-facing `osc.toml` config file
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
        RENAME "osc.toml"
        DESTINATION osc.app/Contents/MacOS/
    )

    # install-time: install an `Info.plist` file
    #
    # it's mac-specific XML file that tells Mac OSX about where the
    # executable is, what the icon is, etc.
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/Info.plist"
        DESTINATION osc.app/Contents/
    )

    # install-time: copy `resources/` (assets) dir
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        DESTINATION osc.app/Contents/MacOS/
    )

    # install-time: copy the Mac-specific desktop icon (.icns)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/osc.icns"
        DESTINATION osc.app/Contents/Resources/
    )

    set(CPACK_GENERATOR DragNDrop)
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
