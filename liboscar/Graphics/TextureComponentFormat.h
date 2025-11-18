#pragma once

#include <cstddef>

namespace osc
{
    enum class TextureComponentFormat {
        Uint8,
        Float32,
        NUM_OPTIONS,
    };

    size_t num_bytes_per_component_in(TextureComponentFormat);
}
