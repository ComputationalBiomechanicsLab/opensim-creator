#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiUndoableDocument.h>

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/uid.h>

#include <optional>
#include <span>
#include <string>
#include <unordered_set>

namespace osc { class MiObject; }

// undoable action support
//
// functions that mutate the undoable data structure and commit changes at the
// correct time
namespace osc
{
    bool point_axis_towards(
        MiUndoableDocument&,
        UID id,
        int axis,
        UID other
    );

    bool TryAssignMeshAttachments(
        MiUndoableDocument&,
        const std::unordered_set<UID>& meshIDs,
        UID newAttachment
    );

    bool TryCreateJoint(
        MiUndoableDocument&,
        UID childID,
        UID parentID
    );

    bool TryOrientObjectAxisAlongTwoPoints(
        MiUndoableDocument&,
        UID id,
        int axis,
        Vector3 p1,
        Vector3 p2
    );

    bool TryOrientObjectAxisAlongTwoObjects(
        MiUndoableDocument&,
        UID id,
        int axis,
        UID obj1,
        UID obj2
    );

    bool TryTranslateObjectBetweenTwoPoints(
        MiUndoableDocument&,
        UID id,
        const Vector3&,
        const Vector3&
    );

    bool TryTranslateBetweenTwoObjects(
        MiUndoableDocument&,
        UID id,
        UID a,
        UID b
    );

    bool TryTranslateObjectToAnotherObject(
        MiUndoableDocument&,
        UID id,
        UID other
    );

    bool TryTranslateToMeshAverageCenter(
        MiUndoableDocument&,
        UID id,
        UID meshID
    );

    bool TryTranslateToMeshBoundsCenter(
        MiUndoableDocument&,
        UID id,
        UID meshID
    );

    bool TryTranslateToMeshMassCenter(
        MiUndoableDocument&,
        UID id,
        UID meshID
    );

    bool TryReassignCrossref(
        MiUndoableDocument&,
        UID id,
        int crossref,
        UID other
    );

    bool DeleteSelected(
        MiUndoableDocument&
    );

    bool DeleteObject(
        MiUndoableDocument&,
        UID id
    );

    void RotateAxis(
        MiUndoableDocument&,
        MiObject& el,
        int axis,
        Radians radians
    );

    bool TryCopyOrientation(
        MiUndoableDocument&,
        UID id,
        UID other
    );

    UID AddBody(
        MiUndoableDocument&,
        const Vector3&,
        UID andTryAttach
    );

    UID AddBody(
        MiUndoableDocument&
    );

    bool AddStationAtLocation(
        MiUndoableDocument&,
        const MiObject& obj,
        const Vector3&
    );

    bool AddStationAtLocation(
        MiUndoableDocument&,
        UID attachment,
        const Vector3&
    );

    void ActionImportLandmarks(
        MiUndoableDocument&,
        std::span<const opyn::NamedLandmark>,
        std::optional<std::string> maybeName = std::nullopt
    );
}
