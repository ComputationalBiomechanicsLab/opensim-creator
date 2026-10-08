#pragma once

#include <libopynsim/graphics/muscle_color_source_scaling.h>
#include <libopynsim/graphics/muscle_color_source.h>
#include <libopynsim/graphics/muscle_decoration_style.h>
#include <libopynsim/graphics/muscle_sizing_style.h>
#include <libopynsim/graphics/open_sim_decoration_option_flags.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/variant/variant.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace osc { class AppSettingValue; }

namespace opyn
{
    class OpenSimDecorationOptions final {
    public:
        OpenSimDecorationOptions();

        MuscleDecorationStyle get_muscle_decoration_style() const;
        void set_muscle_decoration_style(MuscleDecorationStyle);

        MuscleColorSource get_muscle_color_source() const;
        void set_muscle_color_source(MuscleColorSource);

        MuscleSizingStyle get_muscle_sizing_style() const;
        void set_muscle_sizing_style(MuscleSizingStyle);

        MuscleColorSourceScaling get_muscle_color_source_scaling() const;
        void set_muscle_color_source_scaling(MuscleColorSourceScaling);

        // the ones below here are toggle-able options with user-facing strings etc
        size_t get_num_options() const;
        bool get_option_value(ptrdiff_t) const;
        void set_option_value(ptrdiff_t, bool);
        osc::CStringView get_option_label(ptrdiff_t) const;
        std::optional<osc::CStringView> get_option_description(ptrdiff_t) const;

        bool get_should_show_scapulo() const;
        void set_should_show_scapulo(bool);

        bool get_should_show_effective_muscle_line_of_action_for_origin() const;
        void set_should_show_effective_muscle_line_of_action_for_origin(bool);

        bool get_should_show_effective_muscle_line_of_action_for_insertion() const;
        void set_should_show_effective_muscle_line_of_action_for_insertion(bool);

        bool get_should_show_anatomical_muscle_line_of_action_for_origin() const;
        void set_should_show_anatomical_muscle_line_of_action_for_origin(bool);

        bool get_should_show_anatomical_muscle_line_of_action_for_insertion() const;
        void set_should_show_anatomical_muscle_line_of_action_for_insertion(bool);

        bool get_should_show_centers_of_mass() const;
        void set_should_show_centers_of_mass(bool);

        bool get_should_show_point_to_point_springs() const;
        void set_should_show_point_to_point_springs(bool);

        bool get_should_show_contact_forces() const;
        void set_should_show_contact_forces(bool);

        bool get_should_show_force_linear_component() const;
        void set_should_show_force_linear_component(bool);

        bool get_should_show_force_angular_component() const;
        void set_should_show_force_angular_component(bool);

        bool get_should_show_point_forces() const;
        void set_should_show_point_forces(bool);

        bool get_should_show_scholz2015_obstacle_contact_hints() const;
        void set_should_show_scholz2015_obstacle_contact_hints(bool);

        void set_should_show_everything(bool);

        void for_each_option_as_app_setting_value(const std::function<void(std::string_view, const osc::Variant&)>&) const;
        void try_upd_from_values(std::string_view keyPrefix, const std::unordered_map<std::string, osc::Variant>&);

        friend bool operator==(const OpenSimDecorationOptions&, const OpenSimDecorationOptions&) = default;

    private:
        MuscleDecorationStyle muscle_decoration_style_;
        MuscleColorSource muscle_color_source_;
        MuscleSizingStyle muscle_sizing_style_;
        MuscleColorSourceScaling muscle_colour_source_scaling_;
        OpenSimDecorationOptionFlags flags_;
    };
}
