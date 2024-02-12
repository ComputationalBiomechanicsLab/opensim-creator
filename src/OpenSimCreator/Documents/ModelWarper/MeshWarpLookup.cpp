#include "MeshWarpLookup.h"

#include <OpenSimCreator/Documents/ModelWarper/IMeshWarp.h>
#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/Log.h>

#include <filesystem>
#include <sstream>
#include <utility>

using namespace osc;
using namespace osc::mow;

namespace
{
    std::unordered_map<std::string, ClonePtr<IMeshWarp>> CreateLut(
        std::filesystem::path const& modelFileLocation,
        OpenSim::Model const& model)
    {
        std::unordered_map<std::string, ClonePtr<IMeshWarp>> rv;
        rv.reserve(GetNumChildren<OpenSim::Mesh>(model));

        // go through each mesh in the `OpenSim::Model` and attempt to load its landmark pairings
        for (auto const& mesh : model.getComponentList<OpenSim::Mesh>())
        {
            if (auto meshPath = FindGeometryFileAbsPath(model, mesh))
            {
                rv.try_emplace(
                    mesh.getAbsolutePathString(),
                    std::make_unique<MeshWarpPairing>(modelFileLocation, std::move(meshPath).value())
                );
            }
            else
            {
                std::stringstream ss;
                ss << mesh.getGeometryFilename() << ": could not find this mesh file: skipping";
                log_error(std::move(ss).str());
            }
        }

        return rv;
    }
}

osc::mow::MeshWarpLookup::MeshWarpLookup(
    std::filesystem::path const& osimFileLocation,
    OpenSim::Model const& model,
    ModelWarpConfiguration const&) :

    m_ComponentAbsPathToMeshPairing{CreateLut(osimFileLocation, model)}
{}
