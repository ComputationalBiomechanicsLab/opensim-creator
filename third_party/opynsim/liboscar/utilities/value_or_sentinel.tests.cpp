#include "value_or_sentinel.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <format>
#include <sstream>

using namespace osc;

namespace
{
    struct StructuralType {
        friend bool operator==(const StructuralType&, const StructuralType&) = default;

        int a;
        int b;
    };
}

TEST(ValueOrSentinel, can_construct_from_size_t)
{
    ValueOrSentinel<size_t, static_cast<size_t>(-1)> v{10zu};
    ASSERT_EQ(*v, 10zu);
}

TEST(ValueOrSentinel, default_constructed_implicitly_converts_to_false)
{
    ValueOrSentinel<float, -1337.0f> v;
    ASSERT_FALSE(v);
}

TEST(ValueOrSentinel, constructed_with_value_implicitly_converts_to_true)
{
    ValueOrSentinel<int, -5> v{100};
    ASSERT_TRUE(v);
}

TEST(ValueOrSentinel, constructed_with_sentinel_implicitly_converts_to_false)
{
    ValueOrSentinel<StructuralType, {1, 5}> v{{1, 5}};
    ASSERT_FALSE(v);
}

TEST(ValueOrSentinel, sentinel_getter_returns_sentinel)
{
    const StructuralType rv = ValueOrSentinel<StructuralType, {10, 11}>::sentinel();
    const StructuralType expected{10, 11};
    ASSERT_EQ(rv, expected);
}

TEST(ValueOrSentinel, compares_true_with_value_when_filled_with_value)
{
    const ValueOrSentinel<int, -7> a = 10;
    const int b = 10;
    ASSERT_EQ(a, b);
    ASSERT_EQ(b, a);
}

TEST(ValueOrSentinel, compares_true_with_sentinel_value)
{
    const ValueOrSentinel<int, -50> a;  // Sentinel-constructed
    ASSERT_EQ( a,  -50);
    ASSERT_EQ(-50,  a);
    ASSERT_NE( a,   7);
    ASSERT_NE( 7,   a);
}

TEST(ValueOrSentinel, value_throws_if_called_on_sentinel)
{
    // const& overload
    {
        const ValueOrSentinel<int, -5> empty;
        ASSERT_THROW({ empty.value(); }, std::bad_optional_access);
    }

    // & overload
    {
        ValueOrSentinel<int, -5> empty;
        ASSERT_THROW({ empty.value(); }, std::bad_optional_access);
    }

    // const&& overload
    {
        const ValueOrSentinel<int, -5> empty;
        ASSERT_THROW({ std::move(empty).value(); }, std::bad_optional_access);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    }

    // && overload
    {
        ValueOrSentinel<int, -5> empty;
        ASSERT_THROW({ std::move(empty).value(); }, std::bad_optional_access);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    }
}

TEST(ValueOrSentinel, operator_star_accesses_value)
{
    // const& overload
    {
        const ValueOrSentinel<int, -5> v{-3};
        ASSERT_EQ(*v, -3);
    }

    // & overload
    {
        ValueOrSentinel<int, -5> v{-3};
        *v = -4;
        ASSERT_EQ(*v, -4);
    }

    // const&& overload
    {
        const ValueOrSentinel<int, -5> v = -3;
        ASSERT_EQ(*std::move(v), -3);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    }

    // && overload
    {
        const ValueOrSentinel<int, -5> v = -4;
        ASSERT_EQ(*std::move(v), -4);  // NOLINT(hicpp-move-const-arg,performance-move-const-arg)
    }
}

TEST(ValueOrSentinel, can_format_when_it_contains_sentinel)
{
    const ValueOrSentinel<float, 1.314f> empty;
    ASSERT_EQ(std::format("{:.2f}", empty), "ValueOrSentinel(1.31)");
}

TEST(ValueOrSentinel, can_format_when_it_contains_a_value)
{
    const ValueOrSentinel<float, 1.314f> v{10000.5f};
    ASSERT_EQ(std::format("{:.1f}", v), "ValueOrSentinel(10000.5)");
}

TEST(ValueOrSentinel, can_stream_to_string)
{
    std::stringstream ss;
    ss << ValueOrSentinel<int, 25>{40};
    ASSERT_EQ(std::move(ss).str(), "ValueOrSentinel(40)");
}