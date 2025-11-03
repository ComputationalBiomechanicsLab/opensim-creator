cmake_minimum_required(VERSION 3.10)

project(cgltf)

set(OSCDEPS_CGLTF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../cgltf")

file(GLOB OSCDEPS_CGLTF_HEADERS ${OSCDEPS_CGLTF_ROOT}/*.h)

add_library(cgltf INTERFACE)
target_include_directories(cgltf
    INTERFACE $<INSTALL_INTERFACE:include/cgltf>
)

include(GNUInstallDirs)
install(TARGETS cgltf EXPORT cgltfTargets)
install(FILES ${OSCDEPS_CGLTF_HEADERS} DESTINATION include/cgltf)
install(EXPORT cgltfTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cgltf)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cgltfConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cgltfConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cgltf
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/cgltfConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cgltf
)
