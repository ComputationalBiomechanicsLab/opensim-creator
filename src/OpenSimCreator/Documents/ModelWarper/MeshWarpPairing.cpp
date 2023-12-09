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

using osc::lm::Landmark;
using osc::lm::ReadLandmarksFromCSV;
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

    std::vector<Landmark> TryReadLandmarksFromCSVIntoVector(std::filesystem::path const& path)
    {
        std::vector<Landmark> rv;

        std::ifstream in{path};
        if (!in)
        {
            osc::log::info("%s: cannot open landmark file", path.string().c_str());
            return rv;
        }

        ReadLandmarksFromCSV(in, [&rv](auto&& lm) { rv.push_back(std::forward(lm)); });
        return rv;
    }

    bool SameNameOrBothUnnamed(Landmark const& a, Landmark const& b)
    {
        return a.maybeName == b.maybeName;
    }

    std::string GenerateName(size_t suffix)
    {
        std::stringstream ss;
        ss << "unnamed_" << suffix;
        return std::move(ss).str();
    }

    std::vector<LandmarkPairing> PairLandmarks(std::vector<Landmark> a, std::vector<Landmark> b)
    {
        size_t nunnamed = 0;
        std::vector<LandmarkPairing> rv;

        // handle/pair all elements in `a`
        for (auto& lm : a)
        {
            auto const it = std::find_if(b.begin(), b.end(), std::bind_front(SameNameOrBothUnnamed, lm));
            std::string name = lm.maybeName ? *std::move(lm.maybeName) : GenerateName(nunnamed++);

            if (it != b.end())
            {
                rv.emplace_back(std::move(name), lm.position, it->position);
                b.erase(it);  // pop element from b
            }
            else
            {
                rv.emplace_back(std::move(name), lm.position, std::nullopt);
            }
        }

        // handle remaining (unpaired) elements in `b`
        for (auto& lm : b)
        {
            std::string name = lm.maybeName ? std::move(lm.maybeName).value() : GenerateName(nunnamed++);
            rv.emplace_back(name, std::nullopt, lm.position);
        }

        return rv;
    }

    std::vector<LandmarkPairing> TryLoadPairedLandmarks(
        [[maybe_unused]] std::optional<std::filesystem::path> const& maybeSourceLandmarksCSV,
        [[maybe_unused]] std::optional<std::filesystem::path> const& maybeDestinationLandmarksCSV)
    {
        std::vector<Landmark> src;
        if (maybeSourceLandmarksCSV)
        {
            src = TryReadLandmarksFromCSVIntoVector(*maybeSourceLandmarksCSV);
        }
        std::vector<Landmark> dest;
        if (maybeDestinationLandmarksCSV)
        {
            dest = TryReadLandmarksFromCSVIntoVector(*maybeDestinationLandmarksCSV);
        }
        return PairLandmarks(std::move(src), std::move(dest));
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
