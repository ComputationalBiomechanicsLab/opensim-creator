#include "TransparentStringHasher.h"

#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/SharedPreHashedString.h>
#include <liboscar/Utils/StringName.h>

#include <ankerl/unordered_dense.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    // TODO: change to `std::unordered_map` after upgrading from Ubuntu20 (doesn't support C++20's transparent hashing)
    using TransparentMap = ankerl::unordered_dense::map<std::string, int, TransparentStringHasher, std::equal_to<>>;
}

TEST(TransparentStringHasher, can_construct_unordered_map_that_uses_transparent_string_hasher)
{
    [[maybe_unused]] const TransparentMap should_compile;
}

TEST(TransparentStringHasher, transparent_unordered_map_enables_std_string_view_lookups)
{
    const TransparentMap map;
    [[maybe_unused]] const auto it = map.find(std::string_view{"i don't need to be converted into a std::string :)"});
}

TEST(TransparentStringHasher, transparent_unordered_map_enables_CStringView_lookups)
{
    const TransparentMap map;
    [[maybe_unused]] const auto it = map.find(CStringView{"i don't need to be converted into a std::string :)"});
}

TEST(TransparentStringHasher, transparent_unordered_map_enables_StringName_lookups)
{
    const TransparentMap map;
    [[maybe_unused]] const auto it = map.find(StringName{"i don't need to be converted into a std::string :)"});
}

TEST(TransparentStringHasher, produces_same_hash_for_all_of_OSCs_string_types)
{
    for (const char* const str : {"", "some string", "why not try three?"}) {
        const auto hashes = std::to_array({
            TransparentStringHasher{}(str),
            TransparentStringHasher{}(std::string_view{str}),
            TransparentStringHasher{}(CStringView{str}),
            TransparentStringHasher{}(std::string{str}),
            TransparentStringHasher{}(StringName{str}),
            TransparentStringHasher{}(SharedPreHashedString{str}),
        });
        ASSERT_TRUE(rgs::adjacent_find(hashes, rgs::not_equal_to{}) == hashes.end());
    }
}
