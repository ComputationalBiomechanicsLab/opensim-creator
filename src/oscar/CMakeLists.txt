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

    Graphics/Detail/ShaderLocations.hpp
    Graphics/Detail/VertexAttributeFormatHelpers.hpp
    Graphics/Detail/VertexAttributeFormatList.hpp
    Graphics/Detail/VertexAttributeFormatTraits.hpp
    Graphics/Detail/VertexAttributeHelpers.hpp
    Graphics/Detail/VertexAttributeList.hpp
    Graphics/Detail/VertexAttributeTraits.hpp
    Graphics/OpenGL/Gl.cpp
    Graphics/OpenGL/Gl.hpp
    Graphics/AntiAliasingLevel.cpp
    Graphics/AntiAliasingLevel.hpp
    Graphics/BlitFlags.hpp
    Graphics/Camera.hpp
    Graphics/CameraClearFlags.hpp
    Graphics/CameraProjection.hpp
    Graphics/Color.cpp
    Graphics/Color.hpp
    Graphics/ColorHSLA.hpp
    Graphics/ColorHSLA.cpp
    Graphics/ColorSpace.hpp
    Graphics/Color32.hpp
    Graphics/Cubemap.hpp
    Graphics/CubemapFace.hpp
    Graphics/DepthStencilFormat.hpp
    Graphics/Graphics.hpp
    Graphics/GraphicsContext.hpp
    Graphics/GraphicsHelpers.cpp
    Graphics/GraphicsHelpers.hpp
    Graphics/GraphicsImplementation.cpp
    Graphics/ImageLoadingFlags.hpp
    Graphics/Material.hpp
    Graphics/MaterialPropertyBlock.hpp
    Graphics/MeshGenerators.cpp
    Graphics/MeshGenerators.hpp
    Graphics/MeshIndicesView.hpp
    Graphics/Mesh.hpp
    Graphics/MeshTopology.hpp
    Graphics/MeshUpdateFlags.hpp
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
    Graphics/RenderTargetDepthAttachment.hpp
    Graphics/RenderTexture.hpp
    Graphics/RenderTextureDescriptor.hpp
    Graphics/RenderTextureFormat.hpp
    Graphics/RenderTextureReadWrite.hpp
    Graphics/Shader.hpp
    Graphics/ShaderCache.cpp
    Graphics/ShaderCache.hpp
    Graphics/ShaderPropertyType.hpp
    Graphics/SubMeshDescriptor.hpp
    Graphics/TextureChannelFormat.hpp
    Graphics/TextureDimensionality.hpp
    Graphics/TextureFilterMode.hpp
    Graphics/TextureFormat.hpp
    Graphics/TextureGenerators.cpp
    Graphics/TextureGenerators.hpp
    Graphics/TextureWrapMode.hpp
    Graphics/Texture2D.hpp
    Graphics/Unorm8.hpp
    Graphics/VertexAttribute.hpp
    Graphics/VertexAttributeDescriptor.cpp
    Graphics/VertexAttributeDescriptor.hpp
    Graphics/VertexAttributeFormat.hpp
    Graphics/VertexFormat.cpp
    Graphics/VertexFormat.hpp

    Maths/AABB.hpp
    Maths/Angle.hpp
    Maths/BVH.hpp
    Maths/BVHCollision.hpp
    Maths/BVHNode.hpp
    Maths/BVHPrim.hpp
    Maths/Circle.hpp
    Maths/CollisionTests.hpp
    Maths/Disc.hpp
    Maths/EasingFunctions.hpp
    Maths/Ellipsoid.hpp
    Maths/EulerPerspectiveCamera.hpp
    Maths/Eulers.hpp
    Maths/LengthType.hpp
    Maths/Line.hpp
    Maths/Mat.hpp
    Maths/MathHelpers.hpp
    Maths/MathsImplementation.cpp
    Maths/Mat3.hpp
    Maths/Mat4.hpp
    Maths/Mat4x3.hpp
    Maths/Plane.hpp
    Maths/PointDirection.hpp
    Maths/PolarPerspectiveCamera.hpp
    Maths/Qua.hpp
    Maths/Qualifier.hpp
    Maths/Quat.hpp
    Maths/RayCollision.hpp
    Maths/Rect.hpp
    Maths/Segment.hpp
    Maths/Sphere.hpp
    Maths/Tetrahedron.hpp
    Maths/Transform.hpp
    Maths/Triangle.hpp
    Maths/Vec.hpp
    Maths/Vec2.hpp
    Maths/Vec3.hpp
    Maths/Vec4.hpp

    Object/Class.cpp
    Object/Class.hpp
    Object/Node.hpp
    Object/NodePath.cpp
    Object/NodePath.hpp
    Object/Object.cpp
    Object/Object.hpp
    Object/PropertyInfo.cpp
    Object/PropertyInfo.hpp

    Platform/Detail/SDL2Helpers.hpp
    Platform/App.cpp
    Platform/App.hpp
    Platform/AppClock.hpp
    Platform/AppConfig.cpp
    Platform/AppConfig.hpp
    Platform/AppMetadata.cpp
    Platform/AppMetadata.hpp
    Platform/AppSettings.cpp
    Platform/AppSettings.hpp
    Platform/AppSettingValue.cpp
    Platform/AppSettingValue.hpp
    Platform/AppSettingValueType.hpp
    Platform/IScreen.hpp
    Platform/Log.cpp
    Platform/Log.hpp
    Platform/Logger.hpp
    Platform/LogLevel.cpp
    Platform/LogLevel.hpp
    Platform/LogMessage.hpp
    Platform/LogMessageView.hpp
    Platform/LogSink.hpp
    Platform/os.cpp
    Platform/os.hpp
    Platform/Screenshot.hpp
    Platform/ScreenshotAnnotation.hpp

    Scene/CachedSceneRenderer.cpp
    Scene/CachedSceneRenderer.hpp
    Scene/SceneCache.cpp
    Scene/SceneCache.hpp
    Scene/SceneCollision.hpp
    Scene/SceneDecoration.hpp
    Scene/SceneDecorationFlags.hpp
    Scene/SceneHelpers.cpp
    Scene/SceneHelpers.hpp
    Scene/SceneRenderer.cpp
    Scene/SceneRenderer.hpp
    Scene/SceneRendererParams.cpp
    Scene/SceneRendererParams.hpp

    Shims/Cpp20/bit.hpp
    Shims/Cpp20/stop_token.hpp
    Shims/Cpp20/thread.hpp
    Shims/Cpp23/utility.hpp

    UI/Panels/IPanel.hpp
    UI/Panels/LogViewerPanel.cpp
    UI/Panels/LogViewerPanel.hpp
    UI/Panels/PanelManager.cpp
    UI/Panels/PanelManager.hpp
    UI/Panels/PerfPanel.cpp
    UI/Panels/PerfPanel.hpp
    UI/Panels/StandardPanelImpl.cpp
    UI/Panels/StandardPanelImpl.hpp
    UI/Panels/ToggleablePanelFlags.hpp
    UI/Panels/UndoRedoPanel.cpp
    UI/Panels/UndoRedoPanel.hpp
    UI/Tabs/CookiecutterTab.cpp
    UI/Tabs/CookiecutterTab.hpp
    UI/Tabs/ErrorTab.cpp
    UI/Tabs/ErrorTab.hpp
    UI/Tabs/ITab.hpp
    UI/Tabs/ITabHost.hpp
    UI/Tabs/ScreenshotTab.cpp
    UI/Tabs/ScreenshotTab.hpp
    UI/Tabs/StandardTabImpl.hpp
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
    UI/Widgets/IPopup.hpp
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
    UI/Icon.hpp
    UI/IconCache.cpp
    UI/IconCache.hpp
    UI/ImGuiHelpers.cpp
    UI/ImGuiHelpers.hpp
    UI/ImGuizmoHelpers.cpp
    UI/ImGuizmoHelpers.hpp
    UI/imgui_impl_oscargfx.cpp
    UI/imgui_impl_oscargfx.hpp

    Utils/At.hpp
    Utils/Assertions.cpp
    Utils/Assertions.hpp
    Utils/CircularBuffer.hpp
    Utils/ClonePtr.hpp
    Utils/Concepts.hpp
    Utils/CopyOnUpdPtr.hpp
    Utils/CStringView.cpp
    Utils/CStringView.hpp
    Utils/DefaultConstructOnCopy.hpp
    Utils/EnumHelpers.hpp
    Utils/FileChangePoller.cpp
    Utils/FileChangePoller.hpp
    Utils/FilenameExtractor.hpp
    Utils/FilesystemHelpers.cpp
    Utils/FilesystemHelpers.hpp
    Utils/HashHelpers.hpp
    Utils/NonTypelist.hpp
    Utils/NullOStream.hpp
    Utils/NullStreambuf.hpp
    Utils/ObjectRepresentation.hpp
    Utils/ParalellizationHelpers.hpp
    Utils/ParentPtr.hpp
    Utils/Perf.cpp
    Utils/Perf.hpp
    Utils/PerfClock.hpp
    Utils/PerfMeasurement.hpp
    Utils/PerfMeasurementMetadata.hpp
    Utils/ScopeGuard.hpp
    Utils/SetHelpers.hpp
    Utils/Spsc.hpp
    Utils/StringHelpers.cpp
    Utils/StringHelpers.hpp
    Utils/StringName.cpp
    Utils/StringName.hpp
    Utils/StdVariantHelpers.hpp
    Utils/SynchronizedValue.hpp
    Utils/SynchronizedValueGuard.hpp
    Utils/Typelist.hpp
    Utils/UID.cpp
    Utils/UID.hpp
    Utils/UndoRedo.cpp
    Utils/UndoRedo.hpp

    Variant/Variant.cpp
    Variant/Variant.hpp
    Variant/VariantType.cpp
    Variant/VariantType.hpp
)

target_include_directories(oscar PUBLIC

    # so that `#include <oscar/HEADER.hpp>` works
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
