#pragma once

#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/Cubemap.h>
#include <liboscar/graphics/RenderTexture.h>
#include <liboscar/graphics/SharedColorRenderBuffer.h>
#include <liboscar/graphics/SharedDepthStencilRenderBuffer.h>
#include <liboscar/graphics/Texture2D.h>
#include <liboscar/maths/Matrix3x3.h>
#include <liboscar/maths/Matrix4x4.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/maths/Vector4.h>
#include <liboscar/utils/Typelist.h>

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
