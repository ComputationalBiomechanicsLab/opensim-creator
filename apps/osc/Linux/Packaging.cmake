# Linux install/packaging:
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

# set RPATH of `osc` on Linux to $ORIGIN/../lib, because dir structure is:
#
#     lib/*.dylib
#     bin/osc (exe)
set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")

# HACK: Simbody has some sort of cmake configuration/module bug that
#       prevents RUNTIME_DEPENDENCIES from correctly resolving it
#       on Linux
find_package(Simbody REQUIRED CONFIG)
install(
    TARGETS osc
    RUNTIME_DEPENDENCIES
        DIRECTORIES $<TARGET_FILE_DIR:SimTKcommon>
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
    DIRECTORY "${PROJECT_SOURCE_DIR}/resources"
    DESTINATION
    "."
)

# packaging: package installation as a DEB
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libblas3, liblapack3, libstdc++6")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# packaging: configure a script that creates a symlink /usr/local/bin/osc --> /opt/osc/bin/osc
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Linux/postinst.in" "postinst" @ONLY)

# packaging: configure a script that destroys the above symlink on uninstall
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Linux/postrm.in" "postrm" @ONLY)

# packaging: tell debian packager to use the scripts for postinst and postrm actions
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")

# CPack vars etc. now fully configured, so include it
include(CPack)
