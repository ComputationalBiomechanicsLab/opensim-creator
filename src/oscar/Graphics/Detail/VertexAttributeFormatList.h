#pragma once

#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Utils/NonTypelist.h>

namespace osc::detail
{
    using VertexAttributeFormatList = NonTypelist<VertexAttributeFormat,
        VertexAttributeFormat::Float32x2,
        VertexAttributeFormat::Float32x3,
        VertexAttributeFormat::Float32x4,
        VertexAttributeFormat::Unorm8x4
    >;
}
