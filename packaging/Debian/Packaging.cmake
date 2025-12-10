# Debian install/packaging:
#
#     - copy osc + libraries into a standard-layout dir
#           bin/  # executables
#           lib/  # libraries (.so)
#           resources/  # arch-independent runtime resources
#
#     - this dir is "done", and could be sent as a ZIP to an end-user
#
#     - **BUT** the way that OpenSim/osc is built assumes the user has all the
#       necessary runtime libraries available
#
#     - so the packaging step creates a .deb file that declares which runtime dependencies
#       a client should have (or automatically acquire via apt)
#
#     - note: the package assumes the user has a compatible OpenGL driver installed, servers
#             can try to "fake" this by running:
#
#           apt-get install libopengl0 libglx0 libglu1-mesa

# install-time: also package any required system libraries
include(InstallRequiredSystemLibraries)

# install-time: install runtime libraries into the install directory
install(
    TARGETS osc
    RUNTIME_DEPENDENCIES
        DIRECTORIES ${CMAKE_PREFIX_PATH}/lib
        POST_EXCLUDE_REGEXES "^/usr/*" "^/lib/*"  # don't copy system-provided dependencies
)

# install-time: install a user-facing `osc.toml` config file
#
# In contrast to the dev-centric one, this loads resources from the installation
# directory, which has a known path relative to the osc executable (../resources)
if(TRUE)
    set(OSC_CONFIG_RESOURCES_DIR "resources")  # relative to `osc.toml`
    configure_file(
        "${PROJECT_SOURCE_DIR}/osc/osc.toml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_debian.toml"
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_debian.toml"
        RENAME "osc.toml"
        DESTINATION "."
    )
endif()

# install-time: copy `resources/` (assets) dir
install(
    DIRECTORY
        "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator"
        "$<$<BOOL:${OSC_BUNDLE_OSCAR_DEMOS}>:${PROJECT_SOURCE_DIR}/resources/oscar_demos>"
    DESTINATION
        "resources/"
)

# install-time: copy `osc.sh` (bootup script) to `/usr/local/bin/`
install(
    PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Debian/osc.sh"
    RENAME "osc"
    DESTINATION /usr/local/bin/
)

# install-time: copy `osc.desktop` (startup icon) to `/usr/local/share/applications/`
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/Debian/osc.desktop"
    DESTINATION /usr/local/share/applications/
)

# Package installation as a DEB package
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libopengl0")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# CPack vars etc. now fully configured, so include it
include(CPack)
