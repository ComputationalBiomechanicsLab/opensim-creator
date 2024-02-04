include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar_demos
    TestAllRegisteredDemoTabs.cpp
    testoscar_demos.cpp  # entry point
)

target_include_directories(testoscar_demos PUBLIC

    # so that the source code can `#include <testoscar_demos/SomeModule.hpp>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(testoscar_demos PUBLIC

    # set compiler options
    oscar_compiler_configuration

    # library access
    oscar

    # the to-be-tested library
    oscar_demos

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
gtest_discover_tests(testoscar_demos)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar_demos
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:testoscar_demos> $<TARGET_FILE_DIR:testoscar_demos>
        COMMAND_EXPAND_LISTS
    )
endif()
