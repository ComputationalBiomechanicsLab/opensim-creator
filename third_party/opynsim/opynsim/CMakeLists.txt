include(CMakeDependentOption)

option(BUILD_TESTING               "Build test suites"                    ON)
option(OPYN_BUILD_TESTING          "Build test suites (OPynSim-specific)" ${BUILD_TESTING})
option(OPYN_BUILD_PYTHON_TESTING   "Build python test suites"             ${OPYN_BUILD_TESTING})
option(OPYN_BUILD_OPYNSIM_DEBUGGER "Build the opynsim_debugger target"    OFF)
option(OPYN_STABLE_ABI             "Perform a python stable ABI build"    ON)

# Figure out required python components
set(OPYN_REQUIRED_PYTHON_COMPONENTS Interpreter)
if(OPYN_STABLE_ABI)
    set(OPYN_MIN_PYTHON_REQUIRED_VERSION 3.12)  # Dictated by nanobind
    list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development.SABIModule)
else()
    set(OPYN_MIN_PYTHON_REQUIRED_VERSION 3.8)  # Dicatated by opynsim's python source code
    list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development.Module)
endif()
if(OPYN_BUILD_OPYNSIM_DEBUGGER)
    list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development)  # the debugger uses embedded API
endif()

# Find Python
find_package(
    Python ${OPYN_MIN_PYTHON_REQUIRED_VERSION} REQUIRED
    COMPONENTS ${OPYN_REQUIRED_PYTHON_COMPONENTS}
)

# Build
#
# Build the native python extension module using `nanobind`
if(TRUE)
    if(APPLE AND CMAKE_OSX_ARCHITECTURES)
        list(LENGTH CMAKE_OSX_ARCHITECTURES _arch_count)
        if(_arch_count GREATER 1)
            message(FATAL_ERROR
                    "Universal builds are not allowed, because it's simpler to package python libraries one arch per package (imo)"
                    "CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} contains multiple entries."
            )
        endif()
    endif()

    # Include nanobind
    set(NB_USE_SUBMODULE_DEPS OFF)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nanobind ${CMAKE_CURRENT_BINARY_DIR}/opynsim/nanobind SYSTEM)

    # Use nanobind to declare the `_opynsim_native` Python extension module target
    set(OPYN_NANOBIND_ARGS NB_STATIC)
    if(OPYN_STABLE_ABI)
        list(APPEND OPYN_NANOBIND_ARGS STABLE_ABI)
    endif()
    nanobind_add_module(_opynsim_native ${OPYN_NANOBIND_ARGS}
        opynsim/src/_opynsim_native.cpp
    )

    # Set `OPYN_PYTHON_INTERPRETER_TAG` and `OPYN_PYTHON_ABI_TAG` on the native module
    #
    # This can help downstream build/packaging steps determine which python interpreter
    # can use the resulting native module. When compiling with a stable ABI, it's
    # dictated by nanobind. Otherwise, it's dictated by the python version that was used
    # to compile the module.
    if(OPYN_STABLE_ABI)
        string(REGEX MATCH "^[0-9]+\\.[0-9]+" OPYN_MIN_PY_VER_MAJOR_MINOR "${OPYN_MIN_PYTHON_REQUIRED_VERSION}")
        string(REPLACE "." "" _opyn_interpreter_tag "${OPYN_MIN_PY_VER_MAJOR_MINOR}")
        set_target_properties(_opynsim_native PROPERTIES
            OPYN_PYTHON_INTERPRETER_TAG "cp${_opyn_interpreter_tag}"
            OPYN_PYTHON_ABI_TAG         "abi3"
        )
        unset(_opyn_interpreter_tag)
        unset(OPYN_MIN_PY_VER_MAJOR_MINOR)
    else()
        # Remove dot between MAJOR.MINOR → "3.11" → "311"
        string(REGEX MATCH "^[0-9]+\\.[0-9]+" _py_mm "${Python_VERSION}")
        string(REPLACE "." "" _py_tag "${_py_mm}")
        set_target_properties(_opynsim_native PROPERTIES
            OPYN_PYTHON_INTERPRETER_TAG "cp${_py_tag}"
            OPYN_PYTHON_ABI_TAG         "cp${_py_tag}"
        )
        unset(_py_tag)
        unset(_py_mm)
    endif()

    target_compile_features(_opynsim_native PUBLIC cxx_std_20)  # CARE: MSVC C++23 isn't ABI-stable yet

    # Link to the underlying `libopynsim` C++ code
    target_link_libraries(_opynsim_native PRIVATE opynsim)

    # Scrap all public symbols apart from `PyInit_*` / `_PyInit_*`
    #
    # This is to prevent symbol collisions when loading the library in a python
    # environment that might also contain (e.g.) OpenSim
    if (APPLE)
        target_link_options(_opynsim_native PRIVATE "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")
    elseif(UNIX AND NOT APPLE)
        target_link_options(_opynsim_native PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
    endif()

    # Nice to have: Build `_opynsim_native` directly into the python package sourcetree
    #
    # This is purely a practical measure that enables developers to run a Python
    # REPL, tests, etc. from a terminal without having to re-run a built/copy/install
    # script after every python edit.
    #
    # Technical note:
    #     > Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append
    #     > a per-configuration subdirectory to the specified directory ***unless a generator
    #     > expression is used***.
    #         - https://cmake.org/cmake/help/latest/prop_tgt/LIBRARY_OUTPUT_DIRECTORY.html
    set_target_properties(_opynsim_native PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opynsim/
        RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opynsim/
    )
endif()

# Testing
if(OPYN_BUILD_PYTHON_TESTING)
    add_subdirectory(tests)
endif()

# Installation
#
# Creates a directory that has a package layout that can either be directly installed
# into the caller's (presumed, Linux) environment, or added to their PYTHONPATH
if(TRUE)
    set(OPYN_PYTHON_INSTALL_PREFIX "lib/python3/site-packages/opynsim")

    # Install package python sources (retain directory layout but skip src, __pycache__, etc.)
    file(GLOB_RECURSE OPYN_PYTHON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/opynsim/*.py")
    foreach(f ${OPYN_PYTHON_FILES})
        file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/opynsim" "${f}")
        get_filename_component(dest_dir "${rel}" DIRECTORY)
        install(FILES "${f}" DESTINATION "${OPYN_PYTHON_INSTALL_PREFIX}/${dest_dir}" COMPONENT opynsim-python)
    endforeach()

    # Install `_opynsim_native` extension module into the package
    install(
        FILES $<TARGET_FILE:_opynsim_native>
        DESTINATION "${OPYN_PYTHON_INSTALL_PREFIX}"
        COMPONENT opynsim-python
    )
endif()

if(OPYN_BUILD_OPYNSIM_DEBUGGER)
    add_subdirectory(debugger)
endif()
