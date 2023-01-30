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

// icon utils
namespace
{
    class IconWithoutMenu final {
    public:
        IconWithoutMenu(
            osc::Icon icon,
            osc::CStringView title,
            osc::CStringView description) :
            m_Icon{std::move(icon)},
            m_Title{title},
            m_Description{description}
        {
        }

        std::string const& getIconID() const
        {
            return m_ButtonID;
        }

        std::string const& getTitle() const
        {
            return m_Title;
        }

        bool draw()
        {
            bool rv = osc::ImageButton(m_ButtonID, m_Icon.getTexture(), m_Icon.getDimensions());
            osc::DrawTooltipIfItemHovered(m_Title, m_Description);

            return rv;
        }

    private:
        osc::Icon m_Icon;
        std::string m_Title;
        std::string m_ButtonID = "##" + m_Title;
        std::string m_Description;
    };

    class IconWithMenu final {
    public:
        IconWithMenu(
            osc::Icon icon,
            osc::CStringView title,
            osc::CStringView description,
            std::function<void()> const& contentRenderer) :
            m_IconWithoutMenu{icon, title, description},
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

// UI widgets (dropdowns etc.)
namespace
{
    void DrawMuscleRenderingOptionsRadioButtions(osc::CustomDecorationOptions& opts)
    {
        osc::MuscleDecorationStyle const currentStyle = opts.getMuscleDecorationStyle();
        nonstd::span<osc::MuscleDecorationStyle const> const allStyles = osc::GetAllMuscleDecorationStyles();
        nonstd::span<char const* const>  const allStylesStrings = osc::GetAllMuscleDecorationStyleStrings();
        int const currentStyleIndex = osc::GetIndexOf(currentStyle);

        for (int i = 0; i < static_cast<int>(allStyles.size()); ++i)
        {
            if (ImGui::RadioButton(allStylesStrings[i], i == currentStyleIndex))
            {
                opts.setMuscleDecorationStyle(allStyles[i]);
            }
        }
    }

    void DrawMuscleSizingOptionsRadioButtons(osc::CustomDecorationOptions& opts)
    {
        osc::MuscleSizingStyle const currentStyle = opts.getMuscleSizingStyle();
        nonstd::span<osc::MuscleSizingStyle const> const allStyles = osc::GetAllMuscleSizingStyles();
        nonstd::span<char const* const>  const allStylesStrings = osc::GetAllMuscleSizingStyleStrings();
        int const currentStyleIndex = osc::GetIndexOf(currentStyle);

        for (int i = 0; i < static_cast<int>(allStyles.size()); ++i)
        {
            if (ImGui::RadioButton(allStylesStrings[i], i == currentStyleIndex))
            {
                opts.setMuscleSizingStyle(allStyles[i]);
            }
        }
    }

    void DrawMuscleColoringOptionsRadioButtons(osc::CustomDecorationOptions& opts)
    {
        osc::MuscleColoringStyle const currentStyle = opts.getMuscleColoringStyle();
        nonstd::span<osc::MuscleColoringStyle const> const allStyles = osc::GetAllMuscleColoringStyles();
        nonstd::span<char const* const>  const allStylesStrings = osc::GetAllMuscleColoringStyleStrings();
        int const currentStyleIndex = osc::GetIndexOf(currentStyle);

        for (int i = 0; i < static_cast<int>(allStyles.size()); ++i)
        {
            if (ImGui::RadioButton(allStylesStrings[i], i == currentStyleIndex))
            {
                opts.setMuscleColoringStyle(allStyles[i]);
            }
        }
    }

    void DrawMuscleDecorationOptionsEditor(osc::CustomDecorationOptions& opts)
    {
        int id = 0;

        ImGui::PushID(id++);
        ImGui::TextDisabled("Rendering");
        DrawMuscleRenderingOptionsRadioButtions(opts);
        ImGui::PopID();

        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        ImGui::PushID(id++);
        ImGui::TextDisabled("Sizing");
        DrawMuscleSizingOptionsRadioButtons(opts);
        ImGui::PopID();

        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        ImGui::PushID(id++);
        ImGui::TextDisabled("Coloring");
        DrawMuscleColoringOptionsRadioButtons(opts);
        ImGui::PopID();
    }

    void DrawRenderingOptionsEditor(osc::CustomRenderingOptions& opts)
    {
        std::optional<ptrdiff_t> lastGroup;
        for (size_t i = 0; i < opts.getNumOptions(); ++i)
        {
            // print header, if necessary
            ptrdiff_t const group = opts.getOptionGroupIndex(i);
            if (group != lastGroup)
            {
                if (lastGroup)
                {
                    ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
                }
                ImGui::TextDisabled("%s", opts.getGroupLabel(group).c_str());
                lastGroup = group;
            }

            bool value = opts.getOptionValue(i);
            if (ImGui::Checkbox(opts.getOptionLabel(i).c_str(), &value))
            {
                opts.setOptionValue(i, value);
            }
        }
    }

    void DrawAdvancedParamsEditor(osc::ModelRendererParams& params, nonstd::span<osc::SceneDecoration const> drawlist)
    {
        ImGui::Text("reposition camera:");
        ImGui::Separator();

        if (ImGui::Button("+X"))
        {
            osc::FocusAlongX(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along +X, pointing towards the center. Hotkey: X");
        ImGui::SameLine();
        if (ImGui::Button("-X"))
        {
            osc::FocusAlongMinusX(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along -X, pointing towards the center. Hotkey: Ctrl+X");

        ImGui::SameLine();
        if (ImGui::Button("+Y"))
        {
            osc::FocusAlongY(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along +Y, pointing towards the center. Hotkey: Y");
        ImGui::SameLine();
        if (ImGui::Button("-Y"))
        {
            osc::FocusAlongMinusY(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along -Y, pointing towards the center. (no hotkey, because Ctrl+Y is taken by 'Redo'");

        ImGui::SameLine();
        if (ImGui::Button("+Z"))
        {
            osc::FocusAlongZ(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along +Z, pointing towards the center. Hotkey: Z");
        ImGui::SameLine();
        if (ImGui::Button("-Z"))
        {
            osc::FocusAlongMinusZ(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along -Z, pointing towards the center. (no hotkey, because Ctrl+Z is taken by 'Undo')");

        if (ImGui::Button("Zoom in"))
        {
            osc::ZoomIn(params.camera);
        }

        ImGui::SameLine();
        if (ImGui::Button("Zoom out"))
        {
            osc::ZoomOut(params.camera);
        }

        if (ImGui::Button("reset camera"))
        {
            osc::Reset(params.camera);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Reset the camera to its initial (default) location. Hotkey: F");

        if (ImGui::Button("Export to .dae"))
        {
            TryExportSceneToDAE(drawlist);
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Try to export the 3D scene to a portable DAE file, so that it can be viewed in 3rd-party modelling software, such as Blender");

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced camera properties:");
        ImGui::Separator();
        osc::SliderMetersFloat("radius", params.camera.radius, 0.0f, 10.0f);
        ImGui::SliderFloat("theta", &params.camera.theta, 0.0f, 2.0f * osc::fpi);
        ImGui::SliderFloat("phi", &params.camera.phi, 0.0f, 2.0f * osc::fpi);
        ImGui::InputFloat("fov", &params.camera.fov);
        osc::InputMetersFloat("znear", params.camera.znear);
        osc::InputMetersFloat("zfar", params.camera.zfar);
        ImGui::NewLine();
        osc::SliderMetersFloat("pan_x", params.camera.focusPoint.x, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_y", params.camera.focusPoint.y, -100.0f, 100.0f);
        osc::SliderMetersFloat("pan_z", params.camera.focusPoint.z, -100.0f, 100.0f);

        ImGui::Dummy({0.0f, 10.0f});
        ImGui::Text("advanced scene properties:");
        ImGui::Separator();
        ImGui::ColorEdit3("light_color", glm::value_ptr(params.lightColor));
        ImGui::ColorEdit3("background color", glm::value_ptr(params.backgroundColor));
        osc::InputMetersFloat3("floor location", params.floorLocation);
        osc::DrawTooltipBodyOnlyIfItemHovered("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");
    }

    void DrawVisualAidsContextMenuContent(osc::ModelRendererParams& params)
    {
        // generic rendering options
        DrawRenderingOptionsEditor(params.renderingOptions);

        // OpenSim-specific extra rendering options
        ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
        ImGui::TextDisabled("OpenSim");
        bool isDrawingScapulothoracicJoints = params.decorationOptions.getShouldShowScapulo();
        if (ImGui::Checkbox("Scapulothoracic Joints", &isDrawingScapulothoracicJoints))
        {
            params.decorationOptions.setShouldShowScapulo(isDrawingScapulothoracicJoints);
        }

        bool isShowingEffectiveLinesOfAction = params.decorationOptions.getShouldShowEffectiveMuscleLinesOfAction();
        if (ImGui::Checkbox("Lines of Action (effective)", &isShowingEffectiveLinesOfAction))
        {
            params.decorationOptions.setShouldShowEffectiveMuscleLinesOfAction(isShowingEffectiveLinesOfAction);
        }
        ImGui::SameLine();
        osc::DrawHelpMarker("Draws direction vectors that show the effective mechanical effect of the muscle action on the attached body.\n\n'Effective' refers to the fact that this algorithm computes the 'effective' attachment position of the muscle, which can change because of muscle wrapping and via point calculations (see: section 5.4.3 of Yamaguchi's book 'Dynamic Modeling of Musculoskeletal Motion: A Vectorized Approach for Biomechanical Analysis in Three Dimensions', title 'EFFECTIVE ORIGIN AND INSERTION POINTS').\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!");

        bool isShowingAnatomicalLinesOfAction = params.decorationOptions.getShouldShowAnatomicalMuscleLinesOfAction();
        if (ImGui::Checkbox("Lines of Action (anatomical)", &isShowingAnatomicalLinesOfAction))
        {
            params.decorationOptions.setShouldShowAnatomicalMuscleLinesOfAction(isShowingAnatomicalLinesOfAction);
        }
        ImGui::SameLine();
        osc::DrawHelpMarker("Draws direction vectors that show the mechanical effect of the muscle action on the bodies attached to the origin/insertion points.\n\n'Anatomical' here means 'the first/last points of the muscle path' see the documentation for 'effective' lines of action for contrast.\n\nOpenSim Creator's implementation of this algorithm is based on Luca Modenese (@modenaxe)'s implementation here:\n\n    - https://github.com/modenaxe/MuscleForceDirection\n\nThanks to @modenaxe for open-sourcing the original algorithm!");

        bool isShowingCentersOfMass = params.decorationOptions.getShouldShowCentersOfMass();
        if (ImGui::Checkbox("Centers of Mass", &isShowingCentersOfMass))
        {
            params.decorationOptions.setShouldShowCentersOfMass(isShowingCentersOfMass);
        }

        bool isShowingPointToPointSprings = params.decorationOptions.getShouldShowPointToPointSprings();
        if (ImGui::Checkbox("Point-to-Point Springs", &isShowingPointToPointSprings))
        {
            params.decorationOptions.setShouldShowPointToPointSprings(isShowingPointToPointSprings);
        }
    }

    void DrawViewerTopButtonRow(
        osc::ModelRendererParams& params,
        osc::CachedModelRenderer& modelRenderer,
        osc::IconCache& iconCache,
        osc::GuiRuler& ruler)
    {
        float const iconPadding = 2.0f;

        IconWithMenu muscleStylingButton
        {
            iconCache.getIcon("muscle_coloring"),
            "Muscle Styling",
            "Affects how muscles appear in this visualizer panel",
            [&params]() { DrawMuscleDecorationOptionsEditor(params.decorationOptions); },
        };
        muscleStylingButton.draw();
        ImGui::SameLine();

        IconWithMenu vizAidsButton
        {
            iconCache.getIcon("viz_aids"),
            "Visual Aids",
            "Affects what's shown in the 3D scene",
            [&params]() { DrawVisualAidsContextMenuContent(params); },
        };
        vizAidsButton.draw();
        ImGui::SameLine();

        IconWithMenu settingsButton
        {
            iconCache.getIcon("gear"),
            "Scene Settings",
            "Change advanced scene settings",
            [&params, &modelRenderer]() { DrawAdvancedParamsEditor(params, modelRenderer.getDrawlist()); },
        };
        settingsButton.draw();
        ImGui::SameLine();

        IconWithoutMenu rulerButton
        {
            iconCache.getIcon("ruler"),
            "Ruler",
            "Roughly measure something in the scene",
        };
        if (rulerButton.draw())
        {
            ruler.toggleMeasuring();
        }
    }

    enum class CameraControlsReply {
        None,
        ShouldAutoFocusCamera,
    };

    CameraControlsReply DrawCameraControlButtons(
        osc::PolarPerspectiveCamera& camera,
        osc::ImGuiItemHittestResult const& hittest,
        osc::IconCache& iconCache)
    {
        ImGuiStyle const& style = ImGui::GetStyle();
        float const buttonHeight = 2.0f*style.FramePadding.y + ImGui::GetTextLineHeight();
        float const rowSpacing = ImGui::GetStyle().FramePadding.y;
        float const twoRowHeight = 2.0f*buttonHeight + rowSpacing;
        float const xFirstRow = hittest.rect.p1.x + style.WindowPadding.x + osc::CalcAlignmentAxesDimensions().x + style.ItemSpacing.x;
        float const yFirstRow = (hittest.rect.p2.y - style.WindowPadding.y - 0.5f*osc::CalcAlignmentAxesDimensions().y) - 0.5f*twoRowHeight;

        glm::vec2 const firstRowTopLeft = {xFirstRow, yFirstRow};
        float const midRowY = yFirstRow + 0.5f*(buttonHeight + rowSpacing);

        // draw top row
        {
            ImGui::SetCursorScreenPos(firstRowTopLeft);

            IconWithoutMenu plusXbutton
            {
                iconCache.getIcon("plusx"),
                "Focus Camera Along +X",
                "Rotates the camera to focus along the +X direction",
            };
            if (plusXbutton.draw())
            {
                FocusAlongX(camera);
            }

            ImGui::SameLine();

            IconWithoutMenu plusYbutton
            {
                iconCache.getIcon("plusy"),
                "Focus Camera Along +Y",
                "Rotates the camera to focus along the +Y direction",
            };
            if (plusYbutton.draw())
            {
                FocusAlongY(camera);
            }

            ImGui::SameLine();

            IconWithoutMenu plusZbutton
            {
                iconCache.getIcon("plusz"),
                "Focus Camera Along +Z",
                "Rotates the camera to focus along the +Z direction",
            };
            if (plusZbutton.draw())
            {
                FocusAlongZ(camera);
            }

            ImGui::SameLine();
            ImGui::SameLine();

            IconWithoutMenu zoomInButton
            {
                iconCache.getIcon("zoomin"),
                "Zoom in Camera",
                "Moves the camera one step towards its focus point",
            };
            if (zoomInButton.draw())
            {
                ZoomIn(camera);
            }
        }

        // draw bottom row
        {
            ImGui::SetCursorScreenPos({ firstRowTopLeft.x, ImGui::GetCursorScreenPos().y });

            IconWithoutMenu minusXbutton
            {
                iconCache.getIcon("minusx"),
                "Focus Camera Along -X",
                "Rotates the camera to focus along the -X direction",
            };
            if (minusXbutton.draw())
            {
                FocusAlongMinusX(camera);
            }

            ImGui::SameLine();

            IconWithoutMenu minusYbutton
            {
                iconCache.getIcon("minusy"),
                "Focus Camera Along -Y",
                "Rotates the camera to focus along the -Y direction",
            };
            if (minusYbutton.draw())
            {
                FocusAlongMinusY(camera);
            }

            ImGui::SameLine();

            IconWithoutMenu minusZbutton
            {
                iconCache.getIcon("minusz"),
                "Focus Camera Along -Z",
                "Rotates the camera to focus along the -Z direction",
            };
            if (minusZbutton.draw())
            {
                FocusAlongMinusZ(camera);
            }

            ImGui::SameLine();
            ImGui::SameLine();

            IconWithoutMenu zoomOutButton
            {
                iconCache.getIcon("zoomout"),
                "Zoom Out Camera",
                "Moves the camera one step away from its focus point",
            };
            if (zoomOutButton.draw())
            {
                ZoomOut(camera);
            }

            ImGui::SameLine();
            ImGui::SameLine();
        }

        CameraControlsReply rv = CameraControlsReply::None;

        // draw single row
        {
            ImGui::SetCursorScreenPos({ImGui::GetCursorScreenPos().x, midRowY});

            IconWithoutMenu autoFocusButton
            {
                iconCache.getIcon("zoomauto"),
                "Auto-Focus Camera",
                "Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F",
            };
            if (autoFocusButton.draw())
            {
                rv = CameraControlsReply::ShouldAutoFocusCamera;
            }
        }

        return rv;
    }
}

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

        // auto-focus the camera, if the user requested it last frame
        //
        // care: indirectly depends on the scene drawlist being up-to-date
        if (m_AutoFocusCameraNextFrame && m_CachedModelRenderer.getRootAABB())
        {
            AutoFocus(m_Params.camera, *m_CachedModelRenderer.getRootAABB(), AspectRatio(ImGui::GetContentRegionAvail()));
            m_AutoFocusCameraNextFrame = false;
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
        osc::DrawTextureAsImGuiImage(m_CachedModelRenderer.updRenderTexture(), ImGui::GetContentRegionAvail());
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
                    m_AutoFocusCameraNextFrame = true;
                }
                else
                {
                    Reset(m_Params.camera);
                }
            }
            if (ctrlDown && (ImGui::IsKeyPressed(ImGuiKey_8)))
            {
                // solidworks keybind
                m_AutoFocusCameraNextFrame = true;
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
        ImGui::SetCursorScreenPos(m_RenderedImageHittest.rect.p1 + glm::vec2{ImGui::GetStyle().WindowPadding});

        DrawViewerTopButtonRow(m_Params, m_CachedModelRenderer, *m_IconCache, m_Ruler);

        {
            ImGuiStyle const& style = ImGui::GetStyle();
            glm::vec2 const alignmentAxesDims = osc::CalcAlignmentAxesDimensions();
            glm::vec2 const axesTopLeft =
            {
                m_RenderedImageHittest.rect.p1.x + style.WindowPadding.x,
                m_RenderedImageHittest.rect.p2.y - style.WindowPadding.y - alignmentAxesDims.y
            };
            ImGui::SetCursorScreenPos(axesTopLeft);
        }
        DrawAlignmentAxes(m_Params.camera.getViewMtx());


        CameraControlsReply reply = DrawCameraControlButtons(
            m_Params.camera,
            m_RenderedImageHittest,
            *m_IconCache
        );
        m_AutoFocusCameraNextFrame = reply == CameraControlsReply::ShouldAutoFocusCamera;
    }

    // rendering parameters
    ModelRendererParams m_Params;
    CachedModelRenderer m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<MeshCache>(),
        *App::singleton<ShaderCache>(),
    };

    // ImGui compositing/hittesting state
    std::shared_ptr<IconCache> m_IconCache = osc::App::singleton<osc::IconCache>(
        App::resource("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
    osc::ImGuiItemHittestResult m_RenderedImageHittest;

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
