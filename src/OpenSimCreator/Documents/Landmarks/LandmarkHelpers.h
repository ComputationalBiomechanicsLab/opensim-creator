#pragma once

#include <OpenSimCreator/Documents/Landmarks/Landmark.h>
#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.h>

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

    std::string to_string(const CSVParseWarning&);

    void ReadLandmarksFromCSV(
        std::istream&,
        const std::function<void(Landmark&&)>& landmarkConsumer,
        const std::function<void(CSVParseWarning)>& warningConsumer = [](auto){}
    );

    void WriteLandmarksToCSV(
        std::ostream&,
        const std::function<std::optional<Landmark>()>& landmarkProducer,
        LandmarkCSVFlags = LandmarkCSVFlags::None
    );

    // generates names for any unnamed landmarks and ensures that the names are
    // unique amongst all supplied landmarks (both named and unnamed)
    std::vector<NamedLandmark> GenerateNames(
        std::span<const Landmark>,
        std::string_view prefix = "unnamed_"
    );
}
