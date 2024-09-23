find_package(OpenSim REQUIRED CONFIG)

add_library(OpenSimThirdPartyPlugins STATIC

    opensim-jam-org/jam-plugin/Smith2018ArticularContactForce.cpp
    opensim-jam-org/jam-plugin/Smith2018ArticularContactForce.h
    opensim-jam-org/jam-plugin/Smith2018ContactMesh.cpp
    opensim-jam-org/jam-plugin/Smith2018ContactMesh.h

    osimPluginDLL.h
    RegisterTypes_osimPlugin.cpp
    RegisterTypes_osimPlugin.h
)

target_include_directories(OpenSimThirdPartyPlugins PUBLIC

    # so that the source code can `#include <OpenSimCreator/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(OpenSimThirdPartyPlugins PUBLIC
    osimCommon
    osimSimulation
    osimActuators
    osimAnalyses
    osimTools
    osimLepton
)

target_compile_features(OpenSimThirdPartyPlugins PRIVATE cxx_std_14)

set_target_properties(OpenSimThirdPartyPlugins PROPERTIES
    DEFINE_SYMBOL OSIMPLUGIN_EXPORTS
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)
