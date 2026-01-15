#pragma once

#include <liboscar/graphics/detail/depth_stencil_render_buffer_format_list.h>
#include <liboscar/graphics/opengl/depth_stencil_render_buffer_format_open_gl_traits.h>
#include <liboscar/graphics/opengl/gl.h>
#include <liboscar/utils/enum_helpers.h>

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
