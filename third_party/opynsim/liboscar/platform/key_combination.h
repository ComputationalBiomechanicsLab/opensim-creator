#pragma once

#include <liboscar/platform/key.h>
#include <liboscar/platform/modifier_key.h>
#include <liboscar/utilities/hash_helpers.h>

#include <cstddef>
#include <functional>

namespace osc
{
    // Represents a combination of a single `Key` with `ModifierKeys`.
    class KeyCombination final {
    public:
        constexpr KeyCombination(Key key = Key::Unknown) :
            key_{key}
        {}

        constexpr KeyCombination(ModifierKeys modifiers, Key key) :
            modifiers_{modifiers},
            key_{key}
        {}

        friend bool operator==(const KeyCombination&, const KeyCombination&) = default;

        constexpr Key key() const { return key_; }
        constexpr ModifierKeys modifiers() const { return modifiers_; }

    private:
        ModifierKeys modifiers_{};
        Key key_ = Key::Unknown;
    };

    constexpr KeyCombination operator|(ModifierKeys modifiers, Key key)
    {
        return KeyCombination{modifiers, key};
    }
}

template<>
struct std::hash<osc::KeyCombination> final {
    size_t operator()(const osc::KeyCombination& kc) const noexcept
    {
        return osc::hash_of(kc.key(), kc.modifiers());
    }
};
