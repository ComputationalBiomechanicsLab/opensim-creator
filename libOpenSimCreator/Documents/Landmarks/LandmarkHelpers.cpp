#include "LandmarkHelpers.h"

#include <libOpenSimCreator/Documents/Landmarks/MaybeNamedLandmarkPair.h>

#include <liboscar/Formats/CSV.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/StringHelpers.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using osc::lm::CSVParseWarning;
using osc::lm::Landmark;
using osc::lm::NamedLandmark;
using namespace osc;
namespace rgs = std::ranges;

namespace
{
    struct SkipRow final {};

    using ParseResult = std::variant<Landmark, CSVParseWarning, SkipRow>;

    ParseResult ParseRow(size_t lineNum, std::span<const std::string> cols)
    {
        if (cols.empty() || (cols.size() == 1 && strip_whitespace(cols.front()).empty()))
        {
            return SkipRow{};  // whitespace row, or trailing newline
        }
        if (cols.size() < 3)
        {
            return CSVParseWarning{lineNum, "too few columns in this row"};
        }

        // >=4 columns implies that the first column is a label column
        std::optional<std::string> maybeName;
        std::span<const std::string> data = cols;
        if (cols.size() >= 4)
        {
            maybeName = cols.front();
            data = data.subspan(1);
        }

        const std::optional<float> x = from_chars_strip_whitespace(data.front());
        if (!x)
        {
            if (lineNum == 0)
            {
                return SkipRow{};  // it's probably a header label
            }
            else
            {
                return CSVParseWarning{lineNum, "cannot parse X as a number"};
            }
        }
        const std::optional<float> y = from_chars_strip_whitespace(data[1]);
        if (!y)
        {
            if (lineNum == 0)
            {
                return SkipRow{};  // it's probably a header label
            }
            else
            {
                return CSVParseWarning{lineNum, "cannot parse Y as a number"};
            }
        }
        const std::optional<float> z = from_chars_strip_whitespace(data[2]);
        if (!z)
        {
            if (lineNum == 0)
            {
                return SkipRow{};
            }
            else
            {
                return CSVParseWarning{lineNum, "cannot parse Z as a number"};
            }
        }

        return Landmark{std::move(maybeName), Vec3{*x, *y, *z}};
    }

    bool SameNameOrBothUnnamed(const Landmark& a, const Landmark& b)
    {
        return a.maybeName == b.maybeName;
    }

    std::string GenerateName(size_t suffix)
    {
        std::stringstream ss;
        ss << "unnamed_" << suffix;
        return std::move(ss).str();
    }
}

std::string osc::lm::to_string(const CSVParseWarning& warning)
{
    std::stringstream ss;
    const size_t displayedLineNumber = warning.lineNumber+1;  // user-facing software (e.g. IDEs) start at 1
    ss << "line " << displayedLineNumber << ": " << warning.message;
    return std::move(ss).str();
}

void osc::lm::ReadLandmarksFromCSV(
    std::istream& in,
    const std::function<void(Landmark&&)>& landmarkConsumer,
    const std::function<void(CSVParseWarning)>& warningConsumer)
{
    std::vector<std::string> cols;
    for (size_t line = 0; read_csv_row_into_vector(in, cols); ++line)
    {
        std::visit(Overload
        {
            [&landmarkConsumer](Landmark&& lm) { landmarkConsumer(std::move(lm)); },
            [&warningConsumer](CSVParseWarning&& warning) { warningConsumer(std::move(warning)); },
            [](SkipRow) {}
        }, ParseRow(line, cols));
    }
}

std::vector<Landmark> osc::lm::ReadLandmarksFromCSVIntoVectorOrThrow(
    const std::filesystem::path& path)
{
    std::ifstream in{path};
    if (not in) {
        std::stringstream ss;
        ss << path.string() << ": cannot open landmarks file for reading";
        throw std::runtime_error{std::move(ss).str()};
    }

    std::vector<Landmark> rv;
    ReadLandmarksFromCSV(in, [&rv](auto&& lm) { rv.push_back(std::forward<decltype(lm)>(lm)); });
    return rv;
}

void osc::lm::WriteLandmarksToCSV(
    std::ostream& out,
    const std::function<std::optional<Landmark>()>& landmarkProducer,
    LandmarkCSVFlags flags)
{
    // if applicable, emit header
    if (!(flags & LandmarkCSVFlags::NoHeader))
    {
        if (flags & LandmarkCSVFlags::NoNames)
        {
            write_csv_row(out, {{"x", "y", "z"}});
        }
        else
        {
            write_csv_row(out, {{"name", "x", "y", "z"}});
        }
    }

    // emit data emitted by the landmark producer (until std::nullopt) as data rows
    for (auto lm = landmarkProducer(); lm; lm = landmarkProducer())
    {
        using std::to_string;
        auto x = lm->position.x;
        auto y = lm->position.y;
        auto z = lm->position.z;

        if (flags & LandmarkCSVFlags::NoNames)
        {
            write_csv_row(out, {{to_string(x), to_string(y), to_string(z)}});
        }
        else
        {
            write_csv_row(out, {{lm->maybeName.value_or("unnamed"), to_string(x), to_string(y), to_string(z)}});
        }
    }
}

std::vector<NamedLandmark> osc::lm::GenerateNames(
    std::span<const Landmark> lms,
    std::string_view prefix)
{
    // collect up all already-named landmarks
    std::unordered_set<std::string_view> suppliedNames;
    for (const auto& lm : lms)
    {
        if (lm.maybeName)
        {
            suppliedNames.insert(*lm.maybeName);
        }
    }

    // helper: either get, or generate, a name for the given landmark
    auto getName = [&prefix, &suppliedNames, i=0](const Landmark& lm) mutable -> std::string
    {
        if (lm.maybeName)
        {
            return *lm.maybeName;
        }

        auto nextName = [&prefix, &i]() { return std::string{prefix} + std::to_string(i++); };
        std::string name = nextName();
        while (suppliedNames.contains(name))
        {
            name = nextName();
        }
        return name;
    };

    std::vector<NamedLandmark> rv;
    rv.reserve(lms.size());
    for (const auto& lm : lms)
    {
        rv.push_back(NamedLandmark{getName(lm), lm.position});
    }
    return rv;
}

void osc::lm::TryPairingLandmarks(
    std::vector<Landmark> a,
    std::vector<Landmark> b,
    const std::function<void(const MaybeNamedLandmarkPair&)>& consumer)
{
    size_t nunnamed = 0;

    // handle/pair all elements in `a`
    for (auto& lm : a) {
        const auto it = rgs::find_if(b, std::bind_front(SameNameOrBothUnnamed, std::cref(lm)));
        std::string name = lm.maybeName ? *std::move(lm.maybeName) : GenerateName(nunnamed++);

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
        std::string name = lm.maybeName ? std::move(lm.maybeName).value() : GenerateName(nunnamed++);
        consumer(MaybeNamedLandmarkPair{name, std::nullopt, lm.position});
    }
}
