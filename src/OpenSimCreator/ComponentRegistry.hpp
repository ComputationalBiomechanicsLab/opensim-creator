#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <optional>
#include <memory>

namespace OpenSim { class Joint; }
namespace OpenSim { class Constraint; }
namespace OpenSim { class Component; }
namespace OpenSim { class ContactGeometry; }
namespace OpenSim { class Force; }
namespace OpenSim { class Controller; }
namespace OpenSim { class Probe; }

namespace osc
{
    // static registry of types. The registry is guaranteed to:
    //
    // - return entries in constant time
    // - return entries contiguously in memory
    // - return entires in a format that's useful for downstream (e.g. contiguous strings for ImGui)
    template<typename T>
    struct ComponentRegistry {
        static CStringView name() noexcept;
        static CStringView description() noexcept;
        static nonstd::span<std::shared_ptr<T const> const> prototypes() noexcept;
        static nonstd::span<CStringView const> nameStrings() noexcept;
        static nonstd::span<CStringView const> descriptionStrings() noexcept;
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

    struct JointRegistry : ComponentRegistry<OpenSim::Joint> {};
    struct ContactGeometryRegistry : ComponentRegistry<OpenSim::ContactGeometry> {};
    struct ConstraintRegistry : ComponentRegistry<OpenSim::Constraint> {};
    struct ForceRegistry : ComponentRegistry<OpenSim::Force> {};
    struct ControllerRegistry : ComponentRegistry<OpenSim::Controller> {};
    struct ProbeRegistry : ComponentRegistry<OpenSim::Probe> {};
    struct UngroupedRegistry : ComponentRegistry<OpenSim::Component> {};
}
