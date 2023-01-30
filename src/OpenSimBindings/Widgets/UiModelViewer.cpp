#include "UiModelViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Formats/DAE.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Icon.hpp"
#include "src/Graphics/IconCache.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/Rendering/CachedModelRenderer.hpp"
#include "src/OpenSimBindings/Rendering/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Rendering/CustomRenderingOptions.hpp"
#include "src/OpenSimBindings/Rendering/ModelRendererParams.hpp"
#include "src/OpenSimBindings/Rendering/OpenSimDecorationGenerator.hpp"
#include "src/OpenSimBindings/Rendering/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/Rendering/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/Rendering/MuscleSizingStyle.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/GuiRuler.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <IconsFontAwesome5.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

class osc::UiModelViewer::Impl final {
public:

    bool isLeftClicked() const
    {
        return m_RenderedImageHittest.isLeftClickReleasedWithoutDragging;
    }

    bool isRightClicked() const
    {
        return m_RenderedImageHittest.isRightClickReleasedWithoutDragging;
    }

    bool isMousedOver() const
    {
        return m_RenderedImageHittest.isHovered;
    }

    osc::UiModelViewerResponse draw(VirtualConstModelStatePair const& rs)
    {
        OSC_PERF("UiModelViewer/draw");

        UiModelViewerResponse rv;

        handleUserInput();

        if (!ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove))
        {
            // render window isn't visible
            m_RenderedImageHittest = {};
            return rv;
        }

        m_CachedModelRenderer.populate(rs, m_Params);

        std::pair<OpenSim::Component const*, glm::vec3> htResult = hittestRenderWindow(rs);

        // if this is the first frame being rendered, auto-focus the scene
        if (m_IsRenderingFirstFrame && m_CachedModelRenderer.getRootAABB())
        {
            AutoFocus(
                m_Params.camera,
                *m_CachedModelRenderer.getRootAABB(),
                AspectRatio(ImGui::GetContentRegionAvail())
            );
            m_IsRenderingFirstFrame = false;
        }

        // render into texture
        {
            OSC_PERF("UiModelViewer/draw/render");
            m_CachedModelRenderer.draw(
                rs,
                m_Params,
                ImGui::GetContentRegionAvail(),
                App::get().getMSXAASamplesRecommended()
            );
        }

        // blit texture as an ImGui::Image
        osc::DrawTextureAsImGuiImage(m_CachedModelRenderer.updRenderTexture());
        m_RenderedImageHittest = osc::HittestLastImguiItem();

        // draw any ImGui-based overlays over the image
        {
            OSC_PERF("UiModelViewer/draw/overlays");

            drawImGuiOverlays();

            if (m_Ruler.isMeasuring())
            {
                std::optional<GuiRulerMouseHit> maybeHit;
                if (htResult.first)
                {
                    maybeHit.emplace(htResult.first->getName(), htResult.second);
                }
                m_Ruler.draw(m_Params.camera, m_RenderedImageHittest.rect, maybeHit);
            }
        }

        ImGui::EndChild();

        // handle return value

        if (!m_Ruler.isMeasuring())
        {
            // only populate response if the ruler isn't blocking hittesting etc.
            rv.hovertestResult = htResult.first;
            rv.isMousedOver = m_RenderedImageHittest.isHovered;
            if (rv.isMousedOver)
            {
                rv.mouse3DLocation = htResult.second;
            }
        }

        return rv;
    }

private:

    void handleUserInput()
    {
        // update camera if necessary
        if (m_RenderedImageHittest.isHovered)
        {
            bool ctrlDown = osc::IsCtrlOrSuperDown();

            if (ImGui::IsKeyReleased(ImGuiKey_X))
            {
                if (ctrlDown)
                {
                    FocusAlongMinusX(m_Params.camera);
                } else
                {
                    FocusAlongX(m_Params.camera);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y))
            {
                if (!ctrlDown)
                {
                    FocusAlongY(m_Params.camera);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F))
            {
                if (ctrlDown)
                {
                    std::optional<osc::AABB> maybeSceneAABB = m_CachedModelRenderer.getRootAABB();
                    if (maybeSceneAABB)
                    {
                        osc::AutoFocus(
                            m_Params.camera,
                            *maybeSceneAABB,
                            osc::AspectRatio(m_RenderedImageHittest.rect)
                        );
                    }
                }
                else
                {
                    Reset(m_Params.camera);
                }
            }
            if (ctrlDown && (ImGui::IsKeyPressed(ImGuiKey_8)))
            {
                std::optional<osc::AABB> maybeSceneAABB = m_CachedModelRenderer.getRootAABB();
                if (maybeSceneAABB)
                {
                    osc::AutoFocus(
                        m_Params.camera,
                        *maybeSceneAABB,
                        osc::AspectRatio(m_RenderedImageHittest.rect)
                    );
                }
            }
            UpdatePolarCameraFromImGuiUserInput(Dimensions(m_RenderedImageHittest.rect), m_Params.camera);
        }
    }

    std::pair<OpenSim::Component const*, glm::vec3> hittestRenderWindow(osc::VirtualConstModelStatePair const& msp)
    {
        std::pair<OpenSim::Component const*, glm::vec3> rv = {nullptr, {0.0f, 0.0f, 0.0f}};

        if (!m_RenderedImageHittest.isHovered ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            // only do the hit test if the user isn't currently dragging their mouse around
            return rv;
        }

        OSC_PERF("scene hittest");

        // figure out mouse pos in panel's NDC system
        glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
        glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
        glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
        glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
        glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
        glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

        // un-project the mouse position as a ray in worldspace
        Line const cameraRay = m_Params.camera.unprojectTopLeftPosToWorldRay(mouseItemPos, itemDims);

        // get decorations list (used for later testing/filtering)
        nonstd::span<osc::SceneDecoration const> decorations = m_CachedModelRenderer.getDrawlist();

        // find all collisions along the camera ray
        std::vector<SceneCollision> const collisions = m_CachedModelRenderer.getAllSceneCollisions(cameraRay);

        // filter through the collisions list
        int closestIdx = -1;
        float closestDistance = std::numeric_limits<float>::max();
        glm::vec3 closestWorldLoc = {0.0f, 0.0f, 0.0f};

        for (SceneCollision const& c : collisions)
        {
            if (c.distanceFromRayOrigin > closestDistance)
            {
                continue;  // it's further away than the current closest collision
            }

            SceneDecoration const& decoration = decorations[c.decorationIndex];

            if (decoration.id.empty())
            {
                continue;  // it isn't labelled geometry, so probably shouldn't participate
            }

            closestIdx = static_cast<int>(c.decorationIndex);
            closestDistance = c.distanceFromRayOrigin;
            closestWorldLoc = c.worldspaceLocation;
        }

        if (closestIdx >= 0)
        {
            rv.first = osc::FindComponent(msp.getModel(), decorations[closestIdx].id);
            rv.second = closestWorldLoc;
        }

        return rv;
    }

    void drawImGuiOverlays()
    {
        // draw the top overlays
        ImGui::SetCursorScreenPos(m_RenderedImageHittest.rect.p1 + glm::vec2{ImGui::GetStyle().WindowPadding});
        DrawViewerTopButtonRow(m_Params, m_CachedModelRenderer.getDrawlist(), *m_IconCache, m_Ruler);

        // draw the bottom overlays
        ImGuiStyle const& style = ImGui::GetStyle();
        glm::vec2 const alignmentAxesDims = osc::CalcAlignmentAxesDimensions();
        glm::vec2 const axesTopLeft =
        {
            m_RenderedImageHittest.rect.p1.x + style.WindowPadding.x,
            m_RenderedImageHittest.rect.p2.y - style.WindowPadding.y - alignmentAxesDims.y
        };
        ImGui::SetCursorScreenPos(axesTopLeft);
        DrawAlignmentAxes(m_Params.camera.getViewMtx());
        DrawCameraControlButtons(
            m_Params.camera,
            m_RenderedImageHittest.rect,
            m_CachedModelRenderer.getRootAABB(),
            *m_IconCache
        );
    }

    // rendering-related data
    ModelRendererParams m_Params;
    CachedModelRenderer m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<MeshCache>(),
        *App::singleton<ShaderCache>(),
    };
    osc::ImGuiItemHittestResult m_RenderedImageHittest;

    // overlay-related data
    std::shared_ptr<IconCache> m_IconCache = osc::App::singleton<osc::IconCache>(
        App::resource("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
    GuiRuler m_Ruler;

    // a flag that will auto-focus the main scene camera the next time it's used
    //
    // initialized `true`, so that the initially-loaded model is autofocused (#520)
    bool m_IsRenderingFirstFrame = true;
};


// public API (PIMPL)

osc::UiModelViewer::UiModelViewer() :
    m_Impl{std::make_unique<Impl>()}
{
}
osc::UiModelViewer::UiModelViewer(UiModelViewer&&) noexcept = default;
osc::UiModelViewer& osc::UiModelViewer::operator=(UiModelViewer&&) noexcept = default;
osc::UiModelViewer::~UiModelViewer() noexcept = default;

bool osc::UiModelViewer::isLeftClicked() const
{
    return m_Impl->isLeftClicked();
}

bool osc::UiModelViewer::isRightClicked() const
{
    return m_Impl->isRightClicked();
}

bool osc::UiModelViewer::isMousedOver() const
{
    return m_Impl->isMousedOver();
}

osc::UiModelViewerResponse osc::UiModelViewer::draw(VirtualConstModelStatePair const& rs)
{
    return m_Impl->draw(rs);
}
