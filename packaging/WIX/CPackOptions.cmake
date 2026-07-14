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

# Care: this should be stable between releases so that MSIs
# can auto-update etc. This was generated with:
#
#     python -c "import uuid ; print(uuid.uuid44())"
set(CPACK_WIX_UPGRADE_GUID "010c288b-5fe4-4164-afff-010ab377d349")

# If requested, handle code signing
if(CPACK_OSC_CODESIGN_ENABLED)
    # see: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool
    # and: https://www.files.certum.eu/documents/manual_en/Code-Signing-signing-the-code-using-tools-like-Singtool-and-Jarsigner_v2.3.pdf
    set(CPACK_OSC_CERTIFICATE_SUBJECT          "Open Source Developer, Adam Kewley")  # The name of the subject of the signing certificate
    set(CPACK_OSC_CERTIFICATE_TIMESTAMP_SERVER "http://time.certum.pl"             )  # Specifies the URL of the RFC 3161 time stamp serve
    set(CPACK_OSC_CERTIFICATE_TIMESTAMP_DIGEST "SHA256"                            )  # Digest algorithm used by the RFC 3161 time stamp server (e.g. SHA256)
    set(CPACK_OSC_CODESIGN_DIGEST_ALGORITHM    "SHA256"                            )  # File digest algorithm to use for creating file signature (e.g. SHA256)

    # Specify a script that signs the built binaries (`exe`s, `dll`s) just before CPack creates the installer.
    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/codesign_binaries.cmake")

    # Specify a script that signs the installer that CPack builds.
    set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/codesign_installer.cmake")
endif()
