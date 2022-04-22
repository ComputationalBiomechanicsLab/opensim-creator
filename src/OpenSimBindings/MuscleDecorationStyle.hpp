#pragma once

#include <nonstd/span.hpp>

namespace osc
{
	enum class MuscleDecorationStyle {
		OpenSim = 0,
		Scone,
		TOTAL,
		Default = Scone,
	};

	nonstd::span<MuscleDecorationStyle const> GetAllMuscleDecorationStyles();
	nonstd::span<char const* const> GetAllMuscleDecorationStyleStrings();
	int GetIndexOf(MuscleDecorationStyle);
}