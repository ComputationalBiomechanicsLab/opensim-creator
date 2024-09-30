#pragma once

#include <OpenSim/Common/ComponentPath.h>
#include <oscar/Platform/Event.h>

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
