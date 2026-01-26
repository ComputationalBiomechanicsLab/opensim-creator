#include <liboscar/platform/events/event_type.h>
#include <liboscar/platform/events/key_event.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/typelist.h>
#include <liboscar-demos/oscar_demos_typelist.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    class NamedWidget final {
    public:
        template<typename TWidget>
        requires (requires { TWidget::id(); })
        static NamedWidget create()
        {
            return NamedWidget{TWidget::id(), [](Widget* owner) { return std::make_unique<TWidget>(owner); }};
        }

        const auto& name() const { return name_; }
        std::unique_ptr<Widget> construct(Widget* owner) { return constructor_(owner); }
    private:
        template<typename StringLike, typename Constructor>
        requires (std::constructible_from<std::string, StringLike> and std::invocable<Constructor, Widget*>)
        NamedWidget(StringLike&& name, Constructor&& constructor) :
            name_{std::forward<StringLike>(name)},
            constructor_{std::forward<Constructor>(constructor)}
        {}

        std::string name_;
        std::function<std::unique_ptr<Widget>(Widget*)> constructor_;
    };

    std::vector<NamedWidget> all_demos()
    {
        std::vector<NamedWidget> rv;
        [&rv]<typename... Demos>(Typelist<Demos...>)
        {
            static_assert(sizeof...(Demos) > 0);
            (rv.push_back(NamedWidget::create<Demos>()), ...);
        }(OscarDemosTypelist{});
        return rv;
    }

    class OscarDemoViewer : public Widget {
    public:
        OscarDemoViewer()
        {
            active_demo_ = demos_[active_demo_index_].construct(this);
            App::upd().set_main_window_subtitle(active_demo_->name());
            App::upd().make_main_loop_polling();
        }

    private:
        bool impl_on_key_event(KeyEvent& e) final
        {
            if (e.type() != EventType::KeyUp) {
                return false;
            }

            if (e.key() == Key::PageUp or e.key() == Key::PageDown) {
                const size_t offset = e.key() == Key::PageUp ? 1 : (demos_.size() - 1);
                switch_demo((active_demo_index_ + offset) % demos_.size());
                return true;
            }

            if (e.key() == Key::F11) {
                App::upd().make_main_window_fullscreen();
            }

            return false;
        }

        bool impl_on_event(Event& e) final
        {
            if (Widget::impl_on_event(e)) {
                return true;
            }
            if (ui_context_.on_event(e)) {
                return true;
            }
            return active_demo_->on_event(e);
        }

        void impl_on_mount() final { active_demo_->on_mount(); }
        void impl_on_unmount() final { active_demo_->on_unmount(); }
        void impl_on_tick() final { active_demo_->on_tick(); }
        void impl_on_draw() final
        {
            App::upd().clear_main_window();
            ui_context_.on_start_new_frame();

            draw_ui_demo_selector();

            active_demo_->on_draw();
            ui_context_.render();
        }

        void draw_ui_demo_selector()
        {
            const auto& longest_entry = *rgs::max_element(demos_, rgs::less{}, [](auto const& entry) { return entry.name().size(); });
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
                if (ui::begin_combobox("##DemoSelectorCombobox", active_demo_->name())) {
                    for (size_t i = 0; i < demos_.size(); ++i) {
                        if (ui::draw_selectable(demos_[i].name(), demos_[i].name() == active_demo_->name())) {
                            switch_demo(i);
                        }
                    }
                    ui::end_combobox();
                }
                ui::end_panel();
            }
        }

        void switch_demo(size_t new_demo_index)
        {
            active_demo_index_ = new_demo_index;
            active_demo_->on_unmount();
            active_demo_.reset();
            active_demo_ = demos_[active_demo_index_].construct(this);
            active_demo_->on_mount();
            App::upd().set_main_window_subtitle(active_demo_->name());
        }

        std::vector<NamedWidget> demos_ = all_demos();
        ui::Context ui_context_{App::upd()};
        size_t active_demo_index_ = 0;
        std::unique_ptr<Widget> active_demo_;
    };
}

int main(int, char**)
{
    return App::main<OscarDemoViewer>(AppMetadata{});
}
