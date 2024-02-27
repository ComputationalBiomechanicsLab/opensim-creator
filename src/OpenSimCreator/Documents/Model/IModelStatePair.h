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
        IModelStatePair(IModelStatePair const&) = default;
        IModelStatePair(IModelStatePair&&) noexcept = default;
        IModelStatePair& operator=(IModelStatePair const&) = default;
        IModelStatePair& operator=(IModelStatePair&&) noexcept = default;

        friend bool operator==(IModelStatePair const&, IModelStatePair const&) = default;
    public:
        virtual ~IModelStatePair() noexcept = default;

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
