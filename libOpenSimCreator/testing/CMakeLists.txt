include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestOpenSimCreatorConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libOpenSimCreator/testing/TestOpenSimCreatorConfig.h"
)
file(GLOB_RECURSE OSC_OPENSIMCREATOR_TEST_SOURCES
    LIST_DIRECTORIES FALSE
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.tests.cpp"
)
add_executable(TestOpenSimCreator

    "${CMAKE_CURRENT_BINARY_DIR}/generated/libOpenSimCreator/testing/TestOpenSimCreatorConfig.h"
    ${OSC_OPENSIMCREATOR_TEST_SOURCES}
    TestDocumentationModels.cpp
    TestOpenSimLibraryAPI.cpp
)
set_target_properties(TestOpenSimCreator PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(TestOpenSimCreator PUBLIC
    cxx_std_20
)
target_include_directories(TestOpenSimCreator PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"  # so that `#include <libOpenSimCreator/testing/TestOpenSimCreatorConfig.h>` works
)
target_link_libraries(TestOpenSimCreator PRIVATE
    OpenSimCreator
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
if(${OSC_DISCOVER_TESTS})
    gtest_discover_tests(TestOpenSimCreator)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET TestOpenSimCreator
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:TestOpenSimCreator> $<TARGET_RUNTIME_DLLS:TestOpenSimCreator>
        COMMAND_EXPAND_LISTS
    )
endif()
