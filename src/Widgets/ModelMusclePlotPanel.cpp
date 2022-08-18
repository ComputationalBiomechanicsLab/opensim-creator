#include "ModelMusclePlotPanel.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/ModelStateCommit.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/CircularBuffer.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <glm/glm.hpp>
#include <imgui.h>
#include <implot.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/PropertyObjArray.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

namespace SimTK { class State; }

namespace
{
    class MuscleOutput {
    public:
        MuscleOutput(char const* name, char const* units, double(*getter)(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)) :
            m_Name{std::move(name)},
            m_Units{std::move(units)},
            m_Getter{std::move(getter)}
        {
        }

        char const* getName() const
        {
            return m_Name.c_str();
        }

        char const* getUnits() const
        {
            return m_Units.c_str();
        }

        double operator()(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c) const
        {
            return m_Getter(st, muscle, c);
        }

    private:
        friend bool operator<(MuscleOutput const&, MuscleOutput const&);
        friend bool operator==(MuscleOutput const&, MuscleOutput const&);
        friend bool operator!=(MuscleOutput const&, MuscleOutput const&);

        osc::CStringView m_Name;
        osc::CStringView m_Units;
        double(*m_Getter)(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c);
    };

    bool operator<(MuscleOutput const& a, MuscleOutput const& b)
    {
        return a.m_Name < b.m_Name;
    }

    bool operator==(MuscleOutput const& a, MuscleOutput const& b)
    {
        return a.m_Name == b.m_Name && a.m_Units == b.m_Units && a.m_Getter == b.m_Getter;
    }

    bool operator!=(MuscleOutput const& a, MuscleOutput const& b)
    {
        return !(a == b);
    }
}

static double GetMomentArm(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
    return muscle.getGeometryPath().computeMomentArm(st, c);
}

static double GetFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberLength(st);
}

static double GetTendonLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonLength(st);
}

static double GetPennationAngle(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return glm::degrees(muscle.getPennationAngle(st));
}

static double GetNormalizedFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getNormalizedFiberLength(st);
}

static double GetTendonStrain(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonStrain(st);
}

static double GetFiberPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberPotentialEnergy(st);
}

static double GetTendonPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonPotentialEnergy(st);
}

static double GetMusclePotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getMusclePotentialEnergy(st);
}

static double GetTendonForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonForce(st);
}

static double GetActiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getActiveFiberForce(st);
}

static double GetPassiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getPassiveFiberForce(st);
}

static double GetTotalFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberForce(st);
}

static double GetFiberStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberStiffness(st);
}

static double GetFiberStiffnessAlongTendon(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberStiffnessAlongTendon(st);
}

static double GetTendonStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonStiffness(st);
}

static double GetMuscleStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getMuscleStiffness(st);
}

static double GetFiberActivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberActivePower(st);
}

static double GetFiberPassivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getFiberActivePower(st);
}

static double GetTendonPower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonPower(st);
}

static double GetMusclePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
    return muscle.getTendonPower(st);
}

static MuscleOutput GetDefaultMuscleOutput()
{
    return MuscleOutput{"Moment Arm", "Unitless", GetMomentArm};
}

static std::vector<MuscleOutput> InitMuscleOutputs()
{
    std::vector<MuscleOutput> rv =
    {{
        GetDefaultMuscleOutput(),
        {"Tendon Length", "m", GetTendonLength},
        {"Fiber Length", "m", GetFiberLength},
        {"Pennation Angle", "deg", GetPennationAngle},
        {"Normalized Fiber Length", "Unitless", GetNormalizedFiberLength},
        {"Tendon Strain", "Unitless", GetTendonStrain},
        {"Fiber Potential Energy", "J", GetFiberPotentialEnergy},
        {"Tendon Potential Energy", "J", GetTendonPotentialEnergy},
        {"Muscle Potential Energy", "J", GetMusclePotentialEnergy},
        {"Tendon Force", "N", GetTendonForce},
        {"Active Fiber Force", "N", GetActiveFiberForce},
        {"Passive Fiber Force", "N", GetPassiveFiberForce},
        {"Total Fiber Force", "N", GetTotalFiberForce},
        {"Fiber Stiffness", "N/m", GetFiberStiffness},
        {"Fiber Stiffness Along Tendon", "N/m", GetFiberStiffnessAlongTendon},
        {"Tendon Stiffness", "N/m", GetTendonStiffness},
        {"Muscle Stiffness", "N/m", GetMuscleStiffness},
        {"Fiber Active Power", "W", GetFiberActivePower},
        {"Fiber Passive Power", "W", GetFiberPassivePower},
        {"Tendon Power", "W", GetTendonPower},
        {"Muscle Power", "W", GetMusclePower},
    }};
    std::sort(rv.begin(), rv.end());
    return rv;
}

static std::vector<MuscleOutput> const& GetMuscleOutputs()
{
    static std::vector<MuscleOutput> g_MuscleOutputs = InitMuscleOutputs();
    return g_MuscleOutputs;
}

// background computing stuff
namespace
{
    // parameters used to create a plot
    class PlotParameters final {
    public:
        PlotParameters(osc::ModelStateCommit commit,
                       OpenSim::ComponentPath coordinatePath,
                       OpenSim::ComponentPath musclePath,
                       MuscleOutput output,
                       int requestedNumDataPoints) :

            m_Commit{std::move(commit)},
            m_CoordinatePath{std::move(coordinatePath)},
            m_MusclePath{std::move(musclePath)},
            m_Output{std::move(output)},
            m_RequestedNumDataPoints{std::move(requestedNumDataPoints)}
        {
        }

        osc::ModelStateCommit const& getCommit() const
        {
            return m_Commit;
        }

        void setCommit(osc::ModelStateCommit const& commit)
        {
            m_Commit = commit;
        }

        OpenSim::ComponentPath const& getCoordinatePath() const
        {
            return m_CoordinatePath;
        }

        void setCoordinatePath(OpenSim::ComponentPath const& cp)
        {
            m_CoordinatePath = cp;
        }

        OpenSim::ComponentPath const& getMusclePath() const
        {
            return m_MusclePath;
        }

        void setMusclePath(OpenSim::ComponentPath const& cp)
        {
            m_MusclePath = cp;
        }

        MuscleOutput const& getMuscleOutput() const
        {
            return m_Output;
        }

        void setMuscleOutput(MuscleOutput const& output)
        {
            m_Output = output;
        }

        int getNumRequestedDataPoints() const
        {
            return m_RequestedNumDataPoints;
        }

        void setNumRequestedDataPoints(int v)
        {
            m_RequestedNumDataPoints = v;
        }

    private:
        friend bool operator==(PlotParameters const&, PlotParameters const&);
        friend bool operator!=(PlotParameters const&, PlotParameters const&);

        osc::ModelStateCommit m_Commit;
        OpenSim::ComponentPath m_CoordinatePath;
        OpenSim::ComponentPath m_MusclePath;
        MuscleOutput m_Output;
        int m_RequestedNumDataPoints;
    };

    bool operator==(PlotParameters const& a, PlotParameters const& b)
    {
        return
            a.m_Commit == b.m_Commit &&
            a.m_CoordinatePath == b.m_CoordinatePath &&
            a.m_MusclePath == b.m_MusclePath &&
            a.m_Output == b.m_Output &&
            a.m_RequestedNumDataPoints == b.m_RequestedNumDataPoints;
    }

    bool operator!=(PlotParameters const& a, PlotParameters const& b)
    {
        return !(a == b);
    }

    double GetFirstXValue(PlotParameters const& p, OpenSim::Coordinate const& c)
    {
        return c.getRangeMin();
    }

    double GetLastXValue(PlotParameters const& p, OpenSim::Coordinate const& c)
    {
        return c.getRangeMax();
    }

    double GetStepBetweenXValues(PlotParameters const& p, OpenSim::Coordinate const& c)
    {
        double start = GetFirstXValue(p, c);
        double end = GetLastXValue(p, c);

        return (end - start) / std::max(1, p.getNumRequestedDataPoints()-1);
    }

    // a single data point in the plot, as emitted by the plotting backend
    struct PlotDataPoint final {
        float x;
        float y;
    };

    // the status of a "live" plotting task
    enum class PlottingTaskStatus {
        Running,
        Cancelled,
        Finished,
        Error,
    };

    // a "live" plotting task that computes plot datapoints on a background thread
    class PlottingTask final {
    public:
        PlottingTask(PlotParameters const& params, std::function<void(PlotDataPoint)> callback) :
            m_Parameters{params},
            m_DataPointConsumer{std::move(callback)},
            m_WorkerThread{[this](osc::stop_token t) { run(std::move(t)); }}
        {
        }

        void wait()
        {
            if (m_WorkerThread.joinable())
            {
                m_WorkerThread.join();
            }
        }

        void cancelAndWait()
        {
            m_WorkerThread.request_stop();
            wait();
        }

        PlottingTaskStatus getStatus() const
        {
            return m_Status.load();
        }

        std::optional<std::string> getErrorString() const
        {
            return *m_ErrorString.lock();
        }

        PlotParameters const& getParameters() const
        {
            return m_Parameters;
        }

        float getProgress() const
        {
            return m_Progress.load();
        }

    private:
        void run(osc::stop_token stopToken)
        {
            // TODO: exception handling

            if (m_Parameters.getNumRequestedDataPoints() <= 0)
            {
                auto errorLock = m_ErrorString.lock();
                *errorLock = "<= 0 data points requested: cannot create a plot";
                m_Status = PlottingTaskStatus::Error;
                return;
            }

            std::unique_ptr<OpenSim::Model> model = std::make_unique<OpenSim::Model>(*m_Parameters.getCommit().getModel());

            if (stopToken.stop_requested())
            {
                m_Status = PlottingTaskStatus::Cancelled;
                return;
            }

            osc::InitializeModel(*model);

            if (stopToken.stop_requested())
            {
                m_Status = PlottingTaskStatus::Cancelled;
                return;
            }

            SimTK::State& state = osc::InitializeState(*model);

            if (stopToken.stop_requested())
            {
                m_Status = PlottingTaskStatus::Cancelled;
                return;
            }

            OpenSim::Muscle const* maybeMuscle = osc::FindComponent<OpenSim::Muscle>(*model, m_Parameters.getMusclePath());
            if (!maybeMuscle)
            {
                auto errorLock = m_ErrorString.lock();
                *errorLock = m_Parameters.getMusclePath().toString() + ": cannot find a muscle with this name";
                m_Status = PlottingTaskStatus::Error;
                return;
            }
            OpenSim::Muscle const& muscle = *maybeMuscle;

            OpenSim::Coordinate const* maybeCoord = osc::FindComponentMut<OpenSim::Coordinate>(*model, m_Parameters.getCoordinatePath());
            if (!maybeCoord)
            {
                auto errorLock = m_ErrorString.lock();
                *errorLock = m_Parameters.getCoordinatePath().toString() + ": cannot find a coordinate with this name";
                m_Status = PlottingTaskStatus::Error;
                return;
            }
            OpenSim::Coordinate const& coord = *maybeCoord;

            int const numDataPoints = m_Parameters.getNumRequestedDataPoints();
            double const firstXValue = GetFirstXValue(m_Parameters, coord);
            double const stepBetweenXValues = GetStepBetweenXValues(m_Parameters, coord);

            // this fixes an unusual bug (#352), where the underlying assembly solver in the
            // model ends up retaining invalid values across a coordinate (un)lock, which makes
            // it sets coordinate values from X (what we want) to 0 after model assembly
            //
            // I don't exactly know *why* it's doing it - it looks like OpenSim holds a solver
            // internally that, itself, retains invalid coordinate values or something
            //
            // see #352 for a lengthier explanation
            coord.setLocked(state, false);
            model->updateAssemblyConditions(state);

            if (stopToken.stop_requested())
            {
                m_Status = PlottingTaskStatus::Cancelled;
                return;
            }

            for (int i = 0; i < numDataPoints; ++i)
            {
                if (stopToken.stop_requested())
                {
                    m_Status = PlottingTaskStatus::Cancelled;
                    return;
                }

                double xVal = firstXValue + (i * stepBetweenXValues);
                coord.setValue(state, xVal);

                if (stopToken.stop_requested())
                {
                    m_Status = PlottingTaskStatus::Cancelled;
                    return;
                }

                model->equilibrateMuscles(state);

                if (stopToken.stop_requested())
                {
                    m_Status = PlottingTaskStatus::Cancelled;
                    return;
                }

                model->realizeReport(state);

                if (stopToken.stop_requested())
                {
                    m_Status = PlottingTaskStatus::Cancelled;
                    return;
                }

                float const yVal = static_cast<float>(m_Parameters.getMuscleOutput()(state, muscle, coord));

                m_DataPointConsumer(PlotDataPoint{osc::ConvertCoordValueToDisplayValue(coord, xVal), yVal});
                m_Progress = static_cast<float>(i+1) / static_cast<float>(numDataPoints);
            }

            m_Status = PlottingTaskStatus::Finished;
        }

        PlotParameters m_Parameters;
        std::function<void(PlotDataPoint)> m_DataPointConsumer;
        std::atomic<PlottingTaskStatus> m_Status = PlottingTaskStatus::Running;
        std::atomic<float> m_Progress = 0.0f;
        osc::SynchronizedValue<std::string> m_ErrorString;
        osc::jthread m_WorkerThread;
    };

    // a plot, created from the data produced by the plotting task
    class Plot final {
    public:
        explicit Plot(PlotParameters const& parameters) :
            m_Parameters{parameters}
        {
            m_XValues.reserve(m_Parameters.getNumRequestedDataPoints());
            m_YValues.reserve(m_Parameters.getNumRequestedDataPoints());
        }

        PlotParameters const& getParameters() const
        {
            return m_Parameters;
        }

        nonstd::span<float const> getXValues() const
        {
            return m_XValues;
        }

        nonstd::span<float const> getYValues() const
        {
            return m_YValues;
        }

        void append(PlotDataPoint const& p)
        {
            m_XValues.push_back(p.x);
            m_YValues.push_back(p.y);
        }

    private:
        PlotParameters m_Parameters;
        std::vector<float> m_XValues;
        std::vector<float> m_YValues;
    };

    float lerp(float a, float b, float t)
    {
        return (1.0f - t) * a + t * b;
    }

    std::optional<float> ComputeLERPedY(Plot const& p, float x)
    {
        nonstd::span<float const> const xVals = p.getXValues();
        nonstd::span<float const> const yVals = p.getYValues();

        OSC_ASSERT(xVals.size() == yVals.size());

        if (xVals.empty())
        {
            // there are no X values
            return std::nullopt;
        }

        auto const it = std::lower_bound(xVals.begin(), xVals.end(), x);

        if (it == xVals.end())
        {
            // X is out of bounds
            return std::nullopt;
        }

        if (it == xVals.begin())
        {
            // X if off the left-hand side
            return yVals[0];
        }

        size_t const aboveIdx = std::distance(xVals.begin(), it);
        size_t const belowIdx = aboveIdx - 1;

        float const belowX = xVals[belowIdx];
        float const aboveX = xVals[aboveIdx];
        float const t = (x - belowX) / (aboveX - belowX); // [0..1]

        float const belowY = yVals[belowIdx];
        float const aboveY = yVals[aboveIdx];

        return lerp(belowY, aboveY, t);
    }

    std::optional<PlotDataPoint> FindNearestPoint(Plot const& p, float x)
    {
        nonstd::span<float const> const xVals = p.getXValues();
        nonstd::span<float const> const yVals = p.getYValues();

        OSC_ASSERT(xVals.size() == yVals.size() && "a plot should have an equal number of X and Y values");

        if (xVals.empty())
        {
            // there are no X values
            return std::nullopt;
        }

        auto const it = std::lower_bound(xVals.begin(), xVals.end(), x);

        if (it == xVals.begin())
        {
            // closest is the leftmost point
            return PlotDataPoint{xVals.front(), yVals.front()};
        }

        if (it == xVals.end())
        {
            // closest is the rightmost point
            return PlotDataPoint{xVals.back(), yVals.back()};
        }

        size_t const aboveIdx = std::distance(xVals.begin(), it);
        size_t const belowIdx = aboveIdx - 1;

        // else: the iterator is pointing to the element above the X location and we
        //       need to figure out if that's closer than the element below the X
        //       location
        float const belowX = xVals[aboveIdx - 1];
        float const aboveX = xVals[aboveIdx];

        float const aboveDistance = std::abs(aboveX - x);
        float const belowDistance = std::abs(belowX - x);

        size_t const closestIdx =  aboveDistance < belowDistance  ? aboveIdx : belowIdx;

        return PlotDataPoint{xVals[closestIdx], yVals[closestIdx]};
    }

    bool IsXInRange(Plot const& p, float x)
    {
        nonstd::span<float const> const xVals = p.getXValues();

        if (xVals.size() <= 1)
        {
            return false;
        }

        return xVals.front() <= x && x <= xVals.back();
    }

    std::string ComputePlotLineName(Plot const& p, char const* suffix = nullptr)
    {
        osc::ModelStateCommit const& commit = p.getParameters().getCommit();
        std::stringstream ss;
        ss << commit.getCommitMessage();
        if (suffix)
        {
            ss << ' ' << suffix;
        }
        return std::move(ss).str();
    }
}

// state stuff
namespace
{
    // data that is shared between all states
    struct SharedStateData final {
        explicit SharedStateData(std::shared_ptr<osc::UndoableModelStatePair> uim) : Uim{std::move(uim)}
        {
            OSC_ASSERT(Uim != nullptr);
        }

        SharedStateData(std::shared_ptr<osc::UndoableModelStatePair> uim,
                        OpenSim::ComponentPath const& coordPath,
                        OpenSim::ComponentPath const& musclePath) :
            Uim{std::move(uim)},
            PlotParams{Uim->getLatestCommit(), coordPath, musclePath, GetDefaultMuscleOutput(), 180}
        {
            OSC_ASSERT(Uim != nullptr);
        }

        std::shared_ptr<osc::UndoableModelStatePair> Uim;
        PlotParameters PlotParams{Uim->getLatestCommit(), OpenSim::ComponentPath{}, OpenSim::ComponentPath{}, GetDefaultMuscleOutput(), 180 };
    };

    // base class for a single widget state
    class MusclePlotState {
    protected:
        MusclePlotState(SharedStateData* shared_) : shared{std::move(shared_)}
        {
            OSC_ASSERT(shared != nullptr);
        }
    public:
        virtual ~MusclePlotState() noexcept = default;
        virtual std::unique_ptr<MusclePlotState> draw() = 0;

    protected:
        SharedStateData* shared;
    };

    // state in which the plot is being shown to the user
    class ShowingPlotState final : public MusclePlotState {
    public:
        explicit ShowingPlotState(SharedStateData* shared_) :
            MusclePlotState{std::move(shared_)}
        {
        }

        std::unique_ptr<MusclePlotState> draw() override
        {
            pollForPlotParameterChanges();

            if (m_MaybeActivePlottingTask->getStatus() == PlottingTaskStatus::Error)
            {
                ImGui::Text("error: cannot show plot: %s", m_MaybeActivePlottingTask->getErrorString().value().c_str());
                return nullptr;
            }

            PlotParameters const& latestParams = shared->PlotParams;
            auto modelGuard = latestParams.getCommit().getModel();

            OpenSim::Coordinate const* maybeCoord = osc::FindComponent<OpenSim::Coordinate>(*modelGuard, latestParams.getCoordinatePath());
            if (!maybeCoord)
            {
                ImGui::Text("(no coordinate named %s in model)", latestParams.getCoordinatePath().toString().c_str());
                return nullptr;
            }
            OpenSim::Coordinate const& coord = *maybeCoord;

            glm::vec2 availSize = ImGui::GetContentRegionAvail();

            std::string title = computePlotTitle(coord);
            std::string xAxisLabel = computePlotXAxisTitle(coord);
            std::string yAxisLabel = computePlotYAxisTitle();

            double coordinateXInDegrees = osc::ConvertCoordValueToDisplayValue(coord, coord.getValue(shared->Uim->getState()));

            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2{0.025f, 0.05f});
            if (ImPlot::BeginPlot(title.c_str(), availSize, m_PlotFlags))
            {
                ImPlotAxisFlags xAxisFlags = ImPlotAxisFlags_Lock;
                ImPlotAxisFlags yAxisFlags = ImPlotAxisFlags_AutoFit;
                ImPlot::SetupLegend(m_LegendLocation, m_LegendFlags);
                ImPlot::SetupAxes(xAxisLabel.c_str(), yAxisLabel.c_str(), xAxisFlags, yAxisFlags);
                ImPlot::SetupAxisLimits(ImAxis_X1, osc::ConvertCoordValueToDisplayValue(coord, GetFirstXValue(latestParams, coord)), osc::ConvertCoordValueToDisplayValue(coord, GetLastXValue(latestParams, coord)));

                glm::vec4 const baseColor = {1.0f, 1.0f, 1.0f, 1.0f};

                // plot previous plots
                {
                    int i = 0;
                    for (Plot const& previousPlot : m_PreviousPlots)
                    {
                        glm::vec4 color = baseColor;

                        color.a *= static_cast<float>(i + 1) / static_cast<float>(m_PreviousPlots.size() + 1);

                        if (m_ShowMarkersOnPreviousPlots)
                        {
                            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f);
                        }

                        ImPlot::PushStyleColor(ImPlotCol_Line, color);
                        ImGui::PushID(i++);
                        ImPlot::PlotLine(
                            ComputePlotLineName(previousPlot).c_str(),
                            previousPlot.getXValues().data(),
                            previousPlot.getYValues().data(),
                            static_cast<int>(previousPlot.getXValues().size())
                        );
                        ImGui::PopID();
                        ImPlot::PopStyleColor(ImPlotCol_Line);
                    }
                }

                // show markers for the active plot, so that the user can see where the points
                // were evaluated
                if (m_ShowMarkers)
                {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f);
                }

                // then plot currently active plot
                {
                    auto plotLock = m_ActivePlot.lock();

                    ImPlot::PushStyleColor(ImPlotCol_Line, baseColor);
                    ImPlot::PlotLine(
                        ComputePlotLineName(*plotLock, "(current)").c_str(),
                        plotLock->getXValues().data(),
                        plotLock->getYValues().data(),
                        static_cast<int>(plotLock->getXValues().size())
                    );
                    ImPlot::PopStyleColor(ImPlotCol_Line);
                }

                // figure out mouse hover position
                bool isHovered = ImPlot::IsPlotHovered();
                ImPlotPoint p = ImPlot::GetPlotMousePos();

                if (m_SnapCursor && isHovered)
                {
                    auto plotLock = m_ActivePlot.lock();
                    auto maybeNearest = FindNearestPoint(*plotLock, static_cast<float>(p.x));

                    if (IsXInRange(*plotLock, static_cast<float>(p.x)) && maybeNearest)
                    {
                        p.x = maybeNearest->x;
                    }
                }

                // draw vertical drop line where the coordinate's value currently is
                {
                    double v = coordinateXInDegrees;
                    ImPlot::DragLineX(10, &v, {1.0f, 1.0f, 0.0f, 0.6f}, 1.0f, ImPlotDragToolFlags_NoInputs);
                }

                // also, draw an X tag on the axes where the coordinate's value currently is
                {
                    ImPlot::TagX(coordinateXInDegrees, { 1.0f, 1.0f, 1.0f, 1.0f });
                }

                // draw faded vertial drop line where the mouse currently is
                if (isHovered)
                {
                    double v = p.x;
                    ImPlot::DragLineX(11, &v, {1.0f, 1.0f, 0.0f, 0.3f}, 1.0f, ImPlotDragToolFlags_NoInputs);
                }

                // also, draw a faded X tag on the axes where the mouse currently is (in X)
                if (isHovered)
                {
                    ImPlot::TagX(p.x, { 1.0f, 1.0f, 1.0f, 0.6f });
                }

                // Y values: BEWARE
                //
                // the X values for the droplines/tags above come directly from either the model or
                // mouse: both of which are *continuous* (give or take)
                //
                // the Y values are computed from those continous values by searching through the
                // *discrete* data values of the plot and LERPing them
                {
                    std::optional<float> maybeCoordinateY;
                    std::optional<float> maybeHoverY;

                    auto plotLock = m_ActivePlot.lock();
                    maybeCoordinateY = ComputeLERPedY(*plotLock, static_cast<float>(coordinateXInDegrees));
                    maybeHoverY = ComputeLERPedY(*plotLock, static_cast<float>(p.x));

                    if (maybeCoordinateY)
                    {
                        double v = *maybeCoordinateY;
                        ImPlot::DragLineY(13, &v, {1.0f, 1.0f, 0.0f, 0.6f}, 1.0f, ImPlotDragToolFlags_NoInputs);
                        ImPlot::Annotation(static_cast<float>(coordinateXInDegrees), *maybeCoordinateY, { 1.0f, 1.0f, 1.0f, 1.0f }, { 10.0f, 10.0f }, true, "%f", *maybeCoordinateY);
                    }

                    if (isHovered && maybeHoverY)
                    {
                        double v = *maybeHoverY;
                        ImPlot::DragLineY(14, &v, {1.0f, 1.0f, 0.0f, 0.3f}, 1.0f, ImPlotDragToolFlags_NoInputs);
                        ImPlot::Annotation(p.x, *maybeHoverY, { 1.0f, 1.0f, 1.0f, 0.6f }, { 10.0f, 10.0f }, true, "%f", *maybeHoverY);
                    }
                }

                // if the plot is hovered and the user is holding their left-mouse button down,
                // then "scrub" through the coordinate in the model
                //
                // this is handy for users to visually see how a coordinate affects the model
                if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    if (coord.getDefaultLocked())
                    {
                        osc::DrawTooltip("scrubbing disabled", "you cannot scrub this plot because the coordinate is locked");
                    }
                    else
                    {
                        double storedValue = osc::ConvertCoordDisplayValueToStorageValue(coord, static_cast<float>(p.x));
                        osc::ActionSetCoordinateValue(*shared->Uim, coord, storedValue);
                    }
                }

                // when the user stops dragging their left-mouse around, commit the scrubbed-to
                // coordinate to model storage
                if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    if (coord.getDefaultLocked())
                    {
                        osc::DrawTooltip("scrubbing disabled", "you cannot scrub this plot because the coordinate is locked");
                    }
                    else
                    {
                        double storedValue = osc::ConvertCoordDisplayValueToStorageValue(coord, static_cast<float>(p.x));
                        osc::ActionSetCoordinateValueAndSave(*shared->Uim, coord, storedValue);
                    }
                }

                // draw a context menu with helpful options (set num data points, export, etc.)
                if (ImGui::BeginPopupContextItem((title + "_contextmenu").c_str()))
                {
                    drawPlotDataTypeSelector();

                    int currentDataPoints = shared->PlotParams.getNumRequestedDataPoints();
                    if (ImGui::InputInt("num data points", &currentDataPoints, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        shared->PlotParams.setNumRequestedDataPoints(currentDataPoints);
                    }

                    if (ImGui::MenuItem("clear previous plots"))
                    {
                        m_PreviousPlots.clear();
                    }

                    if (ImGui::BeginMenu("legend"))
                    {
                        drawLegendContextMenuContent();
                        ImGui::EndMenu();
                    }

                    ImGui::MenuItem("show markers", nullptr, &m_ShowMarkers);
                    ImGui::MenuItem("show markers on previous plots", nullptr, &m_ShowMarkersOnPreviousPlots);
                    ImGui::MenuItem("snap cursor to datapoints", nullptr, &m_SnapCursor);

                    ImGui::EndPopup();
                }

                ImPlot::EndPlot();
            }
            ImPlot::PopStyleVar();

            return nullptr;
        }

    private:

        void drawPlotDataTypeSelector()
        {
            nonstd::span<MuscleOutput const> allOutputs = GetMuscleOutputs();

            std::vector<char const*> names;
            int active = -1;
            for (int i = 0; i < static_cast<int>(allOutputs.size()); ++i)
            {
                MuscleOutput const& o = allOutputs[i];
                names.push_back(o.getName());
                if (o == shared->PlotParams.getMuscleOutput())
                {
                    active = i;
                }
            }

            if (ImGui::Combo("data type", &active, names.data(), static_cast<int>(names.size())))
            {
                shared->PlotParams.setMuscleOutput(allOutputs[active]);
            }
        }

        void drawLegendContextMenuContent()
        {
            ImGui::CheckboxFlags("Hide", (unsigned int*)&m_PlotFlags, ImPlotFlags_NoLegend);
            ImGui::CheckboxFlags("Outside",(unsigned int*)&m_LegendFlags, ImPlotLegendFlags_Outside);

            const float s = ImGui::GetFrameHeight();
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2,2));
            if (ImGui::Button("NW",ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_NorthWest; } ImGui::SameLine();
            if (ImGui::Button("N", ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_North;     } ImGui::SameLine();
            if (ImGui::Button("NE",ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_NorthEast; }
            if (ImGui::Button("W", ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_West;      } ImGui::SameLine();
            if (ImGui::InvisibleButton("C", ImVec2(1.5f * s, s))) { m_LegendLocation = ImPlotLocation_Center; } ImGui::SameLine();
            if (ImGui::Button("E", ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_East;      }
            if (ImGui::Button("SW",ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_SouthWest; } ImGui::SameLine();
            if (ImGui::Button("S", ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_South;     } ImGui::SameLine();
            if (ImGui::Button("SE",ImVec2(1.5f*s,s))) { m_LegendLocation = ImPlotLocation_SouthEast; }
            ImGui::PopStyleVar();
        }

        std::string computePlotTitle(OpenSim::Coordinate const& c)
        {
            std::stringstream ss;
            ss << shared->PlotParams.getMusclePath().getComponentName() << ' ';
            appendYAxisName(ss);
            ss << " vs ";
            appendXAxisName(c, ss);
            return std::move(ss).str();
        }

        void appendYAxisName(std::stringstream& ss)
        {
            ss << shared->PlotParams.getMuscleOutput().getName();
        }

        void appendXAxisName(OpenSim::Coordinate const& c, std::stringstream& ss)
        {
            ss << c.getName();
        }

        std::string computePlotYAxisTitle()
        {
            std::stringstream ss;
            appendYAxisName(ss);
            ss << " [" << shared->PlotParams.getMuscleOutput().getUnits() << ']';
            return std::move(ss).str();
        }

        std::string computePlotXAxisTitle(OpenSim::Coordinate const& c)
        {
            std::stringstream ss;
            appendXAxisName(c, ss);
            ss << " value [" << osc::GetCoordDisplayValueUnitsString(c) << ']';
            return std::move(ss).str();
        }

        void pollForPlotParameterChanges()
        {
            // ensure latest requested params reflects the latest version of the model
            shared->PlotParams.setCommit(shared->Uim->getLatestCommit());

            // if the current plot doesn't match the latest requested params, kick off
            // a new plotting task
            if (m_ActivePlot.lock()->getParameters() != shared->PlotParams)
            {
                // cancel current plotting task, to prevent unusual thread races while we
                // shuffle data around
                m_MaybeActivePlottingTask->cancelAndWait();

                // (edge-case): if the user selected a different muscle output then the previous
                // plots have to be cleared out
                bool clearPrevious = m_ActivePlot.lock()->getParameters().getMuscleOutput() != shared->PlotParams.getMuscleOutput();

                // set new active plot
                Plot plot{shared->PlotParams};
                auto lock = m_ActivePlot.lock();
                std::swap(*lock, plot);
                m_PreviousPlots.push_back(std::move(plot));

                if (clearPrevious)
                {
                    m_PreviousPlots.clear();
                }

                // start new plotting task
                m_MaybeActivePlottingTask = std::make_unique<PlottingTask>(shared->PlotParams, [this](PlotDataPoint p) { onDataFromPlottingTask(p); });
            }
        }

        void onDataFromPlottingTask(PlotDataPoint p)
        {
            osc::App::upd().requestRedraw();
            m_ActivePlot.lock()->append(p);
        }

        std::unique_ptr<PlottingTask> m_MaybeActivePlottingTask = std::make_unique<PlottingTask>(shared->PlotParams, [this](PlotDataPoint p) { onDataFromPlottingTask(p); });
        osc::SynchronizedValue<Plot> m_ActivePlot{shared->PlotParams};
        osc::CircularBuffer<Plot, 6> m_PreviousPlots;
        bool m_ShowMarkers = true;
        bool m_ShowMarkersOnPreviousPlots = false;
        bool m_SnapCursor = false;
        ImPlotFlags m_PlotFlags = ImPlotFlags_AntiAliased | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoChild | ImPlotFlags_NoFrame;
        ImPlotLocation m_LegendLocation = ImPlotLocation_NorthWest;
        ImPlotLegendFlags m_LegendFlags = ImPlotLegendFlags_None;
    };

    // state in which a user is being prompted to select a coordinate in the model
    class PickCoordinateState final : public MusclePlotState {
    public:
        explicit PickCoordinateState(SharedStateData* shared_) : MusclePlotState{std::move(shared_)}
        {
            // this is what this state is populating
            shared->PlotParams.setCoordinatePath(osc::GetEmptyComponentPath());
        }

        std::unique_ptr<MusclePlotState> draw() override
        {
            std::unique_ptr<MusclePlotState> rv;

            std::vector<OpenSim::Coordinate const*> coordinates;
            for (OpenSim::Coordinate const& coord : shared->Uim->getModel().getComponentList<OpenSim::Coordinate>())
            {
                coordinates.push_back(&coord);
            }
            osc::Sort(coordinates, osc::IsNameLexographicallyLowerThan<OpenSim::Component const*>);

            ImGui::Text("select coordinate:");

            ImGui::BeginChild("MomentArmPlotCoordinateSelection");
            for (OpenSim::Coordinate const* coord : coordinates)
            {
                if (ImGui::Selectable(coord->getName().c_str()))
                {
                    shared->PlotParams.setCoordinatePath(coord->getAbsolutePath());
                    rv = std::make_unique<ShowingPlotState>(shared);
                }
            }
            ImGui::EndChild();

            return rv;
        }
    };

    // state in which a user is being prompted to select a muscle in the model
    class PickMuscleState final : public MusclePlotState {
    public:
        explicit PickMuscleState(SharedStateData* shared_) : MusclePlotState{std::move(shared_)}
        {
            // this is what this state is populating
            shared->PlotParams.setMusclePath(osc::GetEmptyComponentPath());
        }

        std::unique_ptr<MusclePlotState> draw() override
        {
            std::unique_ptr<MusclePlotState> rv;

            std::vector<OpenSim::Muscle const*> muscles;
            for (OpenSim::Muscle const& musc : shared->Uim->getModel().getComponentList<OpenSim::Muscle>())
            {
                muscles.push_back(&musc);
            }
            osc::Sort(muscles, osc::IsNameLexographicallyLowerThan<OpenSim::Component const*>);

            ImGui::Text("select muscle:");

            if (muscles.empty())
            {
                ImGui::TextDisabled("(the model contains no muscles?)");
            }
            else
            {
                ImGui::BeginChild("MomentArmPlotMuscleSelection");
                for (OpenSim::Muscle const* musc : muscles)
                {
                    if (ImGui::Selectable(musc->getName().c_str()))
                    {
                        shared->PlotParams.setMusclePath(musc->getAbsolutePath());
                        rv = std::make_unique<PickCoordinateState>(shared);
                    }
                }
                ImGui::EndChild();
            }

            return rv;
        }
    };
}

// private IMPL for the muscle plot panel
//
// this effectively operates as a state-machine host, where each state (e.g.
// "choose a muscle", "choose a coordinate") is mostly independent
class osc::ModelMusclePlotPanel::Impl final {
public:

    Impl(std::shared_ptr<UndoableModelStatePair> uim, std::string_view panelName) :
        m_SharedData{std::move(uim)},
        m_ActiveState{std::make_unique<PickMuscleState>(&m_SharedData)},
        m_PanelName{std::move(panelName)}
    {
    }

    Impl(std::shared_ptr<UndoableModelStatePair> uim,
         std::string_view panelName,
         OpenSim::ComponentPath const& coordPath,
         OpenSim::ComponentPath const& musclePath) :
        m_SharedData{std::move(uim), coordPath, musclePath},
        m_ActiveState{std::make_unique<ShowingPlotState>(&m_SharedData)},
        m_PanelName{std::move(panelName)}
    {
    }

    std::string const& getName() const
    {
        return m_PanelName;
    }

    bool isOpen() const
    {
        return m_IsOpen;
    }

    void open()
    {
        m_IsOpen = true;
    }

    void close()
    {
        m_IsOpen = false;
    }

    void draw()
    {
        if (m_IsOpen)
        {
            bool isOpen = m_IsOpen;
            if (ImGui::Begin(m_PanelName.c_str(), &isOpen))
            {
                if (auto maybeNextState = m_ActiveState->draw())
                {
                    m_ActiveState = std::move(maybeNextState);
                }
                m_IsOpen = isOpen;
            }
            ImGui::End();

            if (isOpen != m_IsOpen)
            {
                m_IsOpen = isOpen;
            }
        }
    }

private:
    // data that's shared between all states
    SharedStateData m_SharedData;

    // currently active state (this class controls a state machine)
    std::unique_ptr<MusclePlotState> m_ActiveState;

    // name of the panel, as shown in the UI (via ImGui::Begin)
    std::string m_PanelName;

    // if the panel is currently open or not
    bool m_IsOpen = true;
};


// public API (PIMPL)

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(std::shared_ptr<UndoableModelStatePair> uim,
                                                std::string_view panelName) :
    m_Impl{new Impl{std::move(uim), std::move(panelName)}}
{
}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(std::shared_ptr<UndoableModelStatePair> uim,
                                                std::string_view panelName,
                                                OpenSim::ComponentPath const& coordPath,
                                                OpenSim::ComponentPath const& musclePath) :
    m_Impl{new Impl{std::move(uim), std::move(panelName), coordPath, musclePath}}
{
}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(ModelMusclePlotPanel&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ModelMusclePlotPanel& osc::ModelMusclePlotPanel::operator=(ModelMusclePlotPanel&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ModelMusclePlotPanel::~ModelMusclePlotPanel() noexcept
{
    delete m_Impl;
}

std::string const& osc::ModelMusclePlotPanel::getName() const
{
    return m_Impl->getName();
}

bool osc::ModelMusclePlotPanel::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::ModelMusclePlotPanel::open()
{
    m_Impl->open();
}

void osc::ModelMusclePlotPanel::close()
{
    m_Impl->close();
}

void osc::ModelMusclePlotPanel::draw()
{
    m_Impl->draw();
}
