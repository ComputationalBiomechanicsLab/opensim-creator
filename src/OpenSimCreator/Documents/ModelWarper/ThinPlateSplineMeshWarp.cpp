#include "ThinPlateSplineMeshWarp.h"

#include <OpenSimCreator/Documents/Landmarks/LandmarkHelpers.h>
#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Utils/TPS3D.h>

#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/Log.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::lm;
using namespace osc::mow;

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
        if (!in) {
            log_info("%s: cannot open landmark file", path.string().c_str());
            return rv;
        }

        ReadLandmarksFromCSV(in, [&rv](auto&& lm) { rv.push_back(std::forward<decltype(lm)>(lm)); });
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
        for (auto& lm : a) {
            auto const it = std::find_if(b.begin(), b.end(), std::bind_front(SameNameOrBothUnnamed, lm));
            std::string name = lm.maybeName ? *std::move(lm.maybeName) : GenerateName(nunnamed++);

            if (it != b.end()) {
                rv.emplace_back(std::move(name), lm.position, it->position);
                b.erase(it);  // pop element from b
            }
            else {
                rv.emplace_back(std::move(name), lm.position, std::nullopt);
            }
        }

        // handle remaining (unpaired) elements in `b`
        for (auto& lm : b) {
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
        if (maybeSourceLandmarksCSV) {
            src = TryReadLandmarksFromCSVIntoVector(*maybeSourceLandmarksCSV);
        }
        std::vector<Landmark> dest;
        if (maybeDestinationLandmarksCSV) {
            dest = TryReadLandmarksFromCSVIntoVector(*maybeDestinationLandmarksCSV);
        }
        return PairLandmarks(std::move(src), std::move(dest));
    }

    TPSCoefficients3D TryCalcTPSCoefficients(std::span<LandmarkPairing const> maybePairs)
    {
        TPSCoefficientSolverInputs3D rv;
        for (LandmarkPairing const& maybePair: maybePairs) {
            if (auto pair = maybePair.tryGetPairedLocations()) {
                rv.landmarks.push_back(*pair);
            }
        }
        return CalcCoefficients(rv);
    }
}

osc::mow::ThinPlateSplineMeshWarp::ThinPlateSplineMeshWarp(
    std::filesystem::path const& osimFileLocation,
    std::filesystem::path const& sourceMeshFilepath) :

    m_SourceMeshAbsoluteFilepath{std::filesystem::weakly_canonical(sourceMeshFilepath)},
    m_ExpectedSourceLandmarksAbsoluteFilepath{CalcExpectedAssociatedLandmarksFile(m_SourceMeshAbsoluteFilepath)},
    m_SourceLandmarksFileExists{std::filesystem::exists(m_ExpectedSourceLandmarksAbsoluteFilepath)},
    m_ExpectedDestinationMeshAbsoluteFilepath{CalcExpectedDestinationMeshFilepath(osimFileLocation, m_SourceMeshAbsoluteFilepath)},
    m_DestinationMeshFileExists{std::filesystem::exists(m_ExpectedDestinationMeshAbsoluteFilepath)},
    m_ExpectedDestinationLandmarksAbsoluteFilepath{CalcExpectedAssociatedLandmarksFile(m_ExpectedDestinationMeshAbsoluteFilepath)},
    m_DestinationLandmarksFileExists{std::filesystem::exists(m_ExpectedDestinationLandmarksAbsoluteFilepath)},
    m_Landmarks{TryLoadPairedLandmarks(tryGetSourceLandmarksFilepath(), tryGetDestinationLandmarksFilepath())},
    m_TPSCoefficients{make_cow<TPSCoefficients3D>(TryCalcTPSCoefficients(m_Landmarks))}
{}

std::filesystem::path osc::mow::ThinPlateSplineMeshWarp::getSourceMeshAbsoluteFilepath() const
{
    return m_SourceMeshAbsoluteFilepath;
}

bool osc::mow::ThinPlateSplineMeshWarp::hasSourceLandmarksFilepath() const
{
    return m_SourceLandmarksFileExists;
}

std::filesystem::path osc::mow::ThinPlateSplineMeshWarp::recommendedSourceLandmarksFilepath() const
{
    return m_ExpectedSourceLandmarksAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::ThinPlateSplineMeshWarp::tryGetSourceLandmarksFilepath() const
{
    if (m_SourceLandmarksFileExists) {
        return m_ExpectedSourceLandmarksAbsoluteFilepath;
    }
    else {
        return std::nullopt;
    }
}

bool osc::mow::ThinPlateSplineMeshWarp::hasDestinationMeshFilepath() const
{
    return m_DestinationMeshFileExists;
}

std::filesystem::path osc::mow::ThinPlateSplineMeshWarp::recommendedDestinationMeshFilepath() const
{
     return m_ExpectedDestinationMeshAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::ThinPlateSplineMeshWarp::tryGetDestinationMeshAbsoluteFilepath() const
{
    if (m_DestinationMeshFileExists) {
        return m_ExpectedDestinationMeshAbsoluteFilepath;
    }
    else {
        return std::nullopt;
    }
}

bool osc::mow::ThinPlateSplineMeshWarp::hasDestinationLandmarksFilepath() const
{
    return m_DestinationLandmarksFileExists;
}

std::filesystem::path osc::mow::ThinPlateSplineMeshWarp::recommendedDestinationLandmarksFilepath() const
{
    return m_ExpectedDestinationLandmarksAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::ThinPlateSplineMeshWarp::tryGetDestinationLandmarksFilepath() const
{
    if (m_DestinationLandmarksFileExists) {
        return m_ExpectedDestinationLandmarksAbsoluteFilepath;
    }
    else {
        return std::nullopt;
    }
}

size_t osc::mow::ThinPlateSplineMeshWarp::getNumLandmarks() const
{
    return m_Landmarks.size();
}

size_t osc::mow::ThinPlateSplineMeshWarp::getNumSourceLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::hasSource)
    );
}

size_t osc::mow::ThinPlateSplineMeshWarp::getNumDestinationLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::hasDestination)
    );
}

size_t osc::mow::ThinPlateSplineMeshWarp::getNumFullyPairedLandmarks() const
{
    return std::count_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        std::mem_fn(&LandmarkPairing::isFullyPaired)
    );
}

size_t osc::mow::ThinPlateSplineMeshWarp::getNumUnpairedLandmarks() const
{
    return getNumLandmarks() - getNumFullyPairedLandmarks();
}

bool osc::mow::ThinPlateSplineMeshWarp::hasSourceLandmarks() const
{
    return getNumSourceLandmarks() > 0;
}

bool osc::mow::ThinPlateSplineMeshWarp::hasDestinationLandmarks() const
{
    return getNumDestinationLandmarks() > 0;
}

bool osc::mow::ThinPlateSplineMeshWarp::hasUnpairedLandmarks() const
{
    return getNumFullyPairedLandmarks() < getNumLandmarks();
}

bool osc::mow::ThinPlateSplineMeshWarp::hasLandmarkNamed(std::string_view name) const
{
    return std::any_of(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        [name](auto const& lm) { return lm.name() == name; }
    );
}

LandmarkPairing const* osc::mow::ThinPlateSplineMeshWarp::tryGetLandmarkPairingByName(std::string_view name) const
{
    auto const it = std::find_if(
        m_Landmarks.begin(),
        m_Landmarks.end(),
        [name](auto const& lm) { return lm.name() == name; }
    );
    return it != m_Landmarks.end() ? &(*it) : nullptr;
}

std::unique_ptr<IMeshWarp> osc::mow::ThinPlateSplineMeshWarp::implClone() const
{
    return std::make_unique<ThinPlateSplineMeshWarp>(*this);
}

std::vector<WarpDetail> osc::mow::ThinPlateSplineMeshWarp::implWarpDetails() const
{
    std::vector<WarpDetail> rv;
    rv.emplace_back("source mesh filepath", getSourceMeshAbsoluteFilepath().string());
    rv.emplace_back("source landmarks expected filepath", recommendedSourceLandmarksFilepath().string());
    rv.emplace_back("has source landmarks file?", hasSourceLandmarksFilepath() ? "yes" : "no");
    rv.emplace_back("number of source landmarks", std::to_string(getNumSourceLandmarks()));
    rv.emplace_back("destination mesh expected filepath", recommendedDestinationMeshFilepath().string());
    rv.emplace_back("has destination mesh?", hasDestinationMeshFilepath() ? "yes" : "no");
    rv.emplace_back("destination landmarks expected filepath", recommendedDestinationLandmarksFilepath().string());
    rv.emplace_back("has destination landmarks file?", hasDestinationLandmarksFilepath() ? "yes" : "no");
    rv.emplace_back("number of destination landmarks", std::to_string(getNumDestinationLandmarks()));
    rv.emplace_back("number of paired landmarks", std::to_string(getNumFullyPairedLandmarks()));
    rv.emplace_back("number of unpaired landmarks", std::to_string(getNumUnpairedLandmarks()));
    return rv;
}

std::vector<ValidationCheck> osc::mow::ThinPlateSplineMeshWarp::implValidate() const
{
    std::vector<ValidationCheck> rv;

    // has a source landmarks file
    {
        std::stringstream ss;
        ss << "has source landmarks file at " << recommendedSourceLandmarksFilepath().string();
        rv.emplace_back(std::move(ss).str(), hasSourceLandmarksFilepath());
    }

    // has source landmarks
    rv.emplace_back("source landmarks file contains landmarks", hasSourceLandmarks());

    // has destination mesh file
    {
        std::stringstream ss;
        ss << "has destination mesh file at " << recommendedDestinationMeshFilepath().string();
        rv.emplace_back(std::move(ss).str(), hasDestinationMeshFilepath());
    }

    // has destination landmarks file
    {
        std::stringstream ss;
        ss << "has destination landmarks file at " << recommendedDestinationLandmarksFilepath().string();
        rv.emplace_back(std::move(ss).str(), hasDestinationLandmarksFilepath());
    }

    // has destination landmarks
    rv.emplace_back("destination landmarks file contains landmarks", hasDestinationLandmarks());

    // has at least a few paired landmarks
    rv.emplace_back("at least three landmarks can be paired between source/destination", getNumFullyPairedLandmarks() >= 3);

    // (warning): has no unpaired landmarks
    rv.emplace_back("there are no unpaired landmarks", getNumUnpairedLandmarks() == 0 ? ValidationState::Ok : ValidationState::Warning);

    return rv;
}

std::unique_ptr<IPointWarper> osc::mow::ThinPlateSplineMeshWarp::implCompileWarper(Document const& document) const
{
    class TPSWarper : public IPointWarper {
    public:
        TPSWarper(CopyOnUpdPtr<TPSCoefficients3D> coefficients_, float blendingFactor_) :
            m_Coefficients{std::move(coefficients_)},
            m_BlendingFactor{blendingFactor_}
        {}
    private:
        void implWarpInPlace(std::span<Vec3> points) const override
        {
            ApplyThinPlateWarpToPointsInPlace(*m_Coefficients, points, m_BlendingFactor);
        }

        CopyOnUpdPtr<TPSCoefficients3D> m_Coefficients;
        float m_BlendingFactor;
    };

    return std::make_unique<TPSWarper>(m_TPSCoefficients, document.getWarpBlendingFactor());
}
