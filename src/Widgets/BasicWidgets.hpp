#pragma once

namespace osc
{
	class MainUIStateAPI;
	class ParamBlock;
	class VirtualModelStatePair;
}

namespace OpenSim
{
	class Component;
}

namespace osc
{
	void DrawComponentHoverTooltip(OpenSim::Component const&);
	void DrawSelectOwnerMenu(osc::VirtualModelStatePair&, OpenSim::Component const&);
	void DrawRequestOutputsMenu(osc::MainUIStateAPI&, OpenSim::Component const&);
	void DrawSimulationParams(osc::ParamBlock const&);
}