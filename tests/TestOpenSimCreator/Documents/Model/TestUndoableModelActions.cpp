#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <OpenSim/Simulation/Wrap/WrapSphere.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistryEntry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/ObjectPropertyEdit.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <gtest/gtest.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Utils/Algorithms.h>

#include <functional>
#include <memory>

using namespace osc;

// repro for #642
//
// @AdrianHendrik reported that trying to add a body with an invalid name entirely crashes
// OSC, which implies that the operation causes a segfault
TEST(OpenSimActions, ActionAddBodyToModelThrowsIfBodyNameIsInvalid)
{
    UndoableModelStatePair model;

    BodyDetails details;
    details.bodyName = "test 1";
    details.parentFrameAbsPath = "/ground";  // this is what the dialog defaults to

    ASSERT_ANY_THROW({ ActionAddBodyToModel(model, details); });
}

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(OpenSimActions, ActionUpdateModelFromBackingFileReturnsFalseIfFileDoesNotExist)
{
    UndoableModelStatePair model;

    // it just returns `false` if there's no backing file
    ASSERT_FALSE(ActionUpdateModelFromBackingFile(model));

    // ... but if you say it has an invalid backing file path...
    model.setFilesystemPath("doesnt-exist");

    // then it should just return `false`, rather than (e.g.) exploding
    ASSERT_FALSE(ActionUpdateModelFromBackingFile(model));
}

// repro for #654
//
// the bug is in OpenSim, but the action needs to hack around that bug until it is fixed
// upstream
TEST(OpenSimActions, ActionApplyRangeDeletionPropertyEditReturnsFalseToIndicateFailure)
{
    // create undoable model with one body + joint
    auto undoableModel = []()
    {
        OpenSim::Model model;
        auto body = std::make_unique<OpenSim::Body>("body", 1.0, SimTK::Vec3{}, SimTK::Inertia{});
        auto joint = std::make_unique<OpenSim::PinJoint>();
        joint->setName("joint");
        joint->updCoordinate().setName("rotation");
        joint->connectSocket_parent_frame(model.getGround());
        joint->connectSocket_child_frame(*body);
        model.addJoint(joint.release());
        model.addBody(body.release());
        model.finalizeConnections();
        return UndoableModelStatePair{std::make_unique<OpenSim::Model>(std::move(model))};
    }();

    ObjectPropertyEdit edit
    {
        undoableModel.updModel().updComponent<OpenSim::Coordinate>("/jointset/joint/rotation").updProperty_range(),
        [&](OpenSim::AbstractProperty& p)
        {
            p.clear();
        },
    };

    ASSERT_EQ(ActionApplyPropertyEdit(undoableModel, edit), false);

    // hacky extra test: you can remove this, it's just reminder code
    undoableModel.updModel().updComponent<OpenSim::Coordinate>("/jointset/joint/rotation").updProperty_range().clear();
    ASSERT_ANY_THROW({ InitializeModel(undoableModel.updModel()); });
}

// high-level repro for (#773)
//
// the underlying bug appears to be related to finalizing connections in
// the model graph (grep for 773 to see other tests), but the user-reported
// bug is specifically related to renaming a component
TEST(OpenSimActions, DISABLED_ActionSetComponentNameOnModelWithUnusualJointTopologyDoesNotSegfault)
{
    std::filesystem::path const brokenFilePath =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "TestOpenSimCreator" / "opensim-creator_773-2_repro.osim";

    UndoableModelStatePair const loadedModel{brokenFilePath};

    // loop `n` times because the segfault is stochastic
    //
    // ... which is a cute way of saying "really fucking random" :(
    for (size_t i = 0; i < 25; ++i)
    {
        UndoableModelStatePair model = loadedModel;
        ActionSetComponentName(
            model,
            std::string{"/bodyset/humerus_b"},
            "newName"
        );
    }
}

TEST(OpenSimActions, ActionFitSphereToMeshFitsASphereToAMeshInTheModelAndSelectsIt)
{
    std::filesystem::path const geomFile =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "TestOpenSimCreator" / "arrow.vtp";

    UndoableModelStatePair model;
    auto& body = AddBody(model.updModel(), std::make_unique<OpenSim::Body>("name", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0}));
    body.setMass(1.0);
    auto& mesh = dynamic_cast<OpenSim::Mesh&>(AttachGeometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    FinalizeConnections(model.updModel());
    InitializeModel(model.updModel());
    InitializeState(model.updModel());

    ActionFitSphereToMesh(model, mesh);
    ASSERT_TRUE(model.getSelected());
    ASSERT_TRUE(dynamic_cast<OpenSim::Sphere const*>(model.getSelected()));
    ASSERT_EQ(&dynamic_cast<OpenSim::Sphere const*>(model.getSelected())->getFrame().findBaseFrame(), &body.findBaseFrame());
}

TEST(OpenSimActions, ActionFitSphereToMeshAppliesMeshesScaleFactorsCorrectly)
{
    std::filesystem::path const geomFile =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "TestOpenSimCreator" / "arrow.vtp";

    UndoableModelStatePair model;
    auto& body = AddBody(model.updModel(), std::make_unique<OpenSim::Body>("name", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0}));
    body.setMass(1.0);
    auto& unscaledMesh = dynamic_cast<OpenSim::Mesh&>(AttachGeometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    auto& scaledMesh = dynamic_cast<OpenSim::Mesh&>(AttachGeometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    double const scalar = 0.1;
    scaledMesh.set_scale_factors({scalar, scalar, scalar});

    FinalizeConnections(model.updModel());
    InitializeModel(model.updModel());
    InitializeState(model.updModel());

    ActionFitSphereToMesh(model, unscaledMesh);
    ASSERT_TRUE(dynamic_cast<OpenSim::Sphere const*>(model.getSelected()));
    double const unscaledRadius = dynamic_cast<OpenSim::Sphere const&>(*model.getSelected()).get_radius();
    ActionFitSphereToMesh(model, scaledMesh);
    ASSERT_TRUE(dynamic_cast<OpenSim::Sphere const*>(model.getSelected()));
    double const scaledRadius = dynamic_cast<OpenSim::Sphere const&>(*model.getSelected()).get_radius();

    ASSERT_TRUE(equal_within_reldiff(scaledRadius, scalar*unscaledRadius, 0.0001));
}

TEST(OpenSimActions, ActionAddParentOffsetFrameToJointWorksInNormalCase)
{
    UndoableModelStatePair um;
    auto& body = AddBody(um.updModel(), "bodyname", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0});
    auto& joint = AddJoint<OpenSim::FreeJoint>(um.updModel(), "jname", um.getModel().getGround(), body);

    // this should be ok
    FinalizeConnections(um.updModel());
    InitializeModel(um.updModel());
    InitializeState(um.updModel());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getParentFrame(), &um.getModel().getGround());

    // and now we ask for a new `PhysicalOffsetFrame` to be injected into the parent, which works
    ASSERT_TRUE(ActionAddParentOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's parent frame is now a `PhysicalOffsetFrame` that's attached to ground
    OpenSim::PhysicalFrame const& parent1 = joint.getParentFrame();
    ASSERT_NE(&parent1, &um.getModel().getGround());
    ASSERT_TRUE(dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&parent1));
    ASSERT_EQ(&dynamic_cast<OpenSim::PhysicalOffsetFrame const&>(parent1).getParentFrame(), &um.getModel().getGround());
}

// ensure that the caller can keep asking to add parent offset frames to a joint - even if the
// joint is already attached to an offset frame
//
// - DISABLED because there's a bug in OpenSim that prevents this from working: https://github.com/opensim-org/opensim-core/pull/3711
TEST(OpenSimActions, DISABLED_ActionAddParentOffsetFrameToJointWorksInChainedCase)
{
    UndoableModelStatePair um;
    auto& body = AddBody(um.updModel(), "bodyname", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0});
    auto& joint = AddJoint<OpenSim::FreeJoint>(um.updModel(), "jname", um.getModel().getGround(), body);

    // this should be ok
    FinalizeConnections(um.updModel());
    InitializeModel(um.updModel());
    InitializeState(um.updModel());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getParentFrame(), &um.getModel().getGround());

    // and now we ask for a new PhysicalOffsetFrame to be injected into the parent, which should work
    ASSERT_TRUE(ActionAddParentOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's parent frame is now a `PhysicalOffsetFrame` that's attached to ground
    OpenSim::PhysicalFrame const& parent1 = joint.getParentFrame();
    ASSERT_NE(&parent1, &um.getModel().getGround());
    ASSERT_TRUE(dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&parent1));
    ASSERT_EQ(&dynamic_cast<OpenSim::PhysicalOffsetFrame const&>(parent1).getParentFrame(), &um.getModel().getGround());

    // repeating the process creates a chain of `PhysicalOffsetFrame`s
    ASSERT_TRUE(ActionAddParentOffsetFrameToJoint(um, joint.getAbsolutePath()));

    OpenSim::PhysicalFrame const& parent2 = joint.getParentFrame();
    ASSERT_NE(&parent1, &parent2);
    ASSERT_TRUE(dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&parent2));
    ASSERT_EQ(&dynamic_cast<OpenSim::PhysicalOffsetFrame const&>(parent2).getParentFrame(), &parent1);
}

TEST(OpenSimActions, ActionAddChildOffsetFrameToJointWorksInNormalCase)
{
    UndoableModelStatePair um;
    auto& body = AddBody(um.updModel(), "bodyname", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0});
    auto& joint = AddJoint<OpenSim::FreeJoint>(um.updModel(), "jname", um.getModel().getGround(), body);

    // this should be ok
    FinalizeConnections(um.updModel());
    InitializeModel(um.updModel());
    InitializeState(um.updModel());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getChildFrame(), &body);

    // and now we ask for a new `PhysicalOffsetFrame` to be injected into the child, which should work
    ASSERT_TRUE(ActionAddChildOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's child frame is now a `PhysicalOffsetFrame` that's attached to the body
    OpenSim::PhysicalFrame const& child1 = joint.getChildFrame();
    ASSERT_NE(&child1, &body);
    ASSERT_TRUE(dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&child1));
    ASSERT_EQ(&dynamic_cast<OpenSim::PhysicalOffsetFrame const&>(child1).getParentFrame(), &body);
}

// ensure that the caller can keep asking to add child offset frames to a joint - even if the
// joint is already attached to an offset frame
//
// - DISABLED because there's a bug in OpenSim that prevents this from working: https://github.com/opensim-org/opensim-core/pull/3711
TEST(OpenSimActions, DISABLED_ActionAddChildOffsetFrameToJointWorksInChainedCase)
{
    UndoableModelStatePair um;
    auto& body = AddBody(um.updModel(), "bodyname", 1.0, SimTK::Vec3{}, SimTK::Inertia{1.0});
    auto& joint = AddJoint<OpenSim::FreeJoint>(um.updModel(), "jname", um.getModel().getGround(), body);

    // this should be ok
    FinalizeConnections(um.updModel());
    InitializeModel(um.updModel());
    InitializeState(um.updModel());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getChildFrame(), &body);

    // and now we ask for a new `PhysicalOffsetFrame` to be injected into the child, which should work
    ASSERT_TRUE(ActionAddChildOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's child frame is now a `PhysicalOffsetFrame` that's attached to the body
    OpenSim::PhysicalFrame const& child1 = joint.getChildFrame();
    ASSERT_NE(&child1, &body);
    ASSERT_TRUE(dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&child1));
    ASSERT_EQ(&dynamic_cast<OpenSim::PhysicalOffsetFrame const&>(child1).getParentFrame(), &body);

    // repeating the process creates a chain of `PhysicalOffsetFrame`s
    ASSERT_TRUE(ActionAddChildOffsetFrameToJoint(um, joint.getAbsolutePath()));

    OpenSim::PhysicalFrame const& child2 = joint.getChildFrame();
    ASSERT_NE(&child2, &child1);
    ASSERT_TRUE(dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&child2));
    ASSERT_EQ(&dynamic_cast<OpenSim::PhysicalOffsetFrame const&>(child2).getParentFrame(), &child1);
}

TEST(OpenSimActions, ActionAddWrapObjectToPhysicalFrameCanAddWrapCylinderToGround)
{
    // test one concrete instance (this test isn't coupled to the component registry)
    UndoableModelStatePair um;
    auto wrapCylinder = std::make_unique<OpenSim::WrapCylinder>();
    wrapCylinder->setName("should_be_findable_in_model");

    ASSERT_TRUE(ActionAddWrapObjectToPhysicalFrame(um, um.getModel().getGround().getAbsolutePath(), std::move(wrapCylinder)));
    ASSERT_TRUE(um.getModel().findComponent("should_be_findable_in_model"));
}

TEST(OpenSimActions, ActionAddWrapObjectToPhysicalFrameCanAddAllRegisteredWrapObjectsToGround)
{
    UndoableModelStatePair um;
    OpenSim::ComponentPath const groundPath = um.getModel().getGround().getAbsolutePath();

    for (auto const& entry : GetComponentRegistry<OpenSim::WrapObject>()) {
        ASSERT_TRUE(ActionAddWrapObjectToPhysicalFrame(um, groundPath, entry.instantiate()));
    }

    size_t numWrapsInModel = 0;
    for ([[maybe_unused]] auto const& wrap : um.getModel().getComponentList<OpenSim::WrapObject>()) {
        ++numWrapsInModel;
    }

    ASSERT_EQ(numWrapsInModel, GetComponentRegistry<OpenSim::WrapObject>().size());
}

TEST(OpenSimActions, ActionAddPathWrapToGeometryPathWorksInExampleCase)
{
    UndoableModelStatePair um;
    OpenSim::Model& model = um.updModel();

    auto& pof = AddModelComponent<OpenSim::PhysicalOffsetFrame>(model, model.getGround(), SimTK::Transform{SimTK::Vec3{0.0, 1.0, 0.0}});
    auto& body = AddBody(um.updModel(), "body", 1.0f, SimTK::Vec3{}, SimTK::Inertia(0.1));
    AddJoint<OpenSim::FreeJoint>(um.updModel(), "joint", pof, body);
    auto& path = AddModelComponent<OpenSim::GeometryPath>(model);
    path.appendNewPathPoint("p1_ground", model.getGround(), SimTK::Vec3{});
    path.appendNewPathPoint("p2_body", body, SimTK::Vec3{});

    FinalizeConnections(model);
    InitializeModel(model);
    auto const& state = InitializeState(model);

    ASSERT_TRUE(equal_within_epsilon(path.getLength(state), 1.0)) << "an uninterupted path should have this length";

    auto& sphere = AddWrapObject<OpenSim::WrapSphere>(pof);
    sphere.set_radius(0.25);
    sphere.set_translation({0.001, -0.5, 0.0});  // prevent singularities

    FinalizeConnections(model);
    InitializeModel(model);
    auto const& state2 = InitializeState(model);

    ASSERT_TRUE(equal_within_epsilon(path.getLength(state2), 1.0)) << "the wrap object hasn't been added to the model yet";

    ActionAddWrapObjectToGeometryPathWraps(um, path, sphere);

    ASSERT_GT(path.getLength(um.getState()), 1.1)  << "path should start wrapping";
}

TEST(OpenSimActions, ActionRemoveWrapObjectFromGeometryPathWrapsWorksInExampleCase)
{
    UndoableModelStatePair um;
    OpenSim::Model& model = um.updModel();

    auto& pof = AddModelComponent<OpenSim::PhysicalOffsetFrame>(model, model.getGround(), SimTK::Transform{SimTK::Vec3{0.0, 1.0, 0.0}});
    auto& body = AddBody(um.updModel(), "body", 1.0f, SimTK::Vec3{}, SimTK::Inertia(0.1));
    AddJoint<OpenSim::FreeJoint>(um.updModel(), "joint", pof, body);
    auto& path = AddModelComponent<OpenSim::GeometryPath>(model);
    path.appendNewPathPoint("p1_ground", model.getGround(), SimTK::Vec3{});
    path.appendNewPathPoint("p2_body", body, SimTK::Vec3{});
    auto& sphere = AddWrapObject<OpenSim::WrapSphere>(pof);
    sphere.set_radius(0.25);
    sphere.set_translation({0.001, -0.5, 0.0});  // prevent singularities
    FinalizeConnections(model);  // note: out of order because OpenSim seems to otherwise not notice the addition
    path.addPathWrap(sphere);
    InitializeModel(model);
    InitializeState(model);

    ASSERT_GT(path.getLength(um.getState()), 1.1)  << "initial state of model includes wrapping";

    ActionRemoveWrapObjectFromGeometryPathWraps(um, path, sphere);

    ASSERT_TRUE(equal_within_epsilon(path.getLength(um.getState()), 1.0))  << "should stop wrapping";
}
