#pragma once

#include <OpenSimCreator/Graphics/MuscleColorSource.h>
#include <OpenSimCreator/Graphics/MuscleDecorationStyle.h>
#include <OpenSimCreator/Graphics/MuscleSizingStyle.h>
#include <OpenSimCreator/Graphics/MuscleColorSourceScaling.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptionFlags.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Variant/Variant.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace osc { class AppSettingValue; }

namespace osc
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
        CStringView getOptionLabel(ptrdiff_t) const;
        std::optional<CStringView> getOptionDescription(ptrdiff_t) const;

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

        void setShouldShowEverything(bool);

        void forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const Variant&)>&) const;
        void tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, Variant>&);

        friend bool operator==(const OpenSimDecorationOptions&, const OpenSimDecorationOptions&) = default;

    private:
        MuscleDecorationStyle m_MuscleDecorationStyle;
        MuscleColorSource m_MuscleColorSource;
        MuscleSizingStyle m_MuscleSizingStyle;
        MuscleColorSourceScaling m_MuscleColourSourceScaling;
        OpenSimDecorationOptionFlags m_Flags;
    };
}
