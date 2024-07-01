#include <OpenSimCreator/Documents/ModelWarper/ModelWarperConfiguration.h>

#include <TestOpenSimCreator/TestOpenSimCreatorConfig.h>

#include <gtest/gtest.h>

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
	ModelWarperConfiguration configuration;
	configuration.print(R"(C:\Users\adamk\Desktop\somefile.xml)");
}

TEST(ModelWarperConfiguration, LoadingNonExistentFileThrows)
{
	ASSERT_ANY_THROW({ [[maybe_unused]] ModelWarperConfiguration configuration{GetFixturePath("doesnt_exist")}; });
}

TEST(ModelWarperConfiguration, LoadingMalformedFileThrows)
{
	ModelWarperConfiguration configuration{GetFixturePath("Document/ModelWarper/ModelWarperConfiguration/malformed.xml")};
}

