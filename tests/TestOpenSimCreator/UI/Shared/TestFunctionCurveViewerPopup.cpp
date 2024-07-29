#include <OpenSimCreator/UI/Shared/FunctionCurveViewerPopup.h>

#include <gtest/gtest.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Utils/Algorithms.h>

using namespace osc;

TEST(FunctionCurveViewerPopup, RelianceOnMinNanWorksAsExpected)
{
    // this is a bit of an implementation detail, but the internal
    // `PlotPoints` class that `FunctionCurveViewerPopup` uses relies
    // on this behavior because its nullstate (e.g. when there's 0
    // points) is that the x-range and y-range are NaNs

    ASSERT_EQ(min(1.0f, quiet_nan_v<float>), 1.0f);
}

TEST(FunctionCurveViewerPopup, RelianceOnMaxNanWorksAsExpected)
{
    // this is a bit of an implementation detail, but the internal
    // `PlotPoints` class that `FunctionCurveViewerPopup` uses relies
    // on this behavior because its nullstate (e.g. when there's 0
    // points) is that the x-range and y-range are NaNs

    ASSERT_EQ(max(1.0f, quiet_nan_v<float>), 1.0f);
}
