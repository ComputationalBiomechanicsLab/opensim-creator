#pragma once

#include <OpenSimCreator/Documents/MeshImporter/CommittableModelGraph.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>

#include <unordered_set>

namespace osc { class SceneEl; }

// undoable action support
//
// functions that mutate the undoable datastructure and commit changes at the
// correct time
namespace osc
{
    bool PointAxisTowards(
        CommittableModelGraph&,
        UID id,
        int axis,
        UID other
    );

    bool TryAssignMeshAttachments(
        CommittableModelGraph&,
        std::unordered_set<UID> const& meshIDs,
        UID newAttachment
    );

    bool TryCreateJoint(
        CommittableModelGraph&,
        UID childID,
        UID parentID
    );

    bool TryOrientElementAxisAlongTwoPoints(
        CommittableModelGraph&,
        UID id,
        int axis,
        Vec3 p1,
        Vec3 p2
    );

    bool TryOrientElementAxisAlongTwoElements(
        CommittableModelGraph&,
        UID id,
        int axis,
        UID el1,
        UID el2
    );

    bool TryTranslateElementBetweenTwoPoints(
        CommittableModelGraph&,
        UID id,
        Vec3 const& a,
        Vec3 const& b
    );

    bool TryTranslateBetweenTwoElements(
        CommittableModelGraph&,
        UID id,
        UID a,
        UID b
    );

    bool TryTranslateElementToAnotherElement(
        CommittableModelGraph&,
        UID id,
        UID other
    );

    bool TryTranslateToMeshAverageCenter(
        CommittableModelGraph&,
        UID id,
        UID meshID
    );

    bool TryTranslateToMeshBoundsCenter(
        CommittableModelGraph&,
        UID id,
        UID meshID
    );

    bool TryTranslateToMeshMassCenter(
        CommittableModelGraph&,
        UID id,
        UID meshID
    );

    bool TryReassignCrossref(
        CommittableModelGraph&,
        UID id,
        int crossref,
        UID other
    );

    bool DeleteSelected(
        CommittableModelGraph&
    );

    bool DeleteEl(
        CommittableModelGraph&,
        UID id
    );

    void RotateAxisXRadians(
        CommittableModelGraph&,
        SceneEl& el,
        int axis,
        float radians
    );

    bool TryCopyOrientation(
        CommittableModelGraph&,
        UID id,
        UID other
    );

    UID AddBody(
        CommittableModelGraph&,
        Vec3 const& pos,
        UID andTryAttach
    );

    UID AddBody(
        CommittableModelGraph&
    );

    bool AddStationAtLocation(
        CommittableModelGraph&,
        SceneEl const& el,
        Vec3 const& loc
    );

    bool AddStationAtLocation(
        CommittableModelGraph&,
        UID elID,
        Vec3 const& loc
    );
}
