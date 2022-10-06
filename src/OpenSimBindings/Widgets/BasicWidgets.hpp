#pragma once

#include <string>

namespace osc { class MainUIStateAPI; }
namespace osc { class ParamBlock; }
namespace osc { class VirtualModelStatePair; }
namespace OpenSim { class Component; }

namespace osc
{
    void DrawComponentHoverTooltip(OpenSim::Component const&);
    void DrawSelectOwnerMenu(osc::VirtualModelStatePair&, OpenSim::Component const&);
    void DrawWatchOutputMenu(osc::MainUIStateAPI&, OpenSim::Component const&);
    void DrawSimulationParams(osc::ParamBlock const&);
    void DrawSearchBar(std::string&, int maxLen);
}