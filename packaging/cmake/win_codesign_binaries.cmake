# This is a CPack pre-build script that signs all of the binaries in an install directory
# before they're packaged into the final installer (which must also be signed, but in
# a different script).

# Find `.exe`/`.dll`s that need to be signed
file(GLOB_RECURSE _exe_files "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/*.exe")
file(GLOB_RECURSE _dll_files "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/*.dll")

# Sign each binary
foreach(_binary IN LISTS _exe_files _dll_files)
    message(STATUS "Running signtool on ${_binary}")
    execute_process(
        COMMAND
            signtool sign
            /n  "${CPACK_OSC_CERTIFICATE_SUBJECT}"
            /tr "${CPACK_OSC_CERTIFICATE_TIMESTAMP_SERVER}"
            /td "${CPACK_OSC_CERTIFICATE_TIMESTAMP_DIGEST}"
            /fd "${CPACK_OSC_CODESIGN_DIGEST_ALGORITHM}"
            /v
            "${_binary}"
        RESULT_VARIABLE res
        COMMAND_ECHO STDERR
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "signtool sign failed for file ${_binary}")
    endif()
endforeach()

# Verify that every binary has been signed
foreach(_signed_binary IN LISTS _exe_files _dll_files)
    message(STATUS "Verifying signature of ${_signed_binary}")
    execute_process(
        COMMAND signtool verify /pa /v "${_signed_binary}"
        RESULT_VARIABLE verify_res
        COMMAND_ECHO STDERR
    )
    if(NOT verify_res EQUAL 0)
        message(FATAL_ERROR "Signature verification failed for ${_signed_binary}")
    endif()
endforeach()
