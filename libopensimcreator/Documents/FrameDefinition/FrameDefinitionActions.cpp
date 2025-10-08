#include "FrameDefinitionActions.h"

#include <libopensimcreator/Documents/CustomComponents/CrossProductDefinedFrame.h>
#include <libopensimcreator/Documents/CustomComponents/CrossProductEdge.h>
#include <libopensimcreator/Documents/CustomComponents/MidpointLandmark.h>
#include <libopensimcreator/Documents/CustomComponents/PointToPointEdge.h>
#include <libopensimcreator/Documents/CustomComponents/SphereLandmark.h>
#include <libopensimcreator/Documents/FrameDefinition/FrameDefinitionHelpers.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Platform/Log.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>

#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

void osc::fd::ActionAddSphereInMeshFrame(
    IModelStatePair& model,
    const OpenSim::Mesh& mesh,
    const std::optional<Vector3>& clickPositionInGround)
{
    if (model.isReadonly()) {
        return;
    }

    // if the caller requests a location via a click, set the location accordingly
    const SimTK::Vec3 locationInMeshFrame = clickPositionInGround ?
        CalcLocationInFrame(mesh.getFrame(), model.getState(), *clickPositionInGround) :
        SimTK::Vec3{0.0, 0.0, 0.0};

    const std::string sphereName = GenerateSceneElementName("sphere_");
    const std::string commitMessage = GenerateAddedSomethingCommitMessage(sphereName);

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
    IModelStatePair& model,
    const OpenSim::Mesh& mesh,
    const std::optional<Vector3>& clickPositionInGround)
{
    if (model.isReadonly()) {
        return;
    }

    // if the caller requests a location via a click, set the position accordingly
    const SimTK::Vec3 locationInMeshFrame = clickPositionInGround ?
        CalcLocationInFrame(mesh.getFrame(), model.getState(), *clickPositionInGround) :
        SimTK::Vec3{0.0, 0.0, 0.0};

    const std::string pofName = GenerateSceneElementName("pof_");
    const std::string commitMessage = GenerateAddedSomethingCommitMessage(pofName);

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
    IModelStatePair& model,
    const OpenSim::Point& pointA,
    const OpenSim::Point& pointB)
{
    if (model.isReadonly()) {
        return;
    }

    const std::string edgeName = GenerateSceneElementName("edge_");
    const std::string commitMessage = GenerateAddedSomethingCommitMessage(edgeName);

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
    IModelStatePair& model,
    const OpenSim::Point& pointA,
    const OpenSim::Point& pointB)
{
    if (model.isReadonly()) {
        return;
    }

    const std::string midpointName = GenerateSceneElementName("midpoint_");
    const std::string commitMessage = GenerateAddedSomethingCommitMessage(midpointName);

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
    IModelStatePair& model,
    const Edge& edgeA,
    const Edge& edgeB)
{
    if (model.isReadonly()) {
        return;
    }

    const std::string edgeName = GenerateSceneElementName("crossproduct_");
    const std::string commitMessage = GenerateAddedSomethingCommitMessage(edgeName);

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
    IModelStatePair& model,
    OpenSim::ComponentPath componentAbsPath,
    std::string firstSocketName,
    std::string secondSocketName)
{
    if (model.isReadonly()) {
        return;
    }

    // create commit message
    const std::string commitMessage = [&componentAbsPath, &firstSocketName, &secondSocketName]()
    {
        std::stringstream ss;
        ss << "swapped socket '" << firstSocketName << "' with socket '" << secondSocketName << " in " << componentAbsPath.getComponentName();
        return std::move(ss).str();
    }();

    // look things up in the mutable model
    OpenSim::Model& mutModel = model.updModel();
    OpenSim::Component* const component = FindComponentMut(mutModel, componentAbsPath);
    if (not component) {
        log_error("failed to find %s in model, skipping action", componentAbsPath.toString().c_str());
        return;
    }

    OpenSim::AbstractSocket* const firstSocket = FindSocketMut(*component, firstSocketName);
    if (not firstSocket) {
        log_error("failed to find socket %s in %s, skipping action", firstSocketName.c_str(), component->getName().c_str());
        return;
    }

    OpenSim::AbstractSocket* const secondSocket = FindSocketMut(*component, secondSocketName);
    if (not secondSocket) {
        log_error("failed to find socket %s in %s, skipping action", secondSocketName.c_str(), component->getName().c_str());
        return;
    }

    // perform swap
    const std::string firstSocketPath = firstSocket->getConnecteePath();
    firstSocket->setConnecteePath(secondSocket->getConnecteePath());
    secondSocket->setConnecteePath(firstSocketPath);

    // finalize and commit
    InitializeModel(mutModel);
    InitializeState(mutModel);
    model.commit(commitMessage);
}

void osc::fd::ActionSwapPointToPointEdgeEnds(
    IModelStatePair& model,
    const PointToPointEdge& edge)
{
    ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "first_point", "second_point");
}

void osc::fd::ActionSwapCrossProductEdgeOperands(
    IModelStatePair& model,
    const CrossProductEdge& edge)
{
    ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "first_edge", "second_edge");
}

void osc::fd::ActionAddFrame(
    const std::shared_ptr<IModelStatePair>& model,
    const Edge& firstEdge,
    CoordinateDirection firstEdgeAxis,
    const Edge& otherEdge,
    const OpenSim::Point& origin)
{
    if (model->isReadonly()) {
        return;
    }

    const std::string frameName = GenerateSceneElementName("frame_");
    const std::string commitMessage = GenerateAddedSomethingCommitMessage(frameName);

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
    const std::shared_ptr<IModelStatePair>& model,
    const OpenSim::ComponentPath& frameAbsPath,
    const OpenSim::ComponentPath& meshAbsPath,
    const OpenSim::ComponentPath& jointFrameAbsPath,
    const OpenSim::ComponentPath& parentFrameAbsPath)
{
    if (model->isReadonly()) {
        return;
    }

    // validate external inputs

    log_debug("validate external inputs");
    const auto* const meshFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), frameAbsPath);
    if (not meshFrame) {
        log_error("%s: cannot find frame: skipping body creation", frameAbsPath.toString().c_str());
        return;
    }

    const auto* const mesh = FindComponent<OpenSim::Mesh>(model->getModel(), meshAbsPath);
    if (not mesh) {
        log_error("%s: cannot find mesh: skipping body creation", meshAbsPath.toString().c_str());
        return;
    }

    const auto* const jointFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), jointFrameAbsPath);
    if (not jointFrame) {
        log_error("%s: cannot find joint frame: skipping body creation", jointFrameAbsPath.toString().c_str());
        return;
    }

    const auto* const parentFrame = FindComponent<OpenSim::PhysicalFrame>(model->getModel(), parentFrameAbsPath);
    if (not parentFrame) {
        log_error("%s: cannot find parent frame: skipping body creation", parentFrameAbsPath.toString().c_str());
        return;
    }

    // create body
    log_debug("create body");
    const std::string bodyName = meshFrame->getName() + "_body";
    const double bodyMass = 1.0;
    const SimTK::Vec3 bodyCenterOfMass = {0.0, 0.0, 0.0};
    const SimTK::Inertia bodyInertia = {1.0, 1.0, 1.0};
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
        const OpenSim::PhysicalOffsetFrame& pof = AddFrame(*joint, std::move(jointParentPof));
        joint->connectSocket_parent_frame(pof);
    }
    {
        auto jointChildPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        jointChildPof->setParentFrame(*body);
        jointChildPof->setName(meshFrame->getName() + "_child_offset");
        jointChildPof->setOffsetTransform(jointFrame->findTransformBetween(model->getState(), *meshFrame));

        // care: ownership change happens here (#642)
        const OpenSim::PhysicalOffsetFrame& pof = AddFrame(*joint, std::move(jointChildPof));
        joint->connectSocket_child_frame(pof);
    }

    // create PoF for the mesh
    log_debug("create pof");
    auto meshPof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    meshPof->setParentFrame(*body);
    meshPof->setName(mesh->getFrame().getName());
    meshPof->setOffsetTransform(mesh->getFrame().findTransformBetween(model->getState(), *meshFrame));

    // create commit message
    const std::string commitMessage = [&body]()
    {
        std::stringstream ss;
        ss << "created " << body->getName();
        return std::move(ss).str();
    }();

    // start mutating the model
    log_debug("start model mutation");
    try {
        // CARE: store mesh path before mutatingthe model, because the mesh reference
        // may become invalidated by other model mutations
        const OpenSim::ComponentPath meshPath = GetAbsolutePath(*mesh);

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

            if (auto* mutPof = FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, GetAbsolutePathOrEmpty(pof))) {
                log_debug("delete old pof");
                TryDeleteComponentFromModel(mutModel, *mutPof);
                InitializeModel(mutModel);
                InitializeState(mutModel);

                // care: `pof` is now dead
            }
        }

        // delete old mesh
        if (auto* mutMesh = FindComponentMut<OpenSim::Mesh>(mutModel, meshAbsPath)) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a body to the model"});
    }
}
