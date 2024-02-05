include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar
    Formats/TestCSV.cpp
    Formats/TestDAE.cpp

    Graphics/Detail/TestVertexAttributeFormatHelpers.cpp
    Graphics/Detail/TestVertexAttributeHelpers.cpp
    Graphics/Detail/TestVertexAttributeFormatList.cpp
    Graphics/Detail/TestVertexAttributeList.cpp
    Graphics/TestAntiAliasingLevel.cpp
    Graphics/TestCamera.cpp
    Graphics/TestCameraProjection.cpp
    Graphics/TestColor.cpp
    Graphics/TestCubemap.cpp
    Graphics/TestCubemapFace.cpp
    Graphics/TestCullMode.cpp
    Graphics/TestGraphicsHelpers.cpp
    Graphics/TestSubMeshDescriptor.cpp
    Graphics/TestMesh.cpp
    Graphics/TestMeshUpdateFlags.cpp
    Graphics/TestRenderer.cpp
    Graphics/TestRenderTarget.cpp
    Graphics/TestRenderTargetColorAttachment.cpp
    Graphics/TestRenderTargetDepthAttachment.cpp
    Graphics/TestRenderTexture.cpp
    Graphics/TestRenderTextureDescriptor.cpp
    Graphics/TestRenderTextureFormat.cpp
    Graphics/TestTextureChannelFormat.cpp
    Graphics/TestTextureFormat.cpp
    Graphics/TestTexture2D.cpp
    Graphics/TestUnorm8.cpp
    Graphics/TestVertexAttribute.cpp
    Graphics/TestVertexFormat.cpp

    Maths/TestAngle.cpp
    Maths/TestBVH.cpp
    Maths/TestUnitVec.cpp

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

    Object/TestClass.cpp
    Object/TestNode.cpp
    Object/TestNodePath.cpp
    Object/TestObject.cpp
    Object/TestPropertyInfo.cpp

    Platform/TestAppSettingValueType.cpp

    Scene/TestSceneCache.cpp

    Shims/Cpp20/TestBit.cpp

    Utils/TestCStringView.cpp
    Utils/TestEnumHelpers.cpp
    Utils/TestFileChangePoller.cpp
    Utils/TestFilenameExtractor.cpp
    Utils/TestNonTypelist.cpp
    Utils/TestNullOStream.cpp
    Utils/TestNullStreambuf.cpp
    Utils/TestStringHelpers.cpp
    Utils/TestStringName.cpp
    Utils/TestTypelist.cpp

    Variant/TestVariant.cpp
    Variant/TestVariantType.cpp

    TestingHelpers.cpp
    TestingHelpers.hpp
    testoscar.cpp  # entry point
)


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testoscarconfig.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/testoscar/testoscarconfig.hpp"
)

target_include_directories(testoscar PUBLIC

    # so that the source code can `#include <testoscar/SomeModule.hpp>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..

    # so that source code can `#include <tesetoscar/testoscarconfig.hpp>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)

target_link_libraries(testoscar PUBLIC

    # set compiler options
    oscar_compiler_configuration

    # link to the to-be-tested library
    oscar

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
gtest_discover_tests(testoscar)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:testoscar> $<TARGET_FILE_DIR:testoscar>
        COMMAND_EXPAND_LISTS
    )
endif()
