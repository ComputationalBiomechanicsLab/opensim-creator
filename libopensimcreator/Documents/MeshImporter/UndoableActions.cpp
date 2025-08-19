#include "UndoableActions.h"

#include <libopensimcreator/Documents/MeshImporter/Body.h>
#include <libopensimcreator/Documents/MeshImporter/Document.h>
#include <libopensimcreator/Documents/MeshImporter/DocumentHelpers.h>
#include <libopensimcreator/Documents/MeshImporter/Joint.h>
#include <libopensimcreator/Documents/MeshImporter/MIIDs.h>
#include <libopensimcreator/Documents/MeshImporter/MIObject.h>
#include <libopensimcreator/Documents/MeshImporter/MIObjectHelpers.h>
#include <libopensimcreator/Documents/MeshImporter/Mesh.h>
#include <libopensimcreator/Documents/MeshImporter/Station.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/UID.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_set>

using osc::UID;

bool osc::mi::point_axis_towards(
    UndoableDocument& udoc,
    UID id,
    int axis,
    UID other)
{
    point_axis_towards(udoc.upd_scratch(), id, axis, other);
    udoc.commit_scratch("reoriented " + udoc.scratch().getLabelByID(id));
    return true;
}

bool osc::mi::TryAssignMeshAttachments(
    UndoableDocument& udoc,
    const std::unordered_set<UID>& meshIDs,
    UID newAttachment)
{
    Document& doc = udoc.upd_scratch();

    if (newAttachment != MIIDs::Ground() && !doc.contains<Body>(newAttachment))
    {
        return false;  // bogus ID passed
    }

    for (const UID id : meshIDs)
    {
        auto* const ptr = doc.tryUpdByID<Mesh>(id);
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

bool osc::mi::TryCreateJoint(
    UndoableDocument& udoc,
    UID childID,
    UID parentID)
{
    Document& doc = udoc.upd_scratch();

    const Vector3 parentPos = doc.getPosByID(parentID);
    const Vector3 childPos = doc.getPosByID(childID);
    const Vector3 midPoint = midpoint(parentPos, childPos);

    const auto& joint = doc.emplace<Joint>(
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

bool osc::mi::TryOrientObjectAxisAlongTwoPoints(
    UndoableDocument& udoc,
    UID id,
    int axis,
    Vector3 p1,
    Vector3 p2)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
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

bool osc::mi::TryOrientObjectAxisAlongTwoObjects(
    UndoableDocument& udoc,
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

bool osc::mi::TryTranslateObjectBetweenTwoPoints(
    UndoableDocument& udoc,
    UID id,
    const Vector3& a,
    const Vector3& b)
{
    Document& doc = udoc.upd_scratch();
    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    obj->setPos(doc, midpoint(a, b));
    udoc.commit_scratch("translated " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateBetweenTwoObjects(
    UndoableDocument& udoc,
    UID id,
    UID a,
    UID b)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const MIObject* const objA = doc.tryGetByID(a);
    if (!objA)
    {
        return false;
    }

    const MIObject* const objB = doc.tryGetByID(b);
    if (!objB)
    {
        return false;
    }

    obj->setPos(doc, midpoint(objA->getPos(doc), objB->getPos(doc)));
    udoc.commit_scratch("translated " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateObjectToAnotherObject(
    UndoableDocument& udoc,
    UID id,
    UID other)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const MIObject* const otherObj = doc.tryGetByID(other);
    if (!otherObj)
    {
        return false;
    }

    obj->setPos(doc, otherObj->getPos(doc));
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshAverageCenter(
    UndoableDocument& udoc,
    UID id,
    UID meshID)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const auto* const mesh = doc.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    obj->setPos(doc, AverageCenter(*mesh));
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshBoundsCenter(
    UndoableDocument& udoc,
    UID id,
    UID meshID)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const auto* const mesh = doc.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    const Vector3 boundsMidpoint = centroid_of(mesh->calcBounds());

    obj->setPos(doc, boundsMidpoint);
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshMassCenter(
    UndoableDocument& udoc,
    UID id,
    UID meshID)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const auto* const mesh = doc.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    obj->setPos(doc, mass_center_of(*mesh));
    udoc.commit_scratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryReassignCrossref(
    UndoableDocument& udoc,
    UID id,
    int crossref,
    UID other)
{
    if (other == id)
    {
        return false;
    }

    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
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

bool osc::mi::DeleteSelected(UndoableDocument& udoc)
{
    Document& doc = udoc.upd_scratch();
    if (!doc.hasSelection())
    {
        return false;
    }
    doc.deleteSelected();
    udoc.commit_scratch("deleted selection");
    return true;
}

bool osc::mi::DeleteObject(UndoableDocument& udoc, UID id)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
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

void osc::mi::RotateAxis(
    UndoableDocument& udoc,
    MIObject& el,
    int axis,
    Radians radians)
{
    const Document& doc = udoc.upd_scratch();
    el.setXform(doc, rotate_axis(el.getXForm(doc), axis, radians));
    udoc.commit_scratch("reoriented " + el.getLabel());
}

bool osc::mi::TryCopyOrientation(
    UndoableDocument& udoc,
    UID id,
    UID other)
{
    Document& doc = udoc.upd_scratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    const MIObject* const otherObj = doc.tryGetByID(other);
    if (!otherObj)
    {
        return false;
    }

    obj->set_rotation(doc, otherObj->rotation(doc));
    udoc.commit_scratch("reoriented " + obj->getLabel());

    return true;
}


UID osc::mi::AddBody(
    UndoableDocument& udoc,
    const Vector3& position,
    UID andTryAttach)
{
    Document& doc = udoc.upd_scratch();

    const auto& b = doc.emplace<Body>(UID{}, Body::Class().generateName(), Transform{.translation = position});
    doc.deSelectAll();
    doc.select(b.getID());

    auto* const obj = doc.tryUpdByID<Mesh>(andTryAttach);
    if (obj)
    {
        if (obj->getParentID() == MIIDs::Ground() || obj->getParentID() == MIIDs::Empty())
        {
            obj->setParentID(b.getID());
            doc.select(*obj);
        }
    }

    udoc.commit_scratch(std::string{"added "} + b.getLabel());

    return b.getID();
}

UID osc::mi::AddBody(UndoableDocument& udoc)
{
    return AddBody(udoc, {}, MIIDs::Empty());
}

bool osc::mi::AddStationAtLocation(
    UndoableDocument& udoc,
    const MIObject& obj,
    const Vector3& loc)
{
    Document& doc = udoc.upd_scratch();

    if (!CanAttachStationTo(obj))
    {
        return false;
    }

    const auto& station = doc.emplace<StationEl>(
        UID{},
        GetStationAttachmentParent(doc, obj),
        loc,
        StationEl::Class().generateName()
    );
    doc.selectOnly(station);
    udoc.commit_scratch("added station " + station.getLabel());
    return true;
}

bool osc::mi::AddStationAtLocation(
    UndoableDocument& udoc,
    UID attachment,
    const Vector3& loc)
{
    const Document& doc = udoc.upd_scratch();

    const auto* const obj = doc.tryGetByID(attachment);
    if (!obj)
    {
        return false;
    }

    return AddStationAtLocation(udoc, *obj, loc);
}

void osc::mi::ActionImportLandmarks(
    UndoableDocument& udoc,
    std::span<const lm::NamedLandmark> landmarks,
    std::optional<std::string> maybeName)
{
    Document& doc = udoc.upd_scratch();
    for (const auto& lm : landmarks)
    {
        doc.emplace<StationEl>(
            UID{},
            MIIDs::Ground(),
            lm.position,
            lm.name
        );
    }

    std::stringstream ss;
    ss << "imported " << std::move(maybeName).value_or("landmarks");
    udoc.commit_scratch(std::move(ss).str());
}
