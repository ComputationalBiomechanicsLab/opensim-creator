#include "Edge.h"

using osc::fd::EdgePoints;

const EdgePoints& osc::fd::Edge::getLocationsInGround(const SimTK::State& s) const
{
    if (isCacheVariableValid(s, _locationsCV)) {
        return getCacheVariableValue(s, _locationsCV);
    }

    auto& locations = updCacheVariableValue(s, _locationsCV);
    locations = calcLocationsInGround(s);
    markCacheVariableValid(s, _locationsCV);
    return locations;
}

SimTK::Vec3 osc::fd::Edge::getStartLocationInGround(const SimTK::State& s) const
{
    return getLocationsInGround(s).start;
}

SimTK::Vec3 osc::fd::Edge::getEndLocationInGround(const SimTK::State& s) const
{
    return getLocationsInGround(s).end;
}

SimTK::Vec3 osc::fd::Edge::getDirectionInGround(const SimTK::State& s) const
{
    auto [start, end] = getLocationsInGround(s);
    return (end - start).normalize();
}

double osc::fd::Edge::getLengthInGround(const SimTK::State& s) const
{
    auto [start, end] = getLocationsInGround(s);
    return (start - end).norm();
}

void osc::fd::Edge::extendAddToSystem(SimTK::MultibodySystem& system) const
{
    Super::extendAddToSystem(system);
    this->_locationsCV = addCacheVariable("locations", EdgePoints{}, SimTK::Stage::Position);
}
