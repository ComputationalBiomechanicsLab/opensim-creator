#include "src/OpenSimBindings/TypeRegistry.hpp"

#include "src/Utils/CStringView.hpp"

#include <gtest/gtest.h>
#include <OpenSim/Common/ComponentPath.h>
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

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace
{
	// a single instance of a joint to test
	struct TestCase {
		std::string name;
		std::optional<size_t> maybeIndex;
		std::vector<osc::CStringView> expectedNames;

		template<typename T, typename... Names>
		static TestCase create(Names... names)
		{
			return TestCase
			{
				typeid(T).name(),
				osc::TypeRegistry<OpenSim::Joint>::indexOf<T>(),
				{std::forward<Names>(names)...}
			};
		}
	};
}

TEST(TypeRegistry, CoordsHaveExpectedNames)
{
	// ensure the typeregistry sets the default OpenSim coordinate names to something
	// easier to work with
	//
	// the documentation/screenshots etc. assume that coordinates end up with with these
	// names, so if you want to change them you should ensure the change doesn't cause
	// a problem w.r.t. UX, docs, etc.

	// all of the test cases
	std::vector<TestCase> testCases =
	{
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
	for (TestCase const& tc : testCases)
	{
		ASSERT_TRUE(tc.maybeIndex) << tc.name << " does not exist in the registry(it should)";

		auto proto = osc::TypeRegistry<OpenSim::Joint>::prototypes()[*tc.maybeIndex];
		auto const& coordProp = proto->getProperty_coordinates();

		ASSERT_EQ(coordProp.size(), tc.expectedNames.size()) << tc.name <<  " has different number of coords from expected";

		for (int i = 0; i < coordProp.size(); ++i)
		{
			ASSERT_EQ(coordProp.getValue(i).getName(), tc.expectedNames[i]) << tc.name << " coordinate " << i << " has different name from expected";
		}
	}
}

// #298: try adding every available joint type into a blank OpenSim model to ensure
//       that all joint types can be added without an exception/segfault
TEST(JointRegistry, CanAddAnyJointWithoutAnExceptionOrSegfault)
{
	for (std::shared_ptr<OpenSim::Joint const> const& prototype : osc::JointRegistry::prototypes())
	{
        ASSERT_TRUE(prototype != nullptr);

		// create a blank model
        OpenSim::Model model;

        // create a body
        auto body = std::make_unique<OpenSim::Body>();
        body->setName("onebody");
        body->setMass(1.0);  // required

        // create joint between the model's ground and the body
        std::unique_ptr<OpenSim::Joint> joint{prototype->clone()};
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

// #298: try adding every available contact geometry type into a blank OpenSim model
//       to ensure that all contact geometries can be added without an exception/segfault
TEST(ContactGeometryRegistry, CanAddAnyContactGeometryWithoutAnExceptionOrSegfault)
{
	for (std::shared_ptr<OpenSim::ContactGeometry const> const& prototype : osc::ContactGeometryRegistry::prototypes())
	{
        ASSERT_TRUE(prototype != nullptr);

		// create a blank model
        OpenSim::Model model;

		// create contact geometry attached to model's ground frame
        std::unique_ptr<OpenSim::ContactGeometry> geom{prototype->clone()};
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
TEST(ConstraintRegistry, CanAddAnyConstraintWithoutASegfault)
{
    for (std::shared_ptr<OpenSim::Constraint const> const& prototype : osc::ConstraintRegistry::prototypes())
	{
        ASSERT_TRUE(prototype != nullptr);

        // create a blank model
        OpenSim::Model model;

        // default-construct the constraint
        std::unique_ptr<OpenSim::Constraint> constraint{prototype->clone()};

        // add it to the model
        model.addConstraint(constraint.release());

        // initialize the model+system+state
		try
		{
            model.finalizeFromProperties();
            model.buildSystem();
		}
		catch (std::exception const&)
		{
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
TEST(ForceRegistry, CanAddAnyForceWithoutASegfault)
{
    for (std::shared_ptr<OpenSim::Force const> const& prototype : osc::ForceRegistry::prototypes())
	{
        ASSERT_TRUE(prototype != nullptr);

        // create a blank model
        OpenSim::Model model;

        // default-construct the force
        std::unique_ptr<OpenSim::Force> force{prototype->clone()};

        // initialize the model+system+state
        try
		{
			model.addForce(force.release());  // finalizes, so can throw
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (std::exception const&)
		{
            // ok: it might throw because the constraint might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}

// #298: try adding every available controller to a blank OpenSim model
//       to ensure that all of them can be added without a segfault
TEST(ControllerRegistry, CanAddAnyControllerWithoutASegfault)
{
    for (std::shared_ptr<OpenSim::Controller const> const& prototype : osc::ControllerRegistry::prototypes())
	{
        ASSERT_TRUE(prototype != nullptr);

        // create a blank model
        OpenSim::Model model;

        // default-construct the controller
        std::unique_ptr<OpenSim::Controller> controller{prototype->clone()};

        // add it to the model
        model.addController(controller.release());

        // initialize the model+system+state
        try
        {
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (std::exception const&)
        {
            // ok: it might throw because the controller might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}

// #298: try adding every available probe type to a blank OpenSim model
//       to ensure that all of them can be added without a segfault
TEST(ProbeRegistry, CanAddAnyProbeWithoutASegfault)
{
    for (std::shared_ptr<OpenSim::Probe const> const& prototype : osc::ProbeRegistry::prototypes())
	{
        ASSERT_TRUE(prototype != nullptr);

        // create a blank model
        OpenSim::Model model;

        // default-construct the probe
        std::unique_ptr<OpenSim::Probe> probe{prototype->clone()};

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
TEST(UngroupedRegistry, CanAddAnyUngroupedComponentWithoutASegfault)
{
    for (std::shared_ptr<OpenSim::Component const> const& prototype : osc::UngroupedRegistry::prototypes())
    {
        ASSERT_TRUE(prototype != nullptr);

        // create a blank model
        OpenSim::Model model;

        // default-construct the component
        std::unique_ptr<OpenSim::Component> component{prototype->clone()};

        try
        {
            model.addComponent(component.release());
            model.finalizeFromProperties();
            model.buildSystem();
        }
        catch (std::exception const&)
        {
            // ok: it might throw because the component might need more information
            //
            // (but it definitely shouldn't segfault etc. - the error should be recoverable)
        }
    }
}