#include "StringName.h"

#include <ankerl/unordered_dense.h>

#include <oscar/Utils/SynchronizedValue.h>

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
        explicit StringNameDataPtr(std::string_view value_) :
            m_Ptr{std::make_unique<StringNameData>(value_)}
        {
        }

        friend bool operator==(std::string_view lhs, const StringNameDataPtr& rhs)
        {
            return lhs == rhs.m_Ptr->value();
        }

        StringNameData* operator->() const
        {
            return m_Ptr.get();
        }

        StringNameData& operator*() const
        {
            return *m_Ptr;
        }
    private:
        std::unique_ptr<StringNameData> m_Ptr;
    };

    struct StringNameLutHasher final {
        using is_transparent = void;
        using is_avalanching = void;

        [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t
        {
            return ankerl::unordered_dense::hash<std::string_view>{}(str);
        }

        [[nodiscard]] auto operator()(const StringNameDataPtr& ptr) const noexcept -> uint64_t
        {
            return ankerl::unordered_dense::hash<std::string_view>{}(ptr->value());
        }
    };

    using StringNameLookup = ankerl::unordered_dense::set<
        StringNameDataPtr,
        StringNameLutHasher,
        std::equal_to<>
    >;

    SynchronizedValue<StringNameLookup>& GetGlobalStringNameLUT()
    {
        static SynchronizedValue<StringNameLookup> s_Lut;
        return s_Lut;
    }

    template<typename StringLike>
    requires
        std::constructible_from<std::string, StringLike&&> and
        std::convertible_to<StringLike&&, std::string_view>
    StringNameData& PossiblyConstructThenGetData(StringLike&& input)
    {
        auto [it, inserted] = GetGlobalStringNameLUT().lock()->emplace(std::forward<StringLike>(input));
        if (not inserted) {
            (*it)->incrementOwnerCount();
        }
        return **it;
    }

    void DecrementThenPossiblyDestroyData(StringNameData& data)
    {
        if (data.decrementOwnerCount())
        {
            GetGlobalStringNameLUT().lock()->erase(data.value());
        }
    }

    // edge-case for blank string (common)
    const StringName& GetCachedBlankStringData()
    {
        static const StringName s_BlankString{std::string_view{}};
        return s_BlankString;
    }
}

osc::StringName::StringName() : StringName{GetCachedBlankStringData()} {}
osc::StringName::StringName(std::string&& tmp) : m_Data{&PossiblyConstructThenGetData(std::move(tmp))} {}
osc::StringName::StringName(std::string_view sv) : m_Data{&PossiblyConstructThenGetData(sv)} {}
osc::StringName::~StringName() noexcept { DecrementThenPossiblyDestroyData(*m_Data); }

std::ostream& osc::operator<<(std::ostream& o, const StringName& s)
{
    return o << std::string_view{s};
}
