#include "DocumentHelpers.hpp"

#include <OpenSimCreator/Documents/MeshImporter/Body.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Document.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Joint.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Station.hpp>

#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/StdVariantHelpers.hpp>
#include <oscar/Utils/UID.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

using osc::CStringView;
using osc::SceneDecorationFlags;
using osc::Transform;
using osc::UID;
using osc::Vec3;

void osc::mi::SelectOnly(Document& mg, MIObject const& e)
{
    mg.deSelectAll();
    mg.select(e);
}

bool osc::mi::HasSelection(Document const& mg)
{
    return !mg.getSelected().empty();
}

void osc::mi::DeleteSelected(Document& mg)
{
    // copy deletion set to ensure iterator can't be invalidated by deletion
    std::unordered_set<UID> selected = mg.getSelected();

    for (UID id : selected)
    {
        mg.deleteElByID(id);
    }

    mg.deSelectAll();
}

CStringView osc::mi::getLabel(Document const& mg, UID id)
{
    return mg.getElByID(id).getLabel();
}

Transform osc::mi::GetTransform(Document const& mg, UID id)
{
    return mg.getElByID(id).getXForm(mg);
}

Vec3 osc::mi::GetPosition(Document const& mg, UID id)
{
    return mg.getElByID(id).getPos(mg);
}

// returns `true` if `body` participates in any joint in the model graph
bool osc::mi::IsAChildAttachmentInAnyJoint(Document const& mg, MIObject const& el)
{
    auto const iterable = mg.iter<Joint>();
    return std::any_of(iterable.begin(), iterable.end(), [id = el.getID()](Joint const& j)
    {
        return j.getChildID() == id;
    });
}

// returns `true` if a Joint is complete b.s.
bool osc::mi::IsGarbageJoint(Document const& modelGraph, Joint const& jointEl)
{
    if (jointEl.getChildID() == MIIDs::Ground())
    {
        return true;  // ground cannot be a child in a joint
    }

    if (jointEl.getParentID() == jointEl.getChildID())
    {
        return true;  // is directly attached to itself
    }

    if (jointEl.getParentID() != MIIDs::Ground() && !modelGraph.containsEl<Body>(jointEl.getParentID()))
    {
        return true;  // has a parent ID that's invalid for this model graph
    }

    if (!modelGraph.containsEl<Body>(jointEl.getChildID()))
    {
        return true;  // has a child ID that's invalid for this model graph
    }

    return false;
}

// returns `true` if `joint` is indirectly or directly attached to ground via its parent
bool osc::mi::IsJointAttachedToGround(
    Document const& modelGraph,
    Joint const& joint,
    std::unordered_set<UID>& previousVisits)
{
    OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, joint));

    if (joint.getParentID() == MIIDs::Ground())
    {
        return true;  // it's directly attached to ground
    }

    auto const* const parent = modelGraph.tryGetByID<Body>(joint.getParentID());
    if (!parent)
    {
        return false;  // joint's parent is garbage
    }

    // else: recurse to parent
    return IsBodyAttachedToGround(modelGraph, *parent, previousVisits);
}

// returns `true` if `body` is attached to ground
bool osc::mi::IsBodyAttachedToGround(
    Document const& modelGraph,
    Body const& body,
    std::unordered_set<UID>& previouslyVisitedJoints)
{
    bool childInAtLeastOneJoint = false;

    for (Joint const& jointEl : modelGraph.iter<Joint>())
    {
        OSC_ASSERT(!IsGarbageJoint(modelGraph, jointEl));

        if (jointEl.getChildID() == body.getID())
        {
            childInAtLeastOneJoint = true;

            bool const alreadyVisited = !previouslyVisitedJoints.emplace(jointEl.getID()).second;
            if (alreadyVisited)
            {
                continue;  // skip this joint: was previously visited
            }

            if (IsJointAttachedToGround(modelGraph, jointEl, previouslyVisitedJoints))
            {
                return true;  // recurse
            }
        }
    }

    return !childInAtLeastOneJoint;
}

bool osc::mi::GetModelGraphIssues(
    Document const& modelGraph,
    std::vector<std::string>& issuesOut)
{
    issuesOut.clear();

    for (Joint const& joint : modelGraph.iter<Joint>())
    {
        if (IsGarbageJoint(modelGraph, joint))
        {
            std::stringstream ss;
            ss << joint.getLabel() << ": joint is garbage (this is an implementation error)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    for (Body const& body : modelGraph.iter<Body>())
    {
        std::unordered_set<UID> previouslyVisitedJoints;
        if (!IsBodyAttachedToGround(modelGraph, body, previouslyVisitedJoints))
        {
            std::stringstream ss;
            ss << body.getLabel() << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
            issuesOut.push_back(std::move(ss).str());
        }
    }

    return !issuesOut.empty();
}

// returns a string representing the subheader of a scene element
std::string osc::mi::GetContextMenuSubHeaderText(
    Document const& mg,
    MIObject const& e)
{
    std::stringstream ss;
    std::visit(Overload
    {
        [&ss](Ground const&)
        {
            ss << "(scene origin)";
        },
        [&ss, &mg](Mesh const& m)
        {
            ss << '(' << m.getClass().getName() << ", " << m.getPath().filename().string() << ", attached to " << getLabel(mg, m.getParentID()) << ')';
        },
        [&ss](Body const& b)
        {
            ss << '(' << b.getClass().getName() << ')';
        },
        [&ss, &mg](Joint const& j)
        {
            ss << '(' << j.getSpecificTypeName() << ", " << getLabel(mg, j.getChildID()) << " --> " << getLabel(mg, j.getParentID()) << ')';
        },
        [&ss, &mg](StationEl const& s)
        {
            ss << '(' << s.getClass().getName() << ", attached to " << getLabel(mg, s.getParentID()) << ')';
        },
    }, e.toVariant());
    return std::move(ss).str();
}

// returns true if the given element (ID) is in the "selection group" of
bool osc::mi::IsInSelectionGroupOf(
    Document const& mg,
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

    Body const* bodyEl = nullptr;

    if (auto const* be = mg.tryGetByID<Body>(parent))
    {
        bodyEl = be;
    }
    else if (auto const* me = mg.tryGetByID<Mesh>(parent))
    {
        bodyEl = mg.tryGetByID<Body>(me->getParentID());
    }

    if (!bodyEl)
    {
        return false;  // parent isn't attached to any body (or isn't a body)
    }

    if (auto const* be = mg.tryGetByID<Body>(id))
    {
        return be->getID() == bodyEl->getID();
    }
    else if (auto const* me = mg.tryGetByID<Mesh>(id))
    {
        return me->getParentID() == bodyEl->getID();
    }
    else
    {
        return false;
    }
}

void osc::mi::SelectAnythingGroupedWith(Document& mg, UID el)
{
    ForEachIDInSelectionGroup(mg, el, [&mg](UID other)
    {
        mg.select(other);
    });
}

UID osc::mi::GetStationAttachmentParent(Document const& mg, MIObject const& el)
{
    return std::visit(Overload
    {
        [](Ground const&) { return MIIDs::Ground(); },
        [&mg](Mesh const& meshEl) { return mg.containsEl<Body>(meshEl.getParentID()) ? meshEl.getParentID() : MIIDs::Ground(); },
        [](Body const& bodyEl) { return bodyEl.getID(); },
        [](Joint const&) { return MIIDs::Ground(); },
        [](StationEl const&) { return MIIDs::Ground(); },
    }, el.toVariant());
}

void osc::mi::PointAxisTowards(
    Document& mg,
    UID id,
    int axis,
    UID other)
{
    Vec3 const choicePos = GetPosition(mg, other);
    Transform const sourceXform = Transform{.position = GetPosition(mg, id)};

    mg.updElByID(id).setXform(mg, PointAxisTowards(sourceXform, axis, choicePos));
}

// returns recommended rim intensity for an element in the model graph
SceneDecorationFlags osc::mi::computeFlags(
    Document const& mg,
    UID id,
    std::optional<UID> maybeHoverID)
{
    UID const hoverID = maybeHoverID ? *maybeHoverID : MIIDs::Empty();

    if (id == MIIDs::Empty())
    {
        return SceneDecorationFlags::None;
    }
    else if (mg.isSelected(id))
    {
        return SceneDecorationFlags::IsSelected;
    }
    else if (id == hoverID)
    {
        return SceneDecorationFlags::IsHovered | SceneDecorationFlags::IsChildOfHovered;
    }
    else if (IsInSelectionGroupOf(mg, hoverID, id))
    {
        return SceneDecorationFlags::IsChildOfHovered;
    }
    else
    {
        return SceneDecorationFlags::None;
    }
}
