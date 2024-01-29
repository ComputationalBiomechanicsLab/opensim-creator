#include "OpenSimCreator/Documents/FrameDefinition/StationDefinedFrame.hpp"

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <Simbody.h>

using osc::fd::StationDefinedFrame;
using osc::AddBody;
using osc::AddJoint;
using osc::AddModelComponent;
using osc::FinalizeConnections;
using osc::InitializeModel;
using osc::InitializeState;

TEST(StationDefinedFrame, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW({ StationDefinedFrame{}; });
}

TEST(StationDefinedFrame, CanCreateAModelContainingAStandaloneStationDefinedFrame)
{
    OpenSim::Model model;

    // add stations to the model
    auto& p1 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{-1.0, -1.0, 0.0});
    auto& p2 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{-1.0,  1.0, 0.0});
    auto& p3 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{ 1.0,  0.0, 0.0});
    auto& origin = p1;

    // add a `StationDefinedFrame` that uses the stations to the model
    AddModelComponent<StationDefinedFrame>(
        model,
        SimTK::CoordinateAxis::XCoordinateAxis(),
        SimTK::CoordinateAxis::YCoordinateAxis(),
        p1,
        p2,
        p3,
        origin
    );

    // the model should initialize etc. fine
    FinalizeConnections(model);
    InitializeModel(model);
    InitializeState(model);
}

TEST(StationDefinedFrame, CanCreateAModelContainingAStationDefinedFrameAsAChild)
{
    OpenSim::Model model;

    // add stations to the model
    auto& p1 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{-1.0, -1.0, 0.0});
    auto& p2 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{-1.0,  1.0, 0.0});
    auto& p3 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{ 1.0,  0.0, 0.0});
    auto& origin = p1;

    // add a `StationDefinedFrame` that uses the stations to the model
    auto& sdf = AddModelComponent<StationDefinedFrame>(
        model,
        SimTK::CoordinateAxis::XCoordinateAxis(),
        SimTK::CoordinateAxis::YCoordinateAxis(),
        p1,
        p2,
        p3,
        origin
    );

    // add a `Body` that will act as the child of a `Joint`
    auto& body = AddBody(model, "name", 1.0, SimTK::Vec3{0.0, 0.0, 0.0}, SimTK::Inertia{1.0, 1.0, 1.0});

    // add a `Joint` between the `StationDefinedFrame` and the `Body`
    AddJoint<OpenSim::WeldJoint>(model, "weld", sdf, body);

    // the model should initialize etc. fine
    FinalizeConnections(model);
    InitializeModel(model);
    InitializeState(model);
}

TEST(StationDefinedFrame, CanCreateModelContainingStationDefinedFrameAsParentOfOffsetFrame)
{
    OpenSim::Model model;

    // add stations to the model
    auto& p1 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{-1.0, -1.0, 0.0});
    auto& p2 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{-1.0,  1.0, 0.0});
    auto& p3 = AddModelComponent<OpenSim::Station>(model, model.getGround(), SimTK::Vec3{ 1.0,  0.0, 0.0});
    auto& origin = p1;

    // add a `StationDefinedFrame` that uses the stations to the model
    auto& sdf = AddModelComponent<StationDefinedFrame>(
        model,
        SimTK::CoordinateAxis::XCoordinateAxis(),
        SimTK::CoordinateAxis::YCoordinateAxis(),
        p1,
        p2,
        p3,
        origin
    );

    AddModelComponent<OpenSim::PhysicalOffsetFrame>(model, sdf, SimTK::Transform{});

    // the model should initialize etc. fine
    FinalizeConnections(model);
    InitializeModel(model);  // TODO: broken by OpenSim/Simulation/Model/Model.cpp:958: only considers PoFs for finalization order
    InitializeState(model);
}
