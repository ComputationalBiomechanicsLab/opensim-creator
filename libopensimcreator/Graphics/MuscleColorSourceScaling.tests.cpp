#include "MuscleColorSourceScaling.h"

#include <gtest/gtest.h>
#include <liboscar/utils/EnumHelpers.h>

#include <type_traits>
#include <utility>

using namespace osc;

TEST(MuscleColorSourceScaling, GetAllPossibleMuscleColorSourceScalingMetadataReturnsExpectedNumberOfEntries)
{
    ASSERT_EQ(GetAllPossibleMuscleColorSourceScalingMetadata().size(), num_options<MuscleColorSourceScaling>());
}

TEST(MuscleColorSourceScaling,  GetMuscleColorSourceScalingMetadataWorksForAllOptions)
{
    using underlying = std::underlying_type_t<MuscleColorSourceScaling>;

    for (underlying i = 0; i < std::to_underlying(MuscleColorSourceScaling::NUM_OPTIONS); ++i) {
        ASSERT_NO_THROW({ GetMuscleColorSourceScalingMetadata(static_cast<MuscleColorSourceScaling>(i)); });
    }
}

TEST(MuscleColorSourceScaling, GetIndexOfReturnsValidIndices)
{
    using underlying = std::underlying_type_t<MuscleColorSourceScaling>;

    for (underlying i = 0; i < std::to_underlying(MuscleColorSourceScaling::NUM_OPTIONS); ++i) {
        ASSERT_EQ(GetIndexOf(static_cast<MuscleColorSourceScaling>(i)), i);
    }
}
