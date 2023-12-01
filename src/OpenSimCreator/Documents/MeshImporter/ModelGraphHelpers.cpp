#include "ModelGraphHelpers.hpp"

#include <OpenSimCreator/Documents/MeshImporter/BodyEl.hpp>
#include <OpenSimCreator/Documents/MeshImporter/JointEl.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MeshEl.hpp>
#include <OpenSimCreator/Documents/MeshImporter/ModelGraph.hpp>
#include <OpenSimCreator/Documents/MeshImporter/ModelGraphIDs.hpp>
#include <OpenSimCreator/Documents/MeshImporter/SceneEl.hpp>
#include <OpenSimCreator/Documents/MeshImporter/StationEl.hpp>

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

void osc::SelectOnly(ModelGraph& mg, SceneEl const& e)
{
    mg.deSelectAll();
    mg.select(e);
}

bool osc::HasSelection(ModelGraph const& mg)
{
    return !mg.getSelected().empty();
}

void osc::DeleteSelected(ModelGraph& mg)
{
    // copy deletion set to ensure iterator can't be invalidated by deletion
    std::unordered_set<UID> selected = mg.getSelected();

    for (UID id : selected)
    {
        mg.deleteElByID(id);
    }

    mg.deSelectAll();
}

osc::CStringView osc::getLabel(ModelGraph const& mg, UID id)
{
    return mg.getElByID(id).getLabel();
}

osc::Transform osc::GetTransform(ModelGraph const& mg, UID id)
{
    return mg.getElByID(id).getXForm(mg);
}

osc::Vec3 osc::GetPosition(ModelGraph const& mg, UID id)
{
    return mg.getElByID(id).getPos(mg);
}

// returns `true` if `body` participates in any joint in the model graph
bool osc::IsAChildAttachmentInAnyJoint(ModelGraph const& mg, SceneEl const& el)
{
    auto const iterable = mg.iter<JointEl>();
    return std::any_of(iterable.begin(), iterable.end(), [id = el.getID()](JointEl const& j)
    {
        return j.getChildID() == id;
    });
}

// returns `true` if a Joint is complete b.s.
bool osc::IsGarbageJoint(ModelGraph const& modelGraph, JointEl const& jointEl)
{
    if (jointEl.getChildID() == ModelGraphIDs::Ground())
    {
        return true;  // ground cannot be a child in a joint
    }

    if (jointEl.getParentID() == jointEl.getChildID())
    {
        return true;  // is directly attached to itself
    }

    if (jointEl.getParentID() != ModelGraphIDs::Ground() && !modelGraph.containsEl<BodyEl>(jointEl.getParentID()))
    {
        return true;  // has a parent ID that's invalid for this model graph
    }

    if (!modelGraph.containsEl<BodyEl>(jointEl.getChildID()))
    {
        return true;  // has a child ID that's invalid for this model graph
    }

    return false;
}

// returns `true` if `joint` is indirectly or directly attached to ground via its parent
bool osc::IsJointAttachedToGround(
    ModelGraph const& modelGraph,
    JointEl const& joint,
    std::unordered_set<UID>& previousVisits)
{
    OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, joint));

    if (joint.getParentID() == ModelGraphIDs::Ground())
    {
        return true;  // it's directly attached to ground
    }

    auto const* const parent = modelGraph.tryGetElByID<BodyEl>(joint.getParentID());
    if (!parent)
    {
        return false;  // joint's parent is garbage
    }

    // else: recurse to parent
    return IsBodyAttachedToGround(modelGraph, *parent, previousVisits);
}

// returns `true` if `body` is attached to ground
bool osc::IsBodyAttachedToGround(
    ModelGraph const& modelGraph,
    BodyEl const& body,
    std::unordered_set<UID>& previouslyVisitedJoints)
{
    bool childInAtLeastOneJoint = false;

    for (JointEl const& jointEl : modelGraph.iter<JointEl>())
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

bool osc::GetModelGraphIssues(
    ModelGraph const& modelGraph,
    std::vector<std::string>& issuesOut)
{
    issuesOut.clear();

    for (JointEl const& joint : modelGraph.iter<JointEl>())
    {
        if (IsGarbageJoint(modelGraph, joint))
        {
            std::stringstream ss;
            ss << joint.getLabel() << ": joint is garbage (this is an implementation error)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    for (BodyEl const& body : modelGraph.iter<BodyEl>())
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
std::string osc::GetContextMenuSubHeaderText(
    ModelGraph const& mg,
    SceneEl const& e)
{
    std::stringstream ss;
    std::visit(Overload
    {
        [&ss](GroundEl const&)
        {
            ss << "(scene origin)";
        },
        [&ss, &mg](MeshEl const& m)
        {
            ss << '(' << m.getClass().getName() << ", " << m.getPath().filename().string() << ", attached to " << getLabel(mg, m.getParentID()) << ')';
        },
        [&ss](BodyEl const& b)
        {
            ss << '(' << b.getClass().getName() << ')';
        },
        [&ss, &mg](JointEl const& j)
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
bool osc::IsInSelectionGroupOf(
    ModelGraph const& mg,
    UID parent,
    UID id)
{
    if (id == ModelGraphIDs::Empty() || parent == ModelGraphIDs::Empty())
    {
        return false;
    }

    if (id == parent)
    {
        return true;
    }

    BodyEl const* bodyEl = nullptr;

    if (auto const* be = mg.tryGetElByID<BodyEl>(parent))
    {
        bodyEl = be;
    }
    else if (auto const* me = mg.tryGetElByID<MeshEl>(parent))
    {
        bodyEl = mg.tryGetElByID<BodyEl>(me->getParentID());
    }

    if (!bodyEl)
    {
        return false;  // parent isn't attached to any body (or isn't a body)
    }

    if (auto const* be = mg.tryGetElByID<BodyEl>(id))
    {
        return be->getID() == bodyEl->getID();
    }
    else if (auto const* me = mg.tryGetElByID<MeshEl>(id))
    {
        return me->getParentID() == bodyEl->getID();
    }
    else
    {
        return false;
    }
}

void osc::SelectAnythingGroupedWith(ModelGraph& mg, UID el)
{
    ForEachIDInSelectionGroup(mg, el, [&mg](UID other)
    {
        mg.select(other);
    });
}

osc::UID osc::GetStationAttachmentParent(ModelGraph const& mg, SceneEl const& el)
{
    return std::visit(Overload
    {
        [](GroundEl const&) { return ModelGraphIDs::Ground(); },
        [&mg](MeshEl const& meshEl) { return mg.containsEl<BodyEl>(meshEl.getParentID()) ? meshEl.getParentID() : ModelGraphIDs::Ground(); },
        [](BodyEl const& bodyEl) { return bodyEl.getID(); },
        [](JointEl const&) { return ModelGraphIDs::Ground(); },
        [](StationEl const&) { return ModelGraphIDs::Ground(); },
    }, el.toVariant());
}

void osc::PointAxisTowards(
    ModelGraph& mg,
    UID id,
    int axis,
    UID other)
{
    Vec3 const choicePos = GetPosition(mg, other);
    Transform const sourceXform = Transform{.position = GetPosition(mg, id)};

    mg.updElByID(id).setXform(mg, PointAxisTowards(sourceXform, axis, choicePos));
}

// returns recommended rim intensity for an element in the model graph
osc::SceneDecorationFlags osc::computeFlags(
    ModelGraph const& mg,
    UID id,
    std::optional<UID> maybeHoverID)
{
    UID const hoverID = maybeHoverID ? *maybeHoverID : ModelGraphIDs::Empty();

    if (id == ModelGraphIDs::Empty())
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
