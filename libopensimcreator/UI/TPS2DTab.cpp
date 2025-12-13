#include "TPS2DTab.h"

#include <libopensimcreator/Platform/IconCodepoints.h>

#include <liboscar/Formats/SVG.h>
#include <liboscar/Graphics/Camera.h>
#include <liboscar/Graphics/Geometries/PlaneGeometry.h>
#include <liboscar/Graphics/Graphics.h>
#include <liboscar/Graphics/Materials/MeshBasicMaterial.h>
#include <liboscar/Graphics/Materials/MeshBasicTexturedMaterial.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/MatrixFunctions.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/LogViewerPanel.h>
#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/StringHelpers.h>
#include <simmath/LinearAlgebra.h>

#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;

// 2D TPS algorithm stuff
//
// most of the background behind this is discussed in issue #467. For redundancy's sake, here
// are some of the references used to write this implementation:
//
// - primary literature source: https://ieeexplore.ieee.org/document/24792
// - blog explanation: https://profs.etsmtl.ca/hlombaert/thinplates/
// - blog explanation #2: https://khanhha.github.io/posts/Thin-Plate-Splines-Warping/
namespace
{
    // a single source-to-destination landmark pair in 2D space
    //
    // this is typically what the user/caller defines
    struct LandmarkPair2D final {
        Vector2 src;
        Vector2 dest;
    };

    // this is effectviely the "U" term in the TPS algorithm literature (which is usually U(r) = r^2 * log(r^2))
    //
    // i.e. U(||pi - p||) in the literature is equivalent to `RadialBasisFunction2D(pi, p)` here
    float RadialBasisFunction2D(Vector2 controlPoint, Vector2 p)
    {
        const Vector2 diff = controlPoint - p;
        const float r2 = dot(diff, diff);

        if (r2 == 0.0f) {
            // this ensures that the result is always non-zero and non-NaN (this might be
            // necessary for some types of linear solvers?)
            return std::numeric_limits<float>::min();
        }
        else {
            return r2 * log(r2);
        }
    }

    // a single non-affine term of the 2D TPS equation
    //
    // i.e. in `f(p) = a1 + a2*p.x + a3*p.y + SUM{ wi * U(||controlPoint - p||) }` this encodes
    //      the `wi` and `controlPoint` parts of that equation
    struct TPSNonAffineTerm2D final {

        TPSNonAffineTerm2D(Vector2 weight_, Vector2 controlPoint_) :
            weight{weight_},
            controlPoint{controlPoint_}
        {}

        Vector2 weight;
        Vector2 controlPoint;
    };

    // all coefficients in the 2D TPS equation
    //
    // i.e. these are the a1, a2, a3, and w's (+ control points) terms of the equation
    struct TPSCoefficients2D final {
        Vector2 a1 = {0.0f, 0.0f};
        Vector2 a2 = {1.0f, 0.0f};
        Vector2 a3 = {0.0f, 1.0f};
        std::vector<TPSNonAffineTerm2D> weights;
    };

    // evaluates the TPS equation with the given coefficients and input point
    Vector2 Evaluate(const TPSCoefficients2D& coefs, Vector2 p)
    {
        // this implementation effectively evaluates both `fx(x, y)` and `fy(x, y)` at
        // the same time, because `TPSCoefficients2D` stores the X and Y variants of the
        // coefficients together in memory (as `Vector2`s)

        // compute affine terms (a1 + a2*x + a3*y)
        Vector2 rv = coefs.a1 + coefs.a2*p.x + coefs.a3*p.y;

        // accumulate non-affine terms (effectively: wi * U(||controlPoint - p||))
        for (const TPSNonAffineTerm2D& wt : coefs.weights) {
            rv += wt.weight * RadialBasisFunction2D(wt.controlPoint, p);
        }

        return rv;
    }

    // computes all coefficients of the TPS equation (a1, a2, a3, and all the w's)
    TPSCoefficients2D TPSCalcCoefficients(std::span<const LandmarkPair2D> landmarkPairs)
    {
        // this is based on the Bookstein Thin Plate Sline (TPS) warping algorithm
        //
        // 1. A TPS warp is (simplifying here) a linear combination:
        //
        //     f(p) = a1 + a2*p.x + a3*p.y + SUM{ wi * U(||controlPoint_i - p||) }
        //
        //    which can be represented as a matrix multiplication between the terms (1, p.x, p.y,
        //    U(||cpi - p||)) and the coefficients (a1, a2, a3, wi..)
        //
        // 2. The caller provides "landmark pairs": these are (effectively) the input
        //    arguments and the expected output
        //
        // 3. This algorithm uses the input + output to solve for the linear coefficients.
        //    Once those coefficients are known, we then have a linear equation that we
        //    we can pump new inputs into (e.g. mesh points, muscle points)
        //
        // 4. So, given the equation L * [w a] = [v o], where L is a matrix of linear terms,
        //    [w a] is a vector of the linear coefficients (we're solving for these), and [v o]
        //    is the expected output (v), with some (padding) zero elements (o)
        //
        // 5. Create matrix L:
        //
        //   |K  P|
        //   |PT 0|
        //
        //     where:
        //
        //     - K is a symmetric matrix of each *input* landmark pair evaluated via the
        //       basis function:
        //
        //        |U(p00) U(p01) U(p02)  ...  |
        //        |U(p10) U(p11) U(p12)  ...  |
        //        | ...    ...    ...   U(pnn)|
        //
        //     - P is a n-row 3-column matrix containing the number 1 (the constant term),
        //       x, and y (effectively, the p term):
        //
        //       |1 x1 y1|
        //       |1 x2 y2|
        //
        //     - PT is the transpose of P
        //     - 0 is the zero matrix (padding)
        //
        // 6. Use a linear solver to solve L * [w a] = [v o] to yield [w a]
        // 8. Return the coefficients, [w a]

        const int numPairs = static_cast<int>(landmarkPairs.size());

        if (numPairs == 0) {
            // edge-case: there are no pairs, so return an identity-like transform
            return TPSCoefficients2D{};
        }

        // construct matrix L
        SimTK::Matrix L(numPairs + 3, numPairs + 3);

        // populate the K part of matrix L (upper-left)
        for (int row = 0; row < numPairs; ++row)
        {
            for (int col = 0; col < numPairs; ++col) {
                const Vector2& pi_ = landmarkPairs[row].src;
                const Vector2& pj = landmarkPairs[col].src;

                L(row, col) = RadialBasisFunction2D(pi_, pj);
            }
        }

        // populate the P part of matrix L (upper-right)
        {
            const int pStartColumn = numPairs;

            for (int row = 0; row < numPairs; ++row) {
                L(row, pStartColumn)     = 1.0;
                L(row, pStartColumn + 1) = landmarkPairs[row].src.x;
                L(row, pStartColumn + 2) = landmarkPairs[row].src.y;
            }
        }

        // populate the PT part of matrix L (bottom-left)
        {
            const int ptStartRow = numPairs;

            for (int col = 0; col < numPairs; ++col) {
                L(ptStartRow, col)     = 1.0;
                L(ptStartRow + 1, col) = landmarkPairs[col].src.x;
                L(ptStartRow + 2, col) = landmarkPairs[col].src.y;
            }
        }

        // populate the 0 part of matrix L (bottom-right)
        {
            const int zeroStartRow = numPairs;
            const int zeroStartCol = numPairs;

            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    L(zeroStartRow + row, zeroStartCol + col) = 0.0;
                }
            }
        }

        // construct "result" vectors Vx and Vy (these hold the landmark destinations)
        SimTK::Vector Vx(numPairs + 3, 0.0);
        SimTK::Vector Vy(numPairs + 3, 0.0);
        for (int row = 0; row < numPairs; ++row) {
            Vx[row] = landmarkPairs[row].dest.x;
            Vy[row] = landmarkPairs[row].dest.y;
        }

        // construct coefficient vectors that will receive the solver's result
        SimTK::Vector Cx(numPairs + 3, 0.0);
        SimTK::Vector Cy(numPairs + 3, 0.0);

        // solve `L*Cx = Vx` and `L*Cy = Vy` for `Cx` and `Cy` (the coefficients)
        SimTK::FactorQTZ F(L);
        F.solve(Vx, Cx);
        F.solve(Vy, Cy);

        // the coefficient vectors now contain (e.g. for X): [w1, w2, ... wx, a0, a1x, a1y]
        //
        // extract them into the return value

        TPSCoefficients2D rv;

        // populate affine a1, a2, a3 terms
        rv.a1 = {Cx[numPairs],   Cy[numPairs]  };
        rv.a2 = {Cx[numPairs+1], Cy[numPairs+1]};
        rv.a3 = {Cx[numPairs+2], Cy[numPairs+2]};

        // populate `wi` coefficients (+ control points, needed at evaluation-time)
        rv.weights.reserve(numPairs);
        for (int i = 0; i < numPairs; ++i) {
            const Vector2 weight = {Cx[i], Cy[i]};
            const Vector2 controlPoint = landmarkPairs[i].src;
            rv.weights.emplace_back(weight, controlPoint);
        }

        return rv;
    }

    // a class that wraps the 2D TPS algorithm with a basic interface for transforming
    // points
    class ThinPlateWarper2D final {
    public:
        explicit ThinPlateWarper2D(std::span<const LandmarkPair2D> landmarkPairs) :
            m_Coefficients{TPSCalcCoefficients(landmarkPairs)}
        {}

        Vector2 transform(Vector2 p) const
        {
            return Evaluate(m_Coefficients, p);
        }

    private:
        TPSCoefficients2D m_Coefficients;
    };

    // returns a mesh that is the equivalent of applying the 2D TPS warp to all
    // vertices of the input mesh
    Mesh TPSWarpMesh(const ThinPlateWarper2D& t, const Mesh& mesh)
    {
        Mesh rv = mesh;
        rv.transform_vertices([&t](Vector3 v) { return Vector3{t.transform(Vector2{v}), v.z}; });
        return rv;
    }
}

// GUI stuff
namespace
{
    // holds the user's current mouse click state:
    //
    // - initial (the user did nothing with their mouse yet)
    // - first click (the user clicked the source of a landmark pair and the UI is waiting for the destination)
    struct GUIInitialMouseState final {};
    struct GUIFirstClickMouseState final { Vector2 srcNDCPos; };
    using GUIMouseState = std::variant<GUIInitialMouseState, GUIFirstClickMouseState>;
}

class osc::TPS2DTab::Impl final : public TabPrivate {
public:

    explicit Impl(TPS2DTab& owner, Widget* parent) :
        TabPrivate{owner, parent, OSC_ICON_BEZIER_CURVE " TPS2DTab"}
    {
        m_TexturedMaterial.set_texture(m_BoxTexture);
        wireframe_material_.set_color({0.0f, 0.0f, 0.0f, 0.15f});
        wireframe_material_.set_transparent(true);
        wireframe_material_.set_wireframe(true);
        wireframe_material_.set_depth_tested(false);
        m_Camera.set_view_matrix_override(identity<Matrix4x4>());
        m_Camera.set_projection_matrix_override(identity<Matrix4x4>());
        m_Camera.set_background_color(Color::white());
    }

    void onDraw()
    {
        ui::enable_dockspace_over_main_window();

        ui::begin_panel("Input");
        {
            const Vector2 windowDims = ui::get_content_region_available();
            const float minDim = min(windowDims.x, windowDims.y);
            const Vector2 texDims = {minDim, minDim};

            renderMesh(m_InputGrid, texDims, m_InputRender);

            // draw rendered texture via ImGui
            ui::draw_image(*m_InputRender, texDims);
            const ui::HittestResult ht = ui::hittest_last_drawn_item();

            // draw any 2D overlays etc.
            renderOverlayElements(ht);
            if (ht.is_hovered) {
                renderMouseUIElements(ht);
            }
        }

        ui::end_panel();

        Vector2 outputWindowPos;
        Vector2 outputWindowDims;
        ui::begin_panel("Output");
        {
            outputWindowPos = ui::get_cursor_ui_position();
            outputWindowDims = ui::get_content_region_available();
            const float minDim = min(outputWindowDims.x, outputWindowDims.y);
            const Vector2i texDims = Vector2i{minDim, minDim};

            {
                // apply blending factor, compute warp, apply to grid

                std::vector<LandmarkPair2D> pairs = m_LandmarkPairs;
                for (LandmarkPair2D& p : pairs) {
                    p.dest = lerp(p.src, p.dest, m_BlendingFactor);
                }
                ThinPlateWarper2D warper{pairs};
                m_OutputGrid = TPSWarpMesh(warper, m_InputGrid);
            }

            renderMesh(m_OutputGrid, texDims, m_OutputRender);

            // draw rendered texture via ImGui
            ui::draw_image(*m_OutputRender, texDims);
        }
        ui::end_panel();

        // draw scubber overlay
        {
            const float leftPadding = 10.0f;
            const float bottomPadding = 10.0f;
            const float panelHeight = 50.0f;
            ui::set_next_panel_ui_position({ outputWindowPos.x + leftPadding, outputWindowPos.y + outputWindowDims.y - panelHeight - bottomPadding });
            ui::set_next_panel_size({ outputWindowDims.x - leftPadding, panelHeight });
            ui::begin_panel("##scrubber", nullptr, ui::get_minimal_panel_flags().without(ui::PanelFlag::NoInputs));
            ui::set_next_item_width(ui::get_content_region_available().x);
            ui::draw_float_slider("##blend", &m_BlendingFactor, 0.0f, 1.0f);
            ui::end_panel();
        }

        // draw log panel (debugging)
        m_LogViewerPanel.on_draw();
    }


private:

    // render the given mesh as-is to the given output render texture
    void renderMesh(const Mesh& mesh, Vector2 dimensions, std::optional<RenderTexture>& out)
    {
        const RenderTextureParams textureParameters = {
            .pixel_dimensions = App::get().main_window_device_pixel_ratio() * dimensions,
            .device_pixel_ratio = App::get().main_window_device_pixel_ratio(),
            .anti_aliasing_level = App::get().anti_aliasing_level(),
        };
        out.emplace(textureParameters);
        graphics::draw(mesh, identity<Transform>(), m_TexturedMaterial, m_Camera);
        graphics::draw(mesh, identity<Transform>(), wireframe_material_, m_Camera);

        OSC_ASSERT(out.has_value());
        m_Camera.render_to(*out);

        OSC_ASSERT(out.has_value() && "the camera should've given the render texture back to the caller");
    }

    // render any 2D overlays
    void renderOverlayElements(const ui::HittestResult& ht)
    {
        ui::DrawListView drawlist = ui::get_panel_draw_list();

        // render all fully-established landmark pairs
        for (const LandmarkPair2D& p : m_LandmarkPairs) {
            const Vector2 p1 = ht.item_ui_rect.ypd_top_left() + (ht.item_ui_rect.dimensions() * ndc_point_to_topleft_normalized(p.src));
            const Vector2 p2 = ht.item_ui_rect.ypd_top_left() + (ht.item_ui_rect.dimensions() * ndc_point_to_topleft_normalized(p.dest));

            drawlist.add_line(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist.add_rect_filled(Rect::from_corners(p1 - 12.0f, p1 + 12.0f), m_SrcSquareColor);
            drawlist.add_circle_filled({.origin = p2, .radius = 10.0f}, m_DestCircleColor);
        }

        // render any currently-placing landmark pairs in a more-faded color
        if (ht.is_hovered and std::holds_alternative<GUIFirstClickMouseState>(m_MouseState)) {
            const GUIFirstClickMouseState& st = std::get<GUIFirstClickMouseState>(m_MouseState);

            const Vector2 p1 = ht.item_ui_rect.ypd_top_left() + (ht.item_ui_rect.dimensions() * ndc_point_to_topleft_normalized(st.srcNDCPos));
            const Vector2 p2 = ui::get_mouse_ui_position();

            drawlist.add_line(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist.add_rect_filled(Rect::from_corners(p1 - 12.0f, p1 + 12.0f), m_SrcSquareColor);
            drawlist.add_circle_filled({.origin = p2, .radius = 10.0f}, m_DestCircleColor);
        }
    }

    // render any mouse-related overlays
    void renderMouseUIElements(const ui::HittestResult& ht)
    {
        std::visit(Overload{
            [this, &ht](const auto& state) { renderMouseUIElements(ht, state); },
        }, m_MouseState);
    }

    // render any mouse-related overlays for when the user hasn't clicked yet
    void renderMouseUIElements(const ui::HittestResult& ht, GUIInitialMouseState)
    {
        const Vector2 mouseScreenPos = ui::get_mouse_ui_position();
        const Vector2 mouseImagePos = mouseScreenPos - ht.item_ui_rect.ypd_top_left();
        const Vector2 mouseImageRelPos = mouseImagePos / ht.item_ui_rect.dimensions();
        const Vector2 mouseImageNDCPos = topleft_normalized_point_to_ndc(mouseImageRelPos);

        ui::draw_tooltip_body_only(stream_to_string(mouseImageNDCPos));

        if (ui::is_mouse_clicked(ui::MouseButton::Left)) {
            m_MouseState = GUIFirstClickMouseState{mouseImageNDCPos};
        }
    }

    // render any mouse-related overlays for when the user has clicked once
    void renderMouseUIElements(const ui::HittestResult& ht, GUIFirstClickMouseState st)
    {
        const Vector2 mouseScreenPos = ui::get_mouse_ui_position();
        const Vector2 mouseImagePos = mouseScreenPos - ht.item_ui_rect.ypd_top_left();
        const Vector2 mouseImageRelPos = mouseImagePos / ht.item_ui_rect.dimensions();
        const Vector2 mouseImageNDCPos = topleft_normalized_point_to_ndc(mouseImageRelPos);

        ui::draw_tooltip_body_only(stream_to_string(mouseImageNDCPos) + "*");

        if (ui::is_mouse_clicked(ui::MouseButton::Left)) {
            m_LandmarkPairs.push_back({st.srcNDCPos, mouseImageNDCPos});
            m_MouseState = GUIInitialMouseState{};
        }
    }

    // TPS algorithm state
    GUIMouseState m_MouseState = GUIInitialMouseState{};
    std::vector<LandmarkPair2D> m_LandmarkPairs;
    float m_BlendingFactor = 1.0f;

    // GUI state (rendering, colors, etc.)
    Texture2D m_BoxTexture = SVG::read_into_texture(
        App::open_resource("OpenSimCreator/textures/uv_checker.svg")
    );
    Mesh m_InputGrid = PlaneGeometry{{.dimensions = Vector2{2.0f}, .num_segments = Vector2{50}}};
    Mesh m_OutputGrid = m_InputGrid;
    MeshBasicTexturedMaterial m_TexturedMaterial;
    MeshBasicMaterial wireframe_material_;

    Camera m_Camera;
    std::optional<RenderTexture> m_InputRender;
    std::optional<RenderTexture> m_OutputRender;
    Color m_SrcSquareColor = Color::red();
    Color m_DestCircleColor = Color::green();
    Color m_ConnectionLineColor = Color::white();

    // log panel (handy for debugging)
    LogViewerPanel m_LogViewerPanel{&owner()};
};


CStringView osc::TPS2DTab::id() { return "OpenSim/TPS2D"; }

osc::TPS2DTab::TPS2DTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::TPS2DTab::impl_on_draw() { private_data().onDraw(); }
