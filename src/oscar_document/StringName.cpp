#include "StringName.hpp"

#include <ankerl/unordered_dense.h>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using osc::detail::StringNameData;
using osc::StringName;

namespace
{
    // this is what's stored in the lookup table
    //
    // `StringNameData` _must_ be reference-stable w.r.t. downstream
    // code because users are reading/reference incrementing raw
    // pointers
    struct StringNameDataPtr final {
    public:
        StringNameDataPtr(std::string_view value_) :
            m_Ptr{std::make_unique<StringNameData>(value_)}
        {
        }

        friend bool operator==(StringNameDataPtr const& lhs, std::string_view rhs)
        {
            return lhs.m_Ptr->value() == rhs;
        }

        friend bool operator==(std::string_view lhs, StringNameDataPtr const& rhs)
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

        operator std::string_view() const
        {
            return m_Ptr->value();
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
    };

    using StringNameLookup = ankerl::unordered_dense::set<
        StringNameDataPtr,
        StringNameLutHasher,
        std::equal_to<>
    >;

    osc::SynchronizedValue<StringNameLookup>& GetGlobalStringNameLUT()
    {
        static osc::SynchronizedValue<StringNameLookup> s_Lut;
        return s_Lut;
    }

    template<typename StringLike>
    osc::detail::StringNameData& PossiblyConstructThenGetData(StringLike&& input)
    {
        auto [it, inserted] = GetGlobalStringNameLUT().lock()->emplace(std::forward<StringLike>(input));
        if (!inserted)
        {
            (*it)->incrementOwnerCount();
        }
        return **it;
    }

    void DecrementThenPossiblyDestroyData(osc::detail::StringNameData& data)
    {
        if (data.decrementOwnerCount())
        {
            GetGlobalStringNameLUT().lock()->erase(data.value());
        }
    }

    // edge-case for blank string (common)
    osc::StringName const& GetCachedBlankStringData()
    {
        static osc::StringName const s_BlankString{std::string_view{}};
        return s_BlankString;
    }
}

osc::StringName::StringName() : StringName{GetCachedBlankStringData()} {}
osc::StringName::StringName(std::string&& tmp) : m_Data{&PossiblyConstructThenGetData(std::move(tmp))} {}
osc::StringName::StringName(char const* c) : StringName{std::string_view{c}} {}
osc::StringName::StringName(std::string_view sv) : m_Data{&PossiblyConstructThenGetData(sv)} {}
osc::StringName::~StringName() noexcept { DecrementThenPossiblyDestroyData(*m_Data); }
