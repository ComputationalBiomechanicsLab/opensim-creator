option(OSCAR_FORCE_ASSERTS_ENABLED             "Enable OSC's runtime assertions - even if building a release build" ON)
option(OSCAR_RUNTIME_PERF_MEASUREMENTS_ENABLED "Enable whether the `OSC_PERF` macro records performance or not"     ON)
option(OSCAR_EMSCRIPTEN                        "Enable special build parameters for emscripten (emsdk)"             OFF)
mark_as_advanced(OSCAR_EMSCRIPTEN)  # don't try the option this unless you know what you're doing ;)

# find library packages
if (NOT OSCAR_EMSCRIPTEN)
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

# generate `oscarconfig.h`
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/oscarconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/oscarconfig.h"
    @ONLY
)

# gather source code, exclude tests
file(GLOB_RECURSE OSCAR_SOURCE_FILES "*.cpp")
list(FILTER OSCAR_SOURCE_FILES EXCLUDE REGEX "(tests|\.tests\.cpp)")
add_library(oscar STATIC ${OSCAR_SOURCE_FILES})
set_target_properties(oscar PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(oscar PUBLIC cxx_std_23)
if(OSCAR_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/oscar_strict_compiler_options.cmake)
    target_compile_options(oscar PRIVATE ${OSCAR_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(oscar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>         # so that `#include <liboscar/HEADER.h>` works
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>  # so that #include <liboscar/oscarconfig.h> works
    $<INSTALL_INTERFACE:include>                              # because all headers are installed into `liboscar/`
)
target_link_libraries(oscar
PRIVATE
    # these libraries are either provided by emscripten later on, or aren't
    # supported by the emscripten build (they're #ifdef'd out)
    $<$<NOT:$<BOOL:${OSCAR_EMSCRIPTEN}>>:glad>
    $<$<NOT:$<BOOL:${OSCAR_EMSCRIPTEN}>>:SDL3::SDL3>

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

if(OSCAR_INSTALL)
    include(GNUInstallDirs)             # `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_INCLUDEDIR`
    include(CMakePackageConfigHelpers)  # `configure_package_config_file`

    # Install generated `oscarconfig.h`
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/liboscar/oscarconfig.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liboscar")

    # Install all of `liboscar`'s header files to `include/liboscar`
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liboscar"
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `liboscar` library target
    install(TARGETS oscar EXPORT oscarTargets)
    install(EXPORT oscarTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar")

    # Generate + install an `oscarConfig.cmake` file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/oscarConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/oscarConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/oscarConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar")
endif()
