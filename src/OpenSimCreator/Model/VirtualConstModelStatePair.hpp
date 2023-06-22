#pragma once

#include <oscar/Utils/UID.hpp>

#include <optional>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace osc
{
    // virtual readonly accessor to a `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class VirtualConstModelStatePair {
    protected:
        VirtualConstModelStatePair() = default;
        VirtualConstModelStatePair(VirtualConstModelStatePair const&) = default;
        VirtualConstModelStatePair(VirtualConstModelStatePair&&) noexcept = default;
        VirtualConstModelStatePair& operator=(VirtualConstModelStatePair const&) = default;
        VirtualConstModelStatePair& operator=(VirtualConstModelStatePair&&) noexcept = default;
    public:
        virtual ~VirtualConstModelStatePair() noexcept = default;

        OpenSim::Model const& getModel() const
        {
            return implGetModel();
        }
        UID getModelVersion() const
        {
            return implGetModelVersion();
        }

        SimTK::State const& getState() const
        {
            return implGetState();
        }
        UID getStateVersion() const
        {
            return implGetStateVersion();
        }

        OpenSim::Component const* getSelected() const
        {
            return implGetSelected();
        }

        template<typename T>
        T const* getSelectedAs() const
        {
            return dynamic_cast<T const*>(getSelected());
        }

        OpenSim::Component const* getHovered() const
        {
            return implGetHovered();
        }

        // used to scale weird models (e.g. fly leg) in the UI
        float getFixupScaleFactor() const
        {
            return implGetFixupScaleFactor();
        }

    private:
        virtual OpenSim::Model const& implGetModel() const = 0;
        virtual UID implGetModelVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        virtual SimTK::State const& implGetState() const = 0;
        virtual UID implGetStateVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        virtual OpenSim::Component const* implGetSelected() const
        {
            return nullptr;
        }

        virtual OpenSim::Component const* implGetHovered() const
        {
            return nullptr;
        }

        virtual float implGetFixupScaleFactor() const
        {
            return 1.0f;
        }
    };
}