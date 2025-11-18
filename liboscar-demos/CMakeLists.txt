file(GLOB_RECURSE OSCAR_DEMO_SOURCES "*.cpp")
list(FILTER OSCAR_DEMO_SOURCES EXCLUDE REGEX "(tests|\.tests\.cpp)")
add_library(oscar_demos STATIC ${OSCAR_DEMO_SOURCES})
set_target_properties(oscar_demos PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(oscar_demos PUBLIC cxx_std_23)
if(OSCAR_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/oscar_strict_compiler_options.cmake)
    target_compile_options(oscar_demos PRIVATE ${OSCAR_STRICT_COMPILER_OPTIONS})
endif()
target_include_directories(oscar_demos PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>  # so that `#include "liboscar-demos/HEADER.h"` works
    $<INSTALL_INTERFACE:include>                       # because all headers are installed into `liboscar-demos/`
)
target_link_libraries(oscar_demos PUBLIC oscar)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(OSCAR_INSTALL)
    include(GNUInstallDirs)             # `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_INCLUDEDIR`
    include(CMakePackageConfigHelpers)  # `configure_package_config_file`

    # Install all of `liboscar-demos`'s header files to `include/liboscar-demos`
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liboscar-demos"
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `liboscar-demos` target
    install(TARGETS oscar_demos EXPORT oscar_demosTargets)
    install(EXPORT oscar_demosTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar_demos")

    # Generate + install an `oscarConfig.cmake` file (it's what `find_package` looks for)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/oscar_demosConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/oscar_demosConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar_demos"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/oscar_demosConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/oscar_demos")
endif()
