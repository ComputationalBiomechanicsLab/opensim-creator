#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/StringName.h>

#include <concepts>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

namespace osc
{
    // material property block
    //
    // enables callers to apply per-instance properties when using a material (more efficiently
    // than using a different `Material` every time)
    class MaterialPropertyBlock final {
    public:
        MaterialPropertyBlock();

        void clear();
        [[nodiscard]] bool empty() const;

        // calling `set` without a type argument (e.g. `set(prop, value);`) should coerce the value
        // to a `const T&` in order to call `set<T>(prop, value)`
        template<std::convertible_to<std::string_view> StringLike, typename T>
        void set(StringLike&& property_name, const T& value)
        {
            set<T>(std::forward<StringLike>(property_name), value);
        }

        // calling `set_array` without a type argument (e.g. `set_array(prop, some_range)`) deduces
        // the type from the range in order to call `set_array<T>(prop, some_range)`
        template<
            std::convertible_to<std::string_view> StringLike,
            std::ranges::contiguous_range Range
        >
        void set_array(StringLike&& property_name, Range&& values)
        {
            set_array<std::ranges::range_value_t<Range>>(std::forward<StringLike>(property_name), std::forward<Range>(values));
        }

        template<std::same_as<Color>> std::optional<Color> get(std::string_view property_name) const;
        template<std::same_as<Color>> std::optional<Color> get(const StringName& property_name) const;
        template<std::same_as<Color>> void set(std::string_view property_name, const Color&);
        template<std::same_as<Color>> void set(const StringName& property_name, const Color&);
        template<std::same_as<Color>> std::optional<std::span<const Color>> get_array(std::string_view property_name) const;
        template<std::same_as<Color>> std::optional<std::span<const Color>> get_array(const StringName& property_name) const;
        template<std::same_as<Color>> void set_array(std::string_view property_name, std::span<const Color>);
        template<std::same_as<Color>> void set_array(const StringName& property_name, std::span<const Color>);

        template<std::same_as<float>> std::optional<float> get(std::string_view property_name) const;
        template<std::same_as<float>> std::optional<float> get(const StringName& property_name) const;
        template<std::same_as<float>> void set(std::string_view property_name, const float&);
        template<std::same_as<float>> void set(const StringName& property_name, const float&);
        template<std::same_as<float>> std::optional<std::span<const float>> get_array(std::string_view property_name) const;
        template<std::same_as<float>> std::optional<std::span<const float>> get_array(const StringName& property_name) const;
        template<std::same_as<float>> void set_array(std::string_view property_name, std::span<const float>);
        template<std::same_as<float>> void set_array(const StringName& property_name, std::span<const float>);

        template<std::same_as<Vec2>> std::optional<Vec2> get(std::string_view property_name) const;
        template<std::same_as<Vec2>> std::optional<Vec2> get(const StringName& property_name) const;
        template<std::same_as<Vec2>> void set(std::string_view property_name, const Vec2&);
        template<std::same_as<Vec2>> void set(const StringName& property_name, const Vec2&);

        template<std::same_as<Vec3>> std::optional<Vec3> get(std::string_view property_name) const;
        template<std::same_as<Vec3>> std::optional<Vec3> get(const StringName& property_name) const;
        template<std::same_as<Vec3>> void set(std::string_view property_name, const Vec3&);
        template<std::same_as<Vec3>> void set(const StringName& property_name, const Vec3&);
        template<std::same_as<Vec3>> std::optional<std::span<const Vec3>> get_array(std::string_view property_name) const;
        template<std::same_as<Vec3>> std::optional<std::span<const Vec3>> get_array(const StringName& property_name) const;
        template<std::same_as<Vec3>> void set_array(std::string_view property_name, std::span<const Vec3>);
        template<std::same_as<Vec3>> void set_array(const StringName& property_name, std::span<const Vec3>);

        template<std::same_as<Vec4>> std::optional<Vec4> get(std::string_view property_name) const;
        template<std::same_as<Vec4>> std::optional<Vec4> get(const StringName& property_name) const;
        template<std::same_as<Vec4>> void set(std::string_view property_name, const Vec4&);
        template<std::same_as<Vec4>> void set(const StringName& property_name, const Vec4&);

        template<std::same_as<Mat3>> std::optional<Mat3> get(std::string_view property_name) const;
        template<std::same_as<Mat3>> std::optional<Mat3> get(const StringName& property_name) const;
        template<std::same_as<Mat3>> void set(std::string_view property_name, const Mat3&);
        template<std::same_as<Mat3>> void set(const StringName& property_name, const Mat3&);

        template<std::same_as<Mat4>> std::optional<Mat4> get(std::string_view property_name) const;
        template<std::same_as<Mat4>> std::optional<Mat4> get(const StringName& property_name) const;
        template<std::same_as<Mat4>> void set(std::string_view, const Mat4&);
        template<std::same_as<Mat4>> void set(const StringName& property_name, const Mat4&);
        template<std::same_as<Mat4>> std::optional<std::span<const Mat4>> get_array(std::string_view property_name) const;
        template<std::same_as<Mat4>> std::optional<std::span<const Mat4>> get_array(const StringName& property_name) const;
        template<std::same_as<Mat4>> void set_array(std::string_view property_name, std::span<const Mat4>);
        template<std::same_as<Mat4>> void set_array(const StringName& property_name, std::span<const Mat4>);

        template<std::same_as<int>> std::optional<int32_t> get(std::string_view property_name) const;
        template<std::same_as<int>> std::optional<int32_t> get(const StringName& property_name) const;
        template<std::same_as<int>> void set(std::string_view property_name, const int&);
        template<std::same_as<int>> void set(const StringName& property_name, const int&);

        template<std::same_as<bool>> std::optional<bool> get(std::string_view property_name) const;
        template<std::same_as<bool>> std::optional<bool> get(const StringName& property_name) const;
        template<std::same_as<bool>> void set(std::string_view property_name, bool);
        template<std::same_as<bool>> void set(const StringName& property_name, bool);

        template<std::same_as<Texture2D>> std::optional<Texture2D> get(std::string_view property_name) const;
        template<std::same_as<Texture2D>> std::optional<Texture2D> get(const StringName& property_name) const;
        template<std::same_as<Texture2D>> void set(std::string_view property_name, const Texture2D&);
        template<std::same_as<Texture2D>> void set(const StringName& property_name, const Texture2D&);

        template<std::same_as<RenderTexture>> std::optional<RenderTexture> get(std::string_view property_name) const;
        template<std::same_as<RenderTexture>> std::optional<RenderTexture> get(const StringName& property_name) const;
        template<std::same_as<RenderTexture>> void set(std::string_view property_name, const RenderTexture&);
        template<std::same_as<RenderTexture>> void set(const StringName& property_name, const RenderTexture&);

        template<std::same_as<Cubemap>> std::optional<Cubemap> get(std::string_view property_name) const;
        template<std::same_as<Cubemap>> std::optional<Cubemap> get(const StringName& property_name) const;
        template<std::same_as<Cubemap>> void set(std::string_view property_name, const Cubemap&);
        template<std::same_as<Cubemap>> void set(const StringName& property_name, const Cubemap&);

        void unset(std::string_view property_name);
        void unset(const StringName& property_name);

    private:
        friend bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
        friend std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    bool operator==(const MaterialPropertyBlock&, const MaterialPropertyBlock&);
    std::ostream& operator<<(std::ostream&, const MaterialPropertyBlock&);
}
