# Build and install examples.
#
# This is for use when building Simbody from source; it is not for
# end users to build examples themselves. This guarantees that the 
# examples built with Simbody are using the libraries just built regardless
# of what is installed. We install a different CMakeLists.txt along with
# the examples for end users to use.
#
# Examples are built from an explicit list of subdirectories given below,
# and then simpler examples are built one per .cpp file in the top level
# examples directory. If your example needs more than just a single .cpp
# with a single .h of the same name, then put it in its own subdirectory 
# where it can have its own CMakeLists.txt. You must list those
# subdirectories explicitly below; the simpler ones will be discovered
# automatically.
#
# Examples get built with these macros defined:
#    SIMBODY_EXAMPLE_NAME     
#    SIMBODY_EXAMPLES_INSTALL_SRC  (has trailing "/")
# Subdirectory examples should locate their auxiliary files by searching
# for one file, say "geometry/pelvis.stl", using this search path:
#    .  (current directory first)
#    ./examples/${SIMBODY_EXAMPLE_NAME}
#    ${SIMBODY_EXAMPLES_INSTALL_SRC}${SIMBODY_EXAMPLE_NAME}
#
# There is also a "shared" directory that has shared header files for
# the examples, but also may contain shared auxiliary files like common
# geometric shapes. In that case it must also be located at run time
# using the search path:
#    ../shared   (parallel to current directory first)
#    ./examples/shared
#    ${SIMBODY_EXAMPLES_INSTALL_SRC}shared
#
# Examples can depend on any Simbody includes, and may depend on headers
# in the examples/shared directory, in which case they should be
# included as #include "shared/SharedHeaderFile.h". This also permits use 
# of headers between examples, via #include "OtherExample/SomeHeader.h",
# although it is not clear that's a good idea.
# 
# Note that unlike code in the "tests" directory (but like the 
# "tests/adhoc" code), this does not generate CMake ADD_TEST commands so 
# these will never run automatically.
#
# For IDEs that can deal with PROJECT_LABEL properties (at least
# Visual Studio) the projects for building each of these adhoc
# executables will be labeled "Example - TheExampleName" if a file
# TheExampleName.cpp is found in this directory, or if we build from 
# a subdirectory called TheExampleName.

# All subdirectory examples must be listed here.  A subdirectory is needed 
# for any example that consists of more than just one source file and one 
# header file.
set(SUBDIR_EXAMPLES
    BricardMechanism
    Gazebo2Simbody
    TaskSpaceControl-UR10
    TaskSpaceControl-Atlas
)

# There is a top-level "examples" directory in the Windows install,
# but examples go under "doc" elsewhere.
if(WIN32)
  set(EXAMPLES_INSTALL_BIN examples/bin/)
  set(EXAMPLES_INSTALL_SRC examples/src/)
else()
  set(EXAMPLES_INSTALL_BIN ${CMAKE_INSTALL_LIBDIR}/simbody/examples/) # if this changes, change the corresponding
                                                                      # FULL version in file(RELATIVE_PATH ) command
  set(EXAMPLES_INSTALL_SRC ${CMAKE_INSTALL_DOCDIR}/examples/src/)
  # Use full paths for the following as it will be used to create a symlink
  # The symlink itself will be a relative path, so the package will be relocatable
  set(EXAMPLES_SYMLINK_BIN ${CMAKE_INSTALL_FULL_DOCDIR}/examples)
  file(RELATIVE_PATH EXAMPLE_INSTALL_BIN_REL_TO_DOC
      ${EXAMPLES_SYMLINK_BIN}
      ${CMAKE_INSTALL_FULL_LIBDIR}/simbody/examples/)
endif()

# Set RPATH for all example targets in this directory and in subdirectories.
if(${SIMBODY_USE_INSTALL_RPATH})
    file(RELATIVE_PATH exbin_dir_to_install_dir
        "${CMAKE_INSTALL_PREFIX}/${EXAMPLES_INSTALL_BIN}"
        "${CMAKE_INSTALL_PREFIX}")
    set(exbin_dir_to_lib_dir
        "${exbin_dir_to_install_dir}${CMAKE_INSTALL_LIBDIR}")
    if(${SIMBODY_USE_INSTALL_RPATH})
        if(APPLE)
            set(CMAKE_INSTALL_RPATH "@executable_path/${exbin_dir_to_lib_dir}")
        elseif(UNIX)
            set(CMAKE_INSTALL_RPATH "\$ORIGIN/${exbin_dir_to_lib_dir}")
        endif()
    endif()
endif()

# CMAKE_INSTALL_PREFIX may be a relative path name; we want to bake in
# the absolute path for the examples.
get_filename_component(ABS_CMAKE_INSTALL_PREFIX 
                       "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
add_definitions(
    "-DSIMBODY_EXAMPLES_INSTALL_SRC=\"${ABS_CMAKE_INSTALL_PREFIX}/${EXAMPLES_INSTALL_SRC}\"")

# Provide access to top-level examples headers
# so that #include "shared/Blah.h" will work.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# First build examples that have their own subdirectories and CMakeLists
# files. 
foreach(EX_NAME ${SUBDIR_EXAMPLES})
    add_subdirectory(${EX_NAME})
endforeach()

# Next pick up stragglers that consist solely of a .cpp file in the top
# level examples directory, with a single .h file of the same name.
# The example name is the source file name with ".cpp" removed, and
# the generated executable will have that name.

file(GLOB EXAMPLES "*.cpp")
foreach(EX_PROG ${EXAMPLES})
    get_filename_component(EX_SRC  ${EX_PROG} NAME)
    get_filename_component(EX_NAME ${EX_PROG} NAME_WE)
    set(EX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/${EX_NAME}.h")
    if(NOT EXISTS ${EX_HEADER})
        unset(EX_HEADER)
    endif()

    add_executable(${EX_NAME} ${EX_PROG} ${EX_HEADER})
    if(TARGET simbody-visualizer)
        add_dependencies(${EX_NAME} simbody-visualizer)
    endif()
    set_target_properties(${EX_NAME}
        PROPERTIES
        COMPILE_FLAGS "-DSIMBODY_EXAMPLE_NAME=\"${EX_NAME}\""
        FOLDER "Examples" # Organize target list in IDE
        PROJECT_LABEL "Example - ${EX_NAME}")
    target_link_libraries(${EX_NAME} SimTKsimbody)

    if(SIMBODY_BUILD_SHARED_LIBS) # Don't install static examples
        install(TARGETS ${EX_NAME} 
            DESTINATION ${EXAMPLES_INSTALL_BIN}
            # Don't install Debug examples
            CONFIGURATIONS Release RelWithDebInfo MinSizeRel)
    endif()
endforeach()

# install top-level source for examples
file(GLOB SRC_FILES "*.cpp" "*.h")
install(FILES ${SRC_FILES} DESTINATION ${EXAMPLES_INSTALL_SRC})

# install shared source and other files; also copy to binary directory
# so we can use them when running from the build
file(GLOB_RECURSE SHARED_FILES 
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/shared"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/*")
foreach(one_shared_file ${SHARED_FILES})
    configure_file(shared/${one_shared_file}
       ${CMAKE_CURRENT_BINARY_DIR}/shared/${one_shared_file} COPYONLY)
endforeach()

install(DIRECTORY shared/
        DESTINATION ${EXAMPLES_INSTALL_SRC}shared) # already has "/"

# install .txt files except CMakeLists.txt
file(GLOB TXT_FILES "*.txt")
foreach(TXTF ${TXT_FILES})
    if(NOT "${TXTF}" MATCHES "CMakeLists.txt")
    install(FILES ${TXTF} DESTINATION ${EXAMPLES_INSTALL_SRC})
    endif()
endforeach()

# install the installed version of CMakeLists.txt
install(FILES InstalledCMakeLists.txt DESTINATION ${EXAMPLES_INSTALL_SRC}
    RENAME CMakeLists.txt)

if(NOT WIN32)
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink
          ${EXAMPLE_INSTALL_BIN_REL_TO_DOC} ${EXAMPLES_SYMLINK_BIN}/bin)")
endif()
