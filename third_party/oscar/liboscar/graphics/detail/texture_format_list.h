#pragma once

#include <liboscar/graphics/texture_format.h>
#include <liboscar/utils/enum_helpers.h>

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
