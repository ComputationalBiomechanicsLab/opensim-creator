# WiX CPack Packaging Options
#
# Creates a self-extracting `.msi` installer with WiX. You can download and
# install WiX3 (e.g. ``wix314.exe``) from https://github.com/wixtoolset/wix3/releases (see
# OSC documentation).

# use the naming convention `opensimcreator-$version-windows-$arch.exe` (#975)
string(TOLOWER ${CPACK_OSC_CMAKE_SYSTEM_PROCESSOR} _arch_lowercase)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${_arch_lowercase}")
unset(_arch_lowercase)

# Set the install directory for the package (used by NSIS/WiX?)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_WIX_PRODUCT_ICON "${CPACK_OSC_PROJECT_SOURCE_DIR}/osc/osc.ico")

# set `CPACK_WIX_UPGRADE_CODE` as a GUID derived from the project properties
if(TRUE)
    string(MD5 _osc_version_md5_hash "${CPACK_OSC_PROJECT_NAME}-${CPACK_OSC_PROJECT_VERSION}")
    string(LENGTH "${_osc_version_md5_hash}" _hash_len)
    if(_hash_len LESS 32)
        message(FATAL_ERROR "MD5 hash too short: ${_osc_version_md5_hash}")
    endif()
    string(SUBSTRING "${_osc_version_md5_hash}" 0  8  _part1)
    string(SUBSTRING "${_osc_version_md5_hash}" 8  4  _part2)
    string(SUBSTRING "${_osc_version_md5_hash}" 12 4  _part3)
    string(SUBSTRING "${_osc_version_md5_hash}" 16 4  _part4)
    string(SUBSTRING "${_osc_version_md5_hash}" 20 12 _part5)
    set(_osc_version_guid "${_part1}-${_part2}-${_part3}-${_part4}-${_part5}")

    set(CPACK_WIX_UPGRADE_CODE "${_osc_version_guid}")  # must be stable between releases
    set(CPACK_WIX_UPGRADE_GUID "${_osc_version_guid}")  # must be stable between releases

    unset(_osc_version_guid)
    unset(_part5)
    unset(_part4)
    unset(_part3)
    unset(_part2)
    unset(_part1)
    unset(_osc_version_md5_hash)
    message(STATUS "Using deterministic UpgradeCode: ${CPACK_WIX_UPGRADE_CODE}")
endif()

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
