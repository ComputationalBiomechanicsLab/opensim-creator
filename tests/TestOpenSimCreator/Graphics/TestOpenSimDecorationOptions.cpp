#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>

#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Variant.h>
#include <gtest/gtest.h>

#include <string_view>
#include <unordered_map>

using namespace osc;

TEST(OpenSimDecorationOptions, RemembersColorScaling)
{
    OpenSimDecorationOptions opts;
    opts.setMuscleColorSourceScaling(MuscleColorSourceScaling::ModelWide);
    bool emitted = false;
    opts.forEachOptionAsAppSettingValue([&emitted](std::string_view k, const Variant& v)
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
    const std::unordered_map<std::string, Variant> lookup = {
        {"muscle_color_scaling", "model_wide"},
    };

    OpenSimDecorationOptions opts;
    ASSERT_NE(opts.getMuscleColorSourceScaling(), MuscleColorSourceScaling::ModelWide);
    opts.tryUpdFromValues("", lookup);
    ASSERT_EQ(opts.getMuscleColorSourceScaling(), MuscleColorSourceScaling::ModelWide);
}
