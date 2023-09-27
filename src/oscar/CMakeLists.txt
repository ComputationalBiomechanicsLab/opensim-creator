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

    Platform/App.cpp
    Platform/App.hpp
    Platform/AppClock.hpp
    Platform/AppConfig.cpp
    Platform/AppConfig.hpp
    Platform/AppMetadata.cpp
    Platform/AppMetadata.hpp
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

    UI/Panels/LogViewerPanel.cpp
    UI/Panels/LogViewerPanel.hpp
    UI/Panels/Panel.hpp
    UI/Panels/PanelManager.cpp
    UI/Panels/PanelManager.hpp
    UI/Panels/PerfPanel.cpp
    UI/Panels/PerfPanel.hpp
    UI/Panels/StandardPanel.cpp
    UI/Panels/StandardPanel.hpp
    UI/Panels/ToggleablePanelFlags.hpp
    UI/Panels/UndoRedoPanel.cpp
    UI/Panels/UndoRedoPanel.hpp
    UI/Screens/CookiecutterScreen.cpp
    UI/Screens/CookiecutterScreen.hpp
    UI/Screens/Screen.hpp
    UI/Tabs/CookiecutterTab.cpp
    UI/Tabs/CookiecutterTab.hpp
    UI/Tabs/ErrorTab.cpp
    UI/Tabs/ErrorTab.hpp
    UI/Tabs/ScreenshotTab.cpp
    UI/Tabs/ScreenshotTab.hpp
    UI/Tabs/StandardTabBase.hpp
    UI/Tabs/Tab.hpp
    UI/Tabs/TabHost.cpp
    UI/Tabs/TabHost.hpp
    UI/Tabs/TabRegistry.cpp
    UI/Tabs/TabRegistry.hpp
    UI/Tabs/TabRegistryEntry.cpp
    UI/Tabs/TabRegistryEntry.hpp
    UI/Widgets/GuiRuler.cpp
    UI/Widgets/GuiRuler.hpp
    UI/Widgets/IconWithMenu.cpp
    UI/Widgets/IconWithMenu.hpp
    UI/Widgets/IconWithoutMenu.cpp
    UI/Widgets/IconWithoutMenu.hpp
    UI/Widgets/LogViewer.cpp
    UI/Widgets/LogViewer.hpp
    UI/Widgets/Popup.hpp
    UI/Widgets/PopupManager.cpp
    UI/Widgets/PopupManager.hpp
    UI/Widgets/RedoButton.cpp
    UI/Widgets/RedoButton.hpp
    UI/Widgets/SaveChangesPopupConfig.hpp
    UI/Widgets/SaveChangesPopup.cpp
    UI/Widgets/SaveChangesPopup.hpp
    UI/Widgets/SceneViewer.cpp
    UI/Widgets/SceneViewer.hpp
    UI/Widgets/StandardPopup.cpp
    UI/Widgets/StandardPopup.hpp
    UI/Widgets/UndoButton.cpp
    UI/Widgets/UndoButton.hpp
    UI/Widgets/WindowMenu.cpp
    UI/Widgets/WindowMenu.hpp

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
