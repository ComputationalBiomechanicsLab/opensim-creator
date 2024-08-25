#pragma once

#include <oscar/Graphics/DepthStencilFormat.h>

namespace osc::detail
{
    template<DepthStencilFormat>
    struct DepthStencilFormatTraits;

    template<>
    struct DepthStencilFormatTraits<DepthStencilFormat::D24_UNorm_S8_UInt> final {
        static inline constexpr bool has_stencil_component = true;
    };

    template<>
    struct DepthStencilFormatTraits<DepthStencilFormat::D32_SFloat> final {
        static inline constexpr bool has_stencil_component = false;
    };
}
