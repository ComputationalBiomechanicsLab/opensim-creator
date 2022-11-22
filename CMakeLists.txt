cmake_minimum_required(VERSION 3.10)

project(osc VERSION 0.3.1 LANGUAGES C CXX)

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

set(
    OSC_APPNAME "OpenSim Creator"
    CACHE STRING
    "user-facing name of the application"
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
        -Wvla
        -Wdisabled-optimization
        -Wpacked
        -Wformat-security

        # disable: broken by STB headers
        -Wno-unused

        # disable: broken by STB headers
        -Wno-sign-compare

        # disable: broken by STB headers
        -Wno-missing-field-initializers

        # regardless of debug/release, pin the frame pointer register
        # so that stack traces are sane when debugging (even in Release).
        #
        # This adds some overhead (pins one register and requires callers
        # to setup their base pointers etc.) but makes debugging + profiling
        # the application much easier, even in release mode
        -fno-omit-frame-pointer

        # if address sanitizer is specified, use it
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>

        # disabled: doesn't work in some contexts where forward declarations are necessary
        # -Wredundant-decls
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
    src/Bindings/Gl.cpp
    src/Bindings/Gl.hpp
    src/Bindings/GlGlm.hpp

    src/Formats/CSV.hpp
    src/Formats/CSV.cpp
    src/Formats/DAE.hpp
    src/Formats/DAE.cpp
    src/Formats/OBJ.cpp
    src/Formats/OBJ.hpp
    src/Formats/STL.cpp
    src/Formats/STL.hpp

    src/Graphics/AnnotatedImage.hpp
    src/Graphics/CachedSceneRenderer.cpp
    src/Graphics/CachedSceneRenderer.hpp
    src/Graphics/BlitFlags.hpp
    src/Graphics/Camera.hpp
    src/Graphics/CameraClearFlags.hpp
    src/Graphics/CameraProjection.hpp
    src/Graphics/Color.cpp
    src/Graphics/Color.hpp
    src/Graphics/DepthStencilFormat.hpp
    src/Graphics/Graphics.hpp
    src/Graphics/GraphicsContext.hpp
    src/Graphics/GraphicsHelpers.cpp
    src/Graphics/GraphicsHelpers.hpp
    src/Graphics/GraphicsImplementation.cpp
    src/Graphics/Image.cpp
    src/Graphics/Image.hpp
    src/Graphics/ImageAnnotation.hpp
    src/Graphics/ImageFlags.hpp
    src/Graphics/ImageGen.cpp
    src/Graphics/ImageGen.hpp
    src/Graphics/Material.hpp
    src/Graphics/MaterialPropertyBlock.hpp
    src/Graphics/MeshCache.cpp
    src/Graphics/MeshCache.hpp
    src/Graphics/MeshGen.cpp
    src/Graphics/MeshGen.hpp
    src/Graphics/MeshIndicesView.hpp
    src/Graphics/Mesh.hpp
    src/Graphics/MeshTopography.hpp
    src/Graphics/RenderTexture.hpp
    src/Graphics/RenderTextureDescriptor.hpp
    src/Graphics/RenderTextureFormat.hpp
    src/Graphics/Rgb24.hpp
    src/Graphics/Rgba32.hpp
    src/Graphics/SceneCollision.hpp
    src/Graphics/SceneDecoration.cpp
    src/Graphics/SceneDecoration.hpp
    src/Graphics/SceneDecorationFlags.hpp
    src/Graphics/SceneRenderer.cpp
    src/Graphics/SceneRenderer.hpp
    src/Graphics/SceneRendererParams.cpp
    src/Graphics/SceneRendererParams.hpp
    src/Graphics/Shader.hpp
    src/Graphics/ShaderCache.cpp
    src/Graphics/ShaderCache.hpp
    src/Graphics/ShaderLocationIndex.hpp
    src/Graphics/ShaderType.hpp
    src/Graphics/Texture2D.hpp
    src/Graphics/TextureFilterMode.hpp
    src/Graphics/TextureGen.cpp
    src/Graphics/TextureGen.hpp
    src/Graphics/TextureWrapMode.hpp

    src/Maths/AABB.hpp
    src/Maths/BVH.hpp
    src/Maths/CollisionTests.hpp
    src/Maths/Constants.hpp
    src/Maths/Disc.hpp
    src/Maths/EulerPerspectiveCamera.hpp
    src/Maths/Line.hpp
    src/Maths/MathHelpers.hpp
    src/Maths/MathsImplementation.cpp
    src/Maths/Plane.hpp
    src/Maths/PolarPerspectiveCamera.hpp
    src/Maths/RayCollision.hpp
    src/Maths/Rect.hpp
    src/Maths/Segment.hpp
    src/Maths/Sphere.hpp
    src/Maths/Tetrahedron.hpp
    src/Maths/Transform.hpp
    src/Maths/Triangle.hpp

    src/OpenSimBindings/MiddlewareAPIs/EditorAPI.hpp
    src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp
    src/OpenSimBindings/MiddlewareAPIs/SimulatorUIAPI.hpp
    src/OpenSimBindings/Screens/MainUIScreen.cpp
    src/OpenSimBindings/Screens/MainUIScreen.hpp
    src/OpenSimBindings/Tabs/Experimental/MeshHittestTab.cpp
    src/OpenSimBindings/Tabs/Experimental/MeshHittestTab.hpp
    src/OpenSimBindings/Tabs/Experimental/ModelWarpingTab.cpp
    src/OpenSimBindings/Tabs/Experimental/ModelWarpingTab.hpp
    src/OpenSimBindings/Tabs/Experimental/PreviewExperimentalDataTab.cpp
    src/OpenSimBindings/Tabs/Experimental/PreviewExperimentalDataTab.hpp
    src/OpenSimBindings/Tabs/Experimental/RendererGeometryShaderTab.cpp
    src/OpenSimBindings/Tabs/Experimental/RendererGeometryShaderTab.hpp
    src/OpenSimBindings/Tabs/Experimental/TPS2DTab.cpp
    src/OpenSimBindings/Tabs/Experimental/TPS2DTab.hpp
    src/OpenSimBindings/Tabs/Experimental/TPS3DTab.cpp
    src/OpenSimBindings/Tabs/Experimental/TPS3DTab.hpp
    src/OpenSimBindings/Tabs/LoadingTab.cpp
    src/OpenSimBindings/Tabs/LoadingTab.hpp
    src/OpenSimBindings/Tabs/MeshImporterTab.cpp
    src/OpenSimBindings/Tabs/MeshImporterTab.hpp
    src/OpenSimBindings/Tabs/ModelEditorTab.cpp
    src/OpenSimBindings/Tabs/ModelEditorTab.hpp
    src/OpenSimBindings/Tabs/PerformanceAnalyzerTab.cpp
    src/OpenSimBindings/Tabs/PerformanceAnalyzerTab.hpp
    src/OpenSimBindings/Tabs/SimulatorTab.cpp
    src/OpenSimBindings/Tabs/SimulatorTab.hpp
    src/OpenSimBindings/Tabs/SplashTab.cpp
    src/OpenSimBindings/Tabs/SplashTab.hpp
    src/OpenSimBindings/Widgets/AddBodyPopup.cpp
    src/OpenSimBindings/Widgets/AddBodyPopup.hpp
    src/OpenSimBindings/Widgets/AddComponentPopup.cpp
    src/OpenSimBindings/Widgets/AddComponentPopup.hpp
    src/OpenSimBindings/Widgets/BasicWidgets.cpp
    src/OpenSimBindings/Widgets/BasicWidgets.hpp
    src/OpenSimBindings/Widgets/ComponentContextMenu.cpp
    src/OpenSimBindings/Widgets/ComponentContextMenu.hpp
    src/OpenSimBindings/Widgets/ComponentDetails.cpp
    src/OpenSimBindings/Widgets/ComponentDetails.hpp
    src/OpenSimBindings/Widgets/CoordinateEditor.cpp
    src/OpenSimBindings/Widgets/CoordinateEditor.hpp
    src/OpenSimBindings/Widgets/EditorTabStatusBar.cpp
    src/OpenSimBindings/Widgets/EditorTabStatusBar.hpp
    src/OpenSimBindings/Widgets/MainMenu.cpp
    src/OpenSimBindings/Widgets/MainMenu.hpp
    src/OpenSimBindings/Widgets/ModelActionsMenuItems.cpp
    src/OpenSimBindings/Widgets/ModelActionsMenuItems.hpp
    src/OpenSimBindings/Widgets/ModelMusclePlotPanel.cpp
    src/OpenSimBindings/Widgets/ModelMusclePlotPanel.hpp
    src/OpenSimBindings/Widgets/NavigatorPanel.cpp
    src/OpenSimBindings/Widgets/NavigatorPanel.hpp
    src/OpenSimBindings/Widgets/ObjectPropertiesEditor.cpp
    src/OpenSimBindings/Widgets/ObjectPropertiesEditor.hpp
    src/OpenSimBindings/Widgets/OutputWatchesPanel.cpp
    src/OpenSimBindings/Widgets/OutputWatchesPanel.hpp
    src/OpenSimBindings/Widgets/ParamBlockEditorPopup.cpp
    src/OpenSimBindings/Widgets/ParamBlockEditorPopup.hpp
    src/OpenSimBindings/Widgets/PropertiesPanel.cpp
    src/OpenSimBindings/Widgets/PropertiesPanel.hpp
    src/OpenSimBindings/Widgets/ReassignSocketPopup.cpp
    src/OpenSimBindings/Widgets/ReassignSocketPopup.hpp
    src/OpenSimBindings/Widgets/Select1PFPopup.cpp
    src/OpenSimBindings/Widgets/Select1PFPopup.hpp
    src/OpenSimBindings/Widgets/SelectComponentPopup.cpp
    src/OpenSimBindings/Widgets/SelectComponentPopup.hpp
    src/OpenSimBindings/Widgets/SelectGeometryPopup.cpp
    src/OpenSimBindings/Widgets/SelectGeometryPopup.hpp
    src/OpenSimBindings/Widgets/SimulationOutputPlot.cpp
    src/OpenSimBindings/Widgets/SimulationOutputPlot.hpp
    src/OpenSimBindings/Widgets/SimulationScrubber.cpp
    src/OpenSimBindings/Widgets/SimulationScrubber.hpp
    src/OpenSimBindings/Widgets/UiModelViewer.cpp
    src/OpenSimBindings/Widgets/UiModelViewer.hpp
    src/OpenSimBindings/ActionFunctions.cpp
    src/OpenSimBindings/ActionFunctions.hpp
    src/OpenSimBindings/BasicModelStatePair.cpp
    src/OpenSimBindings/BasicModelStatePair.hpp
    src/OpenSimBindings/ComponentOutputExtractor.cpp
    src/OpenSimBindings/ComponentOutputExtractor.hpp
    src/OpenSimBindings/CustomDecorationOptions.hpp
    src/OpenSimBindings/ForwardDynamicSimulation.cpp
    src/OpenSimBindings/ForwardDynamicSimulation.hpp
    src/OpenSimBindings/ForwardDynamicSimulator.cpp
    src/OpenSimBindings/ForwardDynamicSimulator.hpp
    src/OpenSimBindings/ForwardDynamicSimulatorParams.cpp
    src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp
    src/OpenSimBindings/IntegratorMethod.cpp
    src/OpenSimBindings/IntegratorMethod.hpp
    src/OpenSimBindings/IntegratorOutputExtractor.cpp
    src/OpenSimBindings/IntegratorOutputExtractor.hpp
    src/OpenSimBindings/ModelStateCommit.cpp
    src/OpenSimBindings/ModelStateCommit.hpp
    src/OpenSimBindings/MultiBodySystemOutputExtractor.cpp
    src/OpenSimBindings/MultiBodySystemOutputExtractor.hpp
    src/OpenSimBindings/MuscleColoringStyle.cpp
    src/OpenSimBindings/MuscleColoringStyle.hpp
    src/OpenSimBindings/MuscleDecorationStyle.cpp
    src/OpenSimBindings/MuscleDecorationStyle.hpp
    src/OpenSimBindings/MuscleSizingStyle.cpp
    src/OpenSimBindings/MuscleSizingStyle.hpp
    src/OpenSimBindings/OpenSimApp.cpp
    src/OpenSimBindings/OpenSimApp.hpp
    src/OpenSimBindings/OpenSimHelpers.cpp
    src/OpenSimBindings/OpenSimHelpers.hpp
    src/OpenSimBindings/OutputExtractor.cpp
    src/OpenSimBindings/OutputExtractor.hpp
    src/OpenSimBindings/ParamBlock.hpp
    src/OpenSimBindings/ParamBlock.cpp
    src/OpenSimBindings/ParamValue.hpp
    src/OpenSimBindings/SimTKHelpers.cpp
    src/OpenSimBindings/SimTKHelpers.hpp
    src/OpenSimBindings/Simulation.hpp
    src/OpenSimBindings/SimulationClock.hpp
    src/OpenSimBindings/SimulationModelStatePair.cpp
    src/OpenSimBindings/SimulationModelStatePair.hpp
    src/OpenSimBindings/SimulationReport.cpp
    src/OpenSimBindings/SimulationReport.hpp
    src/OpenSimBindings/SimulationStatus.cpp
    src/OpenSimBindings/SimulationStatus.hpp
    src/OpenSimBindings/StoFileSimulation.hpp
    src/OpenSimBindings/StoFileSimulation.cpp
    src/OpenSimBindings/TPS3D.cpp
    src/OpenSimBindings/TPS3D.hpp
    src/OpenSimBindings/TypeRegistry.cpp
    src/OpenSimBindings/TypeRegistry.hpp
    src/OpenSimBindings/UndoableModelStatePair.cpp
    src/OpenSimBindings/UndoableModelStatePair.hpp
    src/OpenSimBindings/VirtualConstModelStatePair.hpp
    src/OpenSimBindings/VirtualModelStatePair.hpp
    src/OpenSimBindings/VirtualOutputExtractor.hpp
    src/OpenSimBindings/VirtualSimulation.hpp

    src/Platform/App.cpp
    src/Platform/App.hpp
    src/Platform/AppClock.hpp
    src/Platform/Config.cpp
    src/Platform/Config.hpp
    src/Platform/IoPoller.cpp
    src/Platform/IoPoller.hpp
    src/Platform/Log.cpp
    src/Platform/Log.hpp
    src/Platform/MouseState.hpp
    src/Platform/os.cpp
    src/Platform/os.hpp
    src/Platform/RecentFile.hpp
    src/Platform/Styling.hpp
    src/Platform/ValueHolder.hpp

    src/Screens/CookiecutterScreen.cpp
    src/Screens/CookiecutterScreen.hpp
    src/Screens/Screen.hpp

    src/Tabs/Experiments/CustomWidgetsTab.cpp
    src/Tabs/Experiments/CustomWidgetsTab.hpp
    src/Tabs/Experiments/HittestTab.cpp
    src/Tabs/Experiments/HittestTab.hpp
    src/Tabs/Experiments/ImGuiDemoTab.cpp
    src/Tabs/Experiments/ImGuiDemoTab.hpp
    src/Tabs/Experiments/ImGuizmoDemoTab.cpp
    src/Tabs/Experiments/ImGuizmoDemoTab.hpp
    src/Tabs/Experiments/ImPlotDemoTab.cpp
    src/Tabs/Experiments/ImPlotDemoTab.hpp
    src/Tabs/Experiments/MeshGenTestTab.cpp
    src/Tabs/Experiments/MeshGenTestTab.hpp
    src/Tabs/Experiments/RendererBasicLightingTab.cpp
    src/Tabs/Experiments/RendererBasicLightingTab.hpp
    src/Tabs/Experiments/RendererBlendingTab.cpp
    src/Tabs/Experiments/RendererBlendingTab.hpp
    src/Tabs/Experiments/RendererCoordinateSystemsTab.cpp
    src/Tabs/Experiments/RendererCoordinateSystemsTab.hpp
    src/Tabs/Experiments/RendererFramebuffersTab.cpp
    src/Tabs/Experiments/RendererFramebuffersTab.hpp
    src/Tabs/Experiments/RendererHelloTriangleTab.cpp
    src/Tabs/Experiments/RendererHelloTriangleTab.hpp
    src/Tabs/Experiments/RendererLightingMapsTab.cpp
    src/Tabs/Experiments/RendererLightingMapsTab.hpp
    src/Tabs/Experiments/RendererMultipleLightsTab.cpp
    src/Tabs/Experiments/RendererMultipleLightsTab.hpp
    src/Tabs/Experiments/RendererSDFTab.cpp
    src/Tabs/Experiments/RendererSDFTab.hpp
    src/Tabs/Experiments/RendererTexturingTab.cpp
    src/Tabs/Experiments/RendererTexturingTab.hpp
    src/Tabs/CookiecutterTab.cpp
    src/Tabs/CookiecutterTab.hpp
    src/Tabs/ErrorTab.cpp
    src/Tabs/ErrorTab.hpp
    src/Tabs/ScreenshotTab.cpp
    src/Tabs/ScreenshotTab.hpp
    src/Tabs/Tab.cpp
    src/Tabs/Tab.hpp
    src/Tabs/TabHost.cpp
    src/Tabs/TabHost.hpp
    src/Tabs/TabRegistry.cpp
    src/Tabs/TabRegistry.hpp

    src/Utils/Algorithms.cpp
    src/Utils/Algorithms.hpp
    src/Utils/Assertions.cpp
    src/Utils/Assertions.hpp
    src/Utils/CircularBuffer.hpp
    src/Utils/ClonePtr.hpp
    src/Utils/Cow.hpp
    src/Utils/Cpp20Shims.hpp
    src/Utils/CStringView.hpp
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
    src/Utils/UndoRedo.cpp
    src/Utils/UndoRedo.hpp

    src/Widgets/GuiRuler.cpp
    src/Widgets/GuiRuler.hpp
    src/Widgets/LogViewer.cpp
    src/Widgets/LogViewer.hpp
    src/Widgets/LogViewerPanel.cpp
    src/Widgets/LogViewerPanel.hpp
    src/Widgets/PerfPanel.cpp
    src/Widgets/PerfPanel.hpp
    src/Widgets/Popup.hpp
    src/Widgets/RedoButton.cpp
    src/Widgets/RedoButton.hpp
    src/Widgets/SaveChangesPopupConfig.hpp
    src/Widgets/SaveChangesPopup.cpp
    src/Widgets/SaveChangesPopup.hpp
    src/Widgets/SceneViewer.cpp
    src/Widgets/SceneViewer.hpp
    src/Widgets/StandardPanel.cpp
    src/Widgets/StandardPanel.hpp
    src/Widgets/StandardPopup.cpp
    src/Widgets/StandardPopup.hpp
    src/Widgets/UndoButton.cpp
    src/Widgets/UndoButton.hpp
    src/Widgets/UndoRedoPanel.cpp
    src/Widgets/UndoRedoPanel.hpp
    src/Widgets/VirtualPanel.hpp
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
add_executable(testosc EXCLUDE_FROM_ALL

    tests/TestAlgorithms.cpp
    tests/TestBVH.cpp
    tests/TestCSV.cpp
    tests/TestDedupedString.cpp
    tests/TestForwardDynamicSimulation.cpp
    tests/TestImage.cpp
    tests/TestOpenSim.cpp
    tests/TestOpenSimHelpers.cpp
    tests/TestOsc.cpp
    tests/TestRenderer.cpp
    tests/TestTypeRegistry.cpp
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
