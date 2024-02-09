#pragma once

#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/TextureChannelFormat.h>
#include <oscar/Graphics/TextureFormat.h>

#include <cstddef>

namespace osc::detail
{
    template<TextureFormat>
    struct TextureFormatTraits;

    template<>
    struct TextureFormatTraits<TextureFormat::R8> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::R8;
        static inline constexpr size_t num_channels = 1;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RG16> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RG;
        static inline constexpr size_t num_channels = 2;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGB24> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGB;
        static inline constexpr size_t num_channels = 3;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGBA32> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGBA;
        static inline constexpr size_t num_channels = 4;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGFloat> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RG;
        static inline constexpr size_t num_channels = 2;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Float32;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGBFloat> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGB;
        static inline constexpr size_t num_channels = 3;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Float32;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGBAFloat> {
        static inline constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
        static inline constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGBA;
        static inline constexpr size_t num_channels = 4;
        static inline constexpr TextureChannelFormat channel_format = TextureChannelFormat::Float32;
    };
}
