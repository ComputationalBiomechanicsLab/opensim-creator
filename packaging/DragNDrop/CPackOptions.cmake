# DragNDrop CPack Packaging Options
#
# Creates a standalone `.dmg` that packages the whole application into a
# single directory tree that can be dragged to `/Applications` on MacOS.

set(CPACK_DMG_CREATE_APPLICATIONS_LINK ON)
set(CPACK_DMG_VOLUME_NAME "${CPACK_OSC_LONG_APPNAME} ${CPACK_OSC_PROJECT_VERSION}")

# Ensure the package is named `opensimcreator-${version}-macos-${arch}.dmg`.
if(CPACK_OSC_CMAKE_OSX_ARCHITECTURES)
    if(CPACK_OSC_CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(_suffix "macos-amd64")
    else()
        set(_suffix "macos-${CPACK_OSC_CMAKE_OSX_ARCHITECTURES}")
    endif()
else()
    string(TOLOWER ${CPACK_OSC_CMAKE_SYSTEM_PROCESSOR} _arch_lowercase)
    set(_suffix "macos-${_arch_lowercase}")
    unset(_arch_lowercase)
endif()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${_suffix}")
unset(_suffix)

# Handle code signing (notarization is separate - below)
if(CPACK_OSC_CODESIGN_ENABLED)
    set(CPACK_OSC_CODESIGN_DEVELOPER_ID "$ENV{OSC_CODESIGN_DEVELOPER_ID}" CACHE STRING "The developer ID string for the codesigning key (e.g. 'Developer ID Application: Some Developer (XYA12398BF)'). Get it with `security find-identity -p codesigning -v`")
    if(NOT CPACK_OSC_CODESIGN_DEVELOPER_ID)
        message(FATAL_ERROR "The environment variable OSC_CODESIGN_DEVELOPER_ID must be set if OSC_CODESIGN_ENABLED is ON this is probably stored in the developer's password vault")
    endif()

    # Specify a script that signs the application bundle (`.app`) before
    # CPack builds the `.dmg`.
    list(APPEND CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/codesign_app.cmake")

    # Specify a script that signs the DMG after CPack builds it.
    list(APPEND CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/codesign_dmg.cmake")
endif()

# Handle notarization
if(CPACK_OSC_NOTARIZATION_ENABLED)
    if(NOT CPACK_OSC_CODESIGN_ENABLED)
        message(FATAL_ERROR "OSC_NOTARIZATION_ENABLED is ${CPACK_OSC_NOTARIZATION_ENABLED} but OSC_CODESIGN_ENABLED is ${CPACK_OSC_CODESIGN_ENABLED}: notarization requires code signing to be enabled")
    endif()

    set(CPACK_OSC_NOTARIZATION_APPLE_ID "$ENV{OSC_NOTARIZATION_APPLE_ID}")  # Apple ID of the signer (e.g. 'some@email.com')
    set(CPACK_OSC_NOTARIZATION_TEAM_ID  "$ENV{OSC_NOTARIZATION_TEAM_ID}")   # Team ID of the signer (usually, random-looking string at the end of OSC_CODESIGN_DEVELOPER_ID)
    set(CPACK_OSC_NOTARIZATION_PASSWORD "$ENV{OSC_NOTARIZATION_PASSWORD}")  # App-specific password of the signer (you can create this from your apple ID account)

    if(NOT CPACK_OSC_NOTARIZATION_APPLE_ID)
        message(FATAL_ERROR "OSC_NOTARIZATION_APPLE_ID must be set if OSC_NOTARIZATION_ENABLED is on")
    endif()
    if(NOT CPACK_OSC_NOTARIZATION_TEAM_ID)
        message(FATAL_ERROR "OSC_NOTARIZATION_TEAM_ID must be set if OSC_NOTARIZATION_ENABLED is on")
    endif()
    if(NOT CPACK_OSC_NOTARIZATION_PASSWORD)
        message(FATAL_ERROR "OSC_NOTARIZATION_PASSWORD must be set if OSC_NOTARIZATION_ENABLE is on")
    endif()

    # Specify a script that notarizes the DMG after CPack builds it.
    list(APPEND CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/notarize_dmg.cmake")
endif()
