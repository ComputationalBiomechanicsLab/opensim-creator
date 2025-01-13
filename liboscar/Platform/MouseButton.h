#pragma once

namespace osc
{
    enum class MouseButton : unsigned {
        None    = 0,     // no button is associated with the `MouseEvent` (e.g. moving the mouse)
        Left    = 1<<0,
        Right   = 1<<1,
        Middle  = 1<<2,
        Back    = 1<<3,  // sometimes called X1 (SDL), ExtraButton1 (Qt)
        Forward = 1<<4,  // sometimes called X2 (SDL), ExtraButton2 (Qt)
    };
}
