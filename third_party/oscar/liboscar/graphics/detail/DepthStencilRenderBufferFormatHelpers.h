#pragma once

#include <liboscar/graphics/detail/CPUDataType.h>
#include <liboscar/graphics/detail/CPUImageFormat.h>
#include <liboscar/graphics/detail/DepthStencilRenderBufferFormatList.h>
#include <liboscar/graphics/detail/DepthStencilRenderBufferFormatTraits.h>
#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/EnumHelpers.h>

#include <array>

namespace osc::detail
{
    constexpr bool has_stencil_component(DepthStencilRenderBufferFormat format)
    {
        constexpr auto lut = []<DepthStencilRenderBufferFormat... Formats>(OptionList<DepthStencilRenderBufferFormat, Formats...>) {
            return std::to_array({ DepthStencilRenderBufferFormatTraits<Formats>::has_stencil_component... });
        }(DepthStencilRenderBufferFormatList{});

        return lut.at(to_index(format));
    }

    constexpr CStringView get_label(DepthStencilRenderBufferFormat format)
    {
        constexpr auto lut = []<DepthStencilRenderBufferFormat... Formats>(OptionList<DepthStencilRenderBufferFormat, Formats...>) {
            return std::to_array({ DepthStencilRenderBufferFormatTraits<Formats>::label... });
        }(DepthStencilRenderBufferFormatList{});

        return lut.at(to_index(format));
    }

    constexpr CPUImageFormat equivalent_cpu_image_format_of(DepthStencilRenderBufferFormat format)
    {
        constexpr auto lut = []<DepthStencilRenderBufferFormat... Formats>(OptionList<DepthStencilRenderBufferFormat, Formats...>) {
            return std::to_array({ DepthStencilRenderBufferFormatTraits<Formats>::equivalent_cpu_image_format... });
        }(DepthStencilRenderBufferFormatList{});

        return lut.at(to_index(format));
    }

    constexpr CPUDataType equivalent_cpu_datatype_of(DepthStencilRenderBufferFormat format)
    {
        constexpr auto lut = []<DepthStencilRenderBufferFormat... Formats>(OptionList<DepthStencilRenderBufferFormat, Formats...>) {
            return std::to_array({ DepthStencilRenderBufferFormatTraits<Formats>::equivalent_cpu_datatype... });
        }(DepthStencilRenderBufferFormatList{});

        return lut.at(to_index(format));
    }
}
