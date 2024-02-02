#pragma once

#include <OpenSimCreator/Documents/Landmarks/Landmark.hpp>
#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.hpp>
#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.hpp>

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace osc::lm
{
    struct CSVParseWarning final {
        size_t lineNumber;
        std::string message;
    };

    std::string to_string(CSVParseWarning const&);

    void ReadLandmarksFromCSV(
        std::istream&,
        std::function<void(Landmark&&)> const& landmarkConsumer,
        std::function<void(CSVParseWarning)> const& warningConsumer = [](auto){}
    );

    void WriteLandmarksToCSV(
        std::ostream&,
        std::function<std::optional<Landmark>()> const& landmarkProducer,
        LandmarkCSVFlags = LandmarkCSVFlags::None
    );

    // generates names for any unnamed landmarks and ensures that the names are
    // unique amongst all supplied landmarks (both named and unnamed)
    std::vector<NamedLandmark> GenerateNames(
        std::span<Landmark const>,
        std::string_view prefix = "unnamed_"
    );
}
