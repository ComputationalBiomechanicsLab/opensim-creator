# Create an executable program that uses the python native API to directly call into
# python (embedded-mode) and run `opynsim`'s `__main__`.
#
# This makes it easier to attach a debugger to this executable via an IDE and directly break
# into native code.

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/opynsim_debugger.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/opynsim_debugger.cpp
    @ONLY
)
add_executable(opynsim_debugger ${CMAKE_CURRENT_BINARY_DIR}/generated/opynsim_debugger.cpp)
target_link_libraries(opynsim_debugger PUBLIC Python::Python)
add_dependencies(opynsim_debugger _opynsim_native)  # ensure bindings are built first
