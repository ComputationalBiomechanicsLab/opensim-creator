#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <OpenSim/Common/ComponentPath.h>

namespace OpenSim { class Coordinate; }
namespace OpenSim { class Muscle; }

namespace osc
{
    class AddMusclePlotEvent : public Event {
    public:
        explicit AddMusclePlotEvent(const OpenSim::Coordinate&, const OpenSim::Muscle&);

        const OpenSim::ComponentPath& getCoordinateAbsPath() const { return m_CoordinateAbsPath; }
        const OpenSim::ComponentPath& getMuscleAbsPath() const { return m_MuscleAbsPath; }
    private:
        OpenSim::ComponentPath m_CoordinateAbsPath;
        OpenSim::ComponentPath m_MuscleAbsPath;
    };
}
