#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <cstddef>

namespace osc
{
    // Describes the type of data held by [1..N] columns in the source data.
    enum class DataPointType {
        Point,
        ForcePoint,
        BodyForce,
        Orientation,
        Unknown,
        NUM_OPTIONS,
    };

    // A compile-time typelist of all possible `DataPointType`s.
    using DataPointTypeList = OptionList<DataPointType,
        DataPointType::Point,
        DataPointType::ForcePoint,
        DataPointType::BodyForce,
        DataPointType::Orientation,
        DataPointType::Unknown
    >;

    // Compile-time traits associated with a `DataPointType`.
    template<DataPointType>
    struct ColumnDataTypeTraits;

    template<>
    struct ColumnDataTypeTraits<DataPointType::Point> final {
        static constexpr CStringView label = "Point";
        static constexpr size_t num_elements = 3;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::ForcePoint> final {
        static constexpr CStringView label = "ForcePoint";
        static constexpr size_t num_elements = 6;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::BodyForce> final {
        static constexpr CStringView label = "BodyForce";
        static constexpr size_t num_elements = 3;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::Orientation> final {
        static constexpr CStringView label = "Orientation";
        static constexpr size_t num_elements = 4;
    };

    template<>
    struct ColumnDataTypeTraits<DataPointType::Unknown> final {
        static constexpr CStringView label = "Unknown";
        static constexpr size_t num_elements = 1;
    };

    // Returns the number of elements in a given `DataPointType`.
    constexpr size_t numElementsIn(DataPointType t)
    {
        constexpr auto lut = []<DataPointType... Types>(OptionList<DataPointType, Types...>)
        {
            return std::to_array({ ColumnDataTypeTraits<Types>::num_elements... });
        }(DataPointTypeList{});

        return lut.at(to_index(t));
    }

    // Returns a human-readable label for a given `DataPointType`.
    constexpr CStringView labelFor(DataPointType t)
    {
        constexpr auto lut = []<DataPointType... Types>(OptionList<DataPointType, Types...>)
        {
            return std::to_array({ ColumnDataTypeTraits<Types>::label... });
        }(DataPointTypeList{});

        return lut.at(to_index(t));
    }
}
