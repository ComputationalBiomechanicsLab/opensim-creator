#include "TPSLandmarkPairWarperFactory.h"

#include <OpenSimCreator/Documents/Landmarks/LandmarkHelpers.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpDocument.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Utils/TPS3D.h>

#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/Log.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Utils/Algorithms.h>

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
namespace rgs = std::ranges;

namespace
{
    std::filesystem::path CalcExpectedAssociatedLandmarksFile(
        const std::filesystem::path& meshAbsolutePath)
    {
        std::filesystem::path expected{meshAbsolutePath};
        expected.replace_extension(".landmarks.csv");
        expected = std::filesystem::weakly_canonical(expected);
        return expected;
    }

    std::filesystem::path CalcExpectedDestinationMeshFilepath(
        const std::filesystem::path& osimFilepath,
        const std::filesystem::path& sourceMeshFilepath)
    {
        std::filesystem::path expected = osimFilepath.parent_path() / "DestinationGeometry" / sourceMeshFilepath.filename();
        expected = std::filesystem::weakly_canonical(expected);
        return expected;
    }

    std::vector<Landmark> TryReadLandmarksFromCSVIntoVector(const std::filesystem::path& path)
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

    bool SameNameOrBothUnnamed(const Landmark& a, const Landmark& b)
    {
        return a.maybeName == b.maybeName;
    }

    std::string GenerateName(size_t suffix)
    {
        std::stringstream ss;
        ss << "unnamed_" << suffix;
        return std::move(ss).str();
    }

    std::vector<MaybePairedLandmark> PairLandmarks(std::vector<Landmark> a, std::vector<Landmark> b)
    {
        size_t nunnamed = 0;
        std::vector<MaybePairedLandmark> rv;

        // handle/pair all elements in `a`
        for (auto& lm : a) {
            const auto it = rgs::find_if(b, std::bind_front(SameNameOrBothUnnamed, std::cref(lm)));
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

    std::vector<MaybePairedLandmark> TryLoadPairedLandmarks(
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

    TPSCoefficients3D TryCalcTPSCoefficients(std::span<const MaybePairedLandmark> maybePairs)
    {
        TPSCoefficientSolverInputs3D rv;
        for (const MaybePairedLandmark& maybePair: maybePairs) {
            if (auto pair = maybePair.tryGetPairedLocations()) {
                rv.landmarks.push_back(*pair);
            }
        }
        return CalcCoefficients(rv);
    }
}

osc::mow::TPSLandmarkPairWarperFactory::TPSLandmarkPairWarperFactory(
    const std::filesystem::path& osimFileLocation,
    const std::filesystem::path& sourceMeshFilepath) :

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

std::filesystem::path osc::mow::TPSLandmarkPairWarperFactory::getSourceMeshAbsoluteFilepath() const
{
    return m_SourceMeshAbsoluteFilepath;
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasSourceLandmarksFilepath() const
{
    return m_SourceLandmarksFileExists;
}

std::filesystem::path osc::mow::TPSLandmarkPairWarperFactory::recommendedSourceLandmarksFilepath() const
{
    return m_ExpectedSourceLandmarksAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::TPSLandmarkPairWarperFactory::tryGetSourceLandmarksFilepath() const
{
    if (m_SourceLandmarksFileExists) {
        return m_ExpectedSourceLandmarksAbsoluteFilepath;
    }
    else {
        return std::nullopt;
    }
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasDestinationMeshFilepath() const
{
    return m_DestinationMeshFileExists;
}

std::filesystem::path osc::mow::TPSLandmarkPairWarperFactory::recommendedDestinationMeshFilepath() const
{
     return m_ExpectedDestinationMeshAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::TPSLandmarkPairWarperFactory::tryGetDestinationMeshAbsoluteFilepath() const
{
    if (m_DestinationMeshFileExists) {
        return m_ExpectedDestinationMeshAbsoluteFilepath;
    }
    else {
        return std::nullopt;
    }
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasDestinationLandmarksFilepath() const
{
    return m_DestinationLandmarksFileExists;
}

std::filesystem::path osc::mow::TPSLandmarkPairWarperFactory::recommendedDestinationLandmarksFilepath() const
{
    return m_ExpectedDestinationLandmarksAbsoluteFilepath;
}

std::optional<std::filesystem::path> osc::mow::TPSLandmarkPairWarperFactory::tryGetDestinationLandmarksFilepath() const
{
    if (m_DestinationLandmarksFileExists) {
        return m_ExpectedDestinationLandmarksAbsoluteFilepath;
    }
    else {
        return std::nullopt;
    }
}

size_t osc::mow::TPSLandmarkPairWarperFactory::getNumLandmarks() const
{
    return m_Landmarks.size();
}

size_t osc::mow::TPSLandmarkPairWarperFactory::getNumSourceLandmarks() const
{
    return rgs::count_if(m_Landmarks, &MaybePairedLandmark::hasSource);
}

size_t osc::mow::TPSLandmarkPairWarperFactory::getNumDestinationLandmarks() const
{
    return rgs::count_if(m_Landmarks, &MaybePairedLandmark::hasDestination);
}

size_t osc::mow::TPSLandmarkPairWarperFactory::getNumFullyPairedLandmarks() const
{
    return rgs::count_if(m_Landmarks, &MaybePairedLandmark::isFullyPaired);
}

size_t osc::mow::TPSLandmarkPairWarperFactory::getNumUnpairedLandmarks() const
{
    return getNumLandmarks() - getNumFullyPairedLandmarks();
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasSourceLandmarks() const
{
    return getNumSourceLandmarks() > 0;
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasDestinationLandmarks() const
{
    return getNumDestinationLandmarks() > 0;
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasUnpairedLandmarks() const
{
    return getNumFullyPairedLandmarks() < getNumLandmarks();
}

bool osc::mow::TPSLandmarkPairWarperFactory::hasLandmarkNamed(std::string_view name) const
{
    return cpp23::contains(m_Landmarks, name, &MaybePairedLandmark::name);
}

const MaybePairedLandmark* osc::mow::TPSLandmarkPairWarperFactory::tryGetLandmarkPairingByName(std::string_view name) const
{
    return find_or_nullptr(m_Landmarks, name, &MaybePairedLandmark::name);
}

std::unique_ptr<IPointWarperFactory> osc::mow::TPSLandmarkPairWarperFactory::implClone() const
{
    return std::make_unique<TPSLandmarkPairWarperFactory>(*this);
}

std::vector<WarpDetail> osc::mow::TPSLandmarkPairWarperFactory::implWarpDetails() const
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

std::vector<ValidationCheckResult> osc::mow::TPSLandmarkPairWarperFactory::implValidate() const
{
    std::vector<ValidationCheckResult> rv;

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
    rv.emplace_back("there are no unpaired landmarks", getNumUnpairedLandmarks() == 0 ? ValidationCheckState::Ok : ValidationCheckState::Warning);

    return rv;
}

std::unique_ptr<IPointWarper> osc::mow::TPSLandmarkPairWarperFactory::implTryCreatePointWarper(const ModelWarpDocument& document) const
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
