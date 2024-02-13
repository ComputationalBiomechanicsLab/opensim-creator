#include "ModelWarpConfiguration.h"

#include <filesystem>

osc::mow::ModelWarpConfiguration::ModelWarpConfiguration(
    std::filesystem::path const&,
    OpenSim::Model const&)
{
    // TODO: go find the model warping configuration file on-disk "near" the osim
    //       file
}
