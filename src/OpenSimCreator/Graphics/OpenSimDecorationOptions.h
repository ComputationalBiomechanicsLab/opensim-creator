#pragma once

#include <OpenSimCreator/Graphics/MuscleColoringStyle.h>
#include <OpenSimCreator/Graphics/MuscleDecorationStyle.h>
#include <OpenSimCreator/Graphics/MuscleSizingStyle.h>
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

        MuscleColoringStyle getMuscleColoringStyle() const;
        void setMuscleColoringStyle(MuscleColoringStyle);

        MuscleSizingStyle getMuscleSizingStyle() const;
        void setMuscleSizingStyle(MuscleSizingStyle);

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
        void getShouldShowForceAngularComponent(bool);

        void forEachOptionAsAppSettingValue(const std::function<void(std::string_view, const Variant&)>&) const;
        void tryUpdFromValues(std::string_view keyPrefix, const std::unordered_map<std::string, Variant>&);

        friend bool operator==(const OpenSimDecorationOptions&, const OpenSimDecorationOptions&) = default;

    private:
        MuscleDecorationStyle m_MuscleDecorationStyle;
        MuscleColoringStyle m_MuscleColoringStyle;
        MuscleSizingStyle m_MuscleSizingStyle;
        OpenSimDecorationOptionFlags m_Flags;
    };
}
