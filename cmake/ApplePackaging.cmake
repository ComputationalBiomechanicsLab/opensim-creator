# Apple OSX packaging:
#
# - Create a DMG (archive) installer that packages the whole application +
#   libraries into a single directory tree that can be copied to
#   /Applications/osc

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

# install-time: install a user-facing `osc.toml` config file
#
#     - in contrast to the dev-centric one, this loads resources from the installation dir,
#       which has a known path relative to the osc executable (../resources)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
    RENAME "osc.toml"
    DESTINATION osc.app/Contents/MacOS/
)

# install-time: install an `Info.plist` file
#
# it's mac-specific XML file that tells Mac OSX about where the
# executable is, what the icon is, etc.
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/Info.plist"
    DESTINATION osc.app/Contents/
)

# install-time: copy `resources/` (assets) dir
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    DESTINATION osc.app/Contents/MacOS/
)

# install-time: copy the Mac-specific desktop icon (.icns)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/textures/osc.icns"
    DESTINATION osc.app/Contents/Resources/
)

set(CPACK_GENERATOR DragNDrop)

# CPack vars etc. now fully configured, so include it
include(CPack)