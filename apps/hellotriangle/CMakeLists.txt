find_package(imgui REQUIRED CONFIG)
find_package(ImGuiColorTextEdit REQUIRED CONFIG)
find_package(lua REQUIRED CONFIG)

add_executable(hellotriangle hellotriangle.cpp)

target_link_libraries(hellotriangle PRIVATE
    oscar_compiler_configuration  # so that it uses standard compiler flags etc.
    oscar
    imgui
    ImGuiColorTextEdit
    lua
)

set_target_properties(hellotriangle PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)

# -------------- installation/packaging ------------- #
if(${OSC_EMSCRIPTEN})
    include(${CMAKE_CURRENT_SOURCE_DIR}/Emscripten/Packaging.cmake)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET hellotriangle
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:hellotriangle> $<TARGET_FILE_DIR:hellotriangle>
        COMMAND_EXPAND_LISTS
    )
endif()
