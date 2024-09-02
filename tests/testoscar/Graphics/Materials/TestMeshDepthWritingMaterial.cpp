#include <oscar/Graphics/Materials/MeshDepthWritingMaterial.h>

#include <oscar/Platform/App.h>
#include <gtest/gtest.h>

using namespace osc;

TEST(MeshDepthWritingMaterial, can_default_construct)
{
    App app;
    [[maybe_unused]] MeshDepthWritingMaterial default_constructed;  // should compile, run, etc.
}
