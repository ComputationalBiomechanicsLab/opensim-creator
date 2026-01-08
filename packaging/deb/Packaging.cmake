# Debian (`.deb`) Packaging Script
#
# Creates a standalone `.deb` package that should be installable on
# Debian (and derivatives, such as Ubuntu and Linux Mint).

# On Linux (installation), copy `osc.sh` (bootup script) to `/usr/local/bin/`
install(
    PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Debian/osc.sh"
    RENAME "osc"
    DESTINATION /usr/local/bin/
)

# On Linux (installation), copy `osc.desktop` (startup icon) to `/usr/local/share/applications/`
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/Debian/osc.desktop"
    DESTINATION /usr/local/share/applications/
)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libopengl0, libstdc++6")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# CPack vars etc. now fully configured, so include it
include(CPack)
