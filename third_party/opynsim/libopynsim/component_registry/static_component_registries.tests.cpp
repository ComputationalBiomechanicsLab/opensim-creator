#include "static_component_registries.h"

#include <gtest/gtest.h>
#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/opynsim.h>
#include <liboscar/utilities/c_string_view.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/GimbalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PlanarJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/SliderJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>

#include <array>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

using namespace opyn;

namespace
{
    // a single instance of a joint to test
    struct TestCase final {

        template<typename T, typename... Names>
        static TestCase create(Names... names)
        {
            return TestCase{
                typeid(T).name(),
                index_of<T>(get_component_registry<OpenSim::Joint>()),
                {std::forward<Names>(names)...},
            };
        }

        std::string name;
        std::optional<size_t> maybe_index;
        std::vector<osc::CStringView> expected_names;
    };
}

TEST(ComponentRegistry, coords_have_expected_names)
{
    opyn::init();

    // ensure the type registry sets the default OpenSim coordinate names to something
    // easier to work with
    //
    // the documentation/screenshots etc. assume that coordinates end up with with these
    // names, so if you want to change them you should ensure the change doesn't cause
    // a problem w.r.t. UX, docs, etc.

    // all of the test cases
    std::array test_cases = {
        TestCase::create<OpenSim::BallJoint>("rx", "ry", "rz"),
        TestCase::create<OpenSim::EllipsoidJoint>("rx", "ry", "rz"),
        TestCase::create<OpenSim::FreeJoint>("rx", "ry", "rz", "tx", "ty", "tz"),
        TestCase::create<OpenSim::GimbalJoint>("rx", "ry", "rz"),
        TestCase::create<OpenSim::PinJoint>("rz"),
        TestCase::create<OpenSim::PlanarJoint>("rz", "tx", "ty"),
        TestCase::create<OpenSim::ScapulothoracicJoint>("rx_abduction", "ry_elevation", "rz_upwardrotation", "ryp_winging"),
        TestCase::create<OpenSim::SliderJoint>("tx"),
        TestCase::create<OpenSim::UniversalJoint>("rx", "ry"),
    };

    // go through each test case and ensure the names match
    for (const TestCase& tc : test_cases) {
        ASSERT_TRUE(tc.maybe_index) << tc.name << " does not exist in the registry(it should)";

        const auto& proto = get_component_registry<OpenSim::Joint>()[*tc.maybe_index].prototype();
        const auto& coord_prop = proto.getProperty_coordinates();

        ASSERT_EQ(coord_prop.size(), tc.expected_names.size()) << tc.name <<  " has different number of coords from expected";

        for (int i = 0; i < coord_prop.size(); ++i) {
            ASSERT_EQ(coord_prop.getValue(i).getName(), tc.expected_names[i]) << tc.name << " coordinate " << i << " has different name from expected";
        }
    }
}

// #298: try adding every available joint type into a blank OpenSim model to ensure
//       that all joint types can be added without an exception/segfault
TEST(JointRegistry, can_add_any_joint_without_an_exception_or_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::Joint>()) {
        // create a blank model
        OpenSim::Model model;

        // create a body
        auto body = std::make_unique<OpenSim::Body>();
        body->setName("onebody");
        body->setMass(1.0);  // required

        // create joint between the model's ground and the body
        auto joint = entry.instantiate();
        joint->connectSocket_parent_frame(model.getGround());
        joint->connectSocket_child_frame(*body);

        // add the joint + body to the model
        model.addJoint(joint.release());
        model.addBody(body.release());

        // initialize the model+system+state
        //
        // (shouldn't throw or segfault)
        model.finalizeFromProperties();
        model.buildSystem();
    }
}

// #298: try converting between every available joint type in an existing model to
//       ensure there's no faults
TEST(JointRegistry, can_convert_between_any_joint_without_an_exception_or_segfault)
{
    const auto& entries = get_component_registry<OpenSim::Joint>();

    for (size_t i = 0; i < entries.size(); ++i) {
        for (size_t j = 0; j < entries.size(); ++j) {
            // create a model with base joint
            OpenSim::Model model;

            auto& body = add_body(model, "body", 1.0, SimTK::Vec3{}, SimTK::Inertia(1.0));
            body.setMass(1.0);

            auto& joint = add_joint(model, entries[i].instantiate());
            joint.connectSocket_parent_frame(model.getGround());
            joint.connectSocket_child_frame(body);

            finalize_connections(model);
            initialize_model(model);
            initialize_state(model);

            // then switch the joint over
            auto new_joint = entries[j].instantiate();
            copy_common_joint_properties(joint, *new_joint);
            auto& joint_set = upd_owner_or_throw<OpenSim::JointSet>(model, joint);
            assign(joint_set, joint, std::move(new_joint));

            finalize_connections(model);
            initialize_model(model);
            initialize_state(model);
        }
    }
}

// #298: try adding every available contact geometry type into a blank OpenSim model
//       to ensure that all contact geometries can be added without an exception/segfault
TEST(ContactGeometryRegistry, can_add_any_contact_geometry_without_an_exception_or_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::ContactGeometry>()) {
        // create a blank model
        OpenSim::Model model;

        // create contact geometry attached to model's ground frame
        auto geom = entry.instantiate();
        geom->connectSocket_frame(model.getGround());

        // add it to the model
        model.addContactGeometry(geom.release());

        // initialize the model+system+state
        //
        // (shouldn't throw or segfault)
        model.finalizeFromProperties();
        model.buildSystem();
    }
}

// #298: try adding every available constraint to a blank OpenSim model
//       to ensure that all of them can be added without a segfault
//
// (throwing is permitted, because constraints typically rely on
//  other stuff, e.g. coordinates, existing in the model)
TEST(ConstraintRegistry, can_add_any_constraint_without_a_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::Constraint>()) {
        // create a blank model
        OpenSim::Model model;

        // default-construct the constraint
        auto constraint = entry.instantiate();

        // add it to the model
        model.addConstraint(constraint.release());

        // initialize the model+system+state
        try {
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (const std::exception&) {  // NOLINT(bugprone-empty-catch)
            // ok: it might throw because the constraint might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}

// #298: try adding every available force to a blank OpenSim model
//       to ensure that all of them can be added without a segfault
//
// (throwing is permitted, because forces typically rely on
//  other stuff, e.g. coordinates, existing in the model)
TEST(ForceRegistry, can_add_any_force_without_a_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::Force>()) {
        // create a blank model
        OpenSim::Model model;

        // default-construct the force
        auto force = entry.instantiate();

        // initialize the model+system+state
        try {
            model.addForce(force.release());  // finalizes, so can throw
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (const std::exception&) {  // NOLINT(bugprone-empty-catch)
            // ok: it might throw because the constraint might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}

// #298: try adding every available controller to a blank OpenSim model
//       to ensure that all of them can be added without a segfault
TEST(ControllerRegistry, can_add_any_controller_without_a_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::Controller>()) {
        // create a blank model
        OpenSim::Model model;

        // default-construct the controller
        auto controller = entry.instantiate();

        // add it to the model
        model.addController(controller.release());

        // initialize the model+system+state
        try {
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (const std::exception&) {  // NOLINT(bugprone-empty-catch)
            // ok: it might throw because the controller might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}

// #298: try adding every available probe type to a blank OpenSim model
//       to ensure that all of them can be added without a segfault
TEST(ProbeRegistry, can_add_any_probe_without_a_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::Probe>()) {
        // create a blank model
        OpenSim::Model model;

        // default-construct the probe
        auto probe = entry.instantiate();

        // add it to the model
        model.addProbe(probe.release());

        // initialize the model+system+state
        //
        // (doesn't seem to throw for any probe I've tested up to now)
        model.finalizeFromProperties();
        model.buildSystem();
    }
}

// #298: try adding every available "ungrouped" component (i.e. a component that
//       cannot be cleanly assigned to a known registry type) to a blank OpenSim
//       model to ensure that all ungrouped components can be added without a
//       segfault
TEST(UngroupedRegistry, can_add_any_ungrouped_component_without_a_segfault)
{
    for (const auto& entry : get_component_registry<OpenSim::Component>()) {
        // create a blank model
        OpenSim::Model model;

        // default-construct the component
        auto component = entry.instantiate();

        try {
            model.addComponent(component.release());
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (const std::exception&) {  // NOLINT(bugprone-empty-catch)
            // ok: it might throw because the component might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}

TEST(WrapObjectRegistry, can_instantiate_all_available_wrap_objects_without_issue)
{
    for (const auto& entry : get_component_registry<OpenSim::WrapObject>()) {
        ASSERT_FALSE(entry.name().empty());
        ASSERT_NO_THROW({ entry.instantiate(); });
    }
}
