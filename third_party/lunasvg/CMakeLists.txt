cmake_minimum_required(VERSION 3.5)

project(lunasvg VERSION 2.4.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

option(BUILD_SHARED_LIBS "Build as a shared library" ON)
option(LUNASVG_BUILD_EXAMPLES "Build example(s)" OFF)

add_library(lunasvg)

add_subdirectory(include)
add_subdirectory(source)
add_subdirectory(3rdparty/plutovg)

target_compile_definitions(lunasvg PRIVATE LUNASVG_BUILD)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(lunasvg PRIVATE LUNASVG_BUILD_STATIC)
endif()

if(LUNASVG_BUILD_EXAMPLES)
    add_executable(svg2png svg2png.cpp)
    target_link_libraries(svg2png PRIVATE lunasvg)
    target_include_directories(svg2png PRIVATE 3rdparty/stb)
endif()

# opensim-creator: ensure the installation creates a standard layout that doesn't
# include the host's machine
include(GNUInstallDirs)
install(FILES include/lunasvg.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS lunasvg EXPORT lunasvg-config)
install(EXPORT lunasvg-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lunasvg NAMESPACE lunasvg::)
