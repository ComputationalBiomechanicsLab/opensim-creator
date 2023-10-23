include(GoogleTest)
find_package(GTest REQUIRED CONFIG)

add_executable(testoscar_document
    TestNodePath.cpp
    TestPropertyDescription.cpp
    TestPropertyTableEntry.cpp
    TestStringName.cpp
    TestVariant.cpp
    TestVariantType.cpp

    testoscar_document.cpp  # entrypoint
)

target_link_libraries(testoscar_document PUBLIC
    # set compiler options
    oscar_compiler_configuration

    # link to the to-be-tested library
    oscar_document

    # link to testing library
    GTest::gtest
    GTest::gtest_main
)

# tell CMake (+IDEs) how to find all tests
gtest_discover_tests(testoscar_document)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET testoscar_document
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:testoscar_document> $<TARGET_FILE_DIR:testoscar_document>
        COMMAND_EXPAND_LISTS
    )
endif()
