#pragma once

namespace osc
{
	class MainUIStateAPI;
	class VirtualModelStatePair;
}

namespace OpenSim
{
	class Component;
}

namespace osc
{
	void DrawSelectOwnerMenu(osc::VirtualModelStatePair&, OpenSim::Component const&);
	void DrawRequestOutputsMenu(osc::MainUIStateAPI&, OpenSim::Component const&);

}