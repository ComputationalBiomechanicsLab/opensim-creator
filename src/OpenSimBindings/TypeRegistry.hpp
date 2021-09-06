#pragma once

#include <nonstd/span.hpp>

#include <memory>
#include <optional>
#include <cstddef>

namespace OpenSim {
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
        [[nodiscard]] static nonstd::span<std::unique_ptr<T const> const> prototypes() noexcept;
        [[nodiscard]] static nonstd::span<char const* const> names() noexcept;
        [[nodiscard]] static nonstd::span<char const* const> descriptions() noexcept;
        [[nodiscard]] static std::optional<size_t> indexOf(T const& v);
    };

    struct JointRegistry : TypeRegistry<OpenSim::Joint> {};
    struct ContactGeometryRegistry : TypeRegistry<OpenSim::ContactGeometry> {};
    struct ConstraintRegistry : TypeRegistry<OpenSim::Constraint> {};
    struct ForceRegistry : TypeRegistry<OpenSim::Force> {};
}
