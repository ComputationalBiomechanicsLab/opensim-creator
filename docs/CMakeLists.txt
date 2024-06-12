# ensure the user's build system has 'sphinx-build'
find_program(
    OSC_SPHINX_EXE
    "sphinx-build"
    DOC "Location of sphinx-build, which is used to compile the documentation"
    REQUIRED
)

# create a build target that builds the documentation using sphinx
add_custom_target(OpenSimCreatorDocumentation
    COMMAND ${OSC_SPHINX_EXE} -M html source build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
