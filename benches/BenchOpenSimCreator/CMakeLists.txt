find_package(benchmark REQUIRED CONFIG)

# BenchOpenSimCreator: main exe that links to `osccore` and benches parts of the APIs
add_executable(BenchOpenSimCreator
    BenchOpenSimHelpers.cpp
    BenchOpenSimRenderer.cpp
)

target_link_libraries(BenchOpenSimCreator PUBLIC
    # set compile options
    oscar_compiler_configuration

    # link to the to-be-tested library
    OpenSimCreator

    # link to testing library
    benchmark::benchmark
    benchmark::benchmark_main
)

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    # HACK: we can't use TARGET_RUNTIME_DLLS because simbody doesn't package libgfortran etc. correctly
    file(GLOB_RECURSE dlls "${CMAKE_PREFIX_PATH}/*.dll")
    add_custom_command(
        TARGET BenchOpenSimCreator
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dlls} $<TARGET_FILE_DIR:BenchOpenSimCreator>
        COMMAND_EXPAND_LISTS
    )
endif()
