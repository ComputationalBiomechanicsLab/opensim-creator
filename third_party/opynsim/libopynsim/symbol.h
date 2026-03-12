#pragma once

#include <liboscar/utilities/string_name.h>

#include <functional>
#include <string_view>

namespace opyn
{
    /// Represents an immutable, cheap-to-copy, cheap-to-compare, cheap-to-hash
    /// symbol in the global process symbol table.
    ///
    /// Used by OPynSim to accelerate associative lookups in larger data
    /// structures, such as `Model`.
    class Symbol final {
    public:
        Symbol() = default;
        explicit Symbol(std::string_view sv) : data_{sv} {}

        friend bool operator==(const Symbol&,        const Symbol&)        = default;
        friend bool operator==(std::string_view lhs, const Symbol& rhs)    { return lhs == rhs.data_; }
        friend bool operator==(const Symbol& lhs,    std::string_view rhs) { return lhs.data_ == rhs; }

        /// Converts `*this` into a `std::string`.
        explicit operator std::string () const { return std::string{data_}; }
    private:
        friend class std::hash<opyn::Symbol>;

        osc::StringName data_;
    };
}

template<>
struct std::hash<opyn::Symbol> final {
    size_t operator()(const opyn::Symbol& symbol) const noexcept { return hasher_(symbol.data_); }
private:
    std::hash<osc::StringName> hasher_;
};
