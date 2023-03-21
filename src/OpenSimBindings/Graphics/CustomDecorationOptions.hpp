#pragma once

#include "src/OpenSimBindings/Graphics/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/Graphics/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/Graphics/MuscleSizingStyle.hpp"
#include "src/Utils/CStringView.hpp"

#include <cstdint>
#include <optional>

namespace osc
{
    class CustomDecorationOptions final {
    public:
        CustomDecorationOptions();

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

    private:
        friend bool operator==(CustomDecorationOptions const&, CustomDecorationOptions const&) noexcept;

        MuscleDecorationStyle m_MuscleDecorationStyle;
        MuscleColoringStyle m_MuscleColoringStyle;
        MuscleSizingStyle m_MuscleSizingStyle;
        uint32_t m_Flags;
    };

    bool operator==(CustomDecorationOptions const&, CustomDecorationOptions const&) noexcept;

    inline bool operator!=(CustomDecorationOptions const& a, CustomDecorationOptions const& b)
    {
        return !(a == b);
    }
}