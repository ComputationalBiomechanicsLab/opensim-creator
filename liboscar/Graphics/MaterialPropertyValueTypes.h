#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Cubemap.h>
#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Graphics/SharedColorRenderBuffer.h>
#include <liboscar/Graphics/SharedDepthStencilRenderBuffer.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Maths/Vector4.h>
#include <liboscar/Utils/Typelist.h>

// Higher-order macro for generating code related to material properties
//
// This should match `MaterialPropertyValueTypes`. Yes, macros suck, but C++26 (reflection)
// is required to work around it.
#define OSC_FOR_EACH_MATERIAL_PROPERTY_VALUE_TYPE(X)  \
    X(bool)                                     \
    X(osc::Color)                               \
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
