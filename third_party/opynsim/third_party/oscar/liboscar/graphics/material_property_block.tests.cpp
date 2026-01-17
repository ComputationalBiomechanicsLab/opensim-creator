#include "material_property_block.h"

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/texture2d.h>
#include <liboscar/maths/matrix3x3.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/maths/vector4.h>
#include <liboscar/tests/test_helpers.h>
#include <liboscar/utils/string_helpers.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rgs = std::ranges;
using namespace osc;
using namespace osc::tests;

namespace
{
    Texture2D generate_red_texture()
    {
        Texture2D rv{Vector2i{2, 2}};
        rv.set_pixels(std::vector<Color>(4, Color::red()));
        return rv;
    }

    RenderTexture generate_render_texture()
    {
        return RenderTexture{};
    }
}

TEST(MaterialPropertyBlock, can_default_construct)
{
    const MaterialPropertyBlock mpb;
}

TEST(MaterialPropertyBlock, can_copy_construct)
{
    const MaterialPropertyBlock mpb;
    const MaterialPropertyBlock copy{mpb};  // NOLINT(performance-unnecessary-copy-initialization)
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
    MaterialPropertyBlock m2;

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

TEST(MaterialPropertyBlock, get_Vector3_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Vector3>("someKey"));
}

TEST(MaterialPropertyBlock, get_Vector4_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Vector4>("someKey"));
}

TEST(MaterialPropertyBlock, get_Matrix3x3_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Matrix3x3>("someKey"));
}

TEST(MaterialPropertyBlock, get_Matrix4x4_returns_nullopt_on_default_constructed)
{
    const MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.get<Matrix4x4>("someKey"));
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

TEST(MaterialPropertyBlock, set_Vector3_causes_get_Vector3_to_return_the_Vector3)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const Vector3 value = generate<Vector3>();

    ASSERT_FALSE(mpb.get<Vector3>(key));

    mpb.set<Vector3>(key, value);
    ASSERT_TRUE(mpb.get<Vector3>(key));
    ASSERT_EQ(mpb.get<Vector3>(key), value);
}

TEST(MaterialPropertyBlock, set_Vector4_causes_get_Vector4_to_return_the_Vector4)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const Vector4 value = generate<Vector4>();

    ASSERT_FALSE(mpb.get<Vector4>(key));

    mpb.set<Vector4>(key, value);
    ASSERT_TRUE(mpb.get<Vector4>(key));
    ASSERT_EQ(mpb.get<Vector4>(key), value);
}

TEST(MaterialPropertyBlock, set_Matrix3x3_causes_get_Mat3_to_return_the_Mat3)
{
    MaterialPropertyBlock mpb;
    const std::string key = "someKey";
    const Matrix3x3 value = generate<Matrix3x3>();

    ASSERT_FALSE(mpb.get<Vector4>(key));

    mpb.set<Matrix3x3>(key, value);
    ASSERT_TRUE(mpb.get<Matrix3x3>(key));
    ASSERT_EQ(mpb.get<Matrix3x3>(key), value);
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
    const std::string key = "someKey";
    const bool value = generate<bool>();

    ASSERT_FALSE(mpb.get<bool>(key));

    mpb.set<bool>(key, value);
    ASSERT_TRUE(mpb.get<bool>(key));
    ASSERT_EQ(mpb.get<bool>(key), value);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    MaterialPropertyBlock mpb;

    const std::string key = "someKey";
    const Texture2D texture = generate_red_texture();

    ASSERT_FALSE(mpb.get<Texture2D>(key));

    mpb.set<Texture2D>(key, texture);

    ASSERT_TRUE(mpb.get<Texture2D>(key));
}

TEST(MaterialPropertyBlock, set_RenderTexture_causes_get_RenderTexture_to_return_the_RenderTexture)
{
    MaterialPropertyBlock mpb;

    const std::string key = "someKey";
    const RenderTexture render_texture = generate_render_texture();

    ASSERT_FALSE(mpb.get<RenderTexture>(key));
    mpb.set(key, render_texture);
    ASSERT_TRUE(mpb.get<RenderTexture>(key));
}

TEST(MaterialPropertyBlock, set_array_RenderTexture_causes_get_array_RenderTexture_to_return_same_sequence_of_RenderTextures)
{
    MaterialPropertyBlock mpb;

    const std::string key = "someKey";
    const std::vector<RenderTexture> render_textures = {generate_render_texture(), generate_render_texture()};

    ASSERT_FALSE(mpb.get_array<RenderTexture>(key));
    mpb.set_array(key, render_textures);
    const auto rv = mpb.get_array<RenderTexture>(key);
    ASSERT_TRUE(rv);
    ASSERT_TRUE(rgs::equal(render_textures, *rv));
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
    const MaterialPropertyBlock m1;
    const MaterialPropertyBlock m2;

    ASSERT_TRUE(m1 == m2);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCopyConstructionComparesEqual)
{
    const MaterialPropertyBlock m;
    const MaterialPropertyBlock copy{m};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(m, copy);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCopyAssignmentComparesEqual)
{
    MaterialPropertyBlock m1;
    const MaterialPropertyBlock m2;

    m1.set<float>("someKey", generate<float>());

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockDifferentMaterialBlocksCompareNotEqual)
{
    MaterialPropertyBlock m1;
    const MaterialPropertyBlock m2;

    m1.set<float>("someKey", generate<float>());

    ASSERT_NE(m1, m2);
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockCanPrintToOutputStream)
{
    const MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlock, MaterialPropertyBlockPrintingToOutputStreamMentionsMaterialPropertyBlock)
{
    const MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;

    ASSERT_TRUE(ss.str().contains("MaterialPropertyBlock"));
}

TEST(MaterialPropertyBlock, set_SharedDepthStencilBuffer_works)
{
    std::vector<SharedDepthStencilRenderBuffer> buffers(2);

    MaterialPropertyBlock block;
    ASSERT_FALSE(block.get_array<SharedDepthStencilRenderBuffer>("someKey"));
    block.set_array("someKey", buffers);
    ASSERT_TRUE(block.get_array<SharedDepthStencilRenderBuffer>("someKey"));
}
