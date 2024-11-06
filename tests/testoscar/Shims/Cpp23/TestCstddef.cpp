#include <oscar/Shims/Cpp23/cstddef.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

using namespace osc::literals;

TEST(literals_z, returns_signed_version_of_size_t)
{
    // C++23 defines the `z` suffix as "the signed version of std::size_t"
    static_assert(std::is_same_v<decltype(10_z), decltype(std::make_signed_t<std::size_t>{10})>);
}

TEST(literals_z, returns_signed_size_t)
{
    static_assert(10_z == static_cast<std::make_signed_t<std::size_t>>(10));
    static_assert(-1_z == static_cast<std::make_signed_t<std::size_t>>(-1));
}

TEST(literals_uz, returns_size_t)
{
    // C++23 defines the `uz` suffix as "std::size_t"
    static_assert(std::is_same_v<decltype(10_uz), decltype(std::size_t{10})>);
}
