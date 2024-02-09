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
find_package(tomlplusplus REQUIRED CONFIG)
find_package(unordered_dense REQUIRED CONFIG)

add_library(oscar STATIC
    Formats/CSV.h
    Formats/CSV.cpp
    Formats/DAE.h
    Formats/DAE.cpp
    Formats/Image.cpp
    Formats/Image.h
    Formats/ImageLoadingFlags.h
    Formats/OBJ.cpp
    Formats/OBJ.h
    Formats/STL.cpp
    Formats/STL.h
    Formats/SVG.h
    Formats/SVG.cpp

    Graphics/Detail/CPUDataType.h
    Graphics/Detail/CPUImageFormat.h
    Graphics/Detail/ShaderLocations.h
    Graphics/Detail/ShaderPropertyTypeList.h
    Graphics/Detail/ShaderPropertyTypeTraits.h
    Graphics/Detail/TextureFormatList.h
    Graphics/Detail/TextureFormatTraits.h
    Graphics/Detail/VertexAttributeFormatHelpers.h
    Graphics/Detail/VertexAttributeFormatList.h
    Graphics/Detail/VertexAttributeFormatTraits.h
    Graphics/Detail/VertexAttributeHelpers.h
    Graphics/Detail/VertexAttributeList.h
    Graphics/Detail/VertexAttributeTraits.h
    Graphics/OpenGL/CPUDataTypeOpenGLTraits.h
    Graphics/OpenGL/CPUImageFormatOpenGLTraits.h
    Graphics/OpenGL/Gl.cpp
    Graphics/OpenGL/Gl.h
    Graphics/OpenGL/TextureFormatOpenGLTraits.h
    Graphics/AntiAliasingLevel.cpp
    Graphics/AntiAliasingLevel.h
    Graphics/BlitFlags.h
    Graphics/Camera.h
    Graphics/CameraClearFlags.h
    Graphics/CameraProjection.h
    Graphics/Color.cpp
    Graphics/Color.h
    Graphics/ColorHSLA.h
    Graphics/ColorHSLA.cpp
    Graphics/ColorSpace.h
    Graphics/Color32.h
    Graphics/Cubemap.h
    Graphics/CubemapFace.h
    Graphics/DepthStencilFormat.h
    Graphics/Graphics.h
    Graphics/GraphicsContext.h
    Graphics/GraphicsHelpers.cpp
    Graphics/GraphicsHelpers.h
    Graphics/GraphicsImplementation.cpp
    Graphics/Material.h
    Graphics/MaterialPropertyBlock.h
    Graphics/MeshGenerators.cpp
    Graphics/MeshGenerators.h
    Graphics/MeshIndicesView.h
    Graphics/Mesh.h
    Graphics/MeshTopology.h
    Graphics/MeshUpdateFlags.h
    Graphics/RenderBuffer.h
    Graphics/RenderBufferLoadAction.h
    Graphics/RenderBufferStoreAction.h
    Graphics/RenderBufferType.h
    Graphics/RenderTarget.cpp
    Graphics/RenderTarget.h
    Graphics/RenderTargetAttachment.cpp
    Graphics/RenderTargetAttachment.h
    Graphics/RenderTargetColorAttachment.cpp
    Graphics/RenderTargetColorAttachment.h
    Graphics/RenderTargetDepthAttachment.h
    Graphics/RenderTexture.h
    Graphics/RenderTextureDescriptor.h
    Graphics/RenderTextureFormat.h
    Graphics/RenderTextureReadWrite.h
    Graphics/Shader.h
    Graphics/ShaderPropertyType.h
    Graphics/SubMeshDescriptor.h
    Graphics/TextureChannelFormat.h
    Graphics/TextureDimensionality.h
    Graphics/TextureFilterMode.h
    Graphics/TextureFormat.h
    Graphics/TextureGenerators.cpp
    Graphics/TextureGenerators.h
    Graphics/TextureWrapMode.h
    Graphics/Texture2D.h
    Graphics/Unorm8.h
    Graphics/VertexAttribute.h
    Graphics/VertexAttributeDescriptor.cpp
    Graphics/VertexAttributeDescriptor.h
    Graphics/VertexAttributeFormat.h
    Graphics/VertexFormat.cpp
    Graphics/VertexFormat.h

    Maths/AABB.h
    Maths/Angle.h
    Maths/BVH.h
    Maths/BVHCollision.h
    Maths/BVHNode.h
    Maths/BVHPrim.h
    Maths/Circle.h
    Maths/CollisionTests.h
    Maths/Disc.h
    Maths/EasingFunctions.h
    Maths/Ellipsoid.h
    Maths/EulerPerspectiveCamera.h
    Maths/Eulers.h
    Maths/LengthType.h
    Maths/Line.h
    Maths/Mat.h
    Maths/MathHelpers.h
    Maths/MathsImplementation.cpp
    Maths/Mat3.h
    Maths/Mat4.h
    Maths/Mat4x3.h
    Maths/Plane.h
    Maths/PointDirection.h
    Maths/PolarPerspectiveCamera.h
    Maths/Qua.h
    Maths/Qualifier.h
    Maths/Quat.h
    Maths/RayCollision.h
    Maths/Rect.h
    Maths/Segment.h
    Maths/Sphere.h
    Maths/Tetrahedron.h
    Maths/Transform.h
    Maths/Triangle.h
    Maths/UnitVec.h
    Maths/UnitVec3.h
    Maths/Vec.h
    Maths/Vec2.h
    Maths/Vec3.h
    Maths/Vec4.h

    Object/Class.cpp
    Object/Class.h
    Object/Node.h
    Object/NodePath.cpp
    Object/NodePath.h
    Object/Object.cpp
    Object/Object.h
    Object/PropertyInfo.cpp
    Object/PropertyInfo.h

    Platform/Detail/SDL2Helpers.h
    Platform/App.cpp
    Platform/App.h
    Platform/AppClock.h
    Platform/AppConfig.cpp
    Platform/AppConfig.h
    Platform/AppMetadata.cpp
    Platform/AppMetadata.h
    Platform/AppSettings.cpp
    Platform/AppSettings.h
    Platform/AppSettingValue.cpp
    Platform/AppSettingValue.h
    Platform/AppSettingValueType.h
    Platform/FilesystemResourceLoader.cpp
    Platform/FilesystemResourceLoader.h
    Platform/IResourceLoader.cpp
    Platform/IResourceLoader.h
    Platform/IScreen.h
    Platform/Log.cpp
    Platform/Log.h
    Platform/Logger.h
    Platform/LogLevel.cpp
    Platform/LogLevel.h
    Platform/LogMessage.h
    Platform/LogMessageView.h
    Platform/LogSink.h
    Platform/os.cpp
    Platform/os.h
    Platform/ResourceLoader.h
    Platform/ResourcePath.h
    Platform/ResourceStream.cpp
    Platform/ResourceStream.h
    Platform/Screenshot.h
    Platform/ScreenshotAnnotation.h

    Scene/CachedSceneRenderer.cpp
    Scene/CachedSceneRenderer.h
    Scene/SceneCache.cpp
    Scene/SceneCache.h
    Scene/SceneCollision.h
    Scene/SceneDecoration.h
    Scene/SceneDecorationFlags.h
    Scene/SceneHelpers.cpp
    Scene/SceneHelpers.h
    Scene/SceneRenderer.cpp
    Scene/SceneRenderer.h
    Scene/SceneRendererParams.cpp
    Scene/SceneRendererParams.h
    Scene/ShaderCache.cpp
    Scene/ShaderCache.h

    Shims/Cpp20/bit.h
    Shims/Cpp20/stop_token.h
    Shims/Cpp20/thread.h
    Shims/Cpp23/utility.h

    UI/Panels/IPanel.h
    UI/Panels/LogViewerPanel.cpp
    UI/Panels/LogViewerPanel.h
    UI/Panels/PanelManager.cpp
    UI/Panels/PanelManager.h
    UI/Panels/PerfPanel.cpp
    UI/Panels/PerfPanel.h
    UI/Panels/StandardPanelImpl.cpp
    UI/Panels/StandardPanelImpl.h
    UI/Panels/ToggleablePanelFlags.h
    UI/Panels/UndoRedoPanel.cpp
    UI/Panels/UndoRedoPanel.h
    UI/Screens/TabTestingScreen.cpp
    UI/Screens/TabTestingScreen.h
    UI/Tabs/CookiecutterTab.cpp
    UI/Tabs/CookiecutterTab.h
    UI/Tabs/ErrorTab.cpp
    UI/Tabs/ErrorTab.h
    UI/Tabs/ITab.h
    UI/Tabs/ITabHost.h
    UI/Tabs/ScreenshotTab.cpp
    UI/Tabs/ScreenshotTab.h
    UI/Tabs/StandardTabImpl.h
    UI/Tabs/TabRegistry.cpp
    UI/Tabs/TabRegistry.h
    UI/Tabs/TabRegistryEntry.cpp
    UI/Tabs/TabRegistryEntry.h
    UI/Widgets/GuiRuler.cpp
    UI/Widgets/GuiRuler.h
    UI/Widgets/IconWithMenu.cpp
    UI/Widgets/IconWithMenu.h
    UI/Widgets/IconWithoutMenu.cpp
    UI/Widgets/IconWithoutMenu.h
    UI/Widgets/LogViewer.cpp
    UI/Widgets/LogViewer.h
    UI/Widgets/IPopup.h
    UI/Widgets/PopupManager.cpp
    UI/Widgets/PopupManager.h
    UI/Widgets/RedoButton.cpp
    UI/Widgets/RedoButton.h
    UI/Widgets/SaveChangesPopupConfig.h
    UI/Widgets/SaveChangesPopup.cpp
    UI/Widgets/SaveChangesPopup.h
    UI/Widgets/SceneViewer.cpp
    UI/Widgets/SceneViewer.h
    UI/Widgets/StandardPopup.cpp
    UI/Widgets/StandardPopup.h
    UI/Widgets/UndoButton.cpp
    UI/Widgets/UndoButton.h
    UI/Widgets/WindowMenu.cpp
    UI/Widgets/WindowMenu.h
    UI/Icon.h
    UI/IconCache.cpp
    UI/IconCache.h
    UI/ImGuiHelpers.cpp
    UI/ImGuiHelpers.h
    UI/ImGuizmoHelpers.cpp
    UI/ImGuizmoHelpers.h
    UI/Panels.h
    UI/Screens.h
    UI/Tabs.h
    UI/ui_context.cpp
    UI/ui_context.h
    UI/ui_graphics_backend.cpp
    UI/ui_graphics_backend.h
    UI/Widgets.h

    Utils/At.h
    Utils/Assertions.cpp
    Utils/Assertions.h
    Utils/CircularBuffer.h
    Utils/ClonePtr.h
    Utils/Concepts.h
    Utils/CopyOnUpdPtr.h
    Utils/CStringView.cpp
    Utils/CStringView.h
    Utils/DefaultConstructOnCopy.h
    Utils/EnumHelpers.h
    Utils/FileChangePoller.cpp
    Utils/FileChangePoller.h
    Utils/FilenameExtractor.h
    Utils/FilesystemHelpers.cpp
    Utils/FilesystemHelpers.h
    Utils/HashHelpers.h
    Utils/NonTypelist.h
    Utils/NullOStream.h
    Utils/NullStreambuf.h
    Utils/ObjectRepresentation.h
    Utils/ParalellizationHelpers.h
    Utils/ParentPtr.h
    Utils/Perf.cpp
    Utils/Perf.h
    Utils/PerfClock.h
    Utils/PerfMeasurement.h
    Utils/PerfMeasurementMetadata.h
    Utils/ScopeGuard.h
    Utils/SetHelpers.h
    Utils/Spsc.h
    Utils/StringHelpers.cpp
    Utils/StringHelpers.h
    Utils/StringName.cpp
    Utils/StringName.h
    Utils/StdVariantHelpers.h
    Utils/SynchronizedValue.h
    Utils/SynchronizedValueGuard.h
    Utils/Typelist.h
    Utils/UID.cpp
    Utils/UID.h
    Utils/UndoRedo.cpp
    Utils/UndoRedo.h

    Variant/Variant.cpp
    Variant/Variant.h
    Variant/VariantType.cpp
    Variant/VariantType.h
    Variant/VariantTypeList.h
    Variant/VariantTypeTraits.h

    Formats.h
    Graphics.h
    Maths.h
    oscar.h
    Object.h
    Platform.h
    Scene.h
    Shims.h
    UI.h
    Utils.h
    Variant.h
)

target_include_directories(oscar PUBLIC

    # so that `#include <oscar/HEADER.h>` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar PUBLIC

    oscar_compiler_configuration

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
    lunasvg::lunasvg
)
