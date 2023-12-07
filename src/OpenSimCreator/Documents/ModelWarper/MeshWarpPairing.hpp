#pragma once

#include <OpenSimCreator/Documents/ModelWarper/LandmarkPairing.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

namespace osc::mow
{
    class MeshWarpPairing final {
    public:
        MeshWarpPairing(
            std::filesystem::path const& osimFilepath,
            std::filesystem::path const& sourceMeshFilepath
        );

        std::filesystem::path const& getSourceMeshAbsoluteFilepath() const
        {
            return m_SourceMeshAbsoluteFilepath;
        }

        bool hasSourceLandmarksFile() const
        {
            return m_SourceLandmarksAbsoluteFilepath.has_value();
        }

        std::optional<std::filesystem::path> const& tryGetSourceLandmarksFilepath() const
        {
            return m_SourceLandmarksAbsoluteFilepath;
        }

        std::optional<std::filesystem::path> const& tryGetDestinationMeshAbsoluteFilepath() const
        {
            return m_DestinationMeshAbsoluteFilepath;
        }

        bool hasDestinationLandmarksFilepath() const
        {
            return m_DestinationLandmarksAbsoluteFilepath.has_value();
        }

        std::optional<std::filesystem::path> const& tryGetDestinationLandmarksFilepath() const
        {
            return m_DestinationLandmarksAbsoluteFilepath;
        }

        size_t getNumLandmarks() const
        {
            return m_Landmarks.size();
        }

        size_t getNumFullyPairedLandmarks() const;

        bool hasLandmarkNamed(std::string_view) const;
        LandmarkPairing const* tryGetLandmarkPairingByName(std::string_view) const;

    private:
        std::filesystem::path m_SourceMeshAbsoluteFilepath;
        std::optional<std::filesystem::path> m_SourceLandmarksAbsoluteFilepath;

        std::optional<std::filesystem::path> m_DestinationMeshAbsoluteFilepath;
        std::optional<std::filesystem::path> m_DestinationLandmarksAbsoluteFilepath;

        std::vector<LandmarkPairing> m_Landmarks;
    };
}
