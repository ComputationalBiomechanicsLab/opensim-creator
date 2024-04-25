#include "MIObject.h"

#include <oscar/Maths/MathHelpers.h>

#include <ranges>

void osc::mi::MIObject::applyRotation(
    IObjectFinder const& lookup,
    Eulers const& eulerAngles,
    Vec3 const& rotationCenter)
{
    Transform t = getXForm(lookup);
    apply_worldspace_rotation(t, eulerAngles, rotationCenter);
    setXform(lookup, t);
}

bool osc::mi::MIObject::isCrossReferencing(
    UID id,
    CrossrefDirection direction) const
{
    return std::ranges::any_of(implGetCrossReferences(), [id, direction](CrossrefDescriptor const& desc)
    {
        return desc.getConnecteeID() == id && (desc.getDirection() & direction);
    });
}
