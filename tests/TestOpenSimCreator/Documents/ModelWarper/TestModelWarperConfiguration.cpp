#include <OpenSimCreator/Documents/ModelWarper/ModelWarperConfiguration.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>
#include <oscar/Utils/TemporaryFile.h>

#include <filesystem>

using namespace osc;
using namespace osc::mow;

namespace
{
	std::filesystem::path GetFixturePath(const std::filesystem::path& subpath)
	{
		return std::filesystem::weakly_canonical(std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / subpath);
	}
}

TEST(ModelWarperConfiguration, CanDefaultConstruct)
{
	[[maybe_unused]] ModelWarperConfiguration configuration;
}

TEST(ModelWarperConfiguration, CanSaveAndLoadDefaultConstructedToAndFromXMLFile)
{
	TemporaryFile temporaryFile;
	temporaryFile.close();  // so that `OpenSim::Object::print`'s implementation can open+write to it

	ModelWarperConfiguration configuration;
	configuration.print(temporaryFile.absolute_path().string());

	ModelWarperConfiguration loadedConfiguration{temporaryFile.absolute_path().string()};
	loadedConfiguration.finalizeFromProperties();
	loadedConfiguration.finalizeConnections(loadedConfiguration);
}

TEST(ModelWarperConfiguration, LoadingNonExistentFileThrows)
{
	ASSERT_ANY_THROW({ [[maybe_unused]] ModelWarperConfiguration configuration{GetFixturePath("doesnt_exist")}; });
}

TEST(ModelWarperConfiguration, CanLoadEmptySequence)
{
	ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/empty_sequence.xml")};
	configuration.finalizeFromProperties();
	configuration.finalizeConnections(configuration);
}

TEST(ModelWarperConfiguration, CanLoadTrivialSingleOffsetFrameWarpingStrategy)
{
	OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});

	ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/single_offsetframe_warper.xml")};
	configuration.finalizeFromProperties();
	configuration.finalizeConnections(configuration);

	auto range = configuration.getComponentList<ProduceErrorOffsetFrameWarpingStrategy>();
	const auto numEls = std::distance(range.begin(), range.end());
	ASSERT_EQ(numEls, 1);
}

TEST(ModelWarperConfiguration, CanContainAMixtureOfOffsetFrameWarpingStrategies)
{
	OpenSim::Object::registerType(ProduceErrorOffsetFrameWarpingStrategy{});
	OpenSim::Object::registerType(ThinPlateSplineOnlyTranslationOffsetFrameWarpingStrategy{});

	ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/mixed_offsetframe_warpers.xml")};
	configuration.finalizeFromProperties();
	configuration.finalizeConnections(configuration);

	auto range = configuration.getComponentList<OffsetFrameWarpingStrategy>();
	const auto numEls = std::distance(range.begin(), range.end());
	ASSERT_EQ(numEls, 2);
}

TEST(ModelWarperConfiguration, CanLoadTrivialSingleStationWarpingStrategy)
{
	OpenSim::Object::registerType(ProduceErrorStationWarpingStrategy{});

	ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/single_station_warper.xml")};
	configuration.finalizeFromProperties();
	configuration.finalizeConnections(configuration);

	auto range = configuration.getComponentList<ProduceErrorStationWarpingStrategy>();
	const auto numEls = std::distance(range.begin(), range.end());
	ASSERT_EQ(numEls, 1);
}
