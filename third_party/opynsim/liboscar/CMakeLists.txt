# find library packages
if (NOT OPYN_EMSCRIPTEN)
    # These libraries are either provided by emscripten later on, or aren't
    # supported by the emscripten build (they're #ifdef'd out in `oscar`'s
    # source code).
    find_package(glad REQUIRED CONFIG)
    find_package(SDL3 REQUIRED CONFIG)
endif()
find_package(imgui REQUIRED CONFIG)
find_package(implot REQUIRED CONFIG)
find_package(tomlplusplus REQUIRED CONFIG)
find_package(stb REQUIRED CONFIG)
find_package(lunasvg REQUIRED CONFIG)
find_package(unordered_dense REQUIRED CONFIG)
find_package(Threads REQUIRED)  # implicitly required by SDL3/plutovg

# generate `oscar_config.h`
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/oscar_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/oscar_config.h"
    @ONLY
)

# generate `oscar.h`
if(TRUE)
    # `oscar.h` is dependent on all other header files in oscar
    file(GLOB_RECURSE _oscar_header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.h")
    list(FILTER _oscar_header_files EXCLUDE REGEX "(tests|opengl|detail)")

    set(OSCAR_INCLUDE_LIST "")
    foreach(_rel_path ${_oscar_header_files})
        string(APPEND OSCAR_INCLUDE_LIST "#include <liboscar/${_rel_path}>\n")
    endforeach()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/oscar.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/oscar.h"
        @ONLY
    )
endif()

# gather source code, exclude tests
file(GLOB_RECURSE _oscar_source_files "*.cpp")
list(FILTER _oscar_source_files EXCLUDE REGEX "tests")
add_library(oscar STATIC ${_oscar_source_files})
unset(_oscar_source_files)

set_target_properties(oscar PROPERTIES CXX_EXTENSIONS OFF)
if(OPYN_BUILD_PYTHON_BINDINGS)
    set_target_properties(oscar PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_compile_features(oscar PUBLIC cxx_std_23)
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/opynsim_strict_compiler_options.cmake)
    target_compile_options(oscar PRIVATE ${OPYN_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(oscar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>         # so that `#include <liboscar/HEADER.h>` works
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>  # so that `#include <liboscar/oscar_config.h>` and `#include <liboscar/oscar.h>` work
    $<INSTALL_INTERFACE:include>                              # because all headers are installed into `liboscar/`
)
target_link_libraries(oscar
PRIVATE
    # these libraries are either provided by emscripten later on, or aren't
    # supported by the emscripten build (they're #ifdef'd out)
    $<$<NOT:$<BOOL:${OPYN_EMSCRIPTEN}>>:glad>
    $<$<NOT:$<BOOL:${OPYN_EMSCRIPTEN}>>:SDL3::SDL3>

    unordered_dense::unordered_dense
    tomlplusplus::tomlplusplus
    imgui
    implot
    stb
    lunasvg::lunasvg
    Threads::Threads  # implicitly required by SDL3
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(TRUE)
    include(GNUInstallDirs)             # `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_INCLUDEDIR`
    include(CMakePackageConfigHelpers)  # `configure_package_config_file`

    # Install generated `oscar_config.h`
    install(
        FILES       "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/oscar_config.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liboscar"
        COMPONENT   opynsim-cpp
    )

    # Install generated `oscar.h`
    install(
        FILES       "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/oscar.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liboscar"
        COMPONENT   opynsim-cpp
    )

    # Install all of `liboscar`'s header files to `include/liboscar`
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liboscar"
        COMPONENT   opynsim-cpp
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `liboscar` library target
    install(
        TARGETS   oscar
        EXPORT    oscarTargets
        COMPONENT opynsim-cpp
    )
    install(
        EXPORT      oscarTargets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar"
        COMPONENT   opynsim-cpp
    )

    # Generate + install an `oscarConfig.cmake` file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/oscarConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/oscarConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar"
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/oscarConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar"
        COMPONENT opynsim-cpp
    )
endif()
