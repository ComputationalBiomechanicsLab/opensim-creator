# Windows packaging
#
# Creates a self-extracting (.exe) installer with NSIS and a portable ZIP
# installer. Requires NSIS.exe, from: https://nsis.sourceforge.io/Download

option(OSC_CODESIGN_ENABLED     "Enable codesigning the built binaries (exes/dlls) and resulting installer"                 OFF)
option(OSC_PACKAGE_PORTABLE_ZIP "Enable creating a portable ZIP package"                                                    ON)
option(OSC_PACKAGE_WITH_NSIS    "Enable using NSIS to package an exe installer (https://nsis.sourceforge.io/Download)"      OFF)
option(OSC_PACKAGE_WITH_WIX     "Enable using WiX to package an MSI installer (https://github.com/wixtoolset/wix/releases)" ON)

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
if(TRUE)
    set(OSC_CONFIG_RESOURCES_DIR "resources")  # relative to `osc.toml`
    configure_file(
        "${PROJECT_SOURCE_DIR}/build_resources/osc.toml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_windows.toml"
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)

    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_windows.toml"
        RENAME "osc.toml"
        DESTINATION "."
    )
endif()

# install the runtime `resources/` (assets) dir
install(
    DIRECTORY
        "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator"
        "$<$<BOOL:${OSC_BUNDLE_OSCAR_DEMOS}>:${PROJECT_SOURCE_DIR}/resources/oscar_demos>"
    DESTINATION "resources"
)

# use the naming convention `opensimcreator-$version-windows-$arch.exe` (#975)
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} OSC_ARCH_LOWERCASE)
set(CPACK_SYSTEM_NAME "windows-${OSC_ARCH_LOWERCASE}")
unset(OSC_ARCH_LOWERCASE)

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${OSC_PACKAGE_NAME}")


if(OSC_PACKAGE_PORTABLE_ZIP)
    list(APPEND CPACK_GENERATOR "ZIP")
endif()

# conditionally use NSIS to package a self-extracting EXE installer
if(OSC_PACKAGE_WITH_NSIS)
    list(APPEND CPACK_GENERATOR "NSIS")

    # set NSIS variables so that the self-extracting installer behaves as expected
    set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator/textures/logo.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "resources/OpenSimCreator/textures/logo.ico")
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
endif()

# conditionally use WiX to package an MSI installer
if(OSC_PACKAGE_WITH_WIX)
    list(APPEND CPACK_GENERATOR "WIX")

    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator/textures/logo.ico")

    set(CPACK_WIX_VERSION 4)  # use the more modern WiX .NET tools

    # set `CPACK_WIX_UPGRADE_CODE` as a GUID derived from the project properties
    if(TRUE)
        string(MD5 OSC_VERSION_MD5_HASH "${PROJECT_NAME}-${PROJECT_VERSION}")
        string(LENGTH "${OSC_VERSION_MD5_HASH}" _hash_len)
        if(_hash_len LESS 32)
            message(FATAL_ERROR "MD5 hash too short: ${OSC_VERSION_MD5_HASH}")
        endif()
        string(SUBSTRING "${OSC_VERSION_MD5_HASH}" 0 8  part1)
        string(SUBSTRING "${OSC_VERSION_MD5_HASH}" 8 4  part2)
        string(SUBSTRING "${OSC_VERSION_MD5_HASH}" 12 4 part3)
        string(SUBSTRING "${OSC_VERSION_MD5_HASH}" 16 4 part4)
        string(SUBSTRING "${OSC_VERSION_MD5_HASH}" 20 12 part5)
        set(OSC_VERSION_GUID "${part1}-${part2}-${part3}-${part4}-${part5}")
        set(CPACK_WIX_UPGRADE_CODE "${OSC_VERSION_GUID}")  # must be stable between releases
        set(CPACK_WIX_UPGRADE_GUID "${OSC_VERSION_GUID}")  # must be stable between releases
        unset(OSC_VERSION_MD5_HASH)
        unset(part1)
        unset(part2)
        unset(part3)
        unset(part4)
        unset(part5)
        unset(OSC_VERSION_GUID)
        message(STATUS "Using deterministic UpgradeCode: ${CPACK_WIX_UPGRADE_CODE}")
    endif()
endif()

# Handle code signing
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
