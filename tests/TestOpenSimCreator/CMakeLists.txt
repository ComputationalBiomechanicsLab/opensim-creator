include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(TestOpenSimCreator

    ComponentRegistry/TestStaticComponentRegistries.cpp
    Documents/FrameDefinition/TestStationDefinedFrame.cpp
    Documents/Frames/TestFrameAxis.cpp
    Documents/Frames/TestFramesHelpers.cpp
    Documents/Landmarks/TestLandmarkHelpers.cpp
    Documents/Model/TestUndoableModelActions.cpp
    Documents/Model/TestUndoableModelStatePair.cpp
    Documents/ModelWarper/TestDocument.cpp
    Documents/ModelWarper/TestFrameDefinitionLookup.cpp
    Documents/ModelWarper/TestMeshWarpPairingLookup.cpp
    Documents/Simulation/TestForwardDynamicSimulation.cpp
    Graphics/TestOpenSimDecorationGenerator.cpp
    MetaTests/TestOpenSimLibraryAPI.cpp
    Platform/TestRecentFiles.cpp
    UI/Widgets/TestAddComponentPopup.cpp
    Utils/TestOpenSimHelpers.cpp
    Utils/TestShapeFitters.cpp

    TestOpenSimCreator.cpp  # entrypoint (main)
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestOpenSimCreatorConfig.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/TestOpenSimCreator/TestOpenSimCreatorConfig.hpp"
)
target_include_directories(TestOpenSimCreator PUBLIC
    # so that source code can `#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)

target_link_libraries(TestOpenSimCreator PUBLIC

    # set compile options
    oscar_compiler_configuration

    # link to the to-be-tested library
    OpenSimCreator

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
gtest_discover_tests(TestOpenSimCreator)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET TestOpenSimCreator
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:TestOpenSimCreator> $<TARGET_FILE_DIR:TestOpenSimCreator>
        COMMAND_EXPAND_LISTS
    )
endif()
