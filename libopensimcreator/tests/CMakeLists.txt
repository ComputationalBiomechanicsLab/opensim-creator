include(GoogleTest)  # gtest_discover_tests

option(OSC_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testopensimcreatorconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libopensimcreator/tests/testopensimcreatorconfig.h"
    @ONLY
)

# Configure an `osc.toml` configuration file specifically for the test suite
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/osc.toml"
    @ONLY
)

file(GLOB_RECURSE _osc_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
add_executable(testopensimcreator
    ${_osc_test_sources}
    DocumentationModels.tests.cpp
    OpenSimLibraryAPI.tests.cpp
    testopensimcreator.cpp  # main
)
unset(_osc_test_sources)

set_target_properties(testopensimcreator PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testopensimcreator PUBLIC cxx_std_23)
if(OSC_USE_STRICT_COMPILER_FLAGS)
    include(../cmake/osc_strict_compiler_options.cmake)
    target_compile_options(testopensimcreator PRIVATE ${OSC_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(testopensimcreator
    # so that `#include <libopensimcreator/tests/testopensimcreatorconfig.h>` works
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)
target_link_libraries(testopensimcreator PRIVATE opensimcreator GTest::gtest)

# tell CMake (+IDEs) how to find all tests
if(OSC_DISCOVER_TESTS)
    gtest_discover_tests(testopensimcreator)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testopensimcreator
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:testopensimcreator> $<TARGET_RUNTIME_DLLS:testopensimcreator>
        COMMAND_EXPAND_LISTS
    )
endif()
