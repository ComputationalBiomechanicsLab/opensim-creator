cmake_minimum_required(VERSION 3.15)

project(osc VERSION 0.5.2 LANGUAGES C CXX)


# -------------- gather user-facing build cache vars ---------------- #

set(
    OSC_USE_CLANG_TIDY OFF
    CACHE BOOL
    "enable/disable running `clang-tidy` (linter) while building"
)
set(
    OSC_USE_IWYU OFF
    CACHE BOOL
    "enable/disable running `iwyu` (include-what-you-use) while building"
)
set(
    OSC_IWYU_PATH "include-what-you-use"
    CACHE STRING
    "path to the `iwyu` executable (if not on the PATH)"
)
set(
    OSC_FORCE_ASSERTS_ENABLED ON
    CACHE BOOL
    "enable OSC's runtime assertions - even if building a release build"
)
set(
    OSC_FORCE_UNDEFINE_NDEBUG OFF
    CACHE BOOL
    "force disable NDEBUG, even if running in Release mode. This enables *a lot* of runtime assertions"
)
set(
    OSC_USE_ASAN OFF
    CACHE BOOL
    "enable address sanitizer (libASAN): useful for spotting memory access violations and leaks"
)
set(
    OSC_DEFAULT_RESOURCE_DIR "../resources"
    CACHE STRING
    "default location of installed runtime resources dir when not specified by user/config"
)
set(
    OSC_REPO_URL "https://github.com/ComputationalBiomechanicsLab/opensim-creator"
    CACHE STRING
    "the internet location of the software's repo. Used by in-app links etc."
)
set(
    OSC_BUILD_DOCS OFF
    CACHE BOOL
    "whether to build the documentation or not (requires sphinx-build is available on the PATH)"
)
set(
    OSC_ORGNAME "cbl"
    CACHE STRING
    "the organization that created the application (affects configuration location)"
)
set(
    OSC_APPNAME "OpenSim Creator"
    CACHE STRING
    "user-facing name of the application"
)
set(
    OSC_PACKAGE_NAME "OpenSimCreator"
    CACHE STRING
    "prefix for the name of the package (e.g. OSC_PACKAGE_NAME-version.exe)"
)
set(
    OSC_SOFTWARE_DESCRIPTION "A standalone GUI for building OpenSim models"
    CACHE STRING
    "desription of what the software does (shows in package installers on Debian, for example)"
)
set(
    OSC_AUTHOR "Adam Kewley"
    CACHE STRING
    "user-facing name of the author (appears in installer, etc.)"
)
set(
    OSC_AUTHOR_EMAIL "contact@adamkewley.com"
    CACHE STRING
    "user-facing email of the author (appears in 'email support', installer, etc.)"
)
set(
    OSC_BUILD_BENCHMARKS OFF
    CACHE BOOL
    "enable/disable building the benchmark suite (handy for development)"
)

# set OSC_BUILD_ID
#
# this is so that errors in the binary can potentially be linked to the source
# that produced the binary later on (user errors, etc.)
if(DEFINED ENV{GITHUB_RUN_ID})
    # build is running inside GitHub Action CI - use that ID
    set(OSC_DEFAULT_BUILD_ID "GITHUB_$ENV{GITHUB_RUN_ID}.$ENV{GITHUB_RUN_NUMBER}")
else()
    # build is probably running on a dev's machine - generate a random ID
    string(RANDOM RANDOM_ID)
    set(OSC_DEFAULT_BUILD_ID "CUSTOM_${RANDOM_ID}")
    unset(RANDOM_ID)
endif()
set(
    OSC_BUILD_ID "${OSC_DEFAULT_BUILD_ID}"
    CACHE STRING
    "unique identifier for this particular build configuration (handy for bug tracing)"
)


# -------------- set/mutate internal vars ---------------- #

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set LINUX
#
# this is based on whether the UNIX variant being used appears to be LINUX
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# set CMAKE_CXX_INCLUDE_WHAT_YOU_USE
#
# this enables CMake's in-built support for iwyu
if(OSC_USE_IWYU)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${OSC_IWYU_PATH};-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;")
endif()

# set CMAKE_CXX_CLANG_TIDY
#
# this enables CMake's in-built support for clang-tidy
if(OSC_USE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
endif()

# handle OSC_FORCE_UNDEFINE_NDEBUG
#
# this unsets the relevant flags globally
if(OSC_FORCE_UNDEFINE_NDEBUG)
    # MSVC
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    # others
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()


# -------------- add/include subdirectories for each part of the project ---------------- #

add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(tests)
if(${OSC_BUILD_DOCS})
    add_subdirectory(docs)
endif()
if(${OSC_BUILD_BENCHMARKS})
    add_subdirectory(benches)
endif()

# install/packaging
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Packaging.cmake)
