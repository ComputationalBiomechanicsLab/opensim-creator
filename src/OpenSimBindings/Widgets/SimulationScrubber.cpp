#include "SimulationScrubber.hpp"

#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/Platform/App.hpp"

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include <chrono>
#include <memory>
#include <optional>
#include <utility>

class osc::SimulationScrubber::Impl final {
public:

    Impl(std::shared_ptr<Simulation> sim) :
        m_Simulation{std::move(sim)}
    {
    }

    bool isPlayingBack() const
    {
        return m_IsPlayingBack;
    }

    // returns the playback position, which changes based on the wall clock (it's a playback)
    // onto the time within a simulation
    SimulationClock::time_point getScrubPositionInSimTime() const
    {
        if (!m_IsPlayingBack)
        {
            return m_PlaybackStartSimtime;
        }
        else
        {
            // map wall time onto sim time

            int nReports = m_Simulation->getNumReports();
            if (nReports <= 0)
            {
                return m_Simulation->getStartTime();
            }
            else
            {
                std::chrono::system_clock::time_point wallNow = std::chrono::system_clock::now();
                auto wallDur = wallNow - m_PlaybackStartWallTime;

                osc::SimulationClock::time_point simNow = m_PlaybackStartSimtime + osc::SimulationClock::duration{wallDur};
                osc::SimulationClock::time_point simLatest = m_Simulation->getSimulationReport(nReports-1).getTime();

                return simNow <= simLatest ? simNow : simLatest;
            }
        }
    }

    std::optional<SimulationReport> tryLookupReportBasedOnScrubbing()
    {
        int nReports = m_Simulation->getNumReports();

        if (nReports <= 0)
        {
            return std::nullopt;
        }

        osc::SimulationClock::time_point t = getScrubPositionInSimTime();

        for (int i = 0; i < nReports; ++i)
        {
            osc::SimulationReport r = m_Simulation->getSimulationReport(i);
            if (r.getTime() >= t)
            {
                return r;
            }
        }

        return m_Simulation->getSimulationReport(nReports-1);
    }

    void scrubTo(SimulationClock::time_point t)
    {
        m_PlaybackStartSimtime = t;
        m_IsPlayingBack = false;
    }

    void onTick()
    {
        if (m_IsPlayingBack)
        {
            auto playbackPos = getScrubPositionInSimTime();
            if (playbackPos < m_Simulation->getEndTime())
            {
                osc::App::upd().requestRedraw();
            }
            else
            {
                m_PlaybackStartSimtime = playbackPos;
                m_IsPlayingBack = false;
                return;
            }
        }
    }

    void onDraw()
    {
        osc::SimulationClock::time_point tStart = m_Simulation->getStartTime();
        osc::SimulationClock::time_point tEnd = m_Simulation->getEndTime();
        osc::SimulationClock::time_point tCur = getScrubPositionInSimTime();

        // play/pause buttons
        if (tCur >= tEnd)
        {
            if (ImGui::Button(ICON_FA_REDO))
            {
                m_PlaybackStartSimtime = tStart;
                m_PlaybackStartWallTime = std::chrono::system_clock::now();
                m_IsPlayingBack = true;
            }
        }
        else if (!m_IsPlayingBack)
        {
            if (ImGui::Button(ICON_FA_PLAY))
            {
                m_PlaybackStartWallTime = std::chrono::system_clock::now();
                m_IsPlayingBack = true;
            }
        }
        else
        {
            if (ImGui::Button(ICON_FA_PAUSE))
            {
                m_PlaybackStartSimtime = getScrubPositionInSimTime();
                m_IsPlayingBack = false;
            }
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        float v = static_cast<float>(tCur.time_since_epoch().count());
        bool userScrubbed = ImGui::SliderFloat("##scrubber",
            &v,
            static_cast<float>(tStart.time_since_epoch().count()),
            static_cast<float>(tEnd.time_since_epoch().count()),
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp);

        if (userScrubbed)
        {
            m_PlaybackStartSimtime = osc::SimulationClock::start() + osc::SimulationClock::duration{static_cast<double>(v)};
            m_PlaybackStartWallTime = std::chrono::system_clock::now();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Left-Click: Change simulation time being shown");
            ImGui::TextUnformatted("Ctrl-Click: Type in the simulation time being shown");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

private:
    std::shared_ptr<Simulation> m_Simulation;
    bool m_IsPlayingBack = true;
    SimulationClock::time_point m_PlaybackStartSimtime = m_Simulation->getStartTime();
    std::chrono::system_clock::time_point m_PlaybackStartWallTime = std::chrono::system_clock::now();
};

osc::SimulationScrubber::SimulationScrubber(std::shared_ptr<Simulation> sim) :
    m_Impl{new Impl{std::move(sim)}}
{
}

osc::SimulationScrubber::SimulationScrubber(SimulationScrubber&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SimulationScrubber& osc::SimulationScrubber::operator=(SimulationScrubber&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SimulationScrubber::~SimulationScrubber() noexcept
{
    delete m_Impl;
}

bool osc::SimulationScrubber::isPlayingBack() const
{
    return m_Impl->isPlayingBack();
}

osc::SimulationClock::time_point osc::SimulationScrubber::getScrubPositionInSimTime() const
{
    return m_Impl->getScrubPositionInSimTime();
}

std::optional<osc::SimulationReport> osc::SimulationScrubber::tryLookupReportBasedOnScrubbing()
{
    return m_Impl->tryLookupReportBasedOnScrubbing();
}

void osc::SimulationScrubber::scrubTo(SimulationClock::time_point t)
{
    return m_Impl->scrubTo(std::move(t));
}

void osc::SimulationScrubber::onTick()
{
    m_Impl->onTick();
}

void osc::SimulationScrubber::onDraw()
{
    m_Impl->onDraw();
}

bool osc::SimulationScrubber::onEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}
