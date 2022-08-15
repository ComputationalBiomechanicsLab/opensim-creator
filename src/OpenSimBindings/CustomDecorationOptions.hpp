#pragma once

#include "src/OpenSimBindings/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/MuscleSizingStyle.hpp"

namespace osc
{
    class CustomDecorationOptions final {
    public:
        MuscleDecorationStyle getMuscleDecorationStyle() const { return m_MuscleDecorationStyle; }
        void setMuscleDecorationStyle(MuscleDecorationStyle s) { m_MuscleDecorationStyle = s; }

        MuscleColoringStyle getMuscleColoringStyle() const { return m_MuscleColoringStyle; }
        void setMuscleColoringStyle(MuscleColoringStyle s) { m_MuscleColoringStyle = s; }

        MuscleSizingStyle getMuscleSizingStyle() const { return m_MuscleSizingStyle; }
        void setMuscleSizingStyle(MuscleSizingStyle s) { m_MuscleSizingStyle = s; }

        bool getShouldShowScapulo() const { return m_ShouldShowScapulo; }
        void setShouldShowScapulo(bool v) { m_ShouldShowScapulo = v; }

    private:
        friend bool operator==(CustomDecorationOptions const&, CustomDecorationOptions const&);
        friend bool operator!=(CustomDecorationOptions const&, CustomDecorationOptions const&);

        MuscleDecorationStyle m_MuscleDecorationStyle = MuscleDecorationStyle::Default;
        MuscleColoringStyle m_MuscleColoringStyle = MuscleColoringStyle::Default;
        MuscleSizingStyle m_MuscleSizingStyle = MuscleSizingStyle::Default;
        bool m_ShouldShowScapulo = false;
    };

    inline bool operator==(CustomDecorationOptions const& a, CustomDecorationOptions const& b)
    {
        return
            a.m_MuscleDecorationStyle == b.m_MuscleDecorationStyle &&
            a.m_MuscleColoringStyle == b.m_MuscleColoringStyle &&
            a.m_MuscleSizingStyle == b.m_MuscleSizingStyle &&
            a.m_ShouldShowScapulo == b.m_ShouldShowScapulo;
    }

    inline bool operator!=(CustomDecorationOptions const& a, CustomDecorationOptions const& b)
    {
        return !(a == b);
    }
}