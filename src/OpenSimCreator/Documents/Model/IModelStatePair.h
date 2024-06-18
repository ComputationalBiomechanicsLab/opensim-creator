#pragma once

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>

namespace OpenSim { class Component; }

namespace osc
{
    // virtual read+write accessor to an `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class IModelStatePair : public IConstModelStatePair {
    protected:
        IModelStatePair() = default;
        IModelStatePair(const IModelStatePair&) = default;
        IModelStatePair(IModelStatePair&&) noexcept = default;
        IModelStatePair& operator=(const IModelStatePair&) = default;
        IModelStatePair& operator=(IModelStatePair&&) noexcept = default;
    public:
        virtual ~IModelStatePair() noexcept = default;

        void setSelected(const OpenSim::Component* newSelection)
        {
            implSetSelected(newSelection);
        }

        void setHovered(const OpenSim::Component* newHover)
        {
            implSetHovered(newHover);
        }

        void setFixupScaleFactor(float newScaleFactor)
        {
            implSetFixupScaleFactor(newScaleFactor);
        }
    private:
        virtual void implSetSelected(const OpenSim::Component*) {}
        virtual void implSetHovered(const OpenSim::Component*) {}
        virtual void implSetFixupScaleFactor(float) {}
    };
}
