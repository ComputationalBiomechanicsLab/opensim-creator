#pragma once

#include <nonstd/span.hpp>

#include <memory>
#include <optional>
#include <cstddef>

namespace OpenSim
{
    class Joint;
    class Constraint;
    class ContactGeometry;
    class Force;
}

namespace osc {
    // static registry of types. The registry is guaranteed to:
    //
    // - return entries in constant time
    // - return entries contiguously in memory
    // - return entires in a format that's useful for downstream (e.g. contiguous strings for ImGui)
    template<typename T>
    struct TypeRegistry {
        static nonstd::span<std::unique_ptr<T const> const> prototypes() noexcept;
        static nonstd::span<std::string const> nameStrings() noexcept;
        static nonstd::span<char const* const> nameCStrings() noexcept;
        static nonstd::span<std::string const* const> descriptionStrings() noexcept;
        static nonstd::span<char const* const> descriptionCStrings() noexcept;
        static std::optional<size_t> indexOf(T const& v) noexcept;
    };

    struct JointRegistry : TypeRegistry<OpenSim::Joint> {};
    struct ContactGeometryRegistry : TypeRegistry<OpenSim::ContactGeometry> {};
    struct ConstraintRegistry : TypeRegistry<OpenSim::Constraint> {};
    struct ForceRegistry : TypeRegistry<OpenSim::Force> {};
}
