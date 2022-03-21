#pragma once

namespace SimTK
{
    class State;
}

namespace OpenSim
{
    class Component;
}

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

        enum ResponseType {
            NothingHappened,
            SelectionChanged,
        };

        struct Response final {
            ResponseType type = NothingHappened;
            OpenSim::Component const* ptr = nullptr;
        };

        Response draw(SimTK::State const&, OpenSim::Component const*);
    };
}
