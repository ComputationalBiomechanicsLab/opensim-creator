#pragma once

#include <liboscar/platform/events/event.h>
#include <OpenSim/Common/ComponentPath.h>

namespace OpenSim
{
    class Coordinate;
    class Muscle;
}

namespace osc
{
    class AddMusclePlotEvent : public Event {
    public:
        explicit AddMusclePlotEvent(const OpenSim::Coordinate&, const OpenSim::Muscle&);

        const OpenSim::ComponentPath& get_coordinate_abs_path() const
        {
            return coordinate_abs_path_;
        }
        const OpenSim::ComponentPath& get_muscle_abs_path() const { return muscle_abs_path_; }

    private:
        OpenSim::ComponentPath coordinate_abs_path_;
        OpenSim::ComponentPath muscle_abs_path_;
    };
}
