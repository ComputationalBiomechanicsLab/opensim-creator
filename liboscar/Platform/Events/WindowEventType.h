#pragma once

namespace osc
{
    enum class WindowEventType {
        GainedMouseFocus,
        LostMouseFocus,
        GainedKeyboardFocus,
        LostKeyboardFocus,
        WindowClosed,
        WindowMoved,
        WindowResized,
        WindowDisplayScaleChanged,
        Unknown,
        NUM_OPTIONS,
    };
}
