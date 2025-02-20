#include "KeyModifier.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(KeyModifier, operator_or_returns_expected_KeyModifiers)
{
    static_assert((KeyModifier::Ctrl | KeyModifier::Alt) == KeyModifiers{KeyModifier::Ctrl, KeyModifier::Alt});
}

TEST(KeyModifiers, can_be_constructed_from_chain_of_ors)
{
    static_assert((KeyModifier::Ctrl | KeyModifier::Alt | KeyModifier::Meta) == KeyModifiers{KeyModifier::Ctrl, KeyModifier::Alt, KeyModifier::Meta});
}
