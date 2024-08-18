#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/CullMode.h>
#include <oscar/Graphics/DepthFunction.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <concepts>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace osc
{
    class Material final {
    public:
        explicit Material(Shader);

        const Shader& shader() const;

        template<typename T, std::convertible_to<std::string_view> StringLike>
        std::optional<T> get(StringLike&& property_name) const
        {
            return properties().get<T>(std::forward<StringLike>(property_name));
        }

        template<typename T, std::convertible_to<std::string_view> StringLike>
        void set(StringLike&& property_name, const T& value)
        {
            upd_properties().set<T>(std::forward<StringLike>(property_name), value);
        }

        template<typename T, std::convertible_to<std::string_view> StringLike>
        std::optional<std::span<const T>> get_array(StringLike&& property_name) const
        {
            return properties().get_array<T>(std::forward<StringLike>(property_name));
        }

        template<typename T, std::convertible_to<std::string_view> StringLike>
        void set_array(StringLike&& property_name, std::span<const T> values)
        {
            upd_properties().set_array<T>(std::forward<StringLike>(property_name), values);
        }

        template<std::convertible_to<std::string_view> StringLike>
        void unset(StringLike&& property_name)
        {
            upd_properties().unset(std::forward<StringLike>(property_name));
        }

        bool is_transparent() const;
        void set_transparent(bool);

        bool is_depth_tested() const;
        void set_depth_tested(bool);

        DepthFunction depth_function() const;
        void set_depth_function(DepthFunction);

        bool is_wireframe() const;
        void set_wireframe(bool);

        CullMode cull_mode() const;
        void set_cull_mode(CullMode);

        friend bool operator==(const Material&, const Material&) = default;

    private:
        const MaterialPropertyBlock& properties() const;
        MaterialPropertyBlock& upd_properties();

        friend std::ostream& operator<<(std::ostream&, const Material&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Material&);
}
