#pragma once

#include "src/OpenSimBindings/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"

namespace osc
{
	class CustomDecorationOptions final {
	public:
		MuscleDecorationStyle getMuscleDecorationStyle() const { return m_MuscleDecorationStyle; }
		void setMuscleDecorationStyle(MuscleDecorationStyle s) { m_MuscleDecorationStyle = s; }

		MuscleColoringStyle getMuscleColoringStyle() const { return m_MuscleColoringStyle; }
		void setMuscleColoringStyle(MuscleColoringStyle s) { m_MuscleColoringStyle = s; }

	private:
		friend bool operator==(CustomDecorationOptions const&, CustomDecorationOptions const&);
		friend bool operator!=(CustomDecorationOptions const&, CustomDecorationOptions const&);

		MuscleDecorationStyle m_MuscleDecorationStyle = MuscleDecorationStyle::Default;
		MuscleColoringStyle m_MuscleColoringStyle = MuscleColoringStyle::Default;
	};

	inline bool operator==(CustomDecorationOptions const& a, CustomDecorationOptions const& b)
	{
		return a.m_MuscleDecorationStyle == b.m_MuscleDecorationStyle && a.m_MuscleColoringStyle == b.m_MuscleColoringStyle;
	}

	inline bool operator!=(CustomDecorationOptions const& a, CustomDecorationOptions const& b)
	{
		return !(a == b);
	}
}