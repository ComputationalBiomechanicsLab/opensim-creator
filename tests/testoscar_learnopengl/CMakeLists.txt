include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar_learnopengl
    TestAllRegisteredLearnOpenGLTabs.cpp
    testoscar_learnopengl.cpp  # entry point
)

target_include_directories(testoscar_learnopengl PUBLIC

    # so that the source code can `#include <testoscar_learnopengl/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(testoscar_learnopengl PUBLIC

    # set compiler options
    oscar_compiler_configuration

    # library access
    oscar

    # the to-be-tested library
    oscar_learnopengl

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
gtest_discover_tests(testoscar_learnopengl)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar_learnopengl
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:testoscar_learnopengl> $<TARGET_FILE_DIR:testoscar_learnopengl>
        COMMAND_EXPAND_LISTS
    )
endif()
