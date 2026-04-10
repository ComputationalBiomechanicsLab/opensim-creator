#pragma once

#include <cstdint>

namespace osc
{
    enum class CullMode : uint8_t {
        Off,
        Front,
        Back,
        NUM_OPTIONS,

        Default = Off,
    };
}
