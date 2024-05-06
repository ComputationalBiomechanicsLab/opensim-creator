#include "WindowMenu.h"

#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PanelManager.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <utility>

osc::WindowMenu::WindowMenu(std::shared_ptr<PanelManager> panel_manager) :
    panel_manager_{std::move(panel_manager)}
{}
osc::WindowMenu::WindowMenu(WindowMenu&&) noexcept = default;
osc::WindowMenu& osc::WindowMenu::operator=(WindowMenu&&) noexcept = default;
osc::WindowMenu::~WindowMenu() noexcept = default;

void osc::WindowMenu::on_draw()
{
    if (ui::BeginMenu("Window")) {
        draw_content();
        ui::EndMenu();
    }
}

void osc::WindowMenu::draw_content()
{
    PanelManager& manager = *panel_manager_;

    size_t num_menu_items_printed = 0;

    // toggleable panels
    for (size_t i = 0; i < manager.num_toggleable_panels(); ++i) {
        bool activated = manager.is_toggleable_panel_activated(i);
        const CStringView name = manager.toggleable_panel_name(i);
        if (ui::MenuItem(name, {}, &activated)) {
            manager.set_toggleable_panel_activated(i, activated);
        }
        ++num_menu_items_printed;
    }

    // dynamic panels
    if (manager.num_dynamic_panels() > 0) {
        ui::Separator();
        for (size_t i = 0; i < manager.num_dynamic_panels(); ++i) {
            bool activated = true;
            const CStringView name = manager.dynamic_panel_name(i);
            if (ui::MenuItem(name, {}, &activated)) {
                manager.deactivate_dynamic_panel(i);
            }
            ++num_menu_items_printed;
        }
    }

    // spawnable submenu
    if (manager.num_spawnable_panels() > 0) {
        ui::Separator();

        if (ui::BeginMenu("Add")) {
            for (size_t i = 0; i < manager.num_spawnable_panels(); ++i) {
                const CStringView name = manager.spawnable_panel_base_name(i);
                if (ui::MenuItem(name)) {
                    manager.create_dynamic_panel(i);
                }
            }
            ui::EndMenu();
            ++num_menu_items_printed;
        }
    }

    if (num_menu_items_printed <= 0) {
        ui::TextDisabled("(no windows available to be toggled)");
    }
}
