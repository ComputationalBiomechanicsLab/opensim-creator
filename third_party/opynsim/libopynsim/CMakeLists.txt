option(BUILD_TESTING              "Build test suites"                                                      ON)
option(OPYN_BUILD_TESTING         "Build test suites (OPynSim-specific)"                                   ${BUILD_TESTING})
option(OPYN_FORCE_ASSERTS_ENABLED "Enable OPynSim's runtime assertions - even if building a release build" ON)

find_package(mdspan REQUIRED)
if(NOT TARGET oscar)  # i.e. if oscar isn't being built inline
    find_package(oscar REQUIRED)
endif()

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
    oscar           # UI
)
target_include_directories(opynsim
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>          # so that `#include <libopynsim/header.h>` works
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/>  # so that `#include <libopynsim/opynsimconfig.h>` works
    $<INSTALL_INTERFACE:include>                               # `libopysim` is installed into the `include` root
)
target_compile_features(opynsim PUBLIC cxx_std_20)  # CARE: MSVC C++23 isn't ABI-stable yet
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/opynsim_strict_compiler_options.cmake)
    target_compile_options(opynsim PRIVATE ${OPYN_STRICT_COMPILER_OPTIONS})
endif()
set_target_properties(opynsim PROPERTIES CXX_EXTENSIONS OFF)
if(OPYN_BUILD_PYTHON_BINDINGS)
    set_target_properties(opynsim PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(OPYN_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Handle installation
if(TRUE)
    include(GNUInstallDirs)             # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
    include(CMakePackageConfigHelpers)  # configure_package_config_file

    # Install generated `opynsimconfig.h`
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/libopynsim/opynsimconfig.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libopynsim" COMPONENT opynsim-cpp)

    # Install all of `libopynsim`'s header files (excl. tests)
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libopynsim"
        COMPONENT opynsim-cpp
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
        PATTERN "cmake" EXCLUDE
    )

    # Install `libopynsim` library target
    install(TARGETS opynsim EXPORT opynsimTargets COMPONENT opynsim-cpp)
    install(EXPORT opynsimTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opynsim" COMPONENT opynsim-cpp)

    # Generate + install an `opynsimConfig.cmake` file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/opynsimConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/opynsimConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opynsim"
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/opynsimConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opynsim" COMPONENT opynsim-cpp)
endif()
