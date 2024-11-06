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
        const std::istreambuf_iterator<char> end;
        return std::string{beg, end};
    }

    std::string slurp(const std::filesystem::path& p)
    {
        std::ifstream fs{p, std::ios::binary};
        return slurp(fs);
    }
}

TEST(ResourceStream, yields_an_empty_string_when_default_constructed)
{
    ASSERT_EQ(slurp(ResourceStream{}), std::string{});
}

TEST(ResourceStream, name_returns_nullstream_when_default_constructed)
{
    ASSERT_EQ(ResourceStream{}.name(), "nullstream");
}

TEST(ResourceStream, yields_content_of_a_file_when_constructed_from_filesystem_path)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    ResourceStream rs{path};

    ASSERT_TRUE(slurp(rs) == slurp(path));
}

TEST(ResourceStream, name_returns_name_of_filesystem_file_when_constructed_from_a_filesystem_path)
{
    const auto path = std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png";
    ASSERT_EQ(ResourceStream{path}.name(), path.filename().string());
}
