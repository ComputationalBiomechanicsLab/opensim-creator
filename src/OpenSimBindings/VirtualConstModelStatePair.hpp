#pragma once

#include "src/Utils/SynchronizedValue.hpp"
#include "src/Utils/UID.hpp"

#include <optional>

namespace OpenSim
{
    class Component;
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // virtual readonly accessor to a `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class VirtualConstModelStatePair {
    public:
        virtual ~VirtualConstModelStatePair() noexcept = default;

        virtual OpenSim::Model const& getModel() const = 0;

        virtual SimTK::State const& getState() const = 0;

        // opt-in virtual API (handy for rendering, UI stuff, etc. but not entirely
        // required for computational stuff)

        // used for UI caching
        virtual UID getModelVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        // used for UI caching
        virtual UID getStateVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        virtual OpenSim::Component const* getSelected() const
        {
            return nullptr;
        }

        virtual OpenSim::Component const* getHovered() const
        {
            return nullptr;
        }

        virtual OpenSim::Component const* getIsolated() const
        {
            return nullptr;
        }

        // used to scale weird models (e.g. fly leg) in the UI
        virtual float getFixupScaleFactor() const
        {
            return 1.0f;
        }

        // concrete helper methods that use the above virutal API

        template<typename T>
        T const* getSelectedAs() const
        {
            return dynamic_cast<T const*>(getSelected());
        }
    };
}