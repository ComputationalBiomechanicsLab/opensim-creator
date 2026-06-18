add_executable(hellooscar hellooscar.cpp)
set_target_properties(hellooscar PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(hellooscar PUBLIC cxx_std_23)
target_compile_options(hellooscar PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang>:
        -Wunguarded-availability            # Enable checks for APIs newer than `CMAKE_OSX_DEPLOYMENT_TARGET`.
        -Werror=unguarded-availability      # Turn those checks into hard compilation errors.
        -Werror=unguarded-availability-new  # Catch newly introduced APIs in the latest SDKs as errors
    >
)
if(OPYN_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/opynsim_strict_compiler_options.cmake)
    opyn_add_strict_compiler_options_to(hellooscar)
endif()

target_link_libraries(hellooscar PRIVATE oscar)

if(TRUE)
    include(GNUInstallDirs)  # CMAKE_INSTALL_LIBDIR, _INCLUDEDIR, etc.
    install(
        TARGETS hellooscar
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT opynsim-cpp
    )
endif()

# For development on Windows, copy all runtime dlls to the binary directory
# (because Windows doesn't have an RPATH).
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
