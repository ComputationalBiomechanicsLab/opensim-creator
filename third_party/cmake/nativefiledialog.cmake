cmake_minimum_required(VERSION 3.10)

project(nativefiledialog)

if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
    include(FindPkgConfig)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    add_library(nativefiledialog STATIC
        src/nfd_gtk.c
        src/nfd_common.c
    )
    target_link_libraries(nativefiledialog INTERFACE
        ${GTK3_LIBRARIES}
    )
    target_include_directories(nativefiledialog
        INTERFACE $<INSTALL_INTERFACE:include/nativefiledialog>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include
        PRIVATE ${GTK3_INCLUDE_DIRS}
    )
    set_target_properties(nativefiledialog PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
elseif(WIN32)
    add_library(nativefiledialog STATIC
        src/nfd_win.cpp
        src/nfd_common.c
    )
    target_include_directories(nativefiledialog
        INTERFACE $<INSTALL_INTERFACE:include/nativefiledialog>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    )
    set_target_properties(nativefiledialog PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
elseif(APPLE)
    add_library(nativefiledialog STATIC
        src/nfd_cocoa.m
        src/nfd_common.c
    )
    target_link_libraries(nativefiledialog INTERFACE
        "-framework Cocoa"
    )
    target_include_directories(nativefiledialog
        INTERFACE $<INSTALL_INTERFACE:include/nativefiledialog>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    )
    set_target_properties(nativefiledialog PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
else()
    message(FATAL_ERROR "no implementation of nfd.h available on this platform: required for native platform file dialogs")
endif()

include(GNUInstallDirs)
install(
    TARGETS nativefiledialog
    EXPORT nativefiledialogTargets
)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/include/nfd.h"
    DESTINATION include/nativefiledialog
)
install(
    EXPORT nativefiledialogTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nativefiledialog
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialogConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/nativefiledialogConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nativefiledialog
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/nativefiledialogConfig.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/nativefiledialog
)
