#include <oscar/Graphics/MaterialPropertyBlock.h>

#include <testoscar/TestingHelpers.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Utils/StringHelpers.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::testing;

namespace
{
    Texture2D generate_red_texture()
    {
        Texture2D rv{Vec2i{2, 2}};
        rv.set_pixels(std::vector<Color>(4, Color::red()));
        return rv;
    }
}

TEST(MaterialPropertyBlock, can_default_construct)
{
    const MaterialPropertyBlock mpb;
}

TEST(MaterialPropertyBlock, can_copy_construct)
{
    const MaterialPropertyBlock mpb;
    const MaterialPropertyBlock copy{mpb};
}

TEST(MaterialPropertyBlock, can_move_construct)
{
    MaterialPropertyBlock mpb;
    const MaterialPropertyBlock copy{std::move(mpb)};
}

TEST(MaterialPropertyBlock, can_copy_assign)
{
    MaterialPropertyBlock m1;
    const MaterialPropertyBlock m2;

    m1 = m2;
}

TEST(MaterialPropertyBlock, can_move_assign)
{
    MaterialPropertyBlock m1;
    const MaterialPropertyBlock m2;

    m1 = std::move(m2);
}

TEST(MaterialPropertyBlock, is_empty_on_construction)
{
    const MaterialPropertyBlock mpb;

    ASSERT_TRUE(mpb.empty());
}

TEST(MaterialPropertyBlock, can_clear_default_constructed)
{
    MaterialPropertyBlock mpb;
    mpb.clear();

    ASSERT_TRUE(mpb.empty());
}

TEST(MaterialPropertyBlock, clear_clears_properties)
{
    MaterialPropertyBlock mpb;

    mpb.set<float>("someKey", generate<float>());

    ASSERT_FALSE(mpb.empty());

    mpb.clear();

    ASSERT_TRUE(mpb.empty());
}

TEST(MaterialPropertyBlock, get_color_on_empty_returns_nullopt)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Color>("someKey"));
}

TEST(MaterialPropertyBlock, can_call_set_color)
{
    MaterialPropertyBlock mpb;
    mpb.set<Color>("someKey", Color::red());
}

TEST(MaterialPropertyBlock, calling_get_Color_after_set_Color_returns_the_color)
{
    MaterialPropertyBlock mpb;
    mpb.set<Color>("someKey", Color::red());

    ASSERT_EQ(mpb.get<Color>("someKey"), Color::red());
}

TEST(MaterialPropertyBlock, get_float_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<float>("someKey"));
}

TEST(MaterialPropertyBlock, get_Vec3_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Vec3>("someKey"));
}

TEST(MaterialPropertyBlock, get_Vec4_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Vec4>("someKey"));
}

TEST(MaterialPropertyBlock, get_Mat3_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Mat3>("someKey"));
}

TEST(MaterialPropertyBlock, get_Mat4_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Mat4>("someKey"));
}

TEST(MaterialPropertyBlock, get_int_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<int>("someKey"));
}

TEST(MaterialPropertyBlock, get_bool_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<bool>("someKey"));
}

TEST(MaterialPropertyBlock, set_float_causes_get_float_to_return_the_float)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const float value = generate<float>();

    ASSERT_FALSE(mpb.get<float>(key));

    mpb.set<float>(key, value);
    ASSERT_TRUE(mpb.get<float>(key));
    ASSERT_EQ(mpb.get<float>(key), value);
}

TEST(MaterialPropertyBlock, set_Vec3_causes_get_Vec3_to_return_the_Vec3)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const Vec3 value = generate<Vec3>();

    ASSERT_FALSE(mpb.get<Vec3>(key));

    mpb.set<Vec3>(key, value);
    ASSERT_TRUE(mpb.get<Vec3>(key));
    ASSERT_EQ(mpb.get<Vec3>(key), value);
}

TEST(MaterialPropertyBlock, set_Vec4_causes_get_Vec4_to_return_the_Vec4)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const Vec4 value = generate<Vec4>();

    ASSERT_FALSE(mpb.get<Vec4>(key));

    mpb.set<Vec4>(key, value);
    ASSERT_TRUE(mpb.get<Vec4>(key));
    ASSERT_EQ(mpb.get<Vec4>(key), value);
}

TEST(MaterialPropertyBlock, set_Mat3_causes_get_Mat3_to_return_the_Mat3)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const Mat3 value = generate<Mat3>();

    ASSERT_FALSE(mpb.get<Vec4>(key));

    mpb.set<Mat3>(key, value);
    ASSERT_TRUE(mpb.get<Mat3>(key));
    ASSERT_EQ(mpb.get<Mat3>(key), value);
}

TEST(MaterialPropertyBlock, set_int_causes_get_int_to_return_the_int)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const int value = generate<int>();

    ASSERT_FALSE(mpb.get<int>(key));

    mpb.set<int>(key, value);
    ASSERT_TRUE(mpb.get<int>(key));
    ASSERT_EQ(mpb.get<int>(key), value);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockSetBoolCausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    bool value = generate<bool>();

    ASSERT_FALSE(mpb.get<bool>(key));

    mpb.set<bool>(key, value);
    ASSERT_TRUE(mpb.get<bool>(key));
    ASSERT_EQ(mpb.get<bool>(key), value);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    MaterialPropertyBlock mpb;

    std::string key = "someKey";
    Texture2D t = generate_red_texture();

    ASSERT_FALSE(mpb.get<Texture2D>(key));

    mpb.set<Texture2D>(key, t);

    ASSERT_TRUE(mpb.get<Texture2D>(key));
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockSetSharedColorRenderBufferOnMaterialCausesGetRenderBufferToReturnTheRenderBuffer)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<SharedColorRenderBuffer>("someKey"));
    mpb.set("someKey", SharedColorRenderBuffer{});
    ASSERT_TRUE(mpb.get<SharedColorRenderBuffer>("someKey"));
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockSetSharedDepthRenderBufferOnMaterialCausesGetRenderBufferToReturnTheRenderBuffer)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<SharedDepthStencilRenderBuffer>("someKey"));
    mpb.set("someKey", SharedDepthStencilRenderBuffer{});
    ASSERT_TRUE(mpb.get<SharedDepthStencilRenderBuffer>("someKey"));
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCanCompareEquals)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    ASSERT_TRUE(m1 == m2);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCopyConstructionComparesEqual)
{
    MaterialPropertyBlock m;
    MaterialPropertyBlock copy{m};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(m, copy);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCopyAssignmentComparesEqual)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    m1.set<float>("someKey", generate<float>());

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockDifferentMaterialBlocksCompareNotEqual)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    m1.set<float>("someKey", generate<float>());

    ASSERT_NE(m1, m2);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCanPrintToOutputStream)
{
    MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockPrintingToOutputStreamMentionsMaterialPropertyBlock)
{
    MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;

    ASSERT_TRUE(contains(ss.str(), "MaterialPropertyBlock"));
}
