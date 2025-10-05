include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/osc_strict_compiler_options.cmake)  # OSC_STRICT_COMPILER_OPTIONS

set(
    OSC_BUILD_OSIM_INLINED OFF
    CACHE BOOL
    "Enable/disable building osim as part of building OpenSimCreator (via `add_subdirectory`) rather\
     than acquiring it externally (via `find_package`). This is useful for developing/testing\
     Simbody/OpenSim while working on OpenSim Creator"
)

find_package(mdspan REQUIRED CONFIG)

if(${OSC_BUILD_OSIM_INLINED})
    add_subdirectory(
        ${PROJECT_SOURCE_DIR}/third_party/libosim
        osim              # build directory
        EXCLUDE_FROM_ALL  # we don't want to build every part of it (transitive target)
        SYSTEM            # the headers from osim (i.e. OpenSim headers) shouldn't be subject to the same warning flags as OSC
    )
    # ensure linting doesn't happen for OpenSim/Simbody sources
    set_property(TARGET osim PROPERTY CXX_CLANG_TIDY)
    set_property(TARGET osim PROPERTY C_CLANG_TIDY)
elseif(NOT TARGET osim)
    find_package(osim REQUIRED)
endif()

file(GLOB_RECURSE OSC_SOURCE_AND_HEADER_FILES
    LIST_DIRECTORIES FALSE
    "*.cpp"
)
list(FILTER OSC_SOURCE_AND_HEADER_FILES EXCLUDE REGEX "testing")
list(FILTER OSC_SOURCE_AND_HEADER_FILES EXCLUDE REGEX "\.tests\.cpp")
add_library(OpenSimCreator STATIC
    ${OSC_SOURCE_AND_HEADER_FILES}
)
set_target_properties(OpenSimCreator PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(OpenSimCreator PUBLIC
    cxx_std_23
)
target_compile_options(OpenSimCreator PRIVATE
    ${OSC_STRICT_COMPILER_OPTIONS}
)
target_include_directories(OpenSimCreator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # so that `#include <libopensimcreator/HEADER.h>` works
)
target_link_libraries(OpenSimCreator PUBLIC
    oscar
    osim
    std::mdspan
)

if(BUILD_TESTING)
    add_subdirectory(testing)
endif()
