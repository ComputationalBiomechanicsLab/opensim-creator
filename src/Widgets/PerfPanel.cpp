#include "PerfPanel.hpp"

#include "src/Platform/App.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Perf.hpp"

#include <imgui.h>

#include <cinttypes>
#include <string>
#include <memory>
#include <utility>

static bool HighestTotalDuration(osc::PerfMeasurement const& a, osc::PerfMeasurement const& b)
{
    return a.getTotalDuration() > b.getTotalDuration();
}

class osc::PerfPanel::Impl final {
public:
    Impl(std::string_view panelName) :
        m_PanelName{std::move(panelName)}
    {
    }

    void open()
    {
        m_IsOpen = true;
    }

    void close()
    {
        m_IsOpen = false;
    }

    bool draw()
    {
        if (!m_IsOpen)
        {
            return false;
        }

        if (!ImGui::Begin(m_PanelName.c_str(), &m_IsOpen))
        {
            ImGui::End();
            return false;
        }

        ImGui::Columns(2);
        ImGui::TextUnformatted("FPS");
        ImGui::NextColumn();
        ImGui::Text("%.0f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::NextColumn();
        ImGui::Columns();

        {
            bool waiting = osc::App::cur().isMainLoopWaiting();
            if (ImGui::Checkbox("waiting", &waiting))
            {
                osc::App::cur().setMainLoopWaiting(waiting);
            }
        }
        {
            bool vsync = osc::App::cur().isVsyncEnabled();
            if (ImGui::Checkbox("VSYNC", &vsync))
            {
                osc::App::cur().setVsync(vsync);
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
            Sort(m_MeasurementBuffer, HighestTotalDuration);
        }

        ImGui::Columns(6);
        for (osc::PerfMeasurement const& pm : m_MeasurementBuffer)
        {
            if (pm.getCallCount() <= 0)
            {
                continue;
            }

            ImGui::TextUnformatted(pm.getLabel().c_str());
            ImGui::NextColumn();
            ImGui::Text("%s:%u", pm.getFilename().c_str(), pm.getLine());
            ImGui::NextColumn();
            ImGui::Text("%" PRId64, pm.getCallCount());
            ImGui::NextColumn();
            ImGui::Text("%lld us", std::chrono::duration_cast<std::chrono::microseconds>(pm.getLastDuration()).count());
            ImGui::NextColumn();
            ImGui::Text("%lld us", std::chrono::duration_cast<std::chrono::microseconds>(pm.getAvgDuration()).count());
            ImGui::NextColumn();
            ImGui::Text("%lld us", std::chrono::duration_cast<std::chrono::microseconds>(pm.getTotalDuration()).count());
            ImGui::NextColumn();
        }
        ImGui::Columns();

        ImGui::End();

        return m_IsOpen;
    }

private:
    bool m_IsOpen = true;
    bool m_IsPaused = false;
    std::string m_PanelName;
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

void osc::PerfPanel::open()
{
    m_Impl->open();
}

void osc::PerfPanel::close()
{
    m_Impl->close();
}

bool osc::PerfPanel::draw()
{
    return m_Impl->draw();
}
