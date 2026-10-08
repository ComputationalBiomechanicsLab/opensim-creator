#include "muscle_color_source_scaling.h"

#include <gtest/gtest.h>
#include <liboscar/utilities/enum_helpers.h>

#include <type_traits>
#include <utility>

using namespace opyn;

TEST(MuscleColorSourceScaling, GetAllPossibleMuscleColorSourceScalingMetadataReturnsExpectedNumberOfEntries)
{
    ASSERT_EQ(get_all_possible_muscle_color_source_scaling_metadata().size(), osc::num_options<MuscleColorSourceScaling>());
}

TEST(MuscleColorSourceScaling,  GetMuscleColorSourceScalingMetadataWorksForAllOptions)
{
    for (std::underlying_type_t<MuscleColorSourceScaling> i = 0; i < std::to_underlying(MuscleColorSourceScaling::NUM_OPTIONS); ++i) {
        ASSERT_NO_THROW({ get_muscle_color_source_scaling_metadata(static_cast<MuscleColorSourceScaling>(i)); });
    }
}

TEST(MuscleColorSourceScaling, GetIndexOfReturnsValidIndices)
{
    for (std::underlying_type_t<MuscleColorSourceScaling> i = 0; i < std::to_underlying(MuscleColorSourceScaling::NUM_OPTIONS); ++i) {
        ASSERT_EQ(get_index_of(static_cast<MuscleColorSourceScaling>(i)), i);
    }
}
