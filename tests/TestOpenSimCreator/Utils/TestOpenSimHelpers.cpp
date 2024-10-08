#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Platform/OpenSimCreatorApp.h>
#include <gtest/gtest.h>
#include <oscar/Platform/Log.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/StringHelpers.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace osc;

namespace
{
    class InnerParent : public OpenSim::Component {
        OpenSim_DECLARE_ABSTRACT_OBJECT(InnerParent, OpenSim::Component)
    };

    class Child1 final : public InnerParent {
        OpenSim_DECLARE_CONCRETE_OBJECT(Child1, OpenSim::Component)
    };

    class Child2 final : public InnerParent {
        OpenSim_DECLARE_CONCRETE_OBJECT(Child2, OpenSim::Component)
        OpenSim_DECLARE_SOCKET(sibling, InnerParent, "sibling connection");

    public:
        Child2()
        {
            updSocket("sibling").setConnecteePath("../child1");
        }
    };

    class Root final : public OpenSim::Component {
        OpenSim_DECLARE_CONCRETE_OBJECT(Root, OpenSim::Component)
        OpenSim_DECLARE_PROPERTY(child1, Child1, "first child");
        OpenSim_DECLARE_PROPERTY(child2, Child2, "second child");

    public:
        Root()
        {
            constructProperty_child1(Child1{});
            constructProperty_child2(Child2{});
        }
    };
}

// repro for #263 (https://github.com/ComputationalBiomechanicsLab/opensim-creator/issues/263)
//
// Effectively, this is what the joint switcher in the UI is doing. It is permitted for the
// code to throw an exception (e.g. because other parts of the model depend on something in
// the joint) but it shouldn't hard crash (it is)
TEST(OpenSimHelpers, DISABLED_CanSwapACustomJointForAFreeJoint)
{
    GloballyInitOpenSim();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "Leg39" / "leg39.osim";

    UndoableModelStatePair model{std::make_unique<OpenSim::Model>(modelPath.string())};

    model.updModel();  // should be fine, before any edits
    model.getState();  // also should be fine

    const auto& registry = GetComponentRegistry<OpenSim::Joint>();
    auto maybeIdx = IndexOf<OpenSim::FreeJoint>(registry);
    ASSERT_TRUE(maybeIdx) << "can't find FreeJoint in type registry?";
    auto idx = *maybeIdx;

    // cache joint paths, because we are changing the model during this test and it might
    // invalidate the model's `getComponentList` function
    std::vector<OpenSim::ComponentPath> allJointPaths;
    for (const OpenSim::Joint& joint : model.getModel().getComponentList<OpenSim::Joint>()) {
        allJointPaths.push_back(joint.getAbsolutePath());
    }

    for (const OpenSim::ComponentPath& p : allJointPaths) {
        const auto& joint = model.getModel().getComponent<OpenSim::Joint>(p);

        std::string msg = "changed " + joint.getAbsolutePathString();

        const OpenSim::Component& parent = joint.getOwner();
        const auto* jointSet = dynamic_cast<const OpenSim::JointSet*>(&parent);

        if (!jointSet)
        {
            continue;  // this joint doesn't count
        }

        int jointIdx = -1;
        for (int i = 0; i < jointSet->getSize(); ++i)
        {
            const OpenSim::Joint* j = &(*jointSet)[i];
            if (j == &joint)
            {
                jointIdx = i;
            }
        }

        ASSERT_NE(jointIdx, -1) << "the joint should exist within its parent set";

        auto replacement = registry[jointIdx].instantiate();

        CopyCommonJointProperties(joint, *replacement);

        // update model
        const_cast<OpenSim::JointSet&>(*jointSet).set(static_cast<int>(idx), replacement.release());
        model.updModel();  // dirty it
        model.commit(msg);

        log_info("%s", msg.c_str());
    }
}

TEST(OpenSimHelpers, GetAbsolutePathStringWorksForModel)
{
    OpenSim::Model m;
    const std::string s = GetAbsolutePathString(m);
    ASSERT_EQ(s, "/");
}

TEST(OpenSimHelpers, GetAbsolutePathStringWithOutparamWorksForModel)
{
    OpenSim::Model m;
    std::string outparam = "somejunk";
    GetAbsolutePathString(m, outparam);
    ASSERT_EQ(outparam, "/");
}

TEST(OpenSimHelpers, GetAbsolutePathStringReturnsSameResultAsOpenSimVersionForComplexModel)
{
    GloballyInitOpenSim();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

    OpenSim::Model m{modelPath.string()};
    std::string outparam;
    for (const OpenSim::Component& c : m.getComponentList()) {
        // test both the "pure" and "assigning" versions at the same time
        GetAbsolutePathString(c, outparam);
        ASSERT_EQ(c.getAbsolutePathString(), GetAbsolutePathString(c));
        ASSERT_EQ(c.getAbsolutePathString(), outparam);
    }
}

TEST(OpenSimHelpers, GetAbsolutePathReturnsSameResultAsOpenSimVersionForComplexModel)
{
    GloballyInitOpenSim();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

    OpenSim::Model m{modelPath.string()};
    for (const OpenSim::Component& c : m.getComponentList()) {
        ASSERT_EQ(c.getAbsolutePath(), GetAbsolutePath(c));
    }
}

TEST(OpenSimHelpers, GetAbsolutePathOrEmptyReuturnsEmptyIfPassedANullptr)
{
    ASSERT_EQ(OpenSim::ComponentPath{}, GetAbsolutePathOrEmpty(nullptr));
}

TEST(OpenSimHelpers, GetAbsolutePathOrEmptyReuturnsSameResultAsOpenSimVersionForComplexModel)
{
    GloballyInitOpenSim();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

    OpenSim::Model m{modelPath.string()};
    for (const OpenSim::Component& c : m.getComponentList()) {
        ASSERT_EQ(c.getAbsolutePath(), GetAbsolutePathOrEmpty(&c));
    }
}

// #665: test that the caller can at least *try* to delete anything they want from a complicated
// model without anything exploding (deletion failure is ok, though)
TEST(OpenSimHelpers, CanTryToDeleteEveryComponentFromComplicatedModelWithNoFaultsOrExceptions)
{
    GloballyInitOpenSim();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

    const OpenSim::Model originalModel{modelPath.string()};
    OpenSim::Model modifiedModel{originalModel};
    InitializeModel(modifiedModel);

    // iterate over the original (const) model, so that iterator
    // invalidation can't happen
    for (const OpenSim::Component& c : originalModel.getComponentList()) {
        // if the component still exists in the to-be-deleted-from model
        // (it may have been indirectly deleted), then try to delete it
        if (OpenSim::Component* lookup = FindComponentMut(modifiedModel, c.getAbsolutePath())) {
            if (TryDeleteComponentFromModel(modifiedModel, *lookup)) {
                log_info("deleted %s (%s)", c.getName().c_str(), c.getConcreteClassName().c_str());
                InitializeModel(modifiedModel);
                InitializeState(modifiedModel);
            }
        }
    }
}

// useful, because it enables adding random geometry etc. into the component set that the user can
// later clean up in the UI
TEST(OpenSimHelpers, CanDeleteAnOffsetFrameFromAModelsComponentSet)
{
    OpenSim::Model model;
    auto& pof = AddModelComponent(model, std::make_unique<OpenSim::PhysicalOffsetFrame>());
    pof.setParentFrame(model.getGround());
    FinalizeConnections(model);
    InitializeModel(model);
    InitializeState(model);

    ASSERT_EQ(model.get_ComponentSet().getSize(), 1);
    ASSERT_TRUE(TryDeleteComponentFromModel(model, pof));
    ASSERT_EQ(model.get_ComponentSet().getSize(), 0);
}

TEST(OpenSimHelpers, AddModelComponentReturnsProvidedPointer)
{
    OpenSim::Model m;

    auto p = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    p->setParentFrame(m.getGround());

    OpenSim::PhysicalOffsetFrame* expected = p.get();
    ASSERT_EQ(&AddModelComponent(m, std::move(p)), expected);
}

TEST(OpenSimHelpers, AddModelComponentAddsComponentToModelComponentSet)
{
    OpenSim::Model m;

    auto p = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    p->setParentFrame(m.getGround());

    OpenSim::PhysicalOffsetFrame& s = AddModelComponent(m, std::move(p));
    FinalizeConnections(m);

    ASSERT_EQ(m.get_ComponentSet().getSize(), 1);
    ASSERT_EQ(dynamic_cast<const OpenSim::Component*>(&m.get_ComponentSet()[0]), dynamic_cast<const OpenSim::Component*>(&s));
}

// mid-level repro for (#773)
//
// the bug is fundamentally because `Component::finalizeConnections` messes
// around with stale pointers to deleted slave components. This mid-level
// test is here in case OSC is doing some kind of magic in `FinalizeConnections`
// that `OpenSim` doesn't do
TEST(OpenSimHelpers, DISABLED_FinalizeConnectionsWithUnusualJointTopologyDoesNotSegfault)
{
    const std::filesystem::path brokenFilePath =
        std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "opensim-creator_773-2_repro.osim";
    OpenSim::Model model{brokenFilePath.string()};
    model.finalizeFromProperties();

    for (size_t i = 0; i < 10; ++i) {
        FinalizeConnections(model);  // the HACK should make this work fine
    }
}

TEST(OpenSimHelpers, ForEachIsNotCalledOnRootComponent)
{
    Root root;
    root.finalizeFromProperties();
    size_t n = 0;
    ForEachComponent(root, [&n](const OpenSim::Component&){ ++n; });
    ASSERT_EQ(n, 2);
}

TEST(OpenSimHelpers, GetNumChildrenReturnsExpectedNumber)
{
    Root root;
    root.finalizeFromProperties();
    ASSERT_EQ(GetNumChildren(root), 2);
}

TEST(OpenSimHelpers, TypedGetNumChildrenOnlyCountsChildrenWithGivenType)
{
    Root root;
    root.finalizeFromProperties();
    ASSERT_EQ(GetNumChildren<Child1>(root), 1);
    ASSERT_EQ(GetNumChildren<Child2>(root), 1);
    ASSERT_EQ(GetNumChildren<InnerParent>(root), 2);
}

TEST(OpenSimHelpers, WriteComponentTopologyGraphAsDotViz)
{
    Root root;
    root.finalizeConnections(root);
    std::stringstream ss;
    WriteComponentTopologyGraphAsDotViz(root, ss);

    const std::string rv = ss.str();
    ASSERT_TRUE(contains(rv, "digraph Component"));
    ASSERT_TRUE(contains(rv, R"("/" -> "/child1")"));
    ASSERT_TRUE(contains(rv, R"("/" -> "/child2")"));
    ASSERT_TRUE(contains(rv, R"("/child2" -> "/child1")"));
    ASSERT_TRUE(contains(rv, R"(label="sibling")"));
}

TEST(OpenSimHelpers, WriteModelMultibodySystemGraphAsDotViz)
{
    OpenSim::Model model;
    model.addBody(std::make_unique<OpenSim::Body>("somebody", 1.0, SimTK::Vec3(0.0), SimTK::Inertia{SimTK::Vec3(1.0)}).release());
    model.buildSystem();

    std::stringstream ss;
    WriteModelMultibodySystemGraphAsDotViz(model, ss);

    const std::string rv = ss.str();
    ASSERT_FALSE(rv.empty());
    ASSERT_TRUE(contains(rv, "digraph"));
    ASSERT_TRUE(contains(rv, R"(somebody" ->)")) << rv;
}

TEST(OpenSimHelpers, GetAllWrapObjectsReferencedByWorksAsExpected)
{
    GloballyInitOpenSim();  // ensure component registry is populated

    struct ExpectedWrap {
        OpenSim::ComponentPath geometryPathAbsPath;
        std::vector<std::string> associatedWrapObjectNames;
    };

    const auto expectedWraps = std::to_array<ExpectedWrap>({
        {OpenSim::ComponentPath{"/forceset/psoas_r/path"}, {"PS_at_brim_r"}},
        {OpenSim::ComponentPath{"/forceset/vasmed_l/path"}, {"KnExt_at_fem_l"}},
        {OpenSim::ComponentPath{"/forceset/gaslat_r/path"}, {"GasLat_at_shank_r", "Gastroc_at_condyles_r"}},
    });

    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";
    OpenSim::Model m{modelPath.string()};
    InitializeModel(m);
    InitializeState(m);

    for (const auto& [geomAbsPath, expectedWrapObjectNames] : expectedWraps) {
        const auto* gp = FindComponent<OpenSim::GeometryPath>(m, geomAbsPath);
        OSC_ASSERT_ALWAYS(gp != nullptr && "maybe the rajagopal model has changed?");
        for (const OpenSim::WrapObject* wo : GetAllWrapObjectsReferencedBy(*gp)) {
            ASSERT_TRUE(cpp23::contains(expectedWrapObjectNames, wo->getName()));
        }
    }
}

TEST(OpenSimHelpers, IsAllElementsUniqueReturnsTrueForUniqueCase)
{
    OpenSim::Array<int> els;
    els.ensureCapacity(5);
    els.append(3);
    els.append(2);
    els.append(1);
    els.append(4);
    els.append(-2);

    ASSERT_TRUE(IsAllElementsUnique(els));
}

TEST(OpenSimHelpers, IsAllElementsUniqueReturnsFalseForNotUniqueCase)
{
    OpenSim::Array<int> els;
    els.ensureCapacity(5);
    els.append(3);
    els.append(4);
    els.append(1);
    els.append(4);  // uh oh
    els.append(-2);

    ASSERT_FALSE(IsAllElementsUnique(els));
}

TEST(OpenSimHelpers, RecommendedDocumentName_ReturnsUntitledWhenProvidedInMemoryModel)
{
    ASSERT_EQ(RecommendedDocumentName(OpenSim::Model{}), "untitled.osim");
}

TEST(OpenSimHelpers, RecommendedDocumentName_ReturnsFilenameIfProvidedLoadedModel)
{
    std::filesystem::path modelPath = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "models" / "Blank" / "blank.osim";
    OpenSim::Model model{modelPath.string()};
    ASSERT_EQ(RecommendedDocumentName(model), "blank.osim");
}
