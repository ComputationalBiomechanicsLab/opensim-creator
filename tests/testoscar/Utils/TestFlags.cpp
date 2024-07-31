#include <oscar/Utils/Flags.h>

#include <gtest/gtest.h>

#include <array>

using namespace osc;

namespace
{
    enum class ExampleDenseFlags {
        None,
        Flag1 = 1<<0,
        Flag2 = 1<<1,
        Flag3 = 1<<2,
        NUM_FLAGS = 3,
    };
}

TEST(Flags, can_default_construct)
{
    Flags<ExampleDenseFlags> default_constructed;
    ASSERT_EQ(default_constructed, ExampleDenseFlags::None);
}

TEST(Flags, can_implicitly_convert_from_single_flag)
{
    ExampleDenseFlags flag = ExampleDenseFlags::Flag1;
    Flags<ExampleDenseFlags> flags = flag;

    ASSERT_TRUE(flags & ExampleDenseFlags::Flag1);
}

TEST(Flags, can_initialize_from_initializer_list_of_flags)
{
    Flags<ExampleDenseFlags> flags = {ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2};
    ASSERT_TRUE(flags & ExampleDenseFlags::Flag1);
    ASSERT_TRUE(flags & ExampleDenseFlags::Flag2);
    ASSERT_FALSE(flags & ExampleDenseFlags::Flag3);
}

TEST(Flags, operator_exclamation_mark_returns_false_if_any_flag_set)
{
    ASSERT_FALSE(!Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag1});
    ASSERT_FALSE(!Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag2});
    ASSERT_FALSE(!Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag3});

    ASSERT_TRUE(!Flags<ExampleDenseFlags>{ExampleDenseFlags::None});
}

TEST(Flags, operator_bool_returns_true_if_any_flag_set)
{
    ASSERT_TRUE(Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag1});
    ASSERT_TRUE(Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag2});
    ASSERT_TRUE(Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag3});

    ASSERT_FALSE(Flags<ExampleDenseFlags>{ExampleDenseFlags::None});
}

TEST(Flags, operator_amphresand_returns_AND_of_two_flags)
{
    using Flags = Flags<ExampleDenseFlags>;
    struct TestCase {
        Flags lhs;
        Flags rhs;
        Flags expected;
    };
    const auto testCases = std::to_array<TestCase>({
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::Flag1},
            .expected = Flags{ExampleDenseFlags::Flag1},
        },
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::Flag2},
            .expected = Flags{ExampleDenseFlags::Flag2},
        },
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::Flag3},
            .expected = Flags{ExampleDenseFlags::None},
        },
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::None},
            .expected = Flags{ExampleDenseFlags::None},
        },
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(testCase.lhs & testCase.rhs, testCase.expected);
    }
}

TEST(Flags, operator_or_equals_works_as_expected)
{
    using Flags = Flags<ExampleDenseFlags>;
    struct TestCase {
        Flags lhs;
        Flags rhs;
        Flags expected;
    };
    const auto testCases = std::to_array<TestCase>({
        {
            .lhs = Flags{ExampleDenseFlags::None},
            .rhs = Flags{ExampleDenseFlags::Flag1},
            .expected = Flags{ExampleDenseFlags::Flag1},
        },
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::Flag2},
            .expected = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
        },
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::Flag3},
            .expected = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2, ExampleDenseFlags::Flag3},
        },
        {
            .lhs = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
            .rhs = Flags{ExampleDenseFlags::None},
            .expected = Flags{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag2},
        },
    });

    for (const auto& testCase : testCases) {
        Flags lhs = testCase.lhs;
        lhs |= testCase.rhs;
        ASSERT_EQ(lhs, testCase.expected);
    }
}

TEST(Flags, lowest_set_returns_none_if_none_are_set)
{
    ASSERT_EQ(Flags<ExampleDenseFlags>{}.lowest_set(), ExampleDenseFlags::None);
    ASSERT_EQ(Flags<ExampleDenseFlags>{ExampleDenseFlags::None}.lowest_set(), ExampleDenseFlags::None);
}

TEST(Flags, lowest_set_returns_lowest_flag_for_non_None_values)
{
    ASSERT_EQ(Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag1}.lowest_set(), ExampleDenseFlags::Flag1);
    ASSERT_EQ(Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag2}.lowest_set(), ExampleDenseFlags::Flag2);
    ASSERT_EQ(Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag3}.lowest_set(), ExampleDenseFlags::Flag3);
    {
        const auto flags = Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag2, ExampleDenseFlags::Flag3};
        ASSERT_EQ(flags.lowest_set(), ExampleDenseFlags::Flag2);
    }
    {
        const auto flags = Flags<ExampleDenseFlags>{ExampleDenseFlags::Flag1, ExampleDenseFlags::Flag3};
        ASSERT_EQ(flags.lowest_set(), ExampleDenseFlags::Flag1);
    }
}