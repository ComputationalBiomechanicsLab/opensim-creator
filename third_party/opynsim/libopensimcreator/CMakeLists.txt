file(GLOB_RECURSE _libopensimcreator_sources "*.cpp")
list(FILTER _libopensimcreator_sources EXCLUDE REGEX "tests")
add_library(opensimcreator STATIC ${_libopensimcreator_sources})
unset(_libopensimcreator_sources)

target_compile_features(opensimcreator PUBLIC cxx_std_23)
set_target_properties(opensimcreator PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    opyn_add_strict_compiler_options_to(opensimcreator)
endif()
if(OPYN_ENABLE_SANITIZERS)
    opyn_add_sanitizer_flags_to(opensimcreator)
endif()
target_include_directories(opensimcreator
    PUBLIC "$<BUILD_INTERFACE:${OPYN_BUILD_INTERFACE_INCLUDE_DIR}>"  # so that `#include <libopensimcreator/header.h>` works
    PUBLIC  $<INSTALL_INTERFACE:include>
)
target_link_libraries(opensimcreator PUBLIC opynsim oscar)

if(OPYN_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Handle installation
if(TRUE)
    include(GNUInstallDirs)             # `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_INCLUDEDIR`
    include(CMakePackageConfigHelpers)  # `configure_package_config_file`

    # Install all of `libopensimcreator`'s header files to `include/libopensimcreator`
    install(
        DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libopensimcreator"
        COMPONENT opensimcreator_cpp
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `libopensimcreator` library target
    install(
        TARGETS opensimcreator
        EXPORT opensimcreatorTargets
        COMPONENT opensimcreator_cpp
    )
    install(
        EXPORT opensimcreatorTargets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator"
        COMPONENT opensimcreator_cpp
    )

    # Generate + install an `opensimcreatorConfig.cmake` file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/opensimcreatorConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/opensimcreatorConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator"
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/opensimcreatorConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator"
        COMPONENT opensimcreator_cpp
    )
endif()
