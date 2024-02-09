#include <oscar/Graphics/CameraProjection.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <cstddef>
#include <sstream>
#include <string>

using osc::CameraProjection;
using osc::NumOptions;

TEST(CameraProjection, CanBeStreamed)
{
    for (size_t i = 0; i < NumOptions<CameraProjection>(); ++i)
    {
        std::stringstream ss;
        ss << static_cast<CameraProjection>(i);

        ASSERT_FALSE(ss.str().empty());
    }
}
