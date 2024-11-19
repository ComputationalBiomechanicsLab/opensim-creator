#include "FunctionCurveViewerPopup.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>

#include <OpenSim/Common/Function.h>
#include <oscar/Formats/CSV.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Panels/PanelPrivate.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Algorithms.h>

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
    Impl(
        FunctionCurveViewerPanel& owner,
        std::string_view popupName,
        std::shared_ptr<const IModelStatePair> targetModel,
        std::function<const OpenSim::Function*()> functionGetter) :

        PanelPrivate{owner, nullptr, popupName, ui::WindowFlag::AlwaysAutoResize},
        m_Model{std::move(targetModel)},
        m_FunctionGetter{std::move(functionGetter)}
    {}
private:
    class FunctionParameters final {
    public:
        explicit FunctionParameters(const IModelStatePair& model) :
            modelVerison{model.getModelVersion()},
            stateVersion{model.getStateVersion()}
        {}

        friend bool operator==(const FunctionParameters& lhs, const FunctionParameters& rhs) = default;

        void setVersionFromModel(const IModelStatePair& model)
        {
            modelVerison = model.getModelVersion();
            stateVersion = model.getStateVersion();
        }

        ClosedInterval<float> getInputRange() const { return inputRange; }
        ClosedInterval<float>& updInputRange() { return inputRange; }
        int getNumPoints() const { return numPoints; }
        int& updNumPoints() { return numPoints; }

    private:
        UID modelVerison;
        UID stateVersion;
        ClosedInterval<float> inputRange = {-1.0f, 1.0f};
        int numPoints = 100;
    };

    class PlotPoints final {
    public:
        using value_type = Vec2;
        using size_type = std::vector<value_type>::size_type;
        using const_reference = std::vector<value_type>::const_reference;

        auto begin() const { return m_Data.begin(); }
        auto end() const { return m_Data.end(); }

        [[nodiscard]] bool empty() const { return m_Data.empty(); }
        size_type size() const { return m_Data.size(); }
        const_reference front() const { return m_Data.front(); }
        ClosedInterval<float> x_range() const { return m_XRange; }
        ClosedInterval<float> y_range() const { return m_YRange; }

        void reserve(size_type new_cap) { m_Data.reserve(new_cap); }

        template<typename... Args>
        requires std::constructible_from<Vec2, Args&&...>
        void emplace_back(Args&&... args)
        {
            const Vec2& v = m_Data.emplace_back(std::forward<Args>(args)...);

            // update X-/Y-range
            m_XRange.lower = min(v.x, m_XRange.lower);
            m_XRange.upper = max(v.x, m_XRange.upper);
            m_YRange.lower = min(v.y, m_YRange.lower);
            m_YRange.upper = max(v.y, m_YRange.upper);
        }

    private:
        std::vector<value_type> m_Data;
        ClosedInterval<float> m_XRange = {quiet_nan_v<float>, quiet_nan_v<float>};
        ClosedInterval<float> m_YRange = {quiet_nan_v<float>, quiet_nan_v<float>};
    };

public:
    void draw_content()
    {
        // update parameter state and check if replotting is necessary
        m_LatestParameters.setVersionFromModel(*m_Model);
        if (m_LatestParameters != m_PlottedParameters) {
            m_PlottedParameters = m_LatestParameters;
            m_PlotPoints = generatePlotPoints(m_LatestParameters);
        }

        drawTopEditors();
        drawPlot();
        if (m_Error) {
            ui::draw_text_wrapped(*m_Error);
        }
    }

private:
    void drawTopEditors()
    {
        ui::draw_float_input("min x", &m_LatestParameters.updInputRange().lower);
        ui::draw_float_input("max x", &m_LatestParameters.updInputRange().upper);
        if (ui::draw_int_input("num points", &m_LatestParameters.updNumPoints())) {
            m_LatestParameters.updNumPoints() = clamp(m_LatestParameters.updNumPoints(), 0, 10000);  // sanity
        }
        if (ui::draw_button("export CSV")) {
            onUserRequestedCSVExport();
        }
    }

    void drawPlot()
    {
        if (m_PlotPoints.empty()) {
            return;  // don't try to plot null data etc.
        }

        const Vec2 dimensions = Vec2{ui::get_content_region_available().x};
        const plot::PlotFlags flags = plot::PlotFlags::NoMenus | plot::PlotFlags::NoBoxSelect | plot::PlotFlags::NoFrame | plot::PlotFlags::NoTitle;
        if (plot::begin(name(), dimensions, flags)) {

            plot::setup_axes("x", "y");
            plot::setup_axis_limits(plot::Axis::X1, m_PlotPoints.x_range(), 0.05f, plot::Condition::Always);
            plot::setup_axis_limits(plot::Axis::Y1, m_PlotPoints.y_range(), 0.05f, plot::Condition::Always);
            plot::setup_finish();

            plot::set_next_marker_style(plot::MarkerType::Circle, 2.0f);
            plot::push_style_color(plot::PlotColorVar::Line, Color::white());
            plot::plot_line("Function Output", m_PlotPoints);
            plot::pop_style_color();

            plot::end();
        }
    }

    PlotPoints generatePlotPoints(const FunctionParameters& params)
    {
        PlotPoints rv;

        const OpenSim::Function* function = m_FunctionGetter();
        if (not function) {
            m_Error = "could not get the function from the model (maybe the model was edited, or the function was deleted?)";
            return rv;
        }

        m_Error.reset();

        rv.reserve(params.getNumPoints());
        const double stepSize = params.getInputRange().step_size(params.getNumPoints());
        SimTK::Vector x(1);
        try {
            for (int step = 0; step < params.getNumPoints(); ++step) {
                x[0] = params.getInputRange().lower + stepSize*step;
                const double y = function->calcValue(x);
                rv.emplace_back(x[0], y);
            }
        }
        catch (const std::exception& ex) {
            m_Error = ex.what();
            rv = {};
            return rv;
        }

        return rv;
    }

    void onUserRequestedCSVExport()
    {
        const auto csvPath = prompt_user_for_file_save_location_add_extension_if_necessary("csv");
        if (not csvPath) {
            return;  // user probably cancelled out
        }

        std::ofstream ostream{*csvPath};
        if (not ostream) {
            return;  // error opening the output file for writing
        }

        // write header
        write_csv_row(ostream, std::to_array<std::string>({ "x", "y" }));

        // write data rows
        for (const auto& [x, y] : m_PlotPoints) {
            write_csv_row(ostream, std::to_array({ std::to_string(x), std::to_string(y) }));
        }
    }

    std::shared_ptr<const IModelStatePair> m_Model;
    std::function<const OpenSim::Function*()> m_FunctionGetter;
    FunctionParameters m_LatestParameters{*m_Model};
    std::optional<FunctionParameters> m_PlottedParameters;
    PlotPoints m_PlotPoints;
    std::optional<std::string> m_Error;
};

osc::FunctionCurveViewerPanel::FunctionCurveViewerPanel(
    std::string_view panelName,
    std::shared_ptr<const IModelStatePair> targetModel,
    std::function<const OpenSim::Function*()> functionGetter) :

    Panel{std::make_unique<Impl>(*this, panelName, std::move(targetModel), std::move(functionGetter))}
{}
void osc::FunctionCurveViewerPanel::impl_draw_content() { private_data().draw_content(); }
