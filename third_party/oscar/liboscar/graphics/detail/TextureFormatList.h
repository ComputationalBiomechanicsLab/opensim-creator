#pragma once

#include <liboscar/graphics/TextureFormat.h>
#include <liboscar/utils/EnumHelpers.h>

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
