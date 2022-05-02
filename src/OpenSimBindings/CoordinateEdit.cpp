#include "CoordinateEdit.hpp"

#include "src/Utils/Algorithms.hpp"

#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

bool osc::CoordinateEdit::applyToState(OpenSim::Coordinate const& c, SimTK::State& st) const
{
    bool applied = false;

    bool wasLocked = c.getLocked(st);

    // always unlock to apply user edits
    if (wasLocked)
    {
        c.setLocked(st, false);
    }

    if (!IsEffectivelyEqual(c.getValue(st), value))
    {
        c.setValue(st, value);  // care: may perform model assembly (expensive)
        applied = true;
    }

    if (!IsEffectivelyEqual(c.getSpeedValue(st), speed))
    {
        c.setSpeedValue(st, speed);
        applied = true;
    }

    // apply the final lock state (was unconditionally unlocked, above)
    if (locked)
    {
        c.setLocked(st, true);
    }

    if (wasLocked != locked)
    {
        applied = true;
    }

    return applied;
}
