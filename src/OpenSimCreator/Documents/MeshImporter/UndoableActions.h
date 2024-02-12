#pragma once

#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.h>
#include <OpenSimCreator/Documents/MeshImporter/UndoableDocument.h>

#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/UID.h>

#include <optional>
#include <span>
#include <string>
#include <unordered_set>

namespace osc::mi { class MIObject; }

// undoable action support
//
// functions that mutate the undoable datastructure and commit changes at the
// correct time
namespace osc::mi
{
    bool PointAxisTowards(
        UndoableDocument&,
        UID id,
        int axis,
        UID other
    );

    bool TryAssignMeshAttachments(
        UndoableDocument&,
        std::unordered_set<UID> const& meshIDs,
        UID newAttachment
    );

    bool TryCreateJoint(
        UndoableDocument&,
        UID childID,
        UID parentID
    );

    bool TryOrientObjectAxisAlongTwoPoints(
        UndoableDocument&,
        UID id,
        int axis,
        Vec3 p1,
        Vec3 p2
    );

    bool TryOrientObjectAxisAlongTwoObjects(
        UndoableDocument&,
        UID id,
        int axis,
        UID obj1,
        UID obj2
    );

    bool TryTranslateObjectBetweenTwoPoints(
        UndoableDocument&,
        UID id,
        Vec3 const&,
        Vec3 const&
    );

    bool TryTranslateBetweenTwoObjects(
        UndoableDocument&,
        UID id,
        UID a,
        UID b
    );

    bool TryTranslateObjectToAnotherObject(
        UndoableDocument&,
        UID id,
        UID other
    );

    bool TryTranslateToMeshAverageCenter(
        UndoableDocument&,
        UID id,
        UID meshID
    );

    bool TryTranslateToMeshBoundsCenter(
        UndoableDocument&,
        UID id,
        UID meshID
    );

    bool TryTranslateToMeshMassCenter(
        UndoableDocument&,
        UID id,
        UID meshID
    );

    bool TryReassignCrossref(
        UndoableDocument&,
        UID id,
        int crossref,
        UID other
    );

    bool DeleteSelected(
        UndoableDocument&
    );

    bool DeleteObject(
        UndoableDocument&,
        UID id
    );

    void RotateAxis(
        UndoableDocument&,
        MIObject& el,
        int axis,
        Radians radians
    );

    bool TryCopyOrientation(
        UndoableDocument&,
        UID id,
        UID other
    );

    UID AddBody(
        UndoableDocument&,
        Vec3 const&,
        UID andTryAttach
    );

    UID AddBody(
        UndoableDocument&
    );

    bool AddStationAtLocation(
        UndoableDocument&,
        MIObject const& obj,
        Vec3 const&
    );

    bool AddStationAtLocation(
        UndoableDocument&,
        UID attachment,
        Vec3 const&
    );

    void ActionImportLandmarks(
        UndoableDocument&,
        std::span<lm::NamedLandmark const>,
        std::optional<std::string> maybeName = std::nullopt
    );
}
