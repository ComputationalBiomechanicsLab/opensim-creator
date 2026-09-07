#pragma once

#include <liboscar/utilities/hash_helpers.h>

#include <filesystem>
#include <functional>
#include <utility>

namespace opyn
{
    // Struct of runtime inputs that are common for all uses of the TPS
    // scaling algorithm.
    struct ThinPlateSplineCommonInputs final {

        explicit ThinPlateSplineCommonInputs(
            std::filesystem::path sourceLandmarksPath_,
            std::filesystem::path destinationLandmarksPath_,
            double sourceLandmarksPrescale_,
            double destinationLandmarksPrescale_,
            double blendingFactor_,
            double warpingPenalty_) :

            sourceLandmarksPath{std::move(sourceLandmarksPath_)},
            destinationLandmarksPath{std::move(destinationLandmarksPath_)},
            sourceLandmarksPrescale{sourceLandmarksPrescale_},
            destinationLandmarksPrescale{destinationLandmarksPrescale_},
            blendingFactor{blendingFactor_},
            warpingPenalty{warpingPenalty_}
        {}

        friend bool operator==(const ThinPlateSplineCommonInputs&, const ThinPlateSplineCommonInputs&) = default;

        std::filesystem::path sourceLandmarksPath;
        std::filesystem::path destinationLandmarksPath;
        double sourceLandmarksPrescale;
        double destinationLandmarksPrescale;
        bool applyAffineTranslation = true;
        bool applyAffineScale = true;
        bool applyAffineRotation = true;
        bool applyNonAffineWarp = true;
        double blendingFactor = 1.0;
        double warpingPenalty = 0.0;
    };
}

template<>
struct std::hash<opyn::ThinPlateSplineCommonInputs> final {
    size_t operator()(const opyn::ThinPlateSplineCommonInputs& inputs) const noexcept
    {
        return osc::hash_of(
            inputs.sourceLandmarksPath,
            inputs.destinationLandmarksPath,
            inputs.sourceLandmarksPrescale,
            inputs.destinationLandmarksPrescale,
            inputs.applyAffineRotation,
            inputs.applyAffineScale,
            inputs.applyAffineRotation,
            inputs.applyNonAffineWarp,
            inputs.blendingFactor,
            inputs.warpingPenalty
        );
    }
};
