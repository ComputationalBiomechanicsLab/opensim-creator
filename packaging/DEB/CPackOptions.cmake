# DEB CPack Packaging Options
#
# Creates a standalone `.deb` package that should be installable on
# Debian (and derivatives, such as Ubuntu and Linux Mint).

# the `.deb` package only installs the application
set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_COMPONENTS_ALL "osc_app")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libopengl0")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# Install the application into `/opt/osc` (we're not distributing it via `apt`)
set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)

# Use a script to inject stuff into `/usr/local`, so that the application is
# visible (`/opt` isn't on the PATH by default).
set(CPACK_OSC_DEB_PACKAGING_DIR "${CMAKE_CURRENT_LIST_DIR}")  # Forwarded variable
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/AddUsrLocalEntries.cmake")

# Compress the archive as much as possible
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
