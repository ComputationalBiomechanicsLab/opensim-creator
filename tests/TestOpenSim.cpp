#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>

#include <gtest/gtest.h>

#include <filesystem>

TEST(OpenSimModel, ProducesCorrectMomentArmOnFirstComputeCall)
{
	// ensure muscles are available
	RegisterTypes_osimActuators();

	// data sources
	std::filesystem::path modelPath{R"(C:\\Users\\adamk\\OneDrive\\Desktop\\opensim-creator\\resources\\models\\Arm26\\arm26.osim)"};
	OpenSim::ComponentPath coordinatePath = "/jointset/r_shoulder/r_shoulder_elev";
	OpenSim::ComponentPath musclePath = "/forceset/BIClong";

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