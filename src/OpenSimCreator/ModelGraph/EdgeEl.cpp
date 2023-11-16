#include "EdgeEl.hpp"

#include <OpenSimCreator/ModelGraph/ISceneElLookup.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>

osc::Transform osc::EdgeEl::implGetXform(ISceneElLookup const& lookup) const
{
    SceneEl const* first = lookup.find(m_FirstAttachmentID);
    SceneEl const* second = lookup.find(m_FirstAttachmentID);
    if (first && second)
    {
        Vec3 const pos = osc::Midpoint(first->getPos(lookup), second->getPos(lookup));
        return Transform{.position = pos};
    }
    else
    {
        return Identity<Transform>();
    }
}

osc::AABB osc::EdgeEl::implCalcBounds(ISceneElLookup const& lookup) const
{
    SceneEl const* first = lookup.find(m_FirstAttachmentID);
    SceneEl const* second = lookup.find(m_FirstAttachmentID);
    if (first && second)
    {
        return osc::BoundingAABBOf(std::to_array(
        {
            first->getPos(lookup),
            second->getPos(lookup),
        }));
    }
    else
    {
        return AABB{};
    }
}
