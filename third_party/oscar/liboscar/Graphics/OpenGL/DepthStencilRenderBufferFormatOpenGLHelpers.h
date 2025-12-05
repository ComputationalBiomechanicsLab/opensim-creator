#pragma once

#include <liboscar/Graphics/Detail/DepthStencilRenderBufferFormatList.h>
#include <liboscar/Graphics/OpenGL/DepthStencilRenderBufferFormatOpenGLTraits.h>
#include <liboscar/Graphics/OpenGL/Gl.h>
#include <liboscar/Utils/EnumHelpers.h>

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
