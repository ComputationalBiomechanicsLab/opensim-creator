#pragma once

#include <oscar/Graphics/ShaderPropertyType.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <sstream>

using namespace osc;

TEST(ShaderPropertyType, can_be_written_to_ostream)
{
    std::stringstream ss;
    ss << ShaderPropertyType::Bool;

    ASSERT_EQ(ss.str(), "Bool");
}

TEST(ShaderPropertyType, can_be_iterated_over_and_all_can_be_written_to_ostream)
{
    for (size_t i = 0; i < num_options<ShaderPropertyType>(); ++i) {
        // shouldn't crash - if it does then we've missed a case somewhere
        std::stringstream ss;
        ss << static_cast<ShaderPropertyType>(i);
    }
}
