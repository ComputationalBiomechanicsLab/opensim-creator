#include "UiModelViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Shaders/EdgeDetectionShader.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Graphics/Shaders/InstancedGouraudColorShader.hpp"
#include "src/Graphics/Shaders/InstancedSolidColorShader.hpp"
#include "src/Graphics/Shaders/NormalsShader.hpp"
#include "src/Graphics/Shaders/SolidColorShader.hpp"
#include "src/Graphics/BasicSceneElement.hpp"
#include "src/Graphics/DAEWriter.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Graphics/Texturing.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/ComponentDecoration.hpp"
#include "src/OpenSimBindings/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/ModelStateRenderer.hpp"
#include "src/OpenSimBindings/ModelStateRendererParams.hpp"
#include "src/OpenSimBindings/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/MuscleSizingStyle.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/GuiRuler.hpp"

#include <GL/glew.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_scancode.h>
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

// export utils
namespace
{
    void TryExportSceneToDAE(nonstd::span<osc::ComponentDecoration const> scene)
    {
        std::filesystem::path p =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("dae");

        std::ofstream outfile{p};

        if (!outfile)
        {
            osc::log::error("cannot save to %s: IO error", p.string().c_str());
            return;
        }

        std::vector<osc::BasicSceneElement> basicDecs;
        for (osc::ComponentDecoration const& dec : scene)
        {
            osc::BasicSceneElement& basic = basicDecs.emplace_back();
            basic.transform = dec.transform;
            basic.mesh = dec.mesh;
            basic.color = dec.color;
        }

        osc::WriteDecorationsAsDAE(basicDecs, outfile);
        osc::log::info("wrote scene as a DAE file to %s", p.string().c_str());
    }
}

// rendering utils
namespace
{
    void DrawSceneAABBs(
        nonstd::span<osc::ComponentDecoration const> decs,
        glm::mat4 const& viewMtx,
        glm::mat4 const& projMtx)
    {
        std::vector<osc::AABB> aabbs;
        aabbs.reserve(decs.size());
        for (auto const& dec : decs)
        {
            aabbs.push_back(GetWorldspaceAABB(dec));
        }
        osc::DrawAABBs(aabbs, viewMtx, projMtx);
    }

    // populates a high-level drawlist for an OpenSim model scene
    class CachedSceneDrawlist final {
    public:
        osc::UID getVersion() const
        {
            return m_Version;
        }

        nonstd::span<osc::ComponentDecoration const> get() const
        {
            return m_Decorations;
        }

        nonstd::span<osc::ComponentDecoration const> populate(
            osc::VirtualConstModelStatePair const& msp,
            osc::CustomDecorationOptions const& decorationOptions)
        {
            OpenSim::Component const* const selected = msp.getSelected();
            OpenSim::Component const* const hovered = msp.getHovered();
            OpenSim::Component const* const isolated = msp.getIsolated();

            if (msp.getModelVersion() != m_LastModelVersion ||
                msp.getStateVersion() != m_LastStateVersion ||
                selected != osc::FindComponent(msp.getModel(), m_LastSelection) ||
                hovered != osc::FindComponent(msp.getModel(), m_LastHover) ||
                isolated != osc::FindComponent(msp.getModel(), m_LastIsolation) ||
                msp.getFixupScaleFactor() != m_LastFixupFactor ||
                decorationOptions != m_LastDecorationOptions)
            {
                // update cache checks
                m_LastModelVersion = msp.getModelVersion();
                m_LastStateVersion = msp.getStateVersion();
                m_LastSelection = selected ? selected->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastHover = hovered ? hovered->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastIsolation = isolated ? isolated->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastFixupFactor = msp.getFixupScaleFactor();
                m_LastDecorationOptions = decorationOptions;
                m_Version = osc::UID{};

                // generate decorations from OpenSim/SimTK backend
                m_Decorations.clear();
                OSC_PERF("generate decorations");
                osc::GenerateModelDecorations(msp, m_Decorations, decorationOptions);
            }
            return m_Decorations;
        }

    private:
        osc::UID m_LastModelVersion;
        osc::UID m_LastStateVersion;
        OpenSim::ComponentPath m_LastSelection;
        OpenSim::ComponentPath m_LastHover;
        OpenSim::ComponentPath m_LastIsolation;
        float m_LastFixupFactor = 1.0f;
        osc::CustomDecorationOptions m_LastDecorationOptions;

        osc::UID m_Version;
        std::vector<osc::ComponentDecoration> m_Decorations;
    };

    class CachedBVH final {
    public:
        osc::UID getVersion() const
        {
            return m_Version;
        }

        osc::BVH const& get() const
        {
            return m_BVH;
        }

        osc::BVH const& populate(CachedSceneDrawlist const& drawlist)
        {
            if (drawlist.getVersion() == m_LastDrawlistVersion)
            {
                return m_BVH;
            }

            // update cache checks
            m_LastDrawlistVersion = drawlist.getVersion();
            m_Version = osc::UID{};

            OSC_PERF("generate BVH");
            osc::UpdateSceneBVH(drawlist.get(), m_BVH);
            return m_BVH;
        }
    private:
        osc::UID m_LastDrawlistVersion;
        osc::UID m_Version;
        osc::BVH m_BVH;
    };
}


// private IMPL
class osc::UiModelViewer::Impl final {
public:
    explicit Impl(UiModelViewerFlags flags) :
        m_Flags{std::move(flags)}
    {
        m_Camera.theta = fpi4;
        m_Camera.phi = fpi4;
    }

    bool isLeftClicked() const
    {
        return m_RenderImage.isLeftClickReleasedWithoutDragging;
    }

    bool isRightClicked() const
    {
        return m_RenderImage.isRightClickReleasedWithoutDragging;
    }

    bool isMousedOver() const
    {
        return m_RenderImage.isHovered;
    }

    void requestAutoFocus()
    {
        m_AutoFocusCameraNextFrame = true;
    }

    osc::UiModelViewerResponse draw(VirtualConstModelStatePair const& rs)
    {
        UiModelViewerResponse rv;

        handleUserInput();
        drawMainMenu();

        if (!ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove))
        {
            // render window isn't visible
            m_RenderImage = {};
            return rv;
        }

        recomputeSceneLightPosition();

        // populate render buffers
        m_SceneDrawlist.populate(rs, m_DecorationOptions);
        m_BVH.populate(m_SceneDrawlist);

        std::pair<OpenSim::Component const*, glm::vec3> htResult = hittestRenderWindow(rs);

        // auto-focus the camera, if the user requested it last frame
        //
        // care: indirectly depends on the scene drawlist being up-to-date
        if (m_AutoFocusCameraNextFrame && !m_BVH.get().nodes.empty())
        {
            AutoFocus(m_Camera, m_BVH.get().nodes[0].bounds);
            m_AutoFocusCameraNextFrame = false;
        }

        // render into texture
        drawSceneTexture(rs);

        // also render in-scene overlays into the texture
        drawInSceneOverlays();

        // blit texture as an ImGui::Image
        m_RenderImage = DrawTextureAsImGuiImageAndHittest(m_Rendererer.updOutputTexture(), ImGui::GetContentRegionAvail());

        // draw any ImGui-based overlays over the image
        drawImGuiOverlays();

        if (m_Ruler.isMeasuring())
        {
            std::optional<GuiRulerMouseHit> maybeHit;
            if (htResult.first)
            {
                maybeHit.emplace(htResult.first->getName(), htResult.second);
            }
            m_Ruler.draw(m_Camera, m_RenderImage.rect, maybeHit);
        }

        ImGui::EndChild();

        // handle return value

        if (!m_Ruler.isMeasuring())
        {
            // only populate response if the ruler isn't blocking hittesting etc.
            rv.hovertestResult = htResult.first;
            rv.isMousedOver = m_RenderImage.isHovered;
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
        if (m_RenderImage.isHovered)
        {
            bool ctrlDown = osc::IsCtrlOrSuperDown();

            if (ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                if (ctrlDown)
                {
                    FocusAlongMinusX(m_Camera);
                } else
                {
                    FocusAlongX(m_Camera);
                }
            }
            if (ImGui::IsKeyPressed(SDL_SCANCODE_Y))
            {
                if (!ctrlDown)
                {
                    FocusAlongY(m_Camera);
                }
            }
            if (ImGui::IsKeyPressed(SDL_SCANCODE_F))
            {
                if (ctrlDown)
                {
                    m_AutoFocusCameraNextFrame = true;
                }
                else
                {
                    Reset(m_Camera);
                }
            }
            if (ctrlDown && (ImGui::IsKeyPressed(SDL_SCANCODE_8)))
            {
                // solidworks keybind
                m_AutoFocusCameraNextFrame = true;
            }
            UpdatePolarCameraFromImGuiUserInput(Dimensions(m_RenderImage.rect), m_Camera);
        }
    }

    void drawMainMenu()
    {
        if (ImGui::BeginMenuBar())
        {
            drawMainMenuContent();
            ImGui::EndMenuBar();
        }
    }

    void drawMainMenuContent()
    {
        if (ImGui::BeginMenu("Options"))
        {
            drawOptionsMenuContent();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene"))
        {
            drawSceneMenuContent();
            ImGui::EndMenu();
        }

        drawRulerMeasurementToggleButton();
    }

    void drawRulerMeasurementToggleButton()
    {
        if (m_Ruler.isMeasuring())
        {
            if (ImGui::MenuItem(ICON_FA_RULER " measuring", nullptr, false, false))
            {
                m_Ruler.stopMeasuring();
            }
        }
        else
        {
            if (ImGui::MenuItem(ICON_FA_RULER " measure", nullptr, false, true))
            {
                m_Ruler.startMeasuring();
            }
            osc::DrawTooltipIfItemHovered("Measure distance", "EXPERIMENTAL: take a *rough* measurement of something in the scene - the UI for this needs to be improved, a lot ;)");
        }
    }

    void drawOptionsMenuContent()
    {
        drawMuscleDecorationsStyleComboBox();
        drawMuscleSizingStyleComboBox();
        drawMuscleColoringStyleComboBox();
        ImGui::Checkbox("wireframe mode", &m_RendererParams.WireframeMode);
        ImGui::Checkbox("show normals", &m_RendererParams.DrawMeshNormals);
        ImGui::Checkbox("draw rims", &m_RendererParams.DrawRims);
        ImGui::CheckboxFlags("show XZ grid", &m_Flags, osc::UiModelViewerFlags_DrawXZGrid);
        ImGui::CheckboxFlags("show XY grid", &m_Flags, osc::UiModelViewerFlags_DrawXYGrid);
        ImGui::CheckboxFlags("show YZ grid", &m_Flags, osc::UiModelViewerFlags_DrawYZGrid);
        ImGui::CheckboxFlags("show alignment axes", &m_Flags, osc::UiModelViewerFlags_DrawAlignmentAxes);
        ImGui::CheckboxFlags("show grid lines", &m_Flags, osc::UiModelViewerFlags_DrawAxisLines);
        ImGui::CheckboxFlags("show AABBs", &m_Flags, osc::UiModelViewerFlags_DrawAABBs);
        ImGui::CheckboxFlags("show BVH", &m_Flags, osc::UiModelViewerFlags_DrawBVH);
        ImGui::CheckboxFlags("show floor", &m_Flags, osc::UiModelViewerFlags_DrawFloor);
    }

    void drawMuscleDecorationsStyleComboBox()
    {
        osc::MuscleDecorationStyle s = m_DecorationOptions.getMuscleDecorationStyle();
        nonstd::span<osc::MuscleDecorationStyle const> allStyles = osc::GetAllMuscleDecorationStyles();
        nonstd::span<char const* const>  allNames = osc::GetAllMuscleDecorationStyleStrings();
        int i = osc::GetIndexOf(s);

        if (ImGui::Combo("muscle decoration style", &i, allNames.data(), static_cast<int>(allStyles.size())))
        {
            m_DecorationOptions.setMuscleDecorationStyle(allStyles[i]);
        }
    }

    void drawMuscleSizingStyleComboBox()
    {
        osc::MuscleSizingStyle s = m_DecorationOptions.getMuscleSizingStyle();
        nonstd::span<osc::MuscleSizingStyle const> allStyles = osc::GetAllMuscleSizingStyles();
        nonstd::span<char const* const>  allNames = osc::GetAllMuscleSizingStyleStrings();
        int i = osc::GetIndexOf(s);

        if (ImGui::Combo("muscle sizing style", &i, allNames.data(), static_cast<int>(allStyles.size())))
        {
            m_DecorationOptions.setMuscleSizingStyle(allStyles[i]);
        }
    }

    void drawMuscleColoringStyleComboBox()
    {
        osc::MuscleColoringStyle s = m_DecorationOptions.getMuscleColoringStyle();
        nonstd::span<osc::MuscleColoringStyle const> allStyles = osc::GetAllMuscleColoringStyles();
        nonstd::span<char const* const>  allNames = osc::GetAllMuscleColoringStyleStrings();
        int i = osc::GetIndexOf(s);

        if (ImGui::Combo("muscle coloring", &i, allNames.data(), static_cast<int>(allStyles.size())))
        {
            m_DecorationOptions.setMuscleColoringStyle(allStyles[i]);
        }
    }

    void drawSceneMenuContent()
    {
        ImGui::Text("reposition camera:");
        ImGui::Separator();

        if (ImGui::Button("+X"))
        {
            FocusAlongX(m_Camera);
        }
        DrawTooltipBodyOnly("Position camera along +X, pointing towards the center. Hotkey: X");
        ImGui::SameLine();
        if (ImGui::Button("-X"))
        {
            FocusAlongMinusX(m_Camera);
        }
        DrawTooltipBodyOnly("Position camera along -X, pointing towards the center. Hotkey: Ctrl+X");

        ImGui::SameLine();
        if (ImGui::Button("+Y"))
        {
            FocusAlongY(m_Camera);
        }
        DrawTooltipBodyOnly("Position camera along +Y, pointing towards the center. Hotkey: Y");
        ImGui::SameLine();
        if (ImGui::Button("-Y"))
        {
            FocusAlongMinusY(m_Camera);
        }
        DrawTooltipBodyOnly("Position camera along -Y, pointing towards the center. (no hotkey, because Ctrl+Y is taken by 'Redo'");

        ImGui::SameLine();
        if (ImGui::Button("+Z"))
        {
            FocusAlongZ(m_Camera);
        }
        DrawTooltipBodyOnly("Position camera along +Z, pointing towards the center. Hotkey: Z");
        ImGui::SameLine();
        if (ImGui::Button("-Z"))
        {
            FocusAlongMinusZ(m_Camera);
        }
        DrawTooltipBodyOnly("Position camera along -Z, pointing towards the center. (no hotkey, because Ctrl+Z is taken by 'Undo')");

        if (ImGui::Button("Zoom in"))
        {
            ZoomIn(m_Camera);
        }

        ImGui::SameLine();
        if (ImGui::Button("Zoom out"))
        {
            ZoomOut(m_Camera);
        }

        if (ImGui::Button("reset camera"))
        {
            Reset(m_Camera);
        }
        DrawTooltipBodyOnly("Reset the camera to its initial (default) location. Hotkey: F");

        if (ImGui::Button("Auto-focus camera"))
        {
            m_AutoFocusCameraNextFrame = true;
        }
        DrawTooltipBodyOnly("Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F");

        if (ImGui::Button("Export to .dae"))
        {
            TryExportSceneToDAE(m_SceneDrawlist.get());

        }
        DrawTooltipBodyOnly("Try to export the 3D scene to a portable DAE file, so that it can be viewed in 3rd-party modelling software, such as Blender");

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced camera properties:");
        ImGui::Separator();
        osc::SliderMetersFloat("radius", &m_Camera.radius, 0.0f, 10.0f);
        ImGui::SliderFloat("theta", &m_Camera.theta, 0.0f, 2.0f * osc::fpi);
        ImGui::SliderFloat("phi", &m_Camera.phi, 0.0f, 2.0f * osc::fpi);
        ImGui::InputFloat("fov", &m_Camera.fov);
        osc::InputMetersFloat("znear", &m_Camera.znear);
        osc::InputMetersFloat("zfar", &m_Camera.zfar);
        ImGui::NewLine();
        osc::SliderMetersFloat("pan_x", &m_Camera.focusPoint.x, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_y", &m_Camera.focusPoint.y, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_z", &m_Camera.focusPoint.z, -100.0f, 100.0f);

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced scene properties:");
        ImGui::Separator();
        ImGui::ColorEdit3("light_color", glm::value_ptr(m_RendererParams.LightColor));
        ImGui::ColorEdit3("background color", glm::value_ptr(m_RendererParams.BackgroundColor));
        osc::InputMetersFloat3("floor location", glm::value_ptr(m_RendererParams.FloorLocation));
        DrawTooltipBodyOnly("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");
    }

    void recomputeSceneLightPosition()
    {
        // automatically change lighting position based on camera position
        glm::vec3 p = glm::normalize(-m_Camera.focusPoint - m_Camera.getPos());
        glm::vec3 up = {0.0f, 1.0f, 0.0f};
        glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.05f * fpi4, up) * glm::vec4{p, 0.0f};
        m_RendererParams.LightDirection = glm::normalize(mp + -up);
    }

    std::pair<OpenSim::Component const*, glm::vec3> hittestRenderWindow(osc::VirtualConstModelStatePair const& msp)
    {
        std::pair<OpenSim::Component const*, glm::vec3> rv = {nullptr, {0.0f, 0.0f, 0.0f}};

        if (!m_RenderImage.isHovered ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            // only do the hit test if the user isn't currently dragging their mouse around
            return rv;
        }

        // figure out mouse pos in panel's NDC system
        glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
        glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
        glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
        glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
        glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
        glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

        // un-project the mouse position as a ray in worldspace
        osc::Line cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mouseItemPos, itemDims);

        // use scene BVH to intersect that ray with the scene
        std::vector<BVHCollision> m_SceneHittestResults;
        BVH_GetRayAABBCollisions(m_BVH.get(), cameraRay, &m_SceneHittestResults);

        // go through triangle BVHes to figure out which, if any, triangle is closest intersecting
        int closestIdx = -1;
        float closestDistance = std::numeric_limits<float>::max();
        glm::vec3 closestWorldLoc = {0.0f, 0.0f, 0.0f};

        // iterate through each scene-level hit and perform a triangle-level hittest
        nonstd::span<osc::ComponentDecoration const> decs = m_SceneDrawlist.get();
        OpenSim::Component const* const isolated = msp.getIsolated();

        for (osc::BVHCollision const& c : m_SceneHittestResults)
        {
            int instanceIdx = c.primId;

            if (isolated && !IsInclusiveChildOf(isolated, decs[instanceIdx].component))
            {
                continue;  // it's not in the current isolation
            }

            glm::mat4 instanceMmtx = ToMat4(decs[instanceIdx].transform);
            osc::Line cameraRayModelspace = TransformLine(cameraRay, ToInverseMat4(decs[instanceIdx].transform));

            auto maybeCollision = decs[instanceIdx].mesh->getClosestRayTriangleCollisionModelspace(cameraRayModelspace);

            if (maybeCollision && maybeCollision.distance < closestDistance)
            {
                closestIdx = instanceIdx;
                closestDistance = maybeCollision.distance;
                glm::vec3 closestLocModelspace = cameraRayModelspace.origin + closestDistance*cameraRayModelspace.dir;
                closestWorldLoc = glm::vec3{instanceMmtx * glm::vec4{closestLocModelspace, 1.0f}};
            }
        }

        if (closestIdx >= 0)
        {
            rv.first = decs[closestIdx].component;
            rv.second = closestWorldLoc;
        }

        return rv;
    }

    void drawSceneTexture(osc::VirtualConstModelStatePair const& rs)
    {
        // setup render params
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        if (contentRegion.x >= 1.0f && contentRegion.y >= 1.0f)
        {
            glm::ivec2 dims{static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y)};
            m_RendererParams.Dimensions = dims;
            m_RendererParams.Samples = osc::App::get().getMSXAASamplesRecommended();
        }

        m_RendererParams.DrawFloor = m_Flags & UiModelViewerFlags_DrawFloor;
        m_RendererParams.ViewMatrix = m_Camera.getViewMtx();
        m_RendererParams.ProjectionMatrix = m_Camera.getProjMtx(AspectRatio(m_Rendererer.getDimensions()));
        m_RendererParams.ViewPos = m_Camera.getPos();
        m_RendererParams.FixupScaleFactor = rs.getFixupScaleFactor();

        if (m_SceneDrawlist.getVersion() != m_RendererPrevDrawlistVersion ||
            m_RendererParams != m_RendererPrevParams)
        {
            m_RendererPrevDrawlistVersion = m_SceneDrawlist.getVersion();
            m_RendererPrevParams = m_RendererParams;
            m_Rendererer.draw(m_SceneDrawlist.get(), m_RendererParams);
        }
    }

    // draws overlays that are "in scene" - i.e. they are part of the rendered texture
    void drawInSceneOverlays()
    {
        glm::mat4 viewMtx = m_Camera.getViewMtx();
        glm::mat4 projMtx = m_Camera.getProjMtx(AspectRatio(m_Rendererer.getDimensions()));

        gl::BindFramebuffer(GL_FRAMEBUFFER, m_Rendererer.updOutputFBO());
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        if (m_Flags & osc::UiModelViewerFlags_DrawXZGrid)
        {
            DrawXZGrid(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawXYGrid)
        {
            DrawXYGrid(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawYZGrid)
        {
            DrawYZGrid(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawAxisLines)
        {
            DrawXZFloorLines(viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawAABBs)
        {
            DrawSceneAABBs(m_SceneDrawlist.get(), viewMtx, projMtx);
        }

        if (m_Flags & osc::UiModelViewerFlags_DrawBVH)
        {
            DrawBVH(m_BVH.get(), viewMtx, projMtx);
        }

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }

    void drawImGuiOverlays()
    {
        if (m_Flags & osc::UiModelViewerFlags_DrawAlignmentAxes)
        {
            DrawAlignmentAxesOverlayInBottomRightOf(m_Camera.getViewMtx(), m_RenderImage.rect);
        }
    }

    // widget state
    UiModelViewerFlags m_Flags = UiModelViewerFlags_Default;
    CustomDecorationOptions m_DecorationOptions;

    // scene state
    CachedSceneDrawlist m_SceneDrawlist;
    CachedBVH m_BVH;
    PolarPerspectiveCamera m_Camera = CreateCameraWithRadius(5.0f);

    // rendering input state
    ModelStateRendererParams m_RendererParams;
    ModelStateRendererParams m_RendererPrevParams;
    UID m_RendererPrevDrawlistVersion;
    ModelStateRenderer m_Rendererer;

    // ImGui compositing/hittesting state
    ImGuiImageHittestResult m_RenderImage;

    // other stuff
    bool m_AutoFocusCameraNextFrame = false;
    GuiRuler m_Ruler;
};


// public API

osc::UiModelViewer::UiModelViewer(UiModelViewerFlags flags) :
    m_Impl{new Impl{flags}}
{
}

osc::UiModelViewer::UiModelViewer(UiModelViewer&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::UiModelViewer& osc::UiModelViewer::operator=(UiModelViewer&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::UiModelViewer::~UiModelViewer() noexcept
{
    delete m_Impl;
}

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

void osc::UiModelViewer::requestAutoFocus()
{
    m_Impl->requestAutoFocus();
}

osc::UiModelViewerResponse osc::UiModelViewer::draw(VirtualConstModelStatePair const& rs)
{
    return m_Impl->draw(rs);
}
