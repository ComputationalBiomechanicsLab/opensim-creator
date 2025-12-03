option(OPYN_DISCOVER_TESTS "Automatically run test discovery (IDE integration)" ON)

find_package(Catch2 REQUIRED CONFIG)

# Glob for all non-test `.cpp` files in this `testing/` directory that may be
# used as utilities by the tests
file(GLOB_RECURSE OPYN_TEST_HELPER_SOURCES *.cpp)
list(FILTER OPYN_TEST_HELPER_SOURCES EXCLUDE REGEX "c2tests.cpp$")

# Glob for all Catch2 test files, which are named `*.c2tests.cpp` in opynsim
file(GLOB_RECURSE OPYN_CATCH2TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../*.c2tests.cpp")

# Declare `testopynsim` test runner executable
add_executable(testopynsim ${OPYN_CATCH2TEST_SOURCES} ${OPYN_TEST_HELPER_SOURCES})
set_target_properties(testopynsim PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testopynsim PUBLIC cxx_std_23)
target_link_libraries(testopynsim PRIVATE opynsim Catch2::Catch2WithMain)

# If requested, tell CMake (+IDEs) how to discover all Catch2 tests
if(OPYN_DISCOVER_TESTS)
    include(Catch)
    catch_discover_tests(testopynsim)
endif()
