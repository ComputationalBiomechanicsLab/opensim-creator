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
    UndoableDocument& cmg,
    UID id,
    int axis,
    UID other)
{
    PointAxisTowards(cmg.updScratch(), id, axis, other);
    cmg.commitScratch("reoriented " + getLabel(cmg.getScratch(), id));
    return true;
}

bool osc::mi::TryAssignMeshAttachments(
    UndoableDocument& cmg,
    std::unordered_set<UID> const& meshIDs,
    UID newAttachment)
{
    Document& mg = cmg.updScratch();

    if (newAttachment != MIIDs::Ground() && !mg.containsEl<Body>(newAttachment))
    {
        return false;  // bogus ID passed
    }

    for (UID id : meshIDs)
    {
        auto* const ptr = mg.tryUpdByID<Mesh>(id);
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
    commitMsg << " to " << mg.getElByID(newAttachment).getLabel();


    cmg.commitScratch(std::move(commitMsg).str());

    return true;
}

bool osc::mi::TryCreateJoint(
    UndoableDocument& cmg,
    UID childID,
    UID parentID)
{
    Document& mg = cmg.updScratch();

    size_t const jointTypeIdx = *IndexOf<OpenSim::WeldJoint>(GetComponentRegistry<OpenSim::Joint>());
    Vec3 const parentPos = GetPosition(mg, parentID);
    Vec3 const childPos = GetPosition(mg, childID);
    Vec3 const midPoint = Midpoint(parentPos, childPos);

    auto const& jointEl = mg.emplaceEl<Joint>(
        UID{},
        jointTypeIdx,
        std::string{},
        parentID,
        childID,
        Transform{.position = midPoint}
    );
    SelectOnly(mg, jointEl);

    cmg.commitScratch("added " + jointEl.getLabel());

    return true;
}

bool osc::mi::TryOrientElementAxisAlongTwoPoints(
    UndoableDocument& cmg,
    UID id,
    int axis,
    Vec3 p1,
    Vec3 p2)
{
    Document& mg = cmg.updScratch();
    MIObject* const el = mg.tryUpdByID(id);

    if (!el)
    {
        return false;
    }

    Vec3 const direction = Normalize(p2 - p1);
    Transform const t = el->getXForm(mg);

    el->setXform(mg, PointAxisAlong(t, axis, direction));
    cmg.commitScratch("reoriented " + el->getLabel());

    return true;
}

bool osc::mi::TryOrientElementAxisAlongTwoElements(
    UndoableDocument& cmg,
    UID id,
    int axis,
    UID el1,
    UID el2)
{
    return TryOrientElementAxisAlongTwoPoints(
        cmg,
        id,
        axis,
        GetPosition(cmg.getScratch(), el1),
        GetPosition(cmg.getScratch(), el2)
    );
}

bool osc::mi::TryTranslateElementBetweenTwoPoints(
    UndoableDocument& cmg,
    UID id,
    Vec3 const& a,
    Vec3 const& b)
{
    Document& mg = cmg.updScratch();
    MIObject* const el = mg.tryUpdByID(id);

    if (!el)
    {
        return false;
    }

    el->setPos(mg, Midpoint(a, b));
    cmg.commitScratch("translated " + el->getLabel());

    return true;
}

bool osc::mi::TryTranslateBetweenTwoElements(
    UndoableDocument& cmg,
    UID id,
    UID a,
    UID b)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    MIObject const* const aEl = mg.tryGetByID(a);
    if (!aEl)
    {
        return false;
    }

    MIObject const* const bEl = mg.tryGetByID(b);
    if (!bEl)
    {
        return false;
    }

    el->setPos(mg, Midpoint(aEl->getPos(mg), bEl->getPos(mg)));
    cmg.commitScratch("translated " + el->getLabel());

    return true;
}

bool osc::mi::TryTranslateElementToAnotherElement(
    UndoableDocument& cmg,
    UID id,
    UID other)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    MIObject const* const otherEl = mg.tryGetByID(other);
    if (!otherEl)
    {
        return false;
    }

    el->setPos(mg, otherEl->getPos(mg));
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshAverageCenter(
    UndoableDocument& cmg,
    UID id,
    UID meshID)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    auto const* const mesh = mg.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    el->setPos(mg, AverageCenter(*mesh));
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshBoundsCenter(
    UndoableDocument& cmg,
    UID id,
    UID meshID)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    auto const* const mesh = mg.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    Vec3 const boundsMidpoint = Midpoint(mesh->calcBounds());

    el->setPos(mg, boundsMidpoint);
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::mi::TryTranslateToMeshMassCenter(
    UndoableDocument& cmg,
    UID id,
    UID meshID)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    auto const* const mesh = mg.tryGetByID<Mesh>(meshID);
    if (!mesh)
    {
        return false;
    }

    el->setPos(mg, MassCenter(*mesh));
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::mi::TryReassignCrossref(
    UndoableDocument& cmg,
    UID id,
    int crossref,
    UID other)
{
    if (other == id)
    {
        return false;
    }

    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    if (!mg.containsEl(other))
    {
        return false;
    }

    el->setCrossReferenceConnecteeID(crossref, other);
    cmg.commitScratch("reassigned " + el->getLabel() + " " + el->getCrossReferenceLabel(crossref));

    return true;
}

bool osc::mi::DeleteSelected(UndoableDocument& cmg)
{
    Document& mg = cmg.updScratch();

    if (!HasSelection(mg))
    {
        return false;
    }

    DeleteSelected(cmg.updScratch());
    cmg.commitScratch("deleted selection");

    return true;
}

bool osc::mi::DeleteEl(UndoableDocument& cmg, UID id)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    std::string const label = to_string(el->getLabel());

    if (!mg.deleteEl(*el))
    {
        return false;
    }

    cmg.commitScratch("deleted " + label);
    return true;
}

void osc::mi::RotateAxisXRadians(
    UndoableDocument& cmg,
    MIObject& el,
    int axis,
    float radians)
{
    Document& mg = cmg.updScratch();
    el.setXform(mg, RotateAlongAxis(el.getXForm(mg), axis, radians));
    cmg.commitScratch("reoriented " + el.getLabel());
}

bool osc::mi::TryCopyOrientation(
    UndoableDocument& cmg,
    UID id,
    UID other)
{
    Document& mg = cmg.updScratch();

    MIObject* const el = mg.tryUpdByID(id);
    if (!el)
    {
        return false;
    }

    MIObject const* const otherEl = mg.tryGetByID(other);
    if (!otherEl)
    {
        return false;
    }

    el->setRotation(mg, otherEl->getRotation(mg));
    cmg.commitScratch("reoriented " + el->getLabel());

    return true;
}


UID osc::mi::AddBody(
    UndoableDocument& cmg,
    Vec3 const& pos,
    UID andTryAttach)
{
    Document& mg = cmg.updScratch();

    auto const& b = mg.emplaceEl<Body>(UID{}, Body::Class().generateName(), Transform{.position = pos});
    mg.deSelectAll();
    mg.select(b.getID());

    auto* const el = mg.tryUpdByID<Mesh>(andTryAttach);
    if (el)
    {
        if (el->getParentID() == MIIDs::Ground() || el->getParentID() == MIIDs::Empty())
        {
            el->setParentID(b.getID());
            mg.select(*el);
        }
    }

    cmg.commitScratch(std::string{"added "} + b.getLabel());

    return b.getID();
}

UID osc::mi::AddBody(UndoableDocument& cmg)
{
    return AddBody(cmg, {}, MIIDs::Empty());
}

bool osc::mi::AddStationAtLocation(
    UndoableDocument& cmg,
    MIObject const& el,
    Vec3 const& loc)
{
    Document& mg = cmg.updScratch();

    if (!CanAttachStationTo(el))
    {
        return false;
    }

    auto const& station = mg.emplaceEl<StationEl>(
        UID{},
        GetStationAttachmentParent(mg, el),
        loc,
        StationEl::Class().generateName()
    );
    SelectOnly(mg, station);
    cmg.commitScratch("added station " + station.getLabel());
    return true;
}

bool osc::mi::AddStationAtLocation(
    UndoableDocument& cmg,
    UID elID,
    Vec3 const& loc)
{
    Document& mg = cmg.updScratch();

    auto const* const el = mg.tryGetByID(elID);
    if (!el)
    {
        return false;
    }

    return AddStationAtLocation(cmg, *el, loc);
}
