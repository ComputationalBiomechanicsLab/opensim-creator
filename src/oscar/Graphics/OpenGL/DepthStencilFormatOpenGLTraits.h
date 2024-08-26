#pragma once

#include <oscar/Graphics/OpenGL/Gl.h>
#include <oscar/Graphics/DepthStencilFormat.h>

namespace osc::detail
{
    template<DepthStencilFormat>
    struct DepthStencilFormatOpenGLTraits;

    template<>
    struct DepthStencilFormatOpenGLTraits<DepthStencilFormat::D24_UNorm_S8_UInt> final {
        static inline constexpr GLenum internal_color_format = GL_DEPTH24_STENCIL8;
    };

    template<>
    struct DepthStencilFormatOpenGLTraits<DepthStencilFormat::D32_SFloat> final {
        static inline constexpr GLenum internal_color_format = GL_DEPTH_COMPONENT32F;
    };
}
