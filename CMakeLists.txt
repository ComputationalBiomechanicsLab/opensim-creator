cmake_minimum_required(VERSION 3.5)

project(osmv VERSION 0.0.1 LANGUAGES C CXX)



# -------------- top-level configuration ---------------- #

# shim CMake for older versions:
#
#     - all shimming should be close to the variables set by this, for easier debugging
if((${CMAKE_MAJOR_VERSION} LESS 3) OR (${CMAKE_MINOR_VERSION} LESS 13))
    message(WARNING "you are using an older version of CMake. This is fine (for now), but you should upgrade")
    set(OSMV_USING_MODERN_CMAKE 0)
else()
    set(OSMV_USING_MODERN_CMAKE 1)
endif()

get_property(OSMV_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

set(OSMV_USE_LINTER OFF CACHE BOOL "enable/disable running `clang-tidy` (linter) while building")
set(OSMV_USE_IWYU OFF CACHE BOOL "enable/disable running `iwyu` (include-what-you-use) while building")
set(OSMV_IWYU_PATH "iwyu" CACHE STRING "path to the `iwyu` executable (if not on the PATH)")
set(OSMV_REPO_PROVIDER "https://github.com" CACHE STRING "URL (or local path, if you setup your mirrors properly) to a git repo provider for external projects")
set(OSMV_ENABLE_ASSERTS OFF CACHE BOOL "enable asserts, even if building a release build")
set(OSMV_USE_ASAN OFF CACHE BOOL "enable address sanitizer: useful for spotting leaks when debugging")

if(${OSMV_USING_MODERN_CMAKE})
    cmake_policy(SET CMP0072 NEW)  # use newer OpenGL linking strategy
endif()
set(OpenGL_GL_PREFERENCE "GLVND")  # use newer OpenGL linking strategy

if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a UNIX system that may not be supported yet - beware!")
    endif()
endif()



# --------------- dependency management ----------------- #

# (all handled from a single CMake file that declares targets etc.)
include(OSMV_GetDependencies.cmake)



# ------------ osmv build target configuration ---------- #

# osmv_config.hpp:
#     configuration file that contains configuration-time vals (e.g. version, libs)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/osmv_config.hpp.in" "osmv_config.hpp")

add_executable(osmv
    ${CMAKE_BINARY_DIR}/osmv_config.hpp

    src/os.hpp
    src/os.cpp
    src/sdl_wrapper.hpp
    src/sdl_wrapper.cpp
    src/shims.hpp
    src/algs.hpp
    src/config.hpp
    src/config.cpp

    src/3d/gl.hpp
    src/3d/gl.cpp
    src/3d/3d_common.hpp
    src/3d/3d_common.cpp
    src/3d/raw_mesh_instance.hpp
    src/3d/raw_drawlist.hpp
    src/3d/raw_drawlist.cpp
    src/3d/raw_renderer.hpp
    src/3d/raw_renderer.cpp
    src/3d/labelled_model_drawlist.hpp
    src/3d/model_drawlist_generator.hpp
    src/3d/model_drawlist_generator.cpp
    src/3d/polar_camera.hpp
    src/3d/polar_camera.cpp

    src/widgets/component_hierarchy_widget.hpp
    src/widgets/component_hierarchy_widget.cpp
    src/widgets/component_selection_widget.hpp
    src/widgets/component_selection_widget.cpp
    src/widgets/model_viewer_widget.hpp
    src/widgets/model_viewer_widget.cpp

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

    src/opensim_wrapper.hpp
    src/opensim_wrapper.cpp
    src/fd_simulation.hpp
    src/fd_simulation.cpp
    src/application.hpp
    src/application.cpp
    src/osmv.cpp
)
target_link_libraries(osmv PRIVATE
    # in gcc/clang the <filesystem> implementation must be explicitly linked
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>

    osmv-all-dependencies  # see: OSMV_GetDependencies.cmake
)
target_include_directories(osmv PRIVATE
    ${PROJECT_BINARY_DIR}  # for osmv_config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}  # for "src/<path>"
)
target_compile_options(osmv PRIVATE

    # msvc flags
    $<$<CXX_COMPILER_ID:MSVC>:
        /permissive-  # I like `and` and `not` keywords: sue me
        /wd4996  # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4455  # disable warning 4455: incorrectly flags std::string::literals::"s"
        /W3  # turn warning level up to "production grade" (note: /W4 explodes because of Simbody)
        /WX  # treat all warnings as errors
        /Oy-  # keep frame pointers around
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

        # regardless of debug/release, pin the frame pointer register
        # so that stack traces are sane when debugging (even in Release).
        #
        # This adds some overhead (pins one register and requires callers
        # to setup their base pointers etc.) but makes debugging + profiling
        # the application much easier, even in release mode
        -fno-omit-frame-pointer

        # use address sanitizer when building in Debug mode, so that invalid
        # memory accesses are spotted during development
        $<$<BOOL:${OSMV_USE_ASAN}>:-fsanitize=address>
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
    $<$<NOT:$<BOOL:${OSMV_USING_MODERN_CMAKE}>>:-std=c++17>
)
if(OSMV_ENABLE_ASSERTS)
    # MSVC
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    # others
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()
set_target_properties(osmv PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED YES
    EXPORT_COMPILE_COMMANDS ON  # needed by clang-tidy
    CLANG_TIDY $<${OSMV_USE_LINTER}:"clang-tidy">
)
if(OSMV_USE_IWYU)
    set_target_properties(osmv PROPERTIES INCLUDE_WHAT_YOU_USE "${OSMV_IWYU_PATH}")
endif()
if(LINUX)
    # Linux RPATH:
    #     - at installation time, Linux install dir is:
    #         bin/
    #         lib/*.so
    #     - so set the install RPATH to reflect this
    set_target_properties(osmv PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
endif()
if(APPLE)
    # Apple RPATH:
    #     - at installation time, Apple install dir is something like:
    #
    #           lib/*.dylib
    #           resources/*
    #           osmv (exe)
    #           osmv.toml
    set_target_properties(osmv PROPERTIES INSTALL_RPATH "@executable_path/lib")
endif()

if(${OSMV_USING_MODERN_CMAKE})
    # this is the "modern" way to explain CXX conformance
    target_compile_features(osmv PUBLIC cxx_std_17)
endif()

target_link_options(osmv PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        /SUBSYSTEM:windows  # open as a desktop app, not CLI
        /ENTRY:mainCRTStartup  # as above
        /ignore:4217  # ImGui uses locally defined symbols
    >

    $<$<AND:$<BOOL:LINUX>,$<CXX_COMPILER_ID:GNU>>:
        # make `ld` emit symbols for all functions - even if they aren't
        # externally used (with an exe, only `main` is used, really)
        #
        # this makes the binary a little bigger (~400 KB) but means
        # that the application can emit its own stack trace /w function
        # names - even in Release mode.
        -rdynamic

        # use address sanitizer when building in Debug mode, so that invalid
        # memory accesses are spotted during development
        $<$<BOOL:${OSMV_USE_ASAN}>:-fsanitize=address>
    >
)


# -------------- development nice-to-haves ------------- #

# generate a dev-centric `osmv.toml`
#
#     - this causes osmv to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DEV_osmv.toml.in" "${CMAKE_BINARY_DIR}/osmv.toml")

# (on Windows): copy libraries into the same dir as osmv.exe
#
#     - this is so that devs can run osmv.exe from an IDE without having to
#       copy libraries around manually etc.
#
#     - this isn't necessary with Unix'es because CMake sets the RPATH up to
#       point to wherever the libraries are located
#
#     - at install time, the libraries are copied into the install location
if (WIN32)
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        add_custom_command(
            TARGET osmv
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_FILE_TO_COPY}" "$<TARGET_FILE_DIR:osmv>"
        )
    endforeach()
endif()



# ---------- installation / packaging (CPack) ----------- #

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${osmv_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${osmv_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_DESCRIPTION "A thin GUI for OpenSim - primarily for development use")

# Windows install/packaging:
#
#     - copy osmv.exe + all DLLs into the bin/ dir
#
#     - this creates a "fat" build in bin/, where all the necessary libraries are just
#       packaged with the install (so users only have to install one thing)
#
#     - see: https://stackoverflow.com/questions/44909846/cmake-exe-cant-find-dll
#
#     - packaging: uses NSIS.exe : get it from https://nsis.sourceforge.io/Download
if (WIN32)
    # install osmv.exe
    install(TARGETS osmv)

    # install DLLs
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION bin)
    endforeach()

    # install a user-centric `osmv.toml`
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osmv executable (../resources)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL_osmv.toml" RENAME "osmv.toml" DESTINATION ".")

    # install `resources/`
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

    # set up NSIS packaging
    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.ico")
    set(CPACK_NSIS_UNINSTALL_NAME "uninstall-osmv")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_HELP_LINK "https://github.com/adamkewley/osmv")
    set(CPACK_NSIS_CONTACT "contact@adamkewley.com")
    set(CPACK_NSIS_MODIFY_PATH ON)  # user can modify install locaton
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN osmv)
    set(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\osmv ${osmv_VERSION_MAJOR}.${osmv_VERSION_MINOR}.${osmv_VERSION_PATCH}\\\\osmv.lnk' '\$INSTDIR\\\\bin\\\\osmv.exe'")
endif()

# Linux install/packaging:
#
#     - copy osmv + libraries into a standard-layout dir
#           bin/  # executables
#           lib/  # libraries (.so)
#           resources/  # arch-independent runtime resources
#
#     - this dir is "done", and could be sent as a ZIP to an end-user
#
#     - **BUT** the way that OpenSim/osmv is built assumes the user has all the
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
    # install osmv
    install(TARGETS osmv DESTINATION bin/)

    # install *bundled* SOs
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION lib/)
    endforeach()

    # install a user-centric `osmv.toml`
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osmv executable (../resources)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL_osmv.toml" RENAME "osmv.toml" DESTINATION ".")

    # install `resources/`
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION ".")

    # set up DEB packaging
    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osmv)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libblas3, liblapack3, libstdc++6")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    configure_file("${PROJECT_SOURCE_DIR}/postinst.in" "postinst" @ONLY)  # create symlink /usr/local/bin/osmv --> /opt/osmv/bin/osmv
    configure_file("${PROJECT_SOURCE_DIR}/postrm.in" "postrm" @ONLY)  # destroy symlink created during install
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")
endif()

# Apple OSX packaging:
#
# - Create a DMG (archive) installer that packages the whole application +
#   libraries into a single directory tree that can be copied to
#   /Applications/osmv
if (APPLE)
    # install main executable into package root (Application/ dir requirement)
    install(TARGETS osmv DESTINATION osmv.app/Contents/MacOS/)

    # install bundled libraries into lib/
    foreach(LIB_FILE_TO_COPY ${OSMV_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION osmv.app/Contents/MacOS/lib/)
    endforeach()

    # install a user-centric `osmv.toml`
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osmv executable (../resources)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL_osmv.toml" RENAME "osmv.toml" DESTINATION osmv.app/Contents/MacOS/)

    # install an Info.plist file for this particular app
    #
    # this is a mac-specific XML file
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" DESTINATION osmv.app/Contents/)

    # install `resources/`
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION osmv.app/Contents/MacOS/)

    # install desktop icon (.icns)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/osmv.icns" DESTINATION osmv.app/Contents/Resources/)

    set(CPACK_GENERATOR DragNDrop)
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
