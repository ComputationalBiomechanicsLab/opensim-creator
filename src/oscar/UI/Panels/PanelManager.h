#pragma once

#include <oscar/UI/Panels/ToggleablePanelFlags.h>
#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

namespace osc { class IPanel; }

namespace osc
{
    // manages a collection of panels that may be toggled, disabled, spawned, etc.
    class PanelManager final {
    public:
        PanelManager();
        PanelManager(const PanelManager&) = delete;
        PanelManager(PanelManager&&) noexcept;
        PanelManager& operator=(const PanelManager&) = delete;
        PanelManager& operator=(PanelManager&&) noexcept;
        ~PanelManager() noexcept;

        // register a panel that can be toggled on/off
        void register_toggleable_panel(
            std::string_view base_name,
            std::function<std::shared_ptr<IPanel>(std::string_view)> panel_constructor,
            ToggleablePanelFlags flags = ToggleablePanelFlags::Default
        );

        // register a panel that can spawn N copies (e.g. visualizers)
        void register_spawnable_panel(
            std::string_view base_name,
            std::function<std::shared_ptr<IPanel>(std::string_view)> panel_constructor,
            size_t num_initially_opened_panels
        );

        // returns the panel with the given name, or `nullptr` if not found
        IPanel* try_upd_panel_by_name(std::string_view);
        template<std::derived_from<IPanel> TConcretePanel>
        TConcretePanel* try_upd_panel_by_name_T(std::string_view name)
        {
            IPanel* p = try_upd_panel_by_name(name);
            return p ? dynamic_cast<TConcretePanel*>(p) : nullptr;
        }

        // methods for panels that are either enabled or disabled (toggleable)
        size_t num_toggleable_panels() const;
        CStringView toggleable_panel_name(size_t) const;
        bool is_toggleable_panel_activated(size_t) const;
        void set_toggleable_panel_activated(size_t, bool);
        void set_toggleable_panel_activated(std::string_view, bool);

        // methods for dynamic panels that have been added at runtime (i.e. have been spawned)
        size_t num_dynamic_panels() const;
        CStringView dynamic_panel_name(size_t) const;
        void deactivate_dynamic_panel(size_t);

        // methods for spawnable panels (i.e. things that can create new dynamic panels)
        size_t num_spawnable_panels() const;
        CStringView spawnable_panel_base_name(size_t) const;
        void create_dynamic_panel(size_t);
        std::string suggested_dynamic_panel_name(std::string_view base_name);
        void push_dynamic_panel(std::string_view base_name, std::shared_ptr<IPanel>);

        void on_mount();
        void on_unmount();
        void on_tick();
        void on_draw();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
