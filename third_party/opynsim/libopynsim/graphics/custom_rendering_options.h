#pragma once

#include <libopynsim/graphics/custom_rendering_option_flags.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace osc { struct SceneRendererParams; }

namespace opyn
{
    class CustomRenderingOptions final {
    public:
        size_t get_num_options() const;
        bool get_option_value(ptrdiff_t) const;
        void set_option_value(ptrdiff_t, bool);
        osc::CStringView get_option_label(ptrdiff_t) const;

        bool get_draw_floor() const;
        void set_draw_floor(bool);

        bool get_draw_mesh_normals() const;
        void set_draw_mesh_normals(bool);

        bool get_draw_shadows() const;
        void set_draw_shadows(bool);

        bool get_draw_selection_rims() const;
        void set_draw_selection_rims(bool);

        bool get_order_independent_transparency() const;
        void set_order_independent_transparency(bool);

        void for_each_option_as_app_setting_value(const std::function<void(std::string_view, const osc::Variant&)>&) const;
        void try_upd_from_values(std::string_view key_prefix, const std::unordered_map<std::string, osc::Variant>&);

        void apply_to(osc::SceneRendererParams&) const;

        friend bool operator==(const CustomRenderingOptions&, const CustomRenderingOptions&) = default;

    private:
        CustomRenderingOptionFlags flags_ = CustomRenderingOptionFlags::Default;
    };
}
