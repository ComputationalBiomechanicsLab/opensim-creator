#include "FramesHelpers.hpp"

#include <OpenSimCreator/Documents/Frames/FrameAxis.hpp>
#include <OpenSimCreator/Documents/Frames/FrameDefinition.hpp>
#include <OpenSimCreator/Documents/Frames/FramesFile.hpp>

#include <oscar/Utils/Assertions.hpp>
#include <toml++/toml.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

using osc::frames::AreOrthogonal;
using osc::frames::FrameAxis;
using osc::frames::FrameDefinition;
using osc::frames::FramesFile;
using osc::frames::TryParseAsFrameAxis;

namespace
{
    std::string TryGetFrameEntry(toml::table const& t, std::string_view tableName, std::string_view key)
    {
        if (auto v = t[key].value<std::string>())
        {
            return std::move(v).value();
        }
        else
        {
            std::stringstream ss;
            ss << tableName << ": is missing entry '" << key << '\'';
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    FrameAxis TryGetFrameEntryAsAxis(toml::table const& t, std::string_view tableName, std::string_view key)
    {
        std::string const str = TryGetFrameEntry(t, tableName, key);
        if (auto axis = TryParseAsFrameAxis(str))
        {
            return *axis;
        }
        else
        {
            std::stringstream ss;
            ss << tableName << ": the entry at '" << key << "' (" << str << ") cannot be parsed as a frame axis (x, y, z, -x, -y, -z)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    FrameDefinition TryParseAsFrameDefinition(std::string_view name, toml::table const& t)
    {
        FrameDefinition rv
        {
            std::string{name},
            TryGetFrameEntry(t, name, "associated_mesh"),
            TryGetFrameEntry(t, name, "origin_location"),
            TryGetFrameEntry(t, name, "axis_edge_begin"),
            TryGetFrameEntry(t, name, "axis_edge_end"),
            TryGetFrameEntryAsAxis(t, name, "axis_edge_axis"),
            TryGetFrameEntry(t, name, "nonparallel_edge_begin"),
            TryGetFrameEntry(t, name, "nonparallel_edge_end"),
            TryGetFrameEntryAsAxis(t, name, "cross_product_edge_axis"),
        };

        if (!AreOrthogonal(rv.getAxisEdgeAxis(), rv.getCrossProductEdgeAxis()))
        {
            std::stringstream ss;
            ss << name << ": axis_edge_axis (" << rv.getAxisEdgeAxis() << ") is not orthogonal to cross_product_edge_axis (" << rv.getCrossProductEdgeAxis() << ')';
            throw std::runtime_error{std::move(ss).str()};
        }

        if (rv.getAxisEdgeBeginLandmarkName() == rv.getAxisEdgeEndLandmarkName())
        {
            std::stringstream ss;
            ss << name << ": axis_edge_begin and axis_edge_end point to the same landmark (" << rv.getAxisEdgeBeginLandmarkName() << ')';
            throw std::runtime_error{std::move(ss).str()};
        }

        if (rv.getNonParallelEdgeBeginLandmarkName() == rv.getNonParallelEdgeEndLandmarkName())
        {
            std::stringstream ss;
            ss << name << ": nonparallel_edge_begin and nonparallel_edge_end point to the same landmark (" << rv.getNonParallelEdgeBeginLandmarkName() << ')';
            throw std::runtime_error{std::move(ss).str()};
        }

        return rv;
    }
}

FramesFile osc::frames::ReadFramesFromTOML(std::istream& in)
{
    toml::table table = toml::parse(in);

    std::vector<FrameDefinition> frameDefs;
    if (auto const* frames = table["frames"].as_table())
    {
        for (auto const& entry : *frames)
        {
            if (auto const* frame = entry.second.as_table())
            {
                frameDefs.push_back(TryParseAsFrameDefinition(entry.first.str(), *frame));
            }
        }
    }

    return FramesFile{std::move(frameDefs)};
}
