#include "KeyCombination.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(KeyCombination, default_constructs_to_an_unknown_key)
{
    static_assert(KeyCombination{}.key() == Key::Unknown);
}

TEST(KeyCombination, modifiers_make_key_compare_not_equal)
{
    static_assert(KeyCombination{KeyModifier::Ctrl, Key::Z} != KeyCombination{Key::Z});
}

TEST(KeyCombination, operator_or_can_construct_key_combinations)
{
    static_assert(Key::X == KeyCombination{Key::X});
    static_assert((KeyModifier::Ctrl | Key::Z) == KeyCombination{KeyModifier::Ctrl, Key::Z});
    static_assert((KeyModifier::Ctrl | KeyModifier::Alt | Key::Delete) == KeyCombination{KeyModifiers{KeyModifier::Ctrl, KeyModifier::Alt}, Key::Delete});
}
