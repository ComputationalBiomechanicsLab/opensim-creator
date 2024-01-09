#include "PerfPanel.hpp"

#include <oscar/Platform/App.hpp>
#include <oscar/UI/Panels/StandardPanelImpl.hpp>
#include <oscar/Utils/Perf.hpp>

#include <imgui.h>

#include <cinttypes>
#include <chrono>
#include <string>
#include <memory>
#include <utility>
#include <vector>

using osc::PerfMeasurement;

namespace
{
    bool LexographicallyHighestLabel(PerfMeasurement const& a, PerfMeasurement const& b)
    {
        return std::string_view{a.getLabel()} > std::string_view{b.getLabel()};
    }
}

class osc::PerfPanel::Impl final : public osc::StandardPanelImpl {
public:

    explicit Impl(std::string_view panelName) :
        StandardPanelImpl{panelName}
    {
    }

private:
    void implDrawContent() final
    {
        ImGui::Columns(2);
        ImGui::TextUnformatted("FPS");
        ImGui::NextColumn();
        ImGui::Text("%.0f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::NextColumn();
        ImGui::Columns();

        {
            bool waiting = App::get().isMainLoopWaiting();
            if (ImGui::Checkbox("waiting", &waiting))
            {
                App::upd().setMainLoopWaiting(waiting);
            }
        }
        {
            bool vsync = App::get().isVsyncEnabled();
            if (ImGui::Checkbox("VSYNC", &vsync))
            {
                App::upd().setVsync(vsync);
            }
        }
        if (ImGui::Button("clear measurements"))
        {
            osc::ClearAllPerfMeasurements();
        }
        ImGui::Checkbox("pause", &m_IsPaused);

        std::vector<PerfMeasurement> measurements;
        if (!m_IsPaused)
        {
            measurements = GetAllPerfMeasurements();
            std::sort(measurements.begin(), measurements.end(), LexographicallyHighestLabel);
        }

        ImGuiTableFlags flags =
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInner;
        if (ImGui::BeginTable("measurements", 6, flags))
        {
            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("Source File");
            ImGui::TableSetupColumn("Num Calls");
            ImGui::TableSetupColumn("Last Duration");
            ImGui::TableSetupColumn("Average Duration");
            ImGui::TableSetupColumn("Total Duration");
            ImGui::TableHeadersRow();

            for (PerfMeasurement const& pm : measurements)
            {
                if (pm.getCallCount() <= 0)
                {
                    continue;
                }

                int column = 0;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(column++);
                ImGui::TextUnformatted(pm.getLabel().c_str());
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%s:%u", pm.getFilename().c_str(), pm.getLine());
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%" PRId64, pm.getCallCount());
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%ld us", static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(pm.getLastDuration()).count()));
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%ld us", static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(pm.getAvgDuration()).count()));
                ImGui::TableSetColumnIndex(column++);
                ImGui::Text("%ld us", static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(pm.getTotalDuration()).count()));
            }

            ImGui::EndTable();
        }
    }

    bool m_IsPaused = false;
};


// public API

osc::PerfPanel::PerfPanel(std::string_view panelName) :
    m_Impl{std::make_unique<Impl>(panelName)}
{
}

osc::PerfPanel::PerfPanel(PerfPanel&&) noexcept = default;
osc::PerfPanel& osc::PerfPanel::operator=(PerfPanel&&) noexcept = default;
osc::PerfPanel::~PerfPanel() noexcept = default;

osc::CStringView osc::PerfPanel::implGetName() const
{
    return m_Impl->getName();
}

bool osc::PerfPanel::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::PerfPanel::implOpen()
{
    return m_Impl->open();
}

void osc::PerfPanel::implClose()
{
    m_Impl->close();
}

void osc::PerfPanel::implOnDraw()
{
    m_Impl->onDraw();
}
