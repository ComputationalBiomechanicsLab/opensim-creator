# Microsoft Installer (MSI) Packaging Script
#
# Creates a self-extracting `.msi` installer with WiX. You can download and
# install WiX3 (e.g. ``wix314.exe``) from https://github.com/wixtoolset/wix3/releases (see
# OSC documentation)

option(OSC_CODESIGN_ENABLED     "Enable codesigning the built binaries (exes/dlls) and resulting installer"                 OFF)
option(OSC_PACKAGE_MSI          "Enable using WiX to package an MSI installer (https://github.com/wixtoolset/wix/releases)" ON)

# use the naming convention `opensimcreator-$version-windows-$arch.exe` (#975)
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _arch_lowercase)
set(CPACK_SYSTEM_NAME "windows-${_arch_lowercase}")
unset(_arch_lowercase)

# Set the install directory for the package (used by NSIS/WiX?)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

# If requested, use WiX to package an MSI installer
if(OSC_PACKAGE_MSI)
    list(APPEND CPACK_GENERATOR "WIX")

    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/osc/osc.ico")

    # set `CPACK_WIX_UPGRADE_CODE` as a GUID derived from the project properties
    if(TRUE)
        string(MD5 _osc_version_md5_hash "${PROJECT_NAME}-${PROJECT_VERSION}")
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
endif()

# If requested, handle code signing
if(OSC_CODESIGN_ENABLED)
    # see: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool
    # and: https://www.files.certum.eu/documents/manual_en/Code-Signing-signing-the-code-using-tools-like-Singtool-and-Jarsigner_v2.3.pdf
    set(OSC_CERTIFICATE_SUBJECT          "Open Source Developer, Adam Kewley" CACHE STRING "The name of the subject of the signing certificate")
    set(OSC_CERTIFICATE_TIMESTAMP_SERVER "http://time.certum.pl"              CACHE STRING "Specifies the URL of the RFC 3161 time stamp serve")
    set(OSC_CERTIFICATE_TIMESTAMP_DIGEST "SHA256"                             CACHE STRING "Digest algorithm used by the RFC 3161 time stamp server (e.g. SHA256)")
    set(OSC_CODESIGN_DIGEST_ALGORITHM    "SHA256"                             CACHE STRING "File digest algorithm to use for creating file signature (e.g. SHA256)")

    # Specify a script that signs the built binaries (`exe`s, `dll`s) just before CPack creates the installer.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Windows/codesign_binaries.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_binaries.cmake"
        @ONLY
    )
    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_binaries.cmake")

    # Specify a script that signs the installer that CPack builds.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Windows/codesign_installer.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_installer.cmake"
        @ONLY
    )
    set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_installer.cmake")
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
