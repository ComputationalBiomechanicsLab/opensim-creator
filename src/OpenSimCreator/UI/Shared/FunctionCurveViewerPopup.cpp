#include "FunctionCurveViewerPopup.h"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>

#include <OpenSim/Common/Function.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/UI/oscimgui.h>

#include <functional>
#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

class osc::FunctionCurveViewerPopup::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName,
        std::shared_ptr<const IConstModelStatePair> targetModel,
        std::function<const OpenSim::Function*()> functionGetter) :

        StandardPopup{popupName, {768.0f, 0.0f}, ImGuiWindowFlags_AlwaysAutoResize},
        m_TargetModel{std::move(targetModel)},
        m_FunctionGetter{std::move(functionGetter)}
    {}
private:
    struct FunctionParameters final {
        friend bool operator==(const FunctionParameters& lhs, const FunctionParameters& rhs) = default;

        UID modelVerison;
        UID stateVersion;
        ClosedInterval<float> inputRange = {-1.0f, 1.0f};
        int numPoints = 100;
    };

    void impl_draw_content() final
    {
        // update parameter state and check if replotting is necessary
        m_LatestParameters.modelVerison = m_TargetModel->getModelVersion();
        m_LatestParameters.stateVersion = m_TargetModel->getStateVersion();
        if (m_LatestParameters != m_PlottedParameters) {
            m_PlottedParameters = m_LatestParameters;
            m_PlotPoints = generatePlotPoints(m_LatestParameters);
        }

        drawTopEditors();
        drawPlot();
        drawBottomButtons();
    }

    void drawTopEditors()
    {
        ui::draw_float_input("min", &m_LatestParameters.inputRange.lower);
        ui::draw_float_input("max", &m_LatestParameters.inputRange.upper);
        if (ui::draw_int_input("num points", &m_LatestParameters.numPoints)) {
            m_LatestParameters.numPoints = clamp(m_LatestParameters.numPoints, 0, 10000);  // sanity
        }
    }

    void drawPlot()
    {
        if (m_PlotPoints.empty()) {
            return;  // don't try to plot null data etc.
        }

        const float availableWidth = ui::get_content_region_avail().x;
        const Vec2 plotDimensions{availableWidth};

        if (ImPlot::BeginPlot("Plot", plotDimensions)) {
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f);
            ImPlot::PlotLine(
                "Plot",
                &m_PlotPoints.front().x,
                &m_PlotPoints.front().y,
                static_cast<int>(m_PlotPoints.size()),
                0,
                0,
                sizeof(typename decltype(m_PlotPoints)::value_type)
            );
        }
    }

    void drawBottomButtons()
    {
        if (ui::draw_button("cancel")) {
            request_close();
        }
    }

    std::vector<Vec2> generatePlotPoints(const FunctionParameters& params)
    {
        std::vector<Vec2> rv;

        const OpenSim::Function* function = m_FunctionGetter();
        if (not function) {
            m_Error = "could not get the function from the model (maybe the model was edited, or the function was deleted?)";
            return rv;
        }

        rv.reserve(params.numPoints);
        const double stepSize = params.inputRange.step_size(params.numPoints);
        SimTK::Vector x(1);
        try {
            for (int step = 0; step < params.numPoints; ++step) {
                x[0] = params.inputRange.lower + stepSize*step;
                const double y = function->calcValue(x);
                rv.push_back({x[0], y});
            }
        }
        catch (const std::exception& ex) {
            m_Error = ex.what();
            rv = {};
            return rv;
        }

        return rv;
    }

    std::shared_ptr<const IConstModelStatePair> m_TargetModel;
    std::function<const OpenSim::Function*()> m_FunctionGetter;
    FunctionParameters m_LatestParameters = {
        .modelVerison = m_TargetModel->getModelVersion(),
        .stateVersion = m_TargetModel->getStateVersion(),
    };
    std::optional<FunctionParameters> m_PlottedParameters;
    std::vector<Vec2> m_PlotPoints;
    std::optional<std::string> m_Error;
};


osc::FunctionCurveViewerPopup::FunctionCurveViewerPopup(
    std::string_view popupName,
    std::shared_ptr<const IConstModelStatePair> targetModel,
    std::function<const OpenSim::Function*()> functionGetter) :

    m_Impl{std::make_unique<Impl>(popupName, std::move(targetModel), std::move(functionGetter))}
{}
osc::FunctionCurveViewerPopup::FunctionCurveViewerPopup(FunctionCurveViewerPopup&&) noexcept = default;
osc::FunctionCurveViewerPopup& osc::FunctionCurveViewerPopup::operator=(FunctionCurveViewerPopup&&) noexcept = default;
osc::FunctionCurveViewerPopup::~FunctionCurveViewerPopup() noexcept = default;

bool osc::FunctionCurveViewerPopup::impl_is_open() const
{
    return m_Impl->is_open();
}
void osc::FunctionCurveViewerPopup::impl_open()
{
    m_Impl->open();
}
void osc::FunctionCurveViewerPopup::impl_close()
{
    m_Impl->close();
}
bool osc::FunctionCurveViewerPopup::impl_begin_popup()
{
    return m_Impl->begin_popup();
}
void osc::FunctionCurveViewerPopup::impl_on_draw()
{
    m_Impl->on_draw();
}
void osc::FunctionCurveViewerPopup::impl_end_popup()
{
    m_Impl->end_popup();
}
