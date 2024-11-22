#include "StringName.h"

#include <oscar/Utils/SynchronizedValue.h>
#include <oscar/Utils/TransparentStringHasher.h>

#include <ankerl/unordered_dense.h>

#include <functional>
#include <string_view>

using namespace osc;

namespace
{
    using FastStringLookup = ankerl::unordered_dense::set<SharedPreHashedString, TransparentStringHasher, std::equal_to<>>;

    SynchronizedValue<FastStringLookup>& get_global_lut()
    {
        static SynchronizedValue<FastStringLookup> s_string_name_global_lut;
        return s_string_name_global_lut;
    }
}

osc::StringName::StringName(std::string_view sv) :
    SharedPreHashedString{*get_global_lut().lock()->emplace(sv).first}
{}

osc::StringName::~StringName() noexcept
{
    if (empty()) {
        return;  // special case: the default constructor doesn't use the global lookup
    }

    if (use_count() > 2) {
        return;  // other `StringName`s with the same value are still using the data
    }

    // else: clear it from the global table
    get_global_lut().lock()->erase(static_cast<const SharedPreHashedString&>(*this));
}
