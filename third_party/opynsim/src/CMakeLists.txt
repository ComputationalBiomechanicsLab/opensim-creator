include(CMakeDependentOption)

option(OPYN_GENERATE_STUBS         "Generate Python stubs (.pyi) for native bindings" ON)

# Sanity-check universal builds (not allowed)
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES _arch_count)
    if(_arch_count GREATER 1)
        message(FATAL_ERROR
            "Universal builds are not allowed, because it's simpler to package python libraries one arch per package (imo)"
            "CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} contains multiple entries."
        )
    endif()
endif()

# Setup Python source code staging.
#
# This ensures the Python source code is copied into the build directory, so
# that each build configuration (Debug, Release, etc.) has its own unique
# staging directory that can be ran independently of the others.
if(TRUE)
    set(OPYN_OPYNSIM_STAGING_DIR "${CMAKE_CURRENT_BINARY_DIR}/opynsim")
    set(OPYN_OPYNSIM_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/opynsim")
    set(OPYN_STAGING_OUTPUTS)

    # Setup custom build command that copies Python sources into the staging
    # directory whenever one changes
    file(GLOB_RECURSE OPYN_OPYNSIM_PYTHON_SOURCES "${OPYN_OPYNSIM_SOURCE_ROOT}/*.py")
    foreach(_src ${OPYN_OPYNSIM_PYTHON_SOURCES})
        file(RELATIVE_PATH _rel ${OPYN_OPYNSIM_SOURCE_ROOT} ${_src})
        set(_dst ${OPYN_OPYNSIM_STAGING_DIR}/${_rel})
        get_filename_component(_dst_dir ${_dst} DIRECTORY)
        add_custom_command(
            OUTPUT ${_dst}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_dst_dir}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_src} ${_dst}
            DEPENDS ${_src}
            COMMENT "Staging ${_rel}"
            VERBATIM
        )
        list(APPEND OPYN_STAGING_OUTPUTS ${_dst})
    endforeach()
    add_custom_target(opynsim_python_source_staging ALL DEPENDS ${OPYN_STAGING_OUTPUTS})
endif()

# Setup native Python extension module (opynsim._core)
#
# This uses nanobind to build C++ code into a native Python extension module
# that's built into the staging directory, which means that the resulting
# staging directory contains everything it needs (Python + native module); for
# example, to be ran from the command-line (using PYTHONPATH=staging_directory).
if(TRUE)
    # Find nanobind
    find_package(nanobind REQUIRED)

    # Use nanobind to declare the `opynsim._core` Python extension module target
    #
    # See https://nanobind.readthedocs.io/en/latest/api_cmake.html for API docs
    set(OPYN_NANOBIND_ARGS
        NB_STATIC             # Compile the nanobind library as a static library
        NB_SUPPRESS_WARNINGS  # Mark nanobind/Python directories as SYSTEM and disable clang-tidying them
    )
    if(OPYN_STABLE_ABI)
        list(APPEND OPYN_NANOBIND_ARGS STABLE_ABI)
    endif()
    file(GLOB_RECURSE _core_sources "opynsim/_core/*.cpp")
    nanobind_add_module(_core ${OPYN_NANOBIND_ARGS} ${_core_sources})
    unset(_core_sources)
    target_include_directories(_core
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}  # so that `#include <opynsim/_core/...>` works
    )

    # Set `OPYN_PYTHON_INTERPRETER_TAG` and `OPYN_PYTHON_ABI_TAG` on the `_core` module.
    #
    # This can help downstream build/packaging steps determine which python interpreter
    # can use the resulting native module. When compiling with a stable ABI, it's
    # dictated by nanobind. Otherwise, it's dictated by the python version that was used
    # to compile the module.
    if(OPYN_STABLE_ABI)
        string(REGEX MATCH "^[0-9]+\\.[0-9]+" OPYN_MIN_PY_VER_MAJOR_MINOR "${OPYN_MIN_PYTHON_REQUIRED_VERSION}")
        string(REPLACE "." "" _opyn_interpreter_tag "${OPYN_MIN_PY_VER_MAJOR_MINOR}")
        set_target_properties(_core PROPERTIES
            OPYN_PYTHON_INTERPRETER_TAG "cp${_opyn_interpreter_tag}"
            OPYN_PYTHON_ABI_TAG         "abi3"
        )
        unset(_opyn_interpreter_tag)
        unset(OPYN_MIN_PY_VER_MAJOR_MINOR)
    else()
        # Remove dot between MAJOR.MINOR → "3.11" → "311"
        string(REGEX MATCH "^[0-9]+\\.[0-9]+" _py_mm "${Python_VERSION}")
        string(REPLACE "." "" _py_tag "${_py_mm}")
        set_target_properties(_core PROPERTIES
            OPYN_PYTHON_INTERPRETER_TAG "cp${_py_tag}"
            OPYN_PYTHON_ABI_TAG         "cp${_py_tag}"
        )
        unset(_py_tag)
        unset(_py_mm)
    endif()

    # CARE: MSVC C++23 isn't ABI-stable yet
    target_compile_features(_core PUBLIC cxx_std_20)

    # Link to the underlying `libopynsim` C++ code
    target_link_libraries(_core PRIVATE opynsim)

    # Scrap all public symbols apart from `PyInit_*` / `_PyInit_*`
    #
    # This is to reduce symbol collisions when loading the library in a python
    # environment that might also contain (e.g.) OpenSim
    if (APPLE)
        target_link_options(_core PRIVATE "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")
    elseif(UNIX AND NOT APPLE)
        target_link_options(_core PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
    endif()

    # Ensure the staging directory is created and contains the necessary
    # Python code before building `_core` into the staging directory
    add_dependencies(_core opynsim_python_source_staging)

    # Make `_core` build to the staging directory
    set_target_properties(_core PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OPYN_OPYNSIM_STAGING_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${OPYN_OPYNSIM_STAGING_DIR}
    )

    # Build Python interface stubs (`.pyi`) for `_core` into the staging directory.
    #
    # This enables type checking etc. in downstream development environments
    # when using the build staging directory.
    # See:
    # - https://nanobind.readthedocs.io/en/latest/typing.html
    # - https://nanobind.readthedocs.io/en/latest/api_cmake.html#command:nanobind_add_stub
    if(OPYN_GENERATE_STUBS)
        nanobind_add_stub(
            _core_development_stubs
            MODULE _core
            PYTHON_PATH $<TARGET_FILE_DIR:_core>
            DEPENDS _core
            MARKER_FILE py.typed
            RECURSIVE
            OUTPUT_PATH ${OPYN_OPYNSIM_STAGING_DIR}
        )
    endif()
endif()

# Installation
#
# Installation is effectively copying the staging directory to the
# installation directory.
if(TRUE)
    set(OPYN_PYTHON_INSTALL_PREFIX "lib/python3/site-packages/opynsim")

    # Install staged sources and stubs
    install(
        DIRECTORY   "${OPYN_OPYNSIM_STAGING_DIR}/"
        DESTINATION "${OPYN_PYTHON_INSTALL_PREFIX}"
        COMPONENT opynsim-python
        FILES_MATCHING
            PATTERN "*.py"
            PATTERN "*.pyi"
            PATTERN "py.typed"
            PATTERN "__pycache__" EXCLUDE
            PATTERN "*.pyc"       EXCLUDE
    )

    # Install `_core` native extension module
    install(
        FILES $<TARGET_FILE:_core>
        DESTINATION "${OPYN_PYTHON_INSTALL_PREFIX}"
        COMPONENT opynsim-python
    )
endif()
