include(GoogleTest)  # gtest_discover_tests

option(OSCAR_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(GTest REQUIRED CONFIG)

# generate runtime configuration for tests
configure_file(
    osc.toml.in
    ${CMAKE_CURRENT_BINARY_DIR}/osc.toml
    @ONLY
)

file(GLOB_RECURSE _oscar_demos_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp")
add_executable(oscar_demos_tests ${_oscar_demos_test_sources})
unset(_oscar_demos_test_sources)

set_target_properties(oscar_demos_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(oscar_demos_tests PUBLIC cxx_std_23)
if(OSCAR_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/oscar_strict_compiler_options.cmake)
    target_compile_options(oscar_demos_tests PRIVATE ${OSCAR_STRICT_COMPILER_OPTIONS})
endif()
target_link_libraries(oscar_demos_tests PRIVATE
    oscar
    oscar_demos
    GTest::gtest
    GTest::gtest_main
)

if(OSCAR_DISCOVER_TESTS)
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
