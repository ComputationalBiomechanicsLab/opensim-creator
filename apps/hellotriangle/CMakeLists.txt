add_executable(hellotriangle hellotriangle.cpp)

target_link_libraries(hellotriangle PUBLIC
    oscar_compiler_configuration  # so that it uses standard compiler flags etc.
    oscar
)

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
