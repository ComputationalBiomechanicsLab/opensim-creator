#include "MeshWarpPairing.hpp"

#include <OpenSimCreator/Documents/Landmarks/LandmarkHelpers.hpp>

#include <oscar/Platform/Log.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

    void TryLoadSourceLandmarks(
        std::filesystem::path const& path,
        std::vector<LandmarkPairing>& out)
    {
        std::ifstream in{path};
        if (!in)
        {
            osc::log::info("%s: cannot open source landmark file", path.string().c_str());
            return;
        }

        osc::lm::ReadLandmarksFromCSV(in, [&out](auto&& lm)
        {
            out.emplace_back(std::move(lm.maybeName).value_or(std::string{}), lm.position, std::nullopt);
        });
    }

    void TryLoadAndPairDestinationLandmarks(
        std::filesystem::path const& path,
        std::vector<LandmarkPairing>& out)
    {
        std::ifstream in{path};
        if (!in)
        {
            osc::log::info("%s: cannot open destination landmark file", path.string().c_str());
            return;
        }

        osc::lm::ReadLandmarksFromCSV(in, [&out, nsource = out.size()](auto&& lm)
        {
            auto const begin = out.begin();
            auto const end = out.begin() + nsource;
            auto const it = std::find_if(begin, end, [&lm](auto const& p)
            {
                return p.getName() == lm.maybeName.value_or(std::string{});
            });

            if (it != end)
            {
                it->setDestinationPos(lm.position);
            }
            else
            {
                out.emplace_back(std::move(lm.maybeName).value_or(std::string{}), std::nullopt, lm.position);
            }
        });
    }

    std::vector<LandmarkPairing> TryLoadPairedLandmarks(
        [[maybe_unused]] std::optional<std::filesystem::path> const& maybeSourceLandmarksCSV,
        [[maybe_unused]] std::optional<std::filesystem::path> const& maybeDestinationLandmarksCSV)
    {
        std::vector<LandmarkPairing> rv;
        if (maybeSourceLandmarksCSV)
        {
            TryLoadSourceLandmarks(*maybeSourceLandmarksCSV, rv);
        }
        if (maybeDestinationLandmarksCSV)
        {
            TryLoadAndPairDestinationLandmarks(*maybeDestinationLandmarksCSV, rv);
        }
        return rv;
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

osc::mow::LandmarkPairing const* osc::mow::MeshWarpPairing::tryGetLandmarkPairingByName(std::string_view name) const
{
    auto const it = std::find_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        [name](auto const& lm) { return lm.getName() == name; }
    );
    return it != m_Landmarks.end() ? &(*it) : nullptr;
}
