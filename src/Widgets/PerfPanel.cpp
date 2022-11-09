#include "PerfPanel.hpp"

#include "src/Platform/App.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Widgets/NamedPanel.hpp"

#include <imgui.h>

#include <cinttypes>
#include <chrono>
#include <string>
#include <memory>
#include <utility>
#include <vector>

static bool HighestTotalDuration(osc::PerfMeasurement const& a, osc::PerfMeasurement const& b)
{
    return a.getTotalDuration() > b.getTotalDuration();
}

static bool LexographicallyHighestLabel(osc::PerfMeasurement const& a, osc::PerfMeasurement const& b)
{
    return a.getLabel() > b.getLabel();
}

class osc::PerfPanel::Impl final : public osc::NamedPanel {
public:

    Impl(std::string_view panelName) :
        NamedPanel{std::move(panelName)}
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
            bool waiting = osc::App::get().isMainLoopWaiting();
            if (ImGui::Checkbox("waiting", &waiting))
            {
                osc::App::upd().setMainLoopWaiting(waiting);
            }
        }
        {
            bool vsync = osc::App::get().isVsyncEnabled();
            if (ImGui::Checkbox("VSYNC", &vsync))
            {
                osc::App::upd().setVsync(vsync);
            }
        }
        if (ImGui::Button("clear measurements"))
        {
            osc::ClearPerfMeasurements();
        }
        ImGui::Checkbox("pause", &m_IsPaused);

        if (!m_IsPaused)
        {
            m_MeasurementBuffer.clear();
            GetAllMeasurements(m_MeasurementBuffer);
            Sort(m_MeasurementBuffer, LexographicallyHighestLabel);
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

            for (osc::PerfMeasurement const& pm : m_MeasurementBuffer)
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
    std::vector<osc::PerfMeasurement> m_MeasurementBuffer;
};


// public API

osc::PerfPanel::PerfPanel(std::string_view panelName) :
    m_Impl{std::make_unique<Impl>(std::move(panelName))}
{
}

osc::PerfPanel::PerfPanel(PerfPanel&&) noexcept = default;
osc::PerfPanel& osc::PerfPanel::operator=(PerfPanel&&) noexcept = default;
osc::PerfPanel::~PerfPanel() = default;

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

void osc::PerfPanel::implDraw()
{
    m_Impl->draw();
}
