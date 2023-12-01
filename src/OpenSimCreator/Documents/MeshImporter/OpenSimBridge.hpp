#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.hpp>
#include <OpenSimCreator/Documents/MeshImporter/OpenSimExportFlags.hpp>

#include <oscar/Maths/Vec3.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace OpenSim { class Model; }
namespace osc::mi { class Joint; }

// OpenSim::Model generation support
//
// the ModelGraph that this UI manipulates ultimately needs to be transformed
// into a standard OpenSim model. This code does that.
namespace osc::mi
{
    Document CreateModelFromOsimFile(std::filesystem::path const&);
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromModelGraph(
        Document const&,
        ModelCreationFlags,
        std::vector<std::string>& issuesOut
    );
    Vec3 GetJointAxisLengths(Joint const&);
}
