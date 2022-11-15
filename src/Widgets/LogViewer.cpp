#include "LogViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/CircularBuffer.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <imgui.h>

#include <string>
#include <sstream>
#include <type_traits>
#include <utility>

namespace
{
    [[nodiscard]] ImVec4 color(osc::log::level::LevelEnum lvl)
    {
        switch (lvl)
        {
        case osc::log::level::trace:
            return ImVec4{0.5f, 0.5f, 0.5f, 1.0f};
        case osc::log::level::debug:
            return ImVec4{0.8f, 0.8f, 0.8f, 1.0f};
        case osc::log::level::info:
            return ImVec4{0.5f, 0.5f, 1.0f, 1.0f};
        case osc::log::level::warn:
            return ImVec4{1.0f, 1.0f, 0.0f, 1.0f};
        case osc::log::level::err:
            return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
        case osc::log::level::critical:
            return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
        default:
            return ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
        }
    }

    void copyTracebackLogToClipboard()
    {
        std::stringstream ss;

        auto& guarded_content = osc::log::getTracebackLog();
        {
            auto const& content = guarded_content.lock();
            for (osc::log::OwnedLogMessage const& msg : *content)
            {
                ss << '[' << osc::log::toCStr(msg.level) << "] " << msg.payload << '\n';
            }
        }

        std::string full_log_content = std::move(ss).str();

        osc::SetClipboardText(full_log_content.c_str());
    }
}

class osc::LogViewer::Impl final {
public:
    void draw()
    {
        // draw top menu bar
        if (ImGui::BeginMenuBar())
        {

            // draw level selector
            {
                int lvl = static_cast<int>(log::getTracebackLevel());
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::Combo("level", &lvl, log::level::g_LogLevelCStrings, log::level::NUM_LEVELS))
                {
                    log::setTracebackLevel(static_cast<log::level::LevelEnum>(lvl));
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("autoscroll", &autoscroll);

            ImGui::SameLine();
            if (ImGui::Button("clear"))
            {
                log::getTracebackLog().lock()->clear();
            }
            osc::App::upd().addFrameAnnotation("LogClearButton", osc::GetItemRect());

            ImGui::SameLine();
            if (ImGui::Button("turn off"))
            {
                log::setTracebackLevel(log::level::off);
            }

            ImGui::SameLine();
            if (ImGui::Button("copy to clipboard"))
            {
                copyTracebackLogToClipboard();
            }

            ImGui::Dummy({0.0f, 10.0f});

            ImGui::EndMenuBar();
        }

        // draw log content lines
        auto& guardedContent = log::getTracebackLog();
        auto const& contentGuard = guardedContent.lock();
        for (log::OwnedLogMessage const& msg : *contentGuard)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color(msg.level));
            ImGui::Text("[%s]", log::toCStr(msg.level));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.payload.c_str());

            if (autoscroll)
            {
                ImGui::SetScrollHereY();
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

osc::LogViewer::~LogViewer() noexcept = default;

void osc::LogViewer::draw()
{
    m_Impl->draw();
}
