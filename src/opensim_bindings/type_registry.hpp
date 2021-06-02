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
    struct Type_registry {
        [[nodiscard]] static nonstd::span<std::unique_ptr<T const> const> prototypes() noexcept;
        [[nodiscard]] static nonstd::span<char const* const> names() noexcept;
        [[nodiscard]] static nonstd::span<char const* const> descriptions() noexcept;
        [[nodiscard]] static std::optional<size_t> index_of(T const& v);
    };

    struct Joint_registry : Type_registry<OpenSim::Joint> {};
    struct Contact_geom_registry : Type_registry<OpenSim::ContactGeometry> {};
    struct Constraint_registry : Type_registry<OpenSim::Constraint> {};
    struct Force_registry : Type_registry<OpenSim::Force> {};
}
