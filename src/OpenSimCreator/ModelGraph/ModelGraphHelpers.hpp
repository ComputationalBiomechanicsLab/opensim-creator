#pragma once

#include <OpenSimCreator/ModelGraph/ModelGraph.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>

#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <string>
#include <unordered_set>
#include <vector>

namespace osc { class BodyEl; }
namespace osc { class JointEl; }

namespace osc
{
    void SelectOnly(ModelGraph&, SceneEl const&);
    bool HasSelection(ModelGraph const&);
    void DeleteSelected(ModelGraph&);
    CStringView getLabel(ModelGraph const&, UID);
    Transform GetTransform(ModelGraph const&, UID);
    Vec3 GetPosition(ModelGraph const&, UID);

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(ModelGraph const&, SceneEl const&);

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(ModelGraph const&, JointEl const&);

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(
        ModelGraph const&,
        BodyEl const&,
        std::unordered_set<UID>& previouslyVisitedJoints
    );

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(
        ModelGraph const&,
        JointEl const&,
        std::unordered_set<UID>&
    );

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(
        ModelGraph const&,
        BodyEl const&,
        std::unordered_set<UID>&
    );

    // returns `true` if `modelGraph` contains issues
    bool GetModelGraphIssues(
        ModelGraph const&,
        std::vector<std::string>&
    );

    // returns a string representing the subheader of a scene element
    std::string GetContextMenuSubHeaderText(
        ModelGraph const& mg,
        SceneEl const&
    );

    // returns true if the given element (ID) is in the "selection group" of
    bool IsInSelectionGroupOf(
        ModelGraph const&,
        UID parent,
        UID id
    );

    template<typename Consumer>
    void ForEachIDInSelectionGroup(
        ModelGraph const& mg,
        UID parent,
        Consumer f)
        requires Invocable<Consumer, UID>
    {
        for (SceneEl const& e : mg.iter())
        {
            UID const id = e.getID();

            if (IsInSelectionGroupOf(mg, parent, id))
            {
                f(id);
            }
        }
    }

    void SelectAnythingGroupedWith(ModelGraph&, UID);

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the scene
    UID GetStationAttachmentParent(ModelGraph const&, SceneEl const&);

    // points an axis of a given element towards some other element in the model graph
    void PointAxisTowards(
        ModelGraph&,
        UID,
        int axis,
        UID
    );

    // returns recommended rim intensity for an element in the model graph
    SceneDecorationFlags computeFlags(
        ModelGraph const&,
        UID id,
        UID hoverID = ModelGraphIDs::Empty()
    );
}
