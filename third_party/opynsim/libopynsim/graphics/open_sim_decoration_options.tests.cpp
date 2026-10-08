#include "open_sim_decoration_options.h"

#include <gtest/gtest.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/string_helpers.h>
#include <liboscar/variant/variant.h>

#include <string_view>
#include <unordered_map>

using namespace opyn;

TEST(OpenSimDecorationOptions, RemembersColorScaling)
{
    OpenSimDecorationOptions opts;
    opts.set_muscle_color_source_scaling(MuscleColorSourceScaling::ModelWide);
    bool emitted = false;
    opts.for_each_option_as_app_setting_value([&emitted](std::string_view k, const osc::Variant& v)
    {
        // Yep, this is hard-coded: it's just here as a sanity check: change/remove
        // it if it's causing trouble.
        if (k == "muscle_color_scaling" and to<std::string>(v) == "model_wide") {
            emitted = true;
        }
    });
    ASSERT_TRUE(emitted);
}

TEST(OpenSimDecorationOptions, ReadsColorScalingFromDict)
{
    const std::unordered_map<std::string, osc::Variant> lookup = {
        {"muscle_color_scaling", osc::Variant{"model_wide"}},
    };

    OpenSimDecorationOptions opts;
    ASSERT_NE(opts.get_muscle_color_source_scaling(), MuscleColorSourceScaling::ModelWide);
    opts.try_upd_from_values("", lookup);
    ASSERT_EQ(opts.get_muscle_color_source_scaling(), MuscleColorSourceScaling::ModelWide);
}
