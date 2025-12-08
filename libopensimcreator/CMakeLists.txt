option(OSC_USE_STRICT_COMPILER_FLAGS "Add stricter compilation flags to targets" OFF)

if(NOT TARGET oscar)
    find_package(oscar REQUIRED)
endif()
if(NOT TARGET opynsim)
    find_package(opynsim REQUIRED)
endif()

file(GLOB_RECURSE _osc_sources "*.cpp")
list(FILTER _osc_sources EXCLUDE REGEX "tests")
add_library(opensimcreator STATIC ${_osc_sources})
unset(_osc_sources)

set_target_properties(opensimcreator PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(opensimcreator PUBLIC cxx_std_23)
if(OSC_USE_STRICT_COMPILER_FLAGS)
    include(cmake/osc_strict_compiler_options.cmake)
    target_compile_options(opensimcreator PRIVATE ${OSC_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(opensimcreator
    # so that `#include <libopensimcreator/HEADER.h>` works
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
)
target_link_libraries(opensimcreator PUBLIC opynsim oscar)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
