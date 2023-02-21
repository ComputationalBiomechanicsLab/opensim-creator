#include "BasicWidgets.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Formats/DAE.hpp"
#include "src/Graphics/IconCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Rendering/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Rendering/CustomRenderingOptions.hpp"
#include "src/OpenSimBindings/Rendering/ModelRendererParams.hpp"
#include "src/OpenSimBindings/Rendering/MuscleDecorationStyle.hpp"
#include "src/OpenSimBindings/Rendering/MuscleSizingStyle.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/ParamValue.hpp"
#include "src/OpenSimBindings/SimulationModelStatePair.hpp"
#include "src/OpenSimBindings/VirtualModelStatePair.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Widgets/GuiRuler.hpp"
#include "src/Widgets/IconWithMenu.hpp"

#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon/basics.h>

#include <cstdio>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>

// export utils
namespace
{
    // prompts the user for a save location and then exports a DAE file containing the 3D scene
    void TryPromptUserToSaveAsDAE(nonstd::span<osc::SceneDecoration const> scene)
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

    void DrawOutputTooltip(OpenSim::AbstractOutput const& o)
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", o.getTypeName().c_str());
        ImGui::EndTooltip();
    }

    bool DrawOutputWithSubfieldsMenu(osc::MainUIStateAPI& api, OpenSim::AbstractOutput const& o)
    {
        bool outputAdded = false;
        int supportedSubfields = osc::GetSupportedSubfields(o);

        // can plot suboutputs
        if (ImGui::BeginMenu(("  " + o.getName()).c_str()))
        {
            for (osc::OutputSubfield f : osc::GetAllSupportedOutputSubfields())
            {
                if (static_cast<int>(f) & supportedSubfields)
                {
                    if (ImGui::MenuItem(GetOutputSubfieldLabel(f)))
                    {
                        api.addUserOutputExtractor(osc::OutputExtractor{osc::ComponentOutputExtractor{o, f}});
                        outputAdded = true;
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::IsItemHovered())
        {
            DrawOutputTooltip(o);
        }

        return outputAdded;
    }

    bool DrawOutputWithNoSubfieldsMenuItem(osc::MainUIStateAPI& api, OpenSim::AbstractOutput const& o)
    {
        // can only plot top-level of output

        bool outputAdded = false;

        if (ImGui::MenuItem(("  " + o.getName()).c_str()))
        {
            api.addUserOutputExtractor(osc::OutputExtractor{osc::ComponentOutputExtractor{o}});
            outputAdded = true;
        }

        if (ImGui::IsItemHovered())
        {
            DrawOutputTooltip(o);
        }

        return outputAdded;
    }

    bool DrawRequestOutputMenuOrMenuItem(osc::MainUIStateAPI& api, OpenSim::AbstractOutput const& o)
    {
        if (osc::GetSupportedSubfields(o) == static_cast<int>(osc::OutputSubfield::None))
        {
            return DrawOutputWithNoSubfieldsMenuItem(api, o);
        }
        else
        {
            return DrawOutputWithSubfieldsMenu(api, o);
        }
    }

    void DrawSimulationParamValue(osc::ParamValue const& v)
    {
        if (std::holds_alternative<double>(v))
        {
            ImGui::Text("%f", static_cast<float>(std::get<double>(v)));
        }
        else if (std::holds_alternative<osc::IntegratorMethod>(v))
        {
            ImGui::Text("%s", osc::GetIntegratorMethodString(std::get<osc::IntegratorMethod>(v)));
        }
        else if (std::holds_alternative<int>(v))
        {
            ImGui::Text("%i", std::get<int>(v));
        }
        else
        {
            ImGui::Text("(unknown value type)");
        }
    }
}


// public API

void osc::DrawComponentHoverTooltip(OpenSim::Component const& hovered)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);

    ImGui::TextUnformatted(hovered.getName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%s", hovered.getConcreteClassName().c_str());

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::DrawSelectOwnerMenu(osc::VirtualModelStatePair& model, OpenSim::Component const& selected)
{
    if (ImGui::BeginMenu("Select Owner"))
    {
        OpenSim::Component const* c = &selected;
        model.setHovered(nullptr);
        while (c->hasOwner())
        {
            c = &c->getOwner();

            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());

            if (ImGui::MenuItem(buf))
            {
                model.setSelected(c);
            }
            if (ImGui::IsItemHovered())
            {
                model.setHovered(c);
            }
        }
        ImGui::EndMenu();
    }
}

bool osc::DrawWatchOutputMenu(MainUIStateAPI& api, OpenSim::Component const& c)
{
    bool outputAdded = false;

    if (ImGui::BeginMenu("Watch Output"))
    {
        osc::DrawHelpMarker("Watch the selected output. This makes it appear in the 'Output Watches' window in the editor panel and the 'Output Plots' window during a simulation");

        // iterate from the selected component upwards to the root
        int imguiId = 0;
        OpenSim::Component const* p = &c;
        while (p)
        {
            ImGui::PushID(imguiId++);

            ImGui::Dummy({0.0f, 2.0f});
            ImGui::TextDisabled("%s (%s)", p->getName().c_str(), p->getConcreteClassName().c_str());
            ImGui::Separator();

            if (p->getNumOutputs() == 0)
            {
                ImGui::TextDisabled("  (has no outputs)");
            }
            else
            {
                for (auto const& [name, output] : p->getOutputs())
                {
                    if (DrawRequestOutputMenuOrMenuItem(api, *output))
                    {
                        outputAdded = true;
                    }
                }
            }

            ImGui::PopID();

            p = p->hasOwner() ? &p->getOwner() : nullptr;
        }

        ImGui::EndMenu();
    }

    return outputAdded;
}

void osc::DrawSimulationParams(osc::ParamBlock const& params)
{
    ImGui::Dummy({0.0f, 1.0f});
    ImGui::TextUnformatted("parameters:");
    ImGui::SameLine();
    osc::DrawHelpMarker("The parameters used when this simulation was launched. These must be set *before* running the simulation");
    ImGui::Separator();
    ImGui::Dummy({0.0f, 2.0f});

    ImGui::Columns(2);
    for (int i = 0, len = params.size(); i < len; ++i)
    {
        std::string const& name = params.getName(i);
        std::string const& description = params.getDescription(i);
        osc::ParamValue const& value = params.getValue(i);

        ImGui::TextUnformatted(name.c_str());
        ImGui::SameLine();
        osc::DrawHelpMarker(name, description);
        ImGui::NextColumn();

        DrawSimulationParamValue(value);
        ImGui::NextColumn();
    }
    ImGui::Columns();
}

void osc::DrawSearchBar(std::string& out, int maxLen)
{
    if (!out.empty())
    {
        if (ImGui::Button("X"))
        {
            out.clear();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Clear the search string");
            ImGui::EndTooltip();
        }
    }
    else
    {
        ImGui::Text(ICON_FA_SEARCH);
    }

    // draw search bar

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    osc::InputString("##hirarchtsearchbar", out, maxLen);
}

void osc::DrawOutputNameColumn(
    VirtualOutputExtractor const& output,
    bool centered,
    SimulationModelStatePair* maybeActiveSate)
{
    if (centered)
    {
        osc::TextCentered(output.getName());
    }
    else
    {
        ImGui::TextUnformatted(output.getName().c_str());
    }

    // if it's specifically a component ouptut, then hover/clicking the text should
    // propagate to the rest of the UI
    //
    // (e.g. if the user mouses over the name of a component output it should make
    // the associated component the current hover to provide immediate feedback to
    // the user)
    if (auto const* co = dynamic_cast<osc::ComponentOutputExtractor const*>(&output); co && maybeActiveSate)
    {
        if (ImGui::IsItemHovered())
        {
            maybeActiveSate->setHovered(osc::FindComponent(maybeActiveSate->getModel(), co->getComponentAbsPath()));
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            maybeActiveSate->setSelected(osc::FindComponent(maybeActiveSate->getModel(), co->getComponentAbsPath()));
        }
    }

    if (!output.getDescription().empty())
    {
        ImGui::SameLine();
        osc::DrawHelpMarker(output.getName(), output.getDescription());
    }
}

void osc::DrawMuscleRenderingOptionsRadioButtions(CustomDecorationOptions& opts)
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

void osc::DrawMuscleSizingOptionsRadioButtons(CustomDecorationOptions& opts)
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

void osc::DrawMuscleColoringOptionsRadioButtons(CustomDecorationOptions& opts)
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

void osc::DrawMuscleDecorationOptionsEditor(CustomDecorationOptions& opts)
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

void osc::DrawRenderingOptionsEditor(CustomRenderingOptions& opts)
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

void osc::DrawCustomDecorationOptionCheckboxes(CustomDecorationOptions& opts)
{
    int imguiID = 0;
    for (size_t i = 0; i < opts.getNumOptions(); ++i)
    {
        ImGui::PushID(imguiID++);

        bool v = opts.getOptionValue(i);
        if (ImGui::Checkbox(opts.getOptionLabel(i).c_str(), &v))
        {
            opts.setOptionValue(i, v);
        }
        if (std::optional<CStringView> description = opts.getOptionDescription(i))
        {
            ImGui::SameLine();
            osc::DrawHelpMarker(*description);
        }

        ImGui::PopID();
    }
}

void osc::DrawAdvancedParamsEditor(
    ModelRendererParams& params,
    nonstd::span<SceneDecoration const> drawlist)
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
        TryPromptUserToSaveAsDAE(drawlist);
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

void osc::DrawVisualAidsContextMenuContent(ModelRendererParams& params)
{
    // generic rendering options
    DrawRenderingOptionsEditor(params.renderingOptions);

    // OpenSim-specific extra rendering options
    ImGui::Dummy({0.0f, 0.25f*ImGui::GetTextLineHeight()});
    ImGui::TextDisabled("OpenSim");
    DrawCustomDecorationOptionCheckboxes(params.decorationOptions);
}

void osc::DrawViewerTopButtonRow(
    ModelRendererParams& params,
    nonstd::span<osc::SceneDecoration const> drawlist,
    IconCache& iconCache,
    std::function<void()> const& drawExtraElements)
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
        [&params, drawlist]() { DrawAdvancedParamsEditor(params, drawlist); },
    };
    settingsButton.draw();
    ImGui::SameLine();

    // caller-provided extra buttons (usually, context-dependent)
    {
        drawExtraElements();
    }
}

void osc::DrawCameraControlButtons(
    PolarPerspectiveCamera& camera,
    Rect const& viewerScreenRect,
    std::optional<AABB> const& maybeSceneAABB,
    IconCache& iconCache)
{
    ImGuiStyle const& style = ImGui::GetStyle();
    float const buttonHeight = 2.0f*style.FramePadding.y + ImGui::GetTextLineHeight();
    float const rowSpacing = ImGui::GetStyle().FramePadding.y;
    float const twoRowHeight = 2.0f*buttonHeight + rowSpacing;
    float const xFirstRow = viewerScreenRect.p1.x + style.WindowPadding.x + osc::CalcAlignmentAxesDimensions().x + style.ItemSpacing.x;
    float const yFirstRow = (viewerScreenRect.p2.y - style.WindowPadding.y - 0.5f*osc::CalcAlignmentAxesDimensions().y) - 0.5f*twoRowHeight;

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

    // draw single row
    {
        ImGui::SetCursorScreenPos({ImGui::GetCursorScreenPos().x, midRowY});

        IconWithoutMenu autoFocusButton
        {
            iconCache.getIcon("zoomauto"),
            "Auto-Focus Camera",
            "Try to automatically adjust the camera's zoom etc. to suit the model's dimensions. Hotkey: Ctrl+F",
        };
        if (autoFocusButton.draw() && maybeSceneAABB)
        {
            osc::AutoFocus(camera, *maybeSceneAABB, osc::AspectRatio(viewerScreenRect));
        }
    }
}

void osc::DrawViewerImGuiOverlays(
    ModelRendererParams& params,
    nonstd::span<SceneDecoration const> drawlist,
    std::optional<AABB> maybeSceneAABB,
    Rect const& renderRect,
    IconCache& iconCache,
    std::function<void()> const& drawExtraElementsInTop)
{
    // draw the top overlays
    ImGui::SetCursorScreenPos(renderRect.p1 + glm::vec2{ImGui::GetStyle().WindowPadding});
    DrawViewerTopButtonRow(params, drawlist, iconCache, drawExtraElementsInTop);

    // compute bottom overlay positions
    ImGuiStyle const& style = ImGui::GetStyle();
    glm::vec2 const alignmentAxesDims = osc::CalcAlignmentAxesDimensions();
    glm::vec2 const axesTopLeft =
    {
        renderRect.p1.x + style.WindowPadding.x,
        renderRect.p2.y - style.WindowPadding.y - alignmentAxesDims.y
    };

    // draw the bottom overlays
    ImGui::SetCursorScreenPos(axesTopLeft);
    DrawAlignmentAxes(
        params.camera.getViewMtx()
    );
    DrawCameraControlButtons(
        params.camera,
        renderRect,
        maybeSceneAABB,
        iconCache
    );
}