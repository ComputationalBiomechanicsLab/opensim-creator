#pragma once

#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/Detail/DepthStencilFormatList.h>
#include <oscar/Graphics/Detail/DepthStencilFormatTraits.h>
#include <oscar/Utils/CStringView.h>

#include <array>

namespace osc::detail
{
    constexpr bool has_stencil_component(DepthStencilFormat format)
    {
        constexpr auto lut = []<DepthStencilFormat... Formats>(OptionList<DepthStencilFormat, Formats...>) {
            return std::to_array({ DepthStencilFormatTraits<Formats>::has_stencil_component... });
        }(DepthStencilFormatList{});

        return lut.at(to_index(format));
    }

    constexpr CStringView get_label(DepthStencilFormat format)
    {
        constexpr auto lut = []<DepthStencilFormat... Formats>(OptionList<DepthStencilFormat, Formats...>) {
            return std::to_array({ DepthStencilFormatTraits<Formats>::label... });
        }(DepthStencilFormatList{});

        return lut.at(to_index(format));
    }

    constexpr CPUImageFormat equivalent_cpu_image_format_of(DepthStencilFormat format)
    {
        constexpr auto lut = []<DepthStencilFormat... Formats>(OptionList<DepthStencilFormat, Formats...>) {
            return std::to_array({ DepthStencilFormatTraits<Formats>::equivalent_cpu_image_format... });
        }(DepthStencilFormatList{});

        return lut.at(to_index(format));
    }

    constexpr CPUDataType equivalent_cpu_datatype_of(DepthStencilFormat format)
    {
        constexpr auto lut = []<DepthStencilFormat... Formats>(OptionList<DepthStencilFormat, Formats...>) {
            return std::to_array({ DepthStencilFormatTraits<Formats>::equivalent_cpu_datatype... });
        }(DepthStencilFormatList{});

        return lut.at(to_index(format));
    }
}
