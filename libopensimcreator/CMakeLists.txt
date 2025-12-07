include(cmake/osc_strict_compiler_options.cmake)  # OSC_STRICT_COMPILER_OPTIONS

if(NOT TARGET oscar)
    find_package(oscar REQUIRED)
endif()
if(NOT TARGET opynsim)
    find_package(opynsim REQUIRED)
endif()

file(GLOB_RECURSE OSC_SOURCE_AND_HEADER_FILES "*.cpp")
list(FILTER OSC_SOURCE_AND_HEADER_FILES EXCLUDE REGEX "tests")
add_library(opensimcreator STATIC ${OSC_SOURCE_AND_HEADER_FILES})
set_target_properties(opensimcreator PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(opensimcreator PUBLIC cxx_std_23)
target_compile_options(opensimcreator PRIVATE ${OSC_STRICT_COMPILER_OPTIONS})
target_include_directories(opensimcreator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # so that `#include <libopensimcreator/HEADER.h>` works
)
target_link_libraries(opensimcreator PUBLIC opynsim oscar)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
