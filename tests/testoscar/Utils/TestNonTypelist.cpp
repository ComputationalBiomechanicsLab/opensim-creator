#include <oscar/Utils/NonTypeList.hpp>

#include <gtest/gtest.h>

using osc::NonTypelist;
using osc::NonTypelistSizeV;
using osc::NonTypeAt;

namespace
{
    enum class SomeEnum {
        First,
        Second,
        Third,
    };
}

TEST(NonTypelist, CanCreateEmptyNonTypeList)
{
    [[maybe_unused]] NonTypelist<SomeEnum> l;  // compiles
}

TEST(NonTypelist, CanHaveOneMember)
{
    [[maybe_unused]] NonTypelist<SomeEnum, SomeEnum::First> l;  // compiles
}

TEST(NonTypelist, CanHaveTwoMembers)
{
    [[maybe_unused]] NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second> l;
}

TEST(NonTypelist, HeadWorksAsExpected)
{
    static_assert(NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second>::head == SomeEnum::First);
}

TEST(NonTypelist, TailsWorksAsExpected)
{
    static_assert(NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second>::tails::head == SomeEnum::Second);
}

TEST(NonTypelist, NonTypelistSizeWorksAsExpected)
{
    static_assert(NonTypelistSizeV<NonTypelist<SomeEnum>> == 0);
    static_assert(NonTypelistSizeV<NonTypelist<SomeEnum, SomeEnum::First>> == 1);
    static_assert(NonTypelistSizeV<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second>> == 2);
}

TEST(NonTypelist, NonTypeAtVWorksAsExpected)
{
    static_assert(NonTypeAt<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second, SomeEnum::Third>, 0>::value == SomeEnum::First);
    static_assert(NonTypeAt<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second, SomeEnum::Third>, 1>::value == SomeEnum::Second);
    static_assert(NonTypeAt<NonTypelist<SomeEnum, SomeEnum::First, SomeEnum::Second, SomeEnum::Third>, 2>::value == SomeEnum::Third);
}
