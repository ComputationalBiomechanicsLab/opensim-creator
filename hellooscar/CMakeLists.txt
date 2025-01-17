add_executable(hellooscar
    hellooscar.cpp
)
set_target_properties(hellooscar PROPERTIES
    CXX_EXTENSIONS OFF
)
target_compile_features(hellooscar PUBLIC
    cxx_std_20
)
target_link_libraries(hellooscar PRIVATE
    oscar
)

# -------------- installation/packaging ------------- #
if(DEFINED OSC_EMSCRIPTEN AND ${OSC_EMSCRIPTEN})
    include(${CMAKE_CURRENT_SOURCE_DIR}/Emscripten/Packaging.cmake)
endif()

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET hellooscar
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:hellooscar> $<TARGET_RUNTIME_DLLS:hellooscar>
        COMMAND_EXPAND_LISTS
    )
endif()
