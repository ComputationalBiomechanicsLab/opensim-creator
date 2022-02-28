#pragma once

#include <memory>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    class ComponentHierarchy final {
    public:
        enum ResponseType {
            NothingHappened,
            SelectionChanged,
            HoverChanged,
        };

        struct Response final {
            OpenSim::Component const* ptr = nullptr;
            ResponseType type = NothingHappened;
        };

        ComponentHierarchy();
        ComponentHierarchy(ComponentHierarchy const&) = delete;
        ComponentHierarchy(ComponentHierarchy&&) noexcept;
        ComponentHierarchy& operator=(ComponentHierarchy const&) = delete;
        ComponentHierarchy& operator=(ComponentHierarchy&&) noexcept;
        ~ComponentHierarchy() noexcept;

        Response draw(
            OpenSim::Component const* root,
            OpenSim::Component const* selection,
            OpenSim::Component const* hover);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
