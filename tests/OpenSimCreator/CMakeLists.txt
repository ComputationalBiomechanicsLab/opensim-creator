enable_testing()

find_package(GTest REQUIRED CONFIG)

# testopensimcreator_config.hpp
#
#     configuration file that contains configuration-time vals (e.g. fixture locations)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/testopensimcreator_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/testopensimcreator_config.hpp"
)

add_executable(testopensimcreator EXCLUDE_FROM_ALL

    Graphics/TestOpenSimDecorationGenerator.cpp

    TestForwardDynamicSimulation.cpp
    TestOpenSim.cpp
    TestOpenSimActions.cpp
    TestOpenSimHelpers.cpp
    TestTypeRegistry.cpp
    TestUndoableModelStatePair.cpp

    testopensimcreator.cpp  # entrypoint (main)
)

target_link_libraries(testopensimcreator PUBLIC

    # set compile options
    oscar_configuration

    # link to the to-be-tested library
    OpenSimCreator

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(testopensimcreator PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/generated  # for testopensimcreator_config.hpp
)

# tell CMake (+IDEs) how to find all tests
include(GoogleTest)
gtest_add_tests(TARGET testopensimcreator)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    # HACK: we can't use TARGET_RUNTIME_DLLS because simbody doesn't package libgfortran etc. correctly
    file(GLOB_RECURSE dlls "${CMAKE_PREFIX_PATH}/*.dll")
    add_custom_command(
        TARGET testopensimcreator
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dlls} $<TARGET_FILE_DIR:testopensimcreator>
        COMMAND_EXPAND_LISTS
    )
endif()
