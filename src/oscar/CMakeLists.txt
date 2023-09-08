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
    Graphics/AntiAliasingLevel.cpp
    Graphics/AntiAliasingLevel.hpp
    Graphics/CachedSceneRenderer.cpp
    Graphics/CachedSceneRenderer.hpp
    Graphics/BlitFlags.hpp
    Graphics/Camera.hpp
    Graphics/CameraClearFlags.hpp
    Graphics/CameraProjection.hpp
    Graphics/Color.cpp
    Graphics/Color.hpp
    Graphics/Color32.hpp
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
    Graphics/TextureChannelFormat.hpp
    Graphics/TextureDimensionality.hpp
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
    Platform/Logger.hpp
    Platform/LogLevel.cpp
    Platform/LogLevel.hpp
    Platform/LogMessage.hpp
    Platform/LogMessageView.hpp
    Platform/LogSink.hpp
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

    Tabs/LearnOpenGL/AdvancedLighting/LOGLBloomTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLBloomTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLDeferredShadingTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLDeferredShadingTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLGammaTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLGammaTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLHDRTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLHDRTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLNormalMappingTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLNormalMappingTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLParallaxMappingTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLParallaxMappingTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLPointShadowsTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLPointShadowsTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLShadowMappingTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLShadowMappingTab.hpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLSSAOTab.cpp
    Tabs/LearnOpenGL/AdvancedLighting/LOGLSSAOTab.hpp

    Tabs/LearnOpenGL/AdvancedOpenGL/LOGLBlendingTab.cpp
    Tabs/LearnOpenGL/AdvancedOpenGL/LOGLBlendingTab.hpp
    Tabs/LearnOpenGL/AdvancedOpenGL/LOGLCubemapsTab.cpp
    Tabs/LearnOpenGL/AdvancedOpenGL/LOGLCubemapsTab.hpp
    Tabs/LearnOpenGL/AdvancedOpenGL/LOGLFramebuffersTab.cpp
    Tabs/LearnOpenGL/AdvancedOpenGL/LOGLFramebuffersTab.hpp

    Tabs/LearnOpenGL/GettingStarted/LOGLCoordinateSystemsTab.cpp
    Tabs/LearnOpenGL/GettingStarted/LOGLCoordinateSystemsTab.hpp
    Tabs/LearnOpenGL/GettingStarted/LOGLHelloTriangleTab.cpp
    Tabs/LearnOpenGL/GettingStarted/LOGLHelloTriangleTab.hpp
    Tabs/LearnOpenGL/GettingStarted/LOGLTexturingTab.cpp
    Tabs/LearnOpenGL/GettingStarted/LOGLTexturingTab.hpp

    Tabs/LearnOpenGL/Lighting/LOGLBasicLightingTab.cpp
    Tabs/LearnOpenGL/Lighting/LOGLBasicLightingTab.hpp
    Tabs/LearnOpenGL/Lighting/LOGLLightingMapsTab.cpp
    Tabs/LearnOpenGL/Lighting/LOGLLightingMapsTab.hpp
    Tabs/LearnOpenGL/Lighting/LOGLMultipleLightsTab.cpp
    Tabs/LearnOpenGL/Lighting/LOGLMultipleLightsTab.hpp

    Tabs/LearnOpenGL/PBR/LOGLPBRDiffuseIrradianceTab.cpp
    Tabs/LearnOpenGL/PBR/LOGLPBRDiffuseIrradianceTab.hpp
    Tabs/LearnOpenGL/PBR/LOGLPBRLightingTab.cpp
    Tabs/LearnOpenGL/PBR/LOGLPBRLightingTab.hpp
    Tabs/LearnOpenGL/PBR/LOGLPBRLightingTexturedTab.cpp
    Tabs/LearnOpenGL/PBR/LOGLPBRLightingTexturedTab.hpp
    Tabs/LearnOpenGL/PBR/LOGLPBRSpecularIrradianceTab.cpp
    Tabs/LearnOpenGL/PBR/LOGLPBRSpecularIrradianceTab.hpp
    Tabs/LearnOpenGL/PBR/LOGLPBRSpecularIrradianceTexturedTab.cpp
    Tabs/LearnOpenGL/PBR/LOGLPBRSpecularIrradianceTexturedTab.hpp

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
    Utils/PerfClock.hpp
    Utils/PerfMeasurement.hpp
    Utils/PerfMeasurementMetadata.hpp
    Utils/ScopeGuard.hpp
    Utils/SetHelpers.hpp
    Utils/SpanHelpers.hpp
    Utils/Spsc.hpp
    Utils/StringHelpers.cpp
    Utils/StringHelpers.hpp
    Utils/SynchronizedValue.hpp
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
