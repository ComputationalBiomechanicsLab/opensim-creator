#pragma once

#include <liboscar/utils/flags.h>

#include <cstdint>

namespace osc
{
    // Represents a single keyboard key modifier.
    //
    // `KeyModifier`s are virtual, which means that don't necessarily
    // represent what's physically written on the keyboard. For
    // example, MacOS tends to use the Command key in place of the
    // Ctrl key for many application keybinds. With this in mind, the
    // backend maps (virtualizes) the Command key to `KeyModifier::Ctrl`
    // so that application code can just write keybinds "as if" only
    // writing for Windows/Linux. See `PhysicalKeyModifier` (and related
    // `Converter`s) if you need to know the underlying physical key.
    enum class KeyModifier : uint16_t {
        // No modifier key is pressed.
        None         = 0,

        // A shift key on the keyboard is pressed.
        Shift        = 1<<0,

        // If on MacOS, a command key on the keyboard is pressed.
        // Otherwise,   a ctrl key on the keyboard is pressed.
        //
        // The difference between MacOS and the others is to normalize
        // how the modifier key is actually used on those OSes. E.g.
        // `Ctrl+V` on Windows usually has the same intent as `Command+V`
        // on MacOS. With this in mind, you should write your keybinds
        // as-if designing for Windows.
        Ctrl         = 1<<1,

        // If on MacOS, a ctrl key on the keyboard is pressed.
        // Otherwise,   a meta (e.g. Windows) key on the keyboard is pressed.
        //
        // The difference between MacOS and the others is to normalize
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

    constexpr KeyModifiers operator|(KeyModifier lhs, KeyModifier rhs)
    {
        return KeyModifiers{lhs, rhs};
    }
}
