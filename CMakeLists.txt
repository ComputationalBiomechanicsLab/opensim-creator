cmake_minimum_required(VERSION 3.5)

project(osc VERSION 0.0.7 LANGUAGES C CXX)


# -------------- gather user-facing build config options ---------------- #

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
    "force disable NDEBUG, even if running in Release mode. This enables *a lot* of runtime assertions (standard lib, SimTK, OpenSim)"
)
set(
    OSC_USE_ASAN OFF
    CACHE BOOL
    "enable address sanitizer (libASAN): useful for spotting memory access violations and leaks"
)
set(
    OSC_DEFAULT_USE_MULTI_VIEWPORT OFF
    CACHE BOOL
    "enable/disable osc using multi viewport when not explicitly specified a the user/config"
)
set(
    OSC_DEFAULT_RESOURCE_DIR "../resources"
    CACHE STRING
    "default location of runtime resources dir when not specified by user/config"
)
set(
    OSC_BUILD_DOCS OFF
    CACHE BOOL
    "whether to build the documentation or not (requires sphinx-build is available on the PATH)"
)


# -------------- top-level build configuration (vars etc.) ---------------- #

# set OSC_GENERATOR_IS_MULTI_CONFIG
#
# effectively, tests if the user is using a multi-config generator, like
# Visual Studio, which lets developers change their build configuration
# (Release, Debug) in a single configuration
#
# this can affect things like deciding which/where to link libraries
get_property(OSC_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# set LINUX
#
# this is based on whether the UNIX variant being used appears to be LINUX
if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a UNIX system that may not be supported yet - beware!")
    endif()
endif()

# set OSC_BUILD_ID
#
# this is so that errors in the binary can potentially be linked to the source
# that produced the binary later on (user errors, etc.)
if(DEFINED ENV{GITHUB_RUN_ID})

    # build is running inside GitHub Action CI - use that ID
    set(OSC_BUILD_ID "GITHUB_$ENV{GITHUB_RUN_ID}.$ENV{GITHUB_RUN_NUMBER}")
else()

    # build is probably running on a dev's machine - generate a random ID
    string(RANDOM RANDOM_ID)
    set(OSC_BUILD_ID "CUSTOM_${RANDOM_ID}")
    unset(RANDOM_ID)
endif()

# set OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL
#
# this is a C-like boolean that tells OSC whether to use a multi-viewport
# mode
if(${OSC_DEFAULT_USE_MULTI_VIEWPORT})
    set(OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL "true")
else()
    set(OSC_DEFAULT_USE_MULTI_VIEWPORT_CBOOL "false")
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

# use an older OpenGL linking strategy
#
# (this seems to be necessary on some systems: need to investigate why)
cmake_policy(SET CMP0072 OLD)
set(OpenGL_GL_PREFERENCE "LEGACY")


# ------------------ configure dependencies -------------------- #

# all dependency management is handled from a separate CMake file that exports
# the dependencies as targets - along with some other useful vars (e.g. a list
# of all dependencies that should be copied into the install, etc.)
include(GetDependencies.cmake)


# --------------- configure `osc` build target ----------------- #

# osc_config.hpp:
#
#     configuration file that contains configuration-time vals (e.g. version, libs)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/osc_config.hpp.in" "osc_config.hpp")

# osc
#
# made from the usual ball of hpp and cpp files
add_executable(osc
    ${CMAKE_BINARY_DIR}/osc_config.hpp

    src/3D/Shaders/ColormappedPlainTextureShader.cpp
    src/3D/Shaders/ColormappedPlainTextureShader.hpp
    src/3D/Shaders/EdgeDetectionShader.cpp
    src/3D/Shaders/EdgeDetectionShader.hpp
    src/3D/Shaders/GouraudMrtShader.cpp
    src/3D/Shaders/GouraudMrtShader.hpp
    src/3D/Shaders/GouraudShader.cpp
    src/3D/Shaders/GouraudShader.hpp
    src/3D/Shaders/InstancedGouraudColorShader.cpp
    src/3D/Shaders/InstancedGouraudColorShader.hpp
    src/3D/Shaders/InstancedSolidColorShader.cpp
    src/3D/Shaders/InstancedSolidColorShader.hpp
    src/3D/Shaders/NormalsShader.cpp
    src/3D/Shaders/NormalsShader.hpp
    src/3D/Shaders/PlainTextureShader.cpp
    src/3D/Shaders/PlainTextureShader.hpp
    src/3D/Shaders/SkipMSXAABlitterShader.cpp
    src/3D/Shaders/SkipMSXAABlitterShader.hpp
    src/3D/Shaders/SolidColorShader.cpp
    src/3D/Shaders/SolidColorShader.hpp

    src/3D/BVH.cpp
    src/3D/BVH.hpp
    src/3D/Constants.hpp
    src/3D/Gl.cpp
    src/3D/Gl.hpp
    src/3D/GlGlm.hpp
    src/3D/InstancedRenderer.cpp
    src/3D/InstancedRenderer.hpp
    src/3D/Mesh.cpp
    src/3D/Mesh.hpp
    src/3D/Model.cpp
    src/3D/Model.hpp
    src/3D/Shader.hpp
    src/3D/ShaderCache.cpp
    src/3D/ShaderCache.hpp
    src/3D/ShaderLocationIndex.hpp
    src/3D/Texturing.cpp
    src/3D/Texturing.hpp

    src/OpenSimBindings/FileChangePoller.cpp
    src/OpenSimBindings/FileChangePoller.hpp
    src/OpenSimBindings/OpenSimHelpers.hpp
    src/OpenSimBindings/OpenSimHelpers.cpp
    src/OpenSimBindings/PlottableOutputSubfield.cpp
    src/OpenSimBindings/PlottableOutputSubfield.hpp
    src/OpenSimBindings/RenderableScene.cpp
    src/OpenSimBindings/RenderableScene.hpp
    src/OpenSimBindings/Simulation.cpp
    src/OpenSimBindings/Simulation.hpp
    src/OpenSimBindings/StateModifications.cpp
    src/OpenSimBindings/StateModifications.hpp
    src/OpenSimBindings/TypeRegistry.cpp
    src/OpenSimBindings/TypeRegistry.hpp
    src/OpenSimBindings/UiModel.cpp
    src/OpenSimBindings/UiModel.hpp
    src/OpenSimBindings/UiSimulation.cpp
    src/OpenSimBindings/UiSimulation.hpp
    src/OpenSimBindings/UndoableUiModel.cpp
    src/OpenSimBindings/UndoableUiModel.hpp

    src/Screens/Experimental/CookiecutterScreen.cpp
    src/Screens/Experimental/CookiecutterScreen.hpp
    src/Screens/Experimental/ExperimentsScreen.cpp
    src/Screens/Experimental/ExperimentsScreen.hpp
    src/Screens/Experimental/HelloTriangleScreen.cpp
    src/Screens/Experimental/HelloTriangleScreen.hpp
    src/Screens/Experimental/HittestScreen.cpp
    src/Screens/Experimental/HittestScreen.hpp
    src/Screens/Experimental/ImGuiDemoScreen.cpp
    src/Screens/Experimental/ImGuiDemoScreen.hpp
    src/Screens/Experimental/ImGuizmoDemoScreen.cpp
    src/Screens/Experimental/ImGuizmoDemoScreen.hpp
    src/Screens/Experimental/InstancedRendererScreen.cpp
    src/Screens/Experimental/InstancedRendererScreen.hpp
    src/Screens/Experimental/LayeredInterfaceScreen.cpp
    src/Screens/Experimental/LayeredInterfaceScreen.hpp
    src/Screens/Experimental/MathExperimentsScreen.cpp
    src/Screens/Experimental/MathExperimentsScreen.hpp
    src/Screens/Experimental/MeshHittestScreen.cpp
    src/Screens/Experimental/MeshHittestScreen.hpp
    src/Screens/Experimental/MeshHittestWithBVHScreen.cpp
    src/Screens/Experimental/MeshHittestWithBVHScreen.hpp
    src/Screens/Experimental/MeshScreen.cpp
    src/Screens/Experimental/MeshScreen.hpp
    src/Screens/Experimental/UiModelViewerScreen.cpp
    src/Screens/Experimental/UiModelViewerScreen.hpp

    src/Screens/ErrorScreen.cpp
    src/Screens/ErrorScreen.hpp
    src/Screens/LoadingScreen.cpp
    src/Screens/LoadingScreen.hpp
    src/Screens/MeshImporterScreen.cpp
    src/Screens/MeshImporterScreen.hpp
    src/Screens/ModelEditorScreen.cpp
    src/Screens/ModelEditorScreen.hpp
    src/Screens/SimulatorScreen.cpp
    src/Screens/SimulatorScreen.hpp
    src/Screens/SplashScreen.cpp
    src/Screens/SplashScreen.hpp

    src/SimTKBindings/SceneGeneratorNew.cpp
    src/SimTKBindings/SceneGeneratorNew.hpp
    src/SimTKBindings/SimTKConverters.hpp
    src/SimTKBindings/SimTKLoadMesh.cpp
    src/SimTKBindings/SimTKLoadMesh.hpp

    src/UI/AddBodyPopup.cpp
    src/UI/AddBodyPopup.hpp
    src/UI/AddComponentPopup.cpp
    src/UI/AddComponentPopup.hpp
    src/UI/AttachGeometryPopup.cpp
    src/UI/AttachGeometryPopup.hpp
    src/UI/ComponentDetails.cpp
    src/UI/ComponentDetails.hpp
    src/UI/ComponentHierarchy.cpp
    src/UI/ComponentHierarchy.hpp
    src/UI/CoordinateEditor.cpp
    src/UI/CoordinateEditor.hpp
    src/UI/F3Editor.cpp
    src/UI/F3Editor.hpp
    src/UI/FdParamsEditorPopup.cpp
    src/UI/FdParamsEditorPopup.hpp
    src/UI/LogViewer.cpp
    src/UI/LogViewer.hpp
    src/UI/MainMenu.cpp
    src/UI/MainMenu.hpp
    src/UI/ModelActionsMenuBar.cpp
    src/UI/ModelActionsMenuBar.hpp
    src/UI/PropertyEditors.cpp
    src/UI/PropertyEditors.hpp
    src/UI/ReassignSocketPopup.hpp
    src/UI/ReassignSocketPopup.cpp
    src/UI/Select1PFPopup.cpp
    src/UI/Select1PFPopup.hpp
    src/UI/Select2PFsPopup.cpp
    src/UI/Select2PFsPopup.hpp
    src/UI/SelectComponentPopup.hpp
    src/UI/UiModelViewer.cpp
    src/UI/UiModelViewer.hpp

    src/Utils/Algorithms.cpp
    src/Utils/Algorithms.hpp
    src/Utils/CheckedIndex.hpp
    src/Utils/CircularBuffer.hpp
    src/Utils/ClonePtr.hpp
    src/Utils/ConcurrencyHelpers.hpp
    src/Utils/Cpp20Shims.hpp
    src/Utils/DefaultConstructOnCopy.hpp
    src/Utils/FilesystemHelpers.cpp
    src/Utils/FilesystemHelpers.hpp
    src/Utils/ImGuiHelpers.cpp
    src/Utils/ImGuiHelpers.hpp
    src/Utils/IoPoller.cpp
    src/Utils/IoPoller.hpp
    src/Utils/MethodTestMacro.hpp
    src/Utils/Perf.hpp
    src/Utils/ScopeGuard.hpp
    src/Utils/Sdl2Bindings.hpp
    src/Utils/Spsc.hpp
    src/Utils/UID.cpp
    src/Utils/UID.hpp

    src/App.cpp
    src/App.hpp
    src/Assertions.cpp
    src/Assertions.hpp
    src/Config.cpp
    src/Config.hpp
    src/Log.cpp
    src/Log.hpp
    src/MeshCache.cpp
    src/MeshCache.hpp
    src/MainEditorState.cpp
    src/MainEditorState.hpp
    src/RecentFile.hpp
    src/Screen.hpp
    src/Styling.hpp
    src/os.cpp
    src/os.hpp
    src/osc.cpp

    # Windows: also link a resources file (rc)
    #
    # the resources file tells MSVC compiler how to compile non-source resources
    # into the output exe. Specifically, it's used to embed the application icon
    # into the `osc` exe
    $<$<CXX_COMPILER_ID:MSVC>:build_resources/resources.rc>
)

# osc: compile-time includes
target_include_directories(osc PRIVATE

    # so `#include "osc_config.hpp"` works
    ${PROJECT_BINARY_DIR}

    # so `#include "src/<path>.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# osc: compile options
target_compile_options(osc PRIVATE

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:
        # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4996

        # disable warning 4455: incorrectly flags std::string::literals::"s"
        /wd4455

        # turn warning level up to "production grade" (note: /W4 explodes because of Simbody)
        /W3

        # treat all warnings as errors
        /WX

        # keep frame pointers around
        /Oy-

        # newer MSVCs may also come with LIBASAN (experimental)
        $<$<BOOL:${OSC_USE_ASAN}>:/fsanitize=address>
    >

    # gcc AND clang flags
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

    # clang flags
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

    # gcc flags
    $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused  # disable: broken by STB headers
    >
)

# osc: compile features
target_compile_features(osc PUBLIC cxx_std_17)

# osc: link libraries
target_link_libraries(osc PRIVATE

    # in earlier gcc/clang the <filesystem> implementation must be explicitly linked
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>

    # see: GetDependencies.cmake
    osc-all-deps
)

# osc: link options
target_link_options(osc PUBLIC

    # MSVC (Windows)
    $<$<CXX_COMPILER_ID:MSVC>:

        # open as a desktop app, not CLI
        /SUBSYSTEM:windows

        # as above, call into `main`
        /ENTRY:mainCRTStartup

        # ignore error from ImGui, which uses locally-defined symbols
        /ignore:4217
    >

    # Linux /w GCC or Clang
    $<$<AND:$<BOOL:LINUX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:

        # make `ld` emit symbols for all functions - even if they aren't
        # externally used (with an exe, only `main` is used, really)
        #
        # this makes the binary a little bigger (~400 KB) but means
        # that the application can emit its own stack trace /w function
        # names - even in Release mode.
        -rdynamic
    >

    # GCC or Clang: handle OSC_USE_ASAN
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >
)

# osc: handle OSC_FORCE_ASSERTS_ENABLED
if(OSC_FORCE_ASSERTS_ENABLED)
    target_compile_definitions(osc PRIVATE -DOSC_FORCE_ASSERTS_ENABLED=1)
endif()

# osc: handle OSC_FORCE_UNDEFINE_NDEBUG
if(OSC_FORCE_UNDEFINE_NDEBUG)
    # MSVC
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    # others
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# osc: handle LINUX RPATH
#
# - at installation time, Linux install dir is:
#
#     bin/
#     lib/*.so
if(LINUX)
    set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
endif()

# osc: handle APPLE @executable_path
#
# - at installation time, Apple install dir is something like:
#
#    lib/*.dylib
#    resources/*
#    osc (exe)
#    osc.toml
if(APPLE)
    set_target_properties(osc PROPERTIES INSTALL_RPATH "@executable_path/lib")
endif()

# osc: target properties
set_target_properties(osc PROPERTIES

    # disable compiler-specific language extensions
    CXX_EXTENSIONS OFF

    # strictly require the requested C++ standard (e.g. C++17)
    CXX_STANDARD_REQUIRED YES

    # export compile commands, so clang-tidy works correctly
    EXPORT_COMPILE_COMMANDS ON
)


# --------------- configure documentation target ------------------ #

# configure documentation generation
#
# if the user sets OSC_BUILD_DOCS then they *require* documentation
# to be generated. Documentation is written in `sphinx`, which should
# be tested here, and the documentation should be generated at
# install-time
if(OSC_BUILD_DOCS)

    # configure time: ensure the user's build system has 'sphinx-build'
    find_program(
        OSC_SPHINX_EXE
        "sphinx-build"
        DOC "Location of sphinx-build, which is used to compile the documentation"
        REQUIRED
    )

    # install time: run `sphinx-build` to build the docs
    install(
        CODE "execute_process(COMMAND ${OSC_SPHINX_EXE} -M html source build WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/docs\")"
    )

    # install time: copy the build docs to the appropriate packaging dir
    if(APPLE)

        # APPLE's `DOC` packaging location is (imho) wrong for DMG packaging.
        # It puts the docs outside the DMG in the user-facing drag and drop
        # GUI? So users are presented with 'osc' and 'share', rather than just
        # 'osc' (ewww)
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/build/html
            DESTINATION osc.app/Contents/MacOS/share/doc
        )

    else()
        # on other OSes, just use the CMake `DOC` default, which typically just
        # bundles it in $INSTALL_LOC/share/doc/html
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/build/html
            TYPE DOC
        )
    endif()
endif()


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

# (on Windows): copy runtime DLLs into the `osc.exe` build dir
#
#     - this is so that devs can run `osc.exe` from an IDE without having to
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


# ------------- installation / packaging (CPack) -------------- #

# install-time: also package any required system libraries
include(InstallRequiredSystemLibraries)

# install-time: set necessary CPack variables
set(CPACK_PACKAGE_NAME "OpenSimCreator")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "Adam Kewley")
set(CPACK_PACKAGE_CONTACT "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/adamkewley/opensim-creator")
set(CPACK_PACKAGE_DESCRIPTION "A standalone GUI for building OpenSim models")
set(CPACK_PACKAGE_EXECUTABLES "osc;OpenSimCreator")

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

    # install-time: install osc.exe
    install(TARGETS osc)

    # install-time: install all dependency DLLs next to the exe
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION bin)
    endforeach()

    # install-time: install a user-facing `osc.toml` config file
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
        RENAME "osc.toml"
        DESTINATION "."
    )

    # install-time: copy `resources/` (assets) dir
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        DESTINATION "."
    )

    # packaging: use NSIS to package everything into a self-extracting installer
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenSimCreator")
    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.ico")
    set(CPACK_NSIS_UNINSTALL_NAME "UninstallOpenSimCreator")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_HELP_LINK "https://github.com/adamkewley/opensim-creator")
    set(CPACK_NSIS_CONTACT "contact@adamkewley.com")
    # do not prompt the user to modify the PATH
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN osc)
    #set(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\OpenSimCreator\\\\osc.lnk' '\$INSTDIR\\\\bin\\\\osc.exe'")
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
    # install-time: install osc executable
    install(TARGETS osc DESTINATION bin/)

    # install-time: install all dependency `.so` files into `lib/`
    #
    # the RPATH stuff ensures that the exe can find these at runtime
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION lib/)
    endforeach()

    # install-time: install a user-facing `osc.toml` config file
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
        RENAME "osc.toml"
        DESTINATION "."
    )

    # install-time: copy `resources/` (assets) dir
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        DESTINATION
        "."
    )

    # packaging: package installation as a DEB
    set(CPACK_GENERATOR DEB)
    set(CPACK_PACKAGING_INSTALL_PREFIX /opt/osc)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libblas3, liblapack3, libstdc++6")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # packaging: configure a script that creates a symlink /usr/local/bin/osc --> /opt/osc/bin/osc
    configure_file("${PROJECT_SOURCE_DIR}/build_resources/postinst.in" "postinst" @ONLY)

    # packaging: configure a script that destroys the above symlink on uninstall
    configure_file("${PROJECT_SOURCE_DIR}/build_resources/postrm.in" "postrm" @ONLY)

    # packaging: tell debian packager to use the scripts for postinst and postrm actions
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")
endif()

# Apple OSX packaging:
#
# - Create a DMG (archive) installer that packages the whole application +
#   libraries into a single directory tree that can be copied to
#   /Applications/osc
#
if (APPLE)

    # install-time: install osc into package root (Application/ dir requirement)
    install(TARGETS osc DESTINATION osc.app/Contents/MacOS/)

    # install-time: install bundled libraries into lib/
    foreach(LIB_FILE_TO_COPY ${OSC_LIB_FILES_TO_COPY})
        install(FILES "${LIB_FILE_TO_COPY}" DESTINATION osc.app/Contents/MacOS/lib/)
    endforeach()

    # install-time: install a user-facing `osc.toml` config file
    #
    #     - in contrast to the dev-centric one, this loads resources from the installation dir,
    #       which has a known path relative to the osc executable (../resources)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/INSTALL_osc.toml"
        RENAME "osc.toml"
        DESTINATION osc.app/Contents/MacOS/
    )

    # install-time: install an `Info.plist` file
    #
    # it's mac-specific XML file that tells Mac OSX about where the
    # executable is, what the icon is, etc.
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_resources/Info.plist"
        DESTINATION osc.app/Contents/
    )

    # install-time: copy `resources/` (assets) dir
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        DESTINATION osc.app/Contents/MacOS/
    )

    # install-time: copy the Mac-specific desktop icon (.icns)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/osc.icns"
        DESTINATION osc.app/Contents/Resources/
    )

    set(CPACK_GENERATOR DragNDrop)
endif()

# CPack vars etc. now fully configured, so include it
include(CPack)
