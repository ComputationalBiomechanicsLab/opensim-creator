#pragma once

#include <OpenSimCreator/Documents/Landmarks/Landmark.hpp>
#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.hpp>

#include <functional>
#include <iosfwd>
#include <optional>

namespace osc::lm
{
    void ReadLandmarksFromCSV(
        std::istream&,
        std::function<void(Landmark&&)> const& landmarkConsumer
    );

    void WriteLandmarksToCSV(
        std::ostream&,
        std::function<std::optional<Landmark>()> const& landmarkProducer,
        LandmarkCSVFlags = LandmarkCSVFlags::None
    );
}
