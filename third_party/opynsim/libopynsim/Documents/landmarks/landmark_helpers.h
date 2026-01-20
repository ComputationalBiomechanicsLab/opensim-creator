#pragma once

#include <libopynsim/Documents/landmarks/landmark.h>
#include <libopynsim/Documents/landmarks/landmark_csv_flags.h>
#include <libopynsim/Documents/landmarks/named_landmark.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace osc { class MaybeNamedLandmarkPair; }

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

    std::vector<Landmark> ReadLandmarksFromCSVIntoVectorOrThrow(
        const std::filesystem::path&
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

    void TryPairingLandmarks(
        std::vector<Landmark>,
        std::vector<Landmark>,
        const std::function<void(const MaybeNamedLandmarkPair&)>& consumer
    );
}
