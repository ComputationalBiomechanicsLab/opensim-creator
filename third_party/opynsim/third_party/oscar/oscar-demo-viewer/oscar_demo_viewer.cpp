#include <liboscar/platform/events/event_type.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/tabs/tab.h>
#include <liboscar/ui/tabs/tab_registry.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utils/assertions.h>
#include <liboscar-demos/oscar_demos_tab_registry.h>

#include <algorithm>
#include <cstddef>
#include <memory>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    class OscarDemoViewer : public Widget {
    public:
        OscarDemoViewer()
        {
            register_demo_tabs(tab_registry_);
            OSC_ASSERT_ALWAYS(not tab_registry_.empty() && "the demo registry cannot be empty");
            active_tab_ = tab_registry_[active_tab_index_].construct_tab(this);
            App::upd().set_main_window_subtitle(active_tab_->name());
            App::upd().make_main_loop_polling();
        }

    private:
        bool impl_on_event(Event& e) final
        {
            if (e.type() == EventType::KeyUp) {
                const auto* kev = dynamic_cast<const KeyEvent*>(&e);
                const Key key = kev->key();
                if (key == Key::PageUp or key == Key::PageDown) {
                    const size_t offset = kev->key() == Key::PageUp ? 1 : (tab_registry_.size() - 1);
                    switch_tab((active_tab_index_ + offset) % tab_registry_.size());
                    return true;
                }
            }
            return ui_context_.on_event(e) ? true : active_tab_->on_event(e);
        }

        void impl_on_mount() final { active_tab_->on_mount(); }
        void impl_on_unmount() final { active_tab_->on_unmount(); }
        void impl_on_tick() final { active_tab_->on_tick(); }
        void impl_on_draw() final
        {
            App::upd().clear_main_window();
            ui_context_.on_start_new_frame();

            draw_ui_demo_selector();

            active_tab_->on_draw();
            ui_context_.render();
        }

        void draw_ui_demo_selector()
        {
            const auto& longest_entry = *rgs::max_element(tab_registry_, rgs::less{}, [](auto const& entry) { return entry.name().size(); });
            const float combo_len = ui::calc_text_size(longest_entry.name()).x();

            // Draw demo selector dropdown
            const ui::PanelFlags panel_flags = {
                ui::PanelFlag::NoBackground,
                ui::PanelFlag::AlwaysAutoResize,
                ui::PanelFlag::NoDecoration,
                ui::PanelFlag::NoDocking,
                ui::PanelFlag::NoTitleBar,
            };
            ui::set_next_panel_ui_position(Vector2{5.0f});
            if (ui::begin_panel("Demo Selector", nullptr, panel_flags)) {
                ui::set_next_item_width(combo_len + 32.0f);  // pad the arrow
                if (ui::begin_combobox("##DemoSelectorCombobox", active_tab_->name())) {
                    for (size_t i = 0; i < tab_registry_.size(); ++i) {
                        if (ui::draw_selectable(tab_registry_[i].name(), tab_registry_[i].name() == active_tab_->name())) {
                            switch_tab(i);
                        }
                    }
                    ui::end_combobox();
                }
                ui::end_panel();
            }
        }

        void switch_tab(size_t new_tab_index)
        {
            active_tab_index_ = new_tab_index;
            active_tab_->on_unmount();
            active_tab_.reset();
            active_tab_ = tab_registry_[active_tab_index_].construct_tab(this);
            active_tab_->on_mount();
            App::upd().set_main_window_subtitle(active_tab_->name());
        }

        TabRegistry tab_registry_;
        ui::Context ui_context_{App::upd()};
        size_t active_tab_index_ = 0;
        std::unique_ptr<Tab> active_tab_;
    };
}

int main(int, char**) { return App::main<OscarDemoViewer>(AppMetadata{}); }
