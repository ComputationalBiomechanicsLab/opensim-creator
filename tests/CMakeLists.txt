enable_testing()

find_package(GTest REQUIRED CONFIG)

# osc_test_config.hpp:
#
#     configuration header that's used by the test suite for (e.g.) locating
#     test fixtures in the source directory
configure_file("osc_test_config.hpp.in" "osc_test_config.hpp")

# osctests: main exe that links to `osccore` and exercises APIs etc.
add_executable(testosc EXCLUDE_FROM_ALL

    oscar/Formats/TestCSV.cpp
    oscar/Formats/TestDAE.cpp

    oscar/Graphics/TestColor.cpp
    oscar/Graphics/TestCubemap.cpp
    oscar/Graphics/TestCubemapFace.cpp
    oscar/Graphics/TestGraphicsHelpers.cpp
    oscar/Graphics/TestImage.cpp
    oscar/Graphics/TestRenderer.cpp
    oscar/Graphics/TestRenderTarget.cpp
    oscar/Graphics/TestRenderTargetColorAttachment.cpp
    oscar/Graphics/TestRenderTargetDepthAttachment.cpp
    oscar/Graphics/TestRenderTextureFormat.cpp
    oscar/Graphics/TestTextureFormat.cpp

    oscar/Maths/TestBVH.cpp

    OpenSimCreator/Graphics/TestOpenSimDecorationGenerator.cpp
    OpenSimCreator/TestForwardDynamicSimulation.cpp
    OpenSimCreator/TestOpenSim.cpp
    OpenSimCreator/TestOpenSimActions.cpp
    OpenSimCreator/TestOpenSimHelpers.cpp
    OpenSimCreator/TestTypeRegistry.cpp
    OpenSimCreator/TestUndoableModelStatePair.cpp

    oscar/Utils/PropertySystem/TestComponent.cpp
    oscar/Utils/PropertySystem/TestPropertySystemMacros.cpp
    oscar/Utils/TestFileChangePoller.cpp
    oscar/Utils/TestAlgorithms.cpp

    testosc.cpp  # entrypoint (main)
)

# osctests: target properties
set_target_properties(testosc PROPERTIES

    # disable compiler-specific language extensions
    CXX_EXTENSIONS OFF

    # strictly require the requested C++ standard (e.g. C++17)
    CXX_STANDARD_REQUIRED YES

    # export compile commands, so clang-tidy works correctly
    EXPORT_COMPILE_COMMANDS ON
)

# osctests: link to `osccore` to exercise it
target_link_libraries(testosc PUBLIC

    # set compile options
    osc_configuration

    # link to the to-be-tested library
    OpenSimCreator

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(testosc PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}  # for osc_test_config.hpp
)

# osctests: tell CMake (+IDEs) to discover each test case
#
# this is how IDEs like Visual Studio integrate their test parser with
# the test framework
include(GoogleTest)
gtest_add_tests(TARGET testosc)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    # HACK: we can't use TARGET_RUNTIME_DLLS because simbody doesn't package libgfortran etc. correctly
    file(GLOB_RECURSE dlls "${CMAKE_PREFIX_PATH}/*.dll")
    add_custom_command(
        TARGET testosc
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dlls} $<TARGET_FILE_DIR:testosc>
        COMMAND_EXPAND_LISTS
    )
endif()