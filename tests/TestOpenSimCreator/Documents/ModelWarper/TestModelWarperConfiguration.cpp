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

TEST(ModelWarperConfiguration, CanSaveDefaultConstructedToXMLFile)
{
	TemporaryFile temporary_file;
	temporary_file.close();  // so that `OpenSim::Object::print`'s implementation can open+write to it

	ModelWarperConfiguration configuration;
	configuration.print(temporary_file.absolute_path().string());
}

TEST(ModelWarperConfiguration, LoadingNonExistentFileThrows)
{
	ASSERT_ANY_THROW({ [[maybe_unused]] ModelWarperConfiguration configuration{GetFixturePath("doesnt_exist")}; });
}
