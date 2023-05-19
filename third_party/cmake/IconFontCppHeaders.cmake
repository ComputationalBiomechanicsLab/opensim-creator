cmake_minimum_required(VERSION 3.10)

project(IconFontCppHeaders)

file(GLOB IFCH_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# https://stackoverflow.com/questions/54271925/how-to-configure-cmakelists-txt-to-install-public-headers-of-a-shared-library
add_library(IconFontCppHeaders INTERFACE)
target_include_directories(IconFontCppHeaders
    INTERFACE $<INSTALL_INTERFACE:include/IconFontCppHeaders>
)

include(GNUInstallDirs)
install(
    TARGETS IconFontCppHeaders
    EXPORT IconFontCppHeadersTargets
)
install(
    FILES ${IFCH_HEADERS}
    DESTINATION "include/IconFontCppHeaders"
)
install(
    EXPORT IconFontCppHeadersTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IconFontCppHeaders
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/IconFontCppHeadersConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/IconFontCppHeadersConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IconFontCppHeaders
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/IconFontCppHeadersConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/IconFontCppHeaders
)