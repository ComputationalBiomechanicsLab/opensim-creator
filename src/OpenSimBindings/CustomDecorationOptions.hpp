#pragma once

#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"

namespace osc
{
	class CustomDecorationOptions final {
	public:
		MuscleDecorationStyle getMuscleDecorationStyle() const { return m_MuscleDecorationStyle; }
		void setMuscleDecorationStyle(MuscleDecorationStyle s) { m_MuscleDecorationStyle = s; }

	private:
		MuscleDecorationStyle m_MuscleDecorationStyle = MuscleDecorationStyle::Scone;
	};
}