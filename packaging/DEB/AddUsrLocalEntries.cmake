# CPack script that installs not-in-the-install-tree elements
# into the DEB package.

# Create symlink `/usr/local/bin/osc` --> `/opt/osc/bin/osc`
file(MAKE_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}/usr/local/bin")
file(CREATE_LINK "/opt/osc/bin/osc" "${CPACK_TEMPORARY_DIRECTORY}/usr/local/bin/osc" SYMBOLIC RESULT symlink_res)
if(NOT symlink_res EQUAL 0)
    message(FATAL_ERROR "Failed to create symlink in CPack staging area")
endif()

# Copy `osc.desktop` into `/usr/local/share/applications` (GNOME/desktop integration)
file(MAKE_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}/usr/local/share/applications")
file(
    COPY             "${CPACK_OSC_DEB_PACKAGING_DIR}/osc.desktop"
    DESTINATION      "${CPACK_TEMPORARY_DIRECTORY}/usr/local/share/applications"
    FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE
        GROUP_READ
        WORLD_READ
)
