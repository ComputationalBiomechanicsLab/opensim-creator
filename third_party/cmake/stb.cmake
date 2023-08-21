cmake_minimum_required(VERSION 3.10)

project(stb)

# https://stackoverflow.com/questions/54271925/how-to-configure-cmakelists-txt-to-install-public-headers-of-a-shared-library
file(GLOB STB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(stb INTERFACE)
target_include_directories(stb
    INTERFACE $<INSTALL_INTERFACE:include/stb>
)

include(GNUInstallDirs)
install(
    TARGETS stb
    EXPORT stbTargets
)
install(
    FILES ${STB_HEADERS}
    DESTINATION include/stb
)
install(
    EXPORT stbTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stb
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/stbConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/stbConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stb
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/stbConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/stb
)
