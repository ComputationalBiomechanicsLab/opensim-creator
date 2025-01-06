#pragma once

#include <cstddef>
#include <functional>
#include <typeinfo>

namespace osc
{
    // A wrapper for storing a reference a `std::type_info` such that it can be used
    // in associative lookups (e.g. `std::map`, `std::unordered_map`).
    //
    // This can be handy for creating arbitrary caches that use `typeid(T)`s as lookups.
    class TypeInfoReference final {
    public:
        TypeInfoReference(const std::type_info& type_info) :
            type_info_{&type_info}
        {}

        const std::type_info& get() const { return *type_info_; }

        friend bool operator==(const TypeInfoReference&, const TypeInfoReference&) = default;
    private:
        const std::type_info* type_info_;
    };
}

template<>
struct std::hash<osc::TypeInfoReference> final {
    size_t operator()(const osc::TypeInfoReference& ref) const noexcept
    {
        return ref.get().hash_code();
    }
};
