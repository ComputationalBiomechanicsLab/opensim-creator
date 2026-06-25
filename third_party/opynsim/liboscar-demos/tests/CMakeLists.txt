include(GoogleTest)  # gtest_discover_tests

option(OPYN_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

# generate runtime configuration for tests
configure_file(
    osc.toml.in
    ${CMAKE_CURRENT_BINARY_DIR}/osc.toml
    @ONLY
)

file(GLOB_RECURSE _oscar_demos_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
file(GLOB_RECURSE _oscar_demos_test_extras "*.cpp")
add_executable(oscar_demos_tests ${_oscar_demos_test_sources} ${_oscar_demos_test_extras})
unset(_oscar_demos_test_sources)
unset(_oscar_demos_test_extras)

target_compile_features(oscar_demos_tests PUBLIC cxx_std_23)
set_target_properties(oscar_demos_tests PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/opynsim_strict_compiler_options.cmake)
    opyn_add_strict_compiler_options_to(oscar_demos_tests)
endif()
target_link_libraries(oscar_demos_tests PRIVATE
    oscar
    oscar_demos
    GTest::gtest
)

if(OPYN_DISCOVER_TESTS)
    gtest_discover_tests(oscar_demos_tests)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET oscar_demos_tests
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:oscar_demos_tests> $<TARGET_RUNTIME_DLLS:oscar_demos_tests>
        COMMAND_EXPAND_LISTS
    )
endif()
