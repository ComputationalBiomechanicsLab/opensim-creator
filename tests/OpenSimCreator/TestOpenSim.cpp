#include "OpenSimCreator/ActionFunctions.hpp"
#include "OpenSimCreator/OpenSimApp.hpp"
#include "OpenSimCreator/OpenSimHelpers.hpp"
#include "OpenSimCreator/UndoableModelStatePair.hpp"
#include "testopensimcreator_config.hpp"

#include <oscar/Platform/Config.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>

#include <filesystem>


// this is a repro for
//
// https://github.com/opensim-org/opensim-core/issues/3211
TEST(OpenSimModel, ProducesCorrectMomentArmOnFirstComputeCall)
{
	auto config = osc::Config::load();
	//osc::GlobalInitOpenSim(*config);  // ensure muscles are available etc.

	// data sources
	std::filesystem::path modelPath{config->getResourceDir() / "models" / "Arm26" / "arm26.osim"};
    OpenSim::ComponentPath coordinatePath{"/jointset/r_shoulder/r_shoulder_elev"};
    OpenSim::ComponentPath musclePath{"/forceset/BIClong"};

	// load osim into a base copy of the model
	OpenSim::Model baseModel{modelPath.string()};
	baseModel.buildSystem();
	baseModel.initializeState();
	baseModel.equilibrateMuscles(baseModel.updWorkingState());

	// copy-construct the model that's actually simulated
	OpenSim::Model model{baseModel};
	model.buildSystem();
	model.initializeState();
	model.updWorkingState() = baseModel.getWorkingState();  // is this technically illegal?

	// take a local copy of the state
	SimTK::State st = model.getWorkingState();

	// lookup components
	OpenSim::Coordinate const& coord = model.getComponent<OpenSim::Coordinate>(coordinatePath);
	OpenSim::Muscle const& musc = model.getComponent<OpenSim::Muscle>(musclePath);

	// setting `fixBug` to `true` makes this test pass
	if (bool fixBug = true)
	{
		musc.getGeometryPath().computeMomentArm(st, coord);
	}

	// compute two moment arms at one particular coordinate value
	coord.setLocked(st, false);
	double values[2];
	double newCoordVal = coord.getValue(st) + 0.01;  // just ensure the coord changes from default
	coord.setValue(st, newCoordVal);
	for (int i = 0; i < 2; ++i)
	{
		st.invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
		model.equilibrateMuscles(st);
		model.realizeDynamics(st);
		values[i] = musc.getGeometryPath().computeMomentArm(st, coord);
	}

	ASSERT_EQ(values[0], values[1]);
}

// repro for a bug found in OpenSim Creator
//
// effectively, `OpenSim::Coordinate::setLocked(SimTK::State&) const` is mutating the
// cooordinate/model (it shouldn't), because the internals rely on bad aliasing
//
// this test just double-checks that the bug exists until an upstream thing fixes it,
// breaks this test, and prompts removing fixups from OSC
TEST(OpenSimModel, EditingACoordinateLockMutatesModel)
{
	auto config = osc::Config::load();
	//osc::GlobalInitOpenSim(*config);  // ensure muscles are available etc.

	std::filesystem::path modelPath{config->getResourceDir() / "models" / "Arm26" / "arm26.osim"};
    OpenSim::ComponentPath coordinatePath{"/jointset/r_shoulder/r_shoulder_elev"};

	OpenSim::Model model{modelPath.string()};
	model.buildSystem();
	model.initializeState();
	model.equilibrateMuscles(model.updWorkingState());
	model.realizeReport(model.updWorkingState());

	OpenSim::Coordinate const& coord = model.getComponent<OpenSim::Coordinate>(coordinatePath);
	SimTK::State state = model.updWorkingState();

	ASSERT_TRUE(model.getWorkingState().isConsistent(state));
	ASSERT_FALSE(coord.getLocked(state));

	coord.setLocked(state, true);  // required
	model.realizeReport(state);  // required: makes the state inconsistent? Despite not changing the system?

	ASSERT_FALSE(model.getWorkingState().isConsistent(state));
}

// repro for an OpenSim bug found in #382
//
// effectively, it is possible to segfault OpenSim by giving it incorrect socket
// assignments: even if the incorrect socket assignmments are provided via an
// `osim` file (i.e. it's not a code bug in OpenSim Creator)
TEST(OpenSimModel, DISABLED_CreatingCircularJointConnectionToGroundDoesNotSegfault)
{
	std::filesystem::path const path =
		std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_382_repro.osim";

	OpenSim::Model model{path.string()};
	model.finalizeFromProperties();
	model.finalizeConnections();  // segfault
}

// repro for an OpenSim bug found in #515
//
// code inside OpenSim::CoordinateCouplerConstraint assumes that a function property
// is always set - even though it is listed as OPTIONAL
TEST(OpenSimModel, DISABLED_CoordinateCouplerConstraintsWithNoCoupledCoordinatesFunctionDoesNotSegfault)
{
	std::filesystem::path const path =
		std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_515_repro.osim";

	OpenSim::Model model{path.string()};
	model.finalizeFromProperties();
	model.finalizeConnections();
	model.buildSystem();  // segfault
}

// repro for an OpenSim bug found in #517
//
// code inside OpenSim::ActivationCoordinateActuator assumes that a coordinate name
// property is always set - even though it is listed as OPTIONAL
TEST(OpenSimModel, DISABLED_ActivationCoordinateActuatorWithNoCoordinateNameDoesNotSegfault)
{
	std::filesystem::path const path =
		std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_517_repro.osim";

	OpenSim::Model model{path.string()};
	model.finalizeFromProperties();
	model.finalizeConnections();  // segfault (exception after applying #621 patch)
}

// repro for an Opensim bug found in #523
//
// code inside OpenSim::PointToPointActuator segfaults if either `bodyA` or `bodyB` is unspecified
TEST(OpenSimModel, DISABLED_PointToPointActuatorWithNoBodyAOrBodyBDoesNotSegfault)
{
	std::filesystem::path const path =
		std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_523_repro.osim";

	OpenSim::Model model{path.string()};
	model.finalizeFromProperties();
	model.finalizeConnections();  // segfault (exception after applying #621 patch)
}

// repro for an OpenSim bug found in #524
//
// code inside OpenSim::SpringGeneralizeForce assumes that the `coordinate` property
// is always set - even though it is listed as OPTIONAL
TEST(OpenSimModel, DISABLED_SpringGeneralizedForceWithNoCoordinateDoesNotSegfault)
{
	std::filesystem::path const path =
		std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_524_repro.osim";

	OpenSim::Model model{path.string()};
	model.finalizeFromProperties();
	model.finalizeConnections();  // segfault (exception after applying #621 patch)
}

// repro for an OpenSim bug found in #621
//
// the way this bug manifests is that:
//
// - load an `osim` containing invalid fields (e.g. `<default_value></default_value>` in a
//   coordinate). This causes OpenSim to initially default the value (via the prototype ctor
//   and `constructProperties()`), but then wipe the default (due to an XML-loading failure)
//   (see: `OpenSim::SimpleProperty<T>::readSimplePropertyFromStream`)
//
// - copy that `osim`, to produce a copy with an empty property (because copying a wiped array
//   creates an actually empty (nullptr) array - rather than a pointer to logically correct data
//   and size==0
//
// - call something that accesses the property (e.g. `buildSystem`) --> boom
TEST(OpenSimModel, LoadingAnOsimWithEmptyFieldsDoesNotSegfault)
{
	std::filesystem::path const brokenFilePath =
		std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_661_repro.osim";

	// sanity check: loading+building an osim is fine
	{
		OpenSim::Model model{brokenFilePath.string()};
		model.buildSystem();  // doesn't segfault, because it relies on unchecked `getProperty` lookups
	}

	OpenSim::Model m1{brokenFilePath.string()};
	OpenSim::Model m2{m1};
	m2.buildSystem();  // segfaults, due to #621 (opensim-core/#3409)
}
