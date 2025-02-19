#pragma once

#include <liboscar/Utils/Flags.h>

#include <cstdint>

namespace osc
{
    enum class KeyModifier : uint16_t {
        // No modifier key is pressed.
        None         = 0,

        // A shift key on the keyboard is pressed.
        Shift        = 1<<0,

        // If on MacOS, a command key on the keyboard is pressed.
        // Otherwise,   a ctrl key on the keyboard is pressed.
        //
        // The difference betweeen MacOS and the others is to normalize
        // how a key is actually used between OSes. E.g. `Ctrl+V` on
        // Windows usually has the same intent as `Command+V` on MacOS.
        // With this in mind, you should write your keybinds as-if
        // designing for Windows.
        Ctrl         = 1<<1,

        // If on MacOS, a ctrl key on the keyboard is pressed.
        // Otherwise,   a meta (e.g. Windows) key on the keyboard is pressed.
        //
        // The difference betweeen MacOS and the others is to normalize
        // how a key is actually used between OSes. E.g. `Ctrl+V` on
        // Windows usually has the same intent as `Command+V` on MacOS.
        // With this in mind, you should write your keybinds as-if
        // designing for Windows.
        Meta         = 1<<2,

        // An alt key on the keyboard is pressed.
        Alt          = 1<<3,

        NUM_FLAGS    =    4,
    };
    using KeyModifiers = Flags<KeyModifier>;
}
