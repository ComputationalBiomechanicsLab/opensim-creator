set(OSCAR_EXTRA_COMPILER_FLAGS "" CACHE STRING "Extra compiler flags for oscar targets")

include(GoogleTest)  # gtest_discover_tests

option(OSCAR_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

configure_file(osc.toml.in ${CMAKE_CURRENT_BINARY_DIR}/osc.toml)  # generate runtime configuration for tests

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testoscarconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/tests/testoscarconfig.h"
)

file(GLOB_RECURSE _oscar_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
file(GLOB_RECURSE _oscar_test_extras "*.cpp")
add_executable(testoscar ${_oscar_test_sources} ${_oscar_test_extras})
unset(_oscar_test_sources)

set_target_properties(testoscar PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testoscar PUBLIC cxx_std_23)
target_compile_options(testoscar PRIVATE ${OSCAR_EXTRA_COMPILER_FLAGS})
target_include_directories(testoscar PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"  # so that `#include <liboscar/tests/testoscarconfig.h>` works
)
target_link_libraries(testoscar PRIVATE
    oscar
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
if(OSCAR_DISCOVER_TESTS)
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
