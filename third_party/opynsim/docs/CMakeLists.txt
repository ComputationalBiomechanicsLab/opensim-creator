find_package(Doxygen)  # Optional: builds C++ documentation

set(OPYN_SPHINX_OUTPUT_DIR  "${CMAKE_CURRENT_BINARY_DIR}/html")
set(OPYN_DOXYGEN_OUTPUT_DIR "${OPYN_SPHINX_OUTPUT_DIR}/doxygen")

# Create a custom build target that clears the Sphinx cache (easy to dirty)
add_custom_target(opynsim_clean_docs
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${OPYN_SPHINX_OUTPUT_DIR}"
)

# Create a custom build target that builds the Sphinx documentation
add_custom_target(opynsim_docs
    DEPENDS _core_development_stubs
    COMMAND
        ${CMAKE_COMMAND} -E env PYTHONPATH=$<TARGET_FILE_DIR:_core>/..
        ${Python_EXECUTABLE} -m sphinx.cmd.build -E "${CMAKE_CURRENT_SOURCE_DIR}/source" "${OPYN_SPHINX_OUTPUT_DIR}"
    COMMENT "Building Sphinx documentation"
)

# Create a custom target that can be ran to live-rebuild the
# documentation for the current configuration.
add_custom_target(opynsim_docs_autobuild
    DEPENDS _core_development_stubs
    USES_TERMINAL
    COMMAND
        ${CMAKE_COMMAND} -E env PYTHONPATH=$<TARGET_FILE_DIR:_core>/..
        ${Python_EXECUTABLE} -m sphinx_autobuild -E "${CMAKE_CURRENT_SOURCE_DIR}/source" "${OPYN_SPHINX_OUTPUT_DIR}"
)

# Create a custom target that can be ran to deploy the built documentation
# to the live server.
#
# NOTE: This assumes the caller has already globally configured SSH (rsync)
#       access to `docs.opynsim.eu` in their `.ssh/config` so that
#       passwordless access works (e.g. with an `IdentityFile` entry or similar).
add_custom_target(opynsim_docs_deploy
    DEPENDS opynsim_docs
    USES_TERMINAL
    COMMAND rsync -avz --delete "${OPYN_SPHINX_OUTPUT_DIR}/" "docs.opynsim.eu:/var/www/docs.opynsim.eu/manual/en/latest"
)

if(${Doxygen_FOUND})
    # If the caller's system has Doxygen then it should be used to
    # build C++ documentation

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )

    # Create a custom build target that builds the C++ documentation
    add_custom_target(opynsim_doxygen_docs
        COMMAND           "${CMAKE_COMMAND}" -E make_directory "${OPYN_DOXYGEN_OUTPUT_DIR}" &&
                          "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT           "Generating internal C++ API documentation with Doxygen"
        VERBATIM
    )

    add_dependencies(opynsim_docs           opynsim_doxygen_docs)
    add_dependencies(opynsim_docs_autobuild opynsim_doxygen_docs)
endif()
