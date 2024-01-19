#include "OpenSimCreator/Documents/FrameDefinition/StationDefinedFrame.hpp"

#include <gtest/gtest.h>

using osc::fd::StationDefinedFrame;

TEST(StationDefinedFrame, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW({ StationDefinedFrame{}; });
}
