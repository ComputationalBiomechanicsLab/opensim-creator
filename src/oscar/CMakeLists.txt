# prefer to choose GLVND for OpenGL
cmake_policy(SET CMP0072 NEW)

# define MSVC warning flags (e.g. /W3 /W4) ourselves
cmake_policy(SET CMP0092 NEW)

# find dependencies (usually built+installed from `third_party/`)
find_package(OpenGL REQUIRED)
find_package(glew REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)
find_package(SDL2 REQUIRED CONFIG)
find_package(nativefiledialog REQUIRED CONFIG)
find_package(imgui REQUIRED CONFIG)
find_package(IconFontCppHeaders REQUIRED CONFIG)
find_package(ImGuizmo REQUIRED CONFIG)
find_package(implot REQUIRED CONFIG)
find_package(stb REQUIRED CONFIG)
find_package(lunasvg REQUIRED CONFIG)
find_package(span-lite REQUIRED CONFIG)
find_package(tomlplusplus REQUIRED CONFIG)
find_package(unordered_dense REQUIRED CONFIG)

# osc_configuration:
#
#     an INTERFACE target that sets common options that are used by the real
#     targets (e.g. use C++17, use these flags, etc.)
add_library(osc_configuration INTERFACE)

# osc_configuration: set general compiler options used by all targets
target_compile_options(osc_configuration INTERFACE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

        # treat all warnings as errors
        /WX

        # keep frame pointers around, so that runtime stack traces can be dumped to error logs
        /Oy-

        # disable MSVC's permissive mode to ensure better ISO C++ conformance
        /permissive-

        # ensure `volatile` variables follow (less-strict) ISO standards
        /volatile:iso

        # ensure preprocessor is standards conformant
        /Zc:preprocessor

        # assume `new` throws when memory cannot be allocated (ISO conformance)
        /Zc:throwingNew

        # only handle standard, synchronous, C++ exceptions (ISO) and treat asynchronous
        # Windows/system structured exceptions as fatal, non-catchable, errors
        /EHsc

        # if requested, enable MSVC's libASAN
        $<$<BOOL:${OSC_USE_ASAN}>:/fsanitize=address>
    >

    # gcc AND clang flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:

        # treat all warnings as errors
        -Werror

        # enable all basic warnings
        -Wall

        # enable extra warnings
        -Wextra

        # enable pedantically extra warnings
        -pedantic

        # warn if using an uninitialized variable
        -Wuninitialized

        # warn if an uninitialized variable is initialized by itself
        -Winit-self

        # warn if a variable's name shadows an earlier (higher scope) variable with the same name
        -Wshadow

        # warn if data is casted to a higher alignment (e.g. char -> int64_t)
        #
        # todo: set to =strict
        -Wcast-align

        # disabled: broken in stb
        # warn if casting C string constants from `char const*` to `char*`
        # -Wwrite-strings

        # warn if a dangling else is detected
        -Wdangling-else

        # warn if a date-time macro expansion is not reproducible
        -Wdate-time

        # warn if a variable-length array (VLA) is detected (disallowed)
        -Wvla

        # warn if the compiler detected that the code is way too complex to optimize
        -Wdisabled-optimization

        # warn if a structure is given the 'packed' attribute (disallowed: alignment)
        -Wpacked

        # disabled: broken in stb
        # warn if a case in a switch statement implicitly falls through after a statement
        # -Wimplicit-fallthrough

        # disabled: requires newer gcc
        # warn if calls to `strcmp` and `strncmp` are determined to be invalid at compile-time
        # -Wstring-compare

        # warn if insecure string formatting (e.g. for printf) is detected
        -Wformat-security

        # disabled: requires newer gcc
        # warn if trying to allocate 0 bytes of memory using an allocation function (could be undef behavior)
        # -Walloc-zero

        # disabled: requires newer gcc
        # warn if using trampoline functions (requires executable stack)
        # -Wtrampolines

        # disabled: broken in STB
        # warn if a pointer is cast in a C-style cast in such a way that it removes qualifiers (e.g. char const* -> char*)
        # -Wcast-qual

        # disabled: broken in osc
        # warn if an implicit conversion may alter a value
        # -Wconversion

        # disabled: requires newer gcc
        # warn if a suspicous use of a logical operator is detected (e.g. i < 0 && i < 0)
        # -Wlogical-op

        # disabled: requires newer gcc
        # warn if a float constant does not have the 'f' suffix
        # -Wunsuffixed-float-constants

        # todo: warn if using now-deprecated C/C++ constructs
        # -Wtraditional

        -Wno-multichar

        # disable: broken by STB headers
        -Wno-unused

        # disable: broken by STB headers
        -Wno-sign-compare

        # disable: broken by STB headers
        -Wno-missing-field-initializers

        # disabled: requires newer gcc and broken by nativefiledialog
        # -Wno-format-truncation

        # disable: offsetof used in OSC's component/propertytree API
        -Wno-invalid-offsetof

        # disabled: doesn't work in some contexts where forward declarations are necessary
        # -Wredundant-decls

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


# oscar_config.hpp:
#
#     configuration file that contains configuration-time vals (e.g. version, libs)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/OscarStaticConfig.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/oscar/OscarStaticConfig.hpp"
)

#------------- configure core library target ------------- #

# [lib]oscar:
#
#     core platform library for window creation, rendering, maths, etc.
add_library(oscar STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/generated/oscar/OscarStaticConfig.hpp

    Bindings/GlmHelpers.cpp
    Bindings/GlmHelpers.hpp
    Bindings/ImGuiHelpers.cpp
    Bindings/ImGuiHelpers.hpp
    Bindings/ImGuizmoHelpers.cpp
    Bindings/ImGuizmoHelpers.hpp
    Bindings/SDL2Helpers.hpp
    Bindings/Gl.cpp
    Bindings/Gl.hpp
    Bindings/GlGlm.hpp

    Formats/CSV.hpp
    Formats/CSV.cpp
    Formats/DAE.hpp
    Formats/DAE.cpp
    Formats/OBJ.cpp
    Formats/OBJ.hpp
    Formats/STL.cpp
    Formats/STL.hpp
    Formats/SVG.hpp
    Formats/SVG.cpp

    Graphics/AnnotatedImage.hpp
    Graphics/CachedSceneRenderer.cpp
    Graphics/CachedSceneRenderer.hpp
    Graphics/BlitFlags.hpp
    Graphics/Camera.hpp
    Graphics/CameraClearFlags.hpp
    Graphics/CameraProjection.hpp
    Graphics/Color.cpp
    Graphics/Color.hpp
    Graphics/Cubemap.hpp
    Graphics/CubemapFace.hpp
    Graphics/DepthStencilFormat.hpp
    Graphics/Graphics.hpp
    Graphics/GraphicsContext.hpp
    Graphics/GraphicsHelpers.cpp
    Graphics/GraphicsHelpers.hpp
    Graphics/GraphicsImplementation.cpp
    Graphics/Icon.hpp
    Graphics/IconCache.cpp
    Graphics/IconCache.hpp
    Graphics/Image.cpp
    Graphics/Image.hpp
    Graphics/ImageAnnotation.hpp
    Graphics/ImageFlags.hpp
    Graphics/ImageGen.cpp
    Graphics/ImageGen.hpp
    Graphics/Material.hpp
    Graphics/MaterialPropertyBlock.hpp
    Graphics/MeshCache.cpp
    Graphics/MeshCache.hpp
    Graphics/MeshGen.cpp
    Graphics/MeshGen.hpp
    Graphics/MeshIndicesView.hpp
    Graphics/Mesh.hpp
    Graphics/MeshTopology.hpp
    Graphics/RenderBuffer.hpp
    Graphics/RenderBufferLoadAction.hpp
    Graphics/RenderBufferStoreAction.hpp
    Graphics/RenderTarget.cpp
    Graphics/RenderTarget.hpp
    Graphics/RenderTargetAttachment.cpp
    Graphics/RenderTargetAttachment.hpp
    Graphics/RenderTargetColorAttachment.cpp
    Graphics/RenderTargetColorAttachment.hpp
    Graphics/RenderTargetDepthAttachment.cpp
    Graphics/RenderTargetDepthAttachment.hpp
    Graphics/RenderTexture.hpp
    Graphics/RenderTextureDescriptor.hpp
    Graphics/RenderTextureFormat.hpp
    Graphics/RenderTextureReadWrite.hpp
    Graphics/Rgb24.hpp
    Graphics/Rgba32.hpp
    Graphics/SceneCollision.hpp
    Graphics/SceneDecoration.cpp
    Graphics/SceneDecoration.hpp
    Graphics/SceneDecorationFlags.hpp
    Graphics/SceneRenderer.cpp
    Graphics/SceneRenderer.hpp
    Graphics/SceneRendererParams.cpp
    Graphics/SceneRendererParams.hpp
    Graphics/Shader.hpp
    Graphics/ShaderCache.cpp
    Graphics/ShaderCache.hpp
    Graphics/ShaderLocationIndex.hpp
    Graphics/ShaderType.hpp
    Graphics/SimpleSceneDecoration.hpp
    Graphics/Texture2D.hpp
    Graphics/TextureFilterMode.hpp
    Graphics/TextureFormat.hpp
    Graphics/TextureGen.cpp
    Graphics/TextureGen.hpp
    Graphics/TextureWrapMode.hpp

    Maths/AABB.hpp
    Maths/BVH.hpp
    Maths/CollisionTests.hpp
    Maths/Constants.hpp
    Maths/Disc.hpp
    Maths/EulerPerspectiveCamera.hpp
    Maths/Line.hpp
    Maths/MathHelpers.hpp
    Maths/MathsImplementation.cpp
    Maths/Plane.hpp
    Maths/PointDirection.hpp
    Maths/PolarPerspectiveCamera.hpp
    Maths/RayCollision.hpp
    Maths/Rect.hpp
    Maths/Segment.hpp
    Maths/Sphere.hpp
    Maths/Tetrahedron.hpp
    Maths/Transform.hpp
    Maths/Triangle.hpp

    Panels/LogViewerPanel.cpp
    Panels/LogViewerPanel.hpp
    Panels/Panel.hpp
    Panels/PanelManager.cpp
    Panels/PanelManager.hpp
    Panels/PerfPanel.cpp
    Panels/PerfPanel.hpp
    Panels/StandardPanel.cpp
    Panels/StandardPanel.hpp
    Panels/ToggleablePanelFlags.hpp
    Panels/UndoRedoPanel.cpp
    Panels/UndoRedoPanel.hpp

    Platform/App.cpp
    Platform/App.hpp
    Platform/AppClock.hpp
    Platform/Config.cpp
    Platform/Config.hpp
    Platform/IoPoller.cpp
    Platform/IoPoller.hpp
    Platform/Log.cpp
    Platform/Log.hpp
    Platform/MouseState.hpp
    Platform/os.cpp
    Platform/os.hpp
    Platform/RecentFile.hpp
    Platform/Styling.hpp

    Screens/CookiecutterScreen.cpp
    Screens/CookiecutterScreen.hpp
    Screens/Screen.hpp

    Tabs/Demos/ImGuiDemoTab.cpp
    Tabs/Demos/ImGuiDemoTab.hpp
    Tabs/Demos/ImGuizmoDemoTab.cpp
    Tabs/Demos/ImGuizmoDemoTab.hpp
    Tabs/Demos/ImPlotDemoTab.cpp
    Tabs/Demos/ImPlotDemoTab.hpp

    Tabs/Experiments/CustomWidgetsTab.cpp
    Tabs/Experiments/CustomWidgetsTab.hpp
    Tabs/Experiments/HittestTab.cpp
    Tabs/Experiments/HittestTab.hpp
    Tabs/Experiments/MeshGenTestTab.cpp
    Tabs/Experiments/MeshGenTestTab.hpp
    Tabs/Experiments/RendererSDFTab.cpp
    Tabs/Experiments/RendererSDFTab.hpp

    Tabs/LearnOpenGL/LOGLBasicLightingTab.cpp
    Tabs/LearnOpenGL/LOGLBasicLightingTab.hpp
    Tabs/LearnOpenGL/LOGLBlendingTab.cpp
    Tabs/LearnOpenGL/LOGLBlendingTab.hpp
    Tabs/LearnOpenGL/LOGLBloomTab.cpp
    Tabs/LearnOpenGL/LOGLBloomTab.hpp
    Tabs/LearnOpenGL/LOGLCoordinateSystemsTab.cpp
    Tabs/LearnOpenGL/LOGLCoordinateSystemsTab.hpp
    Tabs/LearnOpenGL/LOGLCubemapsTab.cpp
    Tabs/LearnOpenGL/LOGLCubemapsTab.hpp
    Tabs/LearnOpenGL/LOGLDeferredShadingTab.cpp
    Tabs/LearnOpenGL/LOGLDeferredShadingTab.hpp
    Tabs/LearnOpenGL/LOGLFramebuffersTab.cpp
    Tabs/LearnOpenGL/LOGLFramebuffersTab.hpp
    Tabs/LearnOpenGL/LOGLGammaTab.cpp
    Tabs/LearnOpenGL/LOGLGammaTab.hpp
    Tabs/LearnOpenGL/LOGLHDRTab.cpp
    Tabs/LearnOpenGL/LOGLHDRTab.hpp
    Tabs/LearnOpenGL/LOGLHelloTriangleTab.cpp
    Tabs/LearnOpenGL/LOGLHelloTriangleTab.hpp
    Tabs/LearnOpenGL/LOGLLightingMapsTab.cpp
    Tabs/LearnOpenGL/LOGLLightingMapsTab.hpp
    Tabs/LearnOpenGL/LOGLMultipleLightsTab.cpp
    Tabs/LearnOpenGL/LOGLMultipleLightsTab.hpp
    Tabs/LearnOpenGL/LOGLNormalMappingTab.cpp
    Tabs/LearnOpenGL/LOGLNormalMappingTab.hpp
    Tabs/LearnOpenGL/LOGLParallaxMappingTab.cpp
    Tabs/LearnOpenGL/LOGLParallaxMappingTab.hpp
    Tabs/LearnOpenGL/LOGLShadowMappingTab.cpp
    Tabs/LearnOpenGL/LOGLShadowMappingTab.hpp
    Tabs/LearnOpenGL/LOGLSSAOTab.cpp
    Tabs/LearnOpenGL/LOGLSSAOTab.hpp
    Tabs/LearnOpenGL/LOGLTexturingTab.cpp
    Tabs/LearnOpenGL/LOGLTexturingTab.hpp

    Tabs/CookiecutterTab.cpp
    Tabs/CookiecutterTab.hpp
    Tabs/ErrorTab.cpp
    Tabs/ErrorTab.hpp
    Tabs/ScreenshotTab.cpp
    Tabs/ScreenshotTab.hpp
    Tabs/Tab.cpp
    Tabs/Tab.hpp
    Tabs/TabHost.cpp
    Tabs/TabHost.hpp
    Tabs/TabRegistry.cpp
    Tabs/TabRegistry.hpp
    Tabs/TabRegistryEntry.cpp
    Tabs/TabRegistryEntry.hpp

    Utils/PropertySystem/AbstractProperty.hpp
    Utils/PropertySystem/AbstractSocket.hpp
    Utils/PropertySystem/Component.cpp
    Utils/PropertySystem/Component.hpp
    Utils/PropertySystem/ComponentIterator.hpp
    Utils/PropertySystem/ComponentList.hpp
    Utils/PropertySystem/ComponentMemberOffset.hpp
    Utils/PropertySystem/ComponentPath.cpp
    Utils/PropertySystem/ComponentPath.hpp
    Utils/PropertySystem/Property.hpp
    Utils/PropertySystem/PropertyDefinition.hpp
    Utils/PropertySystem/PropertyMetadata.hpp
    Utils/PropertySystem/PropertySystemMacros.hpp
    Utils/PropertySystem/PropertyType.hpp
    Utils/PropertySystem/Socket.cpp
    Utils/PropertySystem/Socket.hpp
    Utils/PropertySystem/SocketDefinition.hpp
    Utils/PropertySystem/TypedComponentList.hpp
    Utils/Algorithms.cpp
    Utils/Algorithms.hpp
    Utils/Assertions.cpp
    Utils/Assertions.hpp
    Utils/CircularBuffer.hpp
    Utils/ClonePtr.hpp
    Utils/CopyOnUpdPtr.hpp
    Utils/Cpp20Shims.hpp
    Utils/CStringView.hpp
    Utils/DefaultConstructOnCopy.hpp
    Utils/FileChangePoller.cpp
    Utils/FileChangePoller.hpp
    Utils/FilesystemHelpers.cpp
    Utils/FilesystemHelpers.hpp
    Utils/Macros.hpp
    Utils/MethodTestMacro.hpp
    Utils/Perf.cpp
    Utils/Perf.hpp
    Utils/ScopeGuard.hpp
    Utils/Spsc.hpp
    Utils/SynchronizedValue.hpp
    Utils/UID.cpp
    Utils/UID.hpp
    Utils/UndoRedo.cpp
    Utils/UndoRedo.hpp

    Widgets/GuiRuler.cpp
    Widgets/GuiRuler.hpp
    Widgets/IconWithMenu.cpp
    Widgets/IconWithMenu.hpp
    Widgets/IconWithoutMenu.cpp
    Widgets/IconWithoutMenu.hpp
    Widgets/LogViewer.cpp
    Widgets/LogViewer.hpp
    Widgets/Popup.hpp
    Widgets/PopupManager.cpp
    Widgets/PopupManager.hpp
    Widgets/RedoButton.cpp
    Widgets/RedoButton.hpp
    Widgets/SaveChangesPopupConfig.hpp
    Widgets/SaveChangesPopup.cpp
    Widgets/SaveChangesPopup.hpp
    Widgets/SceneViewer.cpp
    Widgets/SceneViewer.hpp
    Widgets/StandardPopup.cpp
    Widgets/StandardPopup.hpp
    Widgets/UndoButton.cpp
    Widgets/UndoButton.hpp
    Widgets/WindowMenu.cpp
    Widgets/WindowMenu.hpp
)

target_compile_options(oscar PRIVATE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
)

# oscar: compile-time includes
target_include_directories(oscar PUBLIC

    # so that `#include "oscar/OscarStaticConfig.hpp"` works
    ${CMAKE_CURRENT_BINARY_DIR}/generated/

    # so that `#include "oscar/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# oscar: use all the stuff set by `osc_configuration`
target_link_libraries(oscar PUBLIC

    # so that it uses the standard flags etc.
    osc_configuration

    OpenGL::GL
    GLEW::glew_s
    glm::glm
    SDL2::SDL2
    nativefiledialog
    imgui
    IconFontCppHeaders
    ImGuizmo
    implot
    stb
    unordered_dense::unordered_dense
    tomlplusplus::tomlplusplus
    nonstd::span-lite
    lunasvg::lunasvg
)

# oscar: link options
target_link_options(oscar PUBLIC

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
