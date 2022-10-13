#include "ThinPlateWarpTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/GlmHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/LogViewerPanel.hpp"

#include <glm/vec2.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>
#include <Simbody.h>

#include <cstdint>
#include <string>
#include <limits>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    // returns a 2D grid mesh, with one vertex for each grid point and indices with a
    // `Line` topology, connecting the gridpoints together
    osc::Mesh GenerateNxNPointGridLines(glm::vec2 min, glm::vec2 max, glm::ivec2 steps)
    {
        // all Z values in the returned mesh shall be 0
        constexpr float zValue = 0.0f;

        if (steps.x <= 0 || steps.y <= 0)
        {
            // edge case: no steps specified: return empty mesh
            return {};
        }

        // ensure the indices can fit the requested grid
        {
            OSC_ASSERT(steps.x*steps.y <= std::numeric_limits<int32_t>::max() && "requested a grid size that is too large for the mesh class");
        }

        // create vector of grid points
        std::vector<glm::vec3> verts;
        verts.reserve(static_cast<size_t>(steps.x * steps.y));

        // create vector of line indices (indices to the two points that make a grid line)
        std::vector<uint32_t> indices;
        indices.reserve(static_cast<size_t>(4 * steps.x * steps.y));

        // precompute spatial step between points
        glm::vec2 const stepSize = (max - min) / glm::vec2{steps - 1};

        // push first row (no verticals)
        {
            // emit top-leftmost point (no links)
            {
                verts.push_back({min, zValue});
            }

            // emit rest of the first row (only has horizontal links)
            for (int32_t x = 1; x < steps.x; ++x)
            {
                glm::vec3 const pos = {min.x + x*stepSize.x, min.y, zValue};
                verts.push_back(pos);
                uint32_t const index = static_cast<int32_t>(verts.size() - 1);
                indices.push_back(index - 1);  // link to previous point
                indices.push_back(index);      // and then the new point
            }

            OSC_ASSERT(verts.size() == steps.x && "all points in the first row have not been emitted");
            OSC_ASSERT(indices.size() == 2 * (steps.x - 1) && "all lines in the first row have not been emitted");
        }

        // push remaining rows (all points have verticals, first point of each row has no horizontal)
        for (int32_t y = 1; y < steps.y; ++y)
        {
            // emit leftmost point (only has a vertical link)
            {
                verts.push_back({min.x, min.y + y*stepSize.y, zValue});
                uint32_t const index = static_cast<int32_t>(verts.size() - 1);
                indices.push_back(index - steps.x);  // link the point one row above
                indices.push_back(index);            // to the point (vertically)
            }

            // emit rest of the row (has vertical and horizontal links)
            for (int32_t x = 1; x < steps.x; ++x)
            {
                glm::vec3 const pos = {min.x + x*stepSize.x, min.y + y*stepSize.y, zValue};
                verts.push_back(pos);
                uint32_t const index = static_cast<int32_t>(verts.size() - 1);
                indices.push_back(index - 1);        // link the previous point
                indices.push_back(index);            // to the current point (horizontally)
                indices.push_back(index - steps.x);  // link the point one row above
                indices.push_back(index);            // to the current point (vertically)
            }
        }

        OSC_ASSERT(verts.size() == steps.x*steps.y && "incorrect number of vertices emitted");
        OSC_ASSERT(indices.size() <= 4 * steps.y * steps.y && "too many indices were emitted?");

        // emit data as a renderable mesh
        osc::Mesh rv;
        rv.setTopography(osc::MeshTopography::Lines);
        rv.setVerts(std::move(verts));
        rv.setIndices(indices);
        return rv;
    }

    // used to hold the user's mouse click state (they need to click twice)
    struct GUIInitialMouseState final {};
    struct GUIFirstClickMouseState final { glm::vec2 srcNDCPos; };
    using GUIMouseState = std::variant<GUIInitialMouseState, GUIFirstClickMouseState>;

    // a single source-to-destination landmark pair in 2D space
    struct LandmarkPair2D final {
        glm::vec2 src;
        glm::vec2 dest;
    };

    std::ostream& operator<<(std::ostream& o, LandmarkPair2D const& p)
    {
        using osc::operator<<;
        o << "LandmarkPair2D{src = " << p.src << ", dest = " << p.dest << '}';
        return o;
    }

    // this is effectviely the "U" term in the TPS algorithm literature (assumes r^2 * log(r^2) func)
    //
    // i.e. U(||controlPoint - p||) is equivalent to `DifferenceRadialBasisFunction2D(controlPoint, p)`
    float DifferenceRadialBasisFunction2D(glm::vec2 controlPoint, glm::vec2 p)
    {
        glm::vec2 const diff = controlPoint - p;
        float const r2 = glm::dot(diff, diff);

        if (r2 == 0.0f)
        {
            // this is to ensure that the result is always non-zero and non-NaN
            return std::numeric_limits<float>::min();
        }
        else
        {
            return r2 * std::log(r2);
        }
    }

    // a single weight term of the summation part of the linear combination
    //
    // i.e. in wi * U(||controlPoint - p||), this stores `wi` and `controlPoint`
    struct TPSWeightTerm2D final {
        glm::vec2 weight;
        glm::vec2 controlPoint;
    };

    std::ostream& operator<<(std::ostream& o, TPSWeightTerm2D const& wt)
    {
        using osc::operator<<;
        return o << "TPSWeightTerm2D{weight = " << wt.weight << ", controlPoint = " << wt.controlPoint << '}';
    }

    // all linear coefficients in the TPS equation
    //
    // i.e. these are the a1, aXY, and w (+ control point) terms of the equation
    struct TPSCoefficients2D final {
        glm::vec2 a1 = {0.0f, 0.0f};
        glm::vec2 a2x = {1.0f, 1.0f};
        glm::vec2 a2y = {1.0f, 1.0f};
        std::vector<TPSWeightTerm2D> weights;
    };

    std::ostream& operator<<(std::ostream& o, TPSCoefficients2D const& coefs)
    {
        using osc::operator<<;
        o << "TPSCoefficients2D{a1 = " << coefs.a1 << ", a2x = " << coefs.a2x << ", a2y = " << coefs.a2y;
        for (size_t i = 0; i < coefs.weights.size(); ++i)
        {
            o << ", w" << i << " = " << coefs.weights[i];
        }
        o << '}';
        return o;
    }

    // use the provided coefficients to evaluate (transform) the provided point
    glm::vec2 Evaluate(TPSCoefficients2D const& coefs, glm::vec2 p)
    {
        glm::vec2 rv = coefs.a1 + coefs.a2x * p; + coefs.a2y * p;
        for (TPSWeightTerm2D const& wt : coefs.weights)
        {
            rv += wt.weight * DifferenceRadialBasisFunction2D(wt.controlPoint, p);
        }
        return rv;
    }

    TPSCoefficients2D CalcCoefficients(nonstd::span<LandmarkPair2D const> landmarkPairs)
    {
        // this is based on the Bookstein Thin Plate Sline (TPS) warping algorithm
        //
        // 1. A TPS warp is (simplifying here) a linear combination:
        //
        //     f(p) = a1 + a2*p + SUM{ wi * U(||controlPoint_i - p||) }
        //
        //    which can be represented as a matrix multiplication between the terms (1, p,
        //    U(||cpi - p||)) and the linear coefficients (a1, a2, wi..)
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
        // 6. Invert it to yield L^-1
        // 7. Multiply L^-1 * [v o] (desired output values) to yield [w a] (the coefficients)
        // 8. Return the coefficients
        //
        // TODO: this isn't a very efficient implementation. A better one would take advantage
        //       of the fact that U(p01) == U(p10), which makes the entire matrix symmetric, and
        //       the coefficients much easier to compute (via LU decomposition, rather than a
        //       full-fat matrix inversion)

        int const numPairs = static_cast<int>(landmarkPairs.size());

        if (numPairs == 0)
        {
            return {};  // edge-case: there are no pairs, so return an identity-like transform
        }

        // construct matrix L
        SimTK::Matrix_<float> L(numPairs + 3, numPairs + 3);

        // populate the K part of matrix L (upper-left)
        for (int row = 0; row < numPairs; ++row)
        {
            for (int col = 0; col < numPairs; ++col)
            {
                glm::vec2 const& pi = landmarkPairs[row].src;
                glm::vec2 const& pj = landmarkPairs[col].src;

                L(row, col) = DifferenceRadialBasisFunction2D(pi, pj);
            }
        }

        // populate the P part of matrix L (upper-right)
        {
            int const pStartColumn = numPairs;

            for (int row = 0; row < numPairs; ++row)
            {
                L(row, pStartColumn)     = 1.0f;
                L(row, pStartColumn + 1) = landmarkPairs[row].src.x;
                L(row, pStartColumn + 2) = landmarkPairs[row].src.y;
            }
        }

        // populate the PT part of matrix L (bottom-left)
        {
            int const ptStartRow = numPairs;

            for (int col = 0; col < numPairs; ++col)
            {
                L(ptStartRow, col)     = 1.0f;
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
                    L(zeroStartRow + row, zeroStartCol + col) = 0.0f;
                }
            }
        }

        // invert L
        L.invertInPlace();

        // use inverted matrix to compute each coefficient (wi, a1, and a2)
        TPSCoefficients2D rv;
        rv.weights.reserve(numPairs);

        // compute w1...wi
        for (int row = 0; row < numPairs; ++row)
        {
            glm::vec2 wi = {0.0f, 0.0f};
            for (int col = 0; col < numPairs; ++col)
            {
                wi += static_cast<float>(L(row, col)) * landmarkPairs[col].dest;
            }
            rv.weights.push_back(TPSWeightTerm2D{wi, landmarkPairs[row].src});

            // note: we ignore the last 3 columns because they would effectively just multiply
            //       with the padding zero elements in the result vector
        }

        // a1
        {
            int const a1Row = numPairs;

            glm::vec2 a1 = {0.0f, 0.0f};
            for (int col = 0; col < numPairs; ++col)
            {
                a1 += static_cast<float>(L(a1Row, col)) * landmarkPairs[col].dest;
            }
            rv.a1 = a1;

            // note: we ignore the last 3 columns because they would effectively just multiply
            //       with the padding zero elements in the result vector
        }

        // a2x
        {
            int const a2xRow = numPairs + 1;

            glm::vec2 a2x = {0.0f, 0.0f};
            for (int col = 0; col < numPairs; ++col)
            {
                a2x += static_cast<float>(L(a2xRow, col)) * landmarkPairs[col].dest;
            }
            rv.a2x = a2x;
        }

        // a2y
        {
            int const a2yRow = numPairs + 2;

            glm::vec2 a2y = {0.0f, 0.0f};
            for (int col = 0; col < numPairs; ++col)
            {
                a2y += static_cast<float>(L(a2yRow, col)) * landmarkPairs[col].dest;
            }
            rv.a2y = a2y;
        }

        return rv;
    }

    class ThinPlateWarper2D final {
    public:
        ThinPlateWarper2D(nonstd::span<LandmarkPair2D const> landmarkPairs) :
            m_Coefficients{CalcCoefficients(landmarkPairs)}
        {
        }

        glm::vec2 transform(glm::vec2 p) const
        {
            return Evaluate(m_Coefficients, p);
        }

    private:
        TPSCoefficients2D m_Coefficients;
    };

    // apply a thin-plate warp to each of the points in the source mesh
    osc::Mesh ApplyThinPlateWarpToMesh(ThinPlateWarper2D const& t, osc::Mesh const& inputGrid)
    {
        // load source points
        nonstd::span<glm::vec3 const> srcPoints = inputGrid.getVerts();

        // map each source point via the warper
        std::vector<glm::vec3> destPoints;
        destPoints.reserve(srcPoints.size());
        for (glm::vec3 const& srcPoint : srcPoints)
        {
            destPoints.emplace_back(t.transform(srcPoint), srcPoint.z);
        }

        // upload the new points into the returned mesh
        osc::Mesh rv = inputGrid;
        rv.setVerts(destPoints);
        return rv;
    }
}

class osc::ThinPlateWarpTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
        m_Material.setVec4("uColor", {0.0f, 0.0f, 0.0f, 1.0f});
        m_Camera.setViewMatrix(glm::mat4{1.0f});
        m_Camera.setProjectionMatrix(glm::mat4{1.0f});
        m_Camera.setBackgroundColor({1.0f, 1.0f, 1.0f, 1.0f});
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {

    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::Begin("Input");
        {
            glm::vec2 const windowDims = ImGui::GetContentRegionAvail();
            float const minDim = glm::min(windowDims.x, windowDims.y);
            glm::ivec2 const texDims = glm::ivec2{minDim, minDim};

            renderGridMeshToRenderTexture(m_InputGrid, texDims, m_InputRender);
            OSC_ASSERT(m_InputRender.has_value());

            // draw rendered texture via ImGui
            ImGuiImageHittestResult const ht = osc::DrawTextureAsImGuiImageAndHittest(*m_InputRender, texDims);

            // draw any 2D overlays etc.
            renderOverlayElements(ht);
            if (ht.isHovered)
            {
                renderMouseUIElements(ht);
            }
        }
        
        ImGui::End();

        ImGui::Begin("Output");
        {
            glm::vec2 const windowDims = ImGui::GetContentRegionAvail();
            float const minDim = glm::min(windowDims.x, windowDims.y);
            glm::ivec2 const texDims = glm::ivec2{minDim, minDim};

            ThinPlateWarper2D warper{m_LandmarkPairs};
            m_OutputGrid = ApplyThinPlateWarpToMesh(warper, m_InputGrid);

            renderGridMeshToRenderTexture(m_OutputGrid, texDims, m_OutputRender);
            OSC_ASSERT(m_OutputRender.has_value());

            // draw rendered texture via ImGui
            osc::DrawTextureAsImGuiImage(*m_OutputRender, texDims);
        }
        ImGui::End();

        // draw log panel (debugging)
        m_LogViewerPanel.draw();
    }


private:
    void renderGridMeshToRenderTexture(Mesh const& mesh, glm::ivec2 dims, std::optional<RenderTexture>& out)
    {
        RenderTextureDescriptor desc{dims};
        desc.setAntialiasingLevel(App::get().getMSXAASamplesRecommended());
        out.emplace(desc);
        osc::Graphics::DrawMesh(mesh, osc::Transform{}, m_Material, m_Camera);
        m_Camera.swapTexture(out);
        m_Camera.render();
        m_Camera.swapTexture(out);
    }

    void renderOverlayElements(ImGuiImageHittestResult const& ht)
    {
        ImDrawList* drawlist = ImGui::GetWindowDrawList();

        // render all fully-established landmark pairs
        for (LandmarkPair2D const& p : m_LandmarkPairs)
        {
            glm::vec2 const p1 = ht.rect.p1 + Dimensions(ht.rect)*(0.5f*((p.src * glm::vec2{1.0f, -1.0f}) + 1.0f));
            glm::vec2 const p2 = ht.rect.p1 + Dimensions(ht.rect)*(0.5f*((p.dest * glm::vec2{1.0f, -1.0f}) + 1.0f));

            drawlist->AddLine(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist->AddCircleFilled(p1, 10.0f, m_SrcCircleColor);
            drawlist->AddCircleFilled(p2, 10.0f, m_DestCircleColor);
        }

        // render any currenty-placing landmark pairs in a more-faded color
        if (ht.isHovered && std::holds_alternative<GUIFirstClickMouseState>(m_MouseState))
        {
            GUIFirstClickMouseState const& st = std::get<GUIFirstClickMouseState>(m_MouseState);

            glm::vec2 const p1 = ht.rect.p1 + Dimensions(ht.rect)*(0.5f*((st.srcNDCPos * glm::vec2{1.0f, -1.0f}) + 1.0f));
            glm::vec2 const p2 = ImGui::GetMousePos();

            drawlist->AddLine(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist->AddCircleFilled(p1, 10.0f, m_SrcCircleColor);
            drawlist->AddCircleFilled(p2, 10.0f, m_DestCircleColor);
        }
    }

    void renderMouseUIElements(ImGuiImageHittestResult const& ht)
    {
        std::visit(osc::Overload
        {
            [this, &ht](GUIInitialMouseState const& st) { renderMouseUIElements(ht, st); },
            [this, &ht](GUIFirstClickMouseState const& st) { renderMouseUIElements(ht, st); },
        }, m_MouseState);
    }

    void renderMouseUIElements(ImGuiImageHittestResult const& ht, GUIInitialMouseState st)
    {
        glm::vec2 const mp = ImGui::GetMousePos();
        glm::vec2 const imgPos = mp - ht.rect.p1;
        glm::vec2 const ndcPos = ((2.0f * imgPos/Dimensions(ht.rect)) - 1.0f) * glm::vec2{1.0f, -1.0f};

        osc::DrawTooltipBodyOnly(StreamToString(ndcPos).c_str());

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_MouseState = GUIFirstClickMouseState{ndcPos};
        }
    }

    void renderMouseUIElements(ImGuiImageHittestResult const& ht, GUIFirstClickMouseState st)
    {
        glm::vec2 const mp = ImGui::GetMousePos();
        glm::vec2 const imgPos = mp - ht.rect.p1;
        glm::vec2 const ndcPos = ((2.0f * imgPos/Dimensions(ht.rect)) - 1.0f) * glm::vec2{1.0f, -1.0f};

        osc::DrawTooltipBodyOnly((StreamToString(ndcPos) + "*").c_str());

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_LandmarkPairs.push_back({st.srcNDCPos, ndcPos});
            m_MouseState = GUIInitialMouseState{};
        }
    }

    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " ThinPlateWarpTab";
    TabHost* m_Parent;

    LogViewerPanel m_LogViewerPanel{"Log"};

    Mesh m_InputGrid = GenerateNxNPointGridLines({-1.0f, -1.0f}, {1.0f, 1.0f}, {20, 20});
    Mesh m_OutputGrid = m_InputGrid;
    Material m_Material = Material{App::shaders().get("shaders/SolidColor.vert", "shaders/SolidColor.frag")};
    Camera m_Camera;
    std::optional<RenderTexture> m_InputRender;
    std::optional<RenderTexture> m_OutputRender;

    ImU32 m_SrcCircleColor = ImGui::ColorConvertFloat4ToU32({1.0f, 0.0f, 0.0f, 1.0f});
    ImU32 m_DestCircleColor = ImGui::ColorConvertFloat4ToU32({0.0f, 1.0f, 0.0f, 1.0f});
    ImU32 m_ConnectionLineColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 0.6f});

    GUIMouseState m_MouseState = GUIInitialMouseState{};
    std::vector<LandmarkPair2D> m_LandmarkPairs;
};


// public API (PIMPL)

osc::ThinPlateWarpTab::ThinPlateWarpTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::ThinPlateWarpTab::ThinPlateWarpTab(ThinPlateWarpTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ThinPlateWarpTab& osc::ThinPlateWarpTab::operator=(ThinPlateWarpTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ThinPlateWarpTab::~ThinPlateWarpTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::ThinPlateWarpTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ThinPlateWarpTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ThinPlateWarpTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ThinPlateWarpTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ThinPlateWarpTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ThinPlateWarpTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ThinPlateWarpTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ThinPlateWarpTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ThinPlateWarpTab::implOnDraw()
{
    m_Impl->onDraw();
}
