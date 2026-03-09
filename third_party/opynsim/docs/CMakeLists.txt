# Ensure the caller's build system has 'doxygen'
find_package(Doxygen)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
)

# Create a custom build target that builds the C++ documentation
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/doxygen)
add_custom_target(opynsim_doxygen_docs
    COMMAND           ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT           "Generating internal C++ API documentation with Doxygen"
    VERBATIM
)

# Create a custom build target that builds the Sphinx documentation
add_custom_target(opynsim_docs
    COMMAND ${CMAKE_COMMAND} -E env
        PYTHONPATH=$<TARGET_FILE_DIR:_core>/..
        ${Python_EXECUTABLE} -m sphinx.cmd.build
        -M html
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building Sphinx documentation"
)
add_dependencies(opynsim_docs opynsim_doxygen_docs _core_development_stubs)
