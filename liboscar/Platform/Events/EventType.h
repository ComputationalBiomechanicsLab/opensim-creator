#pragma once

namespace osc
{
    // Represents an enumeration of known-at-compile-time event types
    // that can be cheaply queried (compared to downcasting an `Event`).
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
