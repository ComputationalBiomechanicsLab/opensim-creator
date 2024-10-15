include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(TestOpenSimThirdPartyPlugins

    opensim-jam-org/jam-plugin/TestSmith2018ArticularContactForce.cpp
    opensim-jam-org/jam-plugin/TestSmith2018ContactMesh.cpp

    TestOpenSimThirdPartyPlugins.cpp  # entrypoint (main)
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestOpenSimThirdPartyPluginsConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/TestOpenSimThirdPartyPlugins/TestOpenSimThirdPartyPluginsConfig.h"
)
target_include_directories(TestOpenSimThirdPartyPlugins PRIVATE
    # so that source code can `#include <TestOpenSimThirdPartyPlugins/TestOpenSimThirdPartyPluginsConfig.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)
target_link_libraries(TestOpenSimThirdPartyPlugins PRIVATE

    # set compile options
    oscar_compiler_configuration

    # link to the to-be-tested library
    OpenSimThirdPartyPlugins

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)
# additional compile options (on top of `oscar_compiler_configuration`)
target_compile_options(TestOpenSimThirdPartyPlugins PRIVATE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

        # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4996
    >

    # gcc/clang
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        # disable extra-semicolon detection: broken by OpenSim_DECLARE_ macros (see: opensim-core/3496)
        -Wno-extra-semi
    >

    # gcc
    $<$<CXX_COMPILER_ID:GNU>:
        # gcc-12 finds (possibly legit) bounds issues in simbody - requires investigation
        -Wno-array-bounds
    >
)
set_target_properties(TestOpenSimThirdPartyPlugins PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)

# tell CMake (+IDEs) how to find all tests
if(${OSC_DISCOVER_TESTS})
    gtest_discover_tests(TestOpenSimThirdPartyPlugins)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET TestOpenSimThirdPartyPlugins
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:TestOpenSimThirdPartyPlugins> $<TARGET_RUNTIME_DLLS:TestOpenSimThirdPartyPlugins>
        COMMAND_EXPAND_LISTS
    )
endif()
