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

        const OpenSim::Component* getSelected() const
        {
            return implGetSelected();
        }

        template<typename T>
        const T* getSelectedAs() const
        {
            return dynamic_cast<const T*>(getSelected());
        }

        const OpenSim::Component* getHovered() const
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

        virtual const OpenSim::Component* implGetSelected() const
        {
            return nullptr;
        }

        virtual const OpenSim::Component* implGetHovered() const
        {
            return nullptr;
        }

        virtual float implGetFixupScaleFactor() const
        {
            return 1.0f;
        }
    };
}
