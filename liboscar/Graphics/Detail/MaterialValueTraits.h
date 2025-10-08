#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Cubemap.h>
#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Graphics/ShaderPropertyType.h>
#include <liboscar/Graphics/SharedColorRenderBuffer.h>
#include <liboscar/Graphics/SharedDepthStencilRenderBuffer.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Graphics/TextureDimensionality.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Maths/Vector4.h>
#include <liboscar/Utils/Assertions.h>

#include <algorithm>
#include <span>

namespace osc::detail
{
    // Returns the `ShaderPropertyType` (sampler) for the given `TextureDimensionality`.
    constexpr ShaderPropertyType to_sampler_shader_property(TextureDimensionality d)
    {
        static_assert(num_options<TextureDimensionality>() == 2);
        return d == TextureDimensionality::Tex2D ? ShaderPropertyType::Sampler2D : ShaderPropertyType::SamplerCube;
    }

    // Forward declaration of a template that must be specialized for each material value base type.
    template<typename>
    struct MaterialValueTraits;

    template<>
    struct MaterialValueTraits<Color> final {
        static constexpr void constructor_assertions(std::span<const Color>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Color>) { return ShaderPropertyType::Vector4; }
    };

    template<>
    struct MaterialValueTraits<float> final {
        static constexpr void constructor_assertions(std::span<const float>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const float>) { return ShaderPropertyType::Float; }

    };

    template<>
    struct MaterialValueTraits<Vector2> final {
        static constexpr void constructor_assertions(std::span<const Vector2>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Vector2>) { return ShaderPropertyType::Vector2; }
    };

    template<>
    struct MaterialValueTraits<Vector3> final {
        static constexpr void constructor_assertions(std::span<const Vector3>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Vector3>) { return ShaderPropertyType::Vector3; }
    };

    template<>
    struct MaterialValueTraits<Vector4> final {
        static constexpr void constructor_assertions(std::span<const Vector4>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Vector4>) { return ShaderPropertyType::Vector4; }
    };

    template<>
    struct MaterialValueTraits<Matrix3x3> final {
        static constexpr void constructor_assertions(std::span<const Matrix3x3>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Matrix3x3>) { return ShaderPropertyType::Matrix3x3; }
    };

    template<>
    struct MaterialValueTraits<Matrix4x4> final {
        static constexpr void constructor_assertions(std::span<const Matrix4x4>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Matrix4x4>) { return ShaderPropertyType::Matrix4x4; }
    };

    template<>
    struct MaterialValueTraits<int> final {
        static constexpr void constructor_assertions(std::span<const int>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const int>) { return ShaderPropertyType::Int; }
    };

    template<>
    struct MaterialValueTraits<bool> final {
        static constexpr void constructor_assertions(std::span<const bool>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const bool>) { return ShaderPropertyType::Bool; }
    };

    template<>
    struct MaterialValueTraits<Texture2D> final {
        static constexpr void constructor_assertions(std::span<const Texture2D>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Texture2D>) { return ShaderPropertyType::Sampler2D; }
    };

    template<>
    struct MaterialValueTraits<Cubemap> final {
        static constexpr void constructor_assertions(std::span<const Cubemap>) {}
        static constexpr ShaderPropertyType shader_property_type(std::span<const Cubemap>) { return ShaderPropertyType::SamplerCube; }
    };

    template<>
    struct MaterialValueTraits<RenderTexture> final {
        static void constructor_assertions(std::span<const RenderTexture> render_textures)
        {
            OSC_ASSERT(not render_textures.empty());
            const auto dimensionality = render_textures.front().dimensionality();
            const auto has_same_texture_dimensionality = [dimensionality](TextureDimensionality d) { return d == dimensionality; };
            OSC_ASSERT_ALWAYS(std::ranges::all_of(render_textures, has_same_texture_dimensionality, &RenderTexture::dimensionality));
        }

        static ShaderPropertyType shader_property_type(std::span<const RenderTexture> render_textures)
        {
            return to_sampler_shader_property(render_textures.front().dimensionality());
        }
    };

    template<>
    struct MaterialValueTraits<SharedColorRenderBuffer> final {
        static void constructor_assertions(std::span<const SharedColorRenderBuffer> render_buffers)
        {
            OSC_ASSERT(not render_buffers.empty());
            const auto dimensionality = render_buffers.front().dimensionality();
            const auto has_same_texture_dimensionality = [dimensionality](TextureDimensionality d) { return d == dimensionality; };
            OSC_ASSERT_ALWAYS(std::ranges::all_of(render_buffers, has_same_texture_dimensionality, &SharedColorRenderBuffer::dimensionality));
        }

        static ShaderPropertyType shader_property_type(std::span<const SharedColorRenderBuffer> render_buffers)
        {
            return to_sampler_shader_property(render_buffers.front().dimensionality());
        }
    };

    template<>
    struct MaterialValueTraits<SharedDepthStencilRenderBuffer> final {
        static void constructor_assertions(std::span<const SharedDepthStencilRenderBuffer> render_buffers)
        {
            OSC_ASSERT(not render_buffers.empty());
            const auto dimensionality = render_buffers.front().dimensionality();
            const auto has_same_texture_dimensionality = [dimensionality](TextureDimensionality d) { return d == dimensionality; };
            OSC_ASSERT_ALWAYS(std::ranges::all_of(render_buffers, has_same_texture_dimensionality, &SharedDepthStencilRenderBuffer::dimensionality));
        }

        static ShaderPropertyType shader_property_type(std::span<const SharedDepthStencilRenderBuffer> render_buffers)
        {
            return to_sampler_shader_property(render_buffers.front().dimensionality());
        }
    };
}
