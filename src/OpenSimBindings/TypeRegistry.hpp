#pragma once

#include "src/Utils/CStringView.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <optional>
#include <memory>
#include <string>

namespace OpenSim
{
    class Joint;
    class Constraint;
    class ContactGeometry;
    class Force;
}

namespace osc
{
    // static registry of types. The registry is guaranteed to:
    //
    // - return entries in constant time
    // - return entries contiguously in memory
    // - return entires in a format that's useful for downstream (e.g. contiguous strings for ImGui)
    template<typename T>
    struct TypeRegistry {
        static nonstd::span<std::shared_ptr<T const> const> prototypes() noexcept;
        static nonstd::span<CStringView const> nameStrings() noexcept;
        static nonstd::span<char const* const> nameCStrings() noexcept;
        static nonstd::span<CStringView const> descriptionStrings() noexcept;
        static nonstd::span<char const* const> descriptionCStrings() noexcept;
        static std::optional<size_t> indexOf(T const& v) noexcept;

        template<typename U>
        static std::optional<size_t> indexOf() noexcept
        {
            auto protos = prototypes();
            for (size_t i = 0; i < protos.size(); ++i)
            {
                if (typeid(*protos[i]) == typeid(U))
                {
                    return std::optional<size_t>{i};
                }
            }
            return std::nullopt;
        }
    };

    struct JointRegistry : TypeRegistry<OpenSim::Joint> {};
    struct ContactGeometryRegistry : TypeRegistry<OpenSim::ContactGeometry> {};
    struct ConstraintRegistry : TypeRegistry<OpenSim::Constraint> {};
    struct ForceRegistry : TypeRegistry<OpenSim::Force> {};
    // TODO: controllers
    // TODO: probes
}
