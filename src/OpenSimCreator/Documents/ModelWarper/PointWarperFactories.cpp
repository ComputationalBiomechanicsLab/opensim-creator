#include "PointWarperFactories.h"

#include <OpenSimCreator/Documents/ModelWarper/IPointWarperFactory.h>
#include <OpenSimCreator/Documents/ModelWarper/TPSLandmarkPairWarperFactory.h>
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
    std::unordered_map<std::string, ClonePtr<IPointWarperFactory>> CreateLut(
        const std::filesystem::path& modelFileLocation,
        const OpenSim::Model& model)
    {
        std::unordered_map<std::string, ClonePtr<IPointWarperFactory>> rv;
        rv.reserve(GetNumChildren<OpenSim::Mesh>(model));

        // go through each mesh in the `OpenSim::Model` and attempt to load its landmark pairings
        for (const auto& mesh : model.getComponentList<OpenSim::Mesh>()) {
            if (auto meshPath = FindGeometryFileAbsPath(model, mesh)) {
                rv.try_emplace(
                    mesh.getAbsolutePathString(),
                    std::make_unique<TPSLandmarkPairWarperFactory>(modelFileLocation, std::move(meshPath).value())
                );
            }
            else {
                std::stringstream ss;
                ss << mesh.getGeometryFilename() << ": could not find this mesh file: skipping";
                log_error(std::move(ss).str());
            }
        }

        return rv;
    }
}

osc::mow::PointWarperFactories::PointWarperFactories(
    const std::filesystem::path& osimFileLocation,
    const OpenSim::Model& model,
    const ModelWarpConfiguration&) :

    m_AbsPathToWarpLUT{CreateLut(osimFileLocation, model)}
{}
