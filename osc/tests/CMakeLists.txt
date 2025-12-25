if(APPLE)
    find_package(Python REQUIRED COMPONENTS Interpreter)

    add_test(
            NAME test_macos-dependencies
            COMMAND
            "${Python_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_macos-dependencies.py"
            "$<TARGET_FILE:osc>"
    )

    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        add_test(
                NAME test_macos-uses-sdk
                COMMAND
                "${Python_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_macos-uses-sdk.py"
                "${CMAKE_OSX_DEPLOYMENT_TARGET}"
                "$<TARGET_FILE:osc>"
        )
    endif()
endif()