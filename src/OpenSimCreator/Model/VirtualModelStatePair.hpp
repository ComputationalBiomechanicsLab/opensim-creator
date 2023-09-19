#pragma once

#include <OpenSimCreator/Model/VirtualConstModelStatePair.hpp>

namespace OpenSim { class Component; }

namespace osc
{
    // virtual read+write accessor to an `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class VirtualModelStatePair : public VirtualConstModelStatePair {
    protected:
        VirtualModelStatePair() = default;
        VirtualModelStatePair(VirtualModelStatePair const&) = default;
        VirtualModelStatePair(VirtualModelStatePair&&) noexcept = default;
        VirtualModelStatePair& operator=(VirtualModelStatePair const&) = default;
        VirtualModelStatePair& operator=(VirtualModelStatePair&&) noexcept = default;
    public:
        virtual ~VirtualModelStatePair() noexcept = default;

        void setSelected(OpenSim::Component const* newSelection)
        {
            implSetSelected(newSelection);
        }

        void setHovered(OpenSim::Component const* newHover)
        {
            implSetHovered(newHover);
        }

        void setFixupScaleFactor(float newScaleFactor)
        {
            implSetFixupScaleFactor(newScaleFactor);
        }
    private:
        virtual void implSetSelected(OpenSim::Component const*) {}
        virtual void implSetHovered(OpenSim::Component const*) {}
        virtual void implSetFixupScaleFactor(float) {}
    };
}
