// #include <OpenSimThirdPartyPlugins/opensim-jam-org/jam-plugin/Smith2018ContactMesh.h>  // N/A: we load it via the `OpenSim::Object` registry

#include <TestOpenSimThirdPartyPlugins/TestOpenSimThirdPartyPluginsConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSimThirdPartyPlugins/RegisterTypes_osimPlugin.h>

#include <filesystem>

TEST(Smith2018ContactMesh, CanLoadAModelContainingASmith2018ContactMesh)
{
    RegisterTypes_osimSimulation();  // ensure `OpenSim::Ground` etc. basic types are available
    RegisterTypes_osimPlugin();  // ensure `OpenSim::Smith2018ContactMesh` is globally regstered

    std::filesystem::path fixturePath = std::filesystem::path{TESTOPENSIMTHIRDPARTYPLUGINS_RESOURCES_DIR} / "ContainsSmith2018ContactMesh.osim";

    OpenSim::Model model{fixturePath.string()};
    model.buildSystem();
}
