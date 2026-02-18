#include "MiUndoableActions.h"

#include <libopensimcreator/Documents/MeshImporter/MiBody.h>
#include <libopensimcreator/Documents/MeshImporter/MiDocument.h>
#include <libopensimcreator/Documents/MeshImporter/MiDocumentHelpers.h>
#include <libopensimcreator/Documents/MeshImporter/MiJoint.h>
#include <libopensimcreator/Documents/MeshImporter/MiIDs.h>
#include <libopensimcreator/Documents/MeshImporter/MiObject.h>
#include <libopensimcreator/Documents/MeshImporter/MiObjectHelpers.h>
#include <libopensimcreator/Documents/MeshImporter/MiMesh.h>
#include <libopensimcreator/Documents/MeshImporter/MiStation.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_set>

using osc::UID;

bool osc::point_axis_towards(
    MiUndoableDocument& udoc,
    UID id,
    int axis,
    UID other)
{
    point_axis_towards(udoc.upd_scratch(), id, axis, other);
    udoc.commit_scratch("reoriented " + udoc.scratch().getLabelByID(id));
    return true;
}

bool osc::TryAssignMeshAttachments(
    MiUndoableDocument& udoc,
    const std::unordered_set<UID>& meshIDs,
    UID newAttachment)
{
    MiDocument& doc = udoc.upd_scratch();

    if (newAttachment != MiIDs::Ground() && !doc.contains<MiBody>(newAttachment))
    {
        return false;  // bogus ID passed
    }

    for (const UID id : meshIDs)
    {
        auto* const ptr = doc.tryUpdByID<MiMesh>(id);
        if (!ptr)
        {
            continue;  // hardening: ignore invalid assignments
        }

        ptr->setParentID(newAttachment);
    }

    std::stringstream commitMsg;
    commitMsg << "assigned mesh";
    if (meshIDs.size() > 1)
    {
        commitMsg << "es";
    }
    commitMsg << " to " << doc.getByID(newAttachment).getLabel();


    udoc.commit_scratch(std::move(commitMsg).str());

    return true;
}

bool osc::TryCreateJoint(
    MiUndoableDocument& udoc,
    UID childID,
    UID parentID)
{
    MiDocument& doc = udoc.upd_scratch();

    const Vector3 parentPos = doc.getPosByID(parentID);
    const Vector3 childPos = doc.getPosByID(childID);
    const Vector3 midPoint = midpoint(parentPos, childPos);

    const auto& joint = doc.emplace<MiJoint>(
        UID{},
        "WeldJoint",
        std::string{},
        parentID,
        childID,
        Transform{.translation = midPoint}
    );
    doc.selectOnly(joint);

    udoc.commit_scratch("added " + joint.getLabel());

    return true;
}

bool osc::TryOrientObjectAxisAlongTwoPoints(
    MiUndoableDocument& udoc,
    UID id,
    int axis,
    Vector3 p1,
    Vector3 p2)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const Vector3 direction = normalize(p2 - p1);
    const Transform t = obj->getXForm(doc);

    obj->setXform(doc, point_axis_along(t, axis, direction));
    udoc.commit_scratch("reoriented " + obj->getLabel());

    return true;
}

bool osc::TryOrientObjectAxisAlongTwoObjects(
    MiUndoableDocument& udoc,
    UID id,
    int axis,
    UID obj1,
    UID obj2)
{
    return TryOrientObjectAxisAlongTwoPoints(
        udoc,
        id,
        axis,
        udoc.scratch().getPosByID(obj1),
        udoc.scratch().getPosByID(obj2)
    );
}

bool osc::TryTranslateObjectBetweenTwoPoints(
    MiUndoableDocument& udoc,
    UID id,
    const Vector3& a,
    const Vector3& b)
{
    MiDocument& doc = udoc.upd_scratch();
    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    obj->setPos(doc, midpoint(a, b));
    udoc.commit_scratch("translated " + obj->getLabel());

    return true;
}

bool osc::TryTranslateBetweenTwoObjects(
    MiUndoableDocument& udoc,
    UID id,
    UID a,
    UID b)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const MiObject* const objA = doc.tryGetByID(a);
    if (!objA)
    {
        return false;
    }

    const MiObject* const objB = doc.tryGetByID(b);
    if (!objB)
    {
        return false;
    }

    obj->setPos(doc, midpoint(objA->getPos(doc), objB->getPos(doc)));
    udoc.commit_scratch("translated " + obj->getLabel());

    return true;
}

bool osc::TryTranslateObjectToAnotherObject(
    MiUndoableDocument& udoc,
    UID id,
    UID other)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const MiObject* const otherObj = doc.tryGetByID(other);
    if (!otherObj)
    {
        return false;
    }

    obj->setPos(doc, otherObj->getPos(doc));
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::TryTranslateToMeshAverageCenter(
    MiUndoableDocument& udoc,
    UID id,
    UID meshID)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const auto* const mesh = doc.tryGetByID<MiMesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    obj->setPos(doc, AverageCenter(*mesh));
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::TryTranslateToMeshBoundsCenter(
    MiUndoableDocument& udoc,
    UID id,
    UID meshID)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const auto* const mesh = doc.tryGetByID<MiMesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    const std::optional<AABB> bounds = mesh->calcBounds();
    if (!bounds)
    {
        return false;
    }

    const Vector3 boundsMidpoint = centroid_of(*bounds);

    obj->setPos(doc, boundsMidpoint);
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::TryTranslateToMeshMassCenter(
    MiUndoableDocument& udoc,
    UID id,
    UID meshID)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const auto* const mesh = doc.tryGetByID<MiMesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    obj->setPos(doc, mass_center_of(*mesh));
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::TryReassignCrossref(
    MiUndoableDocument& udoc,
    UID id,
    int crossref,
    UID other)
{
    if (other == id)
    {
        return false;
    }

    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    if (!doc.contains(other))
    {
        return false;
    }

    obj->setCrossReferenceConnecteeID(crossref, other);
    udoc.commit_scratch("reassigned " + obj->getLabel() + " " + obj->getCrossReferenceLabel(crossref));

    return true;
}

bool osc::DeleteSelected(MiUndoableDocument& udoc)
{
    MiDocument& doc = udoc.upd_scratch();
    if (!doc.hasSelection())
    {
        return false;
    }
    doc.deleteSelected();
    udoc.commit_scratch("deleted selection");
    return true;
}

bool osc::DeleteObject(MiUndoableDocument& udoc, UID id)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const std::string label = to_string(obj->getLabel());

    if (!doc.deleteByID(obj->getID()))
    {
        return false;
    }

    udoc.commit_scratch("deleted " + label);
    return true;
}

void osc::RotateAxis(
    MiUndoableDocument& udoc,
    MiObject& el,
    int axis,
    Radians radians)
{
    const MiDocument& doc = udoc.upd_scratch();
    el.setXform(doc, rotate_axis(el.getXForm(doc), axis, radians));
    udoc.commit_scratch("reoriented " + el.getLabel());
}

bool osc::TryCopyOrientation(
    MiUndoableDocument& udoc,
    UID id,
    UID other)
{
    MiDocument& doc = udoc.upd_scratch();

    MiObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const MiObject* const otherObj = doc.tryGetByID(other);
    if (!otherObj)
    {
        return false;
    }

    obj->set_rotation(doc, otherObj->rotation(doc));
    udoc.commit_scratch("reoriented " + obj->getLabel());

    return true;
}


UID osc::AddBody(
    MiUndoableDocument& udoc,
    const Vector3& position,
    UID andTryAttach)
{
    MiDocument& doc = udoc.upd_scratch();

    const auto& b = doc.emplace<MiBody>(UID{}, MiBody::Class().generateName(), Transform{.translation = position});
    doc.deSelectAll();
    doc.select(b.getID());

    auto* const obj = doc.tryUpdByID<MiMesh>(andTryAttach);
    if (obj)
    {
        if (obj->getParentID() == MiIDs::Ground() || obj->getParentID() == MiIDs::Empty())
        {
            obj->setParentID(b.getID());
            doc.select(*obj);
        }
    }

    udoc.commit_scratch(std::string{"added "} + b.getLabel());

    return b.getID();
}

UID osc::AddBody(MiUndoableDocument& udoc)
{
    return AddBody(udoc, {}, MiIDs::Empty());
}

bool osc::AddStationAtLocation(
    MiUndoableDocument& udoc,
    const MiObject& obj,
    const Vector3& loc)
{
    MiDocument& doc = udoc.upd_scratch();

    if (!CanAttachStationTo(obj))
    {
        return false;
    }

    const auto& station = doc.emplace<MiStation>(
        UID{},
        GetStationAttachmentParent(doc, obj),
        loc,
        MiStation::Class().generateName()
    );
    doc.selectOnly(station);
    udoc.commit_scratch("added station " + station.getLabel());
    return true;
}

bool osc::AddStationAtLocation(
    MiUndoableDocument& udoc,
    UID attachment,
    const Vector3& loc)
{
    const MiDocument& doc = udoc.upd_scratch();

    const auto* const obj = doc.tryGetByID(attachment);
    if (!obj)
    {
        return false;
    }

    return AddStationAtLocation(udoc, *obj, loc);
}

void osc::ActionImportLandmarks(
    MiUndoableDocument& udoc,
    std::span<const opyn::NamedLandmark> landmarks,
    std::optional<std::string> maybeName)
{
    MiDocument& doc = udoc.upd_scratch();
    for (const auto& lm : landmarks)
    {
        doc.emplace<MiStation>(
            UID{},
            MiIDs::Ground(),
            lm.position,
            lm.name
        );
    }

    std::stringstream ss;
    ss << "imported " << std::move(maybeName).value_or("landmarks");
    udoc.commit_scratch(std::move(ss).str());
}
