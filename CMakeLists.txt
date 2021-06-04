cmake_minimum_required(VERSION 3.5)

project(osc VERSION 0.0.2 LANGUAGES C CXX)


# -------------- gather client build config options ---------------- #

set(OSC_USE_CLANG_TIDY OFF CACHE BOOL "enable/disable running `clang-tidy` (linter) while building")
set(OSC_USE_IWYU OFF CACHE BOOL "enable/disable running `iwyu` (include-what-you-use) while building")
set(OSC_IWYU_PATH "include-what-you-use" CACHE STRING "path to the `iwyu` executable (if not on the PATH)")
set(OSC_FORCE_ASSERTS_ENABLED ON CACHE BOOL "enable OSC's runtime assertions - even if building a release build")
set(OSC_FORCE_UNDEFINE_NDEBUG OFF CACHE BOOL "force disable NDEBUG, even if running in Release mode. This enables *a lot* of runtime assertions (standard lib, SimTK, OpenSim)")
set(OSC_USE_ASAN OFF CACHE BOOL "enable address sanitizer (libASAN): useful for spotting memory access violations and leaks")
set(OSC_DEFAULT_USE_MULTI_VIEWPORT OFF CACHE BOOL "enable/disable osc using multi viewport when not explicitly specified a the user/config")
set(OSC_DEFAULT_RESOURCE_DIR "../resources" CACHE STRING "default location of runtime resources dir when not specified by user/config")


# -------------- top-level build configuration (vars etc.) ---------------- #

# shim CMake for older versions:
#
#     - all shimming should be close to the variables set by this, for easier debugging
if((${CMAKE_MAJOR_VERSION} LESS 3) OR (${CMAKE_MINOR_VERSION} LESS 13))
    message(WARNING "you are using an older version of CMake. This is fine (for now), but you should upgrade")
    set(OSC_USING_MODERN_CMAKE 0)
else()
    set(OSC_USING_MODERN_CMAKE 1)
endif()

# test if the generated build system is multi-config (e.g. Visual Studio, which lets
# devs change from Debug to Release in a dropdown, etc.)
get_property(OSC_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(${OSC_USING_MODERN_CMAKE})
    cmake_policy(SET CMP0072 OLD)  # use newer OpenGL linking strategy
endif()
set(OpenGL_GL_PREFERENCE "LEGACY")  # use newer OpenGL linking strategy

if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a UNIX system that may not be supported yet - beware!")
    endif()
endif()

if(OSC_USE_IWYU)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${OSC_IWYU_PATH};-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;")
endif()

if(OSC_USE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
endif()

# set a build ID so that errors in the binary can be linked to source
if(DEFINED ENV{GITHUB_RUN_ID})
    set(OSC_BUILD_ID "GITHUB_$ENV{GITHUB_RUN_ID}.$ENV{GITHUB_RUN_NUMBER}")
else()
    string(RANDOM RANDOM_ID)
    set(OSC_BUILD_ID "CUSTOM_${RANDOM_ID}")
    unset(RANDOM_ID)
endif()

if(${OSC_DEFAULT_USE_MULTI_VIEWPORT})
    set(OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL "true")
else()
    set(OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL "false")
endif()


# --------------- dependency management ----------------- #

# all dependency management is handled from a separate CMake file that exports
# the dependencies as targets - along with some other useful vars (e.g. a list
# of all dependencies that should be copied into the install, etc.)
include(GetDependencies.cmake)


# ------------ osc build target configuration ---------- #

# osc_build_config.hpp:
#     configuration file that contains configuration-time vals (e.g. version, libs)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/osc_build_config.hpp.in" "osc_build_config.hpp")

add_executable(osc
    ${CMAKE_BINARY_DIR}/osc_build_config.hpp

    src/3d/gl.hpp
    src/3d/gl.cpp
    src/3d/gl_glm.hpp
    src/3d/cameras.hpp
    src/3d/cameras.cpp
    src/3d/shaders.hpp
    src/3d/3d.hpp
    src/3d/3d.cpp

    src/simtk_bindings/simtk_bindings.hpp
    src/simtk_bindings/simtk_bindings.cpp

    src/opensim_bindings/model_drawlist.hpp
    src/opensim_bindings/model_drawlist.cpp
    src/opensim_bindings/simulation.hpp
    src/opensim_bindings/simulation.cpp
    src/opensim_bindings/type_registry.hpp
    src/opensim_bindings/type_registry.cpp
    src/opensim_bindings/opensim_helpers.hpp

    src/ui/add_component_popup.hpp
    src/ui/add_component_popup.cpp
    src/ui/component_hierarchy.hpp
    src/ui/component_hierarchy.cpp
    src/ui/component_details.cpp
    src/ui/component_details.hpp
    src/ui/component_3d_viewer.hpp
    src/ui/component_3d_viewer.cpp
    src/ui/properties_editor.hpp
    src/ui/properties_editor.cpp
    src/ui/reassign_socket_popup.hpp
    src/ui/reassign_socket_popup.cpp
    src/ui/attach_geometry_popup.hpp
    src/ui/attach_geometry_popup.cpp
    src/ui/add_body_popup.hpp
    src/ui/add_body_popup.cpp
    src/ui/log_viewer.hpp
    src/ui/log_viewer.cpp
    src/ui/lockable_f3_editor.hpp
    src/ui/lockable_f3_editor.cpp
    src/ui/main_menu.hpp
    src/ui/main_menu.cpp
    src/ui/help_marker.hpp
    src/ui/help_marker.cpp
    src/ui/evenly_spaced_sparkline.hpp
    src/ui/coordinate_editor.hpp
    src/ui/coordinate_editor.cpp
    src/ui/muscles_table.hpp
    src/ui/muscles_table.cpp
    src/ui/select_2_pfs_popup.hpp
    src/ui/select_2_pfs_popup.cpp
    src/ui/select_1_pf_popup.hpp
    src/ui/select_1_pf_popup.cpp
    src/ui/model_actions.hpp
    src/ui/model_actions.cpp
    src/ui/select_component_popup.hpp
    src/ui/fd_params_editor_popup.hpp
    src/ui/fd_params_editor_popup.cpp

    src/screens/screen.hpp
    src/screens/error_screen.hpp
    src/screens/error_screen.cpp
    src/screens/show_model_screen.hpp
    src/screens/show_model_screen.cpp
    src/screens/loading_screen.hpp
    src/screens/loading_screen.cpp
    src/screens/imgui_demo_screen.hpp
    src/screens/imgui_demo_screen.cpp
    src/screens/opengl_test_screen.hpp
    src/screens/opengl_test_screen.cpp
    src/screens/model_editor_screen.hpp
    src/screens/model_editor_screen.cpp
    src/screens/splash_screen.hpp
    src/screens/splash_screen.cpp
    src/screens/simulator_screen.hpp
    src/screens/simulator_screen.cpp

    src/utils/concurrency.hpp
    src/utils/shims.hpp
    src/utils/sdl_wrapper.hpp
    src/utils/os.hpp
    src/utils/os.cpp
    src/utils/scope_guard.hpp
    src/utils/circular_buffer.hpp
    src/utils/file_change_poller.hpp
    src/utils/helpers.hpp
    src/utils/helpers.cpp
    src/utils/stbi_wrapper.hpp
    src/utils/stbi_wrapper.cpp

    src/main_editor_state.hpp
    src/main_editor_state.cpp
    src/config.hpp
    src/config.cpp
    src/application.hpp
    src/application.cpp
    src/log.hpp
    src/log.cpp
    src/assertions.hpp
    src/assertions.cpp
    src/resources.hpp
    src/resources.cpp
    src/osc.cpp

    # Windows: also link a resources file (rc)
    #
    # this tells the MSVC compiler how to compile non-source resources (e.g. the icon) into the
    # output exe
    $<$<CXX_COMPILER_ID:MSVC>:build_resources/resources.rc>
)
target_link_libraries(osc PRIVATE
    # in gcc/clang the <filesystem> implementation must be explicitly linked
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>

    osc-all-deps  # see: GetDependencies.cmake
)
target_include_directories(osc PRIVATE
    ${PROJECT_BINARY_DIR}  # so `#include "osc_build_config.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}  # so `#include "src/<path>.hpp"` works
)
target_compile_options(osc PRIVATE

    # msvc flags
    $<$<CXX_COMPILER_ID:MSVC>:
        /wd4996  # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4455  # disable warning 4455: incorrectly flags std::string::literals::"s"
        /W3  # turn warning level up to "production grade" (note: /W4 explodes because of Simbody)
        /WX  # treat all warnings as errors
        /Oy-  # keep frame pointers around

        # newer MSVCs may also come with LIBASAN (experimental)
        $<$<BOOL:${OSC_USE_ASAN}>:/fsanitize=address>
    >

    # gcc/clang flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall
        -Wextra
        -pedantic
        -Werror
        -Wuninitialized
        -Winit-self
        -Wshadow
        -Wcast-align
        -Wwrite-strings
        -Wdangling-else
        -Wdate-time
        -Wmissing-declarations
        -Wno-multichar
        -Wredundant-decls
        -Wvla
        -Wdisabled-optimization
        -Wpacked
        -Wno-sign-compare  # disable: broken by STB headers
        -Wformat-security

        # regardless of debug/release, pin the frame pointer register
        # so that stack traces are sane when debugging (even in Release).
        #
        # This adds some overhead (pins one register and requires callers
        # to setup their base pointers etc.) but makes debugging + profiling
        # the application much easier, even in release mode
        -fno-omit-frame-pointer

        # if address sanitizer is specified, use it
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >

    # clang-only
    $<$<CXX_COMPILER_ID:Clang>:
        -Wno-cast-align

        # required in earlier clangs. Just setting
        # -fno-omit-frame-pointer (above) is not enough
        #
        # see:
        #   - https://stackoverflow.com/questions/43864881/fno-omit-frame-pointer-equivalent-compiler-option-for-clang
        #   - fixed here: https://reviews.llvm.org/D64294
        -mno-omit-leaf-frame-pointer
    >

    # gcc-only
    $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused  # disable: broken by STB headers
    >

    # older CMakes do not support C++17 config
    $<$<NOT:$<BOOL:${OSC_USING_MODERN_CMAKE}>>:-std=c++17>
)
if(OSC_FORCE_ASSERTS_ENABLED)
    target_compile_definitions(osc PRIVATE -DOSC_FORCE_ASSERTS_ENABLED=1)
endif()

if(OSC_FORCE_UNDEFINE_NDEBUG)
    # MSVC
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    # others
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

set_target_properties(osc PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED YES
    EXPORT_COMPILE_COMMANDS ON  # needed by clang-tidy
)
if(LINUX)
    # Linux RPATH:
    #     - at installation time, Linux install dir is:
    #         bin/
    #         lib/*.so
    #     - so set the install RPATH to reflect this
    set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
endif()
if(APPLE)
    # Apple RPATH:
    #     - at installation time, Apple install dir is something like:
    #
    #           lib/*.dylib
    #           resources/*
    #           osc (exe)
    #           osc.toml
    set_target_properties(osc PROPERTIES INSTALL_RPATH "@executable_path/lib")
endif()

if(${OSC_USING_MODERN_CMAKE})
    # this is the "modern" way to explain CXX conformance
    target_compile_features(osc PUBLIC cxx_std_17)
endif()

target_link_options(osc PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        /SUBSYSTEM:windows  # open as a desktop app, not CLI
        /ENTRY:mainCRTStartup  # as above
        /ignore:4217  # ImGui uses locally defined symbols
    >

    # only Linux + GNU/Clang support rdynamic?
    $<$<AND:$<BOOL:LINUX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:
        # make `ld` emit symbols for all functions - even if they aren't
        # externally used (with an exe, only `main` is used, really)
        #
        # this makes the binary a little bigger (~400 KB) but means
        # that the application can emit its own stack trace /w function
        # names - even in Release mode.
        -rdynamic
    >

    # if address sanitizer is specified, use it
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >
)


# -------------- development nice-to-haves ------------- #

# generate a dev-centric `osc.toml`
#
#     - this causes osc to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/build_resources/DEV_osc.toml.in" "${CMAKE_BINARY_DIR}/osc.toml")

# (on Windows): copy libraries into the same dir as osc.exe
#
#     - this is so that devs can run osc.exe from an IDE without having to
#       copy libraries around manually etc.
#
#     - this isn't necessary with Unix'es because CMake sets the RPATH up to
#       point to wherever the libraries are located
#
#     - at install time, the libraries are copied into the install location
if (WIN32)
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        add_custom_command(
            TARGET osc
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_FILE_TO_COPY}" "$<TARGET_FILE_DIR:osc>"
        )
    endforeach()
endif()



# ---------- installation / packaging (CPack) ----------- #

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${osc_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${osc_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_DESCRIPTION "A thin GUI for OpenSim - primarily for development use")

# Windows install/packaging:
#
#     - copy osc.exe + all DLLs into the bin/ dir
#
#     - this creates a "fat" build in bin/, where all the necessary libraries are just
#       packaged with the install (so users only have to install one thing)
#
#     - see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
#
#     - packaging: uses NSIS.exe : get it from https://nsis.sourceforge.io/Download
if (WIN32)
    # install osc.exe
    install(TARGETS osc)

    # install DLLs
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION bin)
    endforeach()

    # install a user-centric `osc.toml`
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml" RENAME "osc.toml" DESTINATION ".")

    # install `resources/`
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

    # set up NSIS packaging
    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.ico")
    set(CPACK_NSIS_UNINSTALL_NAME "uninstall-osc")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_HELP_LINK "https://github.com/adamkewley/osc")
    set(CPACK_NSIS_CONTACT "contact@adamkewley.com")
    set(CPACK_NSIS_MODIFY_PATH ON)  # user can modify install locaton
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN osc)
    set(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\osc ${osc_VERSION_MAJOR}.${osc_VERSION_MINOR}.${osc_VERSION_PATCH}\\\\osc.lnk' '\$INSTDIR\\\\bin\\\\osc.exe'")
endif()

# Linux install/packaging:
#
#     - copy osc + libraries into a standard-layout dir
#           bin/  # executables
#           lib/  # libraries (.so)
#           resources/  # arch-independent runtime resources
#
#     - this dir is "done", and could be sent as a ZIP to an end-user
#
#     - **BUT** the way that OpenSim/osc is built assumes the user has all the
#       necessary runtime libraries available
#
#     - so the packaging step creates a .deb file that declares which runtime dependencies
#       a client should have (or automatically acquire via apt)
#
#     - note: the package assumes the user has a compatible OpenGL driver installed, servers
#             can try to "fake" this by running:
#
#           apt-get install libopengl0 libglx0 libglu1-mesa
if (LINUX)
    # install osc
    install(TARGETS osc DESTINATION bin/)

    # install *bundled* SOs
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION lib/)
    endforeach()

    # install a user-centric `osc.toml`
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml" RENAME "osc.toml" DESTINATION ".")

    # install `resources/`
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

    # set up DEB packaging
    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libblas3, liblapack3, libstdc++6")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    configure_file("${PROJECT_SOURCE_DIR}/build_resources/postinst.in" "postinst" @ONLY)  # create symlink /usr/local/bin/osc --> /opt/osc/bin/osc
    configure_file("${PROJECT_SOURCE_DIR}/build_resources/postrm.in" "postrm" @ONLY)  # destroy symlink created during install
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")
endif()

# Apple OSX packaging:
#
# - Create a DMG (archive) installer that packages the whole application +
#   libraries into a single directory tree that can be copied to
#   /Applications/osc
if (APPLE)
    # install main executable into package root (Application/ dir requirement)
    install(TARGETS osc DESTINATION osc.app/Contents/MacOS/)

    # install bundled libraries into lib/
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION osc.app/Contents/MacOS/lib/)
    endforeach()

    # install a user-centric `osc.toml`
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml" RENAME "osc.toml" DESTINATION osc.app/Contents/MacOS/)

    # install an Info.plist file for this particular app
    #
    # this is a mac-specific XML file
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/Info.plist" DESTINATION osc.app/Contents/)

    # install `resources/`
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION osc.app/Contents/MacOS/)

    # install desktop icon (.icns)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/osc.icns" DESTINATION osc.app/Contents/Resources/)

    set(CPACK_GENERATOR DragNDrop)
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
