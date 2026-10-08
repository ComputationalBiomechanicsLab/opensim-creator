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

            source_landmarks_path{std::move(sourceLandmarksPath_)},
            destination_landmarks_path{std::move(destinationLandmarksPath_)},
            source_landmarks_prescale{sourceLandmarksPrescale_},
            destination_landmarks_prescale{destinationLandmarksPrescale_},
            blending_factor{blendingFactor_},
            warping_penalty{warpingPenalty_}
        {}

        friend bool operator==(const ThinPlateSplineCommonInputs&, const ThinPlateSplineCommonInputs&) = default;

        std::filesystem::path source_landmarks_path;
        std::filesystem::path destination_landmarks_path;
        double source_landmarks_prescale;
        double destination_landmarks_prescale;
        bool apply_affine_translation = true;
        bool apply_affine_scale = true;
        bool apply_affine_rotation = true;
        bool apply_non_affine_warp = true;
        double blending_factor = 1.0;
        double warping_penalty = 0.0;
    };
}

template<>
struct std::hash<opyn::ThinPlateSplineCommonInputs> final {
    size_t operator()(const opyn::ThinPlateSplineCommonInputs& inputs) const noexcept
    {
        return osc::hash_of(
            inputs.source_landmarks_path,
            inputs.destination_landmarks_path,
            inputs.source_landmarks_prescale,
            inputs.destination_landmarks_prescale,
            inputs.apply_affine_rotation,
            inputs.apply_affine_scale,
            inputs.apply_affine_rotation,
            inputs.apply_non_affine_warp,
            inputs.blending_factor,
            inputs.warping_penalty
        );
    }
};
