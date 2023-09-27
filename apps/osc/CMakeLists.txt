# osc:
#
#     executable frontend to the `osccore` library
add_executable(osc

    "osc.cpp"  # entrypoint (main)

    # Windows: also link a resources file (rc)
    #
    # the resources file tells MSVC compiler how to compile non-source resources
    # into the output exe. Specifically, it's used to embed the application icon
    # into the `osc` exe
    $<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/../../build_resources/resources.rc>
)

# osc: link to necessary libraries
target_link_libraries(osc PUBLIC

    # so that it uses standard compiler flags etc.
    oscar_compiler_configuration

    # so that it can boot into the main osc codebase
    OpenSimCreator
)

target_include_directories(osc PUBLIC

    # necessary, because Windows' `resources.rc` uses the include path
    # to find resources
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# osc: link options
target_link_options(osc PUBLIC

    # MSVC (Windows)
    $<$<CXX_COMPILER_ID:MSVC>:

        # open as a desktop app, not CLI
        /SUBSYSTEM:windows

        # as above, call into `main`
        /ENTRY:mainCRTStartup
    >
)

# -------------- development nice-to-haves ------------- #

# generate a dev-centric `osc.toml`
#
#     - this causes osc to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../build_resources/DEV_osc.toml.in" "${CMAKE_BINARY_DIR}/osc.toml")

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    # HACK: we can't use TARGET_RUNTIME_DLLS because simbody doesn't package libgfortran etc. correctly
    file(GLOB_RECURSE dlls "${CMAKE_PREFIX_PATH}/*.dll")
    add_custom_command(
        TARGET osc
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dlls} $<TARGET_FILE_DIR:osc>
        COMMAND_EXPAND_LISTS
    )
endif()
