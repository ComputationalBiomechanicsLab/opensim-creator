#include "TPS2DTab.h"

#include <IconsFontAwesome5.h>
#include <oscar/Formats/Image.h>
#include <oscar/Graphics/Geometries/PlaneGeometry.h>
#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/LogViewerPanel.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <Simbody.h>

#include <cmath>
#include <algorithm>
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
        Vec2 src;
        Vec2 dest;
    };

    // this is effectviely the "U" term in the TPS algorithm literature (which is usually U(r) = r^2 * log(r^2))
    //
    // i.e. U(||pi - p||) in the literature is equivalent to `RadialBasisFunction2D(pi, p)` here
    float RadialBasisFunction2D(Vec2 controlPoint, Vec2 p)
    {
        Vec2 const diff = controlPoint - p;
        float const r2 = dot(diff, diff);

        if (r2 == 0.0f)
        {
            // this ensures that the result is always non-zero and non-NaN (this might be
            // necessary for some types of linear solvers?)
            return std::numeric_limits<float>::min();
        }
        else
        {
            return r2 * log(r2);
        }
    }

    // a single non-affine term of the 2D TPS equation
    //
    // i.e. in `f(p) = a1 + a2*p.x + a3*p.y + SUM{ wi * U(||controlPoint - p||) }` this encodes
    //      the `wi` and `controlPoint` parts of that equation
    struct TPSNonAffineTerm2D final {

        TPSNonAffineTerm2D(Vec2 weight_, Vec2 controlPoint_) :
            weight{weight_},
            controlPoint{controlPoint_}
        {
        }

        Vec2 weight;
        Vec2 controlPoint;
    };

    // all coefficients in the 2D TPS equation
    //
    // i.e. these are the a1, a2, a3, and w's (+ control points) terms of the equation
    struct TPSCoefficients2D final {
        Vec2 a1 = {0.0f, 0.0f};
        Vec2 a2 = {1.0f, 0.0f};
        Vec2 a3 = {0.0f, 1.0f};
        std::vector<TPSNonAffineTerm2D> weights;
    };

    // evaluates the TPS equation with the given coefficients and input point
    Vec2 Evaluate(TPSCoefficients2D const& coefs, Vec2 p)
    {
        // this implementation effectively evaluates both `fx(x, y)` and `fy(x, y)` at
        // the same time, because `TPSCoefficients2D` stores the X and Y variants of the
        // coefficients together in memory (as `vec2`s)

        // compute affine terms (a1 + a2*x + a3*y)
        Vec2 rv = coefs.a1 + coefs.a2*p.x + coefs.a3*p.y;

        // accumulate non-affine terms (effectively: wi * U(||controlPoint - p||))
        for (TPSNonAffineTerm2D const& wt : coefs.weights)
        {
            rv += wt.weight * RadialBasisFunction2D(wt.controlPoint, p);
        }

        return rv;
    }

    // computes all coefficients of the TPS equation (a1, a2, a3, and all the w's)
    TPSCoefficients2D CalcCoefficients(std::span<LandmarkPair2D const> landmarkPairs)
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

        int const numPairs = static_cast<int>(landmarkPairs.size());

        if (numPairs == 0)
        {
            // edge-case: there are no pairs, so return an identity-like transform
            return TPSCoefficients2D{};
        }

        // construct matrix L
        SimTK::Matrix L(numPairs + 3, numPairs + 3);

        // populate the K part of matrix L (upper-left)
        for (int row = 0; row < numPairs; ++row)
        {
            for (int col = 0; col < numPairs; ++col)
            {
                Vec2 const& pi_ = landmarkPairs[row].src;
                Vec2 const& pj = landmarkPairs[col].src;

                L(row, col) = RadialBasisFunction2D(pi_, pj);
            }
        }

        // populate the P part of matrix L (upper-right)
        {
            int const pStartColumn = numPairs;

            for (int row = 0; row < numPairs; ++row)
            {
                L(row, pStartColumn)     = 1.0;
                L(row, pStartColumn + 1) = landmarkPairs[row].src.x;
                L(row, pStartColumn + 2) = landmarkPairs[row].src.y;
            }
        }

        // populate the PT part of matrix L (bottom-left)
        {
            int const ptStartRow = numPairs;

            for (int col = 0; col < numPairs; ++col)
            {
                L(ptStartRow, col)     = 1.0;
                L(ptStartRow + 1, col) = landmarkPairs[col].src.x;
                L(ptStartRow + 2, col) = landmarkPairs[col].src.y;
            }
        }

        // populate the 0 part of matrix L (bottom-right)
        {
            int const zeroStartRow = numPairs;
            int const zeroStartCol = numPairs;

            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 3; ++col)
                {
                    L(zeroStartRow + row, zeroStartCol + col) = 0.0;
                }
            }
        }

        // construct "result" vectors Vx and Vy (these hold the landmark destinations)
        SimTK::Vector Vx(numPairs + 3, 0.0);
        SimTK::Vector Vy(numPairs + 3, 0.0);
        for (int row = 0; row < numPairs; ++row)
        {
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
        for (int i = 0; i < numPairs; ++i)
        {
            Vec2 weight = {Cx[i], Cy[i]};
            Vec2 controlPoint = landmarkPairs[i].src;
            rv.weights.emplace_back(weight, controlPoint);
        }

        return rv;
    }

    // a class that wraps the 2D TPS algorithm with a basic interface for transforming
    // points
    class ThinPlateWarper2D final {
    public:
        explicit ThinPlateWarper2D(std::span<LandmarkPair2D const> landmarkPairs) :
            m_Coefficients{CalcCoefficients(landmarkPairs)}
        {
        }

        Vec2 transform(Vec2 p) const
        {
            return Evaluate(m_Coefficients, p);
        }

    private:
        TPSCoefficients2D m_Coefficients;
    };

    // returns a mesh that is the equivalent of applying the 2D TPS warp to all
    // vertices of the input mesh
    Mesh ApplyThinPlateWarpToMesh(ThinPlateWarper2D const& t, Mesh const& mesh)
    {
        Mesh rv = mesh;
        rv.transformVerts([&t](Vec3 v) { return Vec3{t.transform(v), v.z}; });
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
    struct GUIFirstClickMouseState final { Vec2 srcNDCPos; };
    using GUIMouseState = std::variant<GUIInitialMouseState, GUIFirstClickMouseState>;
}

class osc::TPS2DTab::Impl final {
public:

    Impl()
    {
        m_Material.setTexture("uTextureSampler", m_BoxTexture);
        m_WireframeMaterial.setColor({0.0f, 0.0f, 0.0f, 0.15f});
        m_WireframeMaterial.setTransparent(true);
        m_WireframeMaterial.setWireframeMode(true);
        m_WireframeMaterial.setDepthTested(false);
        m_Camera.setViewMatrixOverride(identity<Mat4>());
        m_Camera.setProjectionMatrixOverride(identity<Mat4>());
        m_Camera.setBackgroundColor(Color::white());
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_BEZIER_CURVE " TPS2DTab";
    }

    void onDraw()
    {
        ui::DockSpaceOverViewport(ui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        ui::Begin("Input");
        {
            Vec2 const windowDims = ui::GetContentRegionAvail();
            float const minDim = min(windowDims.x, windowDims.y);
            Vec2i const texDims = Vec2i{minDim, minDim};

            renderMesh(m_InputGrid, texDims, m_InputRender);

            // draw rendered texture via ImGui
            ui::DrawTextureAsImGuiImage(*m_InputRender, texDims);
            ui::ImGuiItemHittestResult const ht = ui::HittestLastImguiItem();

            // draw any 2D overlays etc.
            renderOverlayElements(ht);
            if (ht.isHovered)
            {
                renderMouseUIElements(ht);
            }
        }

        ui::End();

        Vec2 outputWindowPos;
        Vec2 outputWindowDims;
        ui::Begin("Output");
        {
            outputWindowPos = ui::GetCursorScreenPos();
            outputWindowDims = ui::GetContentRegionAvail();
            float const minDim = min(outputWindowDims.x, outputWindowDims.y);
            Vec2i const texDims = Vec2i{minDim, minDim};

            {
                // apply blending factor, compute warp, apply to grid

                std::vector<LandmarkPair2D> pairs = m_LandmarkPairs;
                for (LandmarkPair2D& p : pairs)
                {
                    p.dest = lerp(p.src, p.dest, m_BlendingFactor);
                }
                ThinPlateWarper2D warper{pairs};
                m_OutputGrid = ApplyThinPlateWarpToMesh(warper, m_InputGrid);
            }

            renderMesh(m_OutputGrid, texDims, m_OutputRender);

            // draw rendered texture via ImGui
            ui::DrawTextureAsImGuiImage(*m_OutputRender, texDims);
        }
        ui::End();

        // draw scubber overlay
        {
            float leftPadding = 10.0f;
            float bottomPadding = 10.0f;
            float panelHeight = 50.0f;
            ui::SetNextWindowPos({ outputWindowPos.x + leftPadding, outputWindowPos.y + outputWindowDims.y - panelHeight - bottomPadding });
            ui::SetNextWindowSize({ outputWindowDims.x - leftPadding, panelHeight });
            ui::Begin("##scrubber", nullptr, ui::GetMinimalWindowFlags() & ~ImGuiWindowFlags_NoInputs);
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
            ui::SliderFloat("##blend", &m_BlendingFactor, 0.0f, 1.0f);
            ui::End();
        }

        // draw log panel (debugging)
        m_LogViewerPanel.onDraw();
    }


private:

    // render the given mesh as-is to the given output render texture
    void renderMesh(Mesh const& mesh, Vec2i dims, std::optional<RenderTexture>& out)
    {
        RenderTextureDescriptor desc{dims};
        desc.setAntialiasingLevel(App::get().getCurrentAntiAliasingLevel());
        out.emplace(desc);
        Graphics::DrawMesh(mesh, identity<Transform>(), m_Material, m_Camera);
        Graphics::DrawMesh(mesh, identity<Transform>(), m_WireframeMaterial, m_Camera);

        OSC_ASSERT(out.has_value());
        m_Camera.renderTo(*out);

        OSC_ASSERT(out.has_value() && "the camera should've given the render texture back to the caller");
    }

    // render any 2D overlays
    void renderOverlayElements(ui::ImGuiItemHittestResult const& ht)
    {
        ImDrawList* const drawlist = ui::GetWindowDrawList();

        // render all fully-established landmark pairs
        for (LandmarkPair2D const& p : m_LandmarkPairs)
        {
            Vec2 const p1 = ht.rect.p1 + (dimensions(ht.rect) * NDCPointToTopLeftRelPos(p.src));
            Vec2 const p2 = ht.rect.p1 + (dimensions(ht.rect) * NDCPointToTopLeftRelPos(p.dest));

            drawlist->AddLine(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist->AddRectFilled(p1 - 12.0f, p1 + 12.0f, m_SrcSquareColor);
            drawlist->AddCircleFilled(p2, 10.0f, m_DestCircleColor);
        }

        // render any currenty-placing landmark pairs in a more-faded color
        if (ht.isHovered && std::holds_alternative<GUIFirstClickMouseState>(m_MouseState))
        {
            GUIFirstClickMouseState const& st = std::get<GUIFirstClickMouseState>(m_MouseState);

            Vec2 const p1 = ht.rect.p1 + (dimensions(ht.rect) * NDCPointToTopLeftRelPos(st.srcNDCPos));
            Vec2 const p2 = ui::GetMousePos();

            drawlist->AddLine(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist->AddRectFilled(p1 - 12.0f, p1 + 12.0f, m_SrcSquareColor);
            drawlist->AddCircleFilled(p2, 10.0f, m_DestCircleColor);
        }
    }

    // render any mouse-related overlays
    void renderMouseUIElements(ui::ImGuiItemHittestResult const& ht)
    {
        std::visit(Overload
        {
            [this, &ht](GUIInitialMouseState const& st) { renderMouseUIElements(ht, st); },
            [this, &ht](GUIFirstClickMouseState const& st) { renderMouseUIElements(ht, st); },
        }, m_MouseState);
    }

    // render any mouse-related overlays for when the user hasn't clicked yet
    void renderMouseUIElements(ui::ImGuiItemHittestResult const& ht, GUIInitialMouseState)
    {
        Vec2 const mouseScreenPos = ui::GetMousePos();
        Vec2 const mouseImagePos = mouseScreenPos - ht.rect.p1;
        Vec2 const mouseImageRelPos = mouseImagePos / dimensions(ht.rect);
        Vec2 const mouseImageNDCPos = TopleftRelPosToNDCPoint(mouseImageRelPos);

        ui::DrawTooltipBodyOnly(to_string(mouseImageNDCPos));

        if (ui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_MouseState = GUIFirstClickMouseState{mouseImageNDCPos};
        }
    }

    // render any mouse-related overlays for when the user has clicked once
    void renderMouseUIElements(ui::ImGuiItemHittestResult const& ht, GUIFirstClickMouseState st)
    {
        Vec2 const mouseScreenPos = ui::GetMousePos();
        Vec2 const mouseImagePos = mouseScreenPos - ht.rect.p1;
        Vec2 const mouseImageRelPos = mouseImagePos / dimensions(ht.rect);
        Vec2 const mouseImageNDCPos = TopleftRelPosToNDCPoint(mouseImageRelPos);

        ui::DrawTooltipBodyOnly(to_string(mouseImageNDCPos) + "*");

        if (ui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_LandmarkPairs.push_back({st.srcNDCPos, mouseImageNDCPos});
            m_MouseState = GUIInitialMouseState{};
        }
    }

    // tab data
    UID m_TabID;
    ResourceLoader m_Loader = App::resource_loader();

    // TPS algorithm state
    GUIMouseState m_MouseState = GUIInitialMouseState{};
    std::vector<LandmarkPair2D> m_LandmarkPairs;
    float m_BlendingFactor = 1.0f;

    // GUI state (rendering, colors, etc.)
    Texture2D m_BoxTexture = LoadTexture2DFromImage(
        m_Loader.open("textures/container.jpg"),
        ColorSpace::sRGB
    );
    Mesh m_InputGrid = PlaneGeometry{2.0f, 2.0f, 50, 50};
    Mesh m_OutputGrid = m_InputGrid;
    Material m_Material{Shader{m_Loader.slurp("shaders/TPS2D/Textured.vert"), m_Loader.slurp("shaders/TPS2D/Textured.frag")}};
    MeshBasicMaterial m_WireframeMaterial;

    Camera m_Camera;
    std::optional<RenderTexture> m_InputRender;
    std::optional<RenderTexture> m_OutputRender;
    ImU32 m_SrcSquareColor = ui::ToImU32(Color::red());
    ImU32 m_DestCircleColor = ui::ToImU32(Color::green());
    ImU32 m_ConnectionLineColor = ui::ToImU32(Color::white());

    // log panel (handy for debugging)
    LogViewerPanel m_LogViewerPanel{"Log"};
};


// public API (PIMPL)

CStringView osc::TPS2DTab::id()
{
    return "OpenSim/Experimental/TPS2D";
}

osc::TPS2DTab::TPS2DTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::TPS2DTab::TPS2DTab(TPS2DTab&&) noexcept = default;
osc::TPS2DTab& osc::TPS2DTab::operator=(TPS2DTab&&) noexcept = default;
osc::TPS2DTab::~TPS2DTab() noexcept = default;

UID osc::TPS2DTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::TPS2DTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::TPS2DTab::implOnDraw()
{
    m_Impl->onDraw();
}
