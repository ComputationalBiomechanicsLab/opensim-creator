#include "LogViewer.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CircularBuffer.h>
#include <oscar/Utils/EnumHelpers.h>

#include <string>
#include <sstream>
#include <type_traits>
#include <utility>

using namespace osc;

namespace
{
    Color to_color(LogLevel log_level)
    {
        switch (log_level) {
        case LogLevel::trace:    return {0.5f, 0.5f, 0.5f, 1.0f};
        case LogLevel::debug:    return {0.8f, 0.8f, 0.8f, 1.0f};
        case LogLevel::info:     return {0.5f, 0.5f, 1.0f, 1.0f};
        case LogLevel::warn:     return {1.0f, 1.0f, 0.0f, 1.0f};
        case LogLevel::err:      return {1.0f, 0.0f, 0.0f, 1.0f};
        case LogLevel::critical: return {1.0f, 0.0f, 0.0f, 1.0f};
        default:                 return {1.0f, 1.0f, 1.0f, 1.0f};
        }
    }

    void copy_traceback_log_to_clipboard()
    {
        std::stringstream ss;

        auto& guarded_content = global_get_traceback_log();
        {
            const auto& guard = guarded_content.lock();
            for (const LogMessage& msg : *guard) {
                ss << '[' << msg.level() << "] " << msg.payload() << '\n';
            }
        }

        set_clipboard_text(std::move(ss).str());
    }
}

class osc::LogViewer::Impl final {
public:
    void on_draw()
    {
        const auto logger = global_default_logger();
        if (not logger) {
            return;
        }

        // draw top menu bar
        if (ui::begin_menu_bar()) {
            // draw level selector
            {
                const LogLevel current_log_level = logger->level();
                ui::set_next_item_width(200.0f);
                if (ui::begin_combobox("log_level_", to_cstringview(current_log_level))) {
                    for (LogLevel log_level : make_option_iterable<LogLevel>()) {
                        bool active = log_level == current_log_level;
                        if (ui::draw_selectable(to_cstringview(log_level), &active)) {
                            logger->set_level(log_level);
                        }
                    }
                    ui::end_combobox();
                }
            }

            ui::same_line();
            ui::draw_checkbox("autoscroll", &autoscroll_);

            ui::same_line();
            if (ui::draw_button("clear")) {
                global_get_traceback_log().lock()->clear();
            }
            App::upd().add_frame_annotation("LogClearButton", ui::get_last_drawn_item_screen_rect());

            ui::same_line();
            if (ui::draw_button("turn off")) {
                logger->set_level(LogLevel::off);
            }

            ui::same_line();
            if (ui::draw_button("copy to clipboard")) {
                copy_traceback_log_to_clipboard();
            }

            ui::draw_dummy({0.0f, 10.0f});

            ui::end_menu_bar();
        }

        // draw log content lines
        auto& guarded_content = global_get_traceback_log();
        const auto& guard = guarded_content.lock();
        for (const LogMessage& log_message : *guard) {
            ui::push_style_color(ImGuiCol_Text, ::to_color(log_message.level()));
            ui::draw_text("[%s]", to_cstringview(log_message.level()).c_str());
            ui::pop_style_color();
            ui::same_line();
            ui::draw_text_wrapped(log_message.payload());

            if (autoscroll_) {
                ui::set_scroll_y_here();
            }
        }
    }
private:
    bool autoscroll_ = true;
};


osc::LogViewer::LogViewer() :
    impl_{std::make_unique<Impl>()}
{}
osc::LogViewer::LogViewer(LogViewer&&) noexcept = default;
osc::LogViewer& osc::LogViewer::operator=(LogViewer&&) noexcept = default;
osc::LogViewer::~LogViewer() noexcept = default;

void osc::LogViewer::on_draw()
{
    impl_->on_draw();
}
