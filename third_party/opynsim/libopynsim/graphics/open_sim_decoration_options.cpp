#include "open_sim_decoration_options.h"

#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/conversion.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/variant/variant.h>
#include <liboscar/variant/variant_type.h>

#include <optional>
#include <ranges>

using namespace opyn;
namespace rgs = std::ranges;

opyn::OpenSimDecorationOptions::OpenSimDecorationOptions() :
    muscle_decoration_style_{MuscleDecorationStyle::Default},
    muscle_color_source_{MuscleColorSource::Default},
    muscle_sizing_style_{MuscleSizingStyle::Default},
    muscle_colour_source_scaling_{MuscleColorSourceScaling::Default},
    flags_{OpenSimDecorationOptionFlag::Default}
{}

MuscleDecorationStyle opyn::OpenSimDecorationOptions::get_muscle_decoration_style() const
{
    return muscle_decoration_style_;
}

void opyn::OpenSimDecorationOptions::set_muscle_decoration_style(MuscleDecorationStyle s)
{
    muscle_decoration_style_ = s;
}

MuscleColorSource opyn::OpenSimDecorationOptions::get_muscle_color_source() const
{
    return muscle_color_source_;
}

void opyn::OpenSimDecorationOptions::set_muscle_color_source(MuscleColorSource s)
{
    muscle_color_source_ = s;
}

MuscleSizingStyle opyn::OpenSimDecorationOptions::get_muscle_sizing_style() const
{
    return muscle_sizing_style_;
}

void opyn::OpenSimDecorationOptions::set_muscle_sizing_style(MuscleSizingStyle s)
{
    muscle_sizing_style_ = s;
}

MuscleColorSourceScaling opyn::OpenSimDecorationOptions::get_muscle_color_source_scaling() const
{
    return muscle_colour_source_scaling_;
}

void opyn::OpenSimDecorationOptions::set_muscle_color_source_scaling(MuscleColorSourceScaling s)
{
    muscle_colour_source_scaling_ = s;
}

size_t opyn::OpenSimDecorationOptions::get_num_options() const
{
    return osc::num_flags<OpenSimDecorationOptionFlag>();
}

bool opyn::OpenSimDecorationOptions::get_option_value(ptrdiff_t i) const
{
    return flags_.get(get_ith_option(i));
}

void opyn::OpenSimDecorationOptions::set_option_value(ptrdiff_t i, bool v)
{
    set_ith_option(flags_, i, v);
}

osc::CStringView opyn::OpenSimDecorationOptions::get_option_label(ptrdiff_t i) const
{
    return get_ith_option_metadata(i).label;
}

std::optional<osc::CStringView> opyn::OpenSimDecorationOptions::get_option_description(ptrdiff_t i) const
{
    return get_ith_option_metadata(i).maybe_description;
}

bool opyn::OpenSimDecorationOptions::get_should_show_scapulo() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowScapulo);
}

void opyn::OpenSimDecorationOptions::set_should_show_scapulo(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowScapulo, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_effective_muscle_line_of_action_for_origin() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin);
}

void opyn::OpenSimDecorationOptions::set_should_show_effective_muscle_line_of_action_for_origin(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_effective_muscle_line_of_action_for_insertion() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion);
}

void opyn::OpenSimDecorationOptions::set_should_show_effective_muscle_line_of_action_for_insertion(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_anatomical_muscle_line_of_action_for_origin() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin);
}

void opyn::OpenSimDecorationOptions::set_should_show_anatomical_muscle_line_of_action_for_origin(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_anatomical_muscle_line_of_action_for_insertion() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion);
}

void opyn::OpenSimDecorationOptions::set_should_show_anatomical_muscle_line_of_action_for_insertion(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_centers_of_mass() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass);
}

void opyn::OpenSimDecorationOptions::set_should_show_centers_of_mass(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_point_to_point_springs() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings);
}

void opyn::OpenSimDecorationOptions::set_should_show_point_to_point_springs(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_contact_forces() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowContactForces);
}

void opyn::OpenSimDecorationOptions::set_should_show_contact_forces(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowContactForces, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_force_linear_component() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent);
}

void opyn::OpenSimDecorationOptions::set_should_show_force_linear_component(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_force_angular_component() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent);
}

void opyn::OpenSimDecorationOptions::set_should_show_force_angular_component(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_point_forces() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowPointForces);
}

void opyn::OpenSimDecorationOptions::set_should_show_point_forces(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowPointForces, v);
}

bool opyn::OpenSimDecorationOptions::get_should_show_scholz2015_obstacle_contact_hints() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowScholz2015ObstacleContactHints);
}

void opyn::OpenSimDecorationOptions::set_should_show_scholz2015_obstacle_contact_hints(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowScholz2015ObstacleContactHints, v);
}

void opyn::OpenSimDecorationOptions::set_should_show_everything(bool v)
{
    set_should_show_scapulo(v);
    set_should_show_effective_muscle_line_of_action_for_origin(v);
    set_should_show_effective_muscle_line_of_action_for_insertion(v);
    set_should_show_anatomical_muscle_line_of_action_for_origin(v);
    set_should_show_anatomical_muscle_line_of_action_for_insertion(v);
    set_should_show_centers_of_mass(v);
    set_should_show_point_to_point_springs(v);
    set_should_show_contact_forces(v);
    set_should_show_force_linear_component(v);
    set_should_show_force_angular_component(v);
    set_should_show_point_forces(v);
}

void opyn::OpenSimDecorationOptions::for_each_option_as_app_setting_value(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    callback("muscle_decoration_style", osc::Variant{get_muscle_decoration_style_metadata(muscle_decoration_style_).id});
    callback("muscle_coloring_style",   osc::Variant{get_muscle_coloring_style_metadata(muscle_color_source_).id});
    callback("muscle_sizing_style",     osc::Variant{get_muscle_sizing_style_metadata(muscle_sizing_style_).id});
    callback("muscle_color_scaling",    osc::Variant{get_muscle_color_source_scaling_metadata(muscle_colour_source_scaling_).id});
    for (size_t i = 0; i < osc::num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& meta = get_ith_option_metadata(i);
        callback(meta.id, osc::Variant{flags_.get(get_ith_option(i))});
    }
}

void opyn::OpenSimDecorationOptions::try_upd_from_values(
    std::string_view prefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
{
    // looks up a single element in the lut
    auto lookup = [
        &lut,
        buf = std::string{prefix},
        prefix_len = prefix.size()](std::string_view v) mutable
    {
        buf.resize(prefix_len);
        buf.insert(prefix_len, v);

        return lookup_or_nullptr(lut, buf);
    };

    if (auto* app_val = lookup("muscle_decoration_style"); app_val and app_val->type() == osc::VariantType::String)
    {
        const auto metadata = get_all_muscle_decoration_style_metadata();
        const auto it = rgs::find(metadata, to<std::string>(*app_val), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_decoration_style_ = it->value;
        }
    }

    if (auto* app_val = lookup("muscle_coloring_style"); app_val and app_val->type() == osc::VariantType::String)
    {
        const auto metadata = get_all_possible_muscle_coloring_sources_metadata();
        const auto it = rgs::find(metadata, to<std::string>(*app_val), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_color_source_ = it->value;
        }
    }

    if (auto* app_val = lookup("muscle_sizing_style"); app_val and app_val->type() == osc::VariantType::String)
    {
        const auto metadata = get_all_muscle_sizing_style_metadata();
        const auto it = rgs::find(metadata, to<std::string>(*app_val), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_sizing_style_ = it->value;
        }
    }

    if (auto* app_val = lookup("muscle_color_scaling"); app_val and app_val->type() == osc::VariantType::String)
    {
        const auto metadata = get_all_possible_muscle_color_source_scaling_metadata();
        const auto it = rgs::find(metadata, to<std::string>(*app_val), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_colour_source_scaling_ = it->value;
        }
    }

    for (size_t i = 0; i < osc::num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& metadata = get_ith_option_metadata(i);
        if (auto* app_val = lookup(metadata.id); app_val and app_val->type() == osc::VariantType::Bool) {
            flags_.set(get_ith_option(i), to<bool>(*app_val));
        }
    }
}
