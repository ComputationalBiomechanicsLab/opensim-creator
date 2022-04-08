#pragma once

#include "src/Utils/CStringView.hpp"

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc
{
    // globally de-duplicated and reference-counted string implementation for rapid
    // comparison and hashing
    class DedupedString final {
    public:
        DedupedString(std::string_view);
        DedupedString(DedupedString const&);
        DedupedString(DedupedString&&);
        DedupedString& operator=(DedupedString const&);
        DedupedString& operator=(DedupedString&&) noexcept;
        ~DedupedString() noexcept;

        char const* c_str() const noexcept;
        operator CStringView () const noexcept { return getString(); }
        operator std::string_view () const noexcept { return getString(); }
        operator std::string const& () const noexcept { return getString(); }

        class Impl;
    private:
        std::string const& getString() const noexcept;

        Impl* m_Impl;

        friend struct std::hash<DedupedString>;
        friend bool operator==(DedupedString const&, DedupedString const&);
        friend bool operator!=(DedupedString const&, DedupedString const&);
        friend bool operator<(DedupedString const&, DedupedString const&);
        friend bool operator<=(DedupedString const&, DedupedString const&);
        friend bool operator>(DedupedString const&, DedupedString const&);
        friend bool operator>=(DedupedString const&, DedupedString const&);
        friend std::ostream& operator<<(std::ostream&, DedupedString const&);
        friend std::string to_string(DedupedString const&);
    };

    bool operator==(DedupedString const& a, DedupedString const& b);
    bool operator!=(DedupedString const&, DedupedString const&);
    bool operator<(DedupedString const&, DedupedString const&);
    bool operator<=(DedupedString const&, DedupedString const&);
    bool operator>(DedupedString const&, DedupedString const&);
    bool operator>=(DedupedString const&, DedupedString const&);
    std::ostream& operator<<(std::ostream&, DedupedString const&);
    std::string to_string(DedupedString const&);
}

namespace std
{
    template<>
    struct hash<osc::DedupedString> {
        std::size_t operator()(osc::DedupedString const&) const;
    };
}
