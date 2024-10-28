#include <OpenSimCreator/Graphics/MuscleColorSourceScaling.h>

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/EnumHelpers.h>

#include <gtest/gtest.h>

#include <type_traits>

using namespace osc;

TEST(MuscleColorSourceScaling, GetAllPossibleMuscleColorSourceScalingMetadataReturnsExpectedNumberOfEntries)
{
    ASSERT_EQ(GetAllPossibleMuscleColorSourceScalingMetadata().size(), num_options<MuscleColorSourceScaling>());
}

TEST(MuscleColorSourceScaling,  GetMuscleColorSourceScalingMetadataWorksForAllOptions)
{
    using underlying = std::underlying_type_t<MuscleColorSourceScaling>;

    for (underlying i = 0; i < cpp23::to_underlying(MuscleColorSourceScaling::NUM_OPTIONS); ++i) {
        ASSERT_NO_THROW({ GetMuscleColorSourceScalingMetadata(static_cast<MuscleColorSourceScaling>(i)); });
    }
}

TEST(MuscleColorSourceScaling, GetIndexOfReturnsValidIndices)
{
    using underlying = std::underlying_type_t<MuscleColorSourceScaling>;

    for (underlying i = 0; i < cpp23::to_underlying(MuscleColorSourceScaling::NUM_OPTIONS); ++i) {
        ASSERT_EQ(GetIndexOf(static_cast<MuscleColorSourceScaling>(i)), i);
    }
}
