#pragma once

#include "oscar/Panels/ToggleablePanelFlags.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

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

        // register a panel that can be toggled on/off
        void registerToggleablePanel(
            std::string_view baseName,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            ToggleablePanelFlags flags_ = ToggleablePanelFlags_Default
        );

        // register a panel that can spawn N copies (e.g. visualizers)
        void registerSpawnablePanel(
            std::string_view baseName,
            std::function<std::shared_ptr<osc::Panel>(std::string_view)> constructorFunc_,
            size_t numInitiallyOpenedPanels
        );

        // returns the panel with the given name, or `nullptr` if not found
        Panel* tryUpdPanelByName(std::string_view);
        template<typename TConcretePanel>
        TConcretePanel* tryUpdPanelByNameT(std::string_view name)
        {
            Panel* p = tryUpdPanelByName(name);
            return p ? dynamic_cast<TConcretePanel*>(p) : nullptr;
        }

        // methods for panels that are either enabled or disabled (toggleable)
        size_t getNumToggleablePanels() const;
        CStringView getToggleablePanelName(size_t) const;
        bool isToggleablePanelActivated(size_t) const;
        void setToggleablePanelActivated(size_t, bool);
        void setToggleablePanelActivated(std::string_view, bool);

        // methods for dynamic panels that have been added at runtime (i.e. have been spawned)
        size_t getNumDynamicPanels() const;
        CStringView getDynamicPanelName(size_t) const;
        void deactivateDynamicPanel(size_t);

        // methods for spawnable panels (i.e. things that can create new dynamic panels)
        size_t getNumSpawnablePanels() const;
        CStringView getSpawnablePanelBaseName(size_t) const;
        void createDynamicPanel(size_t);
        std::string computeSuggestedDynamicPanelName(std::string_view baseName);
        void pushDynamicPanel(std::string_view baseName, std::shared_ptr<Panel>);

        void onMount();
        void onUnmount();
        void onTick();
        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
