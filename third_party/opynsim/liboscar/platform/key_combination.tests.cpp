#include "key_combination.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(KeyCombination, default_constructs_to_an_unknown_key)
{
    static_assert(KeyCombination{}.key() == Key::Unknown);
}

TEST(KeyCombination, modifiers_make_key_compare_not_equal)
{
    static_assert(KeyCombination{ModifierKey::Ctrl, Key::Z} != KeyCombination{Key::Z});
}

TEST(KeyCombination, operator_or_can_construct_key_combinations)
{
    static_assert(Key::X == KeyCombination{Key::X});
    static_assert((ModifierKey::Ctrl | Key::Z) == KeyCombination{ModifierKey::Ctrl, Key::Z});
    static_assert(
        (ModifierKey::Ctrl | ModifierKey::Alt | Key::Delete) ==
        KeyCombination{ModifierKeys{ModifierKey::Ctrl, ModifierKey::Alt}, Key::Delete});
}
