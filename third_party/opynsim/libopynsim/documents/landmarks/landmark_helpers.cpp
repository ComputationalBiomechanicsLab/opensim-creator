#include "landmark_helpers.h"

#include <libopynsim/documents/landmarks/maybe_named_landmark_pair.h>

#include <liboscar/formats/csv.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/exception_helpers.h>
#include <liboscar/utilities/std_variant_helpers.h>
#include <liboscar/utilities/string_helpers.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    struct SkipRow final {};

    using ParseResult = std::variant<Landmark, CSVParseWarning, SkipRow>;

    ParseResult parse_row(size_t line_number, std::span<const std::string> cols)
    {
        if (cols.empty() or (cols.size() == 1 and osc::strip_whitespace(cols.front()).empty())) {
            return SkipRow{};  // whitespace row, or trailing newline
        }
        if (cols.size() < 3) {
            return CSVParseWarning{line_number, "too few columns in this row"};
        }

        // >=4 columns implies that the first column is a label column
        std::optional<std::string> maybe_name;
        std::span<const std::string> data = cols;
        if (cols.size() >= 4) {
            maybe_name = cols.front();
            data = data.subspan(1);
        }

        const std::optional<float> x = osc::from_chars_strip_whitespace(data.front());
        if (not x) {
            if (line_number == 0) {
                return SkipRow{};  // it's probably a header label
            } else {
                return CSVParseWarning{line_number, "cannot parse X as a number"};
            }
        }
        const std::optional<float> y = osc::from_chars_strip_whitespace(data[1]);
        if (not y) {
            if (line_number == 0) {
                return SkipRow{};  // it's probably a header label
            } else {
                return CSVParseWarning{line_number, "cannot parse Y as a number"};
            }
        }
        const std::optional<float> z = osc::from_chars_strip_whitespace(data[2]);
        if (not z) {
            if (line_number == 0) {
                return SkipRow{};
            } else {
                return CSVParseWarning{line_number, "cannot parse Z as a number"};
            }
        }

        return Landmark{std::move(maybe_name), osc::Vector3{*x, *y, *z}};
    }

    bool same_name_or_both_unnamed(const Landmark& a, const Landmark& b)
    {
        return a.maybe_name == b.maybe_name;
    }

    std::string generate_name(size_t suffix)
    {
        return std::format("unnamed_{}", suffix);
    }
}

std::string opyn::to_string(const CSVParseWarning& warning)
{
    const size_t displayed_line_number = warning.line_number + 1;  // user-facing software (e.g. IDEs) start at 1
    return std::format("line {}: {}", displayed_line_number, warning.message);
}

void opyn::read_landmarks_from_csv(
    std::istream& in,
    const std::function<void(Landmark&&)>& landmark_consumer,
    const std::function<void(CSVParseWarning)>& warning_consumer)
{
    std::vector<std::string> cols;
    for (size_t line = 0; osc::CSV::read_row_into_vector(in, cols); ++line)
    {
        std::visit(osc::Overload{
            [&landmark_consumer](Landmark&& lm) { landmark_consumer(std::move(lm)); },
            [&warning_consumer](CSVParseWarning&& warning) { warning_consumer(std::move(warning)); },
            [](SkipRow) {}
        }, parse_row(line, cols));
    }
}

std::vector<Landmark> opyn::read_landmarks_from_csv_into_vector_or_throw(
    const std::filesystem::path& path)
{
    std::ifstream in{path};
    if (not in) {
        throw osc::formatted_runtime_error("{}: cannot open landmarks file for reading", path.string());
    }

    std::vector<Landmark> rv;
    read_landmarks_from_csv(in, [&rv](auto&& lm) { rv.push_back(std::forward<decltype(lm)>(lm)); });
    return rv;
}

void opyn::write_landmarks_to_csv(
    std::ostream& out,
    const std::function<std::optional<Landmark>()>& landmark_producer,
    LandmarkCSVFlags flags)
{
    // if applicable, emit header
    if (not (flags & LandmarkCSVFlags::NoHeader)) {
        if (flags & LandmarkCSVFlags::NoNames) {
            osc::CSV::write_row(out, {{"x", "y", "z"}});
        } else {
            osc::CSV::write_row(out, {{"name", "x", "y", "z"}});
        }
    }

    // emit data emitted by the landmark producer (until std::nullopt) as data rows
    for (auto lm = landmark_producer(); lm; lm = landmark_producer()) {
        using std::to_string;
        auto x = lm->position.x();
        auto y = lm->position.y();
        auto z = lm->position.z();

        if (flags & LandmarkCSVFlags::NoNames) {
            osc::CSV::write_row(out, {{to_string(x), to_string(y), to_string(z)}});
        } else {
            osc::CSV::write_row(out, {{lm->maybe_name.value_or("unnamed"), to_string(x), to_string(y), to_string(z)}});
        }
    }
}

std::vector<NamedLandmark> opyn::generate_names(
    std::span<const Landmark> lms,
    std::string_view prefix)
{
    // collect up all already-named landmarks
    std::unordered_set<std::string_view> supplied_names;
    for (const auto& lm : lms) {
        if (lm.maybe_name) {
            supplied_names.insert(*lm.maybe_name);
        }
    }

    // helper: either get, or generate, a name for the given landmark
    auto get_name = [&prefix, &supplied_names, i = 0](const Landmark& lm) mutable -> std::string
    {
        if (lm.maybe_name) {
            return *lm.maybe_name;
        }

        auto next_name = [&prefix, &i] { return std::string{prefix} + std::to_string(i++); };
        std::string name = next_name();
        while (supplied_names.contains(name)) {
            name = next_name();
        }
        return name;
    };

    std::vector<NamedLandmark> rv;
    rv.reserve(lms.size());
    for (const auto& lm : lms) {
        rv.push_back(NamedLandmark{get_name(lm), lm.position});
    }
    return rv;
}

void opyn::try_pairing_landmarks(
    std::vector<Landmark> a,
    std::vector<Landmark> b,
    const std::function<void(const MaybeNamedLandmarkPair&)>& consumer)
{
    size_t num_unnamed = 0;

    // handle/pair all elements in `a`
    for (auto& lm : a) {
        const auto it = rgs::find_if(b, std::bind_front(same_name_or_both_unnamed, std::cref(lm)));
        std::string name = lm.maybe_name ? *std::move(lm.maybe_name) : generate_name(num_unnamed++);

        if (it != b.end()) {
            consumer(MaybeNamedLandmarkPair{std::move(name), lm.position, it->position});
            b.erase(it);  // pop element from b
        }
        else {
            consumer(MaybeNamedLandmarkPair{std::move(name), lm.position, std::nullopt});
        }
    }

    // handle remaining (unpaired) elements in `b`
    for (auto& lm : b) {
        std::string name = lm.maybe_name ? std::move(lm.maybe_name).value() : generate_name(num_unnamed++);
        consumer(MaybeNamedLandmarkPair{name, std::nullopt, lm.position});
    }
}
