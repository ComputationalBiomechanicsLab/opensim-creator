#include <oscar/Graphics/MeshTopology.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <sstream>

using namespace osc;

TEST(MeshTopology, can_be_written_to_a_std_ostream)
{
    for (size_t i = 0; i < num_options<MeshTopology>(); ++i) {
        const auto topology = static_cast<MeshTopology>(i);

        std::stringstream ss;
        ss << topology;

        ASSERT_FALSE(ss.str().empty());
    }
}
