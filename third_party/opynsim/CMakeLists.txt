cmake_minimum_required(VERSION 3.15)

project(opynsim VERSION 0.0.2 LANGUAGES CXX C)

option(BUILD_TESTING                  "Build test suites"                         ON)
option(OPYN_BUILD_TESTING             "Build test suites (OPynSim-specific)"      ${BUILD_TESTING})
option(OPYN_BUILD_PYTHON_BINDINGS     "Build Python bindings for libopynsim"      ON)
option(OPYN_BUILD_DOCS                "Build documentation"                       OFF)
option(OPYN_USE_STRICT_COMPILER_FLAGS "Add stricter compilation flags to targets" OFF)
option(OPYN_PACKAGE                   "Configure packaging the project's targets" ON)
set   (OPYN_VENV "" CACHE PATH        "Optional absolute path to a Python virtual environment that the build should use")

if(OPYN_VENV)
    if(WIN32)
        set(_opyn_python_exe "${OPYN_VENV}/Scripts/python.exe")
    else()
        set(_opyn_python_exe "${OPYN_VENV}/bin/python")
    endif()

    if(EXISTS ${OPYN_VENV})
        set(Python_EXECUTABLE "${_opyn_python_exe}" CACHE FILEPATH "Path to Python interpreter" FORCE)
    else()
        message(FATAL_ERROR "${_opyn_python_exe}: python executable not found, but OPYN_VENV was set to this path?")
    endif()

    unset(_opyn_python_exe)
endif()

if(OPYN_BUILD_TESTING)
    # This has to be done at the top-level, so that `ctest --show-only` works for
    # all test suites. It is how IDEs typically detect tests.
    #
    # `include(CTest)` isn't used because we don't want extra targets (e.g. `Continuous`).
    enable_testing()
endif()

add_subdirectory(libosim SYSTEM)  # SYSTEM because it's all third-party code
add_subdirectory(libopynsim)

if(OPYN_BUILD_DOCS)
    add_subdirectory(docs)
endif()

if(OPYN_BUILD_PYTHON_BINDINGS)
    add_subdirectory(opynsim)
endif()

if(OPYN_PACKAGE)
    add_subdirectory(packaging)
endif()
