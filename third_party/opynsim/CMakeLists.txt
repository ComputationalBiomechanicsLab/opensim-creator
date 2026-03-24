cmake_minimum_required(VERSION 3.15)

project(opynsim VERSION 0.0.5 LANGUAGES CXX C)

option(BUILD_TESTING                  "Build test suites"                         ON)
option(OPYN_BUILD_TESTING             "Build test suites (OPynSim-specific)"      ${BUILD_TESTING})
option(OPYN_BUILD_PYTHON_TESTING      "Build python test suites"                  ${OPYN_BUILD_TESTING})
option(OPYN_BUILD_PYTHON_BINDINGS     "Build Python bindings for libopynsim"      ON)
option(OPYN_BUILD_OPYNSIM_DEBUGGER    "Build the opynsim_debugger target"         OFF)
option(OPYN_BUILD_DOCS                "Build documentation"                       OFF)
option(OPYN_USE_STRICT_COMPILER_FLAGS "Add stricter compilation flags to targets" OFF)
option(OPYN_PACKAGE                   "Configure packaging the project's targets" ON)
option(OPYN_STABLE_ABI                "Perform a python stable ABI build"         ON)
option(OPYN_STRICT_ABI_CHECKS         "Check OPynSim has specified ABI"           ${OPYN_STABLE_ABI})
set   (OPYN_VENV "" CACHE PATH        "Optional absolute path to a Python virtual environment that the build should use")

if(OPYN_BUILD_TESTING)
    # This has to be done at the top-level, so that `ctest --show-only` works for
    # all test suites. It is how IDEs typically detect tests.
    #
    # `include(CTest)` isn't used because we don't want extra targets (e.g. `Continuous`).
    enable_testing()
endif()

if(OPYN_BUILD_OSCAR_INLINE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/oscar)
endif()

add_subdirectory(libosim SYSTEM)  # SYSTEM because it's all third-party code
add_subdirectory(libopynsim)

if(OPYN_BUILD_PYTHON_BINDINGS)
    # If building Python bindings, try to centrally figure out Python environment,
    # version, required components here, so that the sub-parts of the build consistently
    # use the same Python.

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

    # Figure out required python components
    set(OPYN_REQUIRED_PYTHON_COMPONENTS Interpreter Development.Module)
    if(OPYN_STABLE_ABI)
        set(OPYN_MIN_PYTHON_REQUIRED_VERSION 3.12)  # Dictated by nanobind
        list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development.SABIModule)
    else()
        set(OPYN_MIN_PYTHON_REQUIRED_VERSION 3.8)  # Dictated by the package's Python source code
    endif()
    if(OPYN_BUILD_OPYNSIM_DEBUGGER)
        list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development.Embed)
    endif()

    # Finally, find a Python distribution that matches requirements.
    find_package(
        Python ${OPYN_MIN_PYTHON_REQUIRED_VERSION}
        COMPONENTS ${OPYN_REQUIRED_PYTHON_COMPONENTS}
        REQUIRED
    )

    if(OPYN_STRICT_ABI_CHECKS)
        if((NOT Python_VERSION_MAJOR EQUAL 3) OR (NOT Python_VERSION_MINOR EQUAL 12))
            message(FATAL_ERROR "When OPYN_STRICT_ABI_CHECKS is ON, the Python version MUST be 3.12.x (yours is ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}), so that the resulting ABI is valid for >= 3.12 clients.\nOne way to fix this is to create a 3.12 venv and pass it via OPYN_VENV, or use cmake Python hints like Python_ROOT_DIR")
        endif()
    endif()

    # Include sub-parts of the build
    add_subdirectory(src)
    if(OPYN_BUILD_PYTHON_TESTING)
        add_subdirectory(tests)
    endif()
    if(OPYN_BUILD_OPYNSIM_DEBUGGER)
        add_subdirectory(opynsim_debugger)
    endif()
endif()

if(OPYN_BUILD_DOCS)
    add_subdirectory(docs)
endif()

if(OPYN_PACKAGE)
    add_subdirectory(packaging)
endif()
