#include "modifier_key.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(ModifierKey, operator_or_returns_expected_modifiers)
{
    static_assert(
        (ModifierKey::Ctrl | ModifierKey::Alt) ==
        ModifierKeys{ModifierKey::Ctrl, ModifierKey::Alt});
}

TEST(ModifierKey, can_be_constructed_from_chain_of_ors)
{
    static_assert(
        (ModifierKey::Ctrl | ModifierKey::Alt | ModifierKey::Meta) ==
        ModifierKeys{ModifierKey::Ctrl, ModifierKey::Alt, ModifierKey::Meta});
}
