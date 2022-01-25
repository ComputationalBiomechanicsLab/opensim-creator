#include "StateModifications.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

void osc::StateModifications::pushCoordinateEdit(const OpenSim::Coordinate& c, const CoordinateEdit& ce) {
    m_CoordEdits[c.getAbsolutePathString()] = ce;
}

bool osc::StateModifications::removeCoordinateEdit(OpenSim::Coordinate const& c)
{
    return m_CoordEdits.erase(c.getAbsolutePathString()) > 0;
}

bool osc::CoordinateEdit::applyToState(OpenSim::Coordinate const& c, SimTK::State& st) const {
    bool applied = false;

    bool wasLocked = c.getLocked(st);

    // always unlock to apply user edits
    if (wasLocked) {
        c.setLocked(st, false);
    }

    if (c.getValue(st) != value) {
        c.setValue(st, value);
        applied = true;
    }

    if (c.getSpeedValue(st) != speed) {
        c.setSpeedValue(st, speed);
        applied = true;
    }

    // apply the final lock state (was unconditionally unlocked, above)
    c.setLocked(st, locked);
    if (wasLocked != locked) {
        applied = true;
    }

    return applied;
}

bool osc::StateModifications::applyToState(const OpenSim::Model& m, SimTK::State& st) const {
    bool rv = false;

    for (auto& p : m_CoordEdits) {
        if (!m.hasComponent(p.first)) {
            continue;  // TODO: evict it
        }

        OpenSim::Coordinate const& c = m.getComponent<OpenSim::Coordinate>(p.first);

        bool modifiedCoord = p.second.applyToState(c, st);

        if (!modifiedCoord) {
            // TODO: evict it
        }

        rv = rv || modifiedCoord;
    }

    return rv;
}
