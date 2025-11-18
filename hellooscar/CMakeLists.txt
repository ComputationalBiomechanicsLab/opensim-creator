add_executable(hellooscar hellooscar.cpp)
set_target_properties(hellooscar PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(hellooscar PUBLIC cxx_std_23)
if(OSCAR_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/oscar_strict_compiler_options.cmake)
    target_compile_options(hellooscar PRIVATE ${OSCAR_STRICT_COMPILER_OPTIONS})
endif()
target_link_libraries(hellooscar PRIVATE oscar)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if(WIN32)
    add_custom_command(
        TARGET hellooscar
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:hellooscar> $<TARGET_RUNTIME_DLLS:hellooscar>
        COMMAND_EXPAND_LISTS
    )
endif()

