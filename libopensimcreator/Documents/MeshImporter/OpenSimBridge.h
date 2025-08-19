#pragma once

#include <libopensimcreator/Documents/MeshImporter/Document.h>
#include <libopensimcreator/Documents/MeshImporter/OpenSimExportFlags.h>

#include <liboscar/Maths/Vector3.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace OpenSim { class Model; }
namespace osc::mi { class Joint; }

// functions that bridge the mesh importer world to the OpenSim one
namespace osc::mi
{
    Document CreateModelFromOsimFile(const std::filesystem::path&);

    // if there are no issues, returns a new OpenSim::Model created from the document
    //
    // otherwise, returns nullptr and issuesOut will be populated with issue messages
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromMeshImporterDocument(
        const Document&,
        ModelCreationFlags,
        std::vector<std::string>& issuesOut
    );

    Vector3 GetJointAxisLengths(const Joint&);
}
