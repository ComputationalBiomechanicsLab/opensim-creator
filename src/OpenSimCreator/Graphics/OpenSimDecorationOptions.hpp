#pragma once

#include <OpenSimCreator/Graphics/MuscleColoringStyle.hpp>
#include <OpenSimCreator/Graphics/MuscleDecorationStyle.hpp>
#include <OpenSimCreator/Graphics/MuscleSizingStyle.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptionFlags.hpp>

#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>

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

        void forEachOptionAsAppSettingValue(std::function<void(std::string_view, AppSettingValue const&)> const&) const;

    private:
        friend bool operator==(OpenSimDecorationOptions const&, OpenSimDecorationOptions const&) noexcept;

        MuscleDecorationStyle m_MuscleDecorationStyle;
        MuscleColoringStyle m_MuscleColoringStyle;
        MuscleSizingStyle m_MuscleSizingStyle;
        OpenSimDecorationOptionFlags m_Flags;
    };

    bool operator==(OpenSimDecorationOptions const&, OpenSimDecorationOptions const&) noexcept;

    inline bool operator!=(OpenSimDecorationOptions const& a, OpenSimDecorationOptions const& b)
    {
        return !(a == b);
    }
}
