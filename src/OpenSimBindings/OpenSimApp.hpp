#pragma once

#include "src/Platform/App.hpp"

namespace osc
{
	// an `osc::App` that also initializes OpenSim (logging, registering components, etc.)
	class OpenSimApp : public App {
	public:
		OpenSimApp();

	private:
		bool m_Initialized;
	};
}