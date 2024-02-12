#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.h>
#include <OpenSimCreator/Documents/MeshImporter/OpenSimExportFlags.h>

#include <oscar/Maths/Vec3.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace OpenSim { class Model; }
namespace osc::mi { class Joint; }

// functions that bridge the mesh importer world to the OpenSim one
namespace osc::mi
{
    Document CreateModelFromOsimFile(std::filesystem::path const&);

    // if there are no issues, returns a new OpenSim::Model created from the document
    //
    // otherwise, returns nullptr and issuesOut will be populated with issue messages
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromMeshImporterDocument(
        Document const&,
        ModelCreationFlags,
        std::vector<std::string>& issuesOut
    );

    Vec3 GetJointAxisLengths(Joint const&);
}
