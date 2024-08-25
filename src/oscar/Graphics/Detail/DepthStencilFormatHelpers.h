#pragma once

#include <oscar/Graphics/Detail/DepthStencilFormatList.h>
#include <oscar/Graphics/Detail/DepthStencilFormatTraits.h>

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
}
