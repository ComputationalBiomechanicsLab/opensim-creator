#pragma once

#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/TextureComponentFormat.h>
#include <oscar/Graphics/TextureFormat.h>

#include <cstddef>

namespace osc::detail
{
    template<TextureFormat>
    struct TextureFormatTraits;

    template<>
    struct TextureFormatTraits<TextureFormat::R8> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::R8;
        static constexpr size_t num_components = 1;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RG16> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RG;
        static constexpr size_t num_components = 2;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGB24> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGB;
        static constexpr size_t num_components = 3;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGBA32> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::UnsignedByte;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGBA;
        static constexpr size_t num_components = 4;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Uint8;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGFloat> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RG;
        static constexpr size_t num_components = 2;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Float32;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGBFloat> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGB;
        static constexpr size_t num_components = 3;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Float32;
    };

    template<>
    struct TextureFormatTraits<TextureFormat::RGBAFloat> {
        static constexpr CPUDataType equivalent_cpu_datatype = CPUDataType::Float;
        static constexpr CPUImageFormat equivalent_cpu_image_format = CPUImageFormat::RGBA;
        static constexpr size_t num_components = 4;
        static constexpr TextureComponentFormat component_format = TextureComponentFormat::Float32;
    };
}
