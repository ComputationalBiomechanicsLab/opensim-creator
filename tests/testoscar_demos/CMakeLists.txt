include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar_demos
    TestAllRegisteredDemoTabs.cpp
    testoscar_demos.cpp  # entry point
)

target_include_directories(testoscar_demos PRIVATE

    # so that the source code can `#include <testoscar_demos/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(testoscar_demos PRIVATE

    oscar_compiler_configuration
    oscar
    oscar_demos

    GTest::gtest
    GTest::gtest_main
)

set_target_properties(testoscar_demos PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
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
