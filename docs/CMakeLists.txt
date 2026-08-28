include(GNUInstallDirs)

find_package(Python REQUIRED COMPONENTS Interpreter)

# Collect Sphinx configuration/source files, for dependency tracking
file(GLOB_RECURSE OSC_SPHINX_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*"
)
set(OSC_SPHINX_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")
set(OSC_SPHINX_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/sphinx.stamp")

# Builds the Sphinx documentation, while keeping track of the inputs/outputs
add_custom_command(
    OUTPUT "${OSC_SPHINX_STAMP_FILE}"
    COMMAND ${Python_EXECUTABLE} -m sphinx.cmd.build -E "${CMAKE_CURRENT_SOURCE_DIR}/source" "${OSC_SPHINX_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${OSC_SPHINX_STAMP_FILE}"
    DEPENDS ${OSC_SPHINX_SOURCES}
    COMMENT "Building OpenSim Creator's Sphinx documentation"
    VERBATIM
)

# Create a custom build target that builds the Sphinx documentation
add_custom_target(opensimcreator_docs ALL DEPENDS "${OSC_SPHINX_STAMP_FILE}")

# Create a manually-ran custom build target that clears the Sphinx cache (easy to dirty)
add_custom_target(opensimcreator_clean_docs
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${OSC_SPHINX_OUTPUT_DIR}"
    COMMENT "Cleaning Sphinx documentation"
)

# Create a manually-ran custom target that live-rebuild's the documentation for
# the current configuration.
add_custom_target(opensimcreator_docs_autobuild
    USES_TERMINAL
    COMMAND ${Python_EXECUTABLE} -m sphinx_autobuild -E "${CMAKE_CURRENT_SOURCE_DIR}/source" "${OSC_SPHINX_OUTPUT_DIR}"
)

# Create a manually-ran custom target that deploy's the built
# documentation to the live server.
#
# NOTE: This assumes the caller has already globally configured SSH (rsync)
#       access to `docs.opensimcreator.com` in their `.ssh/config` so that
#       passwordless access works (e.g. with an `IdentityFile` entry or similar).
add_custom_target(opensimcreator_docs_deploy
    DEPENDS opensimcreator_docs
    USES_TERMINAL
    COMMAND rsync -avz --delete "${OSC_SPHINX_OUTPUT_DIR}" "docs.opensimcreator.com:/var/www/docs.opensimcreator.com/manual/en/latest"
)

# Install the built documentation into the installation's data directory.
install(
    DIRECTORY "${OSC_SPHINX_OUTPUT_DIR}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/opensimcreator"
    COMPONENT osc_docs
)
