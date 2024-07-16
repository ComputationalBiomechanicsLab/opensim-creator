#include "StringName.h"

#include <ankerl/unordered_dense.h>

#include <oscar/Utils/SynchronizedValue.h>
#include <oscar/Utils/TransparentStringHasher.h>

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;
using osc::detail::StringNameData;

namespace
{
    // this is what's stored in the lookup table
    //
    // `StringNameData` _must_ be reference-stable w.r.t. downstream
    // code because users are reading/reference incrementing raw
    // pointers
    struct StringNameDataPtr final {
    public:
        explicit StringNameDataPtr(std::string_view str) :
            ptr_{std::make_unique<StringNameData>(str)}
        {}

        friend bool operator==(std::string_view lhs, const StringNameDataPtr& rhs)
        {
            return lhs == rhs.ptr_->value();
        }

        StringNameData* operator->() const
        {
            return ptr_.get();
        }

        StringNameData& operator*() const
        {
            return *ptr_;
        }
    private:
        std::unique_ptr<StringNameData> ptr_;
    };

    struct TransparentHasher : public TransparentStringHasher {
        using TransparentStringHasher::operator();

        size_t operator()(const StringNameDataPtr& ptr) const noexcept
        {
            return ptr->hash();
        }
    };

    using StringNameLookup = ankerl::unordered_dense::set<
        StringNameDataPtr,
        TransparentHasher,
        std::equal_to<>
    >;

    SynchronizedValue<StringNameLookup>& get_global_string_name_lut()
    {
        static SynchronizedValue<StringNameLookup> s_lut;
        return s_lut;
    }

    template<typename StringLike>
    requires
        std::constructible_from<std::string, StringLike&&> and
        std::convertible_to<StringLike&&, std::string_view>
    StringNameData& possibly_construct_then_get_data(StringLike&& input)
    {
        auto [it, inserted] = get_global_string_name_lut().lock()->emplace(std::forward<StringLike>(input));
        if (not inserted) {
            (*it)->increment_owner_count();
        }
        return **it;
    }

    void decrement_then_possibly_destruct_data(StringNameData& data)
    {
        if (data.decrement_owner_count()) {
            get_global_string_name_lut().lock()->erase(data.value());
        }
    }

    // edge-case for blank string (common)
    const StringName& get_cached_blank_string_data()
    {
        static const StringName s_blank_string{std::string_view{}};
        return s_blank_string;
    }
}

osc::StringName::StringName() : StringName{get_cached_blank_string_data()} {}
osc::StringName::StringName(std::string&& tmp) : data_{&possibly_construct_then_get_data(std::move(tmp))} {}
osc::StringName::StringName(std::string_view sv) : data_{&possibly_construct_then_get_data(sv)} {}
osc::StringName::~StringName() noexcept { decrement_then_possibly_destruct_data(*data_); }

std::ostream& osc::operator<<(std::ostream& out, const StringName& s)
{
    return out << std::string_view{s};
}
