#pragma once

#include <oscar/Graphics/OpenGL/Gl.h>
#include <oscar/Graphics/TextureFormat.h>

namespace osc::detail
{
    template<TextureFormat>
    struct TextureFormatOpenGLTraits;

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::R8> {
        static constexpr GLint unpack_alignment = 1;
        static constexpr GLenum internal_format_srgb = GL_R8;
        static constexpr GLenum internal_format_linear = GL_R8;
        static constexpr GLenum image_color_format = GL_RED;
        static constexpr GLint pixel_pack_alignment = 1;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RG16> {
        static constexpr GLint unpack_alignment = 1;
        static constexpr GLenum internal_format_srgb = GL_RG;
        static constexpr GLenum internal_format_linear = GL_RG;
        static constexpr GLenum image_color_format = GL_RG;
        static constexpr GLint pixel_pack_alignment = 1;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGB24> {
        static constexpr GLint unpack_alignment = 1;
        static constexpr GLenum internal_format_srgb = GL_SRGB8;
        static constexpr GLenum internal_format_linear = GL_RGB8;
        static constexpr GLenum image_color_format = GL_RGB;
        static constexpr GLint pixel_pack_alignment = 1;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGBA32> {
        static constexpr GLint unpack_alignment = 4;
        static constexpr GLenum internal_format_srgb = GL_SRGB8_ALPHA8;
        static constexpr GLenum internal_format_linear = GL_RGBA8;
        static constexpr GLenum image_color_format = GL_RGBA;
        static constexpr GLint pixel_pack_alignment = 4;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGFloat> {
        static constexpr GLint unpack_alignment = 4;
        static constexpr GLenum internal_format_srgb = GL_RG32F;
        static constexpr GLenum internal_format_linear = GL_RG32F;
        static constexpr GLenum image_color_format = GL_RG;
        static constexpr GLint pixel_pack_alignment = 4;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGBFloat> {
        static constexpr GLint unpack_alignment = 4;
        static constexpr GLenum internal_format_srgb = GL_RGB32F;
        static constexpr GLenum internal_format_linear = GL_RGB32F;
        static constexpr GLenum image_color_format = GL_RGB;
        static constexpr GLint pixel_pack_alignment = 4;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGBAFloat> {
        static constexpr GLint unpack_alignment = 4;
        static constexpr GLenum internal_format_srgb = GL_RGBA32F;
        static constexpr GLenum internal_format_linear = GL_RGBA32F;
        static constexpr GLenum image_color_format = GL_RGBA;
        static constexpr GLint pixel_pack_alignment = 4;
    };
}
