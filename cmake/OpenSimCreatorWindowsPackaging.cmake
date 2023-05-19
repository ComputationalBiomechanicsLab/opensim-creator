# Windows install/packaging:
#
#     - copy `osc.exe` and all of its RUNTIME_DEPENDENCIES into the `bin/` dir
#
#     - this creates a "fat" install, see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
#
#     - packaging: uses NSIS.exe : get it from https://nsis.sourceforge.io/Download

# package any required system libraries (C/C++ runtimes)
include(InstallRequiredSystemLibraries)

# install osc.exe and all of its RUNTIME_DEPENDENCIES
#
# https://stackoverflow.com/questions/62884439/how-to-use-cmake-file-get-runtime-dependencies-in-an-install-statement
install(
    TARGETS osc
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"  # don't install Windows-provided libs
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"  # don't install Windows-provided libs
)

# install a user-facing `osc.toml` config file
#
#     - in contrast to the dev-centric one, this loads resources from the installation dir,
#       which has a known path relative to the osc executable (../resources)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
    RENAME "osc.toml"
    DESTINATION "."
)

# install the runtime `resources/` (assets) dir
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    DESTINATION "."
)

# use NSIS to package everything into a self-extracting installer
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${OSC_PACKAGE_NAME}")
set(CPACK_GENERATOR NSIS)
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/textures/logo.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "resources/textures/logo.ico")
set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
set(CPACK_NSIS_HELP_LINK ${CPACK_PACKAGE_HOMEPAGE_URL})
set(CPACK_NSIS_CONTACT "${OSC_AUTHOR_EMAIL}")
set(CPACK_NSIS_MODIFY_PATH OFF)  # do not prompt the user to modify the PATH
set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)

# BROKE: set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
#
# the reason it's broke is because CMake has changed something in more-recent
# versions that breaks it. There's a PR about it here:
#
# - https://gitlab.kitware.com/cmake/cmake/-/issues/23001

# BROKE: set(CPACK_NSIS_MUI_FINISHPAGE_RUN osc)
#
# it boots the app with admin privs if the installer is ran with admin privs
# see opensim-creator/#95 (or inkscape's CMake file)

# CPack vars etc. now fully configured, so include it
include(CPack)
