#pragma once

#include <oscar/Graphics/OpenGL/Gl.hpp>
#include <oscar/Graphics/TextureFormat.hpp>

namespace osc::detail
{
    template<TextureFormat>
    struct TextureFormatOpenGLTraits;

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::R8> {
        static inline constexpr GLint unpack_alignment = 1;
        static inline constexpr GLenum internal_format_srgb = GL_R8;
        static inline constexpr GLenum internal_format_linear = GL_R8;
        static inline constexpr GLenum image_color_format = GL_RED;
        static inline constexpr GLint pixel_pack_alignment = 1;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RG16> {
        static inline constexpr GLint unpack_alignment = 1;
        static inline constexpr GLenum internal_format_srgb = GL_RG;
        static inline constexpr GLenum internal_format_linear = GL_RG;
        static inline constexpr GLenum image_color_format = GL_RG;
        static inline constexpr GLint pixel_pack_alignment = 1;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGB24> {
        static inline constexpr GLint unpack_alignment = 1;
        static inline constexpr GLenum internal_format_srgb = GL_SRGB8;
        static inline constexpr GLenum internal_format_linear = GL_RGB8;
        static inline constexpr GLenum image_color_format = GL_RGB;
        static inline constexpr GLint pixel_pack_alignment = 1;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGBA32> {
        static inline constexpr GLint unpack_alignment = 4;
        static inline constexpr GLenum internal_format_srgb = GL_SRGB8_ALPHA8;
        static inline constexpr GLenum internal_format_linear = GL_RGBA8;
        static inline constexpr GLenum image_color_format = GL_RGBA;
        static inline constexpr GLint pixel_pack_alignment = 4;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGFloat> {
        static inline constexpr GLint unpack_alignment = 4;
        static inline constexpr GLenum internal_format_srgb = GL_RG32F;
        static inline constexpr GLenum internal_format_linear = GL_RG32F;
        static inline constexpr GLenum image_color_format = GL_RG;
        static inline constexpr GLint pixel_pack_alignment = 4;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGBFloat> {
        static inline constexpr GLint unpack_alignment = 4;
        static inline constexpr GLenum internal_format_srgb = GL_RGB32F;
        static inline constexpr GLenum internal_format_linear = GL_RGB32F;
        static inline constexpr GLenum image_color_format = GL_RGB;
        static inline constexpr GLint pixel_pack_alignment = 4;
    };

    template<>
    struct TextureFormatOpenGLTraits<TextureFormat::RGBAFloat> {
        static inline constexpr GLint unpack_alignment = 4;
        static inline constexpr GLenum internal_format_srgb = GL_RGBA32F;
        static inline constexpr GLenum internal_format_linear = GL_RGBA32F;
        static inline constexpr GLenum image_color_format = GL_RGBA;
        static inline constexpr GLint pixel_pack_alignment = 4;
    };
}
