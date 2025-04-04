#pragma once

#include <libopensimcreator/Documents/Landmarks/MaybeNamedLandmarkPair.h>
#include <libopensimcreator/Documents/ModelWarper/IPointWarperFactory.h>
#include <libopensimcreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <libopensimcreator/Documents/ModelWarper/WarpDetail.h>
#include <libopensimcreator/Utils/TPS3D.h>

#include <liboscar/Maths/Vec3.h>
#include <liboscar/Utils/CopyOnUpdPtr.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace osc::mow
{
    // an `IPointWarperFactory` that creates its `IPointWarper` from a thin-plate
    // spline (TPS) warp that's calculated by pairing landmark correspondences that
    // are loaded from CSV files next to mesh files
    //
    // e.g. if possible, this will look for `meshfile.landmarks.csv` and `Destination/meshfile.landmarks.csv`
    //      and then compile a suitable TPS warp from the landmark pairings
    class TPSLandmarkPairWarperFactory final : public IPointWarperFactory {
    public:

        // constructs the factory by looking around on-disk for appropriate `.landmarks.csv` files
        //
        // use the post-construction validation checks to figure out how sucessful it was
        TPSLandmarkPairWarperFactory(
            const std::filesystem::path& osimFileLocation,
            const std::filesystem::path& sourceMeshFilepath
        );

        std::filesystem::path getSourceMeshAbsoluteFilepath() const;

        bool hasSourceLandmarksFilepath() const;
        std::filesystem::path recommendedSourceLandmarksFilepath() const;
        std::optional<std::filesystem::path> tryGetSourceLandmarksFilepath() const;

        bool hasDestinationMeshFilepath() const;
        std::filesystem::path recommendedDestinationMeshFilepath() const;
        std::optional<std::filesystem::path> tryGetDestinationMeshAbsoluteFilepath() const;

        bool hasDestinationLandmarksFilepath() const;
        std::filesystem::path recommendedDestinationLandmarksFilepath() const;
        std::optional<std::filesystem::path> tryGetDestinationLandmarksFilepath() const;

        size_t getNumLandmarks() const;
        size_t getNumSourceLandmarks() const;
        size_t getNumDestinationLandmarks() const;
        size_t getNumFullyPairedLandmarks() const;
        size_t getNumUnpairedLandmarks() const;
        bool hasSourceLandmarks() const;
        bool hasDestinationLandmarks() const;
        bool hasUnpairedLandmarks() const;

        bool hasLandmarkNamed(std::string_view) const;
        const MaybeNamedLandmarkPair* tryGetLandmarkPairingByName(std::string_view) const;

    private:
        std::unique_ptr<IPointWarperFactory> implClone() const override;
        std::vector<WarpDetail> implWarpDetails() const override;
        std::vector<ValidationCheckResult> implValidate(const WarpableModel&) const override;
        std::unique_ptr<IPointWarper> implTryCreatePointWarper(const WarpableModel&) const override;

        std::filesystem::path m_SourceMeshAbsoluteFilepath;

        std::filesystem::path m_ExpectedSourceLandmarksAbsoluteFilepath;
        bool m_SourceLandmarksFileExists;

        std::filesystem::path m_ExpectedDestinationMeshAbsoluteFilepath;
        bool m_DestinationMeshFileExists;

        std::filesystem::path m_ExpectedDestinationLandmarksAbsoluteFilepath;
        bool m_DestinationLandmarksFileExists;

        std::vector<MaybeNamedLandmarkPair> m_Landmarks;

        CopyOnUpdPtr<TPSCoefficients3D<float>> m_TPSCoefficients;
    };
}
