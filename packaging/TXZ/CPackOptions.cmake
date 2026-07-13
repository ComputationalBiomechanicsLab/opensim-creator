# The TXZ generator generates two packages base on the installation `COMPONENT`:
#
# - `opensimcreator-application`: The application (portable installer)
# - `opensimcreator-docs`       : Built documentation (archive containing html files etc.)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL "opensimcreator-application" "opensimcreator-docs")
set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)

# Naming: Platform
if(WIN32)
    set(_platform "windows")
elseif(APPLE)
    set(_platform "macos")
elseif(UNIX)
    set(_platform "linux")
else()
    set(_platform "unknown")
endif()

# Naming: Architecture
# - Uses `amd64` rather than `x86_64` to match other packages
string(TOLOWER "${CPACK_OSC_CMAKE_SYSTEM_PROCESSOR}" _arch)
if(_arch STREQUAL "x86_64")
    set(_arch "amd64")
endif()

# Setup per-component package names
set(CPACK_ARCHIVE_OPENSIMCREATOR-APPLICATION_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${_platform}-${_arch}")
set(CPACK_ARCHIVE_OPENSIMCREATOR-DOCS_FILE_NAME        "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-docs")
