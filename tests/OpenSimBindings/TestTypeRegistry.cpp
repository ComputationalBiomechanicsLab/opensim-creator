#include "src/OpenSimBindings/TypeRegistry.hpp"

#include "src/Utils/CStringView.hpp"

#include <gtest/gtest.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/GimbalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PlanarJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/SliderJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>

#include <cstddef>
#include <optional>
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