#pragma once

#include <libopynsim/graphics/overlay_decoration_option_flags.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace opyn
{
    class OverlayDecorationOptions final {
    public:
        size_t get_num_options() const;
        bool get_option_value(ptrdiff_t) const;
        void set_option_value(ptrdiff_t, bool);
        osc::CStringView get_option_label(ptrdiff_t) const;
        osc::CStringView get_option_group_label(ptrdiff_t) const;

        bool get_draw_xz_grid() const;
        void set_draw_xz_grid(bool);

        bool get_draw_xy_grid() const;
        void set_draw_xy_grid(bool);

        bool get_draw_yz_grid() const;
        void set_draw_yz_grid(bool);

        bool get_draw_axis_lines() const;
        void set_draw_axis_lines(bool);

        bool get_draw_aabbs() const;
        void set_draw_aabbs(bool);

        bool get_draw_bvh() const;
        void set_draw_bvh(bool);

        void for_each_option_as_app_setting_value(const std::function<void(std::string_view, const osc::Variant&)>&) const;
        void try_upd_from_values(std::string_view keyPrefix, const std::unordered_map<std::string, osc::Variant>&);

        friend bool operator==(const OverlayDecorationOptions&, const OverlayDecorationOptions&) = default;

    private:
        OverlayDecorationOptionFlags flags_ = OverlayDecorationOptionFlags::Default;
    };
}
