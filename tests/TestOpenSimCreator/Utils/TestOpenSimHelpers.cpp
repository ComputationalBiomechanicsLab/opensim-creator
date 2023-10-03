#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.hpp>

#include <gtest/gtest.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/Log.hpp>
#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.hpp>
#include <OpenSimCreator/Registry/ComponentRegistry.hpp>
#include <OpenSimCreator/Registry/StaticComponentRegistries.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// repro for #263 (https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/263)
//
// Effectively, this is what the joint switcher in the UI is doing. It is permitted for the
// code to throw an exception (e.g. because other parts of the model depend on something in
// the joint) but it shouldn't hard crash (it is)
TEST(OpenSimHelpers, DISABLED_CanSwapACustomJointForAFreeJoint)
{
    auto const config = osc::LoadOpenSimCreatorConfig();
    osc::GlobalInitOpenSim(config);  // ensure muscles are available etc.

    std::filesystem::path modelPath = config.getResourceDir() / "models" / "Leg39" / "leg39.osim";

    osc::UndoableModelStatePair model{std::make_unique<OpenSim::Model>(modelPath.string())};

    model.updModel();  // should be fine, before any edits
    model.getState();  // also should be fine

    auto const& registry = osc::GetComponentRegistry<OpenSim::Joint>();
    auto maybeIdx = osc::IndexOf<OpenSim::FreeJoint>(registry);
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
        auto const& joint = model.getModel().getComponent<OpenSim::Joint>(p);

        std::string msg = "changed " + joint.getAbsolutePathString();

        OpenSim::Component const& parent = joint.getOwner();
        auto const* jointSet = dynamic_cast<OpenSim::JointSet const*>(&parent);

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

        auto replacement = registry[jointIdx].instantiate();

        osc::CopyCommonJointProperties(joint, *replacement);

        // update model
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
    auto const config = osc::LoadOpenSimCreatorConfig();
    std::filesystem::path modelPath = config.getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";

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
    auto const config = osc::LoadOpenSimCreatorConfig();
    std::filesystem::path modelPath = config.getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";

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
    auto const config = osc::LoadOpenSimCreatorConfig();
    std::filesystem::path modelPath = config.getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";

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
    auto const config = osc::LoadOpenSimCreatorConfig();
    std::filesystem::path modelPath = config.getResourceDir() / "models" / "RajagopalModel" / "Rajagopal2015.osim";\

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

TEST(OpenSimHelpers, AddModelComponentReturnsProvidedPointer)
{
    OpenSim::Model m;

    auto p = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    p->setParentFrame(m.getGround());

    OpenSim::PhysicalOffsetFrame* expected = p.get();
    ASSERT_EQ(&osc::AddModelComponent(m, std::move(p)), expected);
}

TEST(OpenSimHelpers, AddModelComponentAddsComponentToModelComponentSet)
{
    OpenSim::Model m;

    auto p = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    p->setParentFrame(m.getGround());

    OpenSim::PhysicalOffsetFrame& s = osc::AddModelComponent(m, std::move(p));
    osc::FinalizeConnections(m);

    ASSERT_EQ(m.get_ComponentSet().getSize(), 1);
    ASSERT_EQ(dynamic_cast<OpenSim::Component const*>(&m.get_ComponentSet()[0]), dynamic_cast<OpenSim::Component const*>(&s));
}

// mid-level repro for (#773)
//
// the bug is fundamentally because `Component::finalizeConnections` messes
// around with stale pointers to deleted slave components. This mid-level
// test is here in case OSC is doing some kind of magic in `osc::FinalizeConnections`
// that `OpenSim` doesn't do
TEST(OpenSimHelpers, DISABLED_FinalizeConnectionsWithUnusualJointTopologyDoesNotSegfault)
{
    std::filesystem::path const brokenFilePath =
        std::filesystem::path{OSC_TESTING_SOURCE_DIR} / "build_resources" / "test_fixtures" / "opensim-creator_773-2_repro.osim";
    OpenSim::Model model{brokenFilePath.string()};
    model.finalizeFromProperties();

    for (size_t i = 0; i < 10; ++i)
    {
        osc::FinalizeConnections(model);  // the HACK should make this work fine
    }
}
