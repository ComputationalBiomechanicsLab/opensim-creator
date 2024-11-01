include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testoscarconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/testoscar/testoscarconfig.h"
)
add_executable(testoscar
    DOM/TestClass.cpp
    DOM/TestNode.cpp
    DOM/TestNodePath.cpp
    DOM/TestObject.cpp
    DOM/TestPropertyInfo.cpp

    Formats/TestCSV.cpp
    Formats/TestDAE.cpp
    Formats/TestImage.cpp

    Graphics/Detail/TestVertexAttributeFormatHelpers.cpp
    Graphics/Detail/TestVertexAttributeHelpers.cpp
    Graphics/Detail/TestVertexAttributeFormatList.cpp
    Graphics/Detail/TestVertexAttributeList.cpp
    Graphics/Scene/TestSceneCache.cpp
    Graphics/Scene/TestSceneHelpers.cpp
    Graphics/TestAntiAliasingLevel.cpp
    Graphics/TestCamera.cpp
    Graphics/TestCameraProjection.cpp
    Graphics/TestColor.cpp
    Graphics/TestColorComponent.cpp
    Graphics/TestColorRenderBufferFormat.cpp
    Graphics/TestColorRenderBufferParams.cpp
    Graphics/TestCubemap.cpp
    Graphics/TestCubemapFace.cpp
    Graphics/TestCullMode.cpp
    Graphics/TestDepthStencilRenderBufferFormat.cpp
    Graphics/TestDepthStencilRenderBufferParams.cpp
    Graphics/TestGeometries.cpp
    Graphics/TestMaterialPropertyBlock.cpp
    Graphics/TestSubMeshDescriptor.cpp
    Graphics/TestMesh.cpp
    Graphics/TestMeshIndicesView.cpp
    Graphics/TestRenderer.cpp
    Graphics/TestRenderTarget.cpp
    Graphics/TestRenderTargetColorAttachment.cpp
    Graphics/TestRenderTargetDepthStencilAttachment.cpp
    Graphics/TestRenderTexture.cpp
    Graphics/TestRenderTextureDescriptor.cpp
    Graphics/TestRgba.cpp
    Graphics/TestShaderPropertyType.cpp
    Graphics/TestSharedColorRenderBuffer.cpp
    Graphics/TestSharedDepthStencilRenderBuffer.cpp
    Graphics/TestSnorm8.cpp
    Graphics/TestTextureComponentFormat.cpp
    Graphics/TestTextureFormat.cpp
    Graphics/TestTexture2D.cpp
    Graphics/TestUnorm8.cpp
    Graphics/TestVertexAttribute.cpp
    Graphics/TestVertexFormat.cpp

    Maths/TestAnalyticPlane.cpp
    Maths/TestAngle.cpp
    Maths/TestBVH.cpp
    Maths/TestClosedInterval.cpp
    Maths/TestCommonFunctions.cpp
    Maths/TestCoordinateAxis.cpp
    Maths/TestCoordinateDirection.cpp
    Maths/TestFrustumPlanes.cpp
    Maths/TestNormalized.cpp
    Maths/TestPlaneFunctions.cpp
    Maths/TestTransform.cpp
    Maths/TestUnitVec.cpp
    Maths/TestVec2.cpp
    Maths/TestVec3.cpp
    Maths/TestVec4.cpp

    MetaTests/TestFormatsHeader.cpp
    MetaTests/TestGraphicsHeader.cpp
    MetaTests/TestMathsHeader.cpp
    MetaTests/TestObjectHeader.cpp
    MetaTests/TestOscarHeader.cpp
    MetaTests/TestPlatformHeader.cpp
    MetaTests/TestSceneHeader.cpp
    MetaTests/TestShimsHeader.cpp
    MetaTests/TestUIHeader.cpp
    MetaTests/TestUtilsHeader.cpp
    MetaTests/TestVariantHeader.cpp

    Platform/TestResourceDirectoryEntry.cpp
    Platform/TestResourceLoader.cpp
    Platform/TestResourcePath.cpp
    Platform/TestResourceStream.cpp

    Shims/Cpp20/TestBit.cpp
    Shims/Cpp23/TestCstddef.cpp

    Utils/TestAlgorithms.cpp
    Utils/TestChronoHelpers.cpp
    Utils/TestConversion.cpp
    Utils/TestCStringView.cpp
    Utils/TestEnumHelpers.cpp
    Utils/TestExceptionHelpers.cpp
    Utils/TestFileChangePoller.cpp
    Utils/TestFilenameExtractor.cpp
    Utils/TestHashHelpers.cpp
    Utils/TestFlags.cpp
    Utils/TestLifetimedPtr.cpp
    Utils/TestLifetimeWatcher.cpp
    Utils/TestNonTypelist.cpp
    Utils/TestNullOStream.cpp
    Utils/TestNullStreambuf.cpp
    Utils/TestScopedLifetime.cpp
    Utils/TestSharedLifetimeBlock.cpp
    Utils/TestSharedPreHashedString.cpp
    Utils/TestScopedLifetime.cpp
    Utils/TestStringHelpers.cpp
    Utils/TestStringName.cpp
    Utils/TestTemporaryFile.cpp
    Utils/TestTransparentStringHasher.cpp
    Utils/TestTypelist.cpp
    # TODO: re-enable once MacOS gets std::pmr support
    # Utils/TestVariableLengthArray.cpp

    Variant/TestVariant.cpp
    Variant/TestVariantType.cpp

    TestingHelpers.cpp
    TestingHelpers.h
    testoscar.cpp  # entry point
)
set_target_properties(testoscar PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(testoscar PUBLIC
    cxx_std_20
)
target_include_directories(testoscar PRIVATE

    # so that the source code can `#include <testoscar/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..

    # so that source code can `#include <tesetoscar/testoscarconfig.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)

target_link_libraries(testoscar PRIVATE
    oscar
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
if(${OSC_DISCOVER_TESTS})
    gtest_discover_tests(testoscar)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:testoscar> $<TARGET_RUNTIME_DLLS:testoscar>
        COMMAND_EXPAND_LISTS
    )
endif()
