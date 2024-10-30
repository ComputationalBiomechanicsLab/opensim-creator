#include <oscar/Utils/Typelist.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

using namespace osc;

TEST(Typelist, can_be_empty)
{
    [[maybe_unused]] const Typelist should_compile;
}

TEST(Typelist, head_returns_first_element)
{
    static_assert(std::is_same_v<Typelist<int>::head, int>);
}

TEST(Typelist, tails_returns_last_element)
{
    static_assert(std::is_same_v<Typelist<int, float>::tails::head, float>);
}

TEST(TypelistSizeV, returns_expected_values)
{
    static_assert(TypelistSizeV<Typelist<>> == 0);
    static_assert(TypelistSizeV<Typelist<int>> == 1);
    static_assert(TypelistSizeV<Typelist<int, float>> == 2);
    static_assert(TypelistSizeV<Typelist<int, float, char>> == 3);
    static_assert(TypelistSizeV<Typelist<int, float, char, int64_t>> == 4);
    // ... etc.
}

TEST(TypeAtT, works_as_expected)
{
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 0>, int>);
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 1>, float>);
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 2>, char>);
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 3>, int64_t>);
}
