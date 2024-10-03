// #include <OpenSimThirdPartyPlugins/opensim-jam-org/jam-plugin/Smith2018ArticularContactForce.h>  // N/A: we load it via the `OpenSim::Object` registry

#include <TestOpenSimThirdPartyPlugins/TestOpenSimThirdPartyPluginsConfig.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimThirdPartyPlugins/RegisterTypes_osimPlugin.h>

#include <filesystem>

TEST(Smith2018ArticularContactForce, CanLoadModelFileContainingArticularForce)
{
    RegisterTypes_osimSimulation();  // ensure `OpenSim::Ground` etc. are available
    RegisterTypes_osimPlugin();  // ensure `OpenSim::Smith2018ArticularContactForce` is globally regstered

    std::filesystem::path fixturePath = std::filesystem::path{TESTOPENSIMTHIRDPARTYPLUGINS_RESOURCES_DIR} / "ContainsSmith2018ArticularContactForce.osim";

    OpenSim::Model model{fixturePath.string()};
    model.buildSystem();  // should work
}
