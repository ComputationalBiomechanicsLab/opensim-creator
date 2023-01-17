#pragma once

#include "src/Widgets/ToggleablePanelFlags.hpp"
#include "src/Utils/CStringView.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <functional>
#include <string_view>
#include <memory>

namespace osc { class Panel; }

namespace osc
{
    // manages a collection of panels that may be toggled, disabled, spawned, etc.
    class PanelManager final {
    public:
        PanelManager();
        PanelManager(PanelManager const&) = delete;
        PanelManager(PanelManager&&) noexcept;
        PanelManager& operator=(PanelManager const&) = delete;
        PanelManager& operator=(PanelManager&&) noexcept;
        ~PanelManager() noexcept;

        void registerPanel(
            std::string_view baseName,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            ToggleablePanelFlags flags_ = ToggleablePanelFlags_Default
        );

        size_t getNumToggleablePanels() const;
        CStringView getToggleablePanelName(size_t) const;
        bool isToggleablePanelActivated(size_t) const;
        void setToggleablePanelActivated(size_t, bool);

        void activateAllDefaultOpenPanels();
        void garbageCollectDeactivatedPanels();
        void drawAllActivatedPanels();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}