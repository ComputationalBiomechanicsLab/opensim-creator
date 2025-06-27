include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/osc_strict_compiler_options.cmake)  # OSC_STRICT_COMPILER_OPTIONS

file(GLOB_RECURSE OSC_OSCAR_DEMO_SOURCES
    LIST_DIRECTORIES FALSE
    "*.cpp"
)
list(FILTER OSC_OSCAR_DEMO_SOURCES EXCLUDE REGEX "testing")
list(FILTER OSC_OSCAR_DEMO_SOURCES EXCLUDE REGEX "\.tests\.cpp")

add_library(oscar_demos STATIC
    ${OSC_OSCAR_DEMO_SOURCES}
)
set_target_properties(oscar_demos PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(oscar_demos PUBLIC
    cxx_std_23
)
target_compile_options(oscar_demos PRIVATE
    ${OSC_STRICT_COMPILER_OPTIONS}
)
target_include_directories(oscar_demos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..      # so that `#include "liboscar-demos/HEADER.h"` works
)
target_link_libraries(oscar_demos PUBLIC
    oscar
)

if(BUILD_TESTING)
    add_subdirectory(testing)
endif()
