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
    Color to_color(LogLevel lvl)
    {
        switch (lvl) {
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
        if (ui::BeginMenuBar()) {
            // draw level selector
            {
                const LogLevel current_log_level = logger->level();
                ui::SetNextItemWidth(200.0f);
                if (ui::BeginCombo("log_level_", to_cstringview(current_log_level))) {
                    for (LogLevel log_level : make_option_iterable<LogLevel>()) {
                        bool active = log_level == current_log_level;
                        if (ui::Selectable(to_cstringview(log_level), &active)) {
                            logger->set_level(log_level);
                        }
                    }
                    ui::EndCombo();
                }
            }

            ui::SameLine();
            ui::Checkbox("autoscroll", &autoscroll_);

            ui::SameLine();
            if (ui::Button("clear")) {
                global_get_traceback_log().lock()->clear();
            }
            App::upd().add_frame_annotation("LogClearButton", ui::GetItemRect());

            ui::SameLine();
            if (ui::Button("turn off")) {
                logger->set_level(LogLevel::off);
            }

            ui::SameLine();
            if (ui::Button("copy to clipboard")) {
                copy_traceback_log_to_clipboard();
            }

            ui::Dummy({0.0f, 10.0f});

            ui::EndMenuBar();
        }

        // draw log content lines
        auto& guarded_content = global_get_traceback_log();
        const auto& guard = guarded_content.lock();
        for (const LogMessage& log_message : *guard) {
            ui::PushStyleColor(ImGuiCol_Text, ::to_color(log_message.level()));
            ui::Text("[%s]", to_cstringview(log_message.level()).c_str());
            ui::PopStyleColor();
            ui::SameLine();
            ui::TextWrapped(log_message.payload());

            if (autoscroll_) {
                ui::SetScrollHereY();
            }
        }
    }
private:
    bool autoscroll_ = true;
};


// public API

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
