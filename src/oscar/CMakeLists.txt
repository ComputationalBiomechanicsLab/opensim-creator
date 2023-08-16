# prefer to choose GLVND for OpenGL
cmake_policy(SET CMP0072 NEW)

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

# [lib]oscar:
#
#     core platform library for window creation, rendering, maths, etc.
add_library(oscar STATIC

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
    Graphics/ImageLoadingFlags.hpp
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
    Graphics/RenderBufferType.hpp
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
    Graphics/ShaderLocations.hpp
    Graphics/ShaderType.hpp
    Graphics/SimpleSceneDecoration.hpp
    Graphics/Texture2D.hpp
    Graphics/TextureDimension.hpp
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
    Tabs/LearnOpenGL/LOGLPointShadowsTab.cpp
    Tabs/LearnOpenGL/LOGLPointShadowsTab.hpp
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
    Tabs/StandardTabBase.hpp
    Tabs/Tab.hpp
    Tabs/TabHost.cpp
    Tabs/TabHost.hpp
    Tabs/TabRegistry.cpp
    Tabs/TabRegistry.hpp
    Tabs/TabRegistryEntry.cpp
    Tabs/TabRegistryEntry.hpp

    Utils/ArrayHelpers.hpp
    Utils/Assertions.cpp
    Utils/Assertions.hpp
    Utils/CircularBuffer.hpp
    Utils/ClonePtr.hpp
    Utils/CopyOnUpdPtr.hpp
    Utils/Cpp20Shims.hpp
    Utils/CStringView.hpp
    Utils/DefaultConstructOnCopy.hpp
    Utils/EnumHelpers.hpp
    Utils/FileChangePoller.cpp
    Utils/FileChangePoller.hpp
    Utils/FilesystemHelpers.cpp
    Utils/FilesystemHelpers.hpp
    Utils/HashHelpers.hpp
    Utils/Macros.hpp
    Utils/ParalellizationHelpers.hpp
    Utils/ParentPtr.hpp
    Utils/Perf.cpp
    Utils/Perf.hpp
    Utils/ScopeGuard.hpp
    Utils/SetHelpers.hpp
    Utils/Spsc.hpp
    Utils/StringHelpers.cpp
    Utils/StringHelpers.hpp
    Utils/SynchronizedValue.hpp
    Utils/TypeHelpers.hpp
    Utils/UID.cpp
    Utils/UID.hpp
    Utils/UndoRedo.cpp
    Utils/UndoRedo.hpp
    Utils/VariantHelpers.hpp

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

target_include_directories(oscar PUBLIC

    # so that `#include "oscar/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar PUBLIC

    oscar_configuration

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
