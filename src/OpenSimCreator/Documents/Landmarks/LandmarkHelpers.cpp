#include "LandmarkHelpers.hpp"

#include <oscar/Formats/CSV.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/StdVariantHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using osc::lm::CSVParseWarning;
using osc::lm::Landmark;
using osc::lm::NamedLandmark;
using osc::FromCharsStripWhitespace;
using osc::TrimLeadingAndTrailingWhitespace;
using osc::Vec3;

namespace
{
    struct SkipRow final {};

    using ParseResult = std::variant<Landmark, CSVParseWarning, SkipRow>;

    ParseResult ParseRow(size_t lineNum, std::span<std::string const> cols)
    {
        if (cols.empty() || (cols.size() == 1 && TrimLeadingAndTrailingWhitespace(cols.front()).empty()))
        {
            return SkipRow{};  // whitespace row, or trailing newline
        }
        if (cols.size() < 3)
        {
            return CSVParseWarning{lineNum, "too few columns in this row"};
        }

        // >=4 columns implies that the first column is a label column
        std::optional<std::string> maybeName;
        std::span<std::string const> data = cols;
        if (cols.size() >= 4)
        {
            maybeName = cols.front();
            data = data.subspan(1);
        }

        std::optional<float> const x = FromCharsStripWhitespace(data[0]);
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
        std::optional<float> const y = FromCharsStripWhitespace(data[1]);
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
        std::optional<float> const z = FromCharsStripWhitespace(data[2]);
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
}

std::string osc::lm::to_string(CSVParseWarning const& warning)
{
    std::stringstream ss;
    size_t const displayedLineNumber = warning.lineNumber+1;  // user-facing software (e.g. IDEs) start at 1
    ss << "line " << displayedLineNumber << ": " << warning.message;
    return std::move(ss).str();
}

void osc::lm::ReadLandmarksFromCSV(
    std::istream& in,
    std::function<void(Landmark&&)> const& landmarkConsumer,
    std::function<void(CSVParseWarning)> const& warningConsumer)
{
    std::vector<std::string> cols;
    for (size_t line = 0; ReadCSVRowIntoVector(in, cols); ++line)
    {
        std::visit(Overload
        {
            [&landmarkConsumer](Landmark&& lm) { landmarkConsumer(std::move(lm)); },
            [&warningConsumer](CSVParseWarning&& warning) { warningConsumer(std::move(warning)); },
            [](SkipRow) {}
        }, ParseRow(line, cols));
    }
}

void osc::lm::WriteLandmarksToCSV(
    std::ostream& out,
    std::function<std::optional<Landmark>()> const& landmarkProducer,
    LandmarkCSVFlags flags)
{
    // if applicable, emit header
    if (!(flags & LandmarkCSVFlags::NoHeader))
    {
        if (flags & LandmarkCSVFlags::NoNames)
        {
            WriteCSVRow(out, {{"x", "y", "z"}});
        }
        else
        {
            WriteCSVRow(out, {{"name", "x", "y", "z"}});
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
            WriteCSVRow(out, {{to_string(x), to_string(y), to_string(z)}});
        }
        else
        {
            WriteCSVRow(out, {{lm->maybeName.value_or("unnamed"), to_string(x), to_string(y), to_string(z)}});
        }
    }
}

std::vector<NamedLandmark> osc::lm::GenerateNames(
    std::span<Landmark const> lms,
    std::string_view prefix)
{
    // collect up all already-named landmarks
    std::unordered_set<std::string_view> suppliedNames;
    for (auto const& lm : lms)
    {
        if (lm.maybeName)
        {
            suppliedNames.insert(*lm.maybeName);
        }
    }

    // helper: either get, or generate, a name for the given landmark
    auto getName = [&prefix, &suppliedNames, i=0](Landmark const& lm) mutable -> std::string
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
    for (auto const& lm : lms)
    {
        rv.push_back(NamedLandmark{getName(lm), lm.position});
    }
    return rv;
}
