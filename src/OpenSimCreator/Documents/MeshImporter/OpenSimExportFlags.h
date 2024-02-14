#pragma once

#include <oscar/Shims/Cpp23/utility.h>

namespace osc::mi
{
    // user-editable flags that affect how an OpenSim::Model is created by the mesh importer
    enum class ModelCreationFlags {
        None                      = 0,
        ExportStationsAsMarkers   = 1<<0,
    };

    constexpr bool operator&(ModelCreationFlags const& lhs, ModelCreationFlags const& rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    constexpr ModelCreationFlags operator+(ModelCreationFlags const& lhs, ModelCreationFlags const& rhs)
    {
        return static_cast<ModelCreationFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr ModelCreationFlags operator-(ModelCreationFlags const& lhs, ModelCreationFlags const& rhs)
    {
        return static_cast<ModelCreationFlags>(cpp23::to_underlying(lhs) & ~cpp23::to_underlying(rhs));
    }
}
