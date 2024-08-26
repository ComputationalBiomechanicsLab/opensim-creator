#pragma once

#include <oscar/Graphics/Detail/DepthStencilFormatList.h>
#include <oscar/Graphics/OpenGL/DepthStencilFormatOpenGLTraits.h>
#include <oscar/Graphics/OpenGL/Gl.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>

namespace osc::detail
{
    constexpr GLenum to_opengl_internal_color_format_enum(const DepthStencilFormat& format)
    {
        constexpr auto lut = []<DepthStencilFormat... Formats>(OptionList<DepthStencilFormat, Formats...>) {
            return std::to_array({ DepthStencilFormatOpenGLTraits<Formats>::internal_color_format... });
        }(DepthStencilFormatList{});

        return lut.at(to_index(format));
    }
}
