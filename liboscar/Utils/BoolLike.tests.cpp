#include "BoolLike.h"

#include <liboscar/Shims/Cpp20/bit.h>

#include <gtest/gtest.h>

#include <concepts>

using namespace osc;

TEST(BoolLike, can_be_constructed_from_bool)
{
    static_assert(std::constructible_from<BoolLike, bool>);
}

TEST(BoolLike, implicitly_converts_to_bool)
{
    static_assert(std::convertible_to<BoolLike, bool>);
}

TEST(BoolLike, cast_to_bool_ptr_works)
{
    const BoolLike v{};
    ASSERT_EQ(cast_to_bool_ptr(&v), cpp20::bit_cast<const bool*>(&v));
}

TEST(BoolLike, cast_to_bool_ptr_non_const_works)
{
    BoolLike v{};
    ASSERT_EQ(cast_to_bool_ptr(&v), cpp20::bit_cast<bool*>(&v));
}
