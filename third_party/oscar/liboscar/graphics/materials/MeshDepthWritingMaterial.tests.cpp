#include "MeshDepthWritingMaterial.h"

#include <liboscar/platform/App.h>

#include <gtest/gtest.h>

#include <memory>

using namespace osc;

namespace
{
    std::unique_ptr<App> g_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class MeshDepthWritingMaterialFixture : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            g_app = std::make_unique<App>();
        }

        static void TearDownTestSuite()
        {
            g_app.reset();
        }
    };
}

TEST_F(MeshDepthWritingMaterialFixture, works)
{
    [[maybe_unused]] const MeshDepthWritingMaterial default_constructed;  // should compile, run, etc.
}
