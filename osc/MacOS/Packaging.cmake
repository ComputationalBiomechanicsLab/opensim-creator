# Apple OSX packaging:
#
# - Create a DMG (archive) installer that packages the whole application +
#   libraries into a single directory tree that can be copied to
#   /Applications/osc.app

option(OSC_CODESIGN_ENABLED "Enable codesigning the resulting app bundle and DMG file" OFF)
option(OSC_NOTARIZATION_ENABLED "Enable notarizing (xcrun notarytool) the resulting DMG file" OFF)

# set RPATH of `osc` on mac to @executable_path/lib, because dir structure is:
#
#     lib/*.dylib
#     osc (exe)
set_target_properties(osc PROPERTIES INSTALL_RPATH "@executable_path/lib")

# HACK: Simbody has some sort of cmake configuration/module bug that
#       prevents RUNTIME_DEPENDENCIES from correctly resolving it
#       on Mac
file(GLOB_RECURSE shared_libs "${CMAKE_PREFIX_PATH}/*${CMAKE_SHARED_LIBRARY_SUFFIX}")
install(FILES ${shared_libs} DESTINATION osc.app/Contents/MacOS/lib)
install(
    TARGETS osc
    DESTINATION osc.app/Contents/MacOS/
)

# generate `Info.plist` file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/Info.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist"
    @ONLY
)

# install-time: install the generated `Info.plist` file
#
# it's mac-specific XML file that tells Mac OSX about where the
# executable is, what the icon is, etc.
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist"
    DESTINATION osc.app/Contents/
)

# install-time: install a user-facing `osc.toml` config file
#
#     - in contrast to the dev-centric one, this loads resources from the `osc.app/Resources/` dir,
#       which has a known path relative to the osc executable (../Resources/osc.toml)
if(TRUE)
    set(OSC_CONFIG_RESOURCES_DIR ".")  # relative to `osc.toml`
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_macos.toml"
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)

    install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/osc_macos.toml"
            RENAME "osc.toml"
            DESTINATION osc.app/Contents/Resources
    )
endif()

# install-time: copy `resources/` (assets) dir
install(
    DIRECTORY
        "${PROJECT_SOURCE_DIR}/resources/OpenSimCreator"
        "$<$<BOOL:${OSC_BUNDLE_OSCAR_DEMOS}>:${PROJECT_SOURCE_DIR}/resources/oscar_demos>"
    DESTINATION
        osc.app/Contents/Resources
)

# install-time: copy the Mac-specific desktop icon (.icns)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/osc.icns"
    DESTINATION osc.app/Contents/Resources/
)

# use the naming convention `opensimcreator-$version-macos-$arch.dmg`
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

# use a DMG drag-and-drop packaging method
set(CPACK_GENERATOR DragNDrop)

# Handle code signing (notarization is separate - below)
if(OSC_CODESIGN_ENABLED)
    set(OSC_CODESIGN_DEVELOPER_ID "" CACHE STRING "The developer ID string for the codesigning key (e.g. 'Developer ID Application: Some Developer (XYA12398BF)')")

    # Generate required codesigning scripts
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/codesign_osc.app.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.app.cmake"
        @ONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/codesign_osc.dmg.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.dmg.cmake"
        @ONLY
    )

    # Make CPack run the generated `codesign` script on the `osc.app` bundle that CPack
    # temporarily creates *before* it packages the bundle into the dmg...
    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.app.cmake")

    # ... afterwards, make CPack run `codesign` on the resulting DMG file
    set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/codesign_osc.dmg.cmake")
endif()

# If notarizing, assert that codesigning is enabled (you can only notarize signed code)
# and run notarization on the output DMG
if(OSC_NOTARIZATION_ENABLED)
    if(NOT OSC_CODESIGN_ENABLED)
        message(FATAL_ERROR "OSC_NOTARIZATION_ENABLED is ${OSC_NOTARIZATION_ENABLED} but OSC_CODESIGN_ENABLED is ${OSC_CODESIGN_ENABLED}: notarization requires code signing to be enabled")
    endif()

    set(OSC_NOTARIZATION_APPLE_ID "" CACHE STRING "Apple ID of the signer (e.g. 'some@email.com')")
    set(OSC_NOTARIZATION_TEAM_ID "" CACHE STRING "Team ID of the signer (usually, random-looking string at the end of OSC_CODESIGN_DEVELOPER_ID)")
    set(OSC_NOTARIZATION_PASSWORD "" CACHE STRING "App-specific password of the signer (you can create this from your apple ID account)")

    # Generate required notarization script
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/notarize_osc.dmg.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/notarize_osc.dmg.cmake"
        @ONLY
    )

    # Make CPack run the generated notarization script after the DMG is created
    list(APPEND CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/generated/notarize_osc.dmg.cmake")
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
