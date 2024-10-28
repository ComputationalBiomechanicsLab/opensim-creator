include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

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

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestOpenSimCreatorConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/TestOpenSimCreator/TestOpenSimCreatorConfig.h"
)
target_include_directories(TestOpenSimCreator PRIVATE
    # so that source code can `#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)

target_link_libraries(TestOpenSimCreator PRIVATE

    # set compile options
    oscar_compiler_configuration

    # link to the to-be-tested library
    OpenSimCreator

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# additional compile options (on top of `oscar_compiler_configuration`)
target_compile_options(TestOpenSimCreator PRIVATE

    # gcc/clang
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        # disable extra-semicolon detection: broken by OpenSim_DECLARE_ macros (see: opensim-core/3496)
        -Wno-extra-semi
    >

    # gcc
    $<$<CXX_COMPILER_ID:GNU>:
        # gcc-12 finds (possibly legit) bounds issues in simbody - requires investigation
        -Wno-array-bounds
    >
)

set_target_properties(TestOpenSimCreator PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
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
