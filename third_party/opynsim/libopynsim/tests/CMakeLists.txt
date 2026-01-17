include(GoogleTest)  # gtest_discover_tests

option(OPYN_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testopynsimconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libopynsim/tests/testopynsimconfig.h"
    @ONLY
)

# Configure an `osc.toml` configuration file specifically for the test suite
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/osc.toml"
    @ONLY
)

file(GLOB_RECURSE _opyn_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
file(GLOB_RECURSE _opyn_extra_sources "*.cpp")
add_executable(testopynsim ${_opyn_test_sources} ${_opyn_extra_sources})
unset(_opyn_test_sources)
unset(_opyn_extra_sources)

set_target_properties(testopynsim PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testopynsim PUBLIC cxx_std_20)
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    include(../cmake/opynsim_strict_compiler_options.cmake)
    target_compile_options(testopynsim PRIVATE ${OPYN_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(testopynsim
    # so that `#include <libopynsim/tests/testopynsimconfig.h>` works
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)
target_link_libraries(testopynsim PRIVATE opynsim GTest::gtest)

# tell CMake (+IDEs) how to find all tests
if(OSC_DISCOVER_TESTS)
    gtest_discover_tests(testopynsim)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testopynsim
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:testopynsim> $<TARGET_RUNTIME_DLLS:testopynsim>
        COMMAND_EXPAND_LISTS
    )
endif()
