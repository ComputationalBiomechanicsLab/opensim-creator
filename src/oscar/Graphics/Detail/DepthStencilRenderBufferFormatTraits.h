#pragma once

#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Utils/CStringView.h>

namespace osc::detail
{
    template<DepthStencilRenderBufferFormat>
    struct DepthStencilRenderBufferFormatTraits;

    template<>
    struct DepthStencilRenderBufferFormatTraits<DepthStencilRenderBufferFormat::D24_UNorm_S8_UInt> final {
        static constexpr bool has_stencil_component = true;
        static constexpr CStringView label = "D24_UNorm_S8_UInt";
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::DepthStencil;
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedInt24_8;
    };

    template<>
    struct DepthStencilRenderBufferFormatTraits<DepthStencilRenderBufferFormat::D32_SFloat> final {
        static constexpr bool has_stencil_component = false;
        static constexpr CStringView label = "D32_SFloat";
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::Depth;
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
    };
}
