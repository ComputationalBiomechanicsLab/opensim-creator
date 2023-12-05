#include "MeshWarpPairing.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <optional>

using osc::mow::LandmarkPairing;

namespace
{
    std::optional<std::filesystem::path> TryFindAssociatedLandmarksFile(
        std::filesystem::path const& meshAbsolutePath)
    {
        std::filesystem::path expected{meshAbsolutePath};
        expected.replace_extension(".landmarks.csv");
        expected = std::filesystem::weakly_canonical(expected);

        if (std::filesystem::exists(expected))
        {
            return expected;
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::filesystem::path> TryFindDestinationMesh(
        std::filesystem::path const& osimFilepath,
        std::filesystem::path const& sourceMeshFilepath)
    {
        std::filesystem::path expected = osimFilepath.parent_path() / "DestinationGeometry" / sourceMeshFilepath.filename();
        expected = std::filesystem::weakly_canonical(expected);

        if (std::filesystem::exists(expected))
        {
            return expected;
        }
        else
        {
            return std::nullopt;
        }
    }

    std::vector<LandmarkPairing> TryLoadPairedLandmarks(
        [[maybe_unused]] std::optional<std::filesystem::path> const& maybeSourceLandmarksCSV,
        [[maybe_unused]] std::optional<std::filesystem::path> const& maybeDestinationLandmarksCSV)
    {
        return {};
    }
}

osc::mow::MeshWarpPairing::MeshWarpPairing(
    std::filesystem::path const& osimFilepath,
    std::filesystem::path const& sourceMeshFilepath) :

    m_SourceMeshAbsoluteFilepath{std::filesystem::weakly_canonical(sourceMeshFilepath)},
    m_SourceLandmarksAbsoluteFilepath{TryFindAssociatedLandmarksFile(m_SourceMeshAbsoluteFilepath)},
    m_DestinationMeshAbsoluteFilepath{TryFindDestinationMesh(osimFilepath, m_SourceMeshAbsoluteFilepath)},
    m_DestinationLandmarksAbsoluteFilepath{m_DestinationMeshAbsoluteFilepath ? TryFindAssociatedLandmarksFile(*m_DestinationMeshAbsoluteFilepath) : std::nullopt},
    m_Landmarks{TryLoadPairedLandmarks(m_SourceLandmarksAbsoluteFilepath, m_DestinationLandmarksAbsoluteFilepath)}
{
}

size_t osc::mow::MeshWarpPairing::getNumFullyPairedLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::isFullyPaired)
    );
}

bool osc::mow::MeshWarpPairing::hasLandmarkNamed(std::string_view name) const
{
    return std::any_of(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        [name](auto const& lm) { return lm.getName() == name; }
    );
}
