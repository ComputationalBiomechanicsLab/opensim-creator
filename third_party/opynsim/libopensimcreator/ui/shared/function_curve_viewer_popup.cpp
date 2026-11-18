#include "function_curve_viewer_popup.h"

#include <libopynsim/documents/model/versioned_component_accessor.h>
#include <liboscar/formats/csv.h>
#include <liboscar/maths/closed_interval.h>
#include <liboscar/maths/constants.h>
#include <liboscar/platform/app.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/algorithms.h>
#include <OpenSim/Common/Function.h>

#include <array>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;
namespace plot = osc::ui::plot;

class osc::FunctionCurveViewerPanel::Impl final : public PanelPrivate {
public:
    explicit Impl(
        FunctionCurveViewerPanel& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<const opyn::VersionedComponentAccessor> target_component,
        std::function<const OpenSim::Function*()> function_getter) :

        PanelPrivate{owner, parent, popup_name, ui::PanelFlag::AlwaysAutoResize},
        component_{std::move(target_component)},
        function_getter_{std::move(function_getter)}
    {}

private:
    class FunctionParameters final {
    public:
        explicit FunctionParameters(const opyn::VersionedComponentAccessor& component) :
            component_version_{component.get_component_version()}
        {}

        friend bool operator==(const FunctionParameters&, const FunctionParameters&) = default;

        void set_version_from_component(const opyn::VersionedComponentAccessor& component)
        {
            component_version_ = component.get_component_version();
        }

        ClosedInterval<float> get_input_range() const { return input_range_; }
        ClosedInterval<float>& upd_input_range() { return input_range_; }
        int get_num_points() const { return num_points_; }
        int& upd_num_points() { return num_points_; }

    private:
        UID component_version_;
        ClosedInterval<float> input_range_ = {-1.0f, 1.0f};
        int num_points_ = 100;
    };

    class PlotPoints final {
    public:
        using value_type = Vector2;
        using size_type = std::vector<value_type>::size_type;
        using const_reference = std::vector<value_type>::const_reference;

        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }

        [[nodiscard]] bool empty() const { return data_.empty(); }
        size_type size() const { return data_.size(); }
        const_reference front() const { return data_.front(); }
        ClosedInterval<float> x_range() const { return x_range_; }
        ClosedInterval<float> y_range() const { return y_range_; }

        void reserve(size_type new_cap) { data_.reserve(new_cap); }

        template<typename... Args>
        requires std::constructible_from<Vector2, Args&&...>
        void emplace_back(Args&&... args)
        {
            const Vector2& v = data_.emplace_back(std::forward<Args>(args)...);

            // update X-/Y-range
            x_range_.lower = min(v.x(), x_range_.lower);
            x_range_.upper = max(v.x(), x_range_.upper);
            y_range_.lower = min(v.y(), y_range_.lower);
            y_range_.upper = max(v.y(), y_range_.upper);
        }

    private:
        std::vector<value_type> data_;
        ClosedInterval<float> x_range_ = {quiet_nan_v<float>, quiet_nan_v<float>};
        ClosedInterval<float> y_range_ = {quiet_nan_v<float>, quiet_nan_v<float>};
    };

public:
    void draw_content()
    {
        // update parameter state and check if replotting is necessary
        latest_parameters_.set_version_from_component(*component_);
        if (latest_parameters_ != plotted_parameters_) {
            plotted_parameters_ = latest_parameters_;
            plot_points_ = generate_plot_points(latest_parameters_);
        }

        draw_top_editors();
        draw_plot();
        if (error_) {
            ui::draw_text_wrapped(*error_);
        }
    }

private:
    void draw_top_editors()
    {
        ui::draw_float_input("min x", &latest_parameters_.upd_input_range().lower);
        ui::draw_float_input("max x", &latest_parameters_.upd_input_range().upper);
        if (ui::draw_int_input("num points", &latest_parameters_.upd_num_points())) {
            // sanity
            latest_parameters_.upd_num_points() =
                clamp(latest_parameters_.upd_num_points(), 0, 10000);
        }
        if (ui::draw_button("export CSV")) {
            on_user_requested_csv_export();
        }
    }

    void draw_plot()
    {
        if (plot_points_.empty()) {
            return;  // don't try to plot null data etc.
        }

        const Vector2 dimensions = Vector2{ui::get_content_region_available().x()};
        const plot::PlotFlags flags = plot::PlotFlags::NoMenus |      //
                                      plot::PlotFlags::NoBoxSelect |  //
                                      plot::PlotFlags::NoFrame |      //
                                      plot::PlotFlags::NoTitle;
        if (plot::begin(name(), dimensions, flags)) {

            plot::setup_axes("x", "y");
            plot::setup_axis_limits(
                plot::Axis::X1, plot_points_.x_range(), 0.05f, plot::Condition::Always);
            plot::setup_axis_limits(
                plot::Axis::Y1, plot_points_.y_range(), 0.05f, plot::Condition::Always);
            plot::setup_finish();

            plot::set_next_marker_style(plot::MarkerType::Circle, 2.0f);
            plot::push_style_color(plot::PlotColorVar::Line, Color::white());
            plot::plot_line("Function Output", plot_points_);
            plot::pop_style_color();

            plot::end();
        }
    }

    PlotPoints generate_plot_points(const FunctionParameters& params)
    {
        PlotPoints rv;

        const OpenSim::Function* function = function_getter_();
        if (not function) {
            error_ =
                "could not get the function from the component (maybe the component was edited, or the function was deleted?)";
            return rv;
        }

        error_.reset();

        rv.reserve(params.get_num_points());
        const double step_size = params.get_input_range().step_size(params.get_num_points());
        SimTK::Vector x(1);
        try {
            for (int step = 0; step < params.get_num_points(); ++step) {
                x[0] = params.get_input_range().lower + step_size * step;
                const double y = function->calcValue(x);
                rv.emplace_back(x[0], y);
            }
        } catch (const std::exception& ex) {
            error_ = ex.what();
            rv = {};
            return rv;
        }

        return rv;
    }

    void on_user_requested_csv_export()
    {
        App::upd().prompt_user_to_save_file_with_extension_async(
            [points = plot_points_](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                std::ofstream ostream{*p};
                if (not ostream) {
                    return;  // error opening the output file for writing
                }

                // write header
                CSV::write_row(ostream, std::to_array<std::string>({"x", "y"}));

                // write data rows
                for (const auto& [x, y] : points) {
                    CSV::write_row(ostream, std::to_array({std::to_string(x), std::to_string(y)}));
                }
            },
            "csv");
    }

    std::shared_ptr<const opyn::VersionedComponentAccessor> component_;
    std::function<const OpenSim::Function*()> function_getter_;
    FunctionParameters latest_parameters_{*component_};
    std::optional<FunctionParameters> plotted_parameters_;
    PlotPoints plot_points_;
    std::optional<std::string> error_;
};

osc::FunctionCurveViewerPanel::FunctionCurveViewerPanel(
    Widget* parent,
    std::string_view panel_name,
    std::shared_ptr<const opyn::VersionedComponentAccessor> target_component,
    std::function<const OpenSim::Function*()> function_getter) :

    Panel{std::make_unique<Impl>(
        *this, parent, panel_name, std::move(target_component), std::move(function_getter))}
{}
void osc::FunctionCurveViewerPanel::impl_draw_content()
{
    private_data().draw_content();
}
