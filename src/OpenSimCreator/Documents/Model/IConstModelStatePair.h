#pragma once

#include <oscar/Utils/UID.h>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace osc
{
    // virtual readonly accessor to a `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class IConstModelStatePair {
    protected:
        IConstModelStatePair() = default;
        IConstModelStatePair(const IConstModelStatePair&) = default;
        IConstModelStatePair(IConstModelStatePair&&) noexcept = default;
        IConstModelStatePair& operator=(const IConstModelStatePair&) = default;
        IConstModelStatePair& operator=(IConstModelStatePair&&) noexcept = default;
    public:
        virtual ~IConstModelStatePair() noexcept = default;

        const OpenSim::Model& getModel() const
        {
            return implGetModel();
        }
        UID getModelVersion() const
        {
            return implGetModelVersion();
        }

        const SimTK::State& getState() const
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
        virtual const OpenSim::Model& implGetModel() const = 0;
        virtual UID implGetModelVersion() const
        {
            // assume the version always changes, unless the concrete implementation
            // provides a way of knowing when it doesn't
            return UID{};
        }

        virtual const SimTK::State& implGetState() const = 0;
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
