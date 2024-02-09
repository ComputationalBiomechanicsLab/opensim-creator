#include <oscar/Utils/Typelist.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

using osc::TypeAtT;
using osc::Typelist;
using osc::TypelistSizeV;

TEST(Typelist, EmptyTypelistIsOk)
{
    [[maybe_unused]] Typelist t;  // ensure it compiles
}

TEST(Typelist, HeadReturnsFirstElement)
{
    static_assert(std::is_same_v<Typelist<int>::head, int>);
}

TEST(Typelist, TailsReturnsTailedTypelist)
{
    static_assert(std::is_same_v<Typelist<int, float>::tails::head, float>);
}

TEST(Typelist, TypelistSizeVReturnsExpectedValues)
{
    static_assert(TypelistSizeV<Typelist<>> == 0);
    static_assert(TypelistSizeV<Typelist<int>> == 1);
    static_assert(TypelistSizeV<Typelist<int, float>> == 2);
    static_assert(TypelistSizeV<Typelist<int, float, char>> == 3);
    static_assert(TypelistSizeV<Typelist<int, float, char, int64_t>> == 4);
    // ... etc.
}

TEST(Typelist, TypeAtWorksAsExpected)
{
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 0>, int>);
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 1>, float>);
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 2>, char>);
    static_assert(std::is_same_v<TypeAtT<Typelist<int, float, char, int64_t>, 3>, int64_t>);
}
