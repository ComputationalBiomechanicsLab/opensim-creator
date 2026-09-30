#pragma once

#include <libopynsim/documents/landmarks/landmark.h>
#include <libopynsim/documents/landmarks/landmark_csv_flags.h>
#include <libopynsim/documents/landmarks/named_landmark.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opyn { class MaybeNamedLandmarkPair; }

namespace opyn
{
    struct CSVParseWarning final {
        size_t line_number;
        std::string message;
    };

    std::string to_string(const CSVParseWarning&);

    void read_landmarks_from_csv(
        std::istream&,
        const std::function<void(Landmark&&)>& landmark_consumer,
        const std::function<void(CSVParseWarning)>& warning_consumer = [](auto){}
    );

    std::vector<Landmark> read_landmarks_from_csv_into_vector_or_throw(
        const std::filesystem::path&
    );

    void write_landmarks_to_csv(
        std::ostream&,
        const std::function<std::optional<Landmark>()>& landmark_producer,
        LandmarkCSVFlags = LandmarkCSVFlags::None
    );

    // generates names for any unnamed landmarks and ensures that the names are
    // unique amongst all supplied landmarks (both named and unnamed)
    std::vector<NamedLandmark> generate_names(
        std::span<const Landmark>,
        std::string_view prefix = "unnamed_"
    );

    void try_pairing_landmarks(
        std::vector<Landmark>,
        std::vector<Landmark>,
        const std::function<void(const MaybeNamedLandmarkPair&)>& consumer
    );
}
