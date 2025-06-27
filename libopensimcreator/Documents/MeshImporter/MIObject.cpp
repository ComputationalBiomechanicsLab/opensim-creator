#include "MIObject.h"

#include <liboscar/Maths/MathHelpers.h>

#include <ranges>

namespace rgs = std::ranges;

void osc::mi::MIObject::applyRotation(
    const IObjectFinder& lookup,
    const EulerAngles& eulerAngles,
    const Vec3& rotationCenter)
{
    Transform t = getXForm(lookup);
    apply_world_space_rotation(t, eulerAngles, rotationCenter);
    setXform(lookup, t);
}

bool osc::mi::MIObject::isCrossReferencing(
    UID id,
    CrossrefDirection direction) const
{
    return rgs::any_of(implGetCrossReferences(), [id, direction](const CrossrefDescriptor& desc)
    {
        return desc.getConnecteeID() == id and (desc.getDirection() & direction);
    });
}
