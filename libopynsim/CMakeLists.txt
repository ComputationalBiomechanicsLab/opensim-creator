find_package(mdspan REQUIRED)

# Generate compile-time configuration file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/opynsimconfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libopynsim/opynsimconfig.h"
    @ONLY
)

# Gather source code (excl. tests) into a library
file(GLOB_RECURSE LIBOPYNSIM_SOURCE_FILES "*.cpp")
list(FILTER LIBOPYNSIM_SOURCE_FILES EXCLUDE REGEX "(tests|\.tests\.cpp)")
add_library(opynsim STATIC ${LIBOPYNSIM_SOURCE_FILES})
target_link_libraries(opynsim
PUBLIC
    osim            # OpenSim/Simbody/plugins
    mdspan::mdspan  # Copy-free memory buffer views (handy for fast python bindings)
)
target_include_directories(opynsim
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>          # so that `#include <libopynsim/header.h>` works
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/>  # so that `#include <libopynsim/opynsimconfig.h>` works
    $<INSTALL_INTERFACE:include>                               # `libopysim` is installed into the `include` root
)
target_compile_features(opynsim PUBLIC cxx_std_23)
set_target_properties(opynsim PROPERTIES
    CXX_EXTENSIONS            OFF
    POSITION_INDEPENDENT_CODE ON  # for python bindings
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(OPYN_INSTALL)
    include(GNUInstallDirs)             # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
    include(CMakePackageConfigHelpers)  # configure_package_config_file

    # Install generated `opynsimconfig.h`
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/libopynsim/opynsimconfig.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libopynsim")

    # Install all of `libopynsim`'s header files (excl. tests)
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libopynsim"
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `libopynsim` library target
    install(TARGETS opynsim EXPORT opynsimTargets)
    install(EXPORT opynsimTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opynsim")

    # Generate + install an `opynsimConfig.cmake` file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/opynsimConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/opynsimConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opynsim"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/opynsimConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opynsim")
endif()
