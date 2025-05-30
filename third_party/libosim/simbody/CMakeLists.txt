#-------------------------------------------------------------------------------
# Simbody
#
# This is the master CMake file that coordinates
# the build of Simbody. There are four steps:
#    (1) Get files needed for particular platform
#    (2) Build SimTKcommon library
#    (3) Build SimTKmath library
#    (4) Build SimTKsimbody library
#    (5) Build examples
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.21) # Needed for install(RUNTIME_DEPENDENCY_SET ...)

project(Simbody VERSION 3.8.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

## List of Simbody build configuration options:
#-------------------------------------------------------------------------------

### Major build options:
#--------------------------------
# SIMBODY_BUILD_SHARED_LIBS
# BUILD_VISUALIZER
# BUILD_EXAMPLES
# BUILD_TESTING
# INSTALL_DOCS

# CMake doesn't initialize BUILD_SHARED_LIBS and add_library calls without a library type
# default to STATIC if BUILD_SHARED_LIBS isn't defined. BUT, we don't want to pollute/affect
# the global cache, so we define a project-prefixed variable (which defaults to the cache'd
# BUILD_SHARED_LIBS if defined), and then set BUILD_SHARED_LIBS before each target definition.
set(shared_lib_default ON)
if(DEFINED BUILD_SHARED_LIBS)
    set(shared_lib_default ${BUILD_SHARED_LIBS})
endif()
option(SIMBODY_BUILD_SHARED_LIBS
    "Build dynamic (shared) Simbody libraries. Defaults to ON or the value of BUILD_SHARED_LIBS if defined."
    ${shared_lib_default})

# TODO: Remove after this version cycle?
option(BUILD_DYNAMIC_LIBRARIES
    "[DEPRECATED] Build dynamic (shared) Simbody libraries."
    OFF)
mark_as_advanced(BUILD_DYNAMIC_LIBRARIES)
if(BUILD_DYNAMIC_LIBRARIES)
    message(DEPRECATION "The BUILD_DYNAMIC_LIBRARIES build option is deprecated and will be \
ignored in future versions of Simbody. Please set SIMBODY_BUILD_SHARED_LIBS=ON instead.")
    if(NOT SIMBODY_BUILD_SHARED_LIBS)
        set(SIMBODY_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        unset(BUILD_DYNAMIC_LIBRARIES CACHE)
    endif()
endif()

# TODO: Remove after this version cycle?
option(BUILD_STATIC_LIBRARIES
    "[DEPRECATED] Build static Simbody libraries."
    OFF)
mark_as_advanced(BUILD_STATIC_LIBRARIES)
if(BUILD_STATIC_LIBRARIES)
    message(DEPRECATION "The BUILD_STATIC_LIBRARIES build option is deprecated and will be \
ignored in future versions of Simbody. Please set SIMBODY_BUILD_SHARED_LIBS=OFF instead.")
    if(SIMBODY_BUILD_SHARED_LIBS)
        set(SIMBODY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        unset(BUILD_STATIC_LIBRARIES CACHE)
    endif()
endif()
if(BUILD_DYNAMIC_LIBRARIES AND BUILD_STATIC_LIBRARIES)
    message(FATAL_ERROR "Simultaneous static and shared library builds are no longer supported. \
Please see the README for instructions to produce similar builds.")
endif()

option(BUILD_VISUALIZER
    "Build and install the Simbody visualizer."
    ON)

option(BUILD_EXAMPLES
    "Build and install Simbody examples."
    ON)

# Caution: this variable is automatically created by the CMake
# enable_testing() command, but we'll take it over here for
# our own purposes too.
set(BUILD_TESTING ON CACHE BOOL
    "Build tests for Simbody libraries.")

option(INSTALL_DOCS
    "Build and install Simbody documentation."
    ON)

### Library name related options:
#--------------------------------
# BUILD_USING_NAMESPACE
# BUILD_VERSIONED_LIBRARIES
# SIMBODY_STATIC_LIBRARIES_POSTFIX

set(BUILD_USING_NAMESPACE "" CACHE STRING
    "Add a prefix to library filenames. (e.g. `BUILD_USING_NAMESPACE=ns_` would produce \
libraries like 'libns_SimTKmath.so' on Unix).")
mark_as_advanced(BUILD_USING_NAMESPACE)

option(BUILD_VERSIONED_LIBRARIES
    "Add a version suffix to Simbody libraries (e.g. library names like 'libSimTKMath_3.8.so')"
    OFF)
mark_as_advanced(BUILD_VERSIONED_LIBRARIES)

# TODO: Remove after this version cycle?
option(BUILD_UNVERSIONED_LIBRARIES
    "[DEPRECATED] Opposite of BUILD_VERSIONED_LIBRARIES. Will be ignored in future versions of Simbody"
    OFF)
mark_as_advanced(BUILD_UNVERSIONED_LIBRARIES)
if(BUILD_UNVERSIONED_LIBRARIES)
    message(DEPRECATION "The BUILD_UNVERSIONED_LIBRARIES build option is deprecated and will be \
ignored in future versions of Simbody. Please set BUILD_VERSIONED_LIBRARIES=OFF instead.")
    set(BUILD_VERSIONED_LIBRARIES OFF CACHE BOOL "" FORCE)
    unset(BUILD_UNVERSIONED_LIBRARIES CACHE)
endif()

if(BUILD_VERSIONED_LIBRARIES AND BUILD_UNVERSIONED_LIBRARIES)
    message(FATAL_ERROR "Simultaneous versioned and unversioned library builds are no longer supported. \
Please see the README for instructions to produce similar builds.")
endif()

set(SIMBODY_STATIC_LIBRARIES_POSTFIX "" CACHE STRING
    "Add a postfix to static Simbody libraries (e.g. library names like 'libSimTKMath_static.dll'). \
Typically only useful to distinguish static/dynamic libaries on DLL platforms")
mark_as_advanced(SIMBODY_STATIC_LIBRARIES_POSTFIX)

### Advanced dev options:
#------------------------

option(SIMBODY_COVERAGE
    "Generate code coverage files to assess test coverage (ignored for MSVC compilers)"
    OFF)
mark_as_advanced(SIMBODY_COVERAGE)

option(WINDOWS_USE_EXTERNAL_LIBS
    "Link against system BLAS/LAPACK on Windows. When set to OFF/FALSE (default), Simbody \
will use vendored BLAS/LAPACK libraries."
    OFF)
mark_as_advanced(WINDOWS_USE_EXTERNAL_LIBS)

# Ultimately, BUILD_USING_OTHER_LAPACK is given to target_link_libraries;
# see https://cmake.org/cmake/help/latest/command/target_link_libraries.html#overview
# for a more complete description of different valid incantations for BUILD_USING_OTHER_LAPACK
set(BUILD_USING_OTHER_LAPACK "" CACHE STRING
    "Use non-default BLAS/LAPACK libraries. List library names (no extensions) \
separated by semicolons (full paths or paths that are on the (DY)LD_LIBRARY_PATH \
(UNIX) or PATH (Windows)). (e.g. BUILD_USING_OTHER_LAPACK='mkl_intel_c_dll;\
mkl_sequential_dll;mkl_core_dll' to use Intel MKL on Windows).")
mark_as_advanced(BUILD_USING_OTHER_LAPACK)

### Deprecated and ignored build options:
#----------------------------------------

if(DEFINED BUILD_TESTS_AND_EXAMPLES_STATIC)
    message(DEPRECATION "The BUILD_TESTS_AND_EXAMPLES_STATIC is deprecated. Only one set \
of Simbody libraries are built now (shared or static is set by SIMBODY_BUILD_SHARED_LIBS), \
and examples and tests are linked against these libraries.")
    unset(BUILD_TESTS_AND_EXAMPLES_STATIC CACHE)
endif()

if(DEFINED BUILD_TESTS_AND_EXAMPLES_SHARED)
    message(DEPRECATION "The BUILD_TESTS_AND_EXAMPLES_SHARED is deprecated. Only one set \
of Simbody libraries are built now (shared or static is set by SIMBODY_BUILD_SHARED_LIBS), \
and examples and tests are linked against these libraries.")
    unset(BUILD_TESTS_AND_EXAMPLES_SHARED CACHE)
endif()

#-------------------------------------------------------------------------------

# Check compiler version
if(MSVC)
    if(MSVC_VERSION LESS 1800 OR MSVC_VERSION EQUAL 1800)
        message(FATAL_ERROR "\nMSVC does not support C++ 2011 features, for "
                            "example 'constexpr'. Update to at least MSVC 2015 "
                            "or use a MinGW version on Windows.\nLook at the "
                            "README.md for more information.\nIf you have the"
                            " 'Visual C++ Compiler Nov 2013 CTP (CTP_Nov2013)'"
                            " comment this test and configure normally.")
    endif()
    if(SIMBODY_COVERAGE)
        message(FATAL_ERROR "Code coverage is not possible with MSVC.")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(SIMBODY_REQUIRED_GCC_VERSION 4.9.0)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${SIMBODY_REQUIRED_GCC_VERSION})
        message(FATAL_ERROR "GNU GCC/G++ version is too old to compile Simbody.\n"
                            "Simbody requires at least version : "
                            "${SIMBODY_REQUIRED_GCC_VERSION}")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(SIMBODY_REQUIRED_CLANG_VERSION 3.4)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${SIMBODY_REQUIRED_CLANG_VERSION})
        message(FATAL_ERROR "Clang version is too old to compile Simbody.\n"
                            "Simbody requires at least version : "
                            "${SIMBODY_REQUIRED_CLANG_VERSION}")
    endif()
endif()

if((${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" OR
   ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") AND SIMBODY_COVERAGE)
    find_package(codecov)
endif()

set(SIMBODY_SONAME_VERSION
    "${Simbody_VERSION_MAJOR}.${Simbody_VERSION_MINOR}"
    CACHE INTERNAL
    "Soname version; appended to names of shared libs"
    )

# At this point CMake will have set CMAKE_INSTALL_PREFIX to /usr/local on Linux
# or appropriate ProgramFiles directory on Windows, for example
# C:/Program Files/Simbody, C:/Program Files (x86)/Simbody, or the local
# language equivalent.

if(WIN32)
    set(CMAKE_INSTALL_DOCDIR doc CACHE PATH "documentation root (DATAROOTDIR/doc/PROJECT_NAME)")
else()
    # Redefine DOCDIR to use the project name in lowercase to avoid
    # problems with some platforms: NTFS on Win, XFS or JFS variants
    set(CMAKE_INSTALL_DOCDIR share/doc/simbody CACHE PATH "documentation root (DATAROOTDIR/doc/PROJECT_NAME)")
endif()

# Include GNUInstallDirs to get canonical paths
include(GNUInstallDirs)

# Put executables and libraries (shared and static) in the Simbody project binary folder.
# This is most (only?) relevant on Windows, where libraries need to be in the same folder as the .exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Ensure that debug libraries have "_d" appended to their names.
set(CMAKE_DEBUG_POSTFIX "_d")

set(NS)
if(BUILD_USING_NAMESPACE)
    set(NS "${BUILD_USING_NAMESPACE}_")
endif()

set(VN)
if(BUILD_VERSIONED_LIBRARIES)
    # This is the suffix if we're building and depending on versioned libraries.
    set(VN "_${Simbody_VERSION}")
endif()

#
# These are the output names of all the libraries we may generate.
#
set(SimTKSIMBODY_LIBRARY_NAME ${NS}SimTKsimbody${VN} CACHE INTERNAL
    "Base name of the SimTKsimbody library"
    )
set(SimTKMATH_LIBRARY_NAME ${NS}SimTKmath${VN} CACHE INTERNAL
    "Base name of the SimTKmath library"
    )
set(SimTKCOMMON_LIBRARY_NAME ${NS}SimTKcommon${VN} CACHE INTERNAL
    "Base name of the SimTKcommon library"
    )

# In addition to the platform name we need to know the Application Binary
# Interface (ABI) we're building for. Currently that is either x86, meaning
# 32 bit Intel instruction set, or x64 for 64 bit Intel instruction set.

if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(PLATFORM_ABI x64)
else()
    set(PLATFORM_ABI x86)
endif()


if(NOT MSVC AND NOT XCODE AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Debug, RelWithDebInfo (recommended), or Release build"
        FORCE)
endif()

# RPATH
# -----
set(SIMBODY_USE_INSTALL_RPATH FALSE)
if(APPLE)
    # CMake 2.8.12 introduced the ability to set RPATH for shared libraries on
    # OSX. This helps executables find the libraries they depend on without
    # having to set the DYLD_LIBRARY_PATH environment variable.

    # Consider a library libfoo.dyld and an executable bar.
    # On OSX, libraries have an "install name" that, when linking, is copied
    # into the target (e.g., bar). The "install name" can be the full path to
    # libfoo.dylib, in which case bar will have no trouble finding libfoo.dylib
    # at runtime (since it has the full path to it). This doesn't work if you
    # want to be able to relocate your project. Therefore, it's possible to use
    # special tokens in the "install name" that are evaluated separately. The
    # token "@exectuable_path" is evaluated (at run time) to the full path of
    # the exectuable (e.g., bar) that is trying to load libfoo.dylib. An even
    # more flexible token is "@rpath", which is evaluated to a path (called
    # RPATH) that can be baked into the executable just after compiling or any
    # time before running the executable (using the executable
    # "install_name_tool"). The RPATH stored in executables can also contain
    # "@executable_path", etc. On Linux, rather than "@executable_path", we 
    # must use "$ORIGIN" to find the location of the executable.

    if(APPLE)
        # This variable gets used when configuring the Info.plist for
        # simbody-visualizer.app; see cmake/MacOSXBundleInfo.plist.in.
        set(MACOSX_BUNDLE_HIGH_RESOLUTION_CAPABLE "false")

        # Set the install name of libraries to contain "@rpath".
        # This allows clients of our libraries to point to them however they wish.
        set(CMAKE_MACOSX_RPATH ON)
    endif() 

    # We only need to set RPATH in executables if the libraries are installed
    # into directories that are not already searched by the linker.
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
    # CMake sets isSystemDir to -1 if the lib dir is NOT a system lib dir.
    if("${isSystemDir}" STREQUAL "-1")
        # This variable is used later on to toggle if RPATH should be set for
        # specific targets.
        set(SIMBODY_USE_INSTALL_RPATH TRUE)
    endif()
endif()


## This can be set to a different value by the person running CMake.
set(BUILD_INST_SET ""
    CACHE STRING
    "CPU instruction level compiler is permitted to use (default: let compiler decide).")
mark_as_advanced( BUILD_INST_SET )

## Choose the maximum level of x86 instruction set that the compiler is
## allowed to use.
if(BUILD_INST_SET)
    set(inst_set_to_use ${BUILD_INST_SET})
else()
    set(inst_set_to_use)
endif()

## When building in any of the Release modes, tell gcc/clang to use
## not-quite most agressive optimization.  Here we
## are specifying *all* of the Release flags, overriding CMake's defaults.
## Watch out for optimizer bugs in particular gcc versions!
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" OR
   ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

    if(inst_set_to_use)
        string(TOLOWER ${inst_set_to_use} GCC_INST_SET)
        set(GCC_INST_SET "-m${GCC_INST_SET}")
    else()
        set(GCC_INST_SET)
    endif()

    # Unrolling fixed-count loops was a useful optimization for Simmatrix
    # in earlier gcc versions.
    # Doesn't have a big effect for current compiler crop and may be
    # pushing our luck with optimizer bugs. So let the compilers decide
    # how to handle loops instead.
    ##set(GCC_OPT_ENABLE "-funroll-loops")

    # If you know of optimization bugs that affect Simbody in particular
    # gcc versions, this is the place to turn off those optimizations.
    set(GCC_OPT_DISABLE)

    # C++
    set(BUILD_CXX_FLAGS_DEBUG          "-g ${GCC_INST_SET}")
    set(BUILD_CXX_FLAGS_RELEASE
      "-DNDEBUG -O2 ${GCC_OPT_ENABLE} ${GCC_OPT_DISABLE} ${GCC_INST_SET}")
    set(BUILD_CXX_FLAGS_RELWITHDEBINFO
    "-DNDEBUG -O2 -g ${GCC_OPT_ENABLE} ${GCC_OPT_DISABLE} ${GCC_INST_SET}")
    set(BUILD_CXX_FLAGS_MINSIZEREL     "-DNDEBUG -Os ${GCC_INST_SET}")

    # C
    set(BUILD_C_FLAGS_DEBUG            "-g ${GCC_INST_SET}")
    set(BUILD_C_FLAGS_RELEASE
      "-DNDEBUG -O2 ${GCC_OPT_ENABLE} ${GCC_OPT_DISABLE} ${GCC_INST_SET}")
    set(BUILD_C_FLAGS_RELWITHDEBINFO
    "-DNDEBUG -O2 -g ${GCC_OPT_ENABLE} ${GCC_OPT_DISABLE} ${GCC_INST_SET}")
    set(BUILD_C_FLAGS_MINSIZEREL       "-DNDEBUG -Os ${GCC_INST_SET}")

    # C++
    set(CMAKE_CXX_FLAGS_DEBUG ${BUILD_CXX_FLAGS_DEBUG}
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE ${BUILD_CXX_FLAGS_RELEASE}
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${BUILD_CXX_FLAGS_RELWITHDEBINFO}
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_CXX_FLAGS_MINSIZEREL ${BUILD_CXX_FLAGS_MINSIZEREL}
        CACHE STRING "Control using BUILD_CXX" FORCE)

    # C
    set(CMAKE_C_FLAGS_DEBUG ${BUILD_C_FLAGS_DEBUG}
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_C_FLAGS_RELEASE ${BUILD_C_FLAGS_RELEASE}
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO ${BUILD_C_FLAGS_RELWITHDEBINFO}
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_C_FLAGS_MINSIZEREL ${BUILD_C_FLAGS_MINSIZEREL}
        CACHE STRING "Control using BUILD_CXX" FORCE)
elseif(MSVC)

    ## When building in any of the Release modes, tell VC++ cl compiler to use
    ## intrinsics (i.e. sqrt instruction rather than sqrt subroutine) with
    ## flag /Oi.
    if(inst_set_to_use)
        string(TOUPPER ${inst_set_to_use} CL_INST_SET)
        set(CL_INST_SET "/arch:${CL_INST_SET}")
    else()
        set(CL_INST_SET)
    endif()

    ## C++
    set(BUILD_CXX_FLAGS_DEBUG
    "/D _DEBUG /MDd /Od /Ob0 /RTC1 /Zi /GS- ${CL_INST_SET}") 
    set(BUILD_CXX_FLAGS_RELEASE
    "/D NDEBUG /MD  /O2 /Ob2 /Oi /GS- ${CL_INST_SET}")
    set(BUILD_CXX_FLAGS_RELWITHDEBINFO
    "/D NDEBUG /MD  /O2 /Ob2 /Oi /Zi /GS- ${CL_INST_SET}")
    set(BUILD_CXX_FLAGS_MINSIZEREL
    "/D NDEBUG /MD  /O1 /Ob1 /Oi /GS- ${CL_INST_SET}")

    ## C
    set(BUILD_C_FLAGS_DEBUG
    "/D _DEBUG /MDd /Od /Ob0 /RTC1 /Zi /GS- ${CL_INST_SET}")
    set(BUILD_C_FLAGS_RELEASE
    "/D NDEBUG /MD  /O2 /Ob2 /Oi /GS- ${CL_INST_SET}")
    set(BUILD_C_FLAGS_RELWITHDEBINFO
    "/D NDEBUG /MD  /O2 /Ob2 /Oi /Zi /GS- ${CL_INST_SET}")
    set(BUILD_C_FLAGS_MINSIZEREL
    "/D NDEBUG /MD  /O1 /Ob1 /Oi /GS- ${CL_INST_SET}")

    ## C++
    set(CMAKE_CXX_FLAGS_DEBUG "${BUILD_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_DEBUG}"
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${BUILD_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE}"
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${BUILD_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}"
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${BUILD_CXX_FLAGS_MINSIZEREL} ${CMAKE_CXX_FLAGS_MINSIZEREL}"
        CACHE STRING "Control using BUILD_CXX" FORCE)

    ## C
    set(CMAKE_C_FLAGS_DEBUG "${BUILD_C_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG}"
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "${BUILD_C_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}"
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${BUILD_C_FLAGS_RELWITHDEBINFO} ${CMAKE_C_FLAGS_RELWITHDEBINFO}"
        CACHE STRING "Control using BUILD_CXX" FORCE)
    set(CMAKE_C_FLAGS_MINSIZEREL "${BUILD_C_FLAGS_MINSIZEREL} ${CMAKE_C_FLAGS_MINSIZEREL}"
        CACHE STRING "Control using BUILD_CXX" FORCE)

endif()

#
# Allow automated build and dashboard.
#

if(BUILD_TESTING)
    include(CTest) # automatically calls enable_testing()
    ## When in Debug mode and running valgrind, some of the test
    ## cases take longer than the default 1500 seconds.
    set(CTEST_TESTING_TIMEOUT 7200)
endif()

# These are used in Doxyfile.in and SimbodyConfig.cmake.in.
set(SIMBODY_INSTALL_DOXYGENDIR   "${CMAKE_INSTALL_DOCDIR}/api")
set(SIMBODY_DOXYGEN_TAGFILE_NAME "SimbodyDoxygenTagfile")
if( INSTALL_DOCS )
    add_subdirectory(doc)
endif()

# Specify where visualizer should be installed. This needs to be in the
# root CMakeLists.txt so the cmake config file can see this value.
#
# Also specify where include files are installed.
if(WIN32)
    # Install visualizer to bin, since it needs to be co-located with dll's
    set(SIMBODY_VISUALIZER_REL_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
    # Install include files into base include folder since it's a sandbox
    set(SIMBODY_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
else()
    # Visualizer is not intended to be a user executable. Proper place is
    # inside the lib directory
    set(SIMBODY_VISUALIZER_REL_INSTALL_DIR ${CMAKE_INSTALL_LIBEXECDIR}/simbody)
    # Install include files in simbody subfolder to avoid polluting the
    # global build folder
    set(SIMBODY_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/simbody)
endif()
set(SIMBODY_VISUALIZER_INSTALL_DIR
    ${CMAKE_INSTALL_PREFIX}/${SIMBODY_VISUALIZER_REL_INSTALL_DIR})

add_subdirectory( Platform )
# PLATFORM_INCLUDE_DIRECTORIES now set; 'blas' and 'lapack' targets defined on Windows

if(BUILD_USING_OTHER_LAPACK)
    set(LAPACK_BEING_USED ${BUILD_USING_OTHER_LAPACK} CACHE INTERNAL
        "The BLAS/LAPACK libraries that will be linked against by the Simbody libraries.")

    message(CHECK_START "Trying to compile with requested BLAS/LAPACK libraries")
else()
    # Determine which math libraries to use for this platform.
    # Intel MKL: mkl_intel_c_dll;mkl_sequential_dll;mkl_core_dll
    set(BUILD_USING_OTHER_LAPACK "" CACHE STRING
    "If you have your own Lapack and Blas, put libraries here, separated by semicolons (full paths or paths that are on the (DY)LD_LIBRARY_PATH (UNIX) or PATH (Windows)). See LAPACK_BEING_USED to see what's actually being used.")
    if(WIN32 AND NOT WINDOWS_USE_EXTERNAL_LIBS)
        set(LAPACK_PLATFORM_DEFAULT lapack;blas)
    else()
        find_package(BLAS) # defines IMPORTED target BLAS::BLAS
        find_package(LAPACK) # defines IMPORTED target LAPACK::LAPACK
        if(BLAS_FOUND AND LAPACK_FOUND)
            set(LAPACK_PLATFORM_DEFAULT BLAS::BLAS LAPACK::LAPACK)
        else()
            message(WARNING "Could not find blas/lapack")
        endif()
    endif()
    set(LAPACK_BEING_USED ${LAPACK_PLATFORM_DEFAULT} CACHE INTERNAL
        "The BLAS/LAPACK libraries that will be linked against by the Simbody libraries.")
    message(CHECK_START "Trying to compile with system BLAS/LAPACK libraries")
endif()


# Try to link against the requested BLAS/LAPACK libraries
set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/lapack_test.c
"\
#include \"SimTKlapack.h\"
int main () {
int n = 2;
int stride = 1;
float x[] = {1.0, 1.0};
float y[] = {2.0, 3.0};

float d = sdot_(&n, x, &stride, y, &stride);

return !(d == 5.0); // return zero if sdot_ worked
}
")

try_compile(OTHER_LAPACK_FUNCTIONAL ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/lapack_test.c
    CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}/SimTKcommon/include
    OUTPUT_VARIABLE BLAS_TRYCOMPILE_LOG
    LINK_LIBRARIES ${LAPACK_BEING_USED})
if(OTHER_LAPACK_FUNCTIONAL)
    message(CHECK_PASS "success.")
    message(STATUS "Using BLAS/LAPACK libraries ${LAPACK_BEING_USED}")
else()
    message(CHECK_FAIL "failed.")
    message(AUTHOR_WARNING ${BLAS_TRYCOMPILE_LOG})
    message(FATAL_ERROR "Failed to compile using the BLAS/LAPACK libraries ${LAPACK_BEING_USED}.
        If BUILD_USING_OTHER_LAPACK was given; check that it was set correctly.")

endif()

if(UNIX)
    if(NOT APPLE)
        set(REALTIME_LIB rt)
        # We do not use pthreads directly anymore, but on Linux, C++11 threads
        # are implemented with pthreads, and we must link to it.
        set(PTHREAD_LIB pthread)
    endif()
    set(MATH_LIBRARY m)
    set(DL_LIBRARY dl)
endif()

set(MATH_LIBS_TO_USE    ${LAPACK_BEING_USED} ${PTHREAD_LIB}
                        ${REALTIME_LIB} ${DL_LIBRARY} ${MATH_LIBRARY})
set(MATH_LIBS_TO_USE_VN ${MATH_LIBS_TO_USE})


add_subdirectory( SimTKcommon )
add_subdirectory( SimTKmath )
add_subdirectory( Simbody )

export(EXPORT SimbodyTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/SimbodyTargets.cmake"
    )

if( BUILD_EXAMPLES )
    add_subdirectory( examples )
endif()

if( INSTALL_DOCS )
    file(GLOB TOPLEVEL_DOCS doc/*.pdf doc/*.md)
    install(FILES LICENSE.txt README.md CHANGELOG.md CONTRIBUTING.md ${TOPLEVEL_DOCS}
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

# Add uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

# Make the cmake config files
set(PKG_NAME ${PROJECT_NAME})
set(PKG_LIBRARIES SimTKsimbody SimTKmath SimTKcommon)

if(WIN32)
    set(SIMBODY_CMAKE_DIR cmake)
elseif(UNIX)
    set(SIMBODY_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/simbody/)
endif()

# Configure SimbodyConfig.cmake in a way that allows the installation to be
# relocatable.
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/SimbodyConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/SimbodyConfigForInstall.cmake
    INSTALL_DESTINATION "${SIMBODY_CMAKE_DIR}"
    PATH_VARS # Variables to edit in the SimbodyConfig.cmake.in.
        CMAKE_INSTALL_PREFIX
        SIMBODY_INCLUDE_INSTALL_DIR
        CMAKE_INSTALL_LIBDIR
        CMAKE_INSTALL_BINDIR
        SIMBODY_VISUALIZER_INSTALL_DIR
        SIMBODY_INSTALL_DOXYGENDIR
    )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/SimbodyConfigForInstall.cmake
        DESTINATION ${SIMBODY_CMAKE_DIR}
        RENAME SimbodyConfig.cmake)

# Create a file that allows clients to Simbody to ensure they have the version
# of Simbody they want.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/SimbodyConfigVersion.cmake
    VERSION "${SIMBODY_VERSION}"
    COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/SimbodyConfigVersion.cmake
        DESTINATION ${SIMBODY_CMAKE_DIR})

install(EXPORT SimbodyTargets
    DESTINATION "${SIMBODY_CMAKE_DIR}") #optionally: COMPONENT dev

# Install a sample CMakeLists.txt that uses SimbodyConfig.cmake.
install(FILES ${CMAKE_SOURCE_DIR}/cmake/SampleCMakeLists.txt
        DESTINATION ${SIMBODY_CMAKE_DIR})

# Make the pkgconfig file: select the proper flags depending on the platform
if(WIN32)
    set(PKGCONFIG_PLATFORM_LIBS "-lliblapack -llibblas")
elseif(APPLE)
    set(PKGCONFIG_PLATFORM_LIBS     "-llapack -lblas -ldl")
else()
    set(PKGCONFIG_PLATFORM_LIBS     "-llapack -lblas -lpthread -lrt -ldl -lm")
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/pkgconfig/simbody.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/pkgconfig/simbody.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/pkgconfig/simbody.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/)
