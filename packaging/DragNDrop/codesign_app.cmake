# CPack script that codesigns the application bundle

set(OSC_STAGING_DIR "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/ALL_IN_ONE")

execute_process(
    COMMAND codesign
    --force
    --deep
    --verbose
    --options runtime
    --sign "${CPACK_OSC_CODESIGN_DEVELOPER_ID}"
    "${OSC_STAGING_DIR}/OpenSim Creator.app"
    RESULT_VARIABLE res
    COMMAND_ECHO STDERR
)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "codesign failed!")
endif()
execute_process(
    COMMAND codesign --verify --verbose=4 "${OSC_STAGING_DIR}/OpenSim Creator.app"
    RESULT_VARIABLE res
    COMMAND_ECHO STDERR
)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "codesign --verify failed!")
endif()
