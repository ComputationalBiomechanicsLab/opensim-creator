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
using osc::mow::MeshWarpPairing;

namespace
{
    std::filesystem::path CalcExpectedAssociatedLandmarksFile(
        std::filesystem::path const& meshAbsolutePath)
    {
        std::filesystem::path expected{meshAbsolutePath};
        expected.replace_extension(".landmarks.csv");
        expected = std::filesystem::weakly_canonical(expected);
        return expected;
    }

    std::filesystem::path CalcExpectedDestinationMeshFilepath(
        std::filesystem::path const& osimFilepath,
        std::filesystem::path const& sourceMeshFilepath)
    {
        std::filesystem::path expected = osimFilepath.parent_path() / "DestinationGeometry" / sourceMeshFilepath.filename();
        expected = std::filesystem::weakly_canonical(expected);
        return expected;
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

        ReadLandmarksFromCSV(in, [&rv](auto&& lm) { rv.push_back(std::move(lm)); });
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
    m_ExpectedSourceLandmarksAbsoluteFilepath{CalcExpectedAssociatedLandmarksFile(m_SourceMeshAbsoluteFilepath)},
    m_SourceLandmarksFileExists{std::filesystem::exists(m_ExpectedSourceLandmarksAbsoluteFilepath)},
    m_ExpectedDestinationMeshAbsoluteFilepath{CalcExpectedDestinationMeshFilepath(osimFilepath, m_SourceMeshAbsoluteFilepath)},
    m_DestinationMeshFileExists{std::filesystem::exists(m_ExpectedDestinationMeshAbsoluteFilepath)},
    m_ExpectedDestinationLandmarksAbsoluteFilepath{CalcExpectedAssociatedLandmarksFile(m_ExpectedDestinationMeshAbsoluteFilepath)},
    m_DestinationLandmarksFileExists{std::filesystem::exists(m_ExpectedDestinationLandmarksAbsoluteFilepath)},
    m_Landmarks{TryLoadPairedLandmarks(tryGetSourceLandmarksFilepath(), tryGetDestinationLandmarksFilepath())}
{
}

std::filesystem::path osc::mow::MeshWarpPairing::getSourceMeshAbsoluteFilepath() const
{
    return m_SourceMeshAbsoluteFilepath;
}

bool osc::mow::MeshWarpPairing::hasSourceLandmarksFilepath() const
{
    return m_SourceLandmarksFileExists;
}

std::filesystem::path osc::mow::MeshWarpPairing::recommendedSourceLandmarksFilepath() const
{
    return m_ExpectedSourceLandmarksAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::MeshWarpPairing::tryGetSourceLandmarksFilepath() const
{
    if (m_SourceLandmarksFileExists)
    {
        return m_ExpectedSourceLandmarksAbsoluteFilepath;
    }
    else
    {
        return std::nullopt;
    }
}

bool osc::mow::MeshWarpPairing::hasDestinationMeshFilepath() const
{
    return m_DestinationMeshFileExists;
}

std::filesystem::path osc::mow::MeshWarpPairing::recommendedDestinationMeshFilepath() const
{
     return m_ExpectedDestinationMeshAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::MeshWarpPairing::tryGetDestinationMeshAbsoluteFilepath() const
{
    if (m_DestinationMeshFileExists)
    {
        return m_ExpectedDestinationMeshAbsoluteFilepath;
    }
    else
    {
        return std::nullopt;
    }
}

bool osc::mow::MeshWarpPairing::hasDestinationLandmarksFilepath() const
{
    return m_DestinationLandmarksFileExists;
}

std::filesystem::path osc::mow::MeshWarpPairing::recommendedDestinationLandmarksFilepath() const
{
    return m_ExpectedDestinationLandmarksAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::MeshWarpPairing::tryGetDestinationLandmarksFilepath() const
{
    if (m_DestinationLandmarksFileExists)
    {
        return m_ExpectedDestinationLandmarksAbsoluteFilepath;
    }
    else
    {
        return std::nullopt;
    }
}

size_t osc::mow::MeshWarpPairing::getNumLandmarks() const
{
    return m_Landmarks.size();
}

size_t osc::mow::MeshWarpPairing::getNumSourceLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::hasSourcePos)
    );
}

size_t osc::mow::MeshWarpPairing::getNumDestinationLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::hasDestinationPos)
    );
}

size_t osc::mow::MeshWarpPairing::getNumFullyPairedLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::isFullyPaired)
    );
}

size_t osc::mow::MeshWarpPairing::getNumUnpairedLandmarks() const
{
    return getNumLandmarks() - getNumFullyPairedLandmarks();
}

bool osc::mow::MeshWarpPairing::hasSourceLandmarks() const
{
    return getNumSourceLandmarks() > 0;
}

bool osc::mow::MeshWarpPairing::hasDestinationLandmarks() const
{
    return getNumDestinationLandmarks() > 0;
}

bool osc::mow::MeshWarpPairing::hasUnpairedLandmarks() const
{
    return getNumFullyPairedLandmarks() < getNumLandmarks();
}

bool osc::mow::MeshWarpPairing::hasLandmarkNamed(std::string_view name) const
{
    return std::any_of(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        [name](auto const& lm) { return lm.getName() == name; }
    );
}

LandmarkPairing const* osc::mow::MeshWarpPairing::tryGetLandmarkPairingByName(std::string_view name) const
{
    auto const it = std::find_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        [name](auto const& lm) { return lm.getName() == name; }
    );
    return it != m_Landmarks.end() ? &(*it) : nullptr;
}

void osc::mow::MeshWarpPairing::forEachDetail(std::function<void(Detail)> const& callback) const
{
    callback({ "source mesh filepath", getSourceMeshAbsoluteFilepath().string() });
    callback({ "source landmarks expected filepath", recommendedSourceLandmarksFilepath().string() });
    callback({ "has source landmarks file?", hasSourceLandmarksFilepath() ? "yes" : "no" });
    callback({ "number of source landmarks", std::to_string(getNumSourceLandmarks()) });
    callback({ "destination mesh expected filepath", recommendedDestinationMeshFilepath().string() });
    callback({ "has destination mesh?", hasDestinationMeshFilepath() ? "yes" : "no" });
    callback({ "destination landmarks expected filepath", recommendedDestinationLandmarksFilepath().string() });
    callback({ "has destination landmarks file?", hasDestinationLandmarksFilepath() ? "yes" : "no" });
    callback({ "number of destination landmarks", std::to_string(getNumDestinationLandmarks()) });
    callback({ "number of paired landmarks", std::to_string(getNumFullyPairedLandmarks()) });
    callback({ "number of unpaired landmarks", std::to_string(getNumUnpairedLandmarks()) });
}

void osc::mow::MeshWarpPairing::forEachCheck(std::function<SearchState(Check)> const& callback) const
{
    // has a source landmarks file
    {
        std::stringstream ss;
        ss << "has source landmarks file at " << recommendedSourceLandmarksFilepath().string();
        if (callback({ std::move(ss).str(), hasSourceLandmarksFilepath() }) == SearchState::Stop)
        {
            return;
        }
    }

    // has source landmarks
    {
        if (callback({ "source landmarks file contains landmarks", hasSourceLandmarks() }) == SearchState::Stop)
        {
            return;
        }
    }

    // has destination mesh file
    {
        std::stringstream ss;
        ss << "has destination mesh file at " << recommendedDestinationMeshFilepath().string();
        if (callback({ std::move(ss).str(), hasDestinationMeshFilepath() }) == SearchState::Stop)
        {
            return;
        }
    }

    // has destination landmarks file
    {
        std::stringstream ss;
        ss << "has destination landmarks file at " << recommendedDestinationLandmarksFilepath().string();
        if (callback({ std::move(ss).str(), hasDestinationLandmarksFilepath() }) == SearchState::Stop)
        {
            return;
        }
    }

    // has destination landmarks
    {
        if (callback({ "destination landmarks file contains landmarks", hasDestinationLandmarks() }) == SearchState::Stop)
        {
            return;
        }
    }

    // has at least a few paired landmarks
    {
        if (callback({ "at least three landmarks can be paired between source/destination", getNumFullyPairedLandmarks() >= 3 }) == SearchState::Stop)
        {
            return;
        }
    }

    // (warning): has no unpaired landmarks
    {
        if (callback({ "there are no unpaired landmarks", getNumUnpairedLandmarks() == 0 ? State::Ok : State::Warning }) == SearchState::Stop)
        {
            return;
        }
    }
}

MeshWarpPairing::State osc::mow::MeshWarpPairing::state() const
{
    State worst = State::Ok;
    forEachCheck([&worst](Check c)
    {
        if (c.state == State::Error)
        {
            worst = State::Error;
            return SearchState::Stop;
        }
        else if (c.state == State::Warning)
        {
            worst = State::Warning;
            return SearchState::Continue;
        }
        else
        {
            return SearchState::Continue;
        }
    });
    return worst;
}
