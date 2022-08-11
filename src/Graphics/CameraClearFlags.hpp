#pragma once

namespace osc
{
    enum class CameraClearFlags {
        SolidColor = 0,
        Depth,
        Nothing,
        TOTAL,
        Default = SolidColor,
    };
}