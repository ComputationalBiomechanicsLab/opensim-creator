#pragma once

#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/NonTypelist.hpp>

namespace osc::detail
{
    using TextureFormatList = NonTypelist<TextureFormat,
        TextureFormat::RGBA32,
        TextureFormat::RGB24,
        TextureFormat::R8,
        TextureFormat::RGBFloat,
        TextureFormat::RGBAFloat
    >;
    static_assert(NonTypelistSizeV<TextureFormatList> == NumOptions<TextureFormat>());
}
