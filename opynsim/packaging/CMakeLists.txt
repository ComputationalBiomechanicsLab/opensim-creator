# This is a very low-level way of packaging `opynsim` into a binary wheel,
# which more closely aligns with CMake and how C++ projects are built, rather
# than the `cibuildtool`, `build`, `setuptools`, etc. stack that python
# projects tend to favor.

# The platform tag can be pulled via python, which is a good starting point:
if(TRUE)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "from packaging import tags; print(next(tags.sys_tags()))"
        OUTPUT_VARIABLE PY_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE PY_ERR
        RESULT_VARIABLE PY_RESULT
    )
    if(NOT PY_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to query Python tags with ${Python_EXECUTABLE}:\n"
            "Exit code: ${PY_RESULT}\n"
            "Error output:\n${PY_ERR}"
        )
    endif()
    string(REPLACE "-" ";" TAG_PARTS "${PY_TAG}")
    list(GET TAG_PARTS 0 PYTHON_TAG)
    list(GET TAG_PARTS 1 ABI_TAG)  # TODO: switch this to any3 if building with a stable ABI
    list(GET TAG_PARTS 2 PLATFORM_TAG)
endif()

# Compute paths within the wheel
set(OPYN_WHEEL_TAG "${PYTHON_TAG}-${ABI_TAG}-${PLATFORM_TAG}")
set(OPYN_WHEEL_FILENAME "${PROJECT_NAME}-${PROJECT_VERSION}-${OPYN_WHEEL_TAG}.whl")
set(OPYN_PACKAGING_DIR "${CMAKE_CURRENT_BINARY_DIR}/${OPYN_WHEEL_FILENAME}")
set(OPYN_PACKAGING_DIST_INFO_DIR "${OPYN_PACKAGING_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}.dist-info")

# Generate metadata files
configure_file(METADATA.in ${OPYN_PACKAGING_DIST_INFO_DIR}/METADATA @ONLY)
configure_file(WHEEL.in ${OPYN_PACKAGING_DIST_INFO_DIR}/WHEEL @ONLY)

# Figure out which python files to copy into the wheel
#
# TODO: this should be a POST_BUILD action, not a configure-time one
file(GLOB_RECURSE PYTHON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../opynsim/*.py")
foreach(f ${PYTHON_FILES})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/../" "${f}")
    set(dest "${OPYN_PACKAGING_DIR}/${rel}")
    get_filename_component(dest_dir "${dest}" DIRECTORY)
    file(MAKE_DIRECTORY "${dest_dir}")
    file(COPY "${f}" DESTINATION "${dest_dir}")
endforeach()

# After building, copy the native PYD into the wheel
add_custom_target(copy_native_module ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:_opynsim_native> "${OPYN_PACKAGING_DIR}/opynsim/"
    DEPENDS _opynsim_native
)

# Finally, call `python -m wheel pack ${OPYN_PACKAGING_DIR}`
add_custom_target(build_wheel ALL
    COMMAND ${Python_EXECUTABLE} -m wheel pack --dest-dir ${CMAKE_BINARY_DIR} ${OPYN_PACKAGING_DIR}
    DEPENDS copy_native_module 
)
