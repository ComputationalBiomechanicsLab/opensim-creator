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

MuscleDecorationStyle opyn::OpenSimDecorationOptions::getMuscleDecorationStyle() const
{
    return muscle_decoration_style_;
}

void opyn::OpenSimDecorationOptions::setMuscleDecorationStyle(MuscleDecorationStyle s)
{
    muscle_decoration_style_ = s;
}

MuscleColorSource opyn::OpenSimDecorationOptions::getMuscleColorSource() const
{
    return muscle_color_source_;
}

void opyn::OpenSimDecorationOptions::setMuscleColorSource(MuscleColorSource s)
{
    muscle_color_source_ = s;
}

MuscleSizingStyle opyn::OpenSimDecorationOptions::getMuscleSizingStyle() const
{
    return muscle_sizing_style_;
}

void opyn::OpenSimDecorationOptions::setMuscleSizingStyle(MuscleSizingStyle s)
{
    muscle_sizing_style_ = s;
}

MuscleColorSourceScaling opyn::OpenSimDecorationOptions::getMuscleColorSourceScaling() const
{
    return muscle_colour_source_scaling_;
}

void opyn::OpenSimDecorationOptions::setMuscleColorSourceScaling(MuscleColorSourceScaling s)
{
    muscle_colour_source_scaling_ = s;
}

size_t opyn::OpenSimDecorationOptions::getNumOptions() const
{
    return osc::num_flags<OpenSimDecorationOptionFlag>();
}

bool opyn::OpenSimDecorationOptions::getOptionValue(ptrdiff_t i) const
{
    return flags_.get(GetIthOption(i));
}

void opyn::OpenSimDecorationOptions::setOptionValue(ptrdiff_t i, bool v)
{
    SetIthOption(flags_, i, v);
}

osc::CStringView opyn::OpenSimDecorationOptions::getOptionLabel(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).label;
}

std::optional<osc::CStringView> opyn::OpenSimDecorationOptions::getOptionDescription(ptrdiff_t i) const
{
    return GetIthOptionMetadata(i).maybe_description;
}

bool opyn::OpenSimDecorationOptions::getShouldShowScapulo() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowScapulo);
}

void opyn::OpenSimDecorationOptions::setShouldShowScapulo(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowScapulo, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForOrigin() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin);
}

void opyn::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForOrigin(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForOrigin, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowEffectiveMuscleLineOfActionForInsertion() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion);
}

void opyn::OpenSimDecorationOptions::setShouldShowEffectiveMuscleLineOfActionForInsertion(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowEffectiveLinesOfActionForInsertion, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForOrigin() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin);
}

void opyn::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForOrigin(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForOrigin, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowAnatomicalMuscleLineOfActionForInsertion() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion);
}

void opyn::OpenSimDecorationOptions::setShouldShowAnatomicalMuscleLineOfActionForInsertion(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowAnatomicalMuscleLinesOfActionForInsertion, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowCentersOfMass() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass);
}

void opyn::OpenSimDecorationOptions::setShouldShowCentersOfMass(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowCentersOfMass, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowPointToPointSprings() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings);
}

void opyn::OpenSimDecorationOptions::setShouldShowPointToPointSprings(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowPointToPointSprings, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowContactForces() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowContactForces);
}

void opyn::OpenSimDecorationOptions::setShouldShowContactForces(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowContactForces, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowForceLinearComponent() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent);
}

void opyn::OpenSimDecorationOptions::setShouldShowForceLinearComponent(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowForceLinearComponent, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowForceAngularComponent() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent);
}

void opyn::OpenSimDecorationOptions::setShouldShowForceAngularComponent(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowForceAngularComponent, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowPointForces() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowPointForces);
}

void opyn::OpenSimDecorationOptions::setShouldShowPointForces(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowPointForces, v);
}

bool opyn::OpenSimDecorationOptions::getShouldShowScholz2015ObstacleContactHints() const
{
    return flags_.get(OpenSimDecorationOptionFlag::ShouldShowScholz2015ObstacleContactHints);
}

void opyn::OpenSimDecorationOptions::setShouldShowScholz2015ObstacleContactHints(bool v)
{
    flags_.set(OpenSimDecorationOptionFlag::ShouldShowScholz2015ObstacleContactHints, v);
}

void opyn::OpenSimDecorationOptions::setShouldShowEverything(bool v)
{
    setShouldShowScapulo(v);
    setShouldShowEffectiveMuscleLineOfActionForOrigin(v);
    setShouldShowEffectiveMuscleLineOfActionForInsertion(v);
    setShouldShowAnatomicalMuscleLineOfActionForOrigin(v);
    setShouldShowAnatomicalMuscleLineOfActionForInsertion(v);
    setShouldShowCentersOfMass(v);
    setShouldShowPointToPointSprings(v);
    setShouldShowContactForces(v);
    setShouldShowForceLinearComponent(v);
    setShouldShowForceAngularComponent(v);
    setShouldShowPointForces(v);
}

void opyn::OpenSimDecorationOptions::forEachOptionAsAppSettingValue(
    const std::function<void(std::string_view, const osc::Variant&)>& callback) const
{
    callback("muscle_decoration_style", osc::Variant{GetMuscleDecorationStyleMetadata(muscle_decoration_style_).id});
    callback("muscle_coloring_style",   osc::Variant{GetMuscleColoringStyleMetadata(muscle_color_source_).id});
    callback("muscle_sizing_style",     osc::Variant{GetMuscleSizingStyleMetadata(muscle_sizing_style_).id});
    callback("muscle_color_scaling",    osc::Variant{GetMuscleColorSourceScalingMetadata(muscle_colour_source_scaling_).id});
    for (size_t i = 0; i < osc::num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& meta = GetIthOptionMetadata(i);
        callback(meta.id, osc::Variant{flags_.get(GetIthOption(i))});
    }
}

void opyn::OpenSimDecorationOptions::tryUpdFromValues(
    std::string_view prefix,
    const std::unordered_map<std::string, osc::Variant>& lut)
{
    // looks up a single element in the lut
    auto lookup = [
        &lut,
        buf = std::string{prefix},
        prefixLen = prefix.size()](std::string_view v) mutable
    {
        buf.resize(prefixLen);
        buf.insert(prefixLen, v);

        return lookup_or_nullptr(lut, buf);
    };

    if (auto* appVal = lookup("muscle_decoration_style"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = GetAllMuscleDecorationStyleMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_decoration_style_ = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_coloring_style"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = GetAllPossibleMuscleColoringSourcesMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_color_source_ = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_sizing_style"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = GetAllMuscleSizingStyleMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_sizing_style_ = it->value;
        }
    }

    if (auto* appVal = lookup("muscle_color_scaling"); appVal and appVal->type() == osc::VariantType::String)
    {
        const auto metadata = GetAllPossibleMuscleColorSourceScalingMetadata();
        const auto it = rgs::find(metadata, to<std::string>(*appVal), [](const auto& m) { return m.id; });
        if (it != metadata.end()) {
            muscle_colour_source_scaling_ = it->value;
        }
    }

    for (size_t i = 0; i < osc::num_flags<OpenSimDecorationOptionFlag>(); ++i) {
        const auto& metadata = GetIthOptionMetadata(i);
        if (auto* appVal = lookup(metadata.id); appVal and appVal->type() == osc::VariantType::Bool) {
            flags_.set(GetIthOption(i), to<bool>(*appVal));
        }
    }
}
