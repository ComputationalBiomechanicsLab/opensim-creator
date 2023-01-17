#pragma once

#include <string>

namespace osc { class MainUIStateAPI; }
namespace osc { class ParamBlock; }
namespace osc { class VirtualModelStatePair; }
namespace osc { class VirtualOutputExtractor; }
namespace osc { class SimulationModelStatePair; }
namespace OpenSim { class Component; }

namespace osc
{
    void DrawComponentHoverTooltip(OpenSim::Component const&);
    void DrawSelectOwnerMenu(osc::VirtualModelStatePair&, OpenSim::Component const&);
    void DrawWatchOutputMenu(osc::MainUIStateAPI&, OpenSim::Component const&);
    void DrawSimulationParams(osc::ParamBlock const&);
    void DrawSearchBar(std::string&, int maxLen);
    void DrawOutputNameColumn(
        VirtualOutputExtractor const& output,
        bool centered = true,
        SimulationModelStatePair* maybeActiveSate = nullptr
    );
}