#pragma once

namespace osc
{
    enum class EventType {
        DropFile,
        KeyDown,
        KeyUp,
        TextInput,
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseWheel,
        DisplayStateChange,
        Quit,
        Window,
        Custom,
        NUM_OPTIONS,
    };
}
