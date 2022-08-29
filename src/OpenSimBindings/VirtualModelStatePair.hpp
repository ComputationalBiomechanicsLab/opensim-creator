#pragma once

#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Utils/UID.hpp"

#include <optional>

namespace OpenSim { class Component; }
namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace osc
{
    // virtual read+write accessor to an `OpenSim::Model` + `SimTK::State` pair, with
    // additional opt-in overrides to aid rendering/UX etc.
    class VirtualModelStatePair : public VirtualConstModelStatePair {
    public:
        virtual ~VirtualModelStatePair() noexcept = default;

        virtual void setSelected(OpenSim::Component const*) {}

        virtual void setHovered(OpenSim::Component const*) {}

        virtual void setShowingOnly(OpenSim::Component const*) {}

        virtual void setFixupScaleFactor(float) {}
    };
}