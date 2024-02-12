#pragma once

#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.h>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpConfiguration; }

namespace osc::mow
{
    class MeshWarpLookup final {
    public:
        MeshWarpLookup() = default;
        MeshWarpLookup(
            std::filesystem::path const& osimFileLocation,
            OpenSim::Model const&,
            ModelWarpConfiguration const&
        );

        MeshWarpPairing const* findPairing(std::string const& meshComponentAbsPath) const
        {
            if (auto const it = m_ComponentAbsPathToMeshPairing.find(meshComponentAbsPath);
                it != m_ComponentAbsPathToMeshPairing.end()) {

                return &it->second;
            }
            else {
                return nullptr;
            }
        }
    private:
        std::unordered_map<std::string, MeshWarpPairing> m_ComponentAbsPathToMeshPairing;
    };
}
