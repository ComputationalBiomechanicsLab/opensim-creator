#include "UiModelViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Formats/DAE.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
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
#include "src/OpenSimBindings/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/CustomRenderingOptions.hpp"
#include "src/OpenSimBindings/Icon.hpp"
#include "src/OpenSimBindings/IconCache.hpp"
#include "src/OpenSimBindings/MuscleColoringStyle.hpp"
#include "src/OpenSimBindings/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/MuscleSizingStyle.hpp"
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

// export utils
namespace
{
    // prompts the user for a save location and then exports a DAE file containing the 3D scene
    void TryExportSceneToDAE(nonstd::span<osc::SceneDecoration const> scene)
    {
        std::optional<std::filesystem::path> maybeDAEPath =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("dae");

        if (!maybeDAEPath)
        {
            return;  // user cancelled out
        }
        std::filesystem::path const& daePath = *maybeDAEPath;

        std::ofstream outfile{daePath};

        if (!outfile)
        {
            osc::log::error("cannot save to %s: IO error", daePath.string().c_str());
            return;
        }

        osc::WriteDecorationsAsDAE(scene, outfile);
        osc::log::info("wrote scene as a DAE file to %s", daePath.string().c_str());
    }
}

// rendering utils
namespace
{
    // caches + versions scene state
    class CachedScene final {
    public:
        osc::UID getVersion() const
        {
            return m_Version;
        }

        nonstd::span<osc::SceneDecoration const> getDrawlist() const
        {
            return m_Decorations;
        }

        osc::BVH const& getBVH() const
        {
            return m_BVH;
        }

        void populate(
            osc::VirtualConstModelStatePair const& msp,
            osc::CustomDecorationOptions const& decorationOptions,
            osc::CustomRenderingOptions const& renderingOptions)
        {
            OpenSim::Component const* const selected = msp.getSelected();
            OpenSim::Component const* const hovered = msp.getHovered();

            if (msp.getModelVersion() != m_LastModelVersion ||
                msp.getStateVersion() != m_LastStateVersion ||
                selected != osc::FindComponent(msp.getModel(), m_LastSelection) ||
                hovered != osc::FindComponent(msp.getModel(), m_LastHover) ||
                msp.getFixupScaleFactor() != m_LastFixupFactor ||
                decorationOptions != m_LastDecorationOptions ||
                renderingOptions != m_LastRenderingOptions)
            {
                // update cache checks
                m_LastModelVersion = msp.getModelVersion();
                m_LastStateVersion = msp.getStateVersion();
                m_LastSelection = selected ? selected->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastHover = hovered ? hovered->getAbsolutePath() : OpenSim::ComponentPath{};
                m_LastFixupFactor = msp.getFixupScaleFactor();
                m_LastDecorationOptions = decorationOptions;
                m_LastRenderingOptions = renderingOptions;
                m_Version = osc::UID{};

                // generate decorations from OpenSim/SimTK backend
                {
                    OSC_PERF("generate decorations");
                    m_Decorations.clear();
                    osc::GenerateModelDecorations(msp, m_Decorations, decorationOptions);
                }

                // create a BVH from the not-overlay parts of the scene
                osc::UpdateSceneBVH(m_Decorations, m_BVH);

                // generate screen-specific overlays
                if (renderingOptions.getDrawAABBs())
                {
                    for (size_t i = 0, len = m_Decorations.size(); i < len; ++i)
                    {
                        DrawAABB(*osc::App::singleton<osc::MeshCache>(), GetWorldspaceAABB(m_Decorations[i]), m_Decorations);
                    }
                }

                if (renderingOptions.getDrawBVH())
                {
                    DrawBVH(*osc::App::singleton<osc::MeshCache>(), m_BVH, m_Decorations);
                }

                if (renderingOptions.getDrawXZGrid())
                {
                    DrawXZGrid(*osc::App::singleton<osc::MeshCache>(), m_Decorations);
                }

                if (renderingOptions.getDrawXYGrid())
                {
                    DrawXYGrid(*osc::App::singleton<osc::MeshCache>(), m_Decorations);
                }

                if (renderingOptions.getDrawYZGrid())
                {
                    DrawYZGrid(*osc::App::singleton<osc::MeshCache>(), m_Decorations);
                }

                if (renderingOptions.getDrawAxisLines())
                {
                    DrawXZFloorLines(*osc::App::singleton<osc::MeshCache>(), m_Decorations);
                }
            }
        }

    private:
        osc::UID m_LastModelVersion;
        osc::UID m_LastStateVersion;
        OpenSim::ComponentPath m_LastSelection;
        OpenSim::ComponentPath m_LastHover;
        float m_LastFixupFactor = 1.0f;
        osc::CustomDecorationOptions m_LastDecorationOptions;
        osc::CustomRenderingOptions m_LastRenderingOptions;

        osc::UID m_Version;
        std::vector<osc::SceneDecoration> m_Decorations;
        osc::BVH m_BVH;
    };

    class IconWithoutMenu final {
    public:
        IconWithoutMenu(
            osc::CStringView iconID,
            osc::CStringView title,
            osc::CStringView description) :
            m_IconID{iconID},
            m_Title{title},
            m_Description{description}
        {
        }

        std::string const& getIconID() const
        {
            return m_IconID;
        }

        std::string const& getTitle() const
        {
            return m_Title;
        }

        bool draw()
        {
            auto const cache = osc::App::singleton<osc::IconCache>();
            osc::Icon const& icon = cache->getIcon(m_IconID);
            bool rv = osc::ImageButton(m_ButtonID, icon.getTexture(), icon.getDimensions());
            osc::DrawTooltipIfItemHovered(m_Title, m_Description);

            return rv;
        }

    private:
        std::string m_IconID;
        std::string m_ButtonID = "##" + m_IconID;
        std::string m_Title;
        std::string m_Description;
    };

    class IconWithMenu final {
    public:
        IconWithMenu(
            osc::CStringView iconID,
            osc::CStringView title,
            osc::CStringView description,
            std::function<void()> const& contentRenderer) :
            m_IconWithoutMenu{iconID, title, description},
            m_ContentRenderer{contentRenderer}
        {
        }

        void draw()
        {
            if (m_IconWithoutMenu.draw())
            {
                ImGui::OpenPopup(m_ContextMenuID.c_str());
            }

            if (ImGui::BeginPopup(m_ContextMenuID.c_str(),ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextDisabled("%s", m_IconWithoutMenu.getTitle().c_str());
                ImGui::Dummy({0.0f, 0.5f*ImGui::GetTextLineHeight()});
                m_ContentRenderer();
                ImGui::EndPopup();
            }
        }
    private:
        IconWithoutMenu m_IconWithoutMenu;
        std::string m_ContextMenuID = "##" + m_IconWithoutMenu.getIconID();
        std::function<void()> m_ContentRenderer;
    };
}

class osc::UiModelViewer::Impl final {
public:

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

    osc::UiModelViewerResponse draw(VirtualConstModelStatePair const& rs)
    {
        UiModelViewerResponse rv;

        handleUserInput();

        if (!ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove))
        {
            // render window isn't visible
            m_RenderImage = {};
            return rv;
        }

        // populate render buffers
        m_Scene.populate(rs, m_DecorationOptions, m_RenderingOptions);

        std::pair<OpenSim::Component const*, glm::vec3> htResult = hittestRenderWindow(rs);

        // auto-focus the camera, if the user requested it last frame
        //
        // care: indirectly depends on the scene drawlist being up-to-date
        if (m_AutoFocusCameraNextFrame && !m_Scene.getBVH().nodes.empty())
        {
            AutoFocus(m_Camera, m_Scene.getBVH().nodes[0].getBounds(), AspectRatio(ImGui::GetContentRegionAvail()));
            m_AutoFocusCameraNextFrame = false;
        }

        // render into texture
        drawSceneTexture(rs);

        // blit texture as an ImGui::Image
        osc::DrawTextureAsImGuiImage(m_Rendererer.updRenderTexture(), ImGui::GetContentRegionAvail());
        m_RenderImage = osc::HittestLastImguiItem();

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

            if (ImGui::IsKeyReleased(ImGuiKey_X))
            {
                if (ctrlDown)
                {
                    FocusAlongMinusX(m_Camera);
                } else
                {
                    FocusAlongX(m_Camera);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y))
            {
                if (!ctrlDown)
                {
                    FocusAlongY(m_Camera);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F))
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
            if (ctrlDown && (ImGui::IsKeyPressed(ImGuiKey_8)))
            {
                // solidworks keybind
                m_AutoFocusCameraNextFrame = true;
            }
            UpdatePolarCameraFromImGuiUserInput(Dimensions(m_RenderImage.rect), m_Camera);
        }
    }

    void drawTopButtonRowOverlay()
    {
        ImGui::SetCursorScreenPos(m_RenderImage.rect.p1 + glm::vec2{ImGui::GetStyle().WindowPadding});

        auto const cache = App::singleton<IconCache>();
        float const iconPadding = 2.0f;

        IconWithMenu muscleStylingButton
        {
            "muscle_coloring",
            "Muscle Styling",
            "Affects how muscles appear in this visualizer panel",
            [this]() { drawMuscleStylingContextMenuContent(); },
        };
        muscleStylingButton.draw();
        ImGui::SameLine();

        IconWithMenu vizAidsButton
        {
            "viz_aids",
            "Visual Aids",
            "Affects what's shown in the 3D scene",
            [this]() { drawVisualAidsContextMenuContent(); },
        };
        vizAidsButton.draw();
        ImGui::SameLine();

        IconWithMenu settingsButton
        {
            "gear",
            "Scene Settings",
            "Change advanced scene settings",
            [this]() { drawSceneMenuContent(); },
        };
        settingsButton.draw();
        ImGui::SameLine();

        IconWithoutMenu rulerButton
        {
            "ruler",
            "Ruler",
            "Roughly measure something in the scene",
        };
        if (rulerButton.draw())
        {
            m_Ruler.toggleMeasuring();
        }
    }

    void drawMuscleStylingContextMenuContent()
    {
        // style
        {
            ImGui::TextDisabled("Rendering");

            MuscleDecorationStyle const currentStyle = m_DecorationOptions.getMuscleDecorationStyle();
            nonstd::span<MuscleDecorationStyle const> const allStyles = osc::GetAllMuscleDecorationStyles();
            nonstd::span<char const* const>  const allStylesStrings = osc::GetAllMuscleDecorationStyleStrings();
            int const currentStyleIndex = osc::GetIndexOf(currentStyle);

            for (int i = 0; i < static_cast<int>(allStyles.size()); ++i)
            {
                if (ImGui::RadioButton(allStylesStrings[i], i == currentStyleIndex))
                {
                    m_DecorationOptions.setMuscleDecorationStyle(allStyles[i]);
                }
            }
        }

        // sizing
        {
            ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
            ImGui::TextDisabled("Sizing");

            MuscleSizingStyle const currentStyle = m_DecorationOptions.getMuscleSizingStyle();
            nonstd::span<MuscleSizingStyle const> const allStyles = osc::GetAllMuscleSizingStyles();
            nonstd::span<char const* const>  const allStylesStrings = osc::GetAllMuscleSizingStyleStrings();
            int const currentStyleIndex = osc::GetIndexOf(currentStyle);

            for (int i = 0; i < static_cast<int>(allStyles.size()); ++i)
            {
                if (ImGui::RadioButton(allStylesStrings[i], i == currentStyleIndex))
                {
                    m_DecorationOptions.setMuscleSizingStyle(allStyles[i]);
                }
            }
        }

        // coloring
        {
            ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
            ImGui::TextDisabled("Coloring");

            MuscleColoringStyle const currentStyle = m_DecorationOptions.getMuscleColoringStyle();
            nonstd::span<MuscleColoringStyle const> const allStyles = osc::GetAllMuscleColoringStyles();
            nonstd::span<char const* const>  const allStylesStrings = osc::GetAllMuscleColoringStyleStrings();
            int const currentStyleIndex = osc::GetIndexOf(currentStyle);

            for (int i = 0; i < static_cast<int>(allStyles.size()); ++i)
            {
                if (ImGui::RadioButton(allStylesStrings[i], i == currentStyleIndex))
                {
                    m_DecorationOptions.setMuscleColoringStyle(allStyles[i]);
                }
            }
        }
    }

    void drawVisualAidsContextMenuContent()
    {
        std::optional<ptrdiff_t> lastGroup;
        for (size_t i = 0; i < m_RenderingOptions.getNumOptions(); ++i)
        {
            // print header, if necessary
            ptrdiff_t const group = m_RenderingOptions.getOptionGroupIndex(i);
            if (group != lastGroup)
            {
                if (lastGroup)
                {
                    ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
                }
                ImGui::TextDisabled(m_RenderingOptions.getGroupLabel(group).c_str());
                lastGroup = group;
            }

            bool value = m_RenderingOptions.getOptionValue(i);
            if (ImGui::Checkbox(m_RenderingOptions.getOptionLabel(i).c_str(), &value))
            {
                m_RenderingOptions.setOptionValue(i, value);
            }
        }

        // OpenSim extras
        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        ImGui::TextDisabled("OpenSim");
        bool isDrawingScapulothoracicJoints = m_DecorationOptions.getShouldShowScapulo();
        if (ImGui::Checkbox("scapulothoracic joints", &isDrawingScapulothoracicJoints))
        {
            m_DecorationOptions.setShouldShowScapulo(isDrawingScapulothoracicJoints);
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
        DrawTooltipBodyOnlyIfItemHovered("Position camera along +X, pointing towards the center. Hotkey: X");
        ImGui::SameLine();
        if (ImGui::Button("-X"))
        {
            FocusAlongMinusX(m_Camera);
        }
        DrawTooltipBodyOnlyIfItemHovered("Position camera along -X, pointing towards the center. Hotkey: Ctrl+X");

        ImGui::SameLine();
        if (ImGui::Button("+Y"))
        {
            FocusAlongY(m_Camera);
        }
        DrawTooltipBodyOnlyIfItemHovered("Position camera along +Y, pointing towards the center. Hotkey: Y");
        ImGui::SameLine();
        if (ImGui::Button("-Y"))
        {
            FocusAlongMinusY(m_Camera);
        }
        DrawTooltipBodyOnlyIfItemHovered("Position camera along -Y, pointing towards the center. (no hotkey, because Ctrl+Y is taken by 'Redo'");

        ImGui::SameLine();
        if (ImGui::Button("+Z"))
        {
            FocusAlongZ(m_Camera);
        }
        DrawTooltipBodyOnlyIfItemHovered("Position camera along +Z, pointing towards the center. Hotkey: Z");
        ImGui::SameLine();
        if (ImGui::Button("-Z"))
        {
            FocusAlongMinusZ(m_Camera);
        }
        DrawTooltipBodyOnlyIfItemHovered("Position camera along -Z, pointing towards the center. (no hotkey, because Ctrl+Z is taken by 'Undo')");

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
        DrawTooltipBodyOnlyIfItemHovered("Reset the camera to its initial (default) location. Hotkey: F");

        if (ImGui::Button("Auto-focus camera"))
        {
            m_AutoFocusCameraNextFrame = true;
        }
        DrawTooltipBodyOnlyIfItemHovered("Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F");

        if (ImGui::Button("Export to .dae"))
        {
            TryExportSceneToDAE(m_Scene.getDrawlist());
        }
        DrawTooltipBodyOnlyIfItemHovered("Try to export the 3D scene to a portable DAE file, so that it can be viewed in 3rd-party modelling software, such as Blender");

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced camera properties:");
        ImGui::Separator();
        osc::SliderMetersFloat("radius", m_Camera.radius, 0.0f, 10.0f);
        ImGui::SliderFloat("theta", &m_Camera.theta, 0.0f, 2.0f * osc::fpi);
        ImGui::SliderFloat("phi", &m_Camera.phi, 0.0f, 2.0f * osc::fpi);
        ImGui::InputFloat("fov", &m_Camera.fov);
        osc::InputMetersFloat("znear", m_Camera.znear);
        osc::InputMetersFloat("zfar", m_Camera.zfar);
        ImGui::NewLine();
        osc::SliderMetersFloat("pan_x", m_Camera.focusPoint.x, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_y", m_Camera.focusPoint.y, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_z", m_Camera.focusPoint.z, -100.0f, 100.0f);

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced scene properties:");
        ImGui::Separator();
        ImGui::ColorEdit3("light_color", glm::value_ptr(m_LightColor));
        ImGui::ColorEdit3("background color", glm::value_ptr(m_BackgroundColor));
        osc::InputMetersFloat3("floor location", m_FloorLocation);
        DrawTooltipBodyOnlyIfItemHovered("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");
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

        OSC_PERF("scene hittest");

        // figure out mouse pos in panel's NDC system
        glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
        glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
        glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
        glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
        glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
        glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

        // un-project the mouse position as a ray in worldspace
        Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mouseItemPos, itemDims);

        // get decorations list (used for later testing/filtering)
        nonstd::span<osc::SceneDecoration const> decorations = m_Scene.getDrawlist();

        // find all collisions along the camera ray
        std::vector<SceneCollision> const collisions = GetAllSceneCollisions(m_Scene.getBVH(), decorations, cameraRay);

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

    void drawSceneTexture(osc::VirtualConstModelStatePair const& rs)
    {
        // setup render params
        SceneRendererParams params;

        glm::vec2 contentRegion = ImGui::GetContentRegionAvail();
        if (contentRegion.x >= 1.0f && contentRegion.y >= 1.0f)
        {
            params.dimensions = contentRegion;
            params.samples = osc::App::get().getMSXAASamplesRecommended();
        }

        params.lightDirection = RecommendedLightDirection(m_Camera);
        params.drawFloor = m_RenderingOptions.getDrawFloor();
        params.viewMatrix = m_Camera.getViewMtx();
        params.projectionMatrix = m_Camera.getProjMtx(AspectRatio(m_Rendererer.getDimensions()));
        params.nearClippingPlane = m_Camera.znear;
        params.farClippingPlane = m_Camera.zfar;
        params.viewPos = m_Camera.getPos();
        params.fixupScaleFactor = rs.getFixupScaleFactor();
        params.drawRims = m_RenderingOptions.getDrawSelectionRims();
        params.drawMeshNormals = m_RenderingOptions.getDrawMeshNormals();
        params.drawShadows = m_RenderingOptions.getDrawShadows();
        params.lightColor = m_LightColor;
        params.backgroundColor = m_BackgroundColor;
        params.floorLocation = m_FloorLocation;

        if (m_Scene.getVersion() != m_RendererPrevDrawlistVersion ||
            params != m_RendererPrevParams)
        {
            m_RendererPrevDrawlistVersion = m_Scene.getVersion();
            m_RendererPrevParams = params;
            m_Rendererer.draw(m_Scene.getDrawlist(), params);
        }
    }

    void drawImGuiOverlays()
    {
        drawTopButtonRowOverlay();
        drawAlignmentAxes();
        drawCameraControlButtons();
    }

    void drawAlignmentAxes()
    {
        ImGuiStyle const& style = ImGui::GetStyle();
        glm::vec2 const alignmentAxesDims = osc::CalcAlignmentAxesDimensions();
        glm::vec2 const axesTopLeft =
        {
            m_RenderImage.rect.p1.x + style.WindowPadding.x,
            m_RenderImage.rect.p2.y - style.WindowPadding.y - alignmentAxesDims.y
        };
        ImGui::SetCursorScreenPos(axesTopLeft);
        DrawAlignmentAxes(m_Camera.getViewMtx());
    }

    void drawCameraControlButtons()
    {
        ImGuiStyle const& style = ImGui::GetStyle();
        float const buttonHeight = 2.0f*style.FramePadding.y + ImGui::GetTextLineHeight();
        float const rowSpacing = ImGui::GetStyle().FramePadding.y;
        float const twoRowHeight = 2.0f*buttonHeight + rowSpacing;
        float const xFirstRow = m_RenderImage.rect.p1.x + style.WindowPadding.x + CalcAlignmentAxesDimensions().x + style.ItemSpacing.x;
        float const yFirstRow = (m_RenderImage.rect.p2.y - style.WindowPadding.y - 0.5f*CalcAlignmentAxesDimensions().y) - 0.5f*twoRowHeight;

        glm::vec2 const firstRowTopLeft = {xFirstRow, yFirstRow};
        float const midRowY = yFirstRow + 0.5f*(buttonHeight + rowSpacing);

        // draw top row
        {
            ImGui::SetCursorScreenPos(firstRowTopLeft);

            IconWithoutMenu plusXbutton
            {
                "plusx",
                "Focus Camera Along +X",
                "Rotates the camera to focus along the +X direction",
            };
            if (plusXbutton.draw())
            {
                FocusAlongX(m_Camera);
            }

            ImGui::SameLine();

            IconWithoutMenu plusYbutton
            {
                "plusy",
                "Focus Camera Along +Y",
                "Rotates the camera to focus along the +Y direction",
            };
            if (plusYbutton.draw())
            {
                FocusAlongY(m_Camera);
            }

            ImGui::SameLine();

            IconWithoutMenu plusZbutton
            {
                "plusz",
                "Focus Camera Along +Z",
                "Rotates the camera to focus along the +Z direction",
            };
            if (plusZbutton.draw())
            {
                FocusAlongZ(m_Camera);
            }

            ImGui::SameLine();
            ImGui::SameLine();

            IconWithoutMenu zoomInButton
            {
                "zoomin",
                "Zoom in Camera",
                "Moves the camera one step towards its focus point",
            };
            if (zoomInButton.draw())
            {
                ZoomIn(m_Camera);
            }
        }

        // draw bottom row
        {
            ImGui::SetCursorScreenPos({ firstRowTopLeft.x, ImGui::GetCursorScreenPos().y });

            IconWithoutMenu minusXbutton
            {
                "minusx",
                "Focus Camera Along -X",
                "Rotates the camera to focus along the -X direction",
            };
            if (minusXbutton.draw())
            {
                FocusAlongMinusX(m_Camera);
            }

            ImGui::SameLine();

            IconWithoutMenu minusYbutton
            {
                "minusy",
                "Focus Camera Along -Y",
                "Rotates the camera to focus along the -Y direction",
            };
            if (minusYbutton.draw())
            {
                FocusAlongMinusY(m_Camera);
            }

            ImGui::SameLine();

            IconWithoutMenu minusZbutton
            {
                "minusz",
                "Focus Camera Along -Z",
                "Rotates the camera to focus along the -Z direction",
            };
            if (minusZbutton.draw())
            {
                FocusAlongMinusZ(m_Camera);
            }

            ImGui::SameLine();
            ImGui::SameLine();

            IconWithoutMenu zoomOutButton
            {
                "zoomout",
                "Zoom Out Camera",
                "Moves the camera one step away from its focus point",
            };
            if (zoomOutButton.draw())
            {
                ZoomOut(m_Camera);
            }

            ImGui::SameLine();
            ImGui::SameLine();
        }

        // draw single row
        {
            ImGui::SetCursorScreenPos({ImGui::GetCursorScreenPos().x, midRowY});

            IconWithoutMenu autoFocusButton
            {
                "zoomauto",
                "Auto-Focus Camera",
                "Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F",
            };
            if (autoFocusButton.draw())
            {
                m_AutoFocusCameraNextFrame = true;
            }
        }
    }

    // scene state
    CustomDecorationOptions m_DecorationOptions;
    CustomRenderingOptions m_RenderingOptions;
    glm::vec3 m_LightColor = SceneRendererParams{}.lightColor;
    glm::vec4 m_BackgroundColor = SceneRendererParams{}.backgroundColor;
    glm::vec3 m_FloorLocation = SceneRendererParams{}.floorLocation;
    CachedScene m_Scene;
    PolarPerspectiveCamera m_Camera = CreateCameraWithRadius(5.0f);

    // rendering input state
    SceneRendererParams m_RendererPrevParams;
    UID m_RendererPrevDrawlistVersion;
    SceneRenderer m_Rendererer
    {
        osc::App::config(),
        *osc::App::singleton<osc::MeshCache>(),
        *osc::App::singleton<osc::ShaderCache>(),
    };

    // ImGui compositing/hittesting state
    osc::ImGuiItemHittestResult m_RenderImage;

    // a flag that will auto-focus the main scene camera the next time it's used
    //
    // initialized `true`, so that the initially-loaded model is autofocused (#520)
    bool m_AutoFocusCameraNextFrame = true;
    GuiRuler m_Ruler;
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
