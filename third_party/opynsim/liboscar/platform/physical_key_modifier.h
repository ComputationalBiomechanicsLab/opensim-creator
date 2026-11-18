#pragma once

#include <liboscar/platform/modifier_key.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/flags.h>

#include <cstdint>

namespace osc
{
    // Represents a single keyboard key modifier, as physically represented
    // on the keyboard.
    //
    // This can be useful to know when (e.g.) showing a user shortcuts or
    // keybinds, but beware that `PhysicalModifierKey`s have OS-dependent
    // intent (e.g. `Ctrl+Z` on Windows/Linux, but `Command+Z` on MacOS),
    // which is why `ModifierKey` is virtualized in the first place.
    enum class PhysicalModifierKey : uint16_t {
        // No modifier key is pressed.
        None = 0,

        // A shift key on the keyboard is pressed.
        Shift = 1 << 0,

        // A ctrl key on the keyboard is pressed.
        Ctrl = 1 << 1,

        // A meta (Windows/Command) key on the keyboard is pressed.
        Meta = 1 << 2,

        // An alt key on the keyboard is pressed.
        Alt = 1 << 3,

        NUM_FLAGS = 4,
    };
    using PhysicalModifierKeys = Flags<PhysicalModifierKey>;

    // Converts `ModifierKeys` into `PhysicalModifierKeys`.
    template<>
    struct Converter<ModifierKeys, PhysicalModifierKeys> final {
        PhysicalModifierKeys operator()(ModifierKeys) const;
    };

    // Converts `PhysicalModifierKeys` into `ModifierKeys`.
    template<>
    struct Converter<PhysicalModifierKeys, ModifierKeys> final {
        ModifierKeys operator()(PhysicalModifierKeys) const;
    };
}
