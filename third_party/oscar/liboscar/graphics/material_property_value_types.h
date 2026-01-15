#pragma once

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/cubemap.h>
#include <liboscar/graphics/render_texture.h>
#include <liboscar/graphics/shared_color_render_buffer.h>
#include <liboscar/graphics/shared_depth_stencil_render_buffer.h>
#include <liboscar/graphics/texture2_d.h>
#include <liboscar/maths/matrix3x3.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/maths/vector4.h>
#include <liboscar/utils/typelist.h>

// Higher-order macro for generating code related to material properties
//
// This should match `MaterialPropertyValueTypes`. Yes, macros suck, but C++26 (reflection)
// is required to work around it.
#define OSC_FOR_EACH_MATERIAL_PROPERTY_VALUE_TYPE(X)  \
    X(bool)                                     \
    X(osc::Color)                               \
    X(osc::Color32)                             \
    X(osc::Cubemap)                             \
    X(float)                                    \
    X(int)                                      \
    X(osc::Matrix3x3)                           \
    X(osc::Matrix4x4)                           \
    X(osc::RenderTexture)                       \
    X(osc::SharedColorRenderBuffer)             \
    X(osc::SharedDepthStencilRenderBuffer)      \
    X(osc::Texture2D)                           \
    X(osc::Vector2)                             \
    X(osc::Vector3)                             \
    X(osc::Vector4)                             \

namespace osc
{
    using MaterialPropertyValueTypes = Typelist<
        bool,
        Color,
        Color32,
        Cubemap,
        float,
        int,
        Matrix3x3,
        Matrix4x4,
        RenderTexture,
        SharedColorRenderBuffer,
        SharedDepthStencilRenderBuffer,
        Texture2D,
        Vector2,
        Vector3,
        Vector4
    >;
}
