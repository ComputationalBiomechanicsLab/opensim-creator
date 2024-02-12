#pragma once

namespace OpenSim { class Component; }
namespace SimTK { class State; }

namespace osc
{
    class ComponentDetails final {
    public:
        ComponentDetails() = default;
        ComponentDetails(ComponentDetails const&) = delete;
        ComponentDetails(ComponentDetails&&) noexcept = default;
        ComponentDetails& operator=(ComponentDetails const&) = delete;
        ComponentDetails& operator=(ComponentDetails&&) noexcept = default;
        ~ComponentDetails() noexcept = default;

        enum class ResponseType {
            NothingHappened,
            SelectionChanged,
        };

        struct Response final {
            ResponseType type = ResponseType::NothingHappened;
            OpenSim::Component const* ptr = nullptr;
        };

        Response onDraw(SimTK::State const&, OpenSim::Component const*);
    };
}
