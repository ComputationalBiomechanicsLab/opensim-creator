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

# set RPATH of `osc` on Debian to $ORIGIN/../lib, because dir structure is:
#
#     lib/*.dylib
#     bin/osc (exe)
set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")

# install runtime libraries into the install directory
install(
    TARGETS osc
    RUNTIME_DEPENDENCIES
        DIRECTORIES ${CMAKE_PREFIX_PATH}/lib
        POST_EXCLUDE_REGEXES "^/usr/*" "^/lib/*"  # don't copy system-provided dependencies
)

# install-time: install a user-facing `osc.toml` config file
#
#     - in contrast to the dev-centric one, this loads resources from the installation dir,
#       which has a known path relative to the osc executable (../resources)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/osc_installed_config.toml.in"
    RENAME "osc.toml"
    DESTINATION "."
)

# install-time: copy `resources/` (assets) dir
install(
    DIRECTORY
        "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator"
        "${PROJECT_SOURCE_DIR}/resources/oscar"
        "$<$<BOOL:${OSC_BUNDLE_OSCAR_DEMOS}>:${PROJECT_SOURCE_DIR}/resources/oscar_demos>"
    DESTINATION
        "resources/"
)

install(
    PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Debian/osc.sh"
    RENAME "osc"
    DESTINATION /usr/local/bin/
)

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/Debian/osc.desktop"
    DESTINATION /usr/local/share/applications/
)

# packaging: package installation as a DEB
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libblas3, liblapack3, libgl1, libopengl0, xdg-desktop-portal-gtk")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# CPack vars etc. now fully configured, so include it
include(CPack)
