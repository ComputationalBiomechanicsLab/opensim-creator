include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar_simbody
    TestShapeFitters.cpp
    TestSimTKDecorationGenerator.cpp
    testoscar_simbody.cpp  # entry point
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testoscar_simbody_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/testoscar_simbody/testoscar_simbody_config.h"
)

target_include_directories(testoscar_simbody PRIVATE

    # so that source code can `#include <testoscar_simbody/testoscar_simbody_config.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"

    # so that the source code can `#include <testoscar_simbody/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(testoscar_simbody PRIVATE

    oscar_compiler_configuration
    oscar_simbody

    GTest::gtest
    GTest::gtest_main
)

set_target_properties(testoscar_simbody PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)

if(${OSC_DISCOVER_TESTS})
    gtest_discover_tests(testoscar_simbody)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar_simbody
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:testoscar_simbody> $<TARGET_RUNTIME_DLLS:testoscar_simbody>
        COMMAND_EXPAND_LISTS
    )
endif()
