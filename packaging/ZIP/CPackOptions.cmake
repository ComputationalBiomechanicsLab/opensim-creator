# the `.zip` package only installs the application
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL "opensimcreator-application")
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

if(WIN32)
    # use the naming convention `opensimcreator-$version-windows-$arch.exe` (#975)
    string(TOLOWER "${CPACK_OSC_CMAKE_SYSTEM_PROCESSOR}" _arch_lowercase)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${_arch_lowercase}")
    unset(_arch_lowercase)
endif()
