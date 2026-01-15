#pragma once

#include <liboscar/platform/key_modifier.h>
#include <liboscar/utils/conversion.h>
#include <liboscar/utils/flags.h>

#include <cstdint>

namespace osc
{
    // Represents a single keyboard key modifier, as physically represented
    // on the keyboard.
    //
    // This can be useful to know when (e.g.) showing a user shortcuts or
    // keybinds, but beware that `PhysicalKeyModifier`s have OS-dependent
    // intent (e.g. `Ctrl+Z` on Windows/Linux, but `Command+Z` on MacOS),
    // which is why `KeyModifier` is virtualized in the first place.
    enum class PhysicalKeyModifier : uint16_t {
        // No modifier key is pressed.
        None         = 0,

        // A shift key on the keyboard is pressed.
        Shift        = 1<<0,

        // A ctrl key on the keyboard is pressed.
        Ctrl         = 1<<1,

        // A meta (Windows/Command) key on the keyboard is pressed.
        Meta         = 1<<2,

        // An alt key on the keyboard is pressed.
        Alt          = 1<<3,

        NUM_FLAGS    =    4,
    };
    using PhysicalKeyModifiers = Flags<PhysicalKeyModifier>;

    // Converts `KeyModifiers` into `PhysicalKeyModifiers`.
    template<>
    struct Converter<KeyModifiers, PhysicalKeyModifiers> final {
        PhysicalKeyModifiers operator()(KeyModifiers) const;
    };

    // Converts `PhysicalKeyModifiers` into `KeyModifiers`.
    template<>
    struct Converter<PhysicalKeyModifiers, KeyModifiers> final {
        KeyModifiers operator()(PhysicalKeyModifiers) const;
    };
}
