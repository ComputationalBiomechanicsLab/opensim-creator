#pragma once

#include <OpenSimCreator/Documents/ModelWarper/LandmarkPairing.hpp>

#include <filesystem>
#include <optional>
#include <vector>

namespace osc::mow
{
    class MeshWarpPairing final {
    public:
        MeshWarpPairing(
            std::filesystem::path const& modelFileLocation,
            std::filesystem::path const& sourceMeshLocation
        );
    private:
        std::filesystem::path m_SourceMeshDiskLocation;
        std::optional<std::filesystem::path> m_SourceLandmarksDiskLocation;

        std::optional<std::filesystem::path> m_DestinationDiskLocation;
        std::optional<std::filesystem::path> m_DestinationLandmarksDiskLocation;

        std::vector<LandmarkPairing> m_Landmarks;
    };
}
