#pragma once

#include <liboscar/graphics/detail/opengl/gl.h>
#include <liboscar/graphics/depth_stencil_render_buffer_format.h>

namespace osc::detail
{
    template<DepthStencilRenderBufferFormat>
    struct DepthStencilRenderBufferFormatOpenGLTraits;

    template<>
    struct DepthStencilRenderBufferFormatOpenGLTraits<DepthStencilRenderBufferFormat::D24_UNorm_S8_UInt> final {
        static inline constexpr GLenum internal_color_format = GL_DEPTH24_STENCIL8;
    };

    template<>
    struct DepthStencilRenderBufferFormatOpenGLTraits<DepthStencilRenderBufferFormat::D32_SFloat> final {
        static inline constexpr GLenum internal_color_format = GL_DEPTH_COMPONENT32F;
    };
}
