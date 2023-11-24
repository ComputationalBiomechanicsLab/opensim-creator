#include "ModelGraphOpenSimBridge.hpp"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.hpp>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/Documents/ModelGraph/BodyEl.hpp>
#include <OpenSimCreator/Documents/ModelGraph/JointEl.hpp>
#include <OpenSimCreator/Documents/ModelGraph/MeshEl.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelCreationFlags.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraph.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphHelpers.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/Documents/ModelGraph/StationEl.hpp>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <OpenSimCreator/Utils/SimTKHelpers.hpp>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/OffsetFrame.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/UID.hpp>
#include <SimTKcommon.h>
#include <SimTKcommon/Mechanics.h>
#include <SimTKcommon/SmallMatrix.h>

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

using osc::BodyEl;
using osc::JointEl;
using osc::Mat4;
using osc::Mesh;
using osc::MeshEl;
using osc::ModelCreationFlags;
using osc::ModelGraph;
using osc::ModelGraphIDs;
using osc::StationEl;
using osc::Transform;
using osc::UID;
using osc::Vec3;

namespace
{
    // stand-in method that should be replaced by actual support for scale-less transforms
    // (dare i call them.... frames ;))
    Transform IgnoreScale(Transform const& t)
    {
        return t.withScale(Vec3{1.0f});
    }

    // attaches a mesh to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
    void AttachMeshElToFrame(
        MeshEl const& meshEl,
        Transform const& parentXform,
        OpenSim::PhysicalFrame& parentPhysFrame)
    {
        // create a POF that attaches to the body
        auto meshPhysOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
        meshPhysOffsetFrame->setName(std::string{meshEl.getLabel()} + "_offset");

        // set the POFs transform to be equivalent to the mesh's (in-ground) transform,
        // but in the parent frame
        SimTK::Transform const mesh2ground = ToSimTKTransform(meshEl.getXForm());
        SimTK::Transform const parent2ground = ToSimTKTransform(parentXform);
        meshPhysOffsetFrame->setOffsetTransform(parent2ground.invert() * mesh2ground);

        // attach the mesh data to the transformed POF
        auto mesh = std::make_unique<OpenSim::Mesh>(meshEl.getPath().string());
        mesh->setName(std::string{meshEl.getLabel()});
        mesh->set_scale_factors(osc::ToSimTKVec3(meshEl.getXForm().scale));
        osc::AttachGeometry(*meshPhysOffsetFrame, std::move(mesh));

        // make it a child of the parent's physical frame
        osc::AddComponent(parentPhysFrame, std::move(meshPhysOffsetFrame));
    }

    // create a body for the `model`, but don't add it to the model yet
    //
    // *may* add any attached meshes to the model, though
    std::unique_ptr<OpenSim::Body> CreateDetatchedBody(
        ModelGraph const& mg,
        BodyEl const& bodyEl)
    {
        auto addedBody = std::make_unique<OpenSim::Body>();

        addedBody->setName(std::string{bodyEl.getLabel()});
        addedBody->setMass(bodyEl.getMass());

        // set the inertia of the emitted body to be nonzero
        //
        // the reason we do this is because having a zero inertia on a body can cause
        // the simulator to freak out in some scenarios.
        {
            double const moment = 0.01 * bodyEl.getMass();
            SimTK::Vec3 const moments{moment, moment, moment};
            SimTK::Vec3 const products{0.0, 0.0, 0.0};
            addedBody->setInertia(SimTK::Inertia{moments, products});
        }

        // connect meshes to the body, if necessary
        //
        // the body's orientation is going to be handled when the joints are added (by adding
        // relevant offset frames etc.)
        for (MeshEl const& mesh : mg.iter<MeshEl>())
        {
            if (mesh.getParentID() == bodyEl.getID())
            {
                AttachMeshElToFrame(mesh, bodyEl.getXForm(), *addedBody);
            }
        }

        return addedBody;
    }

    // result of a lookup for (effectively) a physicalframe
    struct JointAttachmentCachedLookupResult final {

        // can be nullptr (indicating Ground)
        BodyEl const* bodyEl = nullptr;

        // can be nullptr (indicating ground/cache hit)
        std::unique_ptr<OpenSim::Body> createdBody;

        // always != nullptr, can point to `createdBody`, or an existing body from the cache, or Ground
        OpenSim::PhysicalFrame* physicalFrame = nullptr;
    };

    // cached lookup of a physical frame
    //
    // if the frame/body doesn't exist yet, constructs it
    JointAttachmentCachedLookupResult LookupPhysFrame(
        ModelGraph const& mg,
        OpenSim::Model& model,
        std::unordered_map<UID, OpenSim::Body*>& visitedBodies,
        UID elID)
    {
        // figure out what the parent body is. There's 3 possibilities:
        //
        // - null (ground)
        // - found, visited before (get it, but don't make it or add it to the model)
        // - found, not visited before (make it, add it to the model, cache it)

        JointAttachmentCachedLookupResult rv;
        rv.bodyEl = mg.tryGetElByID<BodyEl>(elID);
        rv.createdBody = nullptr;
        rv.physicalFrame = nullptr;

        if (rv.bodyEl)
        {
            auto const it = visitedBodies.find(elID);
            if (it == visitedBodies.end())
            {
                // haven't visited the body before
                rv.createdBody = CreateDetatchedBody(mg, *rv.bodyEl);
                rv.physicalFrame = rv.createdBody.get();

                // add it to the cache
                visitedBodies.emplace(elID, rv.createdBody.get());
            }
            else
            {
                // visited the body before, use cached result
                rv.createdBody = nullptr;  // it's not this function's responsibility to add it
                rv.physicalFrame = it->second;
            }
        }
        else
        {
            // the element is connected to ground
            rv.createdBody = nullptr;
            rv.physicalFrame = &model.updGround();
        }

        return rv;
    }

    // compute the name of a joint from its attached frames
    std::string CalcJointName(
        JointEl const& jointEl,
        OpenSim::PhysicalFrame const& parentFrame,
        OpenSim::PhysicalFrame const& childFrame)
    {
        if (!jointEl.getUserAssignedName().empty())
        {
            return std::string{jointEl.getUserAssignedName()};
        }
        else
        {
            return childFrame.getName() + "_to_" + parentFrame.getName();
        }
    }

    // expresses if a joint has a degree of freedom (i.e. != -1) and the coordinate index of
    // that degree of freedom
    struct JointDegreesOfFreedom final {
        std::array<int, 3> orientation = {-1, -1, -1};
        std::array<int, 3> translation = {-1, -1, -1};
    };

    // returns the indices of each degree of freedom that the joint supports
    JointDegreesOfFreedom GetDegreesOfFreedom(OpenSim::Joint const& joint)
    {
        if (typeid(joint) == typeid(OpenSim::FreeJoint))
        {
            return JointDegreesOfFreedom{{0, 1, 2}, {3, 4, 5}};
        }
        else if (typeid(joint) == typeid(OpenSim::PinJoint))
        {
            return JointDegreesOfFreedom{{-1, -1, 0}, {-1, -1, -1}};
        }
        else
        {
            return JointDegreesOfFreedom{};  // unknown joint type
        }
    }

    // sets the names of a joint's coordinates
    void SetJointCoordinateNames(
        OpenSim::Joint& joint,
        std::string const& prefix)
    {
        constexpr auto c_TranslationNames = std::to_array({"_tx", "_ty", "_tz"});
        constexpr auto c_RotationNames = std::to_array({"_rx", "_ry", "_rz"});

        auto const& registry = osc::GetComponentRegistry<OpenSim::Joint>();
        JointDegreesOfFreedom const dofs = GetDegreesOfFreedom(osc::Get(registry, joint).prototype());

        // translations
        for (int i = 0; i < 3; ++i)
        {
            if (dofs.translation[i] != -1)
            {
                joint.upd_coordinates(dofs.translation[i]).setName(prefix + c_TranslationNames[i]);
            }
        }

        // rotations
        for (int i = 0; i < 3; ++i)
        {
            if (dofs.orientation[i] != -1)
            {
                joint.upd_coordinates(dofs.orientation[i]).setName(prefix + c_RotationNames[i]);
            }
        }
    }

    // recursively attaches `joint` to `model` by:
    //
    // - adding child bodies, if necessary
    // - adding an offset frames for each side of the joint
    // - computing relevant offset values for the offset frames, to ensure the bodies/joint-center end up in the right place
    // - setting the joint's default coordinate values based on any differences
    // - RECURSING by figuring out which joints have this joint's child as a parent
    void AttachJointRecursive(
        ModelGraph const& mg,
        OpenSim::Model& model,
        JointEl const& joint,
        std::unordered_map<UID, OpenSim::Body*>& visitedBodies,
        std::unordered_set<UID>& visitedJoints)
    {
        {
            bool const wasInserted = visitedJoints.emplace(joint.getID()).second;
            if (!wasInserted)
            {
                // graph cycle detected: joint was already previously visited and shouldn't be traversed again
                return;
            }
        }

        // lookup each side of the joint, creating the bodies if necessary
        JointAttachmentCachedLookupResult parent = LookupPhysFrame(mg, model, visitedBodies, joint.getParentID());
        JointAttachmentCachedLookupResult child = LookupPhysFrame(mg, model, visitedBodies, joint.getChildID());

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parentPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parentPOF->setName(parent.physicalFrame->getName() + "_offset");
        parentPOF->setParentFrame(*parent.physicalFrame);
        Mat4 toParentPofInParent =  ToInverseMat4(IgnoreScale(GetTransform(mg, joint.getParentID()))) * ToMat4(IgnoreScale(joint.getXForm()));
        parentPOF->set_translation(osc::ToSimTKVec3(toParentPofInParent[3]));
        parentPOF->set_orientation(osc::ToSimTKVec3(osc::ExtractEulerAngleXYZ(toParentPofInParent)));

        // create the child OpenSim::PhysicalOffsetFrame
        auto childPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        childPOF->setName(child.physicalFrame->getName() + "_offset");
        childPOF->setParentFrame(*child.physicalFrame);
        Mat4 const toChildPofInChild = ToInverseMat4(IgnoreScale(GetTransform(mg, joint.getChildID()))) * ToMat4(IgnoreScale(joint.getXForm()));
        childPOF->set_translation(osc::ToSimTKVec3(toChildPofInChild[3]));
        childPOF->set_orientation(osc::ToSimTKVec3(osc::ExtractEulerAngleXYZ(toChildPofInChild)));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto jointUniqPtr = osc::At(osc::GetComponentRegistry<OpenSim::Joint>(), joint.getJointTypeIndex()).instantiate();

        // set its name
        std::string const jointName = CalcJointName(joint, *parent.physicalFrame, *child.physicalFrame);
        jointUniqPtr->setName(jointName);

        // set joint coordinate names
        SetJointCoordinateNames(*jointUniqPtr, jointName);

        // add + connect the joint to the POFs
        //
        // care: ownership change happens here (#642)
        OpenSim::PhysicalOffsetFrame& parentRef = osc::AddFrame(*jointUniqPtr, std::move(parentPOF));
        OpenSim::PhysicalOffsetFrame const& childRef = osc::AddFrame(*jointUniqPtr, std::move(childPOF));
        jointUniqPtr->connectSocket_parent_frame(parentRef);
        jointUniqPtr->connectSocket_child_frame(childRef);

        // if a child body was created during this step (e.g. because it's not a cyclic connection)
        // then add it to the model
        OSC_ASSERT_ALWAYS(parent.createdBody == nullptr && "at this point in the algorithm, all parents should have already been created");
        if (child.createdBody)
        {
            osc::AddBody(model, std::move(child.createdBody));  // add created body to model
        }

        // add the joint to the model
        osc::AddJoint(model, std::move(jointUniqPtr));

        // if there are any meshes attached to the joint, attach them to the parent
        for (MeshEl const& mesh : mg.iter<MeshEl>())
        {
            if (mesh.getParentID() == joint.getID())
            {
                AttachMeshElToFrame(mesh, joint.getXForm(), parentRef);
            }
        }

        // recurse by finding where the child of this joint is the parent of some other joint
        OSC_ASSERT_ALWAYS(child.bodyEl != nullptr && "child should always be an identifiable body element");
        for (JointEl const& otherJoint : mg.iter<JointEl>())
        {
            if (otherJoint.getParentID() == child.bodyEl->getID())
            {
                AttachJointRecursive(mg, model, otherJoint, visitedBodies, visitedJoints);
            }
        }
    }

    // attaches `BodyEl` into `model` by directly attaching it to ground with a WeldJoint
    void AttachBodyDirectlyToGround(
        ModelGraph const& mg,
        OpenSim::Model& model,
        BodyEl const& bodyEl,
        std::unordered_map<UID, OpenSim::Body*>& visitedBodies)
    {
        std::unique_ptr<OpenSim::Body> addedBody = CreateDetatchedBody(mg, bodyEl);
        auto weldJoint = std::make_unique<OpenSim::WeldJoint>();
        auto parentFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        auto childFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();

        // set names
        weldJoint->setName(std::string{bodyEl.getLabel()} + "_to_ground");
        parentFrame->setName("ground_offset");
        childFrame->setName(std::string{bodyEl.getLabel()} + "_offset");

        // make the parent have the same position + rotation as the placed body
        parentFrame->setOffsetTransform(ToSimTKTransform(bodyEl.getXForm()));

        // attach the parent directly to ground and the child directly to the body
        // and make them the two attachments of the joint
        parentFrame->setParentFrame(model.getGround());
        childFrame->setParentFrame(*addedBody);
        weldJoint->connectSocket_parent_frame(*parentFrame);
        weldJoint->connectSocket_child_frame(*childFrame);

        // populate the "already visited bodies" cache
        visitedBodies[bodyEl.getID()] = addedBody.get();

        // add the components into the OpenSim::Model
        osc::AddFrame(*weldJoint, std::move(parentFrame));
        osc::AddFrame(*weldJoint, std::move(childFrame));
        osc::AddBody(model, std::move(addedBody));
        osc::AddJoint(model, std::move(weldJoint));
    }

    void AddStationToModel(
        ModelGraph const& mg,
        ModelCreationFlags flags,
        OpenSim::Model& model,
        StationEl const& stationEl,
        std::unordered_map<UID, OpenSim::Body*>& visitedBodies)
    {

        JointAttachmentCachedLookupResult const res = LookupPhysFrame(mg, model, visitedBodies, stationEl.getParentID());
        OSC_ASSERT_ALWAYS(res.physicalFrame != nullptr && "all physical frames should have been added by this point in the model-building process");

        SimTK::Transform const parentXform = ToSimTKTransform(mg.getElByID(stationEl.getParentID()).getXForm(mg));
        SimTK::Transform const stationXform = ToSimTKTransform(stationEl.getXForm());
        SimTK::Vec3 const locationInParent = (parentXform.invert() * stationXform).p();

        if (flags & ModelCreationFlags::ExportStationsAsMarkers)
        {
            // export as markers in the model's markerset (overridden behavior)
            osc::AddMarker(model, to_string(stationEl.getLabel()), *res.physicalFrame, locationInParent);
        }
        else
        {
            // export as stations in the given frame (default behavior)
            auto station = std::make_unique<OpenSim::Station>(*res.physicalFrame, locationInParent);
            station->setName(to_string(stationEl.getLabel()));

            osc::AddComponent(*res.physicalFrame, std::move(station));
        }
    }

    // tries to find the first body connected to the given PhysicalFrame by assuming
    // that the frame is either already a body or is an offset to a body
    OpenSim::PhysicalFrame const* TryInclusiveRecurseToBodyOrGround(
        OpenSim::Frame const& f,
        std::unordered_set<OpenSim::Frame const*> visitedFrames)
    {
        if (!visitedFrames.emplace(&f).second)
        {
            return nullptr;
        }

        if (auto const* body = dynamic_cast<OpenSim::Body const*>(&f))
        {
            return body;
        }
        else if (auto const* ground = dynamic_cast<OpenSim::Ground const*>(&f))
        {
            return ground;
        }
        else if (auto const* pof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&f))
        {
            return TryInclusiveRecurseToBodyOrGround(pof->getParentFrame(), visitedFrames);
        }
        else if (auto const* station = dynamic_cast<OpenSim::Station const*>(&f))
        {
            return TryInclusiveRecurseToBodyOrGround(station->getParentFrame(), visitedFrames);
        }
        else
        {
            return nullptr;
        }
    }

    // tries to find the first body connected to the given PhysicalFrame by assuming
    // that the frame is either already a body or is an offset to a body
    OpenSim::PhysicalFrame const* TryInclusiveRecurseToBodyOrGround(OpenSim::Frame const& f)
    {
        return TryInclusiveRecurseToBodyOrGround(f, {});
    }

    ModelGraph CreateModelGraphFromInMemoryModel(OpenSim::Model m)
    {
        // init model+state
        osc::InitializeModel(m);
        SimTK::State const& st = osc::InitializeState(m);

        // this is what this method populates
        ModelGraph rv;

        // used to figure out how a body in the OpenSim::Model maps into the ModelGraph
        std::unordered_map<OpenSim::Body const*, UID> bodyLookup;

        // used to figure out how a joint in the OpenSim::Model maps into the ModelGraph
        std::unordered_map<OpenSim::Joint const*, UID> jointLookup;

        // import all the bodies from the model file
        for (OpenSim::Body const& b : m.getComponentList<OpenSim::Body>())
        {
            std::string const name = b.getName();
            Transform const xform = osc::ToTransform(b.getTransformInGround(st));

            auto& el = rv.emplaceEl<BodyEl>(UID{}, name, xform);
            el.setMass(b.getMass());

            bodyLookup.emplace(&b, el.getID());
        }

        // then try and import all the joints (by looking at their connectivity)
        for (OpenSim::Joint const& j : m.getComponentList<OpenSim::Joint>())
        {
            OpenSim::PhysicalFrame const& parentFrame = j.getParentFrame();
            OpenSim::PhysicalFrame const& childFrame = j.getChildFrame();

            OpenSim::PhysicalFrame const* const parentBodyOrGround = TryInclusiveRecurseToBodyOrGround(parentFrame);
            OpenSim::PhysicalFrame const* const childBodyOrGround = TryInclusiveRecurseToBodyOrGround(childFrame);

            if (!parentBodyOrGround || !childBodyOrGround)
            {
                // can't find what they're connected to
                continue;
            }

            auto const maybeType = osc::IndexOf(osc::GetComponentRegistry<OpenSim::Joint>(), j);
            if (!maybeType)
            {
                // joint has a type the mesh importer doesn't support
                continue;
            }

            size_t const type = maybeType.value();
            std::string const name = j.getName();

            UID parent = ModelGraphIDs::Empty();
            if (dynamic_cast<OpenSim::Ground const*>(parentBodyOrGround))
            {
                parent = ModelGraphIDs::Ground();
            }
            else
            {
                auto const it = bodyLookup.find(dynamic_cast<OpenSim::Body const*>(parentBodyOrGround));
                if (it == bodyLookup.end())
                {
                    // joint is attached to a body that isn't ground or cached?
                    continue;
                }
                else
                {
                    parent = it->second;
                }
            }

            UID child = ModelGraphIDs::Empty();
            if (dynamic_cast<OpenSim::Ground const*>(childBodyOrGround))
            {
                // ground can't be a child in a joint
                continue;
            }
            else
            {
                auto const it = bodyLookup.find(dynamic_cast<OpenSim::Body const*>(childBodyOrGround));
                if (it == bodyLookup.end())
                {
                    // joint is attached to a body that isn't ground or cached?
                    continue;
                }
                else
                {
                    child = it->second;
                }
            }

            if (parent == ModelGraphIDs::Empty() || child == ModelGraphIDs::Empty())
            {
                // something horrible happened above
                continue;
            }

            Transform const xform = osc::ToTransform(parentFrame.getTransformInGround(st));

            auto& jointEl = rv.emplaceEl<JointEl>(UID{}, type, name, parent, child, xform);
            jointLookup.emplace(&j, jointEl.getID());
        }


        // then try to import all the meshes
        for (OpenSim::Mesh const& mesh : m.getComponentList<OpenSim::Mesh>())
        {
            std::optional<std::filesystem::path> const maybeMeshPath = osc::FindGeometryFileAbsPath(m, mesh);

            if (!maybeMeshPath)
            {
                continue;
            }

            std::filesystem::path const& realLocation = *maybeMeshPath;

            Mesh meshData;
            try
            {
                meshData = osc::LoadMeshViaSimTK(realLocation.string());
            }
            catch (std::exception const& ex)
            {
                osc::log::error("error loading mesh: %s", ex.what());
                continue;
            }

            OpenSim::Frame const& frame = mesh.getFrame();
            OpenSim::PhysicalFrame const* const frameBodyOrGround = TryInclusiveRecurseToBodyOrGround(frame);

            if (!frameBodyOrGround)
            {
                // can't find what it's connected to?
                continue;
            }

            UID attachment = ModelGraphIDs::Empty();
            if (dynamic_cast<OpenSim::Ground const*>(frameBodyOrGround))
            {
                attachment = ModelGraphIDs::Ground();
            }
            else
            {
                if (auto const bodyIt = bodyLookup.find(dynamic_cast<OpenSim::Body const*>(frameBodyOrGround)); bodyIt != bodyLookup.end())
                {
                    attachment = bodyIt->second;
                }
                else
                {
                    // mesh is attached to something that isn't a ground or a body?
                    continue;
                }
            }

            if (attachment == ModelGraphIDs::Empty())
            {
                // couldn't figure out what to attach to
                continue;
            }

            auto& el = rv.emplaceEl<MeshEl>(UID{}, attachment, meshData, realLocation);
            Transform newTransform = osc::ToTransform(frame.getTransformInGround(st));
            newTransform.scale = osc::ToVec3(mesh.get_scale_factors());

            el.setXform(newTransform);
            el.setLabel(mesh.getName());
        }

        // then try to import all the stations
        for (OpenSim::Station const& station : m.getComponentList<OpenSim::Station>())
        {
            // edge-case: it's a path point: ignore it because it will spam the converter
            if (dynamic_cast<OpenSim::AbstractPathPoint const*>(&station))
            {
                continue;
            }

            if (osc::OwnerIs<OpenSim::AbstractPathPoint>(station))
            {
                continue;
            }

            OpenSim::PhysicalFrame const& frame = station.getParentFrame();
            OpenSim::PhysicalFrame const* const frameBodyOrGround = TryInclusiveRecurseToBodyOrGround(frame);

            UID attachment = ModelGraphIDs::Empty();
            if (dynamic_cast<OpenSim::Ground const*>(frameBodyOrGround))
            {
                attachment = ModelGraphIDs::Ground();
            }
            else
            {
                if (auto const it = bodyLookup.find(dynamic_cast<OpenSim::Body const*>(frameBodyOrGround)); it != bodyLookup.end())
                {
                    attachment = it->second;
                }
                else
                {
                    // station is attached to something that isn't ground or a cached body
                    continue;
                }
            }

            if (attachment == ModelGraphIDs::Empty())
            {
                // can't figure out what to attach to
                continue;
            }

            Vec3 const pos = osc::ToVec3(station.findLocationInFrame(st, m.getGround()));
            std::string const name = station.getName();

            rv.emplaceEl<StationEl>(attachment, pos, name);
        }

        return rv;
    }
}

osc::ModelGraph osc::CreateModelFromOsimFile(std::filesystem::path const& p)
{
    return CreateModelGraphFromInMemoryModel(OpenSim::Model{p.string()});
}

// if there are no issues, returns a new OpenSim::Model created from the Modelgraph
//
// otherwise, returns nullptr and issuesOut will be populated with issue messages
std::unique_ptr<OpenSim::Model> osc::CreateOpenSimModelFromModelGraph(
    ModelGraph const& mg,
    ModelCreationFlags flags,
    std::vector<std::string>& issuesOut)
{
    if (GetModelGraphIssues(mg, issuesOut))
    {
        osc::log::error("cannot create an osim model: issues detected");
        for (std::string const& issue : issuesOut)
        {
            osc::log::error("issue: %s", issue.c_str());
        }
        return nullptr;
    }

    // create the output model
    auto model = std::make_unique<OpenSim::Model>();
    model->updDisplayHints().upd_show_frames() = true;

    // add any meshes that are directly connected to ground (i.e. meshes that are not attached to a body)
    for (MeshEl const& meshEl : mg.iter<MeshEl>())
    {
        if (meshEl.getParentID() == ModelGraphIDs::Ground())
        {
            AttachMeshElToFrame(meshEl, Transform{}, model->updGround());
        }
    }

    // keep track of any bodies/joints already visited (there might be cycles)
    std::unordered_map<UID, OpenSim::Body*> visitedBodies;
    std::unordered_set<UID> visitedJoints;

    // directly connect any bodies that participate in no joints into the model with a default joint
    for (BodyEl const& bodyEl : mg.iter<BodyEl>())
    {
        if (!IsAChildAttachmentInAnyJoint(mg, bodyEl))
        {
            AttachBodyDirectlyToGround(mg, *model, bodyEl, visitedBodies);
        }
    }

    // add bodies that do participate in joints into the model
    //
    // note: these bodies may use the non-participating bodies (above) as parents
    for (JointEl const& jointEl : mg.iter<JointEl>())
    {
        if (jointEl.getParentID() == ModelGraphIDs::Ground() || visitedBodies.find(jointEl.getParentID()) != visitedBodies.end())
        {
            AttachJointRecursive(mg, *model, jointEl, visitedBodies, visitedJoints);
        }
    }

    // add stations into the model
    for (StationEl const& el : mg.iter<StationEl>())
    {
        AddStationToModel(mg, flags, *model, el, visitedBodies);
    }

    // invalidate all properties, so that model.finalizeFromProperties() *must*
    // reload everything with no caching
    //
    // otherwise, parts of the model (cough cough, OpenSim::Geometry::finalizeFromProperties)
    // will fail to load data because it will internally set itself as up to date, even though
    // it failed to load a mesh file because a parent was missing. See #330
    for (OpenSim::Component& c : model->updComponentList())
    {
        for (int i = 0; i < c.getNumProperties(); ++i)
        {
            c.updPropertyByIndex(i);
        }
    }

    // ensure returned model is initialized from latest graph
    model->finalizeConnections();  // ensure all sockets are finalized to paths (#263)
    osc::InitializeModel(*model);
    osc::InitializeState(*model);

    return model;
}

osc::Vec3 osc::GetJointAxisLengths(JointEl const& joint)
{
    auto const& registry = osc::GetComponentRegistry<OpenSim::Joint>();
    JointDegreesOfFreedom const dofs = joint.getJointTypeIndex() < registry.size() ?
        GetDegreesOfFreedom(registry[joint.getJointTypeIndex()].prototype()) :
        JointDegreesOfFreedom{};

    Vec3 rv{};
    for (int i = 0; i < 3; ++i)
    {
        rv[i] = dofs.orientation[static_cast<size_t>(i)] == -1 ? 0.6f : 1.0f;
    }
    return rv;
}
