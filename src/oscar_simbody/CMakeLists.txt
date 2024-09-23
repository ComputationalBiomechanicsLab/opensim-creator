find_package(Simbody REQUIRED CONFIG)

add_library(oscar_simbody STATIC
    UI/MeshHittestTab.cpp
    UI/MeshHittestTab.h
    UI/OscarSimbodyTabRegistry.cpp
    UI/OscarSimbodyTabRegistry.h
    UI/OscarSimbodyTabs.h
    UI/RendererGeometryShaderTab.cpp
    UI/RendererGeometryShaderTab.h
    UI/TPS2DTab.cpp
    UI/TPS2DTab.h

    LandmarkPair3D.cpp
    LandmarkPair3D.h
    ShapeFitters.cpp
    ShapeFitters.h
    SimTKDecorationGenerator.cpp
    SimTKDecorationGenerator.h
    SimTKHelpers.cpp
    SimTKHelpers.h
    SimTKMeshLoader.cpp
    SimTKMeshLoader.h
    TPS3D.cpp
    TPS3D.h
)

target_include_directories(oscar_simbody PUBLIC

    # so that the source code can `#include <OpenSimCreator/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_simbody PUBLIC
    oscar_compiler_configuration
    oscar

    SimTKcommon
    SimTKmath
    SimTKsimbody
)

target_compile_options(oscar_simbody PRIVATE
    # gcc
    $<$<CXX_COMPILER_ID:GNU>:
        # gcc-12 finds (possibly legit) bounds issues in simbody - requires investigation
        -Wno-array-bounds
    >
)

set_target_properties(oscar_simbody PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)
