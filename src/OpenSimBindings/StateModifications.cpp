#include "StateModifications.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Perf.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>


class osc::StateModifications::Impl final {
public:
    Impl() = default;

    void pushCoordinateEdit(OpenSim::Coordinate const& c, CoordinateEdit const& ce)
    {
        m_CoordEdits[c.getAbsolutePathString()] = ce;
    }

    bool removeCoordinateEdit(OpenSim::Coordinate const& c)
    {
        auto it = m_CoordEdits.find(c.getAbsolutePathString());
        if (it != m_CoordEdits.end())
        {
            it->second = CoordinateEdit{c.getDefaultValue(), c.getDefaultSpeedValue(), c.getDefaultLocked()};
            return true;
        }
        else
        {
            return false;
        }
    }

    bool applyToState(OpenSim::Model const& m, SimTK::State& st)
    {
        bool rv = false;

        for (auto& [path, edit] : m_CoordEdits)
        {
            OpenSim::Coordinate const* c = FindComponent<OpenSim::Coordinate>(m, path);

            if (!c)
            {
                continue;  // TODO: evict it
            }

            bool modifiedCoord = false;
            {
                OSC_PERF("coordinate modification");
                modifiedCoord = edit.applyToState(*c, st);
            }

            if (!modifiedCoord)
            {
                // TODO: evict it
            }

            rv = rv || modifiedCoord;
        }

        if (rv)
        {
            // readback the actual coordinate values from the coordinates, because model
            // assembly may have altered them
            for (auto& [path, edit] : m_CoordEdits)
            {
                OpenSim::Coordinate const* c = FindComponent<OpenSim::Coordinate>(m, path);

                if (c)
                {
                    edit.locked = c->getLocked(st);
                    edit.value = c->getValue(st);
                    edit.speed = c->getSpeedValue(st);
                }
            }
        }

        return rv;
    }

private:
    std::unordered_map<std::string, CoordinateEdit> m_CoordEdits;
};


// public API (PIMPL)

osc::StateModifications::StateModifications() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::StateModifications::StateModifications(StateModifications const& other) :
    m_Impl{std::make_unique<Impl>(*other.m_Impl)}
{
}

osc::StateModifications::StateModifications(StateModifications&& tmp) noexcept = default;

osc::StateModifications& osc::StateModifications::operator=(StateModifications const& other)
{
    *m_Impl = *other.m_Impl;
    return *this;
}

osc::StateModifications& osc::StateModifications::operator=(StateModifications&&) noexcept = default;

osc::StateModifications::~StateModifications() noexcept = default;

void osc::StateModifications::pushCoordinateEdit(const OpenSim::Coordinate& c, const CoordinateEdit& ce)
{
    m_Impl->pushCoordinateEdit(c, ce);
}

bool osc::StateModifications::removeCoordinateEdit(OpenSim::Coordinate const& c)
{
    return m_Impl->removeCoordinateEdit(c);
}

bool osc::StateModifications::applyToState(const OpenSim::Model& m, SimTK::State& st)
{
    return m_Impl->applyToState(m, st);
}
