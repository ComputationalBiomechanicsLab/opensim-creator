option(OSC_USE_STRICT_COMPILER_FLAGS "Add stricter compilation flags to targets"      OFF)
option(
    OSC_LIBOPENSIMCREATOR_INSTALL
    "Install libopensimcreator library and headers. Use at your own risk: opensimcreator's C++ API isn't stable, supported, or designed for public consumption (we use it for internal prototypes)"
    OFF
)
mark_as_advanced(OSC_LIBOPENSIMCREATOR_INSTALL)  # Only for internal use at the moment

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
target_compile_options(opensimcreator PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang>:
        -Werror=unguarded-availability      # error if targeting earlier versions of MacOS with a newer SDK (can produce invalid binaries?)
        -Werror=unguarded-availability-new  # error if targeting earlier versions of MacOS with a newer SDK (can produce invalid binaries?)
    >
)
if(OSC_USE_STRICT_COMPILER_FLAGS)
    include(cmake/osc_strict_compiler_options.cmake)
    target_compile_options(opensimcreator PRIVATE ${OSC_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(opensimcreator
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"  # so that `#include <libopensimcreator/HEADER.h>` works
    PUBLIC  $<INSTALL_INTERFACE:include>                        # because all headers are installed into `liboscar/`
)
target_link_libraries(opensimcreator PUBLIC opynsim oscar)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(OSC_LIBOPENSIMCREATOR_INSTALL)
    include(GNUInstallDirs)             # `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_INCLUDEDIR`
    include(CMakePackageConfigHelpers)  # `configure_package_config_file`

    # Install all of `libopensimcreator`'s header files to `include/libopensimcreator`
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libopensimcreator"
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `libopensimcreator` library target
    install(TARGETS opensimcreator EXPORT opensimcreatorTargets)
    install(EXPORT opensimcreatorTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator")

    # Generate + install an `opensimcreatorConfig.cmake` file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/opensimcreatorConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/opensimcreatorConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/opensimcreatorConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator")
endif()
