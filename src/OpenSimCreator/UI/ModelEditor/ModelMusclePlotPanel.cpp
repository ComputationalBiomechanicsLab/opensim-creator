#include "ModelMusclePlotPanel.h"

#include <OpenSimCreator/Documents/Model/ModelStateCommit.h>
#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <oscar/Formats/CSV.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Shims/Cpp20/stop_token.h>
#include <oscar/Shims/Cpp20/thread.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringHelpers.h>
#include <oscar/Utils/SynchronizedValue.h>
#include <oscar/Utils/SynchronizedValueGuard.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace SimTK { class State; }

namespace cpp20 = osc::cpp20;
using namespace osc;

// muscle outputs
//
// wraps OpenSim::Muscle member methods in a higher-level API that the UI
// can present to the user
namespace
{
    // describes a single output from an OpenSim::Muscle
    class PlottableOutput final {
    public:
        PlottableOutput(
            CStringView name,
            CStringView units,
            double(*getter)(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)) :

            m_Name{name},
            m_Units{units},
            m_Getter{getter}
        {
        }

        CStringView getName() const
        {
            return m_Name;
        }

        CStringView getUnits() const
        {
            return m_Units;
        }

        double operator()(
            SimTK::State const& st,
            OpenSim::Muscle const& muscle,
            OpenSim::Coordinate const& c) const
        {
            return m_Getter(st, muscle, c);
        }

        friend bool operator<(PlottableOutput const& lhs, PlottableOutput const& rhs)
        {
            return lhs.m_Name < rhs.m_Name;
        }

        friend bool operator==(PlottableOutput const&, PlottableOutput const&) = default;
    private:
        CStringView m_Name;
        CStringView m_Units;
        double(*m_Getter)(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c);
    };

    double GetMomentArm(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
    {
        return muscle.getGeometryPath().computeMomentArm(st, c);
    }

    double GetFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberLength(st);
    }

    double GetTendonLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonLength(st);
    }

    double GetPennationAngle(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return Degreesd{Radiansd{muscle.getPennationAngle(st)}}.count();
    }

    double GetNormalizedFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getNormalizedFiberLength(st);
    }

    double GetTendonStrain(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonStrain(st);
    }

    double GetFiberPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberPotentialEnergy(st);
    }

    double GetTendonPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonPotentialEnergy(st);
    }

    double GetMusclePotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getMusclePotentialEnergy(st);
    }

    double GetTendonForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonForce(st);
    }

    double GetActiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getActiveFiberForce(st);
    }

    double GetPassiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getPassiveFiberForce(st);
    }

    double GetTotalFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberForce(st);
    }

    double GetFiberStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberStiffness(st);
    }

    double GetFiberStiffnessAlongTendon(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberStiffnessAlongTendon(st);
    }

    double GetTendonStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonStiffness(st);
    }

    double GetMuscleStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getMuscleStiffness(st);
    }

    double GetFiberActivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberActivePower(st);
    }

    double GetFiberPassivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getFiberActivePower(st);
    }

    double GetTendonPower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonPower(st);
    }

    double GetMusclePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
    {
        return muscle.getTendonPower(st);
    }

    PlottableOutput GetDefaultMuscleOutput()
    {
        return PlottableOutput{"Moment Arm", "Unitless", GetMomentArm};
    }

    std::vector<PlottableOutput> GenerateMuscleOutputs()
    {
        std::vector<PlottableOutput> rv =
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
}

// backend datastructures
//
// these are the datastructures that the widget mostly plays around with
namespace
{
    inline constexpr int c_DefaultNumPlotPoints = 65;

    // parameters for generating a plot line
    //
    // i.e. changing any part of the parameters may produce a different curve
    class PlotParameters final {
    public:
        PlotParameters(
            ModelStateCommit commit,
            OpenSim::ComponentPath coordinatePath,
            OpenSim::ComponentPath musclePath,
            PlottableOutput output,
            int requestedNumDataPoints) :

            m_Commit{std::move(commit)},
            m_CoordinatePath{std::move(coordinatePath)},
            m_MusclePath{std::move(musclePath)},
            m_Output{output},
            m_RequestedNumDataPoints{requestedNumDataPoints}
        {
        }

        ModelStateCommit const& getCommit() const
        {
            return m_Commit;
        }

        void setCommit(ModelStateCommit const& commit)
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

        PlottableOutput const& getPlottedOutput() const
        {
            return m_Output;
        }

        void setPlottedOutput(PlottableOutput const& output)
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

        friend bool operator==(PlotParameters const&, PlotParameters const&) = default;
    private:
        ModelStateCommit m_Commit;
        OpenSim::ComponentPath m_CoordinatePath;
        OpenSim::ComponentPath m_MusclePath;
        PlottableOutput m_Output;
        int m_RequestedNumDataPoints;
    };

    double GetFirstXValue(PlotParameters const&, OpenSim::Coordinate const& c)
    {
        return c.getRangeMin();
    }

    double GetLastXValue(PlotParameters const&, OpenSim::Coordinate const& c)
    {
        return c.getRangeMax();
    }

    double GetStepBetweenXValues(PlotParameters const& p, OpenSim::Coordinate const& c)
    {
        double start = GetFirstXValue(p, c);
        double end = GetLastXValue(p, c);

        return (end - start) / std::max(1, p.getNumRequestedDataPoints() - 1);
    }

    // a single data point in the plot, as emitted by a PlottingTask
    struct PlotDataPoint final {
        float x;
        float y;
    };

    // plot data points are naturally ordered by their independent (X) variable
    bool operator<(PlotDataPoint const& lhs, PlotDataPoint const& rhs)
    {
        return lhs.x < rhs.x;
    }

    // virtual interface to a thing that can receive datapoints from a plotter
    class PlotDataPointConsumer {
    protected:
        PlotDataPointConsumer() = default;
        PlotDataPointConsumer(PlotDataPointConsumer const&) = default;
        PlotDataPointConsumer(PlotDataPointConsumer&&) noexcept = default;
        PlotDataPointConsumer& operator=(PlotDataPointConsumer const&) = default;
        PlotDataPointConsumer& operator=(PlotDataPointConsumer&&) noexcept = default;
    public:
        virtual ~PlotDataPointConsumer() noexcept = default;
        virtual void operator()(PlotDataPoint) = 0;
    };

    // the status of a "live" plotting task
    enum class PlottingTaskStatus {
        Running,
        Cancelled,
        Finished,
        Error,
    };

    // mutable data that is shared between the plot worker thread and the top-level
    // plotting task
    class PlottingTaskThreadsafeSharedData final {
    public:
        PlottingTaskStatus getStatus() const
        {
            return m_Status.load();
        }

        std::optional<std::string> getErrorMessage() const
        {
            auto lock = m_ErrorMessage.lock();
            return *lock;
        }

        void setErrorMessage(std::string s)
        {
            auto lock = m_ErrorMessage.lock();
            *lock = std::move(s);
        }

        void setStatus(PlottingTaskStatus s)
        {
            m_Status = s;
        }

    private:
        std::atomic<PlottingTaskStatus> m_Status = PlottingTaskStatus::Running;
        SynchronizedValue<std::string> m_ErrorMessage;
    };

    // all inputs to the plotting function
    struct PlottingTaskInputs final {

        PlottingTaskInputs(
            std::shared_ptr<PlottingTaskThreadsafeSharedData> shared_,
            PlotParameters plotParameters_,
            std::shared_ptr<PlotDataPointConsumer> dataPointConsumer_) :

            shared{std::move(shared_)},
            plotParameters{std::move(plotParameters_)},
            dataPointConsumer{std::move(dataPointConsumer_)}
        {
        }

        std::shared_ptr<PlottingTaskThreadsafeSharedData> shared;
        PlotParameters plotParameters;
        std::shared_ptr<PlotDataPointConsumer> dataPointConsumer;
    };

    // inner (exception unsafe) plot function
    //
    // this is the function that actually does the "work" of computing plot points
    PlottingTaskStatus ComputePlotPointsUnguarded(cpp20::stop_token const& stopToken, PlottingTaskInputs& inputs)
    {
        PlottingTaskThreadsafeSharedData& shared = *inputs.shared;
        PlotParameters const& params = inputs.plotParameters;
        PlotDataPointConsumer& callback = *inputs.dataPointConsumer;

        if (params.getNumRequestedDataPoints() <= 0)
        {
            return PlottingTaskStatus::Finished;
        }

        // create a local copy of the model
        std::unique_ptr<OpenSim::Model> model = std::make_unique<OpenSim::Model>(*params.getCommit().getModel());

        if (stopToken.stop_requested())
        {
            return PlottingTaskStatus::Cancelled;
        }

        // init the model + state

        InitializeModel(*model);

        if (stopToken.stop_requested())
        {
            return PlottingTaskStatus::Cancelled;
        }

        SimTK::State& state = InitializeState(*model);

        if (stopToken.stop_requested())
        {
            return PlottingTaskStatus::Cancelled;
        }

        auto const* maybeMuscle = FindComponent<OpenSim::Muscle>(*model, params.getMusclePath());
        if (!maybeMuscle)
        {
            shared.setErrorMessage(params.getMusclePath().toString() + ": cannot find a muscle with this name");
            return PlottingTaskStatus::Error;
        }
        OpenSim::Muscle const& muscle = *maybeMuscle;

        OpenSim::Coordinate const* maybeCoord = FindComponentMut<OpenSim::Coordinate>(*model, params.getCoordinatePath());
        if (!maybeCoord)
        {
            shared.setErrorMessage(params.getCoordinatePath().toString() + ": cannot find a coordinate with this name");
            return PlottingTaskStatus::Error;
        }
        OpenSim::Coordinate const& coord = *maybeCoord;

        int const numDataPoints = params.getNumRequestedDataPoints();
        double const firstXValue = GetFirstXValue(params, coord);
        double const lastXValue = GetLastXValue(params, coord);
        double const stepBetweenXValues = GetStepBetweenXValues(params, coord);

        if (firstXValue > lastXValue)
        {
            // this invariant is necessary because other algorithms assume X increases over
            // the datapoint collection (e.g. for optimized binary searches, std::lower_bound etc.)

            shared.setErrorMessage(params.getCoordinatePath().toString() + ": cannot plot a coordinate with reversed min/max");
            return PlottingTaskStatus::Error;
        }

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
            return PlottingTaskStatus::Cancelled;
        }

        for (int i = 0; i < numDataPoints; ++i)
        {
            if (stopToken.stop_requested())
            {
                return PlottingTaskStatus::Cancelled;
            }

            double xVal = firstXValue + (i * stepBetweenXValues);
            coord.setValue(state, xVal);

            model->equilibrateMuscles(state);

            if (stopToken.stop_requested())
            {
                return PlottingTaskStatus::Cancelled;
            }

            model->realizeReport(state);

            if (stopToken.stop_requested())
            {
                return PlottingTaskStatus::Cancelled;
            }

            float const xDisplayVal = ConvertCoordValueToDisplayValue(coord, xVal);
            auto const yVal = static_cast<float>(params.getPlottedOutput()(state, muscle, coord));

            callback(PlotDataPoint{xDisplayVal, yVal});
        }

        return PlottingTaskStatus::Finished;
    }

    // top-level "main" function that the Plotting task worker thread executes
    //
    // catches exceptions and propagates them to the task
    int ComputePlotPointsMain(cpp20::stop_token const& stopToken, PlottingTaskInputs inputs)
    {
        try
        {
            inputs.shared->setStatus(PlottingTaskStatus::Running);
            PlottingTaskStatus status = ComputePlotPointsUnguarded(stopToken, inputs);
            inputs.shared->setStatus(status);
            return 0;
        }
        catch (std::exception const& ex)
        {
            log_error("ComputePlotPointsMain: exception thrown while computing a plot: %s", ex.what());
            inputs.shared->setErrorMessage(ex.what());
            inputs.shared->setStatus(PlottingTaskStatus::Error);
            return -1;
        }
    }

    // a "live" plotting task that is being executed on a background thread
    //
    // the plotting task emits each plotpoint through the callback without any mutexes, so
    // it's up to the user of this class to ensure each emitted point is handled correctly
    class PlottingTask final {
    public:
        PlottingTask(
            PlotParameters const& params,
            std::shared_ptr<PlotDataPointConsumer> consumer_) :

            m_WorkerThread{ComputePlotPointsMain, PlottingTaskInputs{m_Shared, params, std::move(consumer_)}}
        {
        }

        PlottingTaskStatus getStatus() const
        {
            return m_Shared->getStatus();
        }

        std::optional<std::string> getErrorString() const
        {
            return m_Shared->getErrorMessage();
        }

    private:
        std::shared_ptr<PlottingTaskThreadsafeSharedData> m_Shared = std::make_shared<PlottingTaskThreadsafeSharedData>();
        cpp20::jthread m_WorkerThread;
    };

    // a data plot (line), potentially computed from a background thread, or loaded via a
    // file
    class Plot final : public PlotDataPointConsumer {
    public:

        // assumed to be a plot that is probably being computed elsewhere
        explicit Plot(PlotParameters const& parameters) :
            m_Parameters{parameters},
            m_Name{parameters.getCommit().getCommitMessage()}
        {
            m_DataPoints.lock()->reserve(m_Parameters->getNumRequestedDataPoints());
        }

        // assumed to be a plot that was loaded from disk
        Plot(std::string name, std::vector<PlotDataPoint> data) :
            m_Parameters{std::nullopt},
            m_Name{std::move(name)},
            m_DataPoints{std::move(data)}
        {
        }

        CStringView getName() const
        {
            return m_Name;
        }

        PlotParameters const* tryGetParameters() const
        {
            return m_Parameters.has_value() ? &m_Parameters.value() : nullptr;
        }

        std::vector<PlotDataPoint> copyDataPoints() const
        {
            auto lock = m_DataPoints.lock();
            return *lock;
        }

        SynchronizedValueGuard<std::vector<PlotDataPoint> const> lockDataPoints() const
        {
            return m_DataPoints.lock();
        }

        SynchronizedValueGuard<std::vector<PlotDataPoint>> lockDataPoints()
        {
            return m_DataPoints.lock();
        }

        void operator()(PlotDataPoint p) final
        {
            {
                auto lock = m_DataPoints.lock();
                lock->push_back(p);
            }

            // something happened on a background thread, the UI thread should probably redraw
            App::upd().requestRedraw();
        }

        bool getIsLocked() const
        {
            return m_IsLocked;
        }

        void setIsLocked(bool v)
        {
            m_IsLocked = v;
        }

        void setCommit(ModelStateCommit const& commit)
        {
            if (m_Parameters)
            {
                m_Parameters->setCommit(commit);
                m_Name = m_Parameters->getCommit().getCommitMessage();
            }
        }

        bool isExternallyProvided() const
        {
            return !m_Parameters.has_value();
        }

        bool isLocked() const
        {
            return m_IsLocked;
        }

    private:
        std::optional<PlotParameters> m_Parameters;
        std::string m_Name;
        bool m_IsLocked = false;
        SynchronizedValue<std::vector<PlotDataPoint>> m_DataPoints;
    };
}

// helpers
//
// used for various UI tasks (e.g. finding the closest point for "snapping" and so on)
namespace
{
    float lerp(float a, float b, float t)
    {
        return (1.0f - t) * a + t * b;
    }

    std::optional<float> ComputeLERPedY(Plot const& p, float x)
    {
        auto lock = p.lockDataPoints();
        std::span<PlotDataPoint const> const points = *lock;

        if (points.empty())
        {
            // there are no data points
            return std::nullopt;
        }

        auto const it = std::lower_bound(points.begin(), points.end(), PlotDataPoint{x, 0.0f});

        if (it == points.end())
        {
            // X is out of bounds
            return std::nullopt;
        }

        if (it == points.begin())
        {
            // X if off the left-hand side
            return points.front().y;
        }

        // else: the iterator is pointing somewhere in the middle of the data
        //       and we need to potentially LERP between two points
        size_t const aboveIdx = std::distance(points.begin(), it);
        size_t const belowIdx = aboveIdx - 1;
        PlotDataPoint const below = points[belowIdx];
        PlotDataPoint const above = points[aboveIdx];

        float const t = (x - below.x) / (above.x - below.x); // [0..1]

        return lerp(below.y, above.y, t);
    }

    std::optional<PlotDataPoint> FindNearestPoint(Plot const& p, float x)
    {
        auto lock = p.lockDataPoints();
        std::span<PlotDataPoint const> points = *lock;

        if (points.empty())
        {
            // there are no data points
            return std::nullopt;
        }

        auto const it = std::lower_bound(points.begin(), points.end(), PlotDataPoint{x, 0.0f});

        if (it == points.begin())
        {
            // closest is the leftmost point
            return points.front();
        }

        if (it == points.end())
        {
            // closest is the rightmost point
            return points.back();
        }

        // else: the iterator is pointing to the element above the X location and we
        //       need to figure out if that's closer than the element below the X
        //       location
        size_t const aboveIdx = std::distance(points.begin(), it);
        size_t const belowIdx = aboveIdx - 1;
        PlotDataPoint const below = points[belowIdx];
        PlotDataPoint const above = points[aboveIdx];

        float const belowDistance = std::abs(below.x - x);
        float const aboveDistance = std::abs(above.x - x);

        size_t const closestIdx =  aboveDistance < belowDistance  ? aboveIdx : belowIdx;

        return points[closestIdx];
    }

    bool IsXInRange(Plot const& p, float x)
    {
        auto lock = p.lockDataPoints();
        std::span<PlotDataPoint const> const points = *lock;

        if (points.size() <= 1)
        {
            return false;
        }

        return points.front().x <= x && x <= points.back().x;
    }

    void PlotLine(CStringView lineName, Plot const& p)
    {
        auto lock = p.lockDataPoints();
        std::span<PlotDataPoint const> points = *lock;


        float const* xPtr = nullptr;
        float const* yPtr = nullptr;
        if (!points.empty())
        {
            xPtr = &points.front().x;
            yPtr = &points.front().y;
        }

        ImPlot::PlotLine(
            lineName.c_str(),
            xPtr,
            yPtr,
            static_cast<int>(points.size()),
            0,
            0,
            sizeof(PlotDataPoint)
        );
    }

    std::string IthPlotLineName(Plot const& p, size_t i)
    {
        std::stringstream ss;

        ss << i << ") " << p.getName();
        if (p.getIsLocked())
        {
            ss << " " ICON_FA_LOCK;
        }
        return std::move(ss).str();
    }

    std::ostream& WriteYAxisName(PlotParameters const& params, std::ostream& o)
    {
        return o << params.getPlottedOutput().getName();
    }

    std::ostream& WriteXAxisName(PlotParameters const& params, std::ostream& o)
    {
        return o << params.getCoordinatePath().getComponentName();
    }

    std::string ComputePlotTitle(PlotParameters const& params)
    {
        std::stringstream ss;
        ss << params.getMusclePath().getComponentName() << ' ';
        WriteYAxisName(params, ss);
        ss << " vs ";
        WriteXAxisName(params, ss);
        return std::move(ss).str();
    }

    std::string ComputePlotYAxisTitle(PlotParameters const& params)
    {
        std::stringstream ss;
        WriteYAxisName(params, ss);
        ss << " [" << params.getPlottedOutput().getUnits() << ']';
        return std::move(ss).str();
    }

    std::string ComputePlotXAxisTitle(PlotParameters const& params, OpenSim::Coordinate const& coord)
    {
        std::stringstream ss;
        WriteXAxisName(params, ss);
        ss << " value [" << GetCoordDisplayValueUnitsString(coord) << ']';
        return std::move(ss).str();
    }

    std::vector<Plot> TryLoadSVCFileAsPlots(std::filesystem::path const& inputPath)
    {
        // create input reader
        std::ifstream inputFileStream{inputPath};
        inputFileStream.exceptions(std::ios_base::badbit);
        if (!inputFileStream)
        {
            return {};  // error opening path
        }

        // try to read header row
        std::vector<std::string> headers;
        if (!ReadCSVRowIntoVector(inputFileStream, headers))
        {
            return {};  // no CSV data (headers) in top row
        }

        // map each CSV row from [$independent, ...$dependent] -> [($independent, $dependent[i])]
        std::vector<std::vector<PlotDataPoint>> columnsAsPlots;
        for (std::vector<std::string> row; ReadCSVRowIntoVector(inputFileStream, row);)
        {
            if (row.size() < 2)
            {
                continue;  // skip: row does not contain enough columns
            }

            std::optional<float> const independentVar = FromCharsStripWhitespace(row.front());
            if (!independentVar)
            {
                continue;  // skip: row does not contain a valid independent variable
            }

            // parse remaining columns as dependent variables
            for (size_t dependentCol = 1; dependentCol < row.size(); ++dependentCol)
            {
                std::string const& dependentVarStr = row[dependentCol];
                std::optional<float> const dependentVar = FromCharsStripWhitespace(dependentVarStr);
                if (!dependentVar)
                {
                    continue;  // skip: column cannot be parsed as a number
                }

                // else: append column as ($independent, $dependent[col]) to the plots vector
                columnsAsPlots.resize(std::max(columnsAsPlots.size(), dependentCol));
                columnsAsPlots[dependentCol-1].push_back({*independentVar, *dependentVar});
            }
        }

        if (columnsAsPlots.empty())
        {
            // 0 series: return no plots
            return {};
        }
        else if (columnsAsPlots.size() == 1)
        {
            // 1 series: return one plot with one series named `$filename`
            return
            {
                Plot{inputPath.filename().string(), std::move(columnsAsPlots.front())}
            };
        }
        else
        {
            // >1 series: return each plot named `$filename ($header)` (or a number)

            std::vector<Plot> rv;
            rv.reserve(columnsAsPlots.size());
            for (size_t i = 0; i < columnsAsPlots.size(); ++i)
            {
                std::stringstream ss;
                ss << inputPath.filename();
                ss << " (";
                if (i < headers.size())
                {
                    ss << headers[i];
                }
                else
                {
                    ss << i;
                }
                ss << ')';

                rv.emplace_back(std::move(ss).str(), std::move(columnsAsPlots[i]));
            }

            return rv;
        }
    }

    void TrySavePlotToCSV(OpenSim::Coordinate const& coord, PlotParameters const& params, Plot const& plot, std::filesystem::path const& outPath)
    {
        std::ofstream fileOutputStream{outPath};
        if (!fileOutputStream)
        {
            return;  // error opening outfile
        }

        // write header
        WriteCSVRow(
            fileOutputStream,
            std::to_array({ ComputePlotXAxisTitle(params, coord), ComputePlotYAxisTitle(params) })
        );

        // write data rows
        auto lock = plot.lockDataPoints();
        for (PlotDataPoint const& p : *lock)
        {
            WriteCSVRow(
                fileOutputStream,
                std::to_array({ std::to_string(p.x), std::to_string(p.y) })
            );
        }
    }

    void ActionPromptUserToSavePlotToCSV(OpenSim::Coordinate const& coord, PlotParameters const& params, Plot const& plot)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (maybeCSVPath)
        {
            TrySavePlotToCSV(coord, params, plot, *maybeCSVPath);
        }
    }

    // holds a collection of plotlines that are to-be-drawn on the plot
    class PlotLines final {
    public:
        explicit PlotLines(PlotParameters const& params) :
            m_ActivePlot{std::make_shared<Plot>(params)},
            m_PlottingTask{params, m_ActivePlot}
        {
        }

        void onBeforeDrawing(UndoableModelStatePair const&, PlotParameters const& desiredParams)
        {
            // perform any datastructure invariant checks etc.

            checkForParameterChangesAndStartPlotting(desiredParams);
            handleUserEnactedDeletions();
            ensurePreviousCurvesDoesNotExceedMax();
        }

        void clearUnlockedPlots()
        {
            std::erase_if(m_PreviousPlots, [](std::shared_ptr<Plot> const& p) { return !p->getIsLocked(); });
        }

        PlottingTaskStatus getPlottingTaskStatus() const
        {
            return m_PlottingTask.getStatus();
        }

        std::optional<std::string> tryGetPlottingTaskErrorMessage() const
        {
            return m_PlottingTask.getErrorString();
        }

        Plot const& getActivePlot() const
        {
            return *m_ActivePlot;
        }

        size_t getNumOtherPlots() const
        {
            return m_PreviousPlots.size();
        }

        Plot const& getOtherPlot(size_t i) const
        {
            return *m_PreviousPlots.at(i);
        }

        void tagOtherPlotForDeletion(size_t i)
        {
            m_PlotTaggedForDeletion = static_cast<ptrdiff_t>(i);
        }

        void setOtherPlotLocked(size_t i, bool v)
        {
            m_PreviousPlots.at(i)->setIsLocked(v);
        }

        void setActivePlotLocked(bool v)
        {
            m_ActivePlot->setIsLocked(v);
        }

        int getMaxHistoryEntries() const
        {
            return m_MaxHistoryEntries;
        }

        void setMaxHistoryEntries(int i)
        {
            if (i < 0)
            {
                return;
            }
            m_MaxHistoryEntries = i;
        }

        void setActivePlotCommit(ModelStateCommit const& commit)
        {
            m_ActivePlot->setCommit(commit);
        }

        void pushPlotAsPrevious(Plot p)
        {
            m_PreviousPlots.push_back(std::make_shared<Plot>(std::move(p)));

            ensurePreviousCurvesDoesNotExceedMax();
        }

        void revertToPreviousPlot(UndoableModelStatePair& model, size_t i)
        {
            // fetch the to-be-reverted-to curve
            std::shared_ptr<Plot> ptr = m_PreviousPlots.at(i);

            PlotParameters const* maybeParams = ptr->tryGetParameters();

            // try to revert the current model to use the plot's commit
            if (maybeParams && model.tryCheckout(maybeParams->getCommit()))
            {
                // it checked out successfully, so update this plotting widget
                // accordingly

                /// remove it from the history list (it'll become active)
                m_PreviousPlots.erase(m_PreviousPlots.begin() + i);

                // swap it with the active curve
                std::swap(ptr, m_ActivePlot);

                // push the active curve into the history
                m_PreviousPlots.push_back(ptr);

                // and GC the history
                ensurePreviousCurvesDoesNotExceedMax();
            }
        }

    private:
        void clearComputedPlots()
        {
            std::erase_if(m_PreviousPlots, [](auto const& ptr) { return ptr->tryGetParameters() != nullptr; });
        }

        void checkForParameterChangesAndStartPlotting(PlotParameters const& desiredParams)
        {
            // additions/changes
            //
            // if the current plot doesn't match the latest requested params, kick off
            // a new plotting task

            PlotParameters const* maybeParams = m_ActivePlot->tryGetParameters();

            if (!maybeParams || (*maybeParams) != desiredParams)
            {
                // (edge-case): if the user selection fundamentally changes what's being plotted
                // then previous plots should be cleared
                bool const clearPrevious =
                    maybeParams != nullptr &&
                    (maybeParams->getPlottedOutput() != desiredParams.getPlottedOutput() ||
                     maybeParams->getCoordinatePath() != desiredParams.getCoordinatePath() ||
                     maybeParams->getMusclePath() != desiredParams.getMusclePath());

                // create new active plot and swap the old active plot into the previous plots
                {
                    std::shared_ptr<Plot> p = std::make_shared<Plot>(desiredParams);
                    std::swap(p, m_ActivePlot);
                    m_PreviousPlots.push_back(p);
                }

                if (clearPrevious)
                {
                    clearComputedPlots();
                }

                // kick off a new plotting task
                m_PlottingTask = PlottingTask{desiredParams, m_ActivePlot};
            }
        }

        void handleUserEnactedDeletions()
        {
            // deletions
            //
            // handle any user-requested deletions by removing the curve from the collection
            if (0 <= m_PlotTaggedForDeletion && m_PlotTaggedForDeletion < std::ssize(m_PreviousPlots))
            {
                m_PreviousPlots.erase(m_PreviousPlots.begin() + m_PlotTaggedForDeletion);
                m_PlotTaggedForDeletion = -1;
            }
        }

        void ensurePreviousCurvesDoesNotExceedMax()
        {
            // algorithm:
            //
            // - go backwards through the history list and count up *unlocked* elements until
            //   either the beginning is hit (there are too few - nothing to GC) or the maximum
            //   number of history entries is hit (`hmax`)s
            //
            // - go forwards through the history list, deleting any *unlocked* elements until
            //   `hmax` is hit
            //
            // - you now have a list containing 0..`hmax` unlocked elements, plus locked elements,
            //   where the unlocked elements are the most recently used

            auto isFirstDeleteablePlot = [nth = 1, max = this->m_MaxHistoryEntries](std::shared_ptr<Plot> const& p) mutable
            {
                if (p->getIsLocked())
                {
                    return false;
                }
                return nth++ > max;
            };

            auto const backwardIt = std::find_if(m_PreviousPlots.rbegin(), m_PreviousPlots.rend(), isFirstDeleteablePlot);
            auto const forwardIt = backwardIt.base();
            ptrdiff_t const idxOfDeleteableEnd = std::distance(m_PreviousPlots.begin(), forwardIt);

            auto shouldDelete = [i = static_cast<ptrdiff_t>(0), idxOfDeleteableEnd](std::shared_ptr<Plot> const& p) mutable
            {
                return i++ < idxOfDeleteableEnd && !p->getIsLocked();
            };

            std::erase_if(m_PreviousPlots, shouldDelete);
        }

        std::shared_ptr<Plot> m_ActivePlot;
        PlottingTask m_PlottingTask;
        std::vector<std::shared_ptr<Plot>> m_PreviousPlots;
        ptrdiff_t m_PlotTaggedForDeletion = -1;
        int m_MaxHistoryEntries = 6;
    };

    struct PlotLineCounts final {
        size_t external;
        size_t locked;
        size_t total;
    };

    PlotLineCounts CountOtherPlotTypes(PlotLines const& lines)
    {
        PlotLineCounts rv{};
        for (size_t i = 0; i < lines.getNumOtherPlots(); ++i)
        {
            Plot const& p = lines.getOtherPlot(i);

            if (p.isExternallyProvided())
            {
                ++rv.external;
            }
            else if (p.isLocked())
            {
                ++rv.locked;
            }
            ++rv.total;
        }
        return rv;
    }

    // tries to hittest the mouse's X position in plot-space
    std::optional<float> TryGetMouseXPositionInPlot(PlotLines const& lines, bool snapToNearest)
    {
        // figure out mouse hover position
        bool const isHovered = ImPlot::IsPlotHovered();
        float mouseX = static_cast<float>(ImPlot::GetPlotMousePos().x);

        // handle snapping the mouse's X position
        if (isHovered && snapToNearest)
        {
            auto maybeNearest = FindNearestPoint(lines.getActivePlot(), mouseX);

            if (IsXInRange(lines.getActivePlot(), mouseX) && maybeNearest)
            {
                mouseX = maybeNearest->x;
            }
        }

        return isHovered ? mouseX : std::optional<float>{};
    }

    // returns a vector of all the headers a CSV file will contain if plotting the given lines
    std::vector<std::string> GetAllCSVHeaders(
        OpenSim::Coordinate const& coord,
        PlotParameters const& params,
        PlotLines const& lines)
    {
        std::vector<std::string> headers;
        headers.reserve(1 + lines.getNumOtherPlots() + 1);

        headers.push_back(ComputePlotXAxisTitle(params, coord));
        for (size_t i = 0, len = lines.getNumOtherPlots(); i < len; ++i)
        {
            headers.emplace_back(to_string(lines.getOtherPlot(i).getName()));
        }
        headers.emplace_back(to_string(lines.getActivePlot().getName()));
        return headers;
    }

    // algorithm helper class: wraps a data + cursor together
    class LineCursor final {
    public:
        explicit LineCursor(Plot const& plot) :
            m_Data{plot.copyDataPoints()}
        {
        }

        std::optional<float> peekX() const
        {
            return m_Cursor != m_Data.end() ? m_Cursor->x : std::optional<float>{};
        }

        std::optional<PlotDataPoint> peek() const
        {
            return m_Cursor != m_Data.end() ? *m_Cursor : std::optional<PlotDataPoint>{};
        }

        LineCursor& operator++()
        {
            OSC_ASSERT(m_Cursor != m_Data.end());
            ++m_Cursor;
            return *this;
        }

    private:
        std::vector<PlotDataPoint> m_Data;
        std::vector<PlotDataPoint>::const_iterator m_Cursor = m_Data.begin();
    };

    bool LessThanAssumingEmptyHighest(std::optional<float> const& a, std::optional<float> const& b)
    {
        // this is defined differently from the C++ standard, which makes the
        // empty optional the "minimum" value, logically
        //
        // see cppreference's definition for `std::optional<T>::operator<`

        if (!a)
        {
            return false;
        }
        if (!b)
        {
            return true;
        }
        return *a < *b;
    }

    // returns true if `a` has a lower X value than `b` - assumes an empty X value is the "highest"
    bool HasLowerX(LineCursor const& a, LineCursor const& b)
    {
        return LessThanAssumingEmptyHighest(a.peekX(), b.peekX());
    }

    // returns data-owning cursors to all lines in the given plotlines
    std::vector<LineCursor> GetCursorsToAllPlotLines(PlotLines const& lines)
    {
        std::vector<LineCursor> cursors;
        cursors.reserve(lines.getNumOtherPlots() + 1);
        for (size_t i = 0, len = lines.getNumOtherPlots(); i < len; ++i)
        {
            cursors.emplace_back(lines.getOtherPlot(i));
        }
        cursors.emplace_back(lines.getActivePlot());
        return cursors;
    }

    // returns the smallest X value accross all given plot lines - if an X value exists
    std::optional<float> CalcSmallestX(std::span<LineCursor const> cursors)
    {
        auto it = std::min_element(cursors.begin(), cursors.end(), HasLowerX);
        return it != cursors.end() ? it->peekX() : std::optional<float>{};
    }

    // try to save the given collection of plotlines to an on-disk CSV file
    //
    // the resulting CSV may be sparsely populated, because each line may have a different
    // number of, and location of, values
    void TrySavePlotLinesToCSV(
        OpenSim::Coordinate const& coord,
        PlotParameters const& params,
        PlotLines const& lines,
        std::filesystem::path const& outPath)
    {
        std::ofstream outputFileStream{outPath};
        if (!outputFileStream)
        {
            return;  // error opening outfile
        }

        // write header
        WriteCSVRow(outputFileStream, GetAllCSVHeaders(coord, params, lines));

        // get incrementable cursors to all curves in the plot
        std::vector<LineCursor> cursors = GetCursorsToAllPlotLines(lines);

        // calculate smallest X value among all curves (if applicable - they may all be empty)
        std::optional<float> maybeX = CalcSmallestX(cursors);

        // keep an eye out for the *next* lowest X value as we iterate
        std::optional<float> maybeNextX;

        while (maybeX)
        {
            std::vector<std::string> cols;
            cols.reserve(1 + cursors.size());

            // emit (potentially deduped) X
            cols.push_back(std::to_string(*maybeX));

            // emit all columns that match up with X
            for (LineCursor& cursor : cursors)
            {
                std::optional<PlotDataPoint> data = cursor.peek();

                if (data && IsLessThanOrEffectivelyEqual(data->x, *maybeX))
                {
                    cols.push_back(std::to_string(data->y));
                    ++cursor;
                    data = cursor.peek();  // to test the next X
                }
                else
                {
                    cols.emplace_back();  // blank cell
                }

                std::optional<float> maybeDataX = data ? std::optional<float>{data->x} : std::optional<float>{};
                if (LessThanAssumingEmptyHighest(maybeDataX, maybeNextX))
                {
                    maybeNextX = maybeDataX;
                }
            }

            WriteCSVRow(outputFileStream, cols);

            maybeX = maybeNextX;
            maybeNextX = std::nullopt;
        }
    }

    // a UI action in which the user in prompted for a CSV file that they would like to overlay
    // over the current plot
    void ActionPromptUserForCSVOverlayFile(PlotLines& lines)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFile("csv");

        if (maybeCSVPath)
        {
            for (Plot& plot : TryLoadSVCFileAsPlots(*maybeCSVPath))
            {
                plot.setIsLocked(true);
                lines.pushPlotAsPrevious(std::move(plot));
            }
        }
    }

    // a UI action in which the user is prompted to save a CSV file to the filesystem and then, if
    // the user selects a filesystem location, writes a sparse CSV file containing all plotlines to
    // that location
    void ActionPromptUserToSavePlotLinesToCSV(OpenSim::Coordinate const& coord, PlotParameters const& params, PlotLines const& lines)
    {
        std::optional<std::filesystem::path> const maybeCSVPath =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");

        if (maybeCSVPath)
        {
            TrySavePlotLinesToCSV(coord, params, lines, *maybeCSVPath);
        }
    }
}

// UI state
//
// top-level state API - all "states" of the widget share this info and implement the
// relevant state API
namespace
{
    // data that is shared between all states of the widget
    class SharedStateData final {
    public:
        SharedStateData(
            IEditorAPI* editorAPI,
            std::shared_ptr<UndoableModelStatePair> uim) :

            m_EditorAPI{editorAPI},
            m_Model{std::move(uim)}
        {}

        SharedStateData(
            IEditorAPI* editorAPI,
            std::shared_ptr<UndoableModelStatePair> uim,
            OpenSim::ComponentPath const& coordPath,
            OpenSim::ComponentPath const& musclePath) :

            m_EditorAPI{editorAPI},
            m_Model{std::move(uim)},
            m_PlotParams{m_Model->getLatestCommit(), coordPath, musclePath, GetDefaultMuscleOutput(), c_DefaultNumPlotPoints}
        {}

        PlotParameters const& getPlotParams() const { return m_PlotParams; }
        PlotParameters& updPlotParams() { return m_PlotParams; }

        UndoableModelStatePair const& getModel() const { return *m_Model; }
        UndoableModelStatePair& updModel() { return *m_Model; }

        IEditorAPI& updEditorAPI() { return *m_EditorAPI; }

        std::span<PlottableOutput const> availableOutputs() const { return m_AvailableMuscleOutputs; }

        PlottableOutput const& getPlottedOutput() const { return getPlotParams().getPlottedOutput(); }
        void setPlottedOutput(PlottableOutput const& newOutput) { updPlotParams().setPlottedOutput(newOutput); }

        int getNumRequestedDatapoints() const { return getPlotParams().getNumRequestedDataPoints(); }
        void setNumRequestedDataPoints(int v) { updPlotParams().setNumRequestedDataPoints(v); }

    private:
        IEditorAPI* m_EditorAPI;
        std::shared_ptr<UndoableModelStatePair> m_Model;
        PlotParameters m_PlotParams{
            m_Model->getLatestCommit(),
            OpenSim::ComponentPath{},
            OpenSim::ComponentPath{},
            GetDefaultMuscleOutput(),
            180
        };
        std::vector<PlottableOutput> m_AvailableMuscleOutputs = GenerateMuscleOutputs();
    };

    // base class for a single widget state
    class MusclePlotState {
    protected:
        explicit MusclePlotState(SharedStateData& shared_) : m_Shared{&shared_} {}
        MusclePlotState(MusclePlotState const&) = default;
        MusclePlotState(MusclePlotState&&) noexcept = default;
        MusclePlotState& operator=(MusclePlotState const&) = default;
        MusclePlotState& operator=(MusclePlotState&&) noexcept = default;
    public:
        virtual ~MusclePlotState() noexcept = default;

        std::unique_ptr<MusclePlotState> onDraw()
        {
            return implOnDraw();
        }

    protected:
        SharedStateData const& getShared() const { return *m_Shared; }
        SharedStateData& updShared() { return *m_Shared; }

    private:
        virtual std::unique_ptr<MusclePlotState> implOnDraw() = 0;

        SharedStateData* m_Shared;
    };
}

// "showing plot" state
//
// this is the biggest, most important, state of the widget: it is what's used when
// the widget is showing a muscle curve to the user
namespace
{
    // state in which the plot is being shown to the user
    //
    // this is the most important state of the state machine
    class ShowingPlotState final : public MusclePlotState {
    public:
        explicit ShowingPlotState(SharedStateData& shared_) :
            MusclePlotState{shared_},
            m_Lines{shared_.getPlotParams()}
        {}

    private:
        std::unique_ptr<MusclePlotState> implOnDraw() final
        {
            onBeforeDrawing();  // perform pre-draw cleanups/updates etc.

            if (m_Lines.getPlottingTaskStatus() == PlottingTaskStatus::Error) {
                if (auto maybeErrorString = m_Lines.tryGetPlottingTaskErrorMessage()) {
                    ImGui::Text("error: cannot show plot: %s", maybeErrorString->c_str());
                }
                return nullptr;
            }

            PlotParameters const& latestParams = getShared().getPlotParams();
            auto modelGuard = latestParams.getCommit().getModel();

            auto const* maybeCoord = FindComponent<OpenSim::Coordinate>(*modelGuard, latestParams.getCoordinatePath());
            if (!maybeCoord) {
                ImGui::Text("(no coordinate named %s in model)", latestParams.getCoordinatePath().toString().c_str());
                return nullptr;
            }
            OpenSim::Coordinate const& coord = *maybeCoord;

            std::string const plotTitle = ComputePlotTitle(latestParams);

            drawPlotTitle(coord, plotTitle);  // draw a custom title bar
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, {0.025f, 0.05f});
            if (ImPlot::BeginPlot(plotTitle.c_str(), ImGui::GetContentRegionAvail(), m_PlotFlags)) {
                PlotParameters const& plotParams = getShared().getPlotParams();

                ImPlot::SetupLegend(
                    m_LegendLocation,
                    m_LegendFlags
                );
                ImPlot::SetupAxes(
                    ComputePlotXAxisTitle(latestParams, coord).c_str(),
                    ComputePlotYAxisTitle(latestParams).c_str(),
                    ImPlotAxisFlags_Lock,
                    ImPlotAxisFlags_AutoFit
                );
                ImPlot::SetupAxisLimits(
                    ImAxis_X1,
                    ConvertCoordValueToDisplayValue(coord, GetFirstXValue(plotParams, coord)),
                    ConvertCoordValueToDisplayValue(coord, GetLastXValue(plotParams, coord))
                );
                ImPlot::SetupFinish();

                std::optional<float> maybeMouseX = TryGetMouseXPositionInPlot(m_Lines, m_SnapCursor);
                drawPlotLines(coord);
                drawOverlays(coord, maybeMouseX);
                handleMouseEvents(coord, maybeMouseX);
                if (!m_LegendPopupIsOpen)
                {
                    tryDrawGeneralPlotPopup(coord, plotTitle);
                }

                ImPlot::EndPlot();
            }

            ImPlot::PopStyleVar();

            return nullptr;
        }


        // called at the start of each `draw` call - it GCs datastructures etc.
        void onBeforeDrawing()
        {
            // ensure the legend test is reset (it's checked every frame)
            m_LegendPopupIsOpen = false;

            // ensure latest requested params reflects the latest version of the model
            updShared().updPlotParams().setCommit(getShared().getModel().getLatestCommit());

            // ensure plot lines are valid, given the current model + desired params
            m_Lines.onBeforeDrawing(getShared().getModel(), getShared().getPlotParams());
        }

        void drawPlotTitle(
            OpenSim::Coordinate const& coord,
            std::string const& plotTitle)
        {
            // the plot title should contain combo boxes that users can use to change plot
            // parameters visually (#397)

            std::string muscleName = Ellipsis(getShared().getPlotParams().getMusclePath().getComponentName(), 15);
            float muscleNameWidth = ImGui::CalcTextSize(muscleName.c_str()).x + 2.0f*ImGui::GetStyle().FramePadding.x;
            std::string outputName = Ellipsis(getShared().getPlotParams().getPlottedOutput().getName(), 15);
            float outputNameWidth = ImGui::CalcTextSize(outputName.c_str()).x + 2.0f*ImGui::GetStyle().FramePadding.x;
            std::string coordName = Ellipsis(getShared().getPlotParams().getCoordinatePath().getComponentName(), 15);
            float coordNameWidth = ImGui::CalcTextSize(coordName.c_str()).x + 2.0f*ImGui::GetStyle().FramePadding.x;

            float totalWidth =
                muscleNameWidth +
                ImGui::CalcTextSize("'s").x +
                ImGui::GetStyle().ItemSpacing.x +
                outputNameWidth +
                ImGui::GetStyle().ItemSpacing.x +
                ImGui::CalcTextSize("vs.").x +
                ImGui::GetStyle().ItemSpacing.x +
                coordNameWidth +
                ImGui::GetStyle().ItemSpacing.x +
                ImGui::GetStyle().FramePadding.x +
                ImGui::CalcTextSize(ICON_FA_BARS " Options").x +
                ImGui::GetStyle().FramePadding.x;

            float cursorStart = 0.5f*(ImGui::GetContentRegionAvail().x - totalWidth);
            ImGui::SetCursorPosX(cursorStart);

            ImGui::SetNextItemWidth(muscleNameWidth);
            if (ImGui::BeginCombo("##musclename", muscleName.c_str(), ImGuiComboFlags_NoArrowButton))
            {
                auto const* current = FindComponent<OpenSim::Muscle>(getShared().getModel().getModel(), getShared().getPlotParams().getMusclePath());
                for (OpenSim::Muscle const& musc : getShared().getModel().getModel().getComponentList<OpenSim::Muscle>())
                {
                    bool selected = &musc == current;
                    if (ImGui::Selectable(musc.getName().c_str(), &selected))
                    {
                        updShared().updPlotParams().setMusclePath(musc.getAbsolutePath());
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x);
            ImGui::Text("'s");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(outputNameWidth);
            if (ImGui::BeginCombo("##outputname", outputName.c_str(), ImGuiComboFlags_NoArrowButton))
            {
                PlottableOutput current = getShared().getPlotParams().getPlottedOutput();
                for (PlottableOutput const& output : getShared().availableOutputs())
                {
                    bool selected = output == current;
                    if (ImGui::Selectable(output.getName().c_str(), &selected))
                    {
                        updShared().updPlotParams().setPlottedOutput(output);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::TextUnformatted("vs.");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(coordNameWidth);
            if (ImGui::BeginCombo("##coordname", coordName.c_str(), ImGuiComboFlags_NoArrowButton))
            {
                auto const* current = FindComponent<OpenSim::Coordinate>(getShared().getModel().getModel(), getShared().getPlotParams().getCoordinatePath());
                for (OpenSim::Coordinate const& c : getShared().getModel().getModel().getComponentList<OpenSim::Coordinate>())
                {
                    bool selected = &c == current;
                    if (ImGui::Selectable(c.getName().c_str(), &selected))
                    {
                        updShared().updPlotParams().setCoordinatePath(GetAbsolutePath(c));
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            // draw little options button that opens the context menu
            //
            // it's easier for users to figure out than having to guess they need to
            // right-click the plot (#399)
            ImGui::Button(ICON_FA_BARS " Options");
            tryDrawGeneralPlotPopup(coord, plotTitle, ImGuiPopupFlags_MouseButtonLeft);
        }

        // draws the actual plot lines in the plot
        void drawPlotLines(OpenSim::Coordinate const& coord)
        {
            // plot not-active plots
            PlotLineCounts const counts = CountOtherPlotTypes(m_Lines);
            size_t externalCounter = 0;
            size_t lockedCounter = 0;
            for (size_t i = 0; i < m_Lines.getNumOtherPlots(); ++i)
            {
                Plot const& plot = m_Lines.getOtherPlot(i);

                Color color = m_ComputedPlotLineBaseColor;

                if (plot.isExternallyProvided())
                {
                    // externally-provided curves should be tinted
                    color *= m_LoadedCurveTint;
                    color.a *= static_cast<float>(++externalCounter) / static_cast<float>(counts.external);
                }
                else if (plot.isLocked())
                {
                    // locked curves should be tinted as such
                    color *= m_LockedCurveTint;
                    color.a *= static_cast<float>(++lockedCounter) / static_cast<float>(counts.locked);
                }
                else
                {
                    // previous curves should fade as they get older
                    color.a *= static_cast<float>(i + 1) / static_cast<float>(counts.total + 1);
                }

                if (m_ShowMarkersOnOtherPlots)
                {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f);
                }

                std::string const lineName = IthPlotLineName(plot, i + 1);

                ImPlot::PushStyleColor(ImPlotCol_Line, Vec4{color});
                PlotLine(lineName, plot);
                ImPlot::PopStyleColor(ImPlotCol_Line);

                if (ImPlot::BeginLegendPopup(lineName.c_str()))
                {
                    m_LegendPopupIsOpen = true;

                    if (ImGui::MenuItem(ICON_FA_TRASH " delete"))
                    {
                        m_Lines.tagOtherPlotForDeletion(i);
                    }
                    if (!plot.getIsLocked() && ImGui::MenuItem(ICON_FA_LOCK " lock"))
                    {
                        m_Lines.setOtherPlotLocked(i, true);
                    }
                    if (plot.getIsLocked() && ImGui::MenuItem(ICON_FA_UNLOCK " unlock"))
                    {
                        m_Lines.setOtherPlotLocked(i, false);
                    }
                    if (plot.tryGetParameters() && ImGui::MenuItem(ICON_FA_UNDO " revert to this"))
                    {
                        m_Lines.revertToPreviousPlot(updShared().updModel(), i);
                    }
                    if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " export to CSV"))
                    {
                        ActionPromptUserToSavePlotToCSV(coord, getShared().getPlotParams(), plot);
                    }
                    ImPlot::EndLegendPopup();
                }
            }

            // then plot the active plot
            {
                Plot const& plot = m_Lines.getActivePlot();
                std::string const lineName = IthPlotLineName(plot, m_Lines.getNumOtherPlots() + 1);

                // locked curves should have a blue tint
                Color color = m_ComputedPlotLineBaseColor;

                if (plot.isExternallyProvided())
                {
                    // externally-provided curves should be tinted
                    color *= m_LoadedCurveTint;
                }
                else if (plot.isLocked())
                {
                    // locked curves should be tinted as such
                    color *= m_LockedCurveTint;
                }

                if (m_ShowMarkersOnActivePlot)
                {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f);
                }

                ImPlot::PushStyleColor(ImPlotCol_Line, Vec4{color});
                PlotLine(lineName, plot);
                ImPlot::PopStyleColor(ImPlotCol_Line);

                if (ImPlot::BeginLegendPopup(lineName.c_str()))
                {
                    m_LegendPopupIsOpen = true;

                    if (!plot.getIsLocked() && ImGui::MenuItem(ICON_FA_LOCK " lock"))
                    {
                        m_Lines.setActivePlotLocked(true);
                    }
                    if (plot.getIsLocked() && ImGui::MenuItem(ICON_FA_UNLOCK " unlock"))
                    {
                        m_Lines.setActivePlotLocked(false);
                    }
                    if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " export to CSV"))
                    {
                        ActionPromptUserToSavePlotToCSV(coord, getShared().getPlotParams(), plot);
                    }
                    ImPlot::EndLegendPopup();
                }
            }
        }

        // draw overlays over the plot lines
        void drawOverlays(
            OpenSim::Coordinate const& coord,
            std::optional<float> maybeMouseX)
        {
            double coordinateXInDegrees = ConvertCoordValueToDisplayValue(coord, coord.getValue(getShared().getModel().getState()));

            // draw vertical drop line where the coordinate's value currently is
            {
                double v = coordinateXInDegrees;

                // CARE: this drag line shouldn't cause ImPlot to re-fit because it will
                // make ImPlot re-fit the plot as the user's mouse moves/drags over it, which
                // looks very very glitchy (#490)
                ImPlot::DragLineX(10, &v, {1.0f, 1.0f, 0.0f, 0.6f}, 1.0f, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoFit);
            }

            // also, draw an X tag on the axes where the coordinate's value currently is
            ImPlot::TagX(coordinateXInDegrees, {1.0f, 1.0f, 1.0f, 1.0f});

            // draw faded vertial drop line where the mouse currently is
            if (maybeMouseX)
            {
                double v = *maybeMouseX;

                // CARE: this drag line shouldn't cause ImPlot to re-fit because it will
                // make ImPlot re-fit the plot as the user's mouse moves/drags over it, which
                // looks very very glitchy (#490)
                ImPlot::DragLineX(11, &v, {1.0f, 1.0f, 0.0f, 0.3f}, 1.0f, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoFit);
            }

            // also, draw a faded X tag on the axes where the mouse currently is (in X)
            if (maybeMouseX)
            {
                ImPlot::TagX(*maybeMouseX, {1.0f, 1.0f, 1.0f, 0.6f});
            }

            // Y values: BEWARE
            //
            // the X values for the droplines/tags above come directly from either the model or
            // mouse: both of which are *continuous* (give or take)
            //
            // the Y values are computed from those continous values by searching through the
            // *discrete* data values of the plot and LERPing them
            {
                // draw current coordinate value as a solid dropline
                {
                    std::optional<float> maybeCoordinateY = ComputeLERPedY(m_Lines.getActivePlot(), static_cast<float>(coordinateXInDegrees));

                    if (maybeCoordinateY) {
                        double v = *maybeCoordinateY;

                        // CARE: this drag line shouldn't cause ImPlot to re-fit because it will
                        // make ImPlot re-fit the plot as the user's mouse moves/drags over it, which
                        // looks very very glitchy (#490)
                        ImPlot::DragLineY(13, &v, {1.0f, 1.0f, 0.0f, 0.6f}, 1.0f, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoFit);

                        ImPlot::Annotation(static_cast<float>(coordinateXInDegrees), *maybeCoordinateY, {1.0f, 1.0f, 1.0f, 1.0f}, {10.0f, 10.0f}, true, "%f", *maybeCoordinateY);
                    }
                }

                // (try to) draw the hovered coordinate value as a faded dropline
                if (maybeMouseX) {
                    std::optional<float> const maybeHoverY = ComputeLERPedY(m_Lines.getActivePlot(), *maybeMouseX);
                    if (maybeHoverY) {
                        double v = *maybeHoverY;

                        // CARE: this drag line shouldn't cause ImPlot to re-fit because it will
                        // make ImPlot re-fit the plot as the user's mouse moves/drags over it, which
                        // looks very very glitchy (#490)
                        ImPlot::DragLineY(14, &v, {1.0f, 1.0f, 0.0f, 0.3f}, 1.0f, ImPlotDragToolFlags_NoInputs | ImPlotDragToolFlags_NoFit);

                        ImPlot::Annotation(*maybeMouseX, *maybeHoverY, {1.0f, 1.0f, 1.0f, 0.6f}, {10.0f, 10.0f}, true, "%f", *maybeHoverY);
                    }
                }
            }
        }

        void handleMouseEvents(
            OpenSim::Coordinate const& coord,
            std::optional<float> maybeMouseX)
        {
            // if the plot is hovered and the user is holding their left-mouse button down,
            // then "scrub" through the output in the model
            //
            // this is handy for users to visually see how the independent variable affects the model
            if (maybeMouseX && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (coord.getDefaultLocked()) {
                    DrawTooltip("scrubbing disabled", "you cannot scrub this plot because the coordinate is locked");
                }
                else {
                    double storedValue = ConvertCoordDisplayValueToStorageValue(coord, *maybeMouseX);
                    ActionSetCoordinateValue(updShared().updModel(), coord, storedValue);
                }
            }

            // when the user stops dragging their left-mouse around, commit the scrubbed-to
            // coordinate to model storage
            if (maybeMouseX && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (coord.getDefaultLocked()) {
                    DrawTooltip("scrubbing disabled", "you cannot scrub this plot because the coordinate is locked");
                }
                else {
                    double storedValue = ConvertCoordDisplayValueToStorageValue(coord, *maybeMouseX);
                    ActionSetCoordinateValueAndSave(updShared().updModel(), coord, storedValue);

                    // trick: we "know" that the last edit to the model was a coordinate edit in this plot's
                    //        independent variable, so we can skip recomputing it
                    ModelStateCommit const& commitAfter = getShared().getModel().getLatestCommit();
                    m_Lines.setActivePlotCommit(commitAfter);
                }
            }
        }

        // try to draw a context menu with helpful options (set num data points, export, etc.)
        void tryDrawGeneralPlotPopup(
            OpenSim::Coordinate const& coord,
            std::string const& plotTitle,
            ImGuiPopupFlags flags = ImGuiPopupFlags_MouseButtonRight)
        {
            if (ImGui::BeginPopupContextItem((plotTitle + "_contextmenu").c_str(), flags)) {
                drawGeneralPlotPopupContent(coord);
                ImGui::EndPopup();
            }
        }

        // draw the content of the general context menu (assume it's already open)
        void drawGeneralPlotPopupContent(OpenSim::Coordinate const& coord)
        {
            drawPlotOutputSelectorCombo();
            drawMaxDatapointsIntInput();
            drawMaxHistoryEntriesIntInput();

            if (ImGui::MenuItem("clear unlocked plots")) {
                m_Lines.clearUnlockedPlots();
            }

            if (ImGui::BeginMenu("legend")) {
                drawLegendContextMenuContent();
                ImGui::EndMenu();
            }

            ImGui::MenuItem("show markers on active plot", nullptr, &m_ShowMarkersOnActivePlot);
            ImGui::MenuItem("show markers on other plots", nullptr, &m_ShowMarkersOnOtherPlots);
            ImGui::MenuItem("snap cursor to datapoints", nullptr, &m_SnapCursor);

            if (ImGui::MenuItem("duplicate plot")) {
                actionDuplicateCurrentPlotIntoNewPanel(coord);
            }

            if (ImGui::MenuItem("import CSV overlay(s)")) {
                ActionPromptUserForCSVOverlayFile(m_Lines);
            }
            DrawTooltipIfItemHovered("import CSV overlay(s)", "Imports the specified CSV file as an overlay over the current plot. This is handy fitting muscle curves against externally-supplied data.\n\nThe provided CSV file must contain a header row and at least two columns of numeric data on each data row. The values in the columns must match this plot's axes.");

            if (ImGui::BeginMenu("export CSV")) {
                drawExportCSVMenuContent(coord);
                ImGui::EndMenu();
            }
        }

        // draws an input for manipulating the number of datapoints that should be plotted
        void drawMaxDatapointsIntInput()
        {
            int currentDataPoints = getShared().getNumRequestedDatapoints();
            if (ImGui::InputInt("num data points", &currentDataPoints, 1, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (currentDataPoints >= 0) {
                    updShared().setNumRequestedDataPoints(currentDataPoints);
                }
            }
        }

        // draws an input for manipulating the number of history entries this plot panel holds
        void drawMaxHistoryEntriesIntInput()
        {
            int maxHistoryEntries = m_Lines.getMaxHistoryEntries();
            if (ImGui::InputInt("max history size", &maxHistoryEntries, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (maxHistoryEntries >= 0) {
                    m_Lines.setMaxHistoryEntries(maxHistoryEntries);
                }
            }
        }

        // draws a combo that lets the user change which output of the
        // current component is plotted
        void drawPlotOutputSelectorCombo()
        {
            auto const& availableOutputs = getShared().availableOutputs();

            std::vector<CStringView> names;
            names.reserve(availableOutputs.size());

            size_t active = 0;
            PlottableOutput currentOutput = getShared().getPlottedOutput();

            for (size_t i = 0; i < availableOutputs.size(); ++i) {
                PlottableOutput const& o = availableOutputs[i];
                names.push_back(o.getName());
                if (o == currentOutput) {
                    active = i;
                }
            }

            if (Combo("output", &active, names)) {
                updShared().setPlottedOutput(availableOutputs[active]);
            }
        }

        // draws the content of a menu for manipulating the plot's legend
        void drawLegendContextMenuContent()
        {
            // draw hide legend button
            {
                static_assert(std::is_same_v<decltype(m_PlotFlags), int>);
                auto f = static_cast<unsigned int>(m_PlotFlags);
                if (ImGui::CheckboxFlags("Hide", &f, ImPlotFlags_NoLegend)) {
                    m_PlotFlags = static_cast<int>(f);
                }
            }

            // draw outside/inside legend button
            {
                static_assert(std::is_same_v<decltype(m_LegendFlags), int>);
                auto f = static_cast<unsigned int>(m_LegendFlags);
                if (ImGui::CheckboxFlags("Outside", &f, ImPlotLegendFlags_Outside)) {
                    m_LegendFlags = static_cast<int>(f);
                }
            }

            const float s = ImGui::GetFrameHeight();
            ImVec2 const dims{1.5f * s, s};

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
            if (ImGui::Button("NW", dims))         { m_LegendLocation = ImPlotLocation_NorthWest; } ImGui::SameLine();
            if (ImGui::Button("N", dims))          { m_LegendLocation = ImPlotLocation_North; }     ImGui::SameLine();
            if (ImGui::Button("NE", dims))         { m_LegendLocation = ImPlotLocation_NorthEast; }
            if (ImGui::Button("W", dims))          { m_LegendLocation = ImPlotLocation_West; }      ImGui::SameLine();
            if (ImGui::InvisibleButton("C", dims)) { m_LegendLocation = ImPlotLocation_Center; }    ImGui::SameLine();
            if (ImGui::Button("E", dims))          { m_LegendLocation = ImPlotLocation_East; }
            if (ImGui::Button("SW", dims))         { m_LegendLocation = ImPlotLocation_SouthWest; } ImGui::SameLine();
            if (ImGui::Button("S", dims))          { m_LegendLocation = ImPlotLocation_South; }     ImGui::SameLine();
            if (ImGui::Button("SE", dims))         { m_LegendLocation = ImPlotLocation_SouthEast; }
            ImGui::PopStyleVar();
        }

        // draws the content of a menu for exporting plot data to a CSV
        void drawExportCSVMenuContent(OpenSim::Coordinate const& coord)
        {
            int id = 0;

            for (size_t i = 0; i < m_Lines.getNumOtherPlots(); ++i) {
                ImGui::PushID(id++);
                if (ImGui::MenuItem(m_Lines.getOtherPlot(i).getName().c_str())) {
                    ActionPromptUserToSavePlotToCSV(coord, getShared().getPlotParams(), m_Lines.getOtherPlot(i));
                }
                ImGui::PopID();
            }

            ImGui::PushID(id++);
            if (ImGui::MenuItem(m_Lines.getActivePlot().getName().c_str())) {
                ActionPromptUserToSavePlotToCSV(coord, getShared().getPlotParams(), m_Lines.getActivePlot());
            }
            ImGui::PopID();

            ImGui::Separator();

            ImGui::PushID(id++);
            if (ImGui::MenuItem("Export All Curves")) {
                ActionPromptUserToSavePlotLinesToCSV(coord, getShared().getPlotParams(), m_Lines);
            }
            DrawTooltipIfItemHovered("Export All Curves to CSV", "Exports all curves in the plot to a CSV file.\n\nThe implementation will try to group things together by X value, but the CSV file *may* contain sparse rows if (e.g.) some curves have a different number of plot points, or some curves were loaded from another CSV, etc.");
            ImGui::PopID();
        }

        // tries to duplicate the current plot (settings etc.) into a new plot panel
        void actionDuplicateCurrentPlotIntoNewPanel(OpenSim::Coordinate const& coord)
        {
            auto const* musc = FindComponent<OpenSim::Muscle>(getShared().getModel().getModel(), getShared().getPlotParams().getMusclePath());
            if (musc) {
                updShared().updEditorAPI().addMusclePlot(coord, *musc);
            }
        }

        // plot data state
        PlotLines m_Lines;

        // UI/drawing/widget state
        Color m_ComputedPlotLineBaseColor = Color::white();
        bool m_LegendPopupIsOpen = false;
        bool m_ShowMarkersOnActivePlot = true;
        bool m_ShowMarkersOnOtherPlots = false;
        bool m_SnapCursor = false;
        ImPlotFlags m_PlotFlags = ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoTitle;
        ImPlotLocation m_LegendLocation = ImPlotLocation_NorthWest;
        ImPlotLegendFlags m_LegendFlags = ImPlotLegendFlags_None;
        Color m_LockedCurveTint = {0.5f, 0.5f, 1.0f, 1.1f};
        Color m_LoadedCurveTint = {0.5f, 1.0f, 0.5f, 1.0f};
    };
}

// other states
 namespace
 {
    // state in which a user is being prompted to select a coordinate in the model
    class PickCoordinateState final : public MusclePlotState {
    public:
        explicit PickCoordinateState(SharedStateData& shared_) :
            MusclePlotState{shared_}
        {
            // this is what this state is populating
            updShared().updPlotParams().setCoordinatePath(OpenSim::ComponentPath{});
        }

    private:
        std::unique_ptr<MusclePlotState> implOnDraw() final
        {
            std::unique_ptr<MusclePlotState> rv;

            std::vector<OpenSim::Coordinate const*> coordinates;
            for (OpenSim::Coordinate const& coord : getShared().getModel().getModel().getComponentList<OpenSim::Coordinate>()) {
                coordinates.push_back(&coord);
            }
            std::sort(
                coordinates.begin(),
                coordinates.end(),
                IsNameLexographicallyLowerThan<OpenSim::Component const*>
            );

            ImGui::Text("select coordinate:");

            ImGui::BeginChild("MomentArmPlotCoordinateSelection");
            for (OpenSim::Coordinate const* coord : coordinates) {
                if (ImGui::Selectable(coord->getName().c_str())) {
                    updShared().updPlotParams().setCoordinatePath(GetAbsolutePath(*coord));
                    rv = std::make_unique<ShowingPlotState>(updShared());
                }
            }
            ImGui::EndChild();

            return rv;
        }
    };

    // state in which a user is being prompted to select a muscle in the model
    class PickMuscleState final : public MusclePlotState {
    public:
        explicit PickMuscleState(SharedStateData& shared_) :
            MusclePlotState{shared_}
        {
            // this is what this state is populating
            updShared().updPlotParams().setMusclePath(OpenSim::ComponentPath{});
        }

    private:
        std::unique_ptr<MusclePlotState> implOnDraw() final
        {
            std::unique_ptr<MusclePlotState> rv;

            std::vector<OpenSim::Muscle const*> muscles;
            for (OpenSim::Muscle const& musc : getShared().getModel().getModel().getComponentList<OpenSim::Muscle>()) {
                muscles.push_back(&musc);
            }
            std::sort(
                muscles.begin(),
                muscles.end(),
                IsNameLexographicallyLowerThan<OpenSim::Component const*>
            );

            ImGui::Text("select muscle:");

            if (muscles.empty()) {
                ImGui::TextDisabled("(the model contains no muscles?)");
            }
            else {
                ImGui::BeginChild("MomentArmPlotMuscleSelection");
                for (OpenSim::Muscle const* musc : muscles) {
                    if (ImGui::Selectable(musc->getName().c_str())) {
                        updShared().updPlotParams().setMusclePath(GetAbsolutePath(*musc));
                        rv = std::make_unique<PickCoordinateState>(updShared());
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

    Impl(
        IEditorAPI* editorAPI,
        std::shared_ptr<UndoableModelStatePair> uim,
        std::string_view panelName) :

        m_SharedData{editorAPI, std::move(uim)},
        m_ActiveState{std::make_unique<PickMuscleState>(m_SharedData)},
        m_PanelName{panelName}
    {}

    Impl(
        IEditorAPI* editorAPI,
        std::shared_ptr<UndoableModelStatePair> uim,
        std::string_view panelName,
        OpenSim::ComponentPath const& coordPath,
        OpenSim::ComponentPath const& musclePath) :

        m_SharedData{editorAPI, std::move(uim), coordPath, musclePath},
        m_ActiveState{std::make_unique<ShowingPlotState>(m_SharedData)},
        m_PanelName{panelName}
    {}

    std::string const& getName() const { return m_PanelName; }
    bool isOpen() const { return m_IsOpen; }
    void open() { m_IsOpen = true; }
    void close() { m_IsOpen = false; }

    void onDraw()
    {
        if (m_IsOpen) {
            bool isOpen = m_IsOpen;

            if (ImGui::Begin(m_PanelName.c_str(), &isOpen)) {
                if (auto maybeNextState = m_ActiveState->onDraw()) {
                    m_ActiveState = std::move(maybeNextState);
                }
                m_IsOpen = isOpen;
            }
            ImGui::End();

            if (isOpen != m_IsOpen) {
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

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(
    IEditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> uim,
    std::string_view panelName) :

    m_Impl{std::make_unique<Impl>(editorAPI, std::move(uim), panelName)}
{}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(
    IEditorAPI* editorAPI,
    std::shared_ptr<UndoableModelStatePair> uim,
    std::string_view panelName,
    OpenSim::ComponentPath const& coordPath,
    OpenSim::ComponentPath const& musclePath) :

    m_Impl{std::make_unique<Impl>(editorAPI, std::move(uim), panelName, coordPath, musclePath)}
{}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(ModelMusclePlotPanel&&) noexcept = default;
osc::ModelMusclePlotPanel& osc::ModelMusclePlotPanel::operator=(ModelMusclePlotPanel&&) noexcept = default;
osc::ModelMusclePlotPanel::~ModelMusclePlotPanel() noexcept = default;

CStringView osc::ModelMusclePlotPanel::implGetName() const { return m_Impl->getName(); }
bool osc::ModelMusclePlotPanel::implIsOpen() const { return m_Impl->isOpen(); }
void osc::ModelMusclePlotPanel::implOpen() { m_Impl->open(); }
void osc::ModelMusclePlotPanel::implClose() { m_Impl->close(); }
void osc::ModelMusclePlotPanel::implOnDraw() { m_Impl->onDraw(); }
