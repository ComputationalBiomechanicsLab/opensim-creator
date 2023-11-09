include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar
    Formats/TestCSV.cpp
    Formats/TestDAE.cpp

    Graphics/TestAntiAliasingLevel.cpp
    Graphics/TestColor.cpp
    Graphics/TestCubemap.cpp
    Graphics/TestCubemapFace.cpp
    Graphics/TestCullMode.cpp
    Graphics/TestGraphicsHelpers.cpp
    Graphics/TestRenderer.cpp
    Graphics/TestRenderTarget.cpp
    Graphics/TestRenderTargetColorAttachment.cpp
    Graphics/TestRenderTargetDepthAttachment.cpp
    Graphics/TestRenderTextureFormat.cpp
    Graphics/TestTextureChannelFormat.cpp
    Graphics/TestTextureFormat.cpp

    Maths/TestBVH.cpp

    Platform/TestAppSettingValueType.cpp

    Shims/Cpp20/TestBit.cpp

    Utils/TestCStringView.cpp
    Utils/TestFileChangePoller.cpp
    Utils/TestFilenameExtractor.cpp
    Utils/TestStringHelpers.cpp

    testoscar.cpp  # entry point
 )


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testoscarconfig.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/testoscar/testoscarconfig.hpp"
)

target_include_directories(testoscar PUBLIC

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
