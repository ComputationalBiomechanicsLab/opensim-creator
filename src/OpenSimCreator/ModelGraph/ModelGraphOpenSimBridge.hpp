#pragma once

#include <OpenSimCreator/ModelGraph/ModelCreationFlags.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraph.hpp>

#include <oscar/Maths/Vec3.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { class JointEl; }

// OpenSim::Model generation support
//
// the ModelGraph that this UI manipulates ultimately needs to be transformed
// into a standard OpenSim model. This code does that.
namespace osc
{
    ModelGraph CreateModelFromOsimFile(std::filesystem::path const&);
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromModelGraph(
        ModelGraph const&,
        ModelCreationFlags,
        std::vector<std::string>& issuesOut
    );
    Vec3 GetJointAxisLengths(JointEl const&);
}
