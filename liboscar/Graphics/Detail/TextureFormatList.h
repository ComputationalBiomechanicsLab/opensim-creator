#pragma once

#include <liboscar/Graphics/TextureFormat.h>
#include <liboscar/Utils/EnumHelpers.h>

namespace osc::detail
{
    using TextureFormatList = OptionList<TextureFormat,
        TextureFormat::R8,
        TextureFormat::RG16,
        TextureFormat::RGB24,
        TextureFormat::RGBA32,
        TextureFormat::RGFloat,
        TextureFormat::RGBFloat,
        TextureFormat::RGBAFloat
    >;
}
