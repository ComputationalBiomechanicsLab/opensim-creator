include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestOpenSimCreatorConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/TestOpenSimCreator/TestOpenSimCreatorConfig.h"
)
add_executable(TestOpenSimCreator

    ComponentRegistry/TestStaticComponentRegistries.cpp
    docs/TestDocumentationModels.cpp
    Documents/CustomComponents/TestInMemoryMesh.cpp
    Documents/Landmarks/TestLandmarkHelpers.cpp
    Documents/Model/TestBasicModelStatePair.cpp
    Documents/Model/TestUndoableModelActions.cpp
    Documents/Model/TestUndoableModelStatePair.cpp
    Documents/ModelWarper/TestCachedModelWarper.cpp
    Documents/ModelWarper/TestFrameWarperFactories.cpp
    Documents/ModelWarper/TestModelWarperConfiguration.cpp
    Documents/ModelWarper/TestPointWarperFactories.cpp
    Documents/ModelWarper/TestWarpableModel.cpp
    Documents/OutputExtractors/TestConstantOutputExtractor.cpp
    Documents/Simulation/TestForwardDynamicSimulation.cpp
    Documents/Simulation/TestSimulationHelpers.cpp
    Graphics/TestMuscleColorSourceScaling.cpp
    Graphics/TestOpenSimDecorationGenerator.cpp
    Graphics/TestSimTKDecorationGenerator.cpp
    Graphics/TestOpenSimDecorationOptions.cpp
    MetaTests/TestOpenSimLibraryAPI.cpp
    Platform/TestRecentFiles.cpp
    UI/Shared/TestFunctionCurveViewerPopup.cpp
    UI/Widgets/TestAddComponentPopup.cpp
    UI/TestAllRegisteredOpenSimCreatorTabs.cpp
    UI/TestLoadingTab.cpp
    Utils/TestOpenSimHelpers.cpp
    Utils/TestShapeFitters.cpp

    TestOpenSimCreator.cpp  # entrypoint (main)
)
set_target_properties(TestOpenSimCreator PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(TestOpenSimCreator PUBLIC
    cxx_std_20
)
target_include_directories(TestOpenSimCreator PRIVATE
    # so that source code can `#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)
target_link_libraries(TestOpenSimCreator PRIVATE
    OpenSimCreator
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
if(${OSC_DISCOVER_TESTS})
    gtest_discover_tests(TestOpenSimCreator)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET TestOpenSimCreator
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:TestOpenSimCreator> $<TARGET_RUNTIME_DLLS:TestOpenSimCreator>
        COMMAND_EXPAND_LISTS
    )
endif()
