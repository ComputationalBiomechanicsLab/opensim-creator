#include "LandmarkHelpers.hpp"

#include <oscar/Formats/CSV.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <optional>
#include <span>
#include <string>
#include <vector>

using osc::lm::Landmark;
using osc::FromCharsStripWhitespace;
using osc::Vec3;

namespace
{
    std::optional<Landmark> ParseRow(std::span<std::string const> cols)
    {
        if (cols.size() < 3)
        {
            return std::nullopt;  // too few columns
        }

        // >=4 columns implies that the first column is probably a label
        std::optional<std::string> maybeName;
        std::span<std::string const> data = cols;
        if (cols.size() >= 4)
        {
            maybeName = cols.front();
            data = data.subspan(1);
        }

        std::optional<float> const x = FromCharsStripWhitespace(data[0]);
        std::optional<float> const y = FromCharsStripWhitespace(data[1]);
        std::optional<float> const z = FromCharsStripWhitespace(data[2]);
        if (!(x && y && z))
        {
            return std::nullopt;  // a column was non-numeric: ignore entire line
        }

        return Landmark{std::move(maybeName), Vec3{*x, *y, *z}};
    }
}

void osc::lm::ReadLandmarksFromCSV(
    std::istream& in,
    std::function<void(Landmark&&)> const& landmarkConsumer)
{
    std::vector<std::string> cols;
    while (ReadCSVRowIntoVector(in, cols))
    {
        if (auto row = ParseRow(cols))
        {
            landmarkConsumer(std::move(row).value());
        }
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
