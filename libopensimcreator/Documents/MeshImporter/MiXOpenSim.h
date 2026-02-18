#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiDocument.h>

#include <liboscar/maths/vector3.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { class MiJoint; }

// functions that bridge the mesh importer world to the OpenSim one
namespace osc
{
    // user-editable flags that affect how an OpenSim::Model is created by the mesh importer
    enum class ModelCreationFlags {
        None                      = 0,
        ExportStationsAsMarkers   = 1<<0,
    };

    constexpr bool operator&(const ModelCreationFlags& lhs, const ModelCreationFlags& rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    constexpr ModelCreationFlags operator+(const ModelCreationFlags& lhs, const ModelCreationFlags& rhs)
    {
        return static_cast<ModelCreationFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    constexpr ModelCreationFlags operator-(const ModelCreationFlags& lhs, const ModelCreationFlags& rhs)
    {
        return static_cast<ModelCreationFlags>(std::to_underlying(lhs) & ~std::to_underlying(rhs));
    }

    MiDocument CreateModelFromOsimFile(const std::filesystem::path&);

    // if there are no issues, returns a new OpenSim::Model created from the document
    //
    // otherwise, returns nullptr and issuesOut will be populated with issue messages
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromMeshImporterDocument(
        const MiDocument&,
        ModelCreationFlags,
        std::vector<std::string>& issuesOut
    );

    Vector3 GetJointAxisLengths(const MiJoint&);
}
