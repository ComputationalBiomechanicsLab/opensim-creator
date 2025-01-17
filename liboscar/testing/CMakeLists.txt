include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testoscarconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/testing/testoscarconfig.h"
)
file(GLOB_RECURSE OSC_OSCAR_TEST_SOURCES
    LIST_DIRECTORIES FALSE
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp"
)
list(FILTER OSC_OSCAR_TEST_SOURCES EXCLUDE REGEX "VariableLengthArray")  # hacked out: can't compile on MacOS yet

add_executable(testoscar
    "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/testing/testoscarconfig.h"
    ${OSC_OSCAR_TEST_SOURCES}
    TestingHelpers.cpp
    TestingHelpers.h
    TestRenderer.cpp
)
set_target_properties(testoscar PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(testoscar PUBLIC
    cxx_std_20
)
include(${PROJECT_SOURCE_DIR}/cmake/osc_strict_compiler_options.cmake)
target_compile_options(testoscar PRIVATE
    ${OSC_STRICT_COMPILER_OPTIONS}
)
target_include_directories(testoscar PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"  # so that `#include <liboscar/testing/testoscarconfig.h>` works
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
