# prefer to choose GLVND for OpenGL
cmake_policy(SET CMP0072 NEW)

# find dependencies (usually built+installed from `third_party/`)
if (NOT ${OSC_EMSCRIPTEN})
    find_package(OpenGL REQUIRED)
    find_package(glew REQUIRED CONFIG)
    find_package(SDL2 REQUIRED CONFIG)
    find_package(nativefiledialog REQUIRED CONFIG)
endif()
find_package(imgui REQUIRED CONFIG)
find_package(IconFontCppHeaders REQUIRED CONFIG)
find_package(ImGuizmo REQUIRED CONFIG)
find_package(ImGuiColorTextEdit REQUIRED CONFIG)
find_package(implot REQUIRED CONFIG)
find_package(stb REQUIRED CONFIG)
find_package(lunasvg REQUIRED CONFIG)
find_package(tomlplusplus REQUIRED CONFIG)
find_package(unordered_dense REQUIRED CONFIG)

add_library(oscar STATIC
    DOM/Class.cpp
    DOM/Class.h
    DOM/Node.h
    DOM/NodePath.cpp
    DOM/NodePath.h
    DOM/Object.cpp
    DOM/PropertyInfo.cpp
    DOM/PropertyInfo.h

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

    Graphics/Geometries/AABBGeometry.cpp
    Graphics/Geometries/AABBGeometry.h
    Graphics/Geometries/BoxGeometry.cpp
    Graphics/Geometries/BoxGeometry.h
    Graphics/Geometries/CircleGeometry.cpp
    Graphics/Geometries/CircleGeometry.h
    Graphics/Geometries/CylinderGeometry.cpp
    Graphics/Geometries/CylinderGeometry.h
    Graphics/Geometries/ConeGeometry.cpp
    Graphics/Geometries/ConeGeometry.h
    Graphics/Geometries/DodecahedronGeometry.cpp
    Graphics/Geometries/DodecahedronGeometry.h
    Graphics/Geometries/GridGeometry.cpp
    Graphics/Geometries/GridGeometry.h
    Graphics/Geometries/IcosahedronGeometry.cpp
    Graphics/Geometries/IcosahedronGeometry.h
    Graphics/Geometries/LatheGeometry.cpp
    Graphics/Geometries/LatheGeometry.h
    Graphics/Geometries/OctahedronGeometry.cpp
    Graphics/Geometries/OctahedronGeometry.h
    Graphics/Geometries/PlaneGeometry.cpp
    Graphics/Geometries/PlaneGeometry.h
    Graphics/Geometries/PolyhedronGeometry.cpp
    Graphics/Geometries/PolyhedronGeometry.h
    Graphics/Geometries/RingGeometry.cpp
    Graphics/Geometries/RingGeometry.h
    Graphics/Geometries/SolidGeometries.h
    Graphics/Geometries/SphereGeometry.cpp
    Graphics/Geometries/SphereGeometry.h
    Graphics/Geometries/TetrahedronGeometry.cpp
    Graphics/Geometries/TetrahedronGeometry.h
    Graphics/Geometries/TorusGeometry.cpp
    Graphics/Geometries/TorusGeometry.h
    Graphics/Geometries/TorusKnotGeometry.cpp
    Graphics/Geometries/TorusKnotGeometry.h
    Graphics/Geometries/WireframeGeometry.cpp
    Graphics/Geometries/WireframeGeometry.h

    Graphics/Materials/MeshBasicMaterial.cpp
    Graphics/Materials/MeshBasicMaterial.h
    Graphics/Materials/MeshPhongMaterial.cpp
    Graphics/Materials/MeshPhongMaterial.h

    Graphics/OpenGL/CPUDataTypeOpenGLTraits.h
    Graphics/OpenGL/CPUImageFormatOpenGLTraits.h
    Graphics/OpenGL/Gl.cpp
    Graphics/OpenGL/Gl.h
    Graphics/OpenGL/TextureFormatOpenGLTraits.h

    Graphics/Scene/CachedSceneRenderer.cpp
    Graphics/Scene/CachedSceneRenderer.h
    Graphics/Scene/SceneCache.cpp
    Graphics/Scene/SceneCache.h
    Graphics/Scene/SceneCollision.h
    Graphics/Scene/SceneDecoration.h
    Graphics/Scene/SceneDecorationFlags.h
    Graphics/Scene/SceneDecorationShading.h
    Graphics/Scene/SceneHelpers.cpp
    Graphics/Scene/SceneHelpers.h
    Graphics/Scene/SceneRenderer.cpp
    Graphics/Scene/SceneRenderer.h
    Graphics/Scene/SceneRendererParams.h

    Graphics/Textures/ChequeredTexture.cpp
    Graphics/Textures/ChequeredTexture.h

    Graphics/AntiAliasingLevel.cpp
    Graphics/AntiAliasingLevel.h
    Graphics/BlendingEquation.h
    Graphics/BlitFlags.h
    Graphics/Camera.h
    Graphics/CameraClearFlags.h
    Graphics/CameraClippingPlanes.h
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
    Graphics/DestinationBlendingFactor.h
    Graphics/Geometries.h
    Graphics/Graphics.h
    Graphics/GraphicsContext.h
    Graphics/GraphicsImplementation.cpp
    Graphics/Material.h
    Graphics/MaterialPropertyBlock.h
    Graphics/Materials.h
    Graphics/MeshIndicesView.h
    Graphics/Mesh.h
    Graphics/MeshFunctions.cpp
    Graphics/MeshFunctions.h
    Graphics/MeshTopology.h
    Graphics/MeshUpdateFlags.h
    Graphics/RenderBufferLoadAction.h
    Graphics/RenderBufferStoreAction.h
    Graphics/RenderBufferType.h
    Graphics/RenderTarget.h
    Graphics/RenderTargetAttachment.h
    Graphics/RenderTargetColorAttachment.h
    Graphics/RenderTargetDepthAttachment.h
    Graphics/RenderTexture.h
    Graphics/RenderTextureFormat.h
    Graphics/RenderTextureParams.h
    Graphics/RenderTextureReadWrite.h
    Graphics/Scene.h
    Graphics/Shader.h
    Graphics/ShaderPropertyType.h
    Graphics/SharedRenderBuffer.h
    Graphics/Snorm.h
    Graphics/Snorm8.h
    Graphics/SourceBlendingFactor.h
    Graphics/SubMeshDescriptor.h
    Graphics/TextureChannelFormat.h
    Graphics/TextureDimensionality.h
    Graphics/TextureFilterMode.h
    Graphics/TextureFormat.h
    Graphics/Textures.h
    Graphics/TextureWrapMode.h
    Graphics/Texture2D.h
    Graphics/Unorm.h
    Graphics/Unorm8.h
    Graphics/VertexAttribute.h
    Graphics/VertexAttributeDescriptor.cpp
    Graphics/VertexAttributeDescriptor.h
    Graphics/VertexAttributeFormat.h
    Graphics/VertexFormat.cpp
    Graphics/VertexFormat.h

    Maths/AABB.h
    Maths/AABBFunctions.h
    Maths/AnalyticPlane.h
    Maths/Angle.h
    Maths/BVH.h
    Maths/BVHCollision.h
    Maths/BVHNode.h
    Maths/BVHPrim.h
    Maths/Circle.h
    Maths/ClosedInterval.h
    Maths/CollisionTests.h
    Maths/CommonFunctions.h
    Maths/Constants.h
    Maths/CoordinateAxis.h
    Maths/CoordinateDirection.h
    Maths/Disc.h
    Maths/EasingFunctions.h
    Maths/Ellipsoid.h
    Maths/EllipsoidFunctions.h
    Maths/EulerPerspectiveCamera.h
    Maths/EulerAngles.h
    Maths/FrustumPlanes.h
    Maths/Functors.h
    Maths/GeometricFunctions.h
    Maths/Line.h
    Maths/LineSegment.h
    Maths/Mat.h
    Maths/MatFunctions.h
    Maths/MathHelpers.h
    Maths/MathsImplementation.cpp
    Maths/Mat3.h
    Maths/Mat4.h
    Maths/Negative.h
    Maths/Normalized.h
    Maths/Plane.h
    Maths/PlaneFunctions.h
    Maths/PointDirection.h
    Maths/PolarPerspectiveCamera.h
    Maths/Qua.h
    Maths/Quat.h
    Maths/QuaternionFunctions.h
    Maths/RayCollision.h
    Maths/Rect.h
    Maths/RectFunctions.h
    Maths/Scalar.h
    Maths/Sphere.h
    Maths/Tetrahedron.h
    Maths/Transform.h
    Maths/TransformFunctions.h
    Maths/Triangle.h
    Maths/TriangleFunctions.h
    Maths/TrigonometricFunctions.h
    Maths/UnitVec.h
    Maths/UnitVec3.h
    Maths/Vec.h
    Maths/VecFunctions.h
    Maths/Vec2.h
    Maths/Vec3.h
    Maths/Vec4.h

    Platform/Detail/SDL2Helpers.h
    Platform/App.cpp
    Platform/App.h
    Platform/AppClock.h
    Platform/AppMainLoopStatus.h
    Platform/AppMetadata.cpp
    Platform/AppMetadata.h
    Platform/AppSettings.cpp
    Platform/AppSettings.h
    Platform/AppSettingScope.h
    Platform/FilesystemResourceLoader.cpp
    Platform/FilesystemResourceLoader.h
    Platform/ILogSink.h
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

    Shims/Cpp20/bit.h
    Shims/Cpp20/stop_token.h
    Shims/Cpp20/thread.h
    Shims/Cpp23/algorithm.h
    Shims/Cpp23/cstddef.h
    Shims/Cpp23/numeric.h
    Shims/Cpp23/ranges.h
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
    UI/Widgets/CameraViewAxes.cpp
    UI/Widgets/CameraViewAxes.h
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
    UI/imgui_impl_sdl2.cpp
    UI/imgui_impl_sdl2.h
    UI/MouseCapturingCamera.h
    UI/oscimgui.cpp
    UI/oscimgui.h
    UI/oscimgui_config.h
    UI/Panels.h
    UI/Screens.h
    UI/Tabs.h
    UI/ui_context.cpp
    UI/ui_context.h
    UI/ui_graphics_backend.cpp
    UI/ui_graphics_backend.h
    UI/Widgets.h

    Utils/Algorithms.h
    Utils/Assertions.cpp
    Utils/Assertions.h
    Utils/ChronoHelpers.h
    Utils/CircularBuffer.h
    Utils/ClonePtr.h
    Utils/Concepts.h
    Utils/Conversion.h
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
    Utils/Flags.h
    Utils/HashHelpers.h
    Utils/LifetimedPtr.h
    Utils/LifetimeWatcher.h
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
    Utils/ScopedLifetime.h
    Utils/ScopeGuard.h
    Utils/SharedLifetimeBlock.h
    Utils/SharedPreHashedString.h
    Utils/Spsc.h
    Utils/StringHelpers.cpp
    Utils/StringHelpers.h
    Utils/StringName.cpp
    Utils/StringName.h
    Utils/StdVariantHelpers.h
    Utils/SynchronizedValue.h
    Utils/SynchronizedValueGuard.h
    Utils/TemporaryFile.cpp
    Utils/TemporaryFile.h
    Utils/TemporaryFileParameters.h
    Utils/TransparentStringHasher.h
    Utils/Typelist.h
    Utils/UID.cpp
    Utils/UID.h
    Utils/UndoRedo.cpp
    Utils/UndoRedo.h
    Utils/WatchableLifetime.h

    Variant/Variant.cpp
    Variant/Variant.h
    Variant/VariantType.cpp
    Variant/VariantType.h
    Variant/VariantTypeList.h
    Variant/VariantTypeTraits.h

    DOM.h
    Formats.h
    Graphics.h
    Maths.h
    oscar.h
    Platform.h
    Shims.h
    Strings.h
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

    $<$<NOT:$<BOOL:${OSC_EMSCRIPTEN}>>:OpenGL::GL>
    $<$<NOT:$<BOOL:${OSC_EMSCRIPTEN}>>:GLEW::glew_s>
    $<$<NOT:$<BOOL:${OSC_EMSCRIPTEN}>>:SDL2::SDL2>
    $<$<NOT:$<BOOL:${OSC_EMSCRIPTEN}>>:nativefiledialog>
    imgui
    IconFontCppHeaders
    ImGuizmo
    ImGuiColorTextEdit
    implot
    stb
    unordered_dense::unordered_dense
    tomlplusplus::tomlplusplus
    lunasvg::lunasvg
)

set_target_properties(oscar PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)
