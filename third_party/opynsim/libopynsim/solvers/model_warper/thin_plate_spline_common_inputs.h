#pragma once

#include <filesystem>
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
