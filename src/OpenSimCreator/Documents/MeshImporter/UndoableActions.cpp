#include "UndoableActions.hpp"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Body.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Document.hpp>
#include <OpenSimCreator/Documents/MeshImporter/DocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Joint.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectHelpers.hpp>
#include <OpenSimCreator/Documents/MeshImporter/Station.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/UID.hpp>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_set>

using osc::UID;

bool osc::mi::PointAxisTowards(
    UndoableDocument& udoc,
    UID id,
    int axis,
    UID other)
{
    PointAxisTowards(udoc.updScratch(), id, axis, other);
    udoc.commitScratch("reoriented " + udoc.getScratch().getLabelByID(id));
    return true;
}

bool osc::mi::TryAssignMeshAttachments(
    UndoableDocument& udoc,
    std::unordered_set<UID> const& meshIDs,
    UID newAttachment)
{
    Document& doc = udoc.updScratch();

    if (newAttachment != MIIDs::Ground() && !doc.contains<Body>(newAttachment))
    {
        return false;  // bogus ID passed
    }

    for (UID id : meshIDs)
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


    udoc.commitScratch(std::move(commitMsg).str());

    return true;
}

bool osc::mi::TryCreateJoint(
    UndoableDocument& udoc,
    UID childID,
    UID parentID)
{
    Document& doc = udoc.updScratch();

    size_t const jointTypeIdx = *IndexOf<OpenSim::WeldJoint>(GetComponentRegistry<OpenSim::Joint>());
    Vec3 const parentPos = doc.getPosByID(parentID);
    Vec3 const childPos = doc.getPosByID(childID);
    Vec3 const midPoint = Midpoint(parentPos, childPos);

    auto const& joint = doc.emplace<Joint>(
        UID{},
        jointTypeIdx,
        std::string{},
        parentID,
        childID,
        Transform{.position = midPoint}
    );
    doc.selectOnly(joint);

    udoc.commitScratch("added " + joint.getLabel());

    return true;
}

bool osc::mi::TryOrientObjectAxisAlongTwoPoints(
    UndoableDocument& udoc,
    UID id,
    int axis,
    Vec3 p1,
    Vec3 p2)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    Vec3 const direction = Normalize(p2 - p1);
    Transform const t = obj->getXForm(doc);

    obj->setXform(doc, PointAxisAlong(t, axis, direction));
    udoc.commitScratch("reoriented " + obj->getLabel());

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
        udoc.getScratch().getPosByID(obj1),
        udoc.getScratch().getPosByID(obj2)
    );
}

bool osc::mi::TryTranslateObjectBetweenTwoPoints(
    UndoableDocument& udoc,
    UID id,
    Vec3 const& a,
    Vec3 const& b)
{
    Document& doc = udoc.updScratch();
    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    obj->setPos(doc, Midpoint(a, b));
    udoc.commitScratch("translated " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateBetweenTwoObjects(
    UndoableDocument& udoc,
    UID id,
    UID a,
    UID b)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    MIObject const* const objA = doc.tryGetByID(a);
    if (!objA)
    {
        return false;
    }

    MIObject const* const objB = doc.tryGetByID(b);
    if (!objB)
    {
        return false;
    }

    obj->setPos(doc, Midpoint(objA->getPos(doc), objB->getPos(doc)));
    udoc.commitScratch("translated " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateObjectToAnotherObject(
    UndoableDocument& udoc,
    UID id,
    UID other)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    MIObject const* const otherObj = doc.tryGetByID(other);
    if (!otherObj)
    {
        return false;
    }

    obj->setPos(doc, otherObj->getPos(doc));
    udoc.commitScratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshAverageCenter(
    UndoableDocument& udoc,
    UID id,
    UID meshID)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    auto const* const mesh = doc.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    obj->setPos(doc, AverageCenter(*mesh));
    udoc.commitScratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshBoundsCenter(
    UndoableDocument& udoc,
    UID id,
    UID meshID)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    auto const* const mesh = doc.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    Vec3 const boundsMidpoint = Midpoint(mesh->calcBounds());

    obj->setPos(doc, boundsMidpoint);
    udoc.commitScratch("moved " + obj->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshMassCenter(
    UndoableDocument& udoc,
    UID id,
    UID meshID)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    auto const* const mesh = doc.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    obj->setPos(doc, MassCenter(*mesh));
    udoc.commitScratch("moved " + obj->getLabel());

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

    Document& doc = udoc.updScratch();

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
    udoc.commitScratch("reassigned " + obj->getLabel() + " " + obj->getCrossReferenceLabel(crossref));

    return true;
}

bool osc::mi::DeleteSelected(UndoableDocument& udoc)
{
    Document& doc = udoc.updScratch();
    if (!doc.hasSelection())
    {
        return false;
    }
    doc.deleteSelected();
    udoc.commitScratch("deleted selection");
    return true;
}

bool osc::mi::DeleteObject(UndoableDocument& udoc, UID id)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    std::string const label = to_string(obj->getLabel());

    if (!doc.deleteByID(obj->getID()))
    {
        return false;
    }

    udoc.commitScratch("deleted " + label);
    return true;
}

void osc::mi::RotateAxisXRadians(
    UndoableDocument& udoc,
    MIObject& el,
    int axis,
    float radians)
{
    Document& doc = udoc.updScratch();
    el.setXform(doc, RotateAlongAxis(el.getXForm(doc), axis, radians));
    udoc.commitScratch("reoriented " + el.getLabel());
}

bool osc::mi::TryCopyOrientation(
    UndoableDocument& udoc,
    UID id,
    UID other)
{
    Document& doc = udoc.updScratch();

    MIObject* const obj = doc.tryUpdByID(id);
    if (!obj)
    {
        return false;
    }

    MIObject const* const otherObj = doc.tryGetByID(other);
    if (!otherObj)
    {
        return false;
    }

    obj->setRotation(doc, otherObj->getRotation(doc));
    udoc.commitScratch("reoriented " + obj->getLabel());

    return true;
}


UID osc::mi::AddBody(
    UndoableDocument& udoc,
    Vec3 const& pos,
    UID andTryAttach)
{
    Document& doc = udoc.updScratch();

    auto const& b = doc.emplace<Body>(UID{}, Body::Class().generateName(), Transform{.position = pos});
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

    udoc.commitScratch(std::string{"added "} + b.getLabel());

    return b.getID();
}

UID osc::mi::AddBody(UndoableDocument& udoc)
{
    return AddBody(udoc, {}, MIIDs::Empty());
}

bool osc::mi::AddStationAtLocation(
    UndoableDocument& udoc,
    MIObject const& obj,
    Vec3 const& loc)
{
    Document& doc = udoc.updScratch();

    if (!CanAttachStationTo(obj))
    {
        return false;
    }

    auto const& station = doc.emplace<StationEl>(
        UID{},
        GetStationAttachmentParent(doc, obj),
        loc,
        StationEl::Class().generateName()
    );
    doc.selectOnly(station);
    udoc.commitScratch("added station " + station.getLabel());
    return true;
}

bool osc::mi::AddStationAtLocation(
    UndoableDocument& udoc,
    UID attachment,
    Vec3 const& loc)
{
    Document& doc = udoc.updScratch();

    auto const* const obj = doc.tryGetByID(attachment);
    if (!obj)
    {
        return false;
    }

    return AddStationAtLocation(udoc, *obj, loc);
}
