#pragma once

namespace osc
{
    enum class EventType {
        Raw,
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
        Custom,
        NUM_OPTIONS,
    };
}
