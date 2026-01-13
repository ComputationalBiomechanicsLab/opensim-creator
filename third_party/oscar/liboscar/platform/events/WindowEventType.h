#pragma once

namespace osc
{
    // Represents the type of window change associated with a `WindowEvent`.
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
