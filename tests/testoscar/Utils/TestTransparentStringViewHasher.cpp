#include <oscar/Utils/TransparentStringViewHasher.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringName.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    using TransparentMap = std::unordered_map<std::string, int, TransparentStringViewHasher, std::equal_to<>>;
}

TEST(TransparentStringViewHasher, can_construct_std_unordered_map_that_uses_transparent_string_hasher)
{
    [[maybe_unused]] TransparentMap map;  // this should work
}

TEST(TransparentStringViewHasher, transparent_unordered_map_enables_std_string_view_lookups)
{
    TransparentMap map;
    [[maybe_unused]] const auto it = map.find(std::string_view{"i don't need to be converted into a std::string :)"});
}

TEST(TransparentStringViewHasher, transparent_unordered_map_enables_CStringView_lookups)
{
    TransparentMap map;
    [[maybe_unused]] const auto it = map.find(CStringView{"i don't need to be converted into a std::string :)"});
}

TEST(TransparentStringViewHasher, transparent_unordered_map_enables_StringName_lookups)
{
    TransparentMap map;
    [[maybe_unused]] const auto it = map.find(StringName{"i don't need to be converted into a std::string :)"});
}

TEST(TransparentStringViewHasher, produces_same_hash_for_all_of_OSCs_string_types)
{
    for (const char* str : {"", "some string", "why not try three?"}) {
        const auto hashes = std::to_array({
            TransparentStringViewHasher{}(str),
            TransparentStringViewHasher{}(std::string_view{str}),
            TransparentStringViewHasher{}(CStringView{str}),
            TransparentStringViewHasher{}(std::string{str}),
            TransparentStringViewHasher{}(StringName{str}),
        });
        ASSERT_TRUE(rgs::equal_range(hashes, hashes.front()));
    }
}
