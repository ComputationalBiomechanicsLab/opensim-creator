#include "model_state_stage.h"

#include <gtest/gtest.h>
#include <liboscar/utilities/enum_helpers.h>

#include <format>
#include <string_view>

using namespace opyn;

TEST(ModelStateStage, to_string_view_returns_expected_results)
{

    static_assert(osc::num_options_v<ModelStateStage> == 9);
    ASSERT_EQ(to_string_view(ModelStateStage::topology), "ModelStateStage.TOPOLOGY");
    ASSERT_EQ(to_string_view(ModelStateStage::model), "ModelStateStage.MODEL");
    ASSERT_EQ(to_string_view(ModelStateStage::instance), "ModelStateStage.INSTANCE");
    ASSERT_EQ(to_string_view(ModelStateStage::time), "ModelStateStage.TIME");
    ASSERT_EQ(to_string_view(ModelStateStage::position), "ModelStateStage.POSITION");
    ASSERT_EQ(to_string_view(ModelStateStage::velocity), "ModelStateStage.VELOCITY");
    ASSERT_EQ(to_string_view(ModelStateStage::dynamics), "ModelStateStage.DYNAMICS");
    ASSERT_EQ(to_string_view(ModelStateStage::acceleration), "ModelStateStage.ACCELERATION");
    ASSERT_EQ(to_string_view(ModelStateStage::report), "ModelStateStage.REPORT");
}

TEST(ModelStateStage, to_string_view_returns_invalid_when_given_invalid_value)
{
    static_assert(osc::num_options_v<ModelStateStage> == 9);
    ASSERT_EQ(to_string_view(static_cast<ModelStateStage>(1337)), "ModelStateStage.INVALID");
}

TEST(ModelStateStage, formatter_returns_expected_results)
{

    static_assert(osc::num_options_v<ModelStateStage> == 9);
    ASSERT_EQ(std::format("{}", ModelStateStage::topology), "ModelStateStage.TOPOLOGY");
    ASSERT_EQ(std::format("{}", ModelStateStage::model), "ModelStateStage.MODEL");
    ASSERT_EQ(std::format("{}", ModelStateStage::instance), "ModelStateStage.INSTANCE");
    ASSERT_EQ(std::format("{}", ModelStateStage::time), "ModelStateStage.TIME");
    ASSERT_EQ(std::format("{}", ModelStateStage::position), "ModelStateStage.POSITION");
    ASSERT_EQ(std::format("{}", ModelStateStage::velocity), "ModelStateStage.VELOCITY");
    ASSERT_EQ(std::format("{}", ModelStateStage::dynamics), "ModelStateStage.DYNAMICS");
    ASSERT_EQ(std::format("{}", ModelStateStage::acceleration), "ModelStateStage.ACCELERATION");
    ASSERT_EQ(std::format("{}", ModelStateStage::report), "ModelStateStage.REPORT");
}

TEST(ModelStateStage, to_string_view_returns_INVALID_when_given_invalid_value)
{
    static_assert(osc::num_options_v<ModelStateStage> == 9);
    ASSERT_EQ(std::format("{}", static_cast<ModelStateStage>(1337)), "ModelStateStage.INVALID");
}
