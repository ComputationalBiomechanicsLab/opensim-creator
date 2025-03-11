set(OSC_DISCOVER_TESTS ON
    CACHE BOOL
    "enable/disable automatically running test discovery (IDE integration)"
)

include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

file(GLOB_RECURSE OSC_OSCAR_DEMO_TEST_SOURCES
    LIST_DIRECTORIES FALSE
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp"
)
add_executable(testoscar_demos
    ${OSC_OSCAR_DEMO_TEST_SOURCES}
)
set_target_properties(testoscar_demos PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(testoscar_demos PUBLIC
    cxx_std_20
)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/osc_strict_compiler_options.cmake)
target_compile_options(testoscar_demos PRIVATE
    ${OSC_STRICT_COMPILER_OPTIONS}
)
target_include_directories(testoscar_demos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # so that `#include <testoscar_demos/HEADER.h>` works
)
target_link_libraries(testoscar_demos PRIVATE
    oscar
    oscar_demos
    GTest::gtest
    GTest::gtest_main
)

if(${OSC_DISCOVER_TESTS})
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
