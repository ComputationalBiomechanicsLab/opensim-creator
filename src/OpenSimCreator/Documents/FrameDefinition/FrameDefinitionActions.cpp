#include "FrameDefinitionActions.h"

#include <OpenSimCreator/Documents/CustomComponents/CrossProductDefinedFrame.h>
#include <OpenSimCreator/Documents/CustomComponents/CrossProductEdge.h>
#include <OpenSimCreator/Documents/CustomComponents/MidpointLandmark.h>
#include <OpenSimCreator/Documents/CustomComponents/PointToPointEdge.h>
#include <OpenSimCreator/Documents/CustomComponents/SphereLandmark.h>
#include <OpenSimCreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <Simbody.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/StringHelpers.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

void osc::fd::ActionAddSphereInMeshFrame(
    UndoableModelStatePair& model,
    const OpenSim::Mesh& mesh,
    std::optional<Vec3> const& maybeClickPosInGround)
{
    // if the caller requests a location via a click, set the position accordingly
    SimTK::Vec3 const locationInMeshFrame = maybeClickPosInGround ?
        CalcLocationInFrame(mesh.getFrame(), model.getState(), *maybeClickPosInGround) :
        SimTK::Vec3{0.0, 0.0, 0.0};

    std::string const sphereName = GenerateSceneElementName("sphere_");
    std::string const commitMessage = GenerateAddedSomethingCommitMessage(sphereName);

    // create sphere component
    std::unique_ptr<SphereLandmark> sphere = [&mesh, &locationInMeshFrame, &sphereName]()
    {
        auto rv = std::make_unique<SphereLandmark>();
        rv->setName(sphereName);
        rv->set_location(locationInMeshFrame);
        rv->connectSocket_parent_frame(mesh.getFrame());
        return rv;
    }();

    // perform the model mutation
    {
        OpenSim::Model& mutableModel = model.updModel();

        const SphereLandmark& sphereRef = AddModelComponent(mutableModel, std::move(sphere));
        FinalizeConnections(mutableModel);
        InitializeModel(mutableModel);
        InitializeState(mutableModel);
        model.setSelected(&sphereRef);
        model.commit(commitMessage);
    }
}

void osc::fd::ActionAddOffsetFrameInMeshFrame(
    UndoableModelStatePair& model,
    const OpenSim::Mesh& mesh,
    std::optional<Vec3> const& maybeClickPosInGround)
{
    // if the caller requests a location via a click, set the position accordingly
    SimTK::Vec3 const locationInMeshFrame = maybeClickPosInGround ?
        CalcLocationInFrame(mesh.getFrame(), model.getState(), *maybeClickPosInGround) :
        SimTK::Vec3{0.0, 0.0, 0.0};

    std::string const pofName = GenerateSceneElementName("pof_");
    std::string const commitMessage = GenerateAddedSomethingCommitMessage(pofName);

    // create physical offset frame
    std::unique_ptr<OpenSim::PhysicalOffsetFrame> pof = [&mesh, &locationInMeshFrame, &pofName]()
    {
        auto rv = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        rv->setName(pofName);
        rv->set_translation(locationInMeshFrame);
        rv->connectSocket_parent(mesh.getFrame());
        return rv;
    }();

    // perform model mutation
    {
        OpenSim::Model& mutableModel = model.updModel();

        const OpenSim::PhysicalOffsetFrame& pofRef = AddModelComponent(mutableModel, std::move(pof));
        FinalizeConnections(mutableModel);
        InitializeModel(mutableModel);
        InitializeState(mutableModel);
        model.setSelected(&pofRef);
        model.commit(commitMessage);
    }
}

void osc::fd::ActionAddPointToPointEdge(
    UndoableModelStatePair& model,
    const OpenSim::Point& pointA,
    const OpenSim::Point& pointB)
{
    std::string const edgeName = GenerateSceneElementName("edge_");
    std::string const commitMessage = GenerateAddedSomethingCommitMessage(edgeName);

    // create edge
    auto edge = std::make_unique<PointToPointEdge>();
    edge->connectSocket_first_point(pointA);
    edge->connectSocket_second_point(pointB);

    // perform model mutation
    {
        OpenSim::Model& mutableModel = model.updModel();

        const PointToPointEdge& edgeRef = AddModelComponent(mutableModel, std::move(edge));
        FinalizeConnections(mutableModel);
        InitializeModel(mutableModel);
        InitializeState(mutableModel);
        model.setSelected(&edgeRef);
        model.commit(commitMessage);
    }
}

void osc::fd::ActionAddMidpoint(
    UndoableModelStatePair& model,
    const OpenSim::Point& pointA,
    const OpenSim::Point& pointB)
{
    std::string const midpointName = GenerateSceneElementName("midpoint_");
    std::string const commitMessage = GenerateAddedSomethingCommitMessage(midpointName);

    // create midpoint component
    auto midpoint = std::make_unique<MidpointLandmark>();
    midpoint->connectSocket_first_point(pointA);
    midpoint->connectSocket_second_point(pointB);

    // perform model mutation
    {
        OpenSim::Model& mutableModel = model.updModel();

        const MidpointLandmark& midpointRef = AddModelComponent(mutableModel, std::move(midpoint));
        FinalizeConnections(mutableModel);
        InitializeModel(mutableModel);
        InitializeState(mutableModel);
        model.setSelected(&midpointRef);
        model.commit(commitMessage);
    }
}

void osc::fd::ActionAddCrossProductEdge(
    UndoableModelStatePair& model,
    const Edge& edgeA,
    const Edge& edgeB)
{
    std::string const edgeName = GenerateSceneElementName("crossproduct_");
    std::string const commitMessage = GenerateAddedSomethingCommitMessage(edgeName);

    // create cross product edge component
    auto edge = std::make_unique<CrossProductEdge>();
    edge->connectSocket_first_edge(edgeA);
    edge->connectSocket_second_edge(edgeB);

    // perform model mutation
    {
        OpenSim::Model& mutableModel = model.updModel();

        const CrossProductEdge& edgeRef = AddModelComponent(mutableModel, std::move(edge));
        FinalizeConnections(mutableModel);
        InitializeModel(mutableModel);
        InitializeState(mutableModel);
        model.setSelected(&edgeRef);
        model.commit(commitMessage);
    }
}

void osc::fd::ActionSwapSocketAssignments(
    UndoableModelStatePair& model,
    OpenSim::ComponentPath componentAbsPath,
    std::string firstSocketName,
    std::string secondSocketName)
{
    // create commit message
    std::string const commitMessage = [&componentAbsPath, &firstSocketName, &secondSocketName]()
    {
        std::stringstream ss;
        ss << "swapped socket '" << firstSocketName << "' with socket '" << secondSocketName << " in " << componentAbsPath.getComponentName();
        return std::move(ss).str();
    }();

    // look things up in the mutable model
    OpenSim::Model& mutModel = model.updModel();
    OpenSim::Component* const component = FindComponentMut(mutModel, componentAbsPath);
    if (!component)
    {
        log_error("failed to find %s in model, skipping action", componentAbsPath.toString().c_str());
        return;
    }

    OpenSim::AbstractSocket* const firstSocket = FindSocketMut(*component, firstSocketName);
    if (!firstSocket)
    {
        log_error("failed to find socket %s in %s, skipping action", firstSocketName.c_str(), component->getName().c_str());
        return;
    }

    OpenSim::AbstractSocket* const secondSocket = FindSocketMut(*component, secondSocketName);
    if (!secondSocket)
    {
        log_error("failed to find socket %s in %s, skipping action", secondSocketName.c_str(), component->getName().c_str());
        return;
    }

    // perform swap
    std::string const firstSocketPath = firstSocket->getConnecteePath();
    firstSocket->setConnecteePath(secondSocket->getConnecteePath());
    secondSocket->setConnecteePath(firstSocketPath);

    // finalize and commit
    InitializeModel(mutModel);
    InitializeState(mutModel);
    model.commit(commitMessage);
}

void osc::fd::ActionSwapPointToPointEdgeEnds(
    UndoableModelStatePair& model,
    const PointToPointEdge& edge)
{
    ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "first_point", "second_point");
}

void osc::fd::ActionSwapCrossProductEdgeOperands(
    UndoableModelStatePair& model,
    const CrossProductEdge& edge)
{
    ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "first_edge", "second_edge");
}

void osc::fd::ActionAddFrame(
    std::shared_ptr<UndoableModelStatePair> const& model,
    const Edge& firstEdge,
    CoordinateDirection firstEdgeAxis,
    const Edge& otherEdge,
    const OpenSim::Point& origin)
{
    std::string const frameName = GenerateSceneElementName("frame_");
    std::string const commitMessage = GenerateAddedSomethingCommitMessage(frameName);

    // create the frame
    auto frame = std::make_unique<CrossProductDefinedFrame>();
    frame->set_axis_edge_axis(stream_to_string(firstEdgeAxis));
    frame->set_first_cross_product_axis(stream_to_string(firstEdgeAxis.axis().next()));
    frame->connectSocket_axis_edge(firstEdge);
    frame->connectSocket_other_edge(otherEdge);
    frame->connectSocket_origin(origin);

    // perform model mutation
    {
        OpenSim::Model& mutModel = model->updModel();

        const CrossProductDefinedFrame& frameRef = AddModelComponent(mutModel, std::move(frame));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model->setSelected(&frameRef);
        model->commit(commitMessage);
    }
}

void osc::fd::ActionCreateBodyFromFrame(
    std::shared_ptr<UndoableModelStatePair> const& model,
    const OpenSim::ComponentPath& frameAbsPath,
    const OpenSim::ComponentPath& meshAbsPath,
    const OpenSim::ComponentPath& jointFrameAbsPath,
    const OpenSim::ComponentPath& parentFrameAbsPath)
{
    // validate external inputs

    log_debug("validate external inputs");
    const auto* const meshFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), frameAbsPath);
    if (!meshFrame)
    {
        log_error("%s: cannot find frame: skipping body creation", frameAbsPath.toString().c_str());
        return;
    }

    const auto* const mesh = FindComponent<OpenSim::Mesh>(model->getModel(), meshAbsPath);
    if (!mesh)
    {
        log_error("%s: cannot find mesh: skipping body creation", meshAbsPath.toString().c_str());
        return;
    }

    const auto* const jointFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), jointFrameAbsPath);
    if (!jointFrame)
    {
        log_error("%s: cannot find joint frame: skipping body creation", jointFrameAbsPath.toString().c_str());
        return;
    }

    const auto* const parentFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), parentFrameAbsPath);
    if (!parentFrame)
    {
        log_error("%s: cannot find parent frame: skipping body creation", parentFrameAbsPath.toString().c_str());
        return;
    }

    // create body
    log_debug("create body");
    std::string const bodyName = meshFrame->getName() + "_body";
    double const bodyMass = 1.0;
    SimTK::Vec3 const bodyCenterOfMass = {0.0, 0.0, 0.0};
    SimTK::Inertia const bodyInertia = {1.0, 1.0, 1.0};
    auto body = std::make_unique<OpenSim::Body>(
        bodyName,
        bodyMass,
        bodyCenterOfMass,
        bodyInertia
    );

    // create joint (centered using offset frames)
    log_debug("create joint");
    auto joint = std::make_unique<OpenSim::FreeJoint>();
    joint->setName(meshFrame->getName() + "_joint");
    {
        auto jointParentPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        jointParentPof->setParentFrame(*parentFrame);
        jointParentPof->setName(meshFrame->getName() + "_parent_offset");
        jointParentPof->setOffsetTransform(jointFrame->findTransformBetween(model->getState(), *parentFrame));

        // care: ownership change happens here (#642)
        OpenSim::PhysicalOffsetFrame& pof = AddFrame(*joint, std::move(jointParentPof));
        joint->connectSocket_parent_frame(pof);
    }
    {
        auto jointChildPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        jointChildPof->setParentFrame(*body);
        jointChildPof->setName(meshFrame->getName() + "_child_offset");
        jointChildPof->setOffsetTransform(jointFrame->findTransformBetween(model->getState(), *meshFrame));

        // care: ownership change happens here (#642)
        OpenSim::PhysicalOffsetFrame& pof = AddFrame(*joint, std::move(jointChildPof));
        joint->connectSocket_child_frame(pof);
    }

    // create PoF for the mesh
    log_debug("create pof");
    auto meshPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    meshPof->setParentFrame(*body);
    meshPof->setName(mesh->getFrame().getName());
    meshPof->setOffsetTransform(mesh->getFrame().findTransformBetween(model->getState(), *meshFrame));

    // create commit message
    std::string const commitMessage = [&body]()
    {
        std::stringstream ss;
        ss << "created " << body->getName();
        return std::move(ss).str();
    }();

    // start mutating the model
    log_debug("start model mutation");
    try
    {
        // CARE: store mesh path before mutatingthe model, because the mesh reference
        // may become invalidated by other model mutations
        OpenSim::ComponentPath const meshPath = GetAbsolutePath(*mesh);

        OpenSim::Model& mutModel = model->updModel();

        OpenSim::PhysicalOffsetFrame& meshPofRef = AddComponent(*body, std::move(meshPof));
        AddJoint(mutModel, std::move(joint));
        OpenSim::Body& bodyRef = AddBody(mutModel, std::move(body));

        // attach copy of source mesh to mesh PoF
        //
        // (must be done after adding body etc. to model and finalizing - #325)
        FinalizeConnections(mutModel);
        AttachGeometry<OpenSim::Mesh>(meshPofRef, *mesh);

        // ensure model is in a valid, initialized, state before moving
        // and reassigning things around
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        // if the mesh's PoF was only used by the mesh then reassign
        // everything to the new PoF and delete the old one
        if (const auto* pof = GetOwner<OpenSim::PhysicalOffsetFrame>(*mesh);
            pof && GetNumChildren(*pof) == 3)  // mesh+frame geom+wrap object set
        {
            log_debug("reassign sockets");
            RecursivelyReassignAllSockets(mutModel, *pof, meshPofRef);
            FinalizeConnections(mutModel);

            if (auto* mutPof = FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, GetAbsolutePathOrEmpty(pof)))
            {
                log_debug("delete old pof");
                TryDeleteComponentFromModel(mutModel, *mutPof);
                InitializeModel(mutModel);
                InitializeState(mutModel);

                // care: `pof` is now dead
            }
        }

        // delete old mesh
        if (auto* mutMesh = FindComponentMut<OpenSim::Mesh>(mutModel, meshAbsPath))
        {
            log_debug("delete old mesh");
            TryDeleteComponentFromModel(mutModel, *mutMesh);
            InitializeModel(mutModel);
            InitializeState(mutModel);
        }

        InitializeModel(mutModel);
        InitializeState(mutModel);
        model->setSelected(&bodyRef);
        model->commit(commitMessage);
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a body to the model: %s", ex.what());
        model->rollback();
    }
}
