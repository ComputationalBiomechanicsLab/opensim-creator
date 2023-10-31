#include <OpenSimCreator/Utils/UndoableModelActions.hpp>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSimCreator/Model/ObjectPropertyEdit.hpp>
#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <oscar/Maths/MathHelpers.hpp>

#include <functional>
#include <memory>

// repro for #642
//
// @AdrianHendrik reported that trying to add a body with an invalid name entirely crashes
// OSC, which implies that the operation causes a segfault
TEST(OpenSimActions, ActionAddBodyToModelThrowsIfBodyNameIsInvalid)
{
    osc::UndoableModelStatePair model;

    osc::BodyDetails details;
    details.bodyName = "test 1";
    details.parentFrameAbsPath = "/ground";  // this is what the dialog defaults to

    ASSERT_ANY_THROW({ osc::ActionAddBodyToModel(model, details); });
}

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(OpenSimActions, ActionUpdateModelFromBackingFileReturnsFalseIfFileDoesNotExist)
{
    osc::UndoableModelStatePair model;

    // it just returns `false` if there's no backing file
    ASSERT_FALSE(osc::ActionUpdateModelFromBackingFile(model));

    // ... but if you say it has an invalid backing file path...
    model.setFilesystemPath("doesnt-exist");

    // then it should just return `false`, rather than (e.g.) exploding
    ASSERT_FALSE(osc::ActionUpdateModelFromBackingFile(model));
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
        return osc::UndoableModelStatePair{std::make_unique<OpenSim::Model>(std::move(model))};
    }();

    osc::ObjectPropertyEdit edit
    {
        undoableModel.updModel().updComponent<OpenSim::Coordinate>("/jointset/joint/rotation").updProperty_range(),
        [&](OpenSim::AbstractProperty& p)
        {
            p.clear();
        },
    };

    ASSERT_EQ(osc::ActionApplyPropertyEdit(undoableModel, edit), false);

    // hacky extra test: you can remove this, it's just reminder code
    undoableModel.updModel().updComponent<OpenSim::Coordinate>("/jointset/joint/rotation").updProperty_range().clear();
    ASSERT_ANY_THROW({ osc::InitializeModel(undoableModel.updModel()); });
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

    osc::UndoableModelStatePair const loadedModel{brokenFilePath};

    // loop `n` times because the segfault is stochastic
    //
    // ... which is a cute way of saying "really fucking random" :(
    for (size_t i = 0; i < 25; ++i)
    {
        osc::UndoableModelStatePair model = loadedModel;
        osc::ActionSetComponentName(
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

    osc::UndoableModelStatePair model;
    auto& body = osc::AddBody(model.updModel(), std::make_unique<OpenSim::Body>("name", 1.0, SimTK::Vec3{}, SimTK::Inertia{{}, {}}));
    body.setMass(1.0);
    auto& mesh = dynamic_cast<OpenSim::Mesh&>(osc::AttachGeometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    osc::FinalizeConnections(model.updModel());
    osc::InitializeModel(model.updModel());
    osc::InitializeState(model.updModel());

    osc::ActionFitSphereToMesh(model, mesh);
    ASSERT_TRUE(model.getSelected());
    ASSERT_TRUE(dynamic_cast<OpenSim::Sphere const*>(model.getSelected()));
    ASSERT_EQ(&dynamic_cast<OpenSim::Sphere const*>(model.getSelected())->getFrame().findBaseFrame(), &body.findBaseFrame());
}

TEST(OpenSimActions, ActionFitSphereToMeshAppliesMeshesScaleFactorsCorrectly)
{
    std::filesystem::path const geomFile =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "TestOpenSimCreator" / "arrow.vtp";

    osc::UndoableModelStatePair model;
    auto& body = osc::AddBody(model.updModel(), std::make_unique<OpenSim::Body>("name", 1.0, SimTK::Vec3{}, SimTK::Inertia{{}, {}}));
    body.setMass(1.0);
    auto& unscaledMesh = dynamic_cast<OpenSim::Mesh&>(osc::AttachGeometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    auto& scaledMesh = dynamic_cast<OpenSim::Mesh&>(osc::AttachGeometry(body, std::make_unique<OpenSim::Mesh>(geomFile.string())));
    double const scalar = 0.1;
    scaledMesh.set_scale_factors({scalar, scalar, scalar});

    osc::FinalizeConnections(model.updModel());
    osc::InitializeModel(model.updModel());
    osc::InitializeState(model.updModel());

    osc::ActionFitSphereToMesh(model, unscaledMesh);
    ASSERT_TRUE(dynamic_cast<OpenSim::Sphere const*>(model.getSelected()));
    double const unscaledRadius = dynamic_cast<OpenSim::Sphere const&>(*model.getSelected()).get_radius();
    osc::ActionFitSphereToMesh(model, scaledMesh);
    ASSERT_TRUE(dynamic_cast<OpenSim::Sphere const*>(model.getSelected()));
    double const scaledRadius = dynamic_cast<OpenSim::Sphere const&>(*model.getSelected()).get_radius();

    ASSERT_TRUE(osc::IsEqualWithinRelativeError(scaledRadius, scalar*unscaledRadius, 0.0001));
}
