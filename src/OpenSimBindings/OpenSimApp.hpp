#pragma once

#include "src/Platform/App.hpp"

namespace osc
{
	// an App that also initializes OpenSim stuff
	class OpenSimApp : public App {
	public:
		OpenSimApp();

	private:
		bool m_Initialized;
	};
}