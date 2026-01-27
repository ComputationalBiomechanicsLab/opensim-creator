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

        MuscleDecorationStyle getMuscleDecorationStyle() const;
        void setMuscleDecorationStyle(MuscleDecorationStyle);

        MuscleColorSource getMuscleColorSource() const;
        void setMuscleColorSource(MuscleColorSource);

        MuscleSizingStyle getMuscleSizingStyle() const;
        void setMuscleSizingStyle(MuscleSizingStyle);

        MuscleColorSourceScaling getMuscleColorSourceScaling() const;
        void setMuscleColorSourceScaling(MuscleColorSourceScaling);

        // the ones below here are toggle-able options with user-facing strings etc
        size_t getNumOptions() const;
        bool getOptionValue(ptrdiff_t) const;
        void setOptionValue(ptrdiff_t, bool);
        osc::CStringView getOptionLabel(ptrdiff_t) const;
        std::optional<osc::CStringView> getOptionDescription(ptrdiff_t) const;

        bool getShouldShowScapulo() const;
        void setShouldShowScapulo(bool);

        bool getShouldShowEffectiveMuscleLineOfActionForOrigin() const;
        void setShouldShowEffectiveMuscleLineOfActionForOrigin(bool);

        bool getShouldShowEffectiveMuscleLineOfActionForInsertion() const;
        void setShouldShowEffectiveMuscleLineOfActionForInsertion(bool);

        bool getShouldShowAnatomicalMuscleLineOfActionForOrigin() const;
        void setShouldShowAnatomicalMuscleLineOfActionForOrigin(bool);

        bool getShouldShowAnatomicalMuscleLineOfActionForInsertion() const;
        void setShouldShowAnatomicalMuscleLineOfActionForInsertion(bool);

        bool getShouldShowCentersOfMass() const;
        void setShouldShowCentersOfMass(bool);

        bool getShouldShowPointToPointSprings() const;
        void setShouldShowPointToPointSprings(bool);

        bool getShouldShowContactForces() const;
        void setShouldShowContactForces(bool);

        bool getShouldShowForceLinearComponent() const;
        void setShouldShowForceLinearComponent(bool);

        bool getShouldShowForceAngularComponent() const;
        void setShouldShowForceAngularComponent(bool);

        bool getShouldShowPointForces() const;
        void setShouldShowPointForces(bool);

        bool getShouldShowScholz2015ObstacleContactHints() const;
        void setShouldShowScholz2015ObstacleContactHints(bool);

        void setShouldShowEverything(bool);

        void forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const osc::Variant&)>&) const;
        void tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, osc::Variant>&);

        friend bool operator==(const OpenSimDecorationOptions&, const OpenSimDecorationOptions&) = default;

    private:
        MuscleDecorationStyle m_MuscleDecorationStyle;
        MuscleColorSource m_MuscleColorSource;
        MuscleSizingStyle m_MuscleSizingStyle;
        MuscleColorSourceScaling m_MuscleColourSourceScaling;
        OpenSimDecorationOptionFlags m_Flags;
    };
}
