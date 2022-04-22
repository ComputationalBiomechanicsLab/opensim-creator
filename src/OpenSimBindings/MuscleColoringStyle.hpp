#pragma once

#include <nonstd/span.hpp>

namespace osc
{
	enum class MuscleColoringStyle {
		Activation = 0,
		Excitation,
		Force,
		FiberLength,
		TOTAL,
		Default = Activation,
	};

	nonstd::span<MuscleColoringStyle const> GetAllMuscleColoringStyles();
	nonstd::span<char const* const> GetAllMuscleColoringStyleStrings();
	int GetIndexOf(MuscleColoringStyle);
}