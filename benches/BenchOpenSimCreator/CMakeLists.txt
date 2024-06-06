find_package(benchmark REQUIRED CONFIG)

# BenchOpenSimCreator: main exe that links to `osccore` and benches parts of the APIs
add_executable(BenchOpenSimCreator

    Utils/BenchOpenSimHelpers.cpp
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
    add_custom_command(
        TARGET BenchOpenSimCreator
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:BenchOpenSimCreator> $<TARGET_FILE_DIR:BenchOpenSimCreator>
        COMMAND_EXPAND_LISTS
    )
endif()
