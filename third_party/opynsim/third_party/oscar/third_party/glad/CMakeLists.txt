cmake_minimum_required(VERSION 3.10)
project(glad VERSION 0.0.1 LANGUAGES C)

include(CMakePackageConfigHelpers) # configure_package_config_file
include(GNUInstallDirs)            # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.

add_library(glad STATIC
    src/glad.c
)
target_include_directories(glad
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INTERFACE $<INSTALL_INTERFACE:include>
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/glad/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/glad")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/KHR")
install(TARGETS glad EXPORT gladTargets)
install(EXPORT gladTargets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/glad")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/gladConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/glad"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/glad")
