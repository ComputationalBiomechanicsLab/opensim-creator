#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc::mi
{
    // user-editable flags that affect how the model is created from the model graph
    enum class ModelCreationFlags {
        None,
        ExportStationsAsMarkers,
    };

    constexpr bool operator&(ModelCreationFlags const& lhs, ModelCreationFlags const& rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }

    constexpr ModelCreationFlags operator+(ModelCreationFlags const& lhs, ModelCreationFlags const& rhs)
    {
        return static_cast<ModelCreationFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr ModelCreationFlags operator-(ModelCreationFlags const& lhs, ModelCreationFlags const& rhs)
    {
        return static_cast<ModelCreationFlags>(osc::to_underlying(lhs) & ~osc::to_underlying(rhs));
    }
}
