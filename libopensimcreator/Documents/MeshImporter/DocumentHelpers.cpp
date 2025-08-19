#include "DocumentHelpers.h"

#include <libopensimcreator/Documents/MeshImporter/Body.h>
#include <libopensimcreator/Documents/MeshImporter/Document.h>
#include <libopensimcreator/Documents/MeshImporter/Joint.h>
#include <libopensimcreator/Documents/MeshImporter/MIIDs.h>
#include <libopensimcreator/Documents/MeshImporter/MIObject.h>
#include <libopensimcreator/Documents/MeshImporter/Mesh.h>
#include <libopensimcreator/Documents/MeshImporter/Station.h>

#include <liboscar/Graphics/Scene/SceneDecorationFlags.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Shims/Cpp23/ranges.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/UID.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

using namespace osc;

bool osc::mi::IsAChildAttachmentInAnyJoint(const Document& doc, const MIObject& obj)
{
    return cpp23::contains(doc.iter<Joint>(), obj.getID(), &Joint::getChildID);
}

bool osc::mi::IsGarbageJoint(const Document& doc, const Joint& joint)
{
    if (joint.getChildID() == MIIDs::Ground())
    {
        return true;  // ground cannot be a child in a joint
    }

    if (joint.getParentID() == joint.getChildID())
    {
        return true;  // is directly attached to itself
    }

    if (joint.getParentID() != MIIDs::Ground() && !doc.contains<Body>(joint.getParentID()))
    {
        return true;  // has a parent ID that's invalid for this document
    }

    if (!doc.contains<Body>(joint.getChildID()))
    {
        return true;  // has a child ID that's invalid for this document
    }

    return false;
}

bool osc::mi::IsJointAttachedToGround(
    const Document& doc,
    const Joint& joint,
    std::unordered_set<UID>& previousVisits)
{
    OSC_ASSERT_ALWAYS(!IsGarbageJoint(doc, joint));

    if (joint.getParentID() == MIIDs::Ground())
    {
        return true;  // it's directly attached to ground
    }

    const auto* const parent = doc.tryGetByID<Body>(joint.getParentID());
    if (!parent)
    {
        return false;  // joint's parent is garbage
    }

    // else: recurse to parent
    return IsBodyAttachedToGround(doc, *parent, previousVisits);
}

bool osc::mi::IsBodyAttachedToGround(
    const Document& doc,
    const Body& body,
    std::unordered_set<UID>& previouslyVisitedJoints)
{
    bool childInAtLeastOneJoint = false;

    for (const Joint& joint : doc.iter<Joint>())
    {
        OSC_ASSERT(!IsGarbageJoint(doc, joint));

        if (joint.getChildID() == body.getID())
        {
            childInAtLeastOneJoint = true;

            const bool alreadyVisited = !previouslyVisitedJoints.emplace(joint.getID()).second;
            if (alreadyVisited)
            {
                continue;  // skip this joint: was previously visited
            }

            if (IsJointAttachedToGround(doc, joint, previouslyVisitedJoints))
            {
                return true;  // recurse
            }
        }
    }

    return !childInAtLeastOneJoint;
}

bool osc::mi::GetIssues(
    const Document& doc,
    std::vector<std::string>& issuesOut)
{
    issuesOut.clear();

    for (const Joint& joint : doc.iter<Joint>())
    {
        if (IsGarbageJoint(doc, joint))
        {
            std::stringstream ss;
            ss << joint.getLabel() << ": joint is garbage (this is an implementation error)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    for (const Body& body : doc.iter<Body>())
    {
        std::unordered_set<UID> previouslyVisitedJoints;
        if (!IsBodyAttachedToGround(doc, body, previouslyVisitedJoints))
        {
            std::stringstream ss;
            ss << body.getLabel() << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
            issuesOut.push_back(std::move(ss).str());
        }
    }

    return !issuesOut.empty();
}

std::string osc::mi::GetContextMenuSubHeaderText(
    const Document& doc,
    const MIObject& obj)
{
    std::stringstream ss;
    std::visit(Overload
    {
        [&ss](const Ground&)
        {
            ss << "(scene origin)";
        },
        [&ss, &doc](const Mesh& m)
        {
            ss << '(' << m.getClass().getName() << ", " << m.getPath().filename().string() << ", attached to " << doc.getLabelByID(m.getParentID()) << ')';
        },
        [&ss](const Body& b)
        {
            ss << '(' << b.getClass().getName() << ')';
        },
        [&ss, &doc](const Joint& j)
        {
            ss << '(' << j.getSpecificTypeName() << ", " << doc.getLabelByID(j.getChildID()) << " --> " << doc.getLabelByID(j.getParentID()) << ')';
        },
        [&ss, &doc](const StationEl& s)
        {
            ss << '(' << s.getClass().getName() << ", attached to " << doc.getLabelByID(s.getParentID()) << ')';
        },
    }, obj.toVariant());
    return std::move(ss).str();
}

bool osc::mi::IsInSelectionGroupOf(
    const Document& doc,
    UID parent,
    UID id)
{
    if (id == MIIDs::Empty() || parent == MIIDs::Empty())
    {
        return false;
    }

    if (id == parent)
    {
        return true;
    }

    const Body* body = nullptr;
    if (const auto* be = doc.tryGetByID<Body>(parent))
    {
        body = be;
    }
    else if (const auto* me = doc.tryGetByID<Mesh>(parent))
    {
        body = doc.tryGetByID<Body>(me->getParentID());
    }

    if (!body)
    {
        return false;  // parent isn't attached to any body (or isn't a body)
    }

    if (const auto* be = doc.tryGetByID<Body>(id))
    {
        return be->getID() == body->getID();
    }
    else if (const auto* me = doc.tryGetByID<Mesh>(id))
    {
        return me->getParentID() == body->getID();
    }
    else
    {
        return false;
    }
}

void osc::mi::SelectAnythingGroupedWith(Document& doc, UID id)
{
    ForEachIDInSelectionGroup(doc, id, [&doc](UID other)
    {
        doc.select(other);
    });
}

UID osc::mi::GetStationAttachmentParent(const Document& doc, const MIObject& obj)
{
    return std::visit(Overload
    {
        [](const Ground&) { return MIIDs::Ground(); },
        [&doc](const Mesh& meshEl) { return doc.contains<Body>(meshEl.getParentID()) ? meshEl.getParentID() : MIIDs::Ground(); },
        [](const Body& bodyEl) { return bodyEl.getID(); },
        [](const Joint&) { return MIIDs::Ground(); },
        [](const StationEl&) { return MIIDs::Ground(); },
    }, obj.toVariant());
}

void osc::mi::point_axis_towards(
    Document& doc,
    UID id,
    int axis,
    UID other)
{
    const Vector3 choicePos = doc.getPosByID(other);
    const Transform sourceXform = {.translation = doc.getPosByID(id)};

    doc.updByID(id).setXform(doc, point_axis_towards(sourceXform, axis, choicePos));
}

SceneDecorationFlags osc::mi::computeFlags(
    const Document& doc,
    UID id,
    std::optional<UID> maybeHoverID)
{
    if (id == MIIDs::Empty()) {
        return SceneDecorationFlag::Default;
    }

    const UID hoverID = maybeHoverID ? *maybeHoverID : MIIDs::Empty();
    SceneDecorationFlags rv = SceneDecorationFlag::Default;
    if (doc.isSelected(id)) {
        rv |= SceneDecorationFlag::RimHighlight0;
    }
    if (id == hoverID or IsInSelectionGroupOf(doc, hoverID, id)) {
        rv |= SceneDecorationFlag::RimHighlight1;
    }
    return rv;
}
