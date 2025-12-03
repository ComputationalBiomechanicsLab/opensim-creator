# Create an executable program that uses the python native API to directly call into
# python (embedded-mode) and run `opynsim`'s `__main__`.
#
# This makes it easier to attach a debugger to this executable via an IDE and directly break
# into native code.

if(NOT OPYN_VENV)
    message(FATAL_ERROR "Cannot build opynsim_debugger against the system python: the `OPYN_VENV` CMake environment variable must be set, and configured to point to an editable opynsim install, have necessary packages installed, etc. (alternatively, set OPYN_BUILD_OPYNSIM_DEBUGGER OFF to skip building the debugger)")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/opynsim_debugger.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated/opynsim_debugger.cpp
    @ONLY
)
add_executable(opynsim_debugger ${CMAKE_CURRENT_BINARY_DIR}/generated/opynsim_debugger.cpp)
target_compile_features(opynsim_debugger PUBLIC cxx_std_20)
target_link_libraries(opynsim_debugger PUBLIC Python::Python)
add_dependencies(opynsim_debugger _opynsim_native)  # ensure bindings are built first
