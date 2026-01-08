# Top-Level CPack script (CPACK_PROJECT_CONFIG_FILE) that dispatches
# to a specialized generator configuration, if found

set(_generator_options_file "${CMAKE_CURRENT_LIST_DIR}/${CPACK_GENERATOR}/CPackOptions.cmake")

if(EXISTS "${_generator_options_file}")
    message(STATUS "CPack: Loading custom options for ${CPACK_GENERATOR} from ${_generator_options_file}")
    include("${_generator_options_file}")
else()
    message(STATUS "CPack: No customization found for generator '${CPACK_GENERATOR}' (using defaults)")
endif()

# Clean up local variable
unset(_generator_options_file)
