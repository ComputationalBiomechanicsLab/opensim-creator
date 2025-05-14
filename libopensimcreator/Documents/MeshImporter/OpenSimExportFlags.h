#pragma once

#include <utility>

namespace osc::mi
{
    // user-editable flags that affect how an OpenSim::Model is created by the mesh importer
    enum class ModelCreationFlags {
        None                      = 0,
        ExportStationsAsMarkers   = 1<<0,
    };

    constexpr bool operator&(const ModelCreationFlags& lhs, const ModelCreationFlags& rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    constexpr ModelCreationFlags operator+(const ModelCreationFlags& lhs, const ModelCreationFlags& rhs)
    {
        return static_cast<ModelCreationFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    constexpr ModelCreationFlags operator-(const ModelCreationFlags& lhs, const ModelCreationFlags& rhs)
    {
        return static_cast<ModelCreationFlags>(std::to_underlying(lhs) & ~std::to_underlying(rhs));
    }
}
