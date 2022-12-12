#pragma once

#include "src/Widgets/ToggleablePanel.hpp"

#include <nonstd/span.hpp>

#include <vector>

namespace osc
{
    // a collection of panels that the user may toggle at runtime
    class ToggleablePanels final {
    public:

        nonstd::span<ToggleablePanel> upd();

        void push_back(ToggleablePanel&&);

        void activateAllDefaultOpenPanels();

        void garbageCollectDeactivatedPanels();

        void drawAllActivatedPanels();

    private:
        std::vector<ToggleablePanel> m_Panels;
    };
}