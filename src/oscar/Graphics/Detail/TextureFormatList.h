#pragma once

#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/NonTypelist.h>

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
