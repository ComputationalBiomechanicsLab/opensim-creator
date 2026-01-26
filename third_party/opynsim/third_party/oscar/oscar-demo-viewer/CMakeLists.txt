add_executable(oscar_demo_viewer
    oscar_demo_viewer.cpp
)
target_link_libraries(oscar_demo_viewer PUBLIC oscar_demos)

if(OSCAR_INSTALL)
    include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
    install(TARGETS oscar_demo_viewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Development: Ensure an `osc.toml` file is available when running the demo
# from the build directory
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/osc.toml"
    @ONLY
)

# Development: On Windows, copy all runtime dlls to the binary directory
# (because Windows doesn't have an RPATH).
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if(WIN32)
    add_custom_command(
        TARGET oscar_demo_viewer
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:oscar_demo_viewer> $<TARGET_RUNTIME_DLLS:oscar_demo_viewer>
        COMMAND_EXPAND_LISTS
    )
endif()
