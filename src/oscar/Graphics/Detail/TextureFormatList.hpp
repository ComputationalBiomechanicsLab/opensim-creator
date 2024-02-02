#pragma once

#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/NonTypelist.hpp>

namespace osc::detail
{
    using TextureFormatList = NonTypelist<TextureFormat,
        TextureFormat::R8,
        TextureFormat::RG16,
        TextureFormat::RGB24,
        TextureFormat::RGBA32,
        TextureFormat::RGFloat,
        TextureFormat::RGBFloat,
        TextureFormat::RGBAFloat
    >;
    static_assert(NonTypelistSizeV<TextureFormatList> == NumOptions<TextureFormat>());
}
