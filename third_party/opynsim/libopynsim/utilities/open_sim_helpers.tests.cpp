#include "open_sim_helpers.h"

#include <libopynsim/tests/testopynsimconfig.h>

#include <gtest/gtest.h>
#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <libopynsim/init.h>
#include <liboscar/platform/log.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/utils/assertions.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>

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
    opyn::init();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "Leg39" / "leg39.osim";

    OpenSim::Model model{modelPath.string()};
    InitializeModel(model);
    InitializeState(model);

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
        InitializeModel(model);
        InitializeState(model);

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
    opyn::init();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

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
    opyn::init();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

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
    opyn::init();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

    OpenSim::Model m{modelPath.string()};
    for (const OpenSim::Component& c : m.getComponentList()) {
        ASSERT_EQ(c.getAbsolutePath(), GetAbsolutePathOrEmpty(&c));
    }
}

// #665: test that the caller can at least *try* to delete anything they want from a complicated
// model without anything exploding (deletion failure is ok, though)
TEST(OpenSimHelpers, CanTryToDeleteEveryComponentFromComplicatedModelWithNoFaultsOrExceptions)
{
    opyn::init();  // ensure muscles are available etc.

    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";

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

// repro for #1070
//
// User reported that they would like OSC to be able to edit models that have
// not-yet-optimized muscle parameters, so the system should ensure that it can
// load and initialize those kinds of models.
TEST(OpenSimHelpers, InitializeModelAndInitializeStateWorkOnModelWithNotOptimizedMuscles)
{
    opyn::init();  // for loading the osim

    const std::filesystem::path brokenFilePath =
        std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "opensim-creator_1070_repro.osim";
    OpenSim::Model model{brokenFilePath.string()};
    InitializeModel(model);  // shouldn't throw

    // sanity check: the model should throw when equilibrating the muscles
    {
        SimTK::State& state = model.initializeState();
        ASSERT_ANY_THROW({ model.equilibrateMuscles(state); }) << "the user-provided osim file should contain a defect that prevents equilibration";
    }

    InitializeState(model);  // shouldn't throw
}

// useful, because it enables adding geometry etc. into the component set that the user can
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
        std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "opensim-creator_773-2_repro.osim";
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
    ASSERT_TRUE(rv.contains("digraph Component"));
    ASSERT_TRUE(rv.contains(R"("/" -> "/child1")"));
    ASSERT_TRUE(rv.contains(R"("/" -> "/child2")"));
    ASSERT_TRUE(rv.contains(R"("/child2" -> "/child1")"));
    ASSERT_TRUE(rv.contains(R"(label="sibling")"));
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
    ASSERT_TRUE(rv.contains("digraph"));
    ASSERT_TRUE(rv.contains(R"(somebody" ->)")) << rv;
}

TEST(OpenSimHelpers, GetAllWrapObjectsReferencedByWorksAsExpected)
{
    opyn::init();  // ensure component registry is populated

    struct ExpectedWrap {
        OpenSim::ComponentPath geometryPathAbsPath;
        std::vector<std::string> associatedWrapObjectNames;
    };

    const auto expectedWraps = std::to_array<ExpectedWrap>({
        {OpenSim::ComponentPath{"/forceset/psoas_r/path"}, {"PS_at_brim_r"}},
        {OpenSim::ComponentPath{"/forceset/vasmed_l/path"}, {"KnExt_at_fem_l"}},
        {OpenSim::ComponentPath{"/forceset/gaslat_r/path"}, {"GasLat_at_shank_r", "Gastroc_at_condyles_r"}},
    });

    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "RajagopalModel" / "Rajagopal2015.osim";
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
    opyn::init();
    std::filesystem::path modelPath = std::filesystem::path{OPYN_TESTING_RESOURCES_DIR} / "models" / "Blank" / "blank.osim";
    OpenSim::Model model{modelPath.string()};
    ASSERT_EQ(RecommendedDocumentName(model), "blank.osim");
}

TEST(OpenSimHelpers, HasModelFileExtension_AcceptsCapitalizedOsimExtension)
{
    // Regression test: some OSIM files on SimTK.org etc. have non-standard
    // file extensions, probably because they were authored on OSes with
    // a case-insensitive filesystem (e.g. Windows). The codebase should try
    // to ignore this error so that legacy files keep loading (#984).
    ASSERT_TRUE(HasModelFileExtension("some/path/to/legacy/model.OSIM"));
    ASSERT_TRUE(HasModelFileExtension("some/path/to/legacy/model.osim"));
    ASSERT_FALSE(HasModelFileExtension("some/path/to/legacy/model.jpeg"));
    ASSERT_FALSE(HasModelFileExtension("some/path/to/legacy/model"));
    ASSERT_FALSE(HasModelFileExtension("some/path/to/legacy/osim"));
}

TEST(OpenSimHelpers, WriteObjectXMLToStringWorksOnBasicRootObject)
{
    OpenSim::Body body{"somebody", 1.0, SimTK::Vec3{2.0, 3.0, 4.0}, SimTK::Inertia{SimTK::Vec3{1.0}}};
    body.finalizeFromProperties();
    const std::string dump = WriteObjectXMLToString(body);

    ASSERT_TRUE(dump.contains("somebody"));
    ASSERT_TRUE(dump.contains("<mass>"));
}

TEST(OpenSimHelpers, ForEachInboundConnectionWorksAsExpected)
{
    // Build a model:
    //
    //          ground
    //            |
    //          body1
    //          |   |
    //     body2a   body2b
    //       |
    //     body3a
    OpenSim::Model model;
    const auto& ground = model.getGround();
    const auto& body1  = AddBody(model, "body1",  1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const auto& body2a = AddBody(model, "body2a", 2.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const auto& body2b = AddBody(model, "body2b", 2.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const auto& body3a = AddBody(model, "body3",  2.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const auto& b1_to_g    = AddJoint<OpenSim::FreeJoint>(model, "body1_to_ground",  ground, body1);
    const auto& b2a_to_b1  = AddJoint<OpenSim::FreeJoint>(model, "body2a_to_body1",  body1,  body2a);
    const auto& b2b_to_b1  = AddJoint<OpenSim::FreeJoint>(model, "body2b_to_body1",  body1,  body2b);
    const auto& b3a_to_b2a = AddJoint<OpenSim::FreeJoint>(model, "body3a_to_body2a", body2a, body3a);
    FinalizeConnections(model);
    InitializeModel(model);

    // helper: makes testing easier
    const auto slurp = []<typename T>(cpp23::generator<T> g)
    {
        std::vector<T> rv;
        for (T&& el : g) {
            rv.push_back(std::move(el));
        }
        return rv;
    };

    // the filter should filter out `FrameGeometry` (junk from OpenSim)
    const auto filter = [](const OpenSim::Component& c)
    {
        return dynamic_cast<const OpenSim::FrameGeometry*>(&c) == nullptr;
    };

    // test ground
    {
        const auto got = slurp(ForEachInboundConnection(&model, &model.getGround(), filter));
        std::vector<ComponentConnectionView> expected = {
            ComponentConnectionView{b1_to_g,   ground, "parent_frame"},
        };
        ASSERT_EQ(got, expected);
    }

    // test body1
    {
        const auto got = slurp(ForEachInboundConnection(&model, &body1, filter));
        std::vector<ComponentConnectionView> expected = {
            ComponentConnectionView{b1_to_g,   body1, "child_frame"},
            ComponentConnectionView{b2a_to_b1, body1, "parent_frame"},
            ComponentConnectionView{b2b_to_b1, body1, "parent_frame"},
        };
        ASSERT_EQ(got, expected);
    }

    // test body2a
    {
        const auto got = slurp(ForEachInboundConnection(&model, &body2a, filter));
        std::vector<ComponentConnectionView> expected = {
            ComponentConnectionView{b2a_to_b1,  body2a, "child_frame"},
            ComponentConnectionView{b3a_to_b2a, body2a, "parent_frame"},
        };
        ASSERT_EQ(got, expected);
    }

    // test body2b
    {
        const auto got = slurp(ForEachInboundConnection(&model, &body2b, filter));
        std::vector<ComponentConnectionView> expected = {
            ComponentConnectionView{b2b_to_b1,  body2b, "child_frame"},
        };
        ASSERT_EQ(got, expected);
    }

    // test body3a
    {
        const auto got = slurp(ForEachInboundConnection(&model, &body3a, filter));
        std::vector<ComponentConnectionView> expected = {
            ComponentConnectionView{b3a_to_b2a,  body3a, "child_frame"},
        };
        ASSERT_EQ(got, expected);
    }
}

TEST(OpenSimHelpers, ScaleModelMassPreserveMassDistribution_WorksOnBasicExample)
{
    // Build a 3 kg model:
    //
    //           ground
    //             |
    //       body1 (1.5 kg)
    //       |            |
    // body2a (1 kg)   body2b (0.5 kg)

    OpenSim::Model model;
    const auto& ground = model.getGround();
    const auto& body1  = AddBody(model, "body1",  1.5, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const auto& body2a = AddBody(model, "body2a", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    const auto& body2b = AddBody(model, "body2b", 0.5, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
    AddJoint<OpenSim::FreeJoint>(model, "body1_to_ground",  ground, body1);
    AddJoint<OpenSim::FreeJoint>(model, "body2a_to_body1",  body1,  body2a);
    AddJoint<OpenSim::FreeJoint>(model, "body2b_to_body1",  body1,  body2b);

    FinalizeConnections(model);
    InitializeModel(model);
    SimTK::State state = InitializeState(model);

    const double originalTotalMass = 3.0;
    const double tolerance = 0.000001;  // 1 microgram
    ASSERT_NEAR(model.getTotalMass(state), originalTotalMass, tolerance);

    const double newTotalMass = 5.0;
    ScaleModelMassPreserveMassDistribution(model, state, newTotalMass);
    InitializeModel(model);
    state = InitializeState(model);

    const double massScalingFactor = newTotalMass / originalTotalMass;

    ASSERT_NEAR(model.getTotalMass(state), newTotalMass, tolerance);
    ASSERT_NEAR(body1.getMass(), massScalingFactor * 1.5, tolerance);
    ASSERT_NEAR(body2a.getMass(), massScalingFactor * 1.0, tolerance);
    ASSERT_NEAR(body2b.getMass(), massScalingFactor * 0.5, tolerance);
}
