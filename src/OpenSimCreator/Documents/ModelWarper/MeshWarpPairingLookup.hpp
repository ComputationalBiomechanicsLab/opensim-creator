#pragma once

#include <OpenSimCreator/Documents/ModelWarper/MeshWarpPairing.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace OpenSim { class Model; }

namespace osc::mow
{
    class MeshWarpPairingLookup final {
    public:
        MeshWarpPairingLookup() = default;
        MeshWarpPairingLookup(
            std::filesystem::path const& modelFileLocation,
            OpenSim::Model const&
        );

        MeshWarpPairing const* lookup(std::string const& meshComponentAbsPath) const
        {
            if (auto const it = m_ComponentAbsPathToMeshPairing.find(meshComponentAbsPath); it != m_ComponentAbsPathToMeshPairing.end())
            {
                return &it->second;
            }
            else
            {
                return nullptr;
            }
        }
    private:
        std::unordered_map<std::string, MeshWarpPairing> m_ComponentAbsPathToMeshPairing;
    };
}
