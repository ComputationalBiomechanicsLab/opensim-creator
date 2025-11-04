# Apple MacOS packaging script
#
# Creates a DMG (archive) installer that packages the whole application into
# a single directory tree that can be dragged to /Applications/osc.app

option(OSC_CODESIGN_ENABLED     "Enable codesigning the resulting app bundle and DMG file"    OFF)
option(OSC_NOTARIZATION_ENABLED "Enable notarizing (xcrun notarytool) the resulting DMG file" OFF)

# Package the software into a DMG file with a "Drag it to the Applications folder" installer.
set(CPACK_GENERATOR DragNDrop)
set(CPACK_DMG_CREATE_APPLICATIONS_LINK ON)
set(CPACK_DMG_VOLUME_NAME "${OSC_LONG_APPNAME} ${PROJECT_VERSION}")

# Install the `osc` bundle target (includes executable and plist).
install(TARGETS osc BUNDLE DESTINATION .)

# Generate + install the user-facing `osc.toml` file
#
# In contrast to the dev-centric one, this loads resources from the `osc.app/Resources/` dir,
# which has a known path relative to the osc executable (../Resources/osc.toml).
set(OSC_CONFIG_RESOURCES_DIR ".")  # relative to `osc.toml`
configure_file(
    "${PROJECT_SOURCE_DIR}/build_resources/osc.toml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_macos.toml"
    @ONLY
)
unset(OSC_CONFIG_RESOURCES_DIR)
install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_macos.toml"
    RENAME      "osc.toml"
    DESTINATION "osc.app/Contents/Resources"
)

# Install the `resources/` (assets) directory.
install(
    DIRECTORY
        "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator"
        "$<$<BOOL:${OSC_BUNDLE_OSCAR_DEMOS}>:${PROJECT_SOURCE_DIR}/resources/oscar_demos>"

    DESTINATION "osc.app/Contents/Resources"
)

# Install the Mac-specific desktop icon (.icns).
install(
    FILES       "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator/textures/logo.icns"
    RENAME      "osc.icns"  # must match `CFBundleIconFile` in `Info.plist
    DESTINATION "osc.app/Contents/Resources"
)

# Ensure the installer is named `opensimcreator-${version}-macos-${arch}.dmg`.
if(CMAKE_OSX_ARCHITECTURES)
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(CPACK_SYSTEM_NAME "macos-amd64")
    else()
        set(CPACK_SYSTEM_NAME "macos-${CMAKE_OSX_ARCHITECTURES}")
    endif()
else()
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} OSC_ARCH_LOWERCASE)
    set(CPACK_SYSTEM_NAME "macos-${OSC_ARCH_LOWERCASE}")
    unset(OSC_ARCH_LOWERCASE)
endif()

# Handle code signing (notarization is separate - below)
if(OSC_CODESIGN_ENABLED)
    set(OSC_CODESIGN_DEVELOPER_ID "" CACHE STRING "The developer ID string for the codesigning key (e.g. 'Developer ID Application: Some Developer (XYA12398BF)'). Get it with `security find-identity -p codesigning -v`")
    if(NOT OSC_CODESIGN_DEVELOPER_ID)
        message(FATAL_ERROR "OSC_CODESIGN_DEVELOPER_ID must be set if OSC_CODESIGN_ENABLED is on. Example value: 'Developer ID Application Some Developer (XYA12398BF)'. Get it with `security find-identity -p codesigning -v`")
    endif()

    # Specify a script that signs the built binaries just before CPack creates the DMG.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/codesign_osc.app.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.app.cmake"
        @ONLY
    )
    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.app.cmake")

    # Specify a script that signs the DMG after CPack creates it.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/codesign_osc.dmg.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.dmg.cmake"
        @ONLY
    )
    list(APPEND CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.dmg.cmake")
endif()

# Handle notarization (requires `OSC_CODESIGN_ENABLED`).
if(OSC_NOTARIZATION_ENABLED)
    if(NOT OSC_CODESIGN_ENABLED)
        message(FATAL_ERROR "OSC_NOTARIZATION_ENABLED is ${OSC_NOTARIZATION_ENABLED} but OSC_CODESIGN_ENABLED is ${OSC_CODESIGN_ENABLED}: notarization requires code signing to be enabled")
    endif()

    set(OSC_NOTARIZATION_APPLE_ID "" CACHE STRING "Apple ID of the signer (e.g. 'some@email.com')")
    set(OSC_NOTARIZATION_TEAM_ID  "" CACHE STRING "Team ID of the signer (usually, random-looking string at the end of OSC_CODESIGN_DEVELOPER_ID)")
    set(OSC_NOTARIZATION_PASSWORD "" CACHE STRING "App-specific password of the signer (you can create this from your apple ID account)")

    # Specify a script that notarizes the DMG after CPack creates it and it's signed (above)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/notarize_osc.dmg.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/notarize_osc.dmg.cmake"
        @ONLY
    )
    list(APPEND CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/notarize_osc.dmg.cmake")
endif()

# CPack variables are fully configured, so include CPack
include(CPack)
