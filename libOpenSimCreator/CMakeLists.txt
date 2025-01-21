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
    cxx_std_20
)
target_compile_options(OpenSimCreator PUBLIC

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
target_compile_definitions(OpenSimCreator PRIVATE
    # OpenSim's `spdlog` transitively uses a deprecated `stdext::checked_array_iterator`
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING>
)
target_include_directories(OpenSimCreator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # so that `#include <libopensimcreator/HEADER.h>` works
)
target_link_libraries(OpenSimCreator PUBLIC
    oscar
    oscar_demos
    osim
)

if(BUILD_TESTING)
    add_subdirectory(testing)
endif()
