file(GLOB_RECURSE LIBOPYNSIM_SOURCE_FILES "*.cpp")
list(FILTER LIBOPYNSIM_SOURCE_FILES EXCLUDE REGEX "tests")
add_library(opensimcreator STATIC ${LIBOPYNSIM_SOURCE_FILES})
unset(LIBOPYNSIM_SOURCE_FILES)

target_compile_features(opensimcreator PUBLIC cxx_std_23)
set_target_properties(opensimcreator PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/opynsim_strict_compiler_options.cmake)
    opyn_add_strict_compiler_options_to(opensimcreator)
endif()
target_include_directories(opensimcreator
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"  # so that `#include <libopensimcreator/HEADER.h>` works
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
        COMPONENT opensimcreator-cpp
        FILES_MATCHING PATTERN "*.h"
        PATTERN "tests" EXCLUDE
    )

    # Install `libopensimcreator` library target
    install(
        TARGETS opensimcreator
        EXPORT opensimcreatorTargets
        COMPONENT opensimcreator-cpp
    )
    install(
        EXPORT opensimcreatorTargets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/opensimcreator"
        COMPONENT opensimcreator-cpp
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
        COMPONENT opensimcreator-cpp
    )
endif()
