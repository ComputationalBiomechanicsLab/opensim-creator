# the `.zip` package only installs the application
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)
set(CPACK_COMPONENTS_ALL "osc_app")

if(WIN32)
    # use the naming convention `opensimcreator-$version-windows-$arch.exe` (#975)
    string(TOLOWER "${CPACK_OSC_CMAKE_SYSTEM_PROCESSOR}" _arch_lowercase)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${_arch_lowercase}")
    unset(_arch_lowercase)
endif()

# If requested, handle code signing
if(CPACK_OSC_CODESIGN_ENABLED)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/win_codesign.cmake")
endif()
