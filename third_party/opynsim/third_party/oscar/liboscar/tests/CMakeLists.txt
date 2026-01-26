set(OSCAR_EXTRA_COMPILER_FLAGS "" CACHE STRING "Extra compiler flags for oscar targets")

include(GoogleTest)  # gtest_discover_tests

option(OSCAR_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

configure_file(osc.toml.in ${CMAKE_CURRENT_BINARY_DIR}/osc.toml)  # generate runtime configuration for tests

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/oscar_tests_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/tests/oscar_tests_config.h"
)

file(GLOB_RECURSE _oscar_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
file(GLOB_RECURSE _oscar_test_extras "*.cpp")
add_executable(oscar_tests ${_oscar_test_sources} ${_oscar_test_extras})
unset(_oscar_test_sources)

set_target_properties(oscar_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(oscar_tests PUBLIC cxx_std_23)
target_compile_options(oscar_tests PRIVATE ${OSCAR_EXTRA_COMPILER_FLAGS})
target_include_directories(oscar_tests PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"  # so that `#include <liboscar/tests/oscar_tests_config.h>` works
)
target_link_libraries(oscar_tests PRIVATE
    oscar
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
if(OSCAR_DISCOVER_TESTS)
    gtest_discover_tests(oscar_tests)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET oscar_tests
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:oscar_tests> $<TARGET_RUNTIME_DLLS:oscar_tests>
        COMMAND_EXPAND_LISTS
    )
endif()
