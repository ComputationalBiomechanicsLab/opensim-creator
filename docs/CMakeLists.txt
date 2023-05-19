# configure time: ensure the user's build system has 'sphinx-build'
find_program(
    OSC_SPHINX_EXE
    "sphinx-build"
    DOC "Location of sphinx-build, which is used to compile the documentation"
    REQUIRED
)

# install time: run `sphinx-build` to build the docs
install(
    CODE "execute_process(COMMAND ${OSC_SPHINX_EXE} -M html source build WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")"
)

# install time: copy the build docs to the appropriate packaging dir
if(APPLE)

    # APPLE's `DOC` packaging location is (imho) wrong for DMG packaging.
    # It puts the docs outside the DMG in the user-facing drag and drop
    # GUI? So users are presented with 'osc' and 'share', rather than just
    # 'osc' (ewww)
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/html
        DESTINATION osc.app/Contents/MacOS/share/doc
    )

else()
    # on other OSes, just use the CMake `DOC` default, which typically just
    # bundles it in $INSTALL_LOC/share/doc/html
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/html
        TYPE DOC
    )
endif()