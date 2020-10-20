cmake_minimum_required(VERSION 3.13)

project(osmv VERSION 0.0.1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# libraries: apart from OpenSim, only quickly-compiled C libraries are used, so
# they are just included in the source tree, rather than fucking around with a
# package manager etc.
add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm-0.9.9.8 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)
find_package(OpenGL REQUIRED)
find_package(OpenSim REQUIRED)

# generate top-level configured file that contains version etc.
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/ RESOURCES_DIR)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/ SHADERS_DIR)
configure_file("${PROJECT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")

if (CMAKE_BUILD_TYPE MATCHES Debug)
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()


add_library(common STATIC
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/gl.hpp
    src/gl.cpp
    src/gl_extensions.hpp
    src/gl_extensions.cpp
    src/glm_extensions.hpp
    src/glm_extensions.cpp
    src/sdl.hpp
    src/sdl.cpp
    src/imgui_extensions.hpp
    src/imgui_extensions.cpp

    src/application.hpp
    src/application.cpp
    src/screen.hpp
    src/show_model_screen.hpp
    src/show_model_screen.cpp
    src/loading_screen.hpp
    src/loading_screen.cpp
    src/opensim_wrapper.hpp
    src/opensim_wrapper.cpp
    src/meshes.hpp
    src/meshes.cpp
)
target_include_directories(common PUBLIC
    # ${OPENGL_INCLUDE_DIR}  // GLEW handles forwarding the OpenGL API
    third_party/SDL2-2.0.12/include
    third_party/glm-0.9.9.8/
    third_party/glew-2.1.0/include
    third_party/imgui-1.78/
    third_party/stb_image/
    ${PROJECT_BINARY_DIR}  # so configure_file config can be #included
)
target_link_libraries(common
    ${OPENGL_LIBRARIES}
    # in release mode, use statically-linked SDL and GLEW with LTO to give the
    # smallest resulting binary and lowest perf cost
    $<$<CONFIG:Debug>:SDL2>
    $<$<NOT:$<CONFIG:Debug>>:SDL2-static>
    $<$<CONFIG:Debug>:glew>
    $<$<NOT:$<CONFIG:Debug>>:glew>

#    osimLepton
    osimCommon
    osimSimulation
#    osimActuators
#    osimAnalyses
    osimTools
    SimTKcommon
    SimTKmath
    SimTKsimbody
)
target_compile_options(common PUBLIC
    # disable MSVC permissiveness. Forces MSVC to obey C++ standard
    # - specifically, this enables the use of 'and', 'not', etc.
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)
target_compile_features(common PUBLIC
    cxx_std_20
)
set_target_properties(common PROPERTIES
    CXX_EXTENSIONS OFF
)
source_group(TREE third_party/)


add_executable(osmv src/osmv.cpp)
target_link_libraries(osmv common)

add_executable(sbv src/sbv.cpp)
target_link_libraries(sbv common)


# BUILD LIB PATHS:
#
# in Windows, copy libraries to the exe dir, because Windows specifically
# searches in the exe's dir for libraries. see:
#
# https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
if (WIN32)
    file(GLOB OPENSIM_LIBS LIST_DIRECTORIES FALSE "${OpenSim_ROOT_DIR}/bin/*.dll")
    foreach(OPENSIM_LIB ${OPENSIM_LIBS})
        add_custom_command(
            TARGET osmv
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSIM_LIB}" "$<TARGET_FILE_DIR:osmv>")
    endforeach()
endif()
