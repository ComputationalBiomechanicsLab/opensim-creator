#include <oscar/Platform/ResourceStream.h>

#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <string>

using namespace osc;

namespace
{
    std::string slurp(std::istream& is)
    {
        std::istreambuf_iterator<char> beg{is};
        std::istreambuf_iterator<char> end;
        return std::string{beg, end};
    }

    std::string slurp(std::filesystem::path const& p)
    {
        std::ifstream fs{p, std::ios::binary};
        return slurp(fs);
    }
}

TEST(ResourceStream, WhenDefaultConstructedYieldsAnEmptyStream)
{
    ASSERT_EQ(slurp(ResourceStream{}), std::string{});
}

TEST(ResourceStream, WhenDefaultConstructedHasNameNullstream)
{
    ASSERT_EQ(ResourceStream{}.name(), "nullstream");
}

TEST(ResourceStream, WhenConstructedFromFilepathCreatesStreamThatReadsFile)
{
    auto const path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    ResourceStream rs{path};

    ASSERT_TRUE(slurp(rs) == slurp(path));
}

TEST(ResourceStream, WhenConstructedFromFilepathHasNameEqualtoFilename)
{
    auto const path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    ASSERT_EQ(ResourceStream{path}.name(), path.filename().string());
}
