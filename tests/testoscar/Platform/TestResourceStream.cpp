#include <oscar/Platform/ResourceStream.hpp>

#include <testoscar/testoscarconfig.hpp>

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
        std::istreambuf_iterator<char> beg{is}, end;
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
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";
    ResourceStream rs{path};

    ASSERT_TRUE(slurp(rs) == slurp(path));
}

TEST(ResourceStream, WhenConstructedFromFilepathHasNameEqualtoFilename)
{
    auto const path = std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar" / "awesomeface.png";
    ASSERT_EQ(ResourceStream{path}.name(), path.filename().string());
}
