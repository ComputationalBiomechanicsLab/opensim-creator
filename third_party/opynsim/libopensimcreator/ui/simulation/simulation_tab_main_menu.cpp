#include "simulation_tab_main_menu.h"

#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/ui/shared/main_menu.h>

#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_manager.h>
#include <liboscar/ui/widgets/window_menu.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationTabMainMenu::Impl final : public WidgetPrivate {
public:
    Impl(
        Widget& owner,
        Widget* parent,
        std::shared_ptr<Simulation> simulation,
        std::shared_ptr<PanelManager> panel_manager) :

        WidgetPrivate{owner, parent},
        simulation_{std::move(simulation)},
        main_menu_file_tab_{&owner},
        main_menu_window_tab_{&owner, std::move(panel_manager)}
    {}

    void on_draw()
    {
        main_menu_file_tab_.on_draw();
        draw_actions_menu();
        main_menu_window_tab_.on_draw();
        main_menu_about_tab_.on_draw();
    }

private:
    void draw_actions_menu()
    {
        if (not ui::begin_menu("Actions")) {
            return;
        }

        if (ui::begin_menu("Change End Time", simulation_->can_change_end_time())) {
            if (ui::draw_menu_item("0.1x")) {
                simulation_->request_new_end_time(
                    simulation_->get_start_time() + (0.1 * simulation_->get_duration()));
            }
            if (ui::draw_menu_item("0.25x")) {
                simulation_->request_new_end_time(
                    simulation_->get_start_time() + (0.25 * simulation_->get_duration()));
            }
            if (ui::draw_menu_item("0.5x")) {
                simulation_->request_new_end_time(
                    simulation_->get_start_time() + (0.5 * simulation_->get_duration()));
            }
            if (ui::draw_menu_item("2x")) {
                simulation_->request_new_end_time(
                    simulation_->get_start_time() + (2 * simulation_->get_duration()));
            }
            if (ui::draw_menu_item("4x")) {
                simulation_->request_new_end_time(
                    simulation_->get_start_time() + (4 * simulation_->get_duration()));
            }
            if (ui::draw_menu_item("10x")) {
                simulation_->request_new_end_time(
                    simulation_->get_start_time() + (10 * simulation_->get_duration()));
            }
            {
                auto count = new_custom_end_time_.count();
                ui::draw_double_input("custom end time", &count);
                if (ui::should_save_last_drawn_item_value()) {
                    simulation_->request_new_end_time(
                        simulation_->get_start_time() + SimulationClock::duration{count});
                }
            }

            ui::end_menu();
        }

        ui::end_menu();
    }

    std::shared_ptr<Simulation> simulation_;

    MainMenuFileTab main_menu_file_tab_;
    MainMenuAboutTab main_menu_about_tab_;
    WindowMenu main_menu_window_tab_;
    SimulationClock::duration new_custom_end_time_ =
        2 * (simulation_->get_end_time() - simulation_->get_start_time());
};

osc::SimulationTabMainMenu::SimulationTabMainMenu(
    Widget* parent,
    std::shared_ptr<Simulation> simulation,
    std::shared_ptr<PanelManager> panel_manager) :

    Widget{std::make_unique<Impl>(*this, parent, std::move(simulation), std::move(panel_manager))}
{}
void osc::SimulationTabMainMenu::impl_on_draw()
{
    private_data().on_draw();
}
