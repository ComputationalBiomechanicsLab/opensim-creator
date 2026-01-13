#pragma once

#include <liboscar/graphics/detail/DepthStencilRenderBufferFormatList.h>
#include <liboscar/graphics/opengl/DepthStencilRenderBufferFormatOpenGLTraits.h>
#include <liboscar/graphics/opengl/Gl.h>
#include <liboscar/utils/EnumHelpers.h>

#include <array>

namespace osc::detail
{
    constexpr GLenum to_opengl_internal_color_format_enum(const DepthStencilRenderBufferFormat& format)
    {
        constexpr auto lut = []<DepthStencilRenderBufferFormat... Formats>(OptionList<DepthStencilRenderBufferFormat, Formats...>)
        {
            return std::to_array({ DepthStencilRenderBufferFormatOpenGLTraits<Formats>::internal_color_format... });
        }(DepthStencilRenderBufferFormatList{});

        return lut.at(to_index(format));
    }
}
