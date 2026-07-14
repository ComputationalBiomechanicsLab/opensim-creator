# WiX CPack Packaging Options
#
# Creates a self-extracting `.msi` installer with WiX. You can download and
# install WiX3 (e.g. ``wix314.exe``) from https://github.com/wixtoolset/wix3/releases (see
# OSC documentation).

# the `.msi` package only installs the application (no docs/libs)
set(CPACK_WIX_COMPONENT_INSTALL OFF)
set(CPACK_COMPONENTS_ALL "osc_app")
set(CPACK_WIX_UI_REF "WixUI_InstallDir")  # Disable component selection dialog in installer

# use the naming convention `opensimcreator-$version-windows-$arch.exe` (#975)
string(TOLOWER ${CPACK_OSC_CMAKE_SYSTEM_PROCESSOR} _arch_lowercase)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${_arch_lowercase}")
unset(_arch_lowercase)

# Set the install directory for the package (used by NSIS/WiX?)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_WIX_PRODUCT_ICON "${CPACK_OSC_PROJECT_SOURCE_DIR}/osc/osc.ico")
set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_LIST_DIR}/ui_banner.bmp")
set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_LIST_DIR}/ui_dialog.bmp")

# This should be stable between releases. Generated with:
#     python -c "import uuid ; print(uuid.uuid44())"
set(CPACK_WIX_UPGRADE_GUID "010c288b-5fe4-4164-afff-010ab377d349")

# If requested, handle code signing
if(CPACK_OSC_CODESIGN_ENABLED)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/win_codesign.cmake")
endif()
