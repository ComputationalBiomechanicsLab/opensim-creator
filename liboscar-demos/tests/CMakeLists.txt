set(OSCAR_EXTRA_COMPILER_FLAGS "" CACHE STRING "Extra compiler flags for oscar targets")

include(GoogleTest)  # gtest_discover_tests

option(OSCAR_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

configure_file(osc.toml.in ${CMAKE_CURRENT_BINARY_DIR}/osc.toml)  # generate runtime configuration for tests

file(GLOB_RECURSE OSCAR_DEMO_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
add_executable(testoscar_demos ${OSCAR_DEMO_TEST_SOURCES})
set_target_properties(testoscar_demos PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testoscar_demos PUBLIC cxx_std_23)
target_compile_options(testoscar_demos PRIVATE ${OSCAR_EXTRA_COMPILER_FLAGS})
target_include_directories(testoscar_demos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # so that `#include <testoscar_demos/HEADER.h>` works
)
target_link_libraries(testoscar_demos PRIVATE
    oscar
    oscar_demos
    GTest::gtest
    GTest::gtest_main
)

if(OSCAR_DISCOVER_TESTS)
    gtest_discover_tests(testoscar_demos)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar_demos
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:testoscar_demos> $<TARGET_RUNTIME_DLLS:testoscar_demos>
        COMMAND_EXPAND_LISTS
    )
endif()
