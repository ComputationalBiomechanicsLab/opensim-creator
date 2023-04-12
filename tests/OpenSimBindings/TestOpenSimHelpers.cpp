#include "src/OpenSimBindings/OpenSimApp.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"

#include <gtest/gtest.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// repro for #263 (https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/263)
//
// Effectively, this is what the joint switcher in the UI is doing. It is permitted for the
// code to throw an exception (e.g. because other parts of the model depend on something in
// the joint) but it shouldn't hard crash (it is)
TEST(OpenSimHelpers, CanSwapACustomJointForAFreeJoint)
{
	return;  // disable

	auto config = osc::Config::load();
	osc::GlobalInitOpenSim(*config);  // ensure muscles are available etc.

	std::filesystem::path modelPath = config->getResourceDir() / "models" / "Leg39" / "leg39.osim";

	osc::UndoableModelStatePair model{std::make_unique<OpenSim::Model>(modelPath.string())};

	model.updModel();  // should be fine, before any edits
	model.getState();  // also should be fine

	auto maybeIdx = osc::JointRegistry::indexOf<OpenSim::FreeJoint>();
	ASSERT_TRUE(maybeIdx) << "can't find FreeJoint in type registry?";
	auto idx = *maybeIdx;

	// cache joint paths, because we are changing the model during this test and it might
	// invalidate the model's `getComponentList` function
	std::vector<OpenSim::ComponentPath> allJointPaths;
	for (OpenSim::Joint const& joint : model.getModel().getComponentList<OpenSim::Joint>())
	{
		allJointPaths.push_back(joint.getAbsolutePath());
	}

	for (OpenSim::ComponentPath const& p : allJointPaths)
	{
		OpenSim::Joint const& joint = model.getModel().getComponent<OpenSim::Joint>(p);

		std::string msg = "changed " + joint.getAbsolutePathString();

		OpenSim::Component const& parent = joint.getOwner();
		OpenSim::JointSet const* jointSet = dynamic_cast<OpenSim::JointSet const*>(&parent);

		if (!jointSet)
		{
			continue;  // this joint doesn't count
		}

		int jointIdx = -1;
		for (int i = 0; i < jointSet->getSize(); ++i)
		{
			OpenSim::Joint const* j = &(*jointSet)[i];
			if (j == &joint)
			{
				jointIdx = i;
			}
		}

		ASSERT_NE(jointIdx, -1) << "the joint should exist within its parent set";

		auto replacement = std::unique_ptr<OpenSim::Joint>{osc::JointRegistry::prototypes()[jointIdx]->clone()};

		osc::CopyCommonJointProperties(joint, *replacement);

		// update model
		auto* ptr = replacement.get();
		const_cast<OpenSim::JointSet&>(*jointSet).set(static_cast<int>(idx), replacement.release());
		model.updModel();  // dirty it
		model.commit(msg);

		osc::log::info("%s", msg.c_str());
	}
}

TEST(OpenSimHelpers, GetAbsolutePathStringWorksForModel)
{
	OpenSim::Model m;
	std::string const s = osc::GetAbsolutePathString(m);
	ASSERT_EQ(s, "/");
}

TEST(OpenSimHelpers, GetAbsolutePathStringWithOutparamWorksForModel)
{
	OpenSim::Model m;
	std::string outparam = "somejunk";
	osc::GetAbsolutePathString(m, outparam);
	ASSERT_EQ(outparam, "/");
}

TEST(OpenSimHelpers, GetAbsolutePathStringReturnsSameResultAsOpenSimVersionForComplexModel)
{
	auto config = osc::Config::load();
	std::filesystem::path modelPath = config->getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";

	OpenSim::Model m{modelPath.string()};
	std::string outparam;
	for (OpenSim::Component const& c : m.getComponentList())
	{
		// test both the "pure" and "assigning" versions at the same time
		osc::GetAbsolutePathString(c, outparam);
		ASSERT_EQ(c.getAbsolutePathString(), osc::GetAbsolutePathString(c));
		ASSERT_EQ(c.getAbsolutePathString(), outparam);
	}
}

TEST(OpenSimHelpers, GetAbsolutePathReturnsSameResultAsOpenSimVersionForComplexModel)
{
	auto config = osc::Config::load();
	std::filesystem::path modelPath = config->getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";

	OpenSim::Model m{modelPath.string()};
	for (OpenSim::Component const& c : m.getComponentList())
	{
		ASSERT_EQ(c.getAbsolutePath(), osc::GetAbsolutePath(c));
	}
}

TEST(OpenSimHelpers, GetAbsolutePathOrEmptyReuturnsEmptyIfPassedANullptr)
{
	ASSERT_EQ(OpenSim::ComponentPath{}, osc::GetAbsolutePathOrEmpty(nullptr));
}

TEST(OpenSimHelpers, GetAbsolutePathOrEmptyReuturnsSameResultAsOpenSimVersionForComplexModel)
{
	auto config = osc::Config::load();
	std::filesystem::path modelPath = config->getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";

	OpenSim::Model m{modelPath.string()};
	for (OpenSim::Component const& c : m.getComponentList())
	{
		ASSERT_EQ(c.getAbsolutePath(), osc::GetAbsolutePathOrEmpty(&c));
	}
}

// #665: test that the caller can at least *try* to delete anything they want from a complicated
// model without anything exploding (deletion failure is ok, though)
TEST(OpenSimHelpers, CanTryToDeleteEveryComponentFromComplicatedModelWithNoFaultsOrExceptions)
{
    auto config = osc::Config::load();
    std::filesystem::path modelPath = config->getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";\

    OpenSim::Model const originalModel{modelPath.string()};
    OpenSim::Model modifiedModel{originalModel};
    osc::InitializeModel(modifiedModel);
    osc::InitializeModel(modifiedModel);

    // iterate over the original (const) model, so that iterator
    // invalidation can't happen
    for (OpenSim::Component const& c : originalModel.getComponentList())
    {
        // if the component still exists in the to-be-deleted-from model
        // (it may have been indirectly deleted), then try to delete it
        OpenSim::Component* lookup = osc::FindComponentMut(modifiedModel, c.getAbsolutePath());
        if (lookup)
        {
            if (osc::TryDeleteComponentFromModel(modifiedModel, *lookup))
            {
                osc::log::info("deleted %s (%s)", c.getName().c_str(), c.getConcreteClassName().c_str());
                osc::InitializeModel(modifiedModel);
                osc::InitializeState(modifiedModel);
            }
        }
    }
}
