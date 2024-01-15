#pragma once

#include <OpenSimCreator/Documents/FrameDefinition/AxisIndex.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace osc::fd
{
    // the potentially negated index of an axis in n-dimensional space
    struct MaybeNegatedAxis final {
        constexpr MaybeNegatedAxis(
            AxisIndex axisIndex_,
            bool isNegated_) :

            axisIndex{axisIndex_},
            isNegated{isNegated_}
        {
        }
        AxisIndex axisIndex;
        bool isNegated;
    };

    constexpr MaybeNegatedAxis Next(MaybeNegatedAxis ax)
    {
        return MaybeNegatedAxis{Next(ax.axisIndex), ax.isNegated};
    }

    // returns `true` if the arguments are orthogonal to eachover; otherwise, returns `false`
    constexpr bool IsOrthogonal(MaybeNegatedAxis const& a, MaybeNegatedAxis const& b)
    {
        return a.axisIndex != b.axisIndex;
    }

    // returns a (possibly negated) `AxisIndex` parsed from the given input
    //
    // if the input is invalid in some way, returns `std::nullopt`
    constexpr std::optional<MaybeNegatedAxis> ParseAxisDimension(std::string_view s)
    {
        if (s.empty())
        {
            return std::nullopt;
        }

        // handle and consume sign prefix
        bool const isNegated = s.front() == '-';
        if (isNegated || s.front() == '+')
        {
            s = s.substr(1);
        }

        if (s.empty())
        {
            return std::nullopt;
        }

        // handle axis suffix
        std::optional<AxisIndex> const maybeAxisIndex = ParseAxisIndex(s.front());
        if (!maybeAxisIndex)
        {
            return std::nullopt;
        }

        return MaybeNegatedAxis{*maybeAxisIndex, isNegated};
    }

    // returns a string representation of the given (possibly negated) axis
    constexpr std::string ToString(MaybeNegatedAxis const& ax)
    {
        std::string rv;
        rv.reserve(2);
        rv.push_back(ax.isNegated ? '-' : '+');
        rv.push_back(ToChar(ax.axisIndex));
        return rv;
    }
}
