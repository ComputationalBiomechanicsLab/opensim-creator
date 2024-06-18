#include <OpenSimCreator/Documents/ModelWarper/CachedModelWarper.h>

#include <gtest/gtest.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpDocument.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

using namespace osc;
using namespace osc::mow;

TEST(CachedModelWarper, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW({ CachedModelWarper{}; });
}

TEST(CachedModelWarper, CanWarpDefaultConstructedModelWarpingDocument)
{
    ModelWarpDocument document;
    CachedModelWarper warper;

    const auto rv = warper.warp(document);
    ASSERT_NE(rv, nullptr);

    // ... and it can be copied into a new model that can be initialized etc. ready for UI usage
    OpenSim::Model copy{rv->getModel()};
    InitializeModel(copy);
    InitializeState(copy);
}
