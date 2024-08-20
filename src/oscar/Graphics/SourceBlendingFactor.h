#pragma once

#include <cstdint>

namespace osc
{
	enum class SourceBlendingFactor : uint8_t {
        One,
        Zero,
        SourceAlpha,
        OneMinusSourceAlpha,
        NUM_OPTIONS,

        Default = SourceAlpha,
	};
}