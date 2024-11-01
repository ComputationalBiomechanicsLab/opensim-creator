add_executable(hellotriangle
    hellotriangle.cpp
)
set_target_properties(hellotriangle PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(hellotriangle PUBLIC
    cxx_std_20
)
target_link_libraries(hellotriangle PRIVATE
    oscar
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
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:hellotriangle> $<TARGET_RUNTIME_DLLS:hellotriangle>
        COMMAND_EXPAND_LISTS
    )
endif()
