#include "DedupedString.hpp"

#include "src/Utils/CStringView.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>


// the deduped data (held as a reference-counted shared ptr by each owner)
class osc::DedupedString::Impl final {
public:
    explicit Impl(std::string_view sv) : Content{std::move(sv)}
    {
    }

    std::string Content;
    std::size_t Hash = std::hash<std::string>{}(Content);
    std::atomic<int> OwnerCount = 1;
};

using DedupedStringLut = std::unordered_map<std::string, std::unique_ptr<osc::DedupedString::Impl>>;

static osc::SynchronizedValue<DedupedStringLut>& GetGlobalDedupedStringLut()
{
    static osc::SynchronizedValue<DedupedStringLut> g_Lut;
    return g_Lut;
}

static osc::DedupedString::Impl* DoDedupedLookup(std::string_view sv)
{
    auto guard = GetGlobalDedupedStringLut().lock();

    // FIXME: once C++20 is released, you can use:
    // - https://stackoverflow.com/questions/34596768/stdunordered-mapfind-using-a-type-different-than-the-key-type
    // - https://en.cppreference.com/w/cpp/container/unordered_map/find
    auto [it, inserted] = guard->try_emplace(std::string{sv}, nullptr);

    if (!inserted)
    {
        // already existed in the table: return deduped pointer
        return it->second.get();
    }
    else
    {
        // wasn't in the table: allocate a new entry and return new pointer
        it->second = std::make_unique<osc::DedupedString::Impl>(sv);
        return it->second.get();
    }
}

osc::DedupedString::DedupedString(std::string_view sv) : m_Impl{DoDedupedLookup(sv)}
{
}

osc::DedupedString::DedupedString(DedupedString const& src) : m_Impl{src.m_Impl}
{
    m_Impl->OwnerCount++;
}

osc::DedupedString::DedupedString(DedupedString&& tmp) :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::DedupedString& osc::DedupedString::operator=(DedupedString const& src)
{
    if (m_Impl != src.m_Impl)
    {
        if (--m_Impl->OwnerCount == 0)
        {
            auto guard = GetGlobalDedupedStringLut().lock();
            guard->erase(m_Impl->Content);
        }

        m_Impl = src.m_Impl;
        m_Impl->OwnerCount++;
    }
    return *this;
}

osc::DedupedString& osc::DedupedString::operator=(DedupedString&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::DedupedString::~DedupedString() noexcept
{
    if (m_Impl && --m_Impl->OwnerCount == 0)
    {
        auto guard = GetGlobalDedupedStringLut().lock();
        guard->erase(m_Impl->Content);
    }
}

char const* osc::DedupedString::c_str() const noexcept
{
    return m_Impl->Content.c_str();
}

std::string const& osc::DedupedString::getString() const noexcept
{
    return m_Impl->Content;
}

bool osc::operator==(DedupedString const& a, DedupedString const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(DedupedString const& a, DedupedString const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(DedupedString const& a, DedupedString const& b)
{
    return a.m_Impl->Content == b.m_Impl->Content;
}

bool osc::operator<=(DedupedString const& a, DedupedString const& b)
{
    return a.m_Impl->Content <= b.m_Impl->Content;
}

bool osc::operator>(DedupedString const& a, DedupedString const& b)
{
    return a.m_Impl->Content > b.m_Impl->Content;
}

bool osc::operator>=(DedupedString const& a, DedupedString const& b)
{
    return a.m_Impl->Content >= b.m_Impl->Content;
}

std::ostream& osc::operator<<(std::ostream& o, DedupedString const& s)
{
    return o << s.m_Impl->Content;
}

std::string osc::to_string(DedupedString const& s)
{
    return s.m_Impl->Content;
}

std::size_t std::hash<osc::DedupedString>::operator()(osc::DedupedString const& s) const
{
    return s.m_Impl->Hash;
}
