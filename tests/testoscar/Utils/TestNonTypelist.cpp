#include <oscar/Utils/NonTypelist.h>

#include <gtest/gtest.h>

using namespace osc;

namespace
{
    enum class SomeEnum {
        First,
        Second,
        Third,
    };
}

TEST(NonTypelist, can_be_empty)
{
    [[maybe_unused]] const NonTypelist<SomeEnum> l;  // compiles
}

TEST(NonTypelist, can_have_one_member)
{
    [[maybe_unused]] const NonTypelist<SomeEnum, SomeEnum::First> l;  // compiles
}

TEST(NonTypelist, can_have_two_members)
{
    [[maybe_unused]] const NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second> l;
}

TEST(NonTypelist, head_retrieves_first_element)
{
    static_assert(NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second>::head == SomeEnum::First);
}

TEST(NonTypelist, tail_retrieves_last_element)
{
    static_assert(NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second>::tails::head == SomeEnum::Second);
}

TEST(NonTypelist, NonTypeListSizeV_returns_expected_results)
{
    static_assert(NonTypelistSizeV<NonTypelist<SomeEnum>> == 0);
    static_assert(NonTypelistSizeV<NonTypelist<SomeEnum, SomeEnum::First>> == 1);
    static_assert(NonTypelistSizeV<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second>> == 2);
}

TEST(NonTypelist, NonTypeAt_works_as_expected)
{
    static_assert(NonTypeAt<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second, SomeEnum::Third>, 0>::value == SomeEnum::First);
    static_assert(NonTypeAt<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second, SomeEnum::Third>, 1>::value == SomeEnum::Second);
    static_assert(NonTypeAt<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second, SomeEnum::Third>, 2>::value == SomeEnum::Third);
}
