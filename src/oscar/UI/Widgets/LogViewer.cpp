#include "LogViewer.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CircularBuffer.h>

#include <string>
#include <sstream>
#include <type_traits>
#include <utility>

using namespace osc;

namespace
{
    ImVec4 ToColor(LogLevel lvl)
    {
        switch (lvl)
        {
        case LogLevel::trace:
            return ImVec4{0.5f, 0.5f, 0.5f, 1.0f};
        case LogLevel::debug:
            return ImVec4{0.8f, 0.8f, 0.8f, 1.0f};
        case LogLevel::info:
            return ImVec4{0.5f, 0.5f, 1.0f, 1.0f};
        case LogLevel::warn:
            return ImVec4{1.0f, 1.0f, 0.0f, 1.0f};
        case LogLevel::err:
            return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
        case LogLevel::critical:
            return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
        default:
            return ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
        }
    }

    void copyTracebackLogToClipboard()
    {
        std::stringstream ss;

        auto& guarded_content = getTracebackLog();
        {
            auto const& content = guarded_content.lock();
            for (LogMessage const& msg : *content)
            {
                ss << '[' << ToCStringView(msg.level) << "] " << msg.payload << '\n';
            }
        }

        std::string full_log_content = std::move(ss).str();

        SetClipboardText(full_log_content.c_str());
    }
}

class osc::LogViewer::Impl final {
public:
    void onDraw()
    {
        auto logger = defaultLogger();
        if (!logger)
        {
            return;
        }

        // draw top menu bar
        if (ui::BeginMenuBar())
        {
            // draw level selector
            {
                LogLevel currentLevel = logger->get_level();
                ui::SetNextItemWidth(200.0f);
                if (ui::BeginCombo("level", ToCStringView(currentLevel).c_str()))
                {
                    for (LogLevel l = FirstLogLevel(); l <= LastLogLevel(); l = Next(l))
                    {
                        bool active = l == currentLevel;
                        if (ui::Selectable(ToCStringView(l).c_str(), &active))
                        {
                            logger->set_level(l);
                        }
                    }
                    ui::EndCombo();
                }
            }

            ui::SameLine();
            ui::Checkbox("autoscroll", &autoscroll);

            ui::SameLine();
            if (ui::Button("clear"))
            {
                getTracebackLog().lock()->clear();
            }
            App::upd().addFrameAnnotation("LogClearButton", ui::GetItemRect());

            ui::SameLine();
            if (ui::Button("turn off"))
            {
                logger->set_level(LogLevel::off);
            }

            ui::SameLine();
            if (ui::Button("copy to clipboard"))
            {
                copyTracebackLogToClipboard();
            }

            ui::Dummy({0.0f, 10.0f});

            ui::EndMenuBar();
        }

        // draw log content lines
        auto& guardedContent = getTracebackLog();
        auto const& contentGuard = guardedContent.lock();
        for (LogMessage const& msg : *contentGuard)
        {
            ui::PushStyleColor(ImGuiCol_Text, ::ToColor(msg.level));
            ui::Text("[%s]", ToCStringView(msg.level).c_str());
            ui::PopStyleColor();
            ui::SameLine();
            ui::TextWrapped(msg.payload);

            if (autoscroll)
            {
                ui::SetScrollHereY();
            }
        }
    }
private:
    bool autoscroll = true;
};


// public API

osc::LogViewer::LogViewer() :
    m_Impl{std::make_unique<Impl>()}
{
}
osc::LogViewer::LogViewer(LogViewer&&) noexcept = default;
osc::LogViewer& osc::LogViewer::operator=(LogViewer&&) noexcept = default;
osc::LogViewer::~LogViewer() noexcept = default;

void osc::LogViewer::onDraw()
{
    m_Impl->onDraw();
}
