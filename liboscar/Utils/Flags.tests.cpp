#include "Flags.h"

#include <liboscar/Shims/Cpp23/utility.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <functional>

using namespace osc;

namespace
{
    enum class ExampleDenseFlag : uint8_t {
        None      = 0,
        Flag1     = 1<<0,
        Flag2     = 1<<1,
        Flag3     = 1<<2,
        NUM_FLAGS = 3,
    };
}

TEST(Flags, from_underlying_works)
{
    const auto flags = Flags<ExampleDenseFlag>::from_underlying(cpp23::to_underlying(ExampleDenseFlag::Flag3));
    ASSERT_EQ(flags.underlying_value(), cpp23::to_underlying(ExampleDenseFlag::Flag3));
}

TEST(Flags, can_default_construct)
{
    const Flags<ExampleDenseFlag> default_constructed;
    ASSERT_EQ(default_constructed, ExampleDenseFlag::None);
}

TEST(Flags, can_implicitly_convert_from_single_flag)
{
    const ExampleDenseFlag flag = ExampleDenseFlag::Flag1;
    const Flags<ExampleDenseFlag> flags = flag;

    ASSERT_TRUE(flags & ExampleDenseFlag::Flag1);
}

TEST(Flags, can_initialize_from_initializer_list_of_flags)
{
    const Flags<ExampleDenseFlag> flags = {ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2};
    ASSERT_TRUE(flags & ExampleDenseFlag::Flag1);
    ASSERT_TRUE(flags & ExampleDenseFlag::Flag2);
    ASSERT_FALSE(flags & ExampleDenseFlag::Flag3);
}

TEST(Flags, operator_exclamation_mark_returns_false_if_any_flag_set)
{
    ASSERT_FALSE(!Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag1});
    ASSERT_FALSE(!Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag2});
    ASSERT_FALSE(!Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag3});

    ASSERT_TRUE(!Flags<ExampleDenseFlag>{ExampleDenseFlag::None});
}

TEST(Flags, operator_bool_returns_true_if_any_flag_set)
{
    ASSERT_TRUE(Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag1});
    ASSERT_TRUE(Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag2});
    ASSERT_TRUE(Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag3});

    ASSERT_FALSE(Flags<ExampleDenseFlag>{ExampleDenseFlag::None});
}

TEST(Flags, operator_amphresand_returns_AND_of_two_flags)
{
    using Flags = Flags<ExampleDenseFlag>;
    struct TestCase {
        Flags lhs;
        Flags rhs;
        Flags expected;
    };
    constexpr auto test_cases = std::to_array<TestCase>({
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag1},
            .expected = Flags{ExampleDenseFlag::Flag1},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag2},
            .expected = Flags{ExampleDenseFlag::Flag2},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag3},
            .expected = Flags{ExampleDenseFlag::None},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::None},
            .expected = Flags{ExampleDenseFlag::None},
        },
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(test_case.lhs & test_case.rhs, test_case.expected);
    }
}

TEST(Flags, operator_or_works_as_expected)
{
    using Flags = Flags<ExampleDenseFlag>;
    struct TestCase {
        Flags lhs;
        Flags rhs;
        Flags expected;
    };
    constexpr auto test_cases = std::to_array<TestCase>({
        {
            .lhs = Flags{ExampleDenseFlag::None},
            .rhs = Flags{ExampleDenseFlag::Flag1},
            .expected = Flags{ExampleDenseFlag::Flag1},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag2},
            .expected = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag3},
            .expected = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2, ExampleDenseFlag::Flag3},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::None},
            .expected = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
        },
    });

    for (const auto& test_case : test_cases) {
        const Flags lhs = test_case.lhs;
        const Flags result = lhs | test_case.rhs;
        ASSERT_EQ(result, test_case.expected);
    }
}

TEST(Flags, operator_or_equals_works_as_expected)
{
    using Flags = Flags<ExampleDenseFlag>;
    struct TestCase {
        Flags lhs;
        Flags rhs;
        Flags expected;
    };
    constexpr auto test_cases = std::to_array<TestCase>({
        {
            .lhs = Flags{ExampleDenseFlag::None},
            .rhs = Flags{ExampleDenseFlag::Flag1},
            .expected = Flags{ExampleDenseFlag::Flag1},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag2},
            .expected = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::Flag3},
            .expected = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2, ExampleDenseFlag::Flag3},
        },
        {
            .lhs = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .rhs = Flags{ExampleDenseFlag::None},
            .expected = Flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
        },
    });

    for (const auto& test_case : test_cases) {
        Flags lhs = test_case.lhs;
        lhs |= test_case.rhs;
        ASSERT_EQ(lhs, test_case.expected);
    }
}

TEST(Flags, lowest_set_returns_none_if_none_are_set)
{
    ASSERT_EQ(Flags<ExampleDenseFlag>{}.lowest_set(), ExampleDenseFlag::None);
    ASSERT_EQ(Flags<ExampleDenseFlag>{ExampleDenseFlag::None}.lowest_set(), ExampleDenseFlag::None);
}

TEST(Flags, lowest_set_returns_lowest_flag_for_non_None_values)
{
    ASSERT_EQ(Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag1}.lowest_set(), ExampleDenseFlag::Flag1);
    ASSERT_EQ(Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag2}.lowest_set(), ExampleDenseFlag::Flag2);
    ASSERT_EQ(Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag3}.lowest_set(), ExampleDenseFlag::Flag3);
    {
        constexpr auto flags = Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag2, ExampleDenseFlag::Flag3};
        ASSERT_EQ(flags.lowest_set(), ExampleDenseFlag::Flag2);
    }
    {
        constexpr auto flags = Flags<ExampleDenseFlag>{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
        ASSERT_EQ(flags.lowest_set(), ExampleDenseFlag::Flag1);
    }
}

TEST(Flags, with_returns_new_enum_with_original_flags_plus_provided_flags_set)
{
    const Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1};
    const Flags<ExampleDenseFlag> flags_after = flags.with(ExampleDenseFlag::Flag2);
    const Flags<ExampleDenseFlag> expected{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2};

    ASSERT_EQ(flags_after, expected);
}

TEST(Flags, with_doesnt_unset_already_set_flag)
{
    const Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1};
    const Flags<ExampleDenseFlag> flags_after = flags.with(ExampleDenseFlag::Flag1);

    ASSERT_EQ(flags, flags_after);
}

TEST(Flags, without_returns_new_enum_with_original_flags_minus_provided_flags_set)
{
    const Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    const Flags<ExampleDenseFlag> flags_after = flags.without(ExampleDenseFlag::Flag3);
    const Flags<ExampleDenseFlag> expected{ExampleDenseFlag::Flag1};

    ASSERT_EQ(flags_after, expected);
}

TEST(Flags, without_doesnt_set_already_unset_flag)
{
    const Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    const Flags<ExampleDenseFlag> flags_after = flags.without(ExampleDenseFlag::Flag2);
    ASSERT_EQ(flags, flags_after);
}

TEST(Flags, get_returns_true_if_given_flag_is_set)
{
    const Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    ASSERT_TRUE(flags.get(ExampleDenseFlag::Flag1));
    ASSERT_FALSE(flags.get(ExampleDenseFlag::Flag2));
    ASSERT_TRUE(flags.get(ExampleDenseFlag::Flag3));
}

TEST(Flags, set_true_sets_the_given_flag)
{
    Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    flags.set(ExampleDenseFlag::Flag2, true);
    const Flags<ExampleDenseFlag> expected{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2, ExampleDenseFlag::Flag3};
    ASSERT_TRUE(flags == expected);
}

TEST(Flags, set_false_unsets_the_given_flag)
{
    Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    flags.set(ExampleDenseFlag::Flag1, false);
    const Flags<ExampleDenseFlag> expected{ExampleDenseFlag::Flag3};
    ASSERT_EQ(flags, expected);
}

TEST(Flags, set_true_on_already_set_flag_does_nothing)
{
    Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    flags.set(ExampleDenseFlag::Flag1, true);
    const Flags<ExampleDenseFlag> expected{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    ASSERT_EQ(flags, expected);
}

TEST(Flags, set_false_on_not_already_set_flag_does_nothing)
{
    Flags<ExampleDenseFlag> flags{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    flags.set(ExampleDenseFlag::Flag2, false);
    const Flags<ExampleDenseFlag> expected{ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag3};
    ASSERT_EQ(flags, expected);
}

TEST(Flags, with_flag_values_swapped_works_as_intended)
{
    struct TestCase final {
        Flags<ExampleDenseFlag> flags;
        ExampleDenseFlag flag0;
        ExampleDenseFlag flag1;
        Flags<ExampleDenseFlag> expected;
    };
    const auto test_cases = std::to_array<TestCase>({
        {
            .flags = {ExampleDenseFlag::Flag2},
            .flag0 = ExampleDenseFlag::Flag1,
            .flag1 = ExampleDenseFlag::Flag2,
            .expected = {ExampleDenseFlag::Flag1},
        },
        {
            .flags = {},
            .flag0 = ExampleDenseFlag::Flag3,
            .flag1 = ExampleDenseFlag::Flag2,
            .expected = {},
        },
        {
            .flags = {ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2},
            .flag0 = ExampleDenseFlag::Flag1,
            .flag1 = ExampleDenseFlag::Flag3,
            .expected = {ExampleDenseFlag::Flag2, ExampleDenseFlag::Flag3},
        },
    });
    for (const auto& test_case : test_cases) {
        const auto input = test_case.flags;
        const auto output = input.with_flag_values_swapped(test_case.flag0, test_case.flag1);
        ASSERT_EQ(output, test_case.expected);
    }
}

TEST(Flags, has_a_to_underlying_specialization)
{
    enum class Some16BitEnum : uint16_t {};
    static_assert(std::is_same_v<decltype(to_underlying(Flags<Some16BitEnum>{})), uint16_t>);

    enum class SomeSigned32BitEnum : int32_t {};
    static_assert(std::is_same_v<decltype(to_underlying(Flags<SomeSigned32BitEnum>{})), int32_t>);
}

TEST(Flags, is_hashable)
{
    using ExampleDenseFlags = Flags<ExampleDenseFlag>;
    const ExampleDenseFlags flags1 = {ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2};
    const ExampleDenseFlags flags2 = {ExampleDenseFlag::Flag1, ExampleDenseFlag::Flag2, ExampleDenseFlag::Flag3};

    const std::hash<ExampleDenseFlags> hasher;
    ASSERT_NE(hasher(flags1), hasher(flags2));
}
