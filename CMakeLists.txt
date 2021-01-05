cmake_minimum_required(VERSION 3.13)

project(osmv VERSION 0.0.1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(CMAKE_BUILD_TYPE MATCHES "Rel")  # e.g. RelWithDebInfo or Release

    # use LTO on all vendored libs/code when building a release
    # - note: this makes builds *much* slower
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# vendored dependencies
# - this project deliberately uses C libraries that only transitively depend on
#   OS-level libs. They're copied in-tree to reduce the amount of fucking around
#   required to build osmv
set(SDL_AUDIO OFF CACHE BOOL "")
set(SDL_TEST OFF CACHE BOOL "")
add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm-0.9.9.8 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)

# opensim: linked separately
# - located via CMAKE_PREFIX_PATH. Builders should set their CMAKE_PREFIX_PATH
#   to the `lib/cmake` dir of an OpenSim install
# - e.g: -DCMAKE_PREFIX_PATH=/home/adam/opensim-install/lib/cmake
find_package(OpenSim REQUIRED)

# configuration: generate osmv_config.hpp
# - contains any per-config info (e.g. version, build type)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/ RESOURCES_DIR)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/ SHADERS_DIR)
configure_file("${PROJECT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")


add_executable(osmv
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/gl.hpp
    src/gl.cpp
    src/gl_extensions.hpp
    src/gl_extensions.cpp
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
    src/3d_common.hpp
    src/3d_common.cpp
    src/fd_simulation.hpp

    src/osmv.cpp
)
target_include_directories(osmv PUBLIC
    third_party/SDL2-2.0.12/include
    third_party/glm-0.9.9.8/
    third_party/glew-2.1.0/include
    third_party/imgui-1.78/
    third_party/stb_image/
    ${PROJECT_BINARY_DIR}  # for osmv_config.hpp
)
target_link_libraries(osmv
    # use static libraries if not in debug mode
    $<$<CONFIG:Debug>:SDL2>
    $<$<NOT:$<CONFIG:Debug>>:SDL2-static>
    $<$<CONFIG:Debug>:glew>
    $<$<NOT:$<CONFIG:Debug>>:glew_s>

    osimCommon
    osimSimulation
    osimActuators
    osimAnalyses
    osimTools
    SimTKcommon
    SimTKmath
    SimTKsimbody

    # osimLepton  doesn't seem necessary
)
target_compile_options(osmv PUBLIC
    # disable MSVC's (default) permissive mode
    # - This forces MSVC to obey ISO C++
    # - I do this mostly because I like the logic keywords (e.g. `x or b`)
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)
target_compile_features(osmv PUBLIC
    # osmv is a GUI project, so doesn't need to worry about downstream API users
    cxx_std_17
)
set_target_properties(osmv PROPERTIES
    CXX_EXTENSIONS OFF
)

# tell IDEs to put vendored code into a separate tree
source_group(TREE third_party/)


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
