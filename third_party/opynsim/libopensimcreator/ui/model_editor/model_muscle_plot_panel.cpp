#include "model_muscle_plot_panel.h"

#include <libopensimcreator/documents/model/model_state_commit.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/open_sim_creator_colors.h>
#include <libopensimcreator/ui/shared/add_muscle_plot_event.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/formats/csv.h>
#include <liboscar/graphics/color.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/file_dialog_filter.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/widget.h>
#include <liboscar/shims/cpp20/stop_token.h>
#include <liboscar/shims/cpp20/thread.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/string_helpers.h>
#include <liboscar/utilities/synchronized_value.h>
#include <liboscar/utilities/synchronized_value_guard.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/AbstractGeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <compare>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace SimTK
{
    class State;
}

using namespace osc;
namespace rgs = std::ranges;
namespace plot = osc::ui::plot;

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
            double (*getter)(
                const SimTK::State& st,
                const OpenSim::Muscle& muscle,
                const OpenSim::Coordinate& c)) :

            name_{name},
            units_{units},
            getter_{getter}
        {}

        CStringView get_name() const { return name_; }

        CStringView get_units() const { return units_; }

        double operator()(
            const SimTK::State& st,
            const OpenSim::Muscle& muscle,
            const OpenSim::Coordinate& c) const
        {
            return getter_(st, muscle, c);
        }

        friend auto operator<=>(const PlottableOutput& lhs, const PlottableOutput& rhs)
        {
            return lhs.name_ <=> rhs.name_;
        }

        friend bool operator==(const PlottableOutput& lhs, const PlottableOutput& rhs)
        {
            return lhs.name_ == rhs.name_;
        }

    private:
        CStringView name_;
        CStringView units_;
        double (*getter_)(
            const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate& c);
    };

    double get_moment_arm(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate& c)
    {
        return muscle.getPath().computeMomentArm(st, c);
    }

    double get_fiber_length(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberLength(st);
    }

    double get_total_length(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getLength(st);
    }

    double get_tendon_length(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonLength(st);
    }

    double get_pennation_angle(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return Degreesd{Radiansd{muscle.getPennationAngle(st)}}.count();
    }

    double get_normalized_fiber_length(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getNormalizedFiberLength(st);
    }

    double get_tendon_strain(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonStrain(st);
    }

    double get_fiber_potential_energy(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberPotentialEnergy(st);
    }

    double get_tendon_potential_energy(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonPotentialEnergy(st);
    }

    double get_muscle_potential_energy(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getMusclePotentialEnergy(st);
    }

    double get_tendon_force(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonForce(st);
    }

    double get_active_fiber_force(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getActiveFiberForce(st);
    }

    double get_passive_fiber_force(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getPassiveFiberForce(st);
    }

    double get_total_fiber_force(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberForce(st);
    }

    double get_fiber_stiffness(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberStiffness(st);
    }

    double get_fiber_stiffness_along_tendon(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberStiffnessAlongTendon(st);
    }

    double get_tendon_stiffness(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonStiffness(st);
    }

    double get_muscle_stiffness(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getMuscleStiffness(st);
    }

    double get_fiber_active_power(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberActivePower(st);
    }

    double get_fiber_passive_power(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getFiberActivePower(st);
    }

    double get_tendon_power(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonPower(st);
    }

    double get_muscle_power(
        const SimTK::State& st, const OpenSim::Muscle& muscle, const OpenSim::Coordinate&)
    {
        return muscle.getTendonPower(st);
    }

    PlottableOutput get_default_muscle_output()
    {
        return PlottableOutput{"Moment Arm", "m", get_moment_arm};
    }

    std::vector<PlottableOutput> generate_muscle_outputs()
    {
        std::vector<PlottableOutput> rv = {{
            get_default_muscle_output(),
            {"Tendon Length", "m", get_tendon_length},
            {"Fiber Length", "m", get_fiber_length},
            {"Total Length", "m", get_total_length},
            {"Pennation Angle", "deg", get_pennation_angle},
            {"Normalized Fiber Length", "Unitless", get_normalized_fiber_length},
            {"Tendon Strain", "Unitless", get_tendon_strain},
            {"Fiber Potential Energy", "J", get_fiber_potential_energy},
            {"Tendon Potential Energy", "J", get_tendon_potential_energy},
            {"Muscle Potential Energy", "J", get_muscle_potential_energy},
            {"Tendon Force", "N", get_tendon_force},
            {"Active Fiber Force", "N", get_active_fiber_force},
            {"Passive Fiber Force", "N", get_passive_fiber_force},
            {"Total Fiber Force", "N", get_total_fiber_force},
            {"Fiber Stiffness", "N/m", get_fiber_stiffness},
            {"Fiber Stiffness Along Tendon", "N/m", get_fiber_stiffness_along_tendon},
            {"Tendon Stiffness", "N/m", get_tendon_stiffness},
            {"Muscle Stiffness", "N/m", get_muscle_stiffness},
            {"Fiber Active Power", "W", get_fiber_active_power},
            {"Fiber Passive Power", "W", get_fiber_passive_power},
            {"Tendon Power", "W", get_tendon_power},
            {"Muscle Power", "W", get_muscle_power},
        }};
        rgs::sort(rv);
        return rv;
    }
}

// backend datastructures
//
// these are the datastructures that the widget mostly plays around with
namespace
{
    inline constexpr int c_default_num_plot_points = 65;

    // parameters for generating a plot line
    //
    // i.e. changing any part of the parameters may produce a different curve
    class PlotParameters final {
    public:
        PlotParameters(
            ModelStateCommit commit,
            OpenSim::ComponentPath coordinate_path,
            OpenSim::ComponentPath muscle_path,
            PlottableOutput output,
            int requested_num_data_points) :

            commit_{std::move(commit)},
            coordinate_path_{std::move(coordinate_path)},
            muscle_path_{std::move(muscle_path)},
            output_{output},
            requested_num_data_points_{requested_num_data_points}
        {}

        const ModelStateCommit& get_commit() const { return commit_; }

        void set_commit(const ModelStateCommit& commit) { commit_ = commit; }

        const OpenSim::ComponentPath& get_coordinate_path() const { return coordinate_path_; }

        void set_coordinate_path(const OpenSim::ComponentPath& cp) { coordinate_path_ = cp; }

        const OpenSim::ComponentPath& get_muscle_path() const { return muscle_path_; }

        void set_muscle_path(const OpenSim::ComponentPath& cp) { muscle_path_ = cp; }

        const PlottableOutput& get_plotted_output() const { return output_; }

        void set_plotted_output(const PlottableOutput& output) { output_ = output; }

        int get_num_requested_data_points() const { return requested_num_data_points_; }

        void set_num_requested_data_points(int v) { requested_num_data_points_ = v; }

        friend bool operator==(const PlotParameters&, const PlotParameters&) = default;

    private:
        ModelStateCommit commit_;
        OpenSim::ComponentPath coordinate_path_;
        OpenSim::ComponentPath muscle_path_;
        PlottableOutput output_;
        int requested_num_data_points_;
    };

    double get_first_x_value(const PlotParameters&, const OpenSim::Coordinate& c)
    {
        return c.getRangeMin();
    }

    double get_last_x_value(const PlotParameters&, const OpenSim::Coordinate& c)
    {
        return c.getRangeMax();
    }

    double get_step_between_x_values(const PlotParameters& p, const OpenSim::Coordinate& c)
    {
        const double start = get_first_x_value(p, c);
        const double end = get_last_x_value(p, c);

        return (end - start) / max(1, p.get_num_requested_data_points() - 1);
    }

    using PlotDataPoint = Vector2;

    // virtual interface to a thing that can receive datapoints from a plotter
    class PlotDataPointConsumer {
    protected:
        PlotDataPointConsumer() = default;
        PlotDataPointConsumer(const PlotDataPointConsumer&) = default;
        PlotDataPointConsumer(PlotDataPointConsumer&&) noexcept = default;
        PlotDataPointConsumer& operator=(const PlotDataPointConsumer&) = default;
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
        PlottingTaskStatus get_status() const { return status_.load(); }

        std::optional<std::string> get_error_message() const
        {
            auto lock = error_message_.lock();
            return *lock;
        }

        void set_error_message(std::string s)
        {
            auto lock = error_message_.lock();
            *lock = std::move(s);
        }

        void set_status(PlottingTaskStatus s) { status_ = s; }

    private:
        std::atomic<PlottingTaskStatus> status_ = PlottingTaskStatus::Running;
        SynchronizedValue<std::string> error_message_;
    };

    // all inputs to the plotting function
    struct PlottingTaskInputs final {

        PlottingTaskInputs(
            std::shared_ptr<PlottingTaskThreadsafeSharedData> shared,
            PlotParameters plot_parameters,
            std::shared_ptr<PlotDataPointConsumer> data_point_consumer) :

            shared{std::move(shared)},
            plotParameters{std::move(plot_parameters)},
            dataPointConsumer{std::move(data_point_consumer)}
        {}

        std::shared_ptr<PlottingTaskThreadsafeSharedData> shared;
        PlotParameters plotParameters;
        std::shared_ptr<PlotDataPointConsumer> dataPointConsumer;
    };

    // inner (exception unsafe) plot function
    //
    // this is the function that actually does the "work" of computing plot points
    PlottingTaskStatus compute_plot_points_unguarded(
        const cpp20::stop_token& stop_token, PlottingTaskInputs& inputs)
    {
        PlottingTaskThreadsafeSharedData& shared = *inputs.shared;
        const PlotParameters& params = inputs.plotParameters;
        PlotDataPointConsumer& callback = *inputs.dataPointConsumer;

        if (params.get_num_requested_data_points() <= 0) {
            return PlottingTaskStatus::Finished;
        }

        // create a local copy of the model
        std::unique_ptr<OpenSim::Model> model =
            std::make_unique<OpenSim::Model>(*params.get_commit().get_model());

        if (stop_token.stop_requested()) {
            return PlottingTaskStatus::Cancelled;
        }

        // init the model + state

        opyn::initialize_model(*model);

        if (stop_token.stop_requested()) {
            return PlottingTaskStatus::Cancelled;
        }

        SimTK::State& state = opyn::initialize_state(*model);

        if (stop_token.stop_requested()) {
            return PlottingTaskStatus::Cancelled;
        }

        const auto* maybe_muscle =
            opyn::find_component<OpenSim::Muscle>(*model, params.get_muscle_path());
        if (not maybe_muscle) {
            shared.set_error_message(
                params.get_muscle_path().toString() + ": cannot find a muscle with this name");
            return PlottingTaskStatus::Error;
        }
        const OpenSim::Muscle& muscle = *maybe_muscle;

        const OpenSim::Coordinate* maybe_coord =
            opyn::find_component_mut<OpenSim::Coordinate>(*model, params.get_coordinate_path());
        if (not maybe_coord) {
            shared.set_error_message(
                params.get_coordinate_path().toString() +
                ": cannot find a coordinate with this name");
            return PlottingTaskStatus::Error;
        }
        const OpenSim::Coordinate& coord = *maybe_coord;

        const int num_data_points = params.get_num_requested_data_points();
        const double first_x_value = get_first_x_value(params, coord);
        const double last_x_value = get_last_x_value(params, coord);
        const double step_between_x_values = get_step_between_x_values(params, coord);

        if (first_x_value > last_x_value) {
            // this invariant is necessary because other algorithms assume X increases over
            // the datapoint collection (e.g. for optimized binary searches, lower_bound etc.)

            shared.set_error_message(
                params.get_coordinate_path().toString() +
                ": cannot plot a coordinate with reversed min/max");
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

        if (stop_token.stop_requested()) {
            return PlottingTaskStatus::Cancelled;
        }

        for (int i = 0; i < num_data_points; ++i) {
            if (stop_token.stop_requested()) {
                return PlottingTaskStatus::Cancelled;
            }

            const double x_val = first_x_value + (i * step_between_x_values);
            coord.setValue(state, x_val);
            const float x_display_val = opyn::convert_coord_value_to_display_value(coord, x_val);

            float y_val = quiet_nan_v<float>;
            try {
                model->equilibrateMuscles(state);
                model->realizeReport(state);
                y_val = static_cast<float>(params.get_plotted_output()(state, muscle, coord));
            } catch (const std::exception& ex) {
                // Either cannot equilibrate the muscle for this state, or cannot
                // realize the state to a level that's plottable. Either way, emit
                // a NaN value to the callback rather than fatally stopping processing,
                // so that the user can see gaps in the plot where the errors are.
                //
                // Related: #1070: users might be editing models that contain
                // in-development muscles that cannot be equilibrated.
                static_cast<void>(ex);
            }
            callback(PlotDataPoint{x_display_val, y_val});
        }

        return PlottingTaskStatus::Finished;
    }

    // top-level "main" function that the Plotting task worker thread executes
    //
    // catches exceptions and propagates them to the task
    int compute_plot_points_main(const cpp20::stop_token& stop_token, PlottingTaskInputs inputs)
    {
        try {
            inputs.shared->set_status(PlottingTaskStatus::Running);
            const PlottingTaskStatus status = compute_plot_points_unguarded(stop_token, inputs);
            inputs.shared->set_status(status);
            return 0;
        } catch (const std::exception& ex) {
            log_error(
                "ComputePlotPointsMain: exception thrown while computing a plot: {}", ex.what());
            inputs.shared->set_error_message(ex.what());
            inputs.shared->set_status(PlottingTaskStatus::Error);
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
            const PlotParameters& params, std::shared_ptr<PlotDataPointConsumer> consumer) :

            worker_thread_{
                compute_plot_points_main, PlottingTaskInputs{shared_, params, std::move(consumer)}}
        {}

        PlottingTaskStatus get_status() const { return shared_->get_status(); }

        std::optional<std::string> get_error_string() const { return shared_->get_error_message(); }

    private:
        std::shared_ptr<PlottingTaskThreadsafeSharedData> shared_ =
            std::make_shared<PlottingTaskThreadsafeSharedData>();
        cpp20::jthread worker_thread_;
    };

    // a data plot (line), potentially computed from a background thread, or loaded via a
    // file
    class Plot final : public PlotDataPointConsumer {
    public:
        // assumed to be a plot that is probably being computed elsewhere
        explicit Plot(const PlotParameters& parameters) :
            parameters_{parameters},
            name_{parameters.get_commit().get_commit_message()}
        {
            data_points_.lock()->reserve(parameters_->get_num_requested_data_points());
        }

        // assumed to be a plot that was loaded from disk
        Plot(std::string name, std::vector<PlotDataPoint> data) :
            parameters_{std::nullopt},
            name_{std::move(name)},
            data_points_{std::move(data)}
        {}

        CStringView get_name() const { return name_; }

        const PlotParameters* try_get_parameters() const
        {
            return parameters_.has_value() ? &parameters_.value() : nullptr;
        }

        std::vector<PlotDataPoint> copy_data_points() const
        {
            auto lock = data_points_.lock();
            return *lock;
        }

        SynchronizedValueGuard<const std::vector<PlotDataPoint>> lock_data_points() const
        {
            return data_points_.lock();
        }

        SynchronizedValueGuard<std::vector<PlotDataPoint>> lock_data_points()
        {
            return data_points_.lock();
        }

        void operator()(PlotDataPoint p) override
        {
            {
                auto lock = data_points_.lock();
                lock->push_back(p);
            }

            // something happened on a background thread, the UI thread should probably redraw
            App::upd().request_redraw();
        }

        bool get_is_locked() const { return is_locked_; }

        void set_is_locked(bool v) { is_locked_ = v; }

        void set_commit(const ModelStateCommit& commit)
        {
            if (parameters_) {
                parameters_->set_commit(commit);
                name_ = parameters_->get_commit().get_commit_message();
            }
        }

        bool is_externally_provided() const { return !parameters_.has_value(); }

        bool is_locked() const { return is_locked_; }

    private:
        std::optional<PlotParameters> parameters_;
        std::string name_;
        bool is_locked_ = false;
        SynchronizedValue<std::vector<PlotDataPoint>> data_points_;
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

    bool x_less_than(const Vector2& lhs, const Vector2& rhs)
    {
        return lhs.x() < rhs.x();
    }

    std::optional<float> compute_ler_ped_y(const Plot& p, float x)
    {
        auto lock = p.lock_data_points();
        const std::span<const PlotDataPoint> points = *lock;

        if (points.empty()) {
            // there are no data points
            return std::nullopt;
        }

        const auto it = rgs::lower_bound(points, PlotDataPoint{x, 0.0f}, x_less_than);

        if (it == points.end()) {
            // X is out of bounds
            return std::nullopt;
        }

        if (it == points.begin()) {
            // X if off the left-hand side
            return points.front().y();
        }

        // else: the iterator is pointing somewhere in the middle of the data
        //       and we need to potentially LERP between two points
        const size_t above_idx = std::distance(points.begin(), it);
        const size_t below_idx = above_idx - 1;
        const PlotDataPoint below = points[below_idx];
        const PlotDataPoint above = points[above_idx];

        const float t = (x - below.x()) / (above.x() - below.x());  // [0..1]

        return lerp(below.y(), above.y(), t);
    }

    std::optional<PlotDataPoint> find_nearest_point(const Plot& p, float x)
    {
        auto lock = p.lock_data_points();
        const std::span<const PlotDataPoint> points = *lock;

        if (points.empty()) {
            // there are no data points
            return std::nullopt;
        }

        const auto it = rgs::lower_bound(points, PlotDataPoint{x, 0.0f}, x_less_than);

        if (it == points.begin()) {
            // closest is the leftmost point
            return points.front();
        }

        if (it == points.end()) {
            // closest is the rightmost point
            return points.back();
        }

        // else: the iterator is pointing to the element above the X location and we
        //       need to figure out if that's closer than the element below the X
        //       location
        const size_t above_idx = std::distance(points.begin(), it);
        const size_t below_idx = above_idx - 1;
        const PlotDataPoint below = points[below_idx];
        const PlotDataPoint above = points[above_idx];

        const float below_distance = abs(below.x() - x);
        const float above_distance = abs(above.x() - x);

        const size_t closest_idx = above_distance < below_distance ? above_idx : below_idx;

        return points[closest_idx];
    }

    bool is_x_in_range(const Plot& p, float x)
    {
        auto lock = p.lock_data_points();
        const std::span<const PlotDataPoint> points = *lock;

        if (points.size() <= 1) {
            return false;
        }

        return points.front().x() <= x && x <= points.back().x();
    }

    std::string ith_plot_line_name(const Plot& p, size_t i)
    {
        std::stringstream ss;

        ss << i << ") " << p.get_name();
        if (p.get_is_locked()) {
            ss << " " MSMICONS_LOCK;
        }
        return std::move(ss).str();
    }

    std::ostream& write_y_axis_name(const PlotParameters& params, std::ostream& o)
    {
        return o << params.get_plotted_output().get_name();
    }

    std::ostream& write_x_axis_name(const PlotParameters& params, std::ostream& o)
    {
        return o << params.get_coordinate_path().getComponentName();
    }

    std::string compute_plot_title(const PlotParameters& params)
    {
        std::stringstream ss;
        ss << params.get_muscle_path().getComponentName() << ' ';
        write_y_axis_name(params, ss);
        ss << " vs ";
        write_x_axis_name(params, ss);
        return std::move(ss).str();
    }

    std::string compute_plot_y_axis_title(const PlotParameters& params)
    {
        std::stringstream ss;
        write_y_axis_name(params, ss);
        ss << " [" << params.get_plotted_output().get_units() << ']';
        return std::move(ss).str();
    }

    std::string compute_plot_x_axis_title(
        const PlotParameters& params, const OpenSim::Coordinate& coord)
    {
        std::stringstream ss;
        write_x_axis_name(params, ss);
        ss << " value [" << opyn::get_coord_display_value_units_string(coord) << ']';
        return std::move(ss).str();
    }

    std::vector<Plot> try_load_svc_file_as_plots(const std::filesystem::path& input_path)
    {
        // create input reader
        std::ifstream input_file_stream{input_path};
        input_file_stream.exceptions(std::ios_base::badbit);
        if (not input_file_stream) {
            return {};  // error opening path
        }

        // try to read header row
        std::vector<std::string> headers;
        if (not CSV::read_row_into_vector(input_file_stream, headers)) {
            return {};  // no CSV data (headers) in top row
        }

        // map each CSV row from [$independent, ...$dependent] -> [($independent, $dependent[i])]
        std::vector<std::vector<PlotDataPoint>> columns_as_plots;
        for (std::vector<std::string> row; CSV::read_row_into_vector(input_file_stream, row);) {
            if (row.size() < 2) {
                continue;  // skip: row does not contain enough columns
            }

            const std::optional<float> independent_var = from_chars_strip_whitespace(row.front());
            if (not independent_var) {
                continue;  // skip: row does not contain a valid independent variable
            }

            // parse remaining columns as dependent variables
            for (size_t dependent_col = 1; dependent_col < row.size(); ++dependent_col) {
                const std::string& dependent_var_str = row[dependent_col];
                const std::optional<float> dependent_var =
                    from_chars_strip_whitespace(dependent_var_str);
                if (not dependent_var) {
                    continue;  // skip: column cannot be parsed as a number
                }

                // else: append column as ($independent, $dependent[col]) to the plots vector
                columns_as_plots.resize(max(columns_as_plots.size(), dependent_col));
                columns_as_plots[dependent_col - 1].emplace_back(*independent_var, *dependent_var);
            }
        }

        if (columns_as_plots.empty()) {
            // 0 series: return no plots
            return {};
        } else if (columns_as_plots.size() == 1) {
            // 1 series: return one plot with one series named `$filename`
            return {Plot{input_path.filename().string(), std::move(columns_as_plots.front())}};
        } else {
            // >1 series: return each plot named `$filename ($header)` (or a number)

            std::vector<Plot> rv;
            rv.reserve(columns_as_plots.size());
            for (size_t i = 0; i < columns_as_plots.size(); ++i) {
                std::stringstream ss;
                ss << input_path.filename();
                ss << " (";
                if (i < headers.size()) {
                    ss << headers[i];
                } else {
                    ss << i;
                }
                ss << ')';

                rv.emplace_back(std::move(ss).str(), std::move(columns_as_plots[i]));
            }

            return rv;
        }
    }

    void write_plot_as_csv(
        const OpenSim::Coordinate& coord,
        const PlotParameters& params,
        const Plot& plot,
        std::ostream& out)
    {
        // write header
        CSV::write_row(
            out,
            std::to_array(
                {compute_plot_x_axis_title(params, coord), compute_plot_y_axis_title(params)}));

        // write data rows
        auto lock = plot.lock_data_points();
        for (const PlotDataPoint& p : *lock) {
            CSV::write_row(out, std::to_array({std::to_string(p.x()), std::to_string(p.y())}));
        }
    }

    void action_prompt_user_to_save_plot_to_csv(
        const OpenSim::Coordinate& coord, const PlotParameters& params, const Plot& plot)
    {
        // Pre-write the CSV in memory so that the asynchronous user request isn't dependent on
        // any UI state.
        std::stringstream ss;
        write_plot_as_csv(coord, params, plot, ss);

        // Asynchronously request the save location from the user and write it
        App::upd().prompt_user_to_save_file_with_extension_async(
            [csv = std::move(ss).str()](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }
                std::ofstream ofs{*p};
                if (not ofs) {
                    log_error("{}: cannot open path for writing", p->string());
                    return;
                }
                ofs << csv;
            },
            "csv");
    }

    // holds a collection of plotlines that are to-be-drawn on the plot
    class PlotLines final {
    public:
        explicit PlotLines(const PlotParameters& params) :
            active_plot_{std::make_shared<Plot>(params)},
            plotting_task_{params, active_plot_}
        {}

        void on_before_drawing(const UndoableModelStatePair&, const PlotParameters& desired_params)
        {
            // perform any datastructure invariant checks etc.

            check_for_parameter_changes_and_start_plotting(desired_params);
            handle_user_enacted_deletions();
            ensure_previous_curves_does_not_exceed_max();
        }

        void clear_unlocked_plots()
        {
            std::erase_if(previous_plots_, [](const std::shared_ptr<Plot>& p) {
                return not p->get_is_locked();
            });
        }

        PlottingTaskStatus get_plotting_task_status() const { return plotting_task_.get_status(); }

        std::optional<std::string> try_get_plotting_task_error_message() const
        {
            return plotting_task_.get_error_string();
        }

        const Plot& get_active_plot() const { return *active_plot_; }

        size_t get_num_other_plots() const { return previous_plots_.size(); }

        const Plot& get_other_plot(size_t i) const { return *previous_plots_.at(i); }

        void tag_other_plot_for_deletion(size_t i)
        {
            plot_tagged_for_deletion_ = static_cast<ptrdiff_t>(i);
        }

        void set_other_plot_locked(size_t i, bool v) { previous_plots_.at(i)->set_is_locked(v); }

        void set_active_plot_locked(bool v) { active_plot_->set_is_locked(v); }

        int get_max_history_entries() const { return max_history_entries_; }

        void set_max_history_entries(int i)
        {
            if (i < 0) {
                return;
            }
            max_history_entries_ = i;
        }

        void set_active_plot_commit(const ModelStateCommit& commit)
        {
            active_plot_->set_commit(commit);
        }

        void push_plot_as_previous(Plot p)
        {
            previous_plots_.push_back(std::make_shared<Plot>(std::move(p)));

            ensure_previous_curves_does_not_exceed_max();
        }

        void revert_to_previous_plot(UndoableModelStatePair& model, size_t i)
        {
            // fetch the to-be-reverted-to curve
            std::shared_ptr<Plot> ptr = previous_plots_.at(i);

            const PlotParameters* maybe_params = ptr->try_get_parameters();

            // try to revert the current model to use the plot's commit
            if (maybe_params and model.try_checkout(maybe_params->get_commit())) {
                // it checked out successfully, so update this plotting widget
                // accordingly

                /// remove it from the history list (it'll become active)
                previous_plots_.erase(previous_plots_.begin() + i);

                // swap it with the active curve
                std::swap(ptr, active_plot_);

                // push the active curve into the history
                previous_plots_.push_back(ptr);

                // and GC the history
                ensure_previous_curves_does_not_exceed_max();
            }
        }

    private:
        void clear_computed_plots()
        {
            std::erase_if(previous_plots_, [](const auto& ptr) {
                return ptr->try_get_parameters() != nullptr;
            });
        }

        void check_for_parameter_changes_and_start_plotting(const PlotParameters& desired_params)
        {
            // additions/changes
            //
            // if the current plot doesn't match the latest requested params, kick off
            // a new plotting task

            const PlotParameters* maybe_params = active_plot_->try_get_parameters();

            if (not maybe_params or (*maybe_params) != desired_params) {
                // (edge-case): if the user selection fundamentally changes what's being plotted
                // then previous plots should be cleared
                const bool clear_previous =
                    maybe_params != nullptr &&
                    (maybe_params->get_plotted_output() != desired_params.get_plotted_output() ||
                     maybe_params->get_coordinate_path() != desired_params.get_coordinate_path() ||
                     maybe_params->get_muscle_path() != desired_params.get_muscle_path());

                // create new active plot and swap the old active plot into the previous plots
                {
                    std::shared_ptr<Plot> p = std::make_shared<Plot>(desired_params);
                    std::swap(p, active_plot_);
                    previous_plots_.push_back(p);
                }

                if (clear_previous) {
                    clear_computed_plots();
                }

                // kick off a new plotting task
                plotting_task_ = PlottingTask{desired_params, active_plot_};
            }
        }

        void handle_user_enacted_deletions()
        {
            // deletions
            //
            // handle any user-requested deletions by removing the curve from the collection
            if (0 <= plot_tagged_for_deletion_ and
                plot_tagged_for_deletion_ < std::ssize(previous_plots_)) {

                previous_plots_.erase(previous_plots_.begin() + plot_tagged_for_deletion_);
                plot_tagged_for_deletion_ = -1;
            }
        }

        void ensure_previous_curves_does_not_exceed_max()
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

            auto is_first_deleteable_plot = [nth = 1, max = this->max_history_entries_](
                                                const std::shared_ptr<Plot>& p) mutable {
                if (p->get_is_locked()) {
                    return false;
                }
                return nth++ > max;
            };

            const auto backward_it =
                find_if(previous_plots_.rbegin(), previous_plots_.rend(), is_first_deleteable_plot);
            const auto forward_it = backward_it.base();
            const ptrdiff_t idx_of_deleteable_end =
                std::distance(previous_plots_.begin(), forward_it);

            auto should_delete = [i = 0z,
                                  idx_of_deleteable_end](const std::shared_ptr<Plot>& p) mutable {
                return i++ < idx_of_deleteable_end && !p->get_is_locked();
            };

            std::erase_if(previous_plots_, should_delete);
        }

        std::shared_ptr<Plot> active_plot_;
        PlottingTask plotting_task_;
        std::vector<std::shared_ptr<Plot>> previous_plots_;
        ptrdiff_t plot_tagged_for_deletion_ = -1;
        int max_history_entries_ = 6;
    };

    struct PlotLineCounts final {
        size_t external;
        size_t locked;
        size_t total;
    };

    PlotLineCounts count_other_plot_types(const PlotLines& lines)
    {
        PlotLineCounts rv{};
        for (size_t i = 0; i < lines.get_num_other_plots(); ++i) {
            const Plot& p = lines.get_other_plot(i);

            if (p.is_externally_provided()) {
                ++rv.external;
            } else if (p.is_locked()) {
                ++rv.locked;
            }
            ++rv.total;
        }
        return rv;
    }

    // tries to hittest the mouse's X position in plot-space
    std::optional<float> try_get_mouse_x_position_in_plot(
        const PlotLines& lines, bool snap_to_nearest)
    {
        // figure out mouse hover position
        const bool is_hovered = plot::is_plot_hovered();
        float mouse_x = plot::get_plot_mouse_position().x();

        // handle snapping the mouse's X position
        if (is_hovered && snap_to_nearest) {
            auto maybe_nearest = find_nearest_point(lines.get_active_plot(), mouse_x);

            if (is_x_in_range(lines.get_active_plot(), mouse_x) && maybe_nearest) {
                mouse_x = maybe_nearest->x();
            }
        }

        return is_hovered ? mouse_x : std::optional<float>{};
    }

    // returns a vector of all the headers a CSV file will contain if plotting the given lines
    std::vector<std::string> get_all_csv_headers(
        const OpenSim::Coordinate& coord, const PlotParameters& params, const PlotLines& lines)
    {
        std::vector<std::string> headers;
        headers.reserve(1 + lines.get_num_other_plots() + 1);

        headers.push_back(compute_plot_x_axis_title(params, coord));
        for (size_t i = 0, len = lines.get_num_other_plots(); i < len; ++i) {
            headers.emplace_back(to_string(lines.get_other_plot(i).get_name()));
        }
        headers.emplace_back(to_string(lines.get_active_plot().get_name()));
        return headers;
    }

    // algorithm helper class: wraps a data + cursor together
    class LineCursor final {
    public:
        explicit LineCursor(const Plot& plot) :
            data_{plot.copy_data_points()}
        {}

        std::optional<float> peek_x() const
        {
            return cursor_ != data_.end() ? cursor_->x() : std::optional<float>{};
        }

        std::optional<PlotDataPoint> peek() const
        {
            return cursor_ != data_.end() ? *cursor_ : std::optional<PlotDataPoint>{};
        }

        LineCursor& operator++()
        {
            OSC_ASSERT(cursor_ != data_.end());
            ++cursor_;
            return *this;
        }

    private:
        std::vector<PlotDataPoint> data_;
        std::vector<PlotDataPoint>::const_iterator cursor_ = data_.begin();
    };

    bool less_than_assuming_empty_highest(
        const std::optional<float>& a, const std::optional<float>& b)
    {
        // this is defined differently from the C++ standard, which makes the
        // empty optional the "minimum" value, logically
        //
        // see cppreference's definition for `std::optional<T>::operator<`

        if (not a) {
            return false;
        }
        if (not b) {
            return true;
        }
        return *a < *b;
    }

    // returns true if `a` has a lower X value than `b` - assumes an empty X value is the "highest"
    bool has_lower_x(const LineCursor& a, const LineCursor& b)
    {
        return less_than_assuming_empty_highest(a.peek_x(), b.peek_x());
    }

    // returns data-owning cursors to all lines in the given plotlines
    std::vector<LineCursor> get_cursors_to_all_plot_lines(const PlotLines& lines)
    {
        std::vector<LineCursor> cursors;
        cursors.reserve(lines.get_num_other_plots() + 1);
        for (size_t i = 0, len = lines.get_num_other_plots(); i < len; ++i) {
            cursors.emplace_back(lines.get_other_plot(i));
        }
        cursors.emplace_back(lines.get_active_plot());
        return cursors;
    }

    // returns the smallest X value accross all given plot lines - if an X value exists
    std::optional<float> calc_smallest_x(std::span<const LineCursor> cursors)
    {
        auto it = rgs::min_element(cursors, has_lower_x);
        return it != cursors.end() ? it->peek_x() : std::optional<float>{};
    }

    // tries to write the given collection of plotlines in a CSV format to the output stream.
    //
    // the resulting CSV may be sparsely populated, because each line may have a different
    // number of, and location of, values.
    void try_write_plot_lines_as_csv(
        const OpenSim::Coordinate& coord,
        const PlotParameters& params,
        const PlotLines& lines,
        std::ostream& out)
    {
        // write header
        CSV::write_row(out, get_all_csv_headers(coord, params, lines));

        // get incrementable cursors to all curves in the plot
        std::vector<LineCursor> cursors = get_cursors_to_all_plot_lines(lines);

        // calculate smallest X value among all curves (if applicable - they may all be empty)
        std::optional<float> maybe_x = calc_smallest_x(cursors);

        // keep an eye out for the *next* lowest X value as we iterate
        std::optional<float> maybe_next_x;

        while (maybe_x) {
            std::vector<std::string> cols;
            cols.reserve(1 + cursors.size());

            // emit (potentially deduped) X
            cols.push_back(std::to_string(*maybe_x));

            // emit all columns that match up with X
            for (LineCursor& cursor : cursors) {
                std::optional<PlotDataPoint> data = cursor.peek();

                if (data && (data->x() <= *maybe_x || equal_within_epsilon(data->x(), *maybe_x))) {
                    cols.push_back(std::to_string(data->y()));
                    ++cursor;
                    data = cursor.peek();  // to test the next X
                } else {
                    cols.emplace_back();  // blank cell
                }

                const std::optional<float> maybe_data_x =
                    data ? std::optional<float>{data->x()} : std::optional<float>{};
                if (less_than_assuming_empty_highest(maybe_data_x, maybe_next_x)) {
                    maybe_next_x = maybe_data_x;
                }
            }

            CSV::write_row(out, cols);

            maybe_x = maybe_next_x;
            maybe_next_x = std::nullopt;
        }
    }

    // a UI action in which the user in prompted for a CSV file that they would like to overlay
    // over the current plot
    void action_prompt_user_for_csv_overlay_file(const std::shared_ptr<PlotLines>& lines)
    {
        App::upd().prompt_user_to_select_file_async(
            [lines](const FileDialogResponse& response) {
                if (response.size() != 1) {
                    // Error, cancellation, or the user somehow selected more than one file.
                    return;
                }

                for (Plot& plot : try_load_svc_file_as_plots(response.front())) {
                    plot.set_is_locked(true);
                    lines->push_plot_as_previous(std::move(plot));
                }
            },
            {
                CSV::file_dialog_filter(),
                FileDialogFilter::all_files(),
            });
    }

    // a UI action in which the user is prompted to save a CSV file to the filesystem and then, if
    // the user selects a filesystem location, writes a sparse CSV file containing all plotlines to
    // that location
    void action_prompt_user_to_save_plot_lines_to_csv(
        const OpenSim::Coordinate& coord, const PlotParameters& params, const PlotLines& lines)
    {
        // Pre-write the plotlines in memory so that the asynchronous prompt doesn't depend on a
        // bunch of state.
        std::stringstream ss;
        try_write_plot_lines_as_csv(coord, params, lines, ss);

        App::upd().prompt_user_to_save_file_with_extension_async(
            [csv = std::move(ss).str()](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }
                std::ofstream ofs{*p};
                if (not ofs) {
                    log_error("{}: could not open file for writing", p->string());
                    return;
                }
                ofs << csv;
            });
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
        SharedStateData(Widget& parent, std::shared_ptr<UndoableModelStatePair> uim) :

            parent_{&parent},
            model_{std::move(uim)}
        {}

        SharedStateData(
            Widget& parent,
            std::shared_ptr<UndoableModelStatePair> uim,
            const OpenSim::ComponentPath& coord_path,
            const OpenSim::ComponentPath& muscle_path) :

            parent_{&parent},
            model_{std::move(uim)},
            plot_params_{
                model_->get_latest_commit(),
                coord_path,
                muscle_path,
                get_default_muscle_output(),
                c_default_num_plot_points}
        {}

        const PlotParameters& get_plot_params() const { return plot_params_; }
        PlotParameters& upd_plot_params() { return plot_params_; }

        const UndoableModelStatePair& get_model() const { return *model_; }
        UndoableModelStatePair& upd_model() { return *model_; }

        Widget& get_parent_widget() { return *parent_; }

        std::span<const PlottableOutput> available_outputs() const
        {
            return available_muscle_outputs_;
        }

        const PlottableOutput& get_plotted_output() const
        {
            return get_plot_params().get_plotted_output();
        }
        void set_plotted_output(const PlottableOutput& new_output)
        {
            upd_plot_params().set_plotted_output(new_output);
        }

        int get_num_requested_datapoints() const
        {
            return get_plot_params().get_num_requested_data_points();
        }
        void set_num_requested_data_points(int v)
        {
            upd_plot_params().set_num_requested_data_points(v);
        }

    private:
        Widget* parent_ = nullptr;
        std::shared_ptr<UndoableModelStatePair> model_;
        PlotParameters plot_params_{
            model_->get_latest_commit(),
            OpenSim::ComponentPath{},
            OpenSim::ComponentPath{},
            get_default_muscle_output(),
            180};
        std::vector<PlottableOutput> available_muscle_outputs_ = generate_muscle_outputs();
    };

    // base class for a single widget state
    class MusclePlotState {
    protected:
        explicit MusclePlotState(SharedStateData& shared) :
            shared_{&shared}
        {}
        MusclePlotState(const MusclePlotState&) = default;
        MusclePlotState(MusclePlotState&&) noexcept = default;
        MusclePlotState& operator=(const MusclePlotState&) = default;
        MusclePlotState& operator=(MusclePlotState&&) noexcept = default;

    public:
        virtual ~MusclePlotState() noexcept = default;

        std::unique_ptr<MusclePlotState> on_draw() { return impl_on_draw(); }

    protected:
        bool is_model_locked() const { return shared_->get_model().is_readonly(); }
        const SharedStateData& get_shared() const { return *shared_; }
        SharedStateData& upd_shared() { return *shared_; }

    private:
        virtual std::unique_ptr<MusclePlotState> impl_on_draw() = 0;

        SharedStateData* shared_;
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
        explicit ShowingPlotState(SharedStateData& shared) :
            MusclePlotState{shared},
            lines_{std::make_shared<PlotLines>(shared.get_plot_params())}
        {}

    private:
        std::unique_ptr<MusclePlotState> impl_on_draw() override
        {
            on_before_drawing();  // perform pre-draw cleanups/updates etc.

            if (lines_->get_plotting_task_status() == PlottingTaskStatus::Error) {
                if (auto maybe_error_string = lines_->try_get_plotting_task_error_message()) {
                    ui::draw_text("error: cannot show plot: {}", *maybe_error_string);
                }
                return nullptr;
            }

            const PlotParameters& latest_params = get_shared().get_plot_params();
            auto model_guard = latest_params.get_commit().get_model();

            const auto* maybe_coord = opyn::find_component<OpenSim::Coordinate>(
                *model_guard, latest_params.get_coordinate_path());
            if (not maybe_coord) {
                ui::draw_text(
                    "(no coordinate named {} in model)",
                    latest_params.get_coordinate_path().toString());
                return nullptr;
            }
            const OpenSim::Coordinate& coord = *maybe_coord;

            const std::string plot_title = compute_plot_title(latest_params);

            draw_plot_title(coord, plot_title);  // draw a custom title bar

            plot::push_style_var(plot::PlotStyleVar::FitPadding, {0.025f, 0.05f});
            if (plot::begin(plot_title, ui::get_content_region_available(), plot_flags_)) {

                const PlotParameters& plot_params = get_shared().get_plot_params();

                plot::setup_legend(legend_location_, legend_flags_);
                plot::setup_axes(
                    compute_plot_x_axis_title(latest_params, coord),
                    compute_plot_y_axis_title(latest_params),
                    plot::AxisFlags::Lock,
                    plot::AxisFlags::AutoFit);
                plot::setup_axis_limits(
                    plot::Axis::X1,
                    ClosedInterval{
                        opyn::convert_coord_value_to_display_value(
                            coord, get_first_x_value(plot_params, coord)),
                        opyn::convert_coord_value_to_display_value(
                            coord, get_last_x_value(plot_params, coord)),
                    },
                    0.025f,
                    plot::Condition::Always);
                plot::setup_finish();

                const std::optional<float> maybe_mouse_x =
                    try_get_mouse_x_position_in_plot(*lines_, snap_cursor_);
                draw_plot_lines(coord);
                draw_overlays(coord, maybe_mouse_x);
                handle_mouse_events(coord, maybe_mouse_x);
                if (not legend_popup_is_open_) {
                    try_draw_general_plot_popup(coord, plot_title);
                }

                plot::end();
            }
            plot::pop_style_var();

            return nullptr;
        }

        // called at the start of each `draw` call - it GCs datastructures etc.
        void on_before_drawing()
        {
            // ensure the legend test is reset (it's checked every frame)
            legend_popup_is_open_ = false;

            // ensure latest requested params reflects the latest version of the model
            upd_shared().upd_plot_params().set_commit(get_shared().get_model().get_latest_commit());

            // ensure plot lines are valid, given the current model + desired params
            lines_->on_before_drawing(get_shared().get_model(), get_shared().get_plot_params());
        }

        void draw_plot_title(const OpenSim::Coordinate& coord, const std::string& plot_title)
        {
            // the plot title should contain combo boxes that users can use to change plot
            // parameters visually (#397)

            const std::string muscle_name = truncate_with_ellipsis(
                get_shared().get_plot_params().get_muscle_path().getComponentName(), 15);
            const float muscle_name_width =
                ui::calc_text_size(muscle_name).x() + (2.0f * ui::get_style_frame_padding().x());
            const std::string output_name = truncate_with_ellipsis(
                get_shared().get_plot_params().get_plotted_output().get_name(), 15);
            const float output_name_width =
                ui::calc_text_size(output_name).x() + (2.0f * ui::get_style_frame_padding().x());
            const std::string coord_name = truncate_with_ellipsis(
                get_shared().get_plot_params().get_coordinate_path().getComponentName(), 15);
            const float coord_name_width =
                ui::calc_text_size(coord_name).x() + (2.0f * ui::get_style_frame_padding().x());

            const float total_width =
                muscle_name_width + ui::calc_text_size("'s").x() +
                ui::get_style_item_spacing().x() + output_name_width +
                ui::get_style_item_spacing().x() + ui::calc_text_size("vs.").x() +
                ui::get_style_item_spacing().x() + coord_name_width +
                ui::get_style_item_spacing().x() + ui::get_style_frame_padding().x() +
                ui::calc_text_size(MSMICONS_BARS " Options").x() +
                ui::get_style_frame_padding().x();

            const float cursor_start =
                0.5f * (ui::get_content_region_available().x() - total_width);
            ui::set_cursor_panel_x(cursor_start);

            ui::set_next_item_width(muscle_name_width);
            if (ui::begin_combobox("##musclename", muscle_name, ui::ComboFlag::NoArrowButton)) {
                const auto* current = opyn::find_component<OpenSim::Muscle>(
                    get_shared().get_model().get_model(),
                    get_shared().get_plot_params().get_muscle_path());
                for (const OpenSim::Muscle& musc :
                     get_shared().get_model().get_model().getComponentList<OpenSim::Muscle>()) {
                    bool selected = &musc == current;
                    if (ui::draw_selectable(musc.getName(), &selected)) {
                        upd_shared().upd_plot_params().set_muscle_path(musc.getAbsolutePath());
                    }
                }
                ui::end_combobox();
            }

            ui::same_line();
            ui::set_cursor_panel_x(ui::get_cursor_panel_x() - ui::get_style_item_spacing().x());
            ui::draw_text("'s");
            ui::same_line();
            ui::set_next_item_width(output_name_width);
            if (ui::begin_combobox("##outputname", output_name, ui::ComboFlag::NoArrowButton)) {
                const PlottableOutput current = get_shared().get_plot_params().get_plotted_output();
                for (const PlottableOutput& output : get_shared().available_outputs()) {
                    bool selected = output == current;
                    if (ui::draw_selectable(output.get_name(), &selected)) {
                        upd_shared().upd_plot_params().set_plotted_output(output);
                    }
                }
                ui::end_combobox();
            }
            ui::same_line();
            ui::draw_text("vs.");
            ui::same_line();
            ui::set_next_item_width(coord_name_width);
            if (ui::begin_combobox("##coordname", coord_name, ui::ComboFlag::NoArrowButton)) {
                const auto* current = opyn::find_component<OpenSim::Coordinate>(
                    get_shared().get_model().get_model(),
                    get_shared().get_plot_params().get_coordinate_path());
                for (const OpenSim::Coordinate& c :
                     get_shared().get_model().get_model().getComponentList<OpenSim::Coordinate>()) {

                    bool selected = &c == current;
                    if (ui::draw_selectable(c.getName(), &selected)) {
                        upd_shared().upd_plot_params().set_coordinate_path(
                            opyn::get_absolute_path(c));
                    }
                }
                ui::end_combobox();
            }
            ui::same_line();

            // draw little options button that opens the context menu
            //
            // it's easier for users to figure out than having to guess they need to
            // right-click the plot (#399)
            ui::draw_button(MSMICONS_BARS " Options");
            try_draw_general_plot_popup(coord, plot_title, ui::PopupFlag::MouseButtonLeft);
        }

        // draws the actual plot lines in the plot
        void draw_plot_lines(const OpenSim::Coordinate& coord)
        {
            // plot not-active plots
            const PlotLineCounts counts = count_other_plot_types(*lines_);
            size_t external_counter = 0;
            size_t locked_counter = 0;
            for (size_t i = 0; i < lines_->get_num_other_plots(); ++i) {
                const Plot& plot = lines_->get_other_plot(i);

                Color color = computed_plot_line_base_color_;

                if (plot.is_externally_provided()) {
                    // externally-provided curves should be tinted
                    color *= loaded_curve_tint_;
                    color.a *= static_cast<float>(++external_counter) /
                               static_cast<float>(counts.external);
                } else if (plot.is_locked()) {
                    // locked curves should be tinted as such
                    color *= locked_curve_tint_;
                    color.a *=
                        static_cast<float>(++locked_counter) / static_cast<float>(counts.locked);
                } else {
                    // previous curves should fade as they get older
                    color.a *= static_cast<float>(i + 1) / static_cast<float>(counts.total + 1);
                }

                if (show_markers_on_other_plots_) {
                    plot::set_next_marker_style(plot::MarkerType::Circle, 3.0f);
                }

                const std::string line_name = ith_plot_line_name(plot, i + 1);

                plot::push_style_color(plot::PlotColorVar::Line, color);
                plot::plot_line(line_name, *plot.lock_data_points());
                plot::pop_style_color();

                if (plot::begin_legend_popup(line_name)) {
                    legend_popup_is_open_ = true;

                    if (ui::draw_menu_item(MSMICONS_TRASH " delete")) {
                        lines_->tag_other_plot_for_deletion(i);
                    }
                    if (not plot.get_is_locked() and ui::draw_menu_item(MSMICONS_LOCK " lock")) {
                        lines_->set_other_plot_locked(i, true);
                    }
                    if (plot.get_is_locked() and ui::draw_menu_item(MSMICONS_UNLOCK " unlock")) {
                        lines_->set_other_plot_locked(i, false);
                    }
                    if (plot.try_get_parameters() and
                        ui::draw_menu_item(MSMICONS_UNDO " revert to this")) {

                        lines_->revert_to_previous_plot(upd_shared().upd_model(), i);
                    }
                    if (ui::draw_menu_item(MSMICONS_FILE_EXPORT " export to CSV")) {
                        action_prompt_user_to_save_plot_to_csv(
                            coord, get_shared().get_plot_params(), plot);
                    }
                    plot::end_legend_popup();
                }
            }

            // then plot the active plot
            {
                const Plot& plot = lines_->get_active_plot();
                const std::string line_name =
                    ith_plot_line_name(plot, lines_->get_num_other_plots() + 1);

                // locked curves should have a blue tint
                Color color = computed_plot_line_base_color_;

                if (plot.is_externally_provided()) {
                    // externally-provided curves should be tinted
                    color *= loaded_curve_tint_;
                } else if (plot.is_locked()) {
                    // locked curves should be tinted as such
                    color *= locked_curve_tint_;
                }

                if (show_markers_on_active_plot_) {
                    plot::set_next_marker_style(plot::MarkerType::Circle, 3.0f);
                }

                plot::push_style_color(plot::PlotColorVar::Line, color);
                plot::plot_line(line_name, *plot.lock_data_points());
                plot::pop_style_color();

                if (plot::begin_legend_popup(line_name)) {
                    legend_popup_is_open_ = true;

                    if (not plot.get_is_locked() and ui::draw_menu_item(MSMICONS_LOCK " lock")) {
                        lines_->set_active_plot_locked(true);
                    }
                    if (plot.get_is_locked() and ui::draw_menu_item(MSMICONS_UNLOCK " unlock")) {
                        lines_->set_active_plot_locked(false);
                    }
                    if (ui::draw_menu_item(MSMICONS_FILE_EXPORT " export to CSV")) {
                        action_prompt_user_to_save_plot_to_csv(
                            coord, get_shared().get_plot_params(), plot);
                    }
                    plot::end_legend_popup();
                }
            }
        }

        // draw overlays over the plot lines
        void draw_overlays(const OpenSim::Coordinate& coord, std::optional<float> maybe_mouse_x)
        {
            const double coordinate_x_in_degrees = opyn::convert_coord_value_to_display_value(
                coord, coord.getValue(get_shared().get_model().get_state()));

            // draw vertical drop line where the coordinate's value currently is
            {
                double v = coordinate_x_in_degrees;

                // CARE: this drag line shouldn't cause the plotter to re-fit because it will
                // make the plotter re-fit the plot as the user's mouse moves/drags over it,
                // which looks very very glitchy (#490)
                plot::drag_line_x(
                    10,
                    &v,
                    OSCColors::scrub_current(),
                    1.0f,
                    {plot::DragToolFlag::NoInputs, plot::DragToolFlag::NoFit});
            }

            // also, draw an X tag on the axes where the coordinate's value currently is
            plot::tag_x(coordinate_x_in_degrees, Color::white());

            // if it's possible to edit the coordinate via the plot, draw faded vertial drop line
            // where the mouse currently is
            if (maybe_mouse_x) {
                double v = *maybe_mouse_x;

                // CARE: this drag line shouldn't cause the plotter to re-fit because it will
                // make the plotter re-fit the plot as the user's mouse moves/drags over it,
                // which looks very very glitchy (#490)
                plot::drag_line_x(
                    11,
                    &v,
                    OSCColors::scrub_hovered(),
                    1.0f,
                    {plot::DragToolFlag::NoInputs, plot::DragToolFlag::NoFit});

                // also, draw a faded X tag on the axes where the mouse currently is (in X)
                plot::tag_x(*maybe_mouse_x, Color::white().with_alpha(0.6f));
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
                    std::optional<float> maybe_coordinate_y = compute_ler_ped_y(
                        lines_->get_active_plot(), static_cast<float>(coordinate_x_in_degrees));

                    if (maybe_coordinate_y) {
                        double v = *maybe_coordinate_y;

                        // CARE: this drag line shouldn't cause the plotter to re-fit because it
                        // will make the plotter re-fit the plot as the user's mouse moves/drags
                        // over it, which looks very very glitchy (#490)
                        plot::drag_line_y(
                            13,
                            &v,
                            OSCColors::scrub_current(),
                            1.0f,
                            {plot::DragToolFlag::NoInputs, plot::DragToolFlag::NoFit});
                        plot::draw_annotation(
                            {coordinate_x_in_degrees, *maybe_coordinate_y},
                            Color::white(),
                            {10.0f, 10.0f},
                            true,
                            "%f",
                            *maybe_coordinate_y);
                    }
                }

                // (try to) draw the hovered coordinate value as a faded dropline
                if (maybe_mouse_x) {
                    const std::optional<float> maybe_hover_y =
                        compute_ler_ped_y(lines_->get_active_plot(), *maybe_mouse_x);
                    if (maybe_hover_y) {
                        double v = *maybe_hover_y;

                        // CARE: this drag line shouldn't cause the plotter to re-fit because it
                        // will make the plotter re-fit the plot as the user's mouse moves/drags
                        // over it, which looks very very glitchy (#490)
                        plot::drag_line_y(
                            14,
                            &v,
                            OSCColors::scrub_hovered(),
                            1.0f,
                            {plot::DragToolFlag::NoInputs, plot::DragToolFlag::NoFit});
                        plot::draw_annotation(
                            {*maybe_mouse_x, *maybe_hover_y},
                            Color::white().with_alpha(0.6f),
                            {10.0f, 10.0f},
                            true,
                            "%f",
                            *maybe_hover_y);
                    }
                }
            }
        }

        void handle_mouse_events(
            const OpenSim::Coordinate& coord, std::optional<float> maybe_mouse_x)
        {
            // if the plot is hovered and the user is holding their left-mouse button down,
            // then "scrub" through the output in the model
            //
            // this is handy for users to visually see how the independent variable affects the
            // model
            if (maybe_mouse_x and ui::is_mouse_down(MouseButton::Left)) {
                if (is_model_locked()) {
                    ui::draw_tooltip("scrubbing disabled", "the model is locked");
                }
                if (coord.getDefaultLocked()) {
                    ui::draw_tooltip(
                        "scrubbing disabled",
                        "you cannot scrub this plot because the coordinate is locked");
                } else {
                    const double stored_value =
                        opyn::convert_coord_display_value_to_storage_value(coord, *maybe_mouse_x);
                    action_set_coordinate_value(upd_shared().upd_model(), coord, stored_value);
                }
            }

            // when the user stops dragging their left-mouse around, commit the scrubbed-to
            // coordinate to model storage
            if (maybe_mouse_x && ui::is_mouse_released(MouseButton::Left)) {
                if (is_model_locked()) {
                    ui::draw_tooltip("scrubbing disabled", "the model is locked");
                }
                if (coord.getDefaultLocked()) {
                    ui::draw_tooltip(
                        "scrubbing disabled",
                        "you cannot scrub this plot because the coordinate is locked");
                } else {
                    const double stored_value =
                        opyn::convert_coord_display_value_to_storage_value(coord, *maybe_mouse_x);
                    action_set_coordinate_value_and_save(
                        upd_shared().upd_model(), coord, stored_value);

                    // trick: we "know" that the last edit to the model was a coordinate edit in
                    // this plot's
                    //        independent variable, so we can skip recomputing it
                    const ModelStateCommit commit_after =
                        get_shared().get_model().get_latest_commit();
                    lines_->set_active_plot_commit(commit_after);
                }
            }
        }

        // try to draw a context menu with helpful options (set num data points, export, etc.)
        void try_draw_general_plot_popup(
            const OpenSim::Coordinate& coord,
            const std::string& plot_title,
            ui::PopupFlags flags = ui::PopupFlag::MouseButtonRight)
        {
            if (ui::begin_popup_context_menu(plot_title + "_contextmenu", flags)) {
                draw_general_plot_popup_content(coord);
                ui::end_popup();
            }
        }

        // draw the content of the general context menu (assume it's already open)
        void draw_general_plot_popup_content(const OpenSim::Coordinate& coord)
        {
            draw_plot_output_selector_combo();
            draw_max_datapoints_int_input();
            draw_max_history_entries_int_input();

            if (ui::draw_menu_item("clear unlocked plots")) {
                lines_->clear_unlocked_plots();
            }

            if (ui::begin_menu("legend")) {
                draw_legend_context_menu_content();
                ui::end_menu();
            }

            ui::draw_menu_item("show markers on active plot", {}, &show_markers_on_active_plot_);
            ui::draw_menu_item("show markers on other plots", {}, &show_markers_on_other_plots_);
            ui::draw_menu_item("snap cursor to datapoints", {}, &snap_cursor_);

            if (ui::draw_menu_item("duplicate plot")) {
                action_duplicate_current_plot_into_new_panel(coord);
            }

            if (ui::draw_menu_item("import CSV overlay(s)")) {
                action_prompt_user_for_csv_overlay_file(lines_);
            }
            ui::draw_tooltip_if_item_hovered(
                "import CSV overlay(s)",
                "Imports the specified CSV file as an overlay over the current plot. This is handy fitting muscle curves against externally-supplied data.\n\nThe provided CSV file must contain a header row and at least two columns of numeric data on each data row. The values in the columns must match this plot's axes.");

            if (ui::begin_menu("export CSV")) {
                draw_export_csv_menu_content(coord);
                ui::end_menu();
            }
        }

        // draws an input for manipulating the number of datapoints that should be plotted
        void draw_max_datapoints_int_input()
        {
            int current_data_points = get_shared().get_num_requested_datapoints();
            ui::draw_int_input("num data points", &current_data_points, 0, 0);
            if (ui::is_item_deactivated_after_edit()) {
                if (current_data_points >= 0) {
                    upd_shared().set_num_requested_data_points(current_data_points);
                }
            }
        }

        // draws an input for manipulating the number of history entries this plot panel holds
        void draw_max_history_entries_int_input()
        {
            int max_history_entries = lines_->get_max_history_entries();
            ui::draw_int_input("max history size", &max_history_entries, 0, 0);
            if (ui::is_item_deactivated_after_edit()) {
                if (max_history_entries >= 0) {
                    lines_->set_max_history_entries(max_history_entries);
                }
            }
        }

        // draws a combo that lets the user change which output of the
        // current component is plotted
        void draw_plot_output_selector_combo()
        {
            const auto& available_outputs = get_shared().available_outputs();

            std::vector<CStringView> names;
            names.reserve(available_outputs.size());

            size_t active = 0;
            const PlottableOutput current_output = get_shared().get_plotted_output();

            for (size_t i = 0; i < available_outputs.size(); ++i) {
                const PlottableOutput& o = available_outputs[i];
                names.push_back(o.get_name());
                if (o == current_output) {
                    active = i;
                }
            }

            if (ui::draw_combobox("output", &active, names)) {
                upd_shared().set_plotted_output(available_outputs[active]);
            }
        }

        // draws the content of a menu for manipulating the plot's legend
        void draw_legend_context_menu_content()
        {
            // draw hide legend button
            {
                bool v = plot_flags_ & plot::PlotFlags::NoLegend;
                if (ui::draw_checkbox("Hide", &v)) {
                    plot_flags_ = plot_flags_ ^ plot::PlotFlags::NoLegend;
                }
            }

            // draw outside/inside legend button
            {
                bool v = legend_flags_ & plot::LegendFlags::Outside;
                if (ui::draw_checkbox("Outside", &v)) {
                    legend_flags_ = legend_flags_ ^ plot::LegendFlags::Outside;
                }
            }

            const float s = ui::get_frame_height();
            const Vector2 dims{1.5f * s, s};

            ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 2.0f});
            if (ui::draw_button("NW", dims)) {
                legend_location_ = plot::Location::NorthWest;
            }
            ui::same_line();
            if (ui::draw_button("N", dims)) {
                legend_location_ = plot::Location::North;
            }
            ui::same_line();
            if (ui::draw_button("NE", dims)) {
                legend_location_ = plot::Location::NorthEast;
            }
            if (ui::draw_button("W", dims)) {
                legend_location_ = plot::Location::West;
            }
            ui::same_line();
            if (ui::draw_invisible_button("C", dims)) {
                legend_location_ = plot::Location::Center;
            }
            ui::same_line();
            if (ui::draw_button("E", dims)) {
                legend_location_ = plot::Location::East;
            }
            if (ui::draw_button("SW", dims)) {
                legend_location_ = plot::Location::SouthWest;
            }
            ui::same_line();
            if (ui::draw_button("S", dims)) {
                legend_location_ = plot::Location::South;
            }
            ui::same_line();
            if (ui::draw_button("SE", dims)) {
                legend_location_ = plot::Location::SouthEast;
            }
            ui::pop_style_var();
        }

        // draws the content of a menu for exporting plot data to a CSV
        void draw_export_csv_menu_content(const OpenSim::Coordinate& coord)
        {
            int id = 0;

            for (size_t i = 0; i < lines_->get_num_other_plots(); ++i) {
                ui::push_id(id++);
                if (ui::draw_menu_item(lines_->get_other_plot(i).get_name())) {
                    action_prompt_user_to_save_plot_to_csv(
                        coord, get_shared().get_plot_params(), lines_->get_other_plot(i));
                }
                ui::pop_id();
            }

            ui::push_id(id++);
            if (ui::draw_menu_item(lines_->get_active_plot().get_name())) {
                action_prompt_user_to_save_plot_to_csv(
                    coord, get_shared().get_plot_params(), lines_->get_active_plot());
            }
            ui::pop_id();

            ui::draw_separator();

            ui::push_id(id++);
            if (ui::draw_menu_item("Export All Curves")) {
                action_prompt_user_to_save_plot_lines_to_csv(
                    coord, get_shared().get_plot_params(), *lines_);
            }
            ui::draw_tooltip_if_item_hovered(
                "Export All Curves to CSV",
                "Exports all curves in the plot to a CSV file.\n\nThe implementation will try to group things together by X value, but the CSV file *may* contain sparse rows if (e.g.) some curves have a different number of plot points, or some curves were loaded from another CSV, etc.");
            ui::pop_id();
        }

        // tries to duplicate the current plot (settings etc.) into a new plot panel
        void action_duplicate_current_plot_into_new_panel(const OpenSim::Coordinate& coord)
        {
            const auto* musc = opyn::find_component<OpenSim::Muscle>(
                get_shared().get_model().get_model(),
                get_shared().get_plot_params().get_muscle_path());
            if (musc) {
                App::post_event<AddMusclePlotEvent>(upd_shared().get_parent_widget(), coord, *musc);
            }
        }

        // plot data state
        std::shared_ptr<PlotLines> lines_;  // Initialized in ctor.

        // UI/drawing/widget state
        Color computed_plot_line_base_color_ = Color::white();
        bool legend_popup_is_open_ = false;
        bool show_markers_on_active_plot_ = true;
        bool show_markers_on_other_plots_ = false;
        bool snap_cursor_ = false;
        plot::PlotFlags plot_flags_ = plot::PlotFlags::NoMenus |      //
                                      plot::PlotFlags::NoBoxSelect |  //
                                      plot::PlotFlags::NoFrame |      //
                                      plot::PlotFlags::NoTitle;
        plot::Location legend_location_ = plot::Location::NorthWest;
        plot::LegendFlags legend_flags_ = plot::LegendFlags::Default;
        Color locked_curve_tint_ = {0.5f, 0.5f, 1.0f, 1.1f};
        Color loaded_curve_tint_ = {0.5f, 1.0f, 0.5f, 1.0f};
    };
}

// other states
namespace
{
    // state in which a user is being prompted to select a coordinate in the model
    class PickCoordinateState final : public MusclePlotState {
    public:
        explicit PickCoordinateState(SharedStateData& shared) :
            MusclePlotState{shared}
        {
            // this is what this state is populating
            upd_shared().upd_plot_params().set_coordinate_path(OpenSim::ComponentPath{});
        }

    private:
        std::unique_ptr<MusclePlotState> impl_on_draw() override
        {
            std::unique_ptr<MusclePlotState> rv;

            std::vector<const OpenSim::Coordinate*> coordinates;
            for (const OpenSim::Coordinate& coord :
                 get_shared().get_model().get_model().getComponentList<OpenSim::Coordinate>()) {

                coordinates.push_back(&coord);
            }
            rgs::sort(coordinates, rgs::less{}, [](const auto* ptr) { return ptr->getName(); });

            ui::draw_text("select coordinate:");

            ui::begin_child_panel("MomentArmPlotCoordinateSelection");
            for (const OpenSim::Coordinate* coord : coordinates) {
                if (ui::draw_selectable(coord->getName())) {
                    upd_shared().upd_plot_params().set_coordinate_path(
                        opyn::get_absolute_path(*coord));
                    rv = std::make_unique<ShowingPlotState>(upd_shared());
                }
            }
            ui::end_child_panel();

            return rv;
        }
    };

    // state in which a user is being prompted to select a muscle in the model
    class PickMuscleState final : public MusclePlotState {
    public:
        explicit PickMuscleState(SharedStateData& shared) :
            MusclePlotState{shared}
        {
            // this is what this state is populating
            upd_shared().upd_plot_params().set_muscle_path(OpenSim::ComponentPath{});
        }

    private:
        std::unique_ptr<MusclePlotState> impl_on_draw() override
        {
            std::unique_ptr<MusclePlotState> rv;

            std::vector<const OpenSim::Muscle*> muscles;
            for (const OpenSim::Muscle& musc :
                 get_shared().get_model().get_model().getComponentList<OpenSim::Muscle>()) {

                muscles.push_back(&musc);
            }
            rgs::sort(muscles, rgs::less{}, [](const auto* ptr) { return ptr->getName(); });

            ui::draw_text("select muscle:");

            if (muscles.empty()) {
                ui::draw_text_disabled("(the model contains no muscles?)");
            } else {
                ui::begin_child_panel("MomentArmPlotMuscleSelection");
                for (const OpenSim::Muscle* musc : muscles) {
                    if (ui::draw_selectable(musc->getName())) {
                        upd_shared().upd_plot_params().set_muscle_path(
                            opyn::get_absolute_path(*musc));
                        rv = std::make_unique<PickCoordinateState>(upd_shared());
                    }
                }
                ui::end_child_panel();
            }

            return rv;
        }
    };
}

// private IMPL for the muscle plot panel
//
// this effectively operates as a state-machine host, where each state (e.g.
// "choose a muscle", "choose a coordinate") is mostly independent
class osc::ModelMusclePlotPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        ModelMusclePlotPanel& owner,
        Widget* parent,
        std::shared_ptr<UndoableModelStatePair> uim,
        std::string_view panel_name) :

        PanelPrivate{owner, parent, panel_name},
        shared_data_{*parent, std::move(uim)},
        active_state_{std::make_unique<PickMuscleState>(shared_data_)}
    {}

    explicit Impl(
        ModelMusclePlotPanel& owner,
        Widget* parent,
        std::shared_ptr<UndoableModelStatePair> uim,
        std::string_view panel_name,
        const OpenSim::ComponentPath& coord_path,
        const OpenSim::ComponentPath& muscle_path) :

        PanelPrivate{owner, parent, panel_name},
        shared_data_{*parent, std::move(uim), coord_path, muscle_path},
        active_state_{std::make_unique<ShowingPlotState>(shared_data_)}
    {}

    void draw_content()
    {
        if (auto maybe_next_state = active_state_->on_draw()) {
            active_state_ = std::move(maybe_next_state);
        }
    }

private:
    // data that's shared between all states
    SharedStateData shared_data_;

    // currently active state (this class controls a state machine)
    std::unique_ptr<MusclePlotState> active_state_;
};

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(
    Widget* parent, std::shared_ptr<UndoableModelStatePair> uim, std::string_view panel_name) :

    Panel{std::make_unique<Impl>(*this, parent, std::move(uim), panel_name)}
{}
osc::ModelMusclePlotPanel::ModelMusclePlotPanel(
    Widget* parent,
    std::shared_ptr<UndoableModelStatePair> uim,
    std::string_view panel_name,
    const OpenSim::ComponentPath& coord_path,
    const OpenSim::ComponentPath& muscle_path) :

    Panel{
        std::make_unique<Impl>(*this, parent, std::move(uim), panel_name, coord_path, muscle_path)}
{}
void osc::ModelMusclePlotPanel::impl_draw_content()
{
    private_data().draw_content();
}
