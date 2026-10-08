#include "undoable_model_actions.h"

#include <libopynsim/documents/model/object_property_edit.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/open_sim_creator_app.h>
#include <libopensimcreator/tests/testopensimcreatorconfig.h>

#include <gtest/gtest.h>
#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/component_registry_entry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <libopynsim/opynsim.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/math_helpers.h>
#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Constant.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/StationDefinedFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/CoordinateCouplerConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>
#include <OpenSim/Simulation/Wrap/WrapSphere.h>

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
    model.upd_model().setInputFileName("doesnt-exist");

    // then it should just return `false`, rather than (e.g.) exploding
    ASSERT_FALSE(ActionUpdateModelFromBackingFile(model));
}

// repro for #654
//
// the bug is in OpenSim, but the action needs to hack around that bug until it is fixed
// upstream
TEST(OpenSimActions, ActionApplyRangeDeletionPropertyEditReturnsFalseToIndicateFailure)
{
    GloballyInitOpenSim();  // ensure component registry is populated

    // create undoable model with one body + joint
    auto undoableModel = []()
    {
        OpenSim::Model model;
        auto body = std::make_unique<OpenSim::Body>("body", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0});
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
        undoableModel.upd_model().updComponent<OpenSim::Coordinate>("/jointset/joint/rotation").updProperty_range(),
        [&](OpenSim::AbstractProperty& p)
        {
            p.clear();
        },
    };

    ASSERT_EQ(ActionApplyPropertyEdit(undoableModel, edit), false);

    // hacky extra test: you can remove this, it's just reminder code
    undoableModel.upd_model().updComponent<OpenSim::Coordinate>("/jointset/joint/rotation").updProperty_range().clear();
    ASSERT_ANY_THROW({ opyn::initialize_model(undoableModel.upd_model()); });
}

// high-level repro for (#773)
//
// the underlying bug appears to be related to finalizing connections in
// the model graph (grep for 773 to see other tests), but the user-reported
// bug is specifically related to renaming a component
TEST(OpenSimActions, ActionSetComponentNameOnModelWithUnusualJointTopologyDoesNotSegfault)
{
    const std::filesystem::path brokenFilePath =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "opensim-creator_773-2_repro.osim";

    const UndoableModelStatePair loadedModel{brokenFilePath};

    // loop `n` times because the segfault is stochastic
    //
    // ... which is a cute way of saying "really fucking unpredictable" :(
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
    const std::filesystem::path geomFile =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "arrow.vtp";

    UndoableModelStatePair model;
    auto& body = opyn::add_body(model.upd_model(), std::make_unique<OpenSim::Body>("name", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0}));
    body.setMass(1.0);
    auto& mesh = dynamic_cast<OpenSim::Mesh&>(opyn::attach_geometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    opyn::finalize_connections(model.upd_model());
    opyn::initialize_model(model.upd_model());
    opyn::initialize_state(model.upd_model());

    ActionFitSphereToMesh(model, mesh);
    ASSERT_TRUE(model.get_selected());
    ASSERT_TRUE(dynamic_cast<const OpenSim::Sphere*>(model.get_selected()));
    ASSERT_EQ(&dynamic_cast<const OpenSim::Sphere*>(model.get_selected())->getFrame().findBaseFrame(), &body.findBaseFrame());
}

TEST(OpenSimActions, ActionFitSphereToMeshAppliesMeshesScaleFactorsCorrectly)
{
    const std::filesystem::path geomFile =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "arrow.vtp";

    UndoableModelStatePair model;
    auto& body = opyn::add_body(model.upd_model(), std::make_unique<OpenSim::Body>("name", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0}));
    body.setMass(1.0);
    auto& unscaledMesh = dynamic_cast<OpenSim::Mesh&>(opyn::attach_geometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    auto& scaledMesh = dynamic_cast<OpenSim::Mesh&>(opyn::attach_geometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    const double scalar = 0.1;
    scaledMesh.set_scale_factors({scalar, scalar, scalar});

    opyn::finalize_connections(model.upd_model());
    opyn::initialize_model(model.upd_model());
    opyn::initialize_state(model.upd_model());

    ActionFitSphereToMesh(model, unscaledMesh);
    ASSERT_TRUE(dynamic_cast<const OpenSim::Sphere*>(model.get_selected()));
    const double unscaledRadius = dynamic_cast<const OpenSim::Sphere&>(*model.get_selected()).get_radius();
    ActionFitSphereToMesh(model, scaledMesh);
    ASSERT_TRUE(dynamic_cast<const OpenSim::Sphere*>(model.get_selected()));
    const double scaledRadius = dynamic_cast<const OpenSim::Sphere&>(*model.get_selected()).get_radius();

    ASSERT_TRUE(equal_within_reldiff(scaledRadius, scalar*unscaledRadius, 0.0001));
}

TEST(OpenSimActions, ActionAddParentOffsetFrameToJointWorksInNormalCase)
{
    UndoableModelStatePair um;
    auto& body = opyn::add_body(um.upd_model(), "bodyname", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0});
    auto& joint = opyn::add_joint<OpenSim::FreeJoint>(um.upd_model(), "jname", um.get_model().getGround(), body);

    // this should be ok
    opyn::finalize_connections(um.upd_model());
    opyn::initialize_model(um.upd_model());
    opyn::initialize_state(um.upd_model());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getParentFrame(), &um.get_model().getGround());

    // and now we ask for a new `PhysicalOffsetFrame` to be injected into the parent, which works
    ASSERT_TRUE(ActionAddParentOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's parent frame is now a `PhysicalOffsetFrame` that's attached to ground
    const OpenSim::PhysicalFrame& parent1 = joint.getParentFrame();
    ASSERT_NE(&parent1, &um.get_model().getGround());
    ASSERT_TRUE(dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&parent1));
    ASSERT_EQ(&dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(parent1).getParentFrame(), &um.get_model().getGround());
}

// ensure that the caller can keep asking to add parent offset frames to a joint - even if the
// joint is already attached to an offset frame
//
// - DISABLED because there's a bug in OpenSim that prevents this from working: https://github.com/opensim-org/opensim-core/pull/3711
TEST(OpenSimActions, ActionAddParentOffsetFrameToJointWorksInChainedCase)
{
    UndoableModelStatePair um;
    auto& body = opyn::add_body(um.upd_model(), "bodyname", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0});
    auto& joint = opyn::add_joint<OpenSim::FreeJoint>(um.upd_model(), "jname", um.get_model().getGround(), body);

    // this should be ok
    opyn::finalize_connections(um.upd_model());
    opyn::initialize_model(um.upd_model());
    opyn::initialize_state(um.upd_model());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getParentFrame(), &um.get_model().getGround());

    // and now we ask for a new PhysicalOffsetFrame to be injected into the parent, which should work
    ASSERT_TRUE(ActionAddParentOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's parent frame is now a `PhysicalOffsetFrame` that's attached to ground
    const OpenSim::PhysicalFrame& parent1 = joint.getParentFrame();
    ASSERT_NE(&parent1, &um.get_model().getGround());
    ASSERT_TRUE(dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&parent1));
    ASSERT_EQ(&dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(parent1).getParentFrame(), &um.get_model().getGround());

    // repeating the process creates a chain of `PhysicalOffsetFrame`s
    ASSERT_TRUE(ActionAddParentOffsetFrameToJoint(um, joint.getAbsolutePath()));

    const OpenSim::PhysicalFrame& parent2 = joint.getParentFrame();
    ASSERT_NE(&parent1, &parent2);
    ASSERT_TRUE(dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&parent2));
    ASSERT_EQ(&dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(parent2).getParentFrame(), &parent1);
}

TEST(OpenSimActions, ActionAddChildOffsetFrameToJointWorksInNormalCase)
{
    UndoableModelStatePair um;
    auto& body = opyn::add_body(um.upd_model(), "bodyname", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0});
    auto& joint = opyn::add_joint<OpenSim::FreeJoint>(um.upd_model(), "jname", um.get_model().getGround(), body);

    // this should be ok
    opyn::finalize_connections(um.upd_model());
    opyn::initialize_model(um.upd_model());
    opyn::initialize_state(um.upd_model());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getChildFrame(), &body);

    // and now we ask for a new `PhysicalOffsetFrame` to be injected into the child, which should work
    ASSERT_TRUE(ActionAddChildOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's child frame is now a `PhysicalOffsetFrame` that's attached to the body
    const OpenSim::PhysicalFrame& child1 = joint.getChildFrame();
    ASSERT_NE(&child1, &body);
    ASSERT_TRUE(dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&child1));
    ASSERT_EQ(&dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(child1).getParentFrame(), &body);
}

// ensure that the caller can keep asking to add child offset frames to a joint - even if the
// joint is already attached to an offset frame
//
// - DISABLED because there's a bug in OpenSim that prevents this from working: https://github.com/opensim-org/opensim-core/pull/3711
TEST(OpenSimActions, ActionAddChildOffsetFrameToJointWorksInChainedCase)
{
    UndoableModelStatePair um;
    auto& body = opyn::add_body(um.upd_model(), "bodyname", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{1.0});
    auto& joint = opyn::add_joint<OpenSim::FreeJoint>(um.upd_model(), "jname", um.get_model().getGround(), body);

    // this should be ok
    opyn::finalize_connections(um.upd_model());
    opyn::initialize_model(um.upd_model());
    opyn::initialize_state(um.upd_model());

    // the joint is initially directly attached to ground
    ASSERT_EQ(&joint.getChildFrame(), &body);

    // and now we ask for a new `PhysicalOffsetFrame` to be injected into the child, which should work
    ASSERT_TRUE(ActionAddChildOffsetFrameToJoint(um, joint.getAbsolutePath()));

    // the joint's child frame is now a `PhysicalOffsetFrame` that's attached to the body
    const OpenSim::PhysicalFrame& child1 = joint.getChildFrame();
    ASSERT_NE(&child1, &body);
    ASSERT_TRUE(dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&child1));
    ASSERT_EQ(&dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(child1).getParentFrame(), &body);

    // repeating the process creates a chain of `PhysicalOffsetFrame`s
    ASSERT_TRUE(ActionAddChildOffsetFrameToJoint(um, joint.getAbsolutePath()));

    const OpenSim::PhysicalFrame& child2 = joint.getChildFrame();
    ASSERT_NE(&child2, &child1);
    ASSERT_TRUE(dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&child2));
    ASSERT_EQ(&dynamic_cast<const OpenSim::PhysicalOffsetFrame&>(child2).getParentFrame(), &child1);
}

TEST(OpenSimActions, ActionAddWrapObjectToPhysicalFrameCanAddWrapCylinderToGround)
{
    // test one concrete instance (this test isn't coupled to the component registry)
    UndoableModelStatePair um;
    auto wrapCylinder = std::make_unique<OpenSim::WrapCylinder>();
    wrapCylinder->setName("should_be_findable_in_model");

    ASSERT_TRUE(ActionAddWrapObjectToPhysicalFrame(um, um.get_model().getGround().getAbsolutePath(), std::move(wrapCylinder)));
    ASSERT_TRUE(um.get_model().findComponent("should_be_findable_in_model"));
}

TEST(OpenSimActions, ActionAddWrapObjectToPhysicalFrameCanAddAllRegisteredWrapObjectsToGround)
{
    UndoableModelStatePair um;
    const OpenSim::ComponentPath groundPath = um.get_model().getGround().getAbsolutePath();

    for (const auto& entry : opyn::get_component_registry<OpenSim::WrapObject>()) {
        ASSERT_TRUE(ActionAddWrapObjectToPhysicalFrame(um, groundPath, entry.instantiate()));
    }

    size_t numWrapsInModel = 0;
    for ([[maybe_unused]] const auto& wrap : um.get_model().getComponentList<OpenSim::WrapObject>()) {
        ++numWrapsInModel;
    }

    ASSERT_EQ(numWrapsInModel, opyn::get_component_registry<OpenSim::WrapObject>().size());
}

TEST(OpenSimActions, ActionAddPathWrapToGeometryPathWorksInExampleCase)
{
    UndoableModelStatePair um;
    OpenSim::Model& model = um.upd_model();

    auto& pof = opyn::add_model_component<OpenSim::PhysicalOffsetFrame>(model, model.getGround(), SimTK::Transform{SimTK::Vec3{0.0, 1.0, 0.0}});
    auto& body = opyn::add_body(model, "body", 1.0f, SimTK::Vec3{0.0}, SimTK::Inertia(0.1));
    opyn::add_joint<OpenSim::FreeJoint>(model, "joint", pof, body);
    auto& path = opyn::add_model_component<OpenSim::GeometryPath>(model);
    path.appendNewPathPoint("p1_ground", model.getGround(), SimTK::Vec3{0.0});
    path.appendNewPathPoint("p2_body", body, SimTK::Vec3{0.0});

    opyn::finalize_connections(model);
    opyn::initialize_model(model);
    const auto& state = opyn::initialize_state(model);

    ASSERT_NEAR(path.getLength(state), 1.0, epsilon_v<double>) << "an uninterrupted path should have this length";

    auto& sphere = opyn::add_wrap_object<OpenSim::WrapSphere>(pof);
    sphere.set_radius(0.25);
    sphere.set_translation({0.001, -0.5, 0.0});  // prevent singularities

    opyn::finalize_connections(model);
    opyn::initialize_model(model);
    const auto& state2 = opyn::initialize_state(model);

    ASSERT_NEAR(path.getLength(state2), 1.0, epsilon_v<double>) << "the wrap object hasn't been added to the model yet";

    ActionAddWrapObjectToGeometryPathWraps(um, path, sphere);

    ASSERT_GT(path.getLength(um.get_state()), 1.1)  << "path should start wrapping";
}

TEST(OpenSimActions, ActionRemoveWrapObjectFromGeometryPathWrapsWorksInExampleCase)
{
    UndoableModelStatePair um;
    OpenSim::Model& model = um.upd_model();

    auto& pof = opyn::add_model_component<OpenSim::PhysicalOffsetFrame>(model, model.getGround(), SimTK::Transform{SimTK::Vec3{0.0, 1.0, 0.0}});
    auto& body = opyn::add_body(model, "body", 1.0f, SimTK::Vec3{0.0}, SimTK::Inertia(0.1));
    opyn::add_joint<OpenSim::FreeJoint>(model, "joint", pof, body);
    auto& path = opyn::add_model_component<OpenSim::GeometryPath>(model);
    path.appendNewPathPoint("p1_ground", model.getGround(), SimTK::Vec3{0.0});
    path.appendNewPathPoint("p2_body", body, SimTK::Vec3{0.0});
    auto& sphere = opyn::add_wrap_object<OpenSim::WrapSphere>(pof);
    sphere.set_radius(0.25);
    sphere.set_translation({0.001, -0.5, 0.0});  // prevent singularities
    opyn::finalize_connections(model);  // note: out of order because OpenSim seems to otherwise not notice the addition
    path.addPathWrap(sphere);
    opyn::initialize_model(model);
    opyn::initialize_state(model);

    ASSERT_GT(path.getLength(um.get_state()), 1.1)  << "initial state of model includes wrapping";

    ActionRemoveWrapObjectFromGeometryPathWraps(um, path, sphere);

    ASSERT_NEAR(path.getLength(um.get_state()), 1.0, epsilon_v<double>)  << "should stop wrapping";
}

// related issue: #890
//
// when a model is hot-reloaded from disk, the scene scale factor should be retained from
// the in-editor model, to support the user changing it to a non-default value while they
// seperately edit the underlying model file
TEST(OpenSimActions, ActionUpdateModelFromBackingFileShouldRetainSceneScaleFactor)
{
    const std::filesystem::path backingFile = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "Blank" / "blank.osim";

    UndoableModelStatePair model{backingFile};
    model.set_up_to_date_with_filesystem(model.getLastFilesystemWriteTime() - std::chrono::seconds{1});  // ensure it's invalid

    ASSERT_TRUE(opyn::has_input_file_name(model.get_model()));

    // set the scale factor to a nonstandard value
    ASSERT_NE(model.get_fixup_scale_factor(), 0.5f);
    model.set_fixup_scale_factor(0.5f);
    ASSERT_EQ(model.get_fixup_scale_factor(), 0.5f);

    // reload the model from disk
    ASSERT_TRUE(ActionUpdateModelFromBackingFile(model)) << "this should work fine";

    ASSERT_EQ(model.get_fixup_scale_factor(), 0.5f) << "the scene scale factor should be retained after a reload";
}

// related issue: #887
//
// the user wanted this toggle in the UI. At time of writing, it's really only used for `SmoothSphereHalfSpaceForce`
TEST(OpenSimActions, ActionToggleForcesTogglesTheForces)
{
    UndoableModelStatePair model;
    ASSERT_TRUE(opyn::is_showing_forces(model.get_model()));
    ActionToggleForces(model);
    ASSERT_FALSE(opyn::is_showing_forces(model.get_model()));
    model.doUndo();
    ASSERT_TRUE(opyn::is_showing_forces(model.get_model()));
}

// related issue: #957
//
// This is a very basic test to ensure that the zeroing functionality works in the most trivial case.
TEST(OpenSimActions, ActionZeroAllCoordinatesZeroesAllCoordinatesInAModel)
{
    UndoableModelStatePair model;
    auto& body = opyn::add_body(model.upd_model(), "somebody", 1.0, SimTK::Vec3(0.0), SimTK::Inertia{1.0});
    auto& fj = opyn::add_joint<OpenSim::FreeJoint>(model.upd_model(), "somejoint", model->getGround(), body);
    model.upd_model().finalizeFromProperties();
    model.upd_model().finalizeConnections();
    fj.updCoordinate(OpenSim::FreeJoint::Coord::TranslationY).set_default_value(1.0);

    ASSERT_EQ(fj.getCoordinate(OpenSim::FreeJoint::Coord::TranslationY).get_default_value(), 1.0);
    ActionZeroAllCoordinates(model);
    ASSERT_EQ(fj.getCoordinate(OpenSim::FreeJoint::Coord::TranslationY).get_default_value(), 0.0);
}

// Test `ActionBakeStationDefinedFrames` feature (#1004) simple usage pattern.
TEST(OpenSimActions, ActionBakeStationDefinedFramesWorksInTrivialCase)
{
    // Build basic model with SDF
    UndoableModelStatePair model;
    auto& a = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{1.0, 0.0, 0.0});
    auto& b = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 1.0, 0.0});
    auto& c = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 0.0, 0.0});
    auto& sdf = opyn::add_model_component<OpenSim::StationDefinedFrame>(
        model.upd_model(),
        "sdf",
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::XCoordinateAxis{}},
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::ZCoordinateAxis{}},
        a,
        b,
        c,
        c
    );

    // Finalize/initialize the model
    model.upd_model().buildSystem();
    const SimTK::State& state = model.upd_model().initializeState();
    const SimTK::Transform sdfTransform = sdf.getTransformInGround(state);

    // Ensure `StationDefinedFrame` is actually transforming
    ASSERT_NE(sdfTransform, SimTK::Transform{}) << "The `StationDefinedFrame` should have a non-identity transform";

    // Ensure counts are within expectations before baking
    {
        auto lst = model->getComponentList<OpenSim::StationDefinedFrame>();
        ASSERT_EQ(std::distance(lst.begin(), lst.end()), 1) << "The model should only contain one `StationDefinedFrame`";
        auto lst_2 = model->getComponentList<OpenSim::PhysicalOffsetFrame>();
        ASSERT_EQ(std::distance(lst_2.begin(), lst_2.end()), 0) << "The model should contain no `PhysicalOffsetFrame`s (hasn't been baked yet)";
    }

    // Bake the frames
    ActionBakeStationDefinedFrames(model);

    // Ensure counts are within expectations after baking
    {
        auto lst = model->getComponentList<OpenSim::StationDefinedFrame>();
        ASSERT_EQ(std::distance(lst.begin(), lst.end()), 0) << "The model shouldn't contain any `StationDefinedFrame`s (baking should delete them)";
        auto lst_2 = model->getComponentList<OpenSim::PhysicalOffsetFrame>();
        ASSERT_EQ(std::distance(lst_2.begin(), lst_2.end()), 1) << "The model should contain one `PhysicalOffsetFrame` (from baking)";
    }

    // Ensure transform after baking is equivalent to `StationDefinedFrame`'s original transform
    const SimTK::State& stateAfter = model.upd_model().initializeState();
    const SimTK::Transform offsetTransform = model->getComponentList<OpenSim::PhysicalOffsetFrame>().begin()->getTransformInGround(stateAfter);

    ASSERT_EQ(offsetTransform.p(), sdfTransform.p());
    ASSERT_EQ(offsetTransform.R().convertRotationToBodyFixedXYZ(), sdfTransform.R().convertRotationToBodyFixedXYZ());
}

// Test `ActionBakeStationDefinedFrames` feature (#1004) also copies any attached geometry.
TEST(OpenSimActions, ActionBakeStationDefinedFramesCopiesAttachedGeometry)
{
    // Build basic model with SDF
    UndoableModelStatePair model;
    auto& a = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{1.0, 0.0, 0.0});
    auto& b = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 1.0, 0.0});
    auto& c = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 0.0, 0.0});
    auto& sdf = opyn::add_model_component<OpenSim::StationDefinedFrame>(
        model.upd_model(),
        "sdf",
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::XCoordinateAxis{}},
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::ZCoordinateAxis{}},
        a,
        b,
        c,
        c
    );

    // Attach geometry to the SDF
    sdf.attachGeometry(std::make_unique<OpenSim::Sphere>(2.5).release());
    sdf.attachGeometry(std::make_unique<OpenSim::Ellipsoid>(1.0, 2.0, 3.0).release());

    // Finalize/initialize the model
    model.upd_model().buildSystem();

    // The SDF should still have the geometry attached
    ASSERT_EQ(sdf.getProperty_attached_geometry().size(), 2);

    // Bake the frames
    ActionBakeStationDefinedFrames(model);

    // Find the baked `PhysicalOffsetFrame`
    auto pofs = model->getComponentList<OpenSim::PhysicalOffsetFrame>();
    ASSERT_EQ(std::distance(pofs.begin(), pofs.end()), 1);
    const OpenSim::PhysicalOffsetFrame& pof = *pofs.begin();

    // Ensure the baked frame also includes the attached geometry.
    ASSERT_EQ(pof.getProperty_attached_geometry().size(), 2);

    // Check first attached geometry.
    {
        const auto& sphere = dynamic_cast<const OpenSim::Sphere&>(pof.get_attached_geometry(0));
        ASSERT_EQ(sphere.get_radius(), 2.5);
    }

    // Check second attached geometry.
    {
        const auto& ellipsoid = dynamic_cast<const OpenSim::Ellipsoid&>(pof.get_attached_geometry(1));
        ASSERT_EQ(ellipsoid.get_radii(), SimTK::Vec3(1.0, 2.0, 3.0));
    }
}

// Test `ActionBakeStationDefinedFrames` feature (#1004) also copies any associated wrap objects.
TEST(OpenSimActions, ActionBakeStationDefinedFramesCopiesWrapObjects)
{
    // Build basic model with SDF
    UndoableModelStatePair model;
    auto& a = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{1.0, 0.0, 0.0});
    auto& b = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 1.0, 0.0});
    auto& c = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 0.0, 0.0});
    auto& sdf = opyn::add_model_component<OpenSim::StationDefinedFrame>(
        model.upd_model(),
        "sdf",
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::XCoordinateAxis{}},
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::ZCoordinateAxis{}},
        a,
        b,
        c,
        c
    );

    // Add `OpenSim::WrapObject`s to the SDF
    sdf.addWrapObject(std::make_unique<OpenSim::WrapSphere>().release());
    sdf.addWrapObject(std::make_unique<OpenSim::WrapCylinder>().release());

    // Finalize/initialize the model
    model.upd_model().buildSystem();

    // The SDF should still have the wrap objects attached to it
    ASSERT_EQ(sdf.getWrapObjectSet().getSize(), 2);

    // Bake the SDF
    ActionBakeStationDefinedFrames(model);

    // Find the baked `PhysicalOffsetFrame`
    auto pofs = model->getComponentList<OpenSim::PhysicalOffsetFrame>();
    ASSERT_EQ(std::distance(pofs.begin(), pofs.end()), 1);
    const OpenSim::PhysicalOffsetFrame& pof = *pofs.begin();

    // Ensure the baked frame also includes the wrap objects.
    ASSERT_EQ(pof.getWrapObjectSet().getSize(), 2);

    // Check first wrap object.
    ASSERT_NE(dynamic_cast<const OpenSim::WrapSphere*>(&pof.getWrapObjectSet()[0]), nullptr);

    // Check second wrap object.
    ASSERT_NE(dynamic_cast<const OpenSim::WrapCylinder*>(&pof.getWrapObjectSet()[1]), nullptr);
}

// Test `ActionBakeStationDefinedFrames` feature (#1004) also copies any subcomponents.
TEST(OpenSimActions, ActionBakeStationDefinedFramesCopiesSubcomponents)
{
    // Build basic model with SDF
    UndoableModelStatePair model;
    auto& a = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{1.0, 0.0, 0.0});
    auto& b = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 1.0, 0.0});
    auto& c = opyn::add_model_component<OpenSim::Station>(model.upd_model(), model.get_model().getGround(), SimTK::Vec3{0.0, 0.0, 0.0});
    auto& sdf = opyn::add_model_component<OpenSim::StationDefinedFrame>(
        model.upd_model(),
        "sdf",
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::XCoordinateAxis{}},
        SimTK::CoordinateDirection{SimTK::CoordinateAxis::ZCoordinateAxis{}},
        a,
        b,
        c,
        c
    );

    // Add subcomponents to the SDF's component list (in this case, make it extra
    // hard by making them dependent on the SDF ;)).
    sdf.addComponent(std::make_unique<OpenSim::Marker>("marker1", sdf, SimTK::Vec3(1.0, 0.0, 0.0)).release());
    sdf.addComponent(std::make_unique<OpenSim::Marker>("marker2", sdf, SimTK::Vec3(0.0, 1.0, 0.0)).release());

    // Add wrap objects to the SDF's component list
    auto wrapCylinder = std::make_unique<OpenSim::WrapCylinder>();
    wrapCylinder->setName("magical_wrap_cylinder");
    wrapCylinder->set_radius(5.0);
    sdf.addWrapObject(wrapCylinder.release());

    // Finalize/initialize the model
    model.upd_model().buildSystem();

    // The SDF should contain the subcomponents
    {
        auto lst = sdf.getComponentList<OpenSim::Marker>();
        ASSERT_EQ(std::distance(lst.begin(), lst.end()), 2);
    }

    // Bake the SDF
    ActionBakeStationDefinedFrames(model);

    // Find the baked `PhysicalOffsetFrame`
    auto pofs = model->getComponentList<OpenSim::PhysicalOffsetFrame>();
    ASSERT_EQ(std::distance(pofs.begin(), pofs.end()), 1);
    const OpenSim::PhysicalOffsetFrame& pof = *pofs.begin();

    // The baked SDF should contain the subcomponents
    {
        auto lst = pof.getComponentList<OpenSim::Marker>();
        ASSERT_EQ(std::distance(lst.begin(), lst.end()), 2);
    }

    // The baked SDF should contain wrap objects
    const OpenSim::WrapObject* wo = pof.getWrapObject("magical_wrap_cylinder");
    ASSERT_NE(wo, nullptr);
    const auto* cylinder = dynamic_cast<const OpenSim::WrapCylinder*>(wo);
    ASSERT_NE(cylinder, nullptr);
    ASSERT_EQ(cylinder->get_radius(), 5.0);
}

TEST(OpenSimActions, ActionAddPathPointToGeometryPathWorksAsExpected)
{
    UndoableModelStatePair model;
    auto& gp = opyn::add_model_component<OpenSim::GeometryPath>(model.upd_model());
    gp.appendNewPathPoint("p1", model.get_model().getGround(), SimTK::Vec3{0.0, 0.0, 0.0});
    gp.appendNewPathPoint("p2", model.get_model().getGround(), SimTK::Vec3{1.0, 0.0, 0.0});
    opyn::finalize_connections(model.upd_model());
    opyn::initialize_model(model.upd_model());
    opyn::initialize_state(model.upd_model());
    const auto gpPath = gp.getAbsolutePath();
    const auto groundPath = model.get_model().getGround().getAbsolutePath();

    ASSERT_TRUE(ActionAddPathPointToGeometryPath(model, gpPath, groundPath)) << "should work";
    ASSERT_EQ(gp.getPathPointSet().getSize(), 3);
}

TEST(OpenSimActions, ActionMoveMarkerToModelMarkerSet_MovesMarker)
{
    // Create a model with a marker defined somewhere else in the
    // model (using generic `ComponentSet`s):
    //
    //  model
    //    |
    // bodyset
    //    |
    //  body
    //    |
    //   pof
    //    |
    //  marker
    //
    // And then move it into the `markerset` and ensure it's still attached to the
    // original pof, with the correct location, etc.

    UndoableModelStatePair model;
    auto& body = opyn::add_body(model.upd_model(), "body", 1.0,  SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const SimTK::Vec3 pofOffset{0.25};
    auto& pof = opyn::add_component<OpenSim::PhysicalOffsetFrame>(body, "pof", body, SimTK::Transform{pofOffset});
    const SimTK::Vec3 markerOffset{0.3};
    auto& marker = opyn::add_component<OpenSim::Marker>(pof, "marker", pof, markerOffset);
    opyn::finalize_connections(model.upd_model());
    opyn::initialize_model(model.upd_model());
    const SimTK::State& state = opyn::initialize_state(model.upd_model());

    ASSERT_EQ(marker.getLocationInGround(state), pofOffset + markerOffset);
    ASSERT_EQ(&marker.getParentFrame(), &pof);
    ASSERT_EQ(&marker.getOwner(), &pof);

    ActionMoveMarkerToModelMarkerSet(model, marker);

    ASSERT_EQ(marker.getLocationInGround(state), pofOffset + markerOffset);
    ASSERT_EQ(&marker.getParentFrame(), &pof);
    ASSERT_EQ(&marker.getOwner(), &model.get_model().getMarkerSet());
}

TEST(OpenSimActions, ActionSetCoordinateLockedAndSave_EnsuresValueCanBeEditedAfterwards)
{
    // Regression test for issue #1164
    //
    // The issue was that unlocking a coordinate in the UI would put the model
    // in a state where the user couldn't subsequently edit the value of the
    // `OpenSim::Coordinate`. The reason it happened was because the coordinate
    // value setter was trying to satisfy constraints, but the underlying assembly
    // solver in the model wasn't updated with the lock change.

    opyn::init();

    UndoableModelStatePair model;
    auto& body = opyn::add_body(model.upd_model(), "head", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    auto& pin = opyn::add_joint<OpenSim::BallJoint>(
        model.upd_model(),
        "ball",
        model.upd_model().getGround(),
        SimTK::Vec3{0.0, 1.0, 0.0},
        SimTK::Vec3{0.0},
        body,
        SimTK::Vec3{0.0, -1.0, 0.0},
        SimTK::Vec3{0.0}
    );
    auto& coordinate = pin.updCoordinate(OpenSim::BallJoint::Coord::Rotation1X);
    coordinate.setDefaultLocked(true);  // ensure X is initially locked

    // Extra concern: the model must contain a constraint
    //
    // The #1164 bug only manifests when the model contains a constraint. The way
    // the bug manifests is that, when setting a coordinate value, the constraint
    // is detected and then the model assembler is used to track the coordinates
    // after the change to re-enforce the constraint - it's the assembler that's
    // in an invalid state (it thinks the coordinate is still locked).
    auto& constraint = opyn::add_constraint<OpenSim::CoordinateCouplerConstraint>(model.upd_model());
    constraint.setFunction(OpenSim::Constant(0.0));
    constraint.setDependentCoordinateName(pin.getCoordinate(OpenSim::BallJoint::Coord::Rotation2Y).getName());

    opyn::finalize_connections(model.upd_model());
    opyn::initialize_model(model.upd_model());
    SimTK::State& state = opyn::initialize_state(model.upd_model());

    ASSERT_TRUE(coordinate.getLocked(state));
    ActionSetCoordinateLockedAndSave(model, coordinate, false);  // Unlock coordinate using UI action
    ASSERT_FALSE(coordinate.getLocked(state));

    const double originalValue = coordinate.getValue(state);
    const double newValue = originalValue + 1.1;

    // This is what moving the slider, but not letting go, does.
    ActionSetCoordinateValue(model, coordinate, newValue);

    ASSERT_EQ(coordinate.getValue(state), newValue) << "This should be updated, if it isn't the unlocking might be causing issues?";
}
