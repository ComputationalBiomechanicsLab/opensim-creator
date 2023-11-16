#include "CommittableModelGraphActions.hpp"

#include <OpenSimCreator/ModelGraph/BodyEl.hpp>
#include <OpenSimCreator/ModelGraph/JointEl.hpp>
#include <OpenSimCreator/ModelGraph/MeshEl.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraph.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphHelpers.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>
#include <OpenSimCreator/ModelGraph/SceneElHelpers.hpp>
#include <OpenSimCreator/ModelGraph/StationEl.hpp>
#include <OpenSimCreator/Registry/ComponentRegistry.hpp>
#include <OpenSimCreator/Registry/StaticComponentRegistries.hpp>
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

bool osc::PointAxisTowards(
    CommittableModelGraph& cmg,
    UID id,
    int axis,
    UID other)
{
    PointAxisTowards(cmg.updScratch(), id, axis, other);
    cmg.commitScratch("reoriented " + getLabel(cmg.getScratch(), id));
    return true;
}

bool osc::TryAssignMeshAttachments(
    CommittableModelGraph& cmg,
    std::unordered_set<UID> const& meshIDs,
    UID newAttachment)
{
    ModelGraph& mg = cmg.updScratch();

    if (newAttachment != ModelGraphIDs::Ground() && !mg.containsEl<BodyEl>(newAttachment))
    {
        return false;  // bogus ID passed
    }

    for (UID id : meshIDs)
    {
        auto* const ptr = mg.tryUpdElByID<MeshEl>(id);
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

bool osc::TryCreateJoint(
    CommittableModelGraph& cmg,
    UID childID,
    UID parentID)
{
    ModelGraph& mg = cmg.updScratch();

    size_t const jointTypeIdx = *osc::IndexOf<OpenSim::WeldJoint>(osc::GetComponentRegistry<OpenSim::Joint>());
    Vec3 const parentPos = GetPosition(mg, parentID);
    Vec3 const childPos = GetPosition(mg, childID);
    Vec3 const midPoint = osc::Midpoint(parentPos, childPos);

    auto const& jointEl = mg.emplaceEl<JointEl>(
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

bool osc::TryOrientElementAxisAlongTwoPoints(
    CommittableModelGraph& cmg,
    UID id,
    int axis,
    Vec3 p1,
    Vec3 p2)
{
    ModelGraph& mg = cmg.updScratch();
    SceneEl* const el = mg.tryUpdElByID(id);

    if (!el)
    {
        return false;
    }

    Vec3 const direction = osc::Normalize(p2 - p1);
    Transform const t = el->getXForm(mg);

    el->setXform(mg, PointAxisAlong(t, axis, direction));
    cmg.commitScratch("reoriented " + el->getLabel());

    return true;
}

bool osc::TryOrientElementAxisAlongTwoElements(
    CommittableModelGraph& cmg,
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

bool osc::TryTranslateElementBetweenTwoPoints(
    CommittableModelGraph& cmg,
    UID id,
    Vec3 const& a,
    Vec3 const& b)
{
    ModelGraph& mg = cmg.updScratch();
    SceneEl* const el = mg.tryUpdElByID(id);

    if (!el)
    {
        return false;
    }

    el->setPos(mg, osc::Midpoint(a, b));
    cmg.commitScratch("translated " + el->getLabel());

    return true;
}

bool osc::TryTranslateBetweenTwoElements(
    CommittableModelGraph& cmg,
    UID id,
    UID a,
    UID b)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
    if (!el)
    {
        return false;
    }

    SceneEl const* const aEl = mg.tryGetElByID(a);
    if (!aEl)
    {
        return false;
    }

    SceneEl const* const bEl = mg.tryGetElByID(b);
    if (!bEl)
    {
        return false;
    }

    el->setPos(mg, osc::Midpoint(aEl->getPos(mg), bEl->getPos(mg)));
    cmg.commitScratch("translated " + el->getLabel());

    return true;
}

bool osc::TryTranslateElementToAnotherElement(
    CommittableModelGraph& cmg,
    UID id,
    UID other)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
    if (!el)
    {
        return false;
    }

    SceneEl const* const otherEl = mg.tryGetElByID(other);
    if (!otherEl)
    {
        return false;
    }

    el->setPos(mg, otherEl->getPos(mg));
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::TryTranslateToMeshAverageCenter(
    CommittableModelGraph& cmg,
    UID id,
    UID meshID)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
    if (!el)
    {
        return false;
    }

    auto const* const mesh = mg.tryGetElByID<MeshEl>(meshID);
    if (!mesh)
    {
        return false;
    }

    el->setPos(mg, osc::AverageCenter(*mesh));
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::TryTranslateToMeshBoundsCenter(
    CommittableModelGraph& cmg,
    UID id,
    UID meshID)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
    if (!el)
    {
        return false;
    }

    auto const* const mesh = mg.tryGetElByID<MeshEl>(meshID);
    if (!mesh)
    {
        return false;
    }

    Vec3 const boundsMidpoint = Midpoint(mesh->calcBounds());

    el->setPos(mg, boundsMidpoint);
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::TryTranslateToMeshMassCenter(
    CommittableModelGraph& cmg,
    UID id,
    UID meshID)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
    if (!el)
    {
        return false;
    }

    auto const* const mesh = mg.tryGetElByID<MeshEl>(meshID);
    if (!mesh)
    {
        return false;
    }

    el->setPos(mg, MassCenter(*mesh));
    cmg.commitScratch("moved " + el->getLabel());

    return true;
}

bool osc::TryReassignCrossref(
    CommittableModelGraph& cmg,
    UID id,
    int crossref,
    UID other)
{
    if (other == id)
    {
        return false;
    }

    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
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

bool osc::DeleteSelected(CommittableModelGraph& cmg)
{
    ModelGraph& mg = cmg.updScratch();

    if (!HasSelection(mg))
    {
        return false;
    }

    DeleteSelected(cmg.updScratch());
    cmg.commitScratch("deleted selection");

    return true;
}

bool osc::DeleteEl(CommittableModelGraph& cmg, UID id)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
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

void osc::RotateAxisXRadians(
    CommittableModelGraph& cmg,
    SceneEl& el,
    int axis,
    float radians)
{
    ModelGraph& mg = cmg.updScratch();
    el.setXform(mg, RotateAlongAxis(el.getXForm(mg), axis, radians));
    cmg.commitScratch("reoriented " + el.getLabel());
}

bool osc::TryCopyOrientation(
    CommittableModelGraph& cmg,
    UID id,
    UID other)
{
    ModelGraph& mg = cmg.updScratch();

    SceneEl* const el = mg.tryUpdElByID(id);
    if (!el)
    {
        return false;
    }

    SceneEl const* const otherEl = mg.tryGetElByID(other);
    if (!otherEl)
    {
        return false;
    }

    el->setRotation(mg, otherEl->getRotation(mg));
    cmg.commitScratch("reoriented " + el->getLabel());

    return true;
}


osc::UID osc::AddBody(
    CommittableModelGraph& cmg,
    Vec3 const& pos,
    UID andTryAttach)
{
    ModelGraph& mg = cmg.updScratch();

    auto const& b = mg.emplaceEl<BodyEl>(UID{}, BodyEl::Class().generateName(), Transform{.position = pos});
    mg.deSelectAll();
    mg.select(b.getID());

    auto* const el = mg.tryUpdElByID<MeshEl>(andTryAttach);
    if (el)
    {
        if (el->getParentID() == ModelGraphIDs::Ground() || el->getParentID() == ModelGraphIDs::Empty())
        {
            el->setParentID(b.getID());
            mg.select(*el);
        }
    }

    cmg.commitScratch(std::string{"added "} + b.getLabel());

    return b.getID();
}

osc::UID osc::AddBody(CommittableModelGraph& cmg)
{
    return AddBody(cmg, {}, ModelGraphIDs::Empty());
}

bool osc::AddStationAtLocation(
    CommittableModelGraph& cmg,
    SceneEl const& el,
    Vec3 const& loc)
{
    ModelGraph& mg = cmg.updScratch();

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

bool osc::AddStationAtLocation(
    CommittableModelGraph& cmg,
    UID elID,
    Vec3 const& loc)
{
    ModelGraph& mg = cmg.updScratch();

    auto const* const el = mg.tryGetElByID(elID);
    if (!el)
    {
        return false;
    }

    return AddStationAtLocation(cmg, *el, loc);
}
