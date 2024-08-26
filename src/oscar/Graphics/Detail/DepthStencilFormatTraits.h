#pragma once

#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Utils/CStringView.h>

namespace osc::detail
{
    template<DepthStencilFormat>
    struct DepthStencilFormatTraits;

    template<>
    struct DepthStencilFormatTraits<DepthStencilFormat::D24_UNorm_S8_UInt> final {
        static inline constexpr bool has_stencil_component = true;
        static inline constexpr CStringView label = "D24_UNorm_S8_UInt";
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::DepthStencil;
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedInt24_8;
    };

    template<>
    struct DepthStencilFormatTraits<DepthStencilFormat::D32_SFloat> final {
        static inline constexpr bool has_stencil_component = false;
        static inline constexpr CStringView label = "D32_SFloat";
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::Depth;
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
    };
}
