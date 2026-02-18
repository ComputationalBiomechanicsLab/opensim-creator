#include "mi_object.h"

#include <liboscar/maths/math_helpers.h>

#include <algorithm>

namespace rgs = std::ranges;

void osc::MiObject::applyRotation(
    const MiObjectFinder& lookup,
    const EulerAngles& eulerAngles,
    const Vector3& rotationCenter)
{
    Transform t = getXForm(lookup);
    apply_world_space_rotation(t, eulerAngles, rotationCenter);
    setXform(lookup, t);
}

bool osc::MiObject::isCrossReferencing(
    UID id,
    MiCrossrefDirection direction) const
{
    return rgs::any_of(implGetCrossReferences(), [id, direction](const MiCrossrefDescriptor& desc)
    {
        return desc.getConnecteeID() == id and (desc.getDirection() & direction);
    });
}
