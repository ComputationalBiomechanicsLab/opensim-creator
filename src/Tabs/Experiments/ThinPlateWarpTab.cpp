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
#include "src/Utils/Algorithms.hpp"

#include <glm/vec2.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

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
    struct GUIFirstClickMouseState final { glm::vec2 imgPos; };
    using GUIMouseState = std::variant<GUIInitialMouseState, GUIFirstClickMouseState>;

    // a single source-to-destination landmark pair in 2D space
    struct LandmarkPair2D final {
        glm::vec2 src;
        glm::vec2 dest;
    };

    // this is effectviely the "U" term in the TPS algorithm literature (assumes r^2 * log(r^2) func)
    //
    // i.e. U(||controlPoint - p||) is equivalent to `DifferenceRadialBasisFunction2D(controlPoint, p)`
    float DifferenceRadialBasisFunction2D(glm::vec2 controlPoint, glm::vec2 p)
    {
        glm::vec2 const diff = controlPoint - p;
        float const r2 = glm::dot(diff, diff);
        return r2 * std::log(r2);
    }

    // a single weight term of the summation part of the linear combination
    //
    // i.e. in wi * U(||controlPoint - p||), this stores `wi` and `controlPoint`
    struct TPSWeightTerm2D final {
        float weight;
        glm::vec2 controlPoint;
    };

    // all linear coefficients in the TPS equation
    //
    // i.e. these are the a1, aXY, and w (+ control point) terms of the equation
    struct TPSCoefficients2D final {
        glm::vec2 a1 = {0.0f, 0.0f};
        glm::vec2 aXY = {1.0f, 1.0f};
        std::vector<TPSWeightTerm2D> weights;
    };

    // use the provided coefficients to evaluate (transform) the provided point
    glm::vec2 Evaluate(TPSCoefficients2D const& coefs, glm::vec2 p)
    {
        glm::vec2 rv = coefs.a1 + coefs.aXY * p;
        for (TPSWeightTerm2D const& wt : coefs.weights)
        {
            rv += wt.weight * DifferenceRadialBasisFunction2D(wt.controlPoint, p);
        }
        return rv;
    }

    TPSCoefficients2D CalcCoefficients(nonstd::span<LandmarkPair2D const> landmarkPairs)
    {
        TPSCoefficients2D rv;
        rv.weights.resize(landmarkPairs.size());  // TODO: just ensuring the evaluation method works with identity (0) terms
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
            ImGuiImageHittestResult const ht = osc::DrawTextureAsImGuiImageAndHittest(*m_OutputRender, texDims);

            // draw warning msg
            {
                glm::vec2 p = ImGui::GetCursorScreenPos();

                ImGui::SetCursorScreenPos(ht.rect.p1 + glm::vec2{10.0f, 10.0f});
                ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.0f, 0.0f, 1.0f});
                ImGui::Text("Warning: warping algorithm is not yet implemented");
                ImGui::PopStyleColor();

                ImGui::SetCursorScreenPos(p);
            }
        }
        ImGui::End();
    }


private:
    void renderGridMeshToRenderTexture(Mesh const& mesh, glm::ivec2 dims, std::optional<RenderTexture>& out)
    {
        RenderTextureDescriptor desc = {dims};
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
            glm::vec2 const p1 = ht.rect.p1 + p.src;
            glm::vec2 const p2 = ht.rect.p1 + p.dest;

            drawlist->AddLine(p1, p2, m_ConnectionLineColor, 5.0f);
            drawlist->AddCircleFilled(p1, 10.0f, m_SrcCircleColor);
            drawlist->AddCircleFilled(p2, 10.0f, m_DestCircleColor);
        }

        // render any currenty-placing landmark pairs in a more-faded color
        if (ht.isHovered && std::holds_alternative<GUIFirstClickMouseState>(m_MouseState))
        {
            GUIFirstClickMouseState const& st = std::get<GUIFirstClickMouseState>(m_MouseState);

            glm::vec2 const p1 = ht.rect.p1 + st.imgPos;
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
        std::string const txt = StreamToString(imgPos);

        osc::DrawTooltipBodyOnly(txt.c_str());

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_MouseState = GUIFirstClickMouseState{imgPos};
        }
    }

    void renderMouseUIElements(ImGuiImageHittestResult const& ht, GUIFirstClickMouseState st)
    {
        glm::vec2 const mp = ImGui::GetMousePos();
        glm::vec2 const imgPos = mp - ht.rect.p1;
        std::string const txt = StreamToString(imgPos) + "*";

        osc::DrawTooltipBodyOnly(txt.c_str());

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_LandmarkPairs.push_back({st.imgPos, imgPos});
            m_MouseState = GUIInitialMouseState{};
        }
    }

    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " ThinPlateWarpTab";
    TabHost* m_Parent;

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
