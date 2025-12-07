include(../cmake/osc_strict_compiler_options.cmake)  # OSC_STRICT_COMPILER_OPTIONS
include(GoogleTest)  # gtest_discover_tests

option(OSC_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestOpenSimCreatorConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libopensimcreator/testing/TestOpenSimCreatorConfig.h"
)
file(GLOB_RECURSE OSC_OPENSIMCREATOR_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
add_executable(TestOpenSimCreator
    ${OSC_OPENSIMCREATOR_TEST_SOURCES}
    TestDocumentationModels.cpp
    TestOpenSimLibraryAPI.cpp
    TestOpenSimCreator.cpp  # main
)
set_target_properties(TestOpenSimCreator PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(TestOpenSimCreator PUBLIC cxx_std_23)
target_compile_options(TestOpenSimCreator PRIVATE ${OSC_STRICT_COMPILER_OPTIONS})
target_include_directories(TestOpenSimCreator PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"  # so that `#include <libopensimcreator/testing/TestOpenSimCreatorConfig.h>` works
)
target_link_libraries(TestOpenSimCreator PRIVATE
    OpenSimCreator
    GTest::gtest
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
