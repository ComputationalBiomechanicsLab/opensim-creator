#include "BasicWidgets.hpp"

#include "OpenSimCreator/Bindings/SimTKHelpers.hpp"
#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"
#include "OpenSimCreator/Graphics/MuscleDecorationStyle.hpp"
#include "OpenSimCreator/Graphics/MuscleSizingStyle.hpp"
#include "OpenSimCreator/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Model/VirtualModelStatePair.hpp"
#include "OpenSimCreator/Outputs/ComponentOutputExtractor.hpp"
#include "OpenSimCreator/Outputs/OutputExtractor.hpp"
#include "OpenSimCreator/Simulation/IntegratorMethod.hpp"
#include "OpenSimCreator/Simulation/SimulationModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/ParamBlock.hpp"
#include "OpenSimCreator/Utils/ParamValue.hpp"
#include "OpenSimCreator/Utils/UndoableModelActions.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Formats/DAE.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/IconCache.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Maths/Constants.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/RecentFile.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Widgets/GuiRuler.hpp>
#include <oscar/Widgets/IconWithMenu.hpp>
#include <OscarConfiguration.hpp>

#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <SimTKcommon/basics.h>

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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

        osc::WriteDecorationsAsDAE(outfile, scene);
        osc::log::info("wrote scene as a DAE file to %s", daePath.string().c_str());
    }

    void DrawOutputTooltip(OpenSim::AbstractOutput const& o)
    {
        osc::DrawTooltip(o.getTypeName());
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
                    if (ImGui::MenuItem(GetOutputSubfieldLabel(f).c_str()))
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
            ImGui::Text("%s", osc::GetIntegratorMethodString(std::get<osc::IntegratorMethod>(v)).c_str());
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

void osc::DrawNothingRightClickedContextMenuHeader()
{
    ImGui::TextDisabled("(nothing selected)");
}

void osc::DrawRightClickedComponentContextMenuHeader(OpenSim::Component const& c)
{
    ImGui::TextUnformatted(Ellipsis(c.getName(), 15).c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%s", c.getConcreteClassName().c_str());
}

void osc::DrawContextMenuSeparator()
{
    ImGui::Separator();
    ImGui::Dummy({0.0f, 3.0f});
}

void osc::DrawComponentHoverTooltip(OpenSim::Component const& hovered)
{
    osc::BeginTooltip();

    ImGui::TextUnformatted(hovered.getName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%s", hovered.getConcreteClassName().c_str());

    osc::EndTooltip();
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

            std::string const menuLabel = [&c]()
            {
                std::stringstream ss;
                ss << c->getName() << '(' << c->getConcreteClassName() << ')';
                return std::move(ss).str();
            }();

            if (ImGui::MenuItem(menuLabel.c_str()))
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

void osc::DrawSearchBar(std::string& out)
{
    if (!out.empty())
    {
        if (ImGui::Button("X"))
        {
            out.clear();
        }
        osc::DrawTooltipBodyOnlyIfItemHovered("Clear the search string");
    }
    else
    {
        ImGui::Text(ICON_FA_SEARCH);
    }

    // draw search bar

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    osc::InputString("##hirarchtsearchbar", out);
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

void osc::DrawWithRespectToMenuContainingMenuPerFrame(
    OpenSim::Component const& root,
    std::function<void(OpenSim::Frame const&)> const& onFrameMenuOpened)
{
    ImGui::TextDisabled("With Respect to:");
    ImGui::Separator();

    int imguiID = 0;
    for (OpenSim::Frame const& frame : root.getComponentList<OpenSim::Frame>())
    {
        ImGui::PushID(imguiID++);
        if (ImGui::BeginMenu(frame.getName().c_str()))
        {
            onFrameMenuOpened(frame);
            ImGui::EndMenu();
        }
        ImGui::PopID();
    }
}

void osc::DrawWithRespectToMenuContainingMenuItemPerFrame(
    OpenSim::Component const& root,
    std::function<void(OpenSim::Frame const&)> const& onFrameMenuItemClicked)
{
    ImGui::TextDisabled("With Respect to:");
    ImGui::Separator();

    int imguiID = 0;
    for (OpenSim::Frame const& frame : root.getComponentList<OpenSim::Frame>())
    {
        ImGui::PushID(imguiID++);
        if (ImGui::MenuItem(frame.getName().c_str()))
        {
            onFrameMenuItemClicked(frame);
        }
        ImGui::PopID();
    }
}

void osc::DrawPointTranslationInformationWithRespectTo(
    OpenSim::Frame const& frame,
    SimTK::State const& state,
    glm::vec3 locationInGround)
{
    SimTK::Transform const groundToFrame = frame.getTransformInGround(state).invert();
    glm::vec3 position = osc::ToVec3(groundToFrame * osc::ToSimTKVec3(locationInGround));

    ImGui::Text("translation");
    ImGui::SameLine();
    osc::DrawHelpMarker("translation", "Translational offset (in meters) of the point expressed in the chosen frame");
    ImGui::SameLine();
    ImGui::InputFloat3("##translation", glm::value_ptr(position), "%.6f", ImGuiInputTextFlags_ReadOnly);
}

void osc::DrawDirectionInformationWithRepsectTo(
    OpenSim::Frame const& frame,
    SimTK::State const& state,
    glm::vec3 directionInGround)
{
    SimTK::Transform const groundToFrame = frame.getTransformInGround(state).invert();
    glm::vec3 direction = osc::ToVec3(groundToFrame.xformBaseVecToFrame(osc::ToSimTKVec3(directionInGround)));

    ImGui::Text("direction");
    ImGui::SameLine();
    osc::DrawHelpMarker("direction", "a unit vector expressed in the given frame");
    ImGui::SameLine();
    ImGui::InputFloat3("##direction", glm::value_ptr(direction), "%.6f", ImGuiInputTextFlags_ReadOnly);
}

void osc::DrawFrameInformationExpressedIn(
    OpenSim::Frame const& parent,
    SimTK::State const& state,
    OpenSim::Frame const& otherFrame)
{
    SimTK::Transform const xform = parent.findTransformBetween(state, otherFrame);
    glm::vec3 position = osc::ToVec3(xform.p());
    glm::vec3 rotationEulers = osc::ToVec3(xform.R().convertRotationToBodyFixedXYZ());

    ImGui::Text("translation");
    ImGui::SameLine();
    osc::DrawHelpMarker("translation", "Translational offset (in meters) of the frame's origin expressed in the chosen frame");
    ImGui::SameLine();
    ImGui::InputFloat3("##translation", glm::value_ptr(position), "%.6f", ImGuiInputTextFlags_ReadOnly);

    ImGui::Text("orientation");
    ImGui::SameLine();
    osc::DrawHelpMarker("orientation", "Orientation offset (in radians) of the frame, expressed in the chosen frame as a frame-fixed x-y-z rotation sequence");
    ImGui::SameLine();
    ImGui::InputFloat3("##orientation", glm::value_ptr(rotationEulers), "%.6f", ImGuiInputTextFlags_ReadOnly);
}

void osc::DrawCalculateMenu(
    OpenSim::Component const& root,
    SimTK::State const& state,
    OpenSim::Point const& point,
    CalculateMenuFlags flags)
{
    osc::CStringView const label = flags & CalculateMenuFlags::NoCalculatorIcon ?
        "Calculate" :
        ICON_FA_CALCULATOR " Calculate";

    if (ImGui::BeginMenu(label.c_str()))
    {
        if (ImGui::BeginMenu("Position"))
        {
            auto const onFrameMenuOpened = [&state, &point](OpenSim::Frame const& frame)
            {
                osc::DrawPointTranslationInformationWithRespectTo(
                    frame,
                    state,
                    osc::ToVec3(point.getLocationInGround(state))
                );
            };

            osc::DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened);
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

void osc::DrawCalculateMenu(
    OpenSim::Component const& root,
    SimTK::State const& state,
    OpenSim::Frame const& frame,
    CalculateMenuFlags flags)
{
    osc::CStringView const label = flags & CalculateMenuFlags::NoCalculatorIcon ?
        "Calculate" :
        ICON_FA_CALCULATOR " Calculate";

    if (ImGui::BeginMenu(label.c_str()))
    {
        if (ImGui::BeginMenu("Transform"))
        {
            auto const onFrameMenuOpened = [&state, &frame](OpenSim::Frame const& otherFrame)
            {
                osc::DrawFrameInformationExpressedIn(frame, state, otherFrame);
            };
            osc::DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened);
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

void osc::TryDrawCalculateMenu(
    OpenSim::Component const& root,
    SimTK::State const& state,
    OpenSim::Component const& selected,
    CalculateMenuFlags flags)
{
    if (auto const* const frame = dynamic_cast<OpenSim::Frame const*>(&selected))
    {
        DrawCalculateMenu(root, state, *frame, flags);
    }
    else if (auto const* const point = dynamic_cast<OpenSim::Point const*>(&selected))
    {
        DrawCalculateMenu(root, state, *point, flags);
    }
}

void osc::DrawMuscleRenderingOptionsRadioButtions(OpenSimDecorationOptions& opts)
{
    osc::MuscleDecorationStyle const currentStyle = opts.getMuscleDecorationStyle();
    nonstd::span<osc::MuscleDecorationStyle const> const allStyles = osc::GetAllMuscleDecorationStyles();
    nonstd::span<osc::CStringView const>  const allStylesStrings = osc::GetAllMuscleDecorationStyleStrings();
    ptrdiff_t const currentStyleIndex = osc::GetIndexOf(currentStyle);

    for (ptrdiff_t i = 0; i < osc::ssize(allStyles); ++i)
    {
        if (ImGui::RadioButton(allStylesStrings[i].c_str(), i == currentStyleIndex))
        {
            opts.setMuscleDecorationStyle(allStyles[i]);
        }
    }
}

void osc::DrawMuscleSizingOptionsRadioButtons(OpenSimDecorationOptions& opts)
{
    osc::MuscleSizingStyle const currentStyle = opts.getMuscleSizingStyle();
    nonstd::span<osc::MuscleSizingStyle const> const allStyles = osc::GetAllMuscleSizingStyles();
    nonstd::span<CStringView const>  const allStylesStrings = osc::GetAllMuscleSizingStyleStrings();
    ptrdiff_t const currentStyleIndex = osc::GetIndexOf(currentStyle);

    for (ptrdiff_t i = 0; i < osc::ssize(allStyles); ++i)
    {
        if (ImGui::RadioButton(allStylesStrings[i].c_str(), i == currentStyleIndex))
        {
            opts.setMuscleSizingStyle(allStyles[i]);
        }
    }
}

void osc::DrawMuscleColoringOptionsRadioButtons(OpenSimDecorationOptions& opts)
{
    osc::MuscleColoringStyle const currentStyle = opts.getMuscleColoringStyle();
    nonstd::span<osc::MuscleColoringStyle const> const allStyles = osc::GetAllMuscleColoringStyles();
    nonstd::span<CStringView const>  const allStylesStrings = osc::GetAllMuscleColoringStyleStrings();
    ptrdiff_t const currentStyleIndex = osc::GetIndexOf(currentStyle);

    for (ptrdiff_t i = 0; i < osc::ssize(allStyles); ++i)
    {
        if (ImGui::RadioButton(allStylesStrings[i].c_str(), i == currentStyleIndex))
        {
            opts.setMuscleColoringStyle(allStyles[i]);
        }
    }
}

void osc::DrawMuscleDecorationOptionsEditor(OpenSimDecorationOptions& opts)
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

void osc::DrawOverlayOptionsEditor(OverlayDecorationOptions& opts)
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

void osc::DrawCustomDecorationOptionCheckboxes(OpenSimDecorationOptions& opts)
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
    osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along +X, pointing towards the center (Hotkey: X).");
    ImGui::SameLine();
    if (ImGui::Button("-X"))
    {
        osc::FocusAlongMinusX(params.camera);
    }
    osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along -X, pointing towards the center (Hotkey: Ctrl+X).");

    ImGui::SameLine();
    if (ImGui::Button("+Y"))
    {
        osc::FocusAlongY(params.camera);
    }
    osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along +Y, pointing towards the center (Hotkey: Y).");
    ImGui::SameLine();
    if (ImGui::Button("-Y"))
    {
        osc::FocusAlongMinusY(params.camera);
    }
    osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along -Y, pointing towards the center.");

    ImGui::SameLine();
    if (ImGui::Button("+Z"))
    {
        osc::FocusAlongZ(params.camera);
    }
    osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along +Z, pointing towards the center.");
    ImGui::SameLine();
    if (ImGui::Button("-Z"))
    {
        osc::FocusAlongMinusZ(params.camera);
    }
    osc::DrawTooltipBodyOnlyIfItemHovered("Position camera along -Z, pointing towards the center.");

    if (ImGui::Button("Zoom In (Hotkey: =)"))
    {
        osc::ZoomIn(params.camera);
    }

    ImGui::SameLine();
    if (ImGui::Button("Zoom Out (Hotkey: -)"))
    {
        osc::ZoomOut(params.camera);
    }

    if (ImGui::Button("Reset Camera"))
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
    ImGui::ColorEdit3("light_color", ValuePtr(params.lightColor));
    ImGui::ColorEdit3("background color", ValuePtr(params.backgroundColor));
    osc::InputMetersFloat3("floor location", params.floorLocation);
    osc::DrawTooltipBodyOnlyIfItemHovered("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");
}

void osc::DrawVisualAidsContextMenuContent(ModelRendererParams& params)
{
    // generic rendering options
    DrawRenderingOptionsEditor(params.renderingOptions);

    // overlay options
    DrawOverlayOptionsEditor(params.overlayOptions);

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
    IconWithMenu muscleStylingButton
    {
        iconCache.getIcon("muscle_coloring"),
        "Muscle Styling",
        "Affects how muscles appear in this visualizer panel",
        [&params]() { DrawMuscleDecorationOptionsEditor(params.decorationOptions); },
    };
    muscleStylingButton.onDraw();
    ImGui::SameLine();

    IconWithMenu vizAidsButton
    {
        iconCache.getIcon("viz_aids"),
        "Visual Aids",
        "Affects what's shown in the 3D scene",
        [&params]() { DrawVisualAidsContextMenuContent(params); },
    };
    vizAidsButton.onDraw();
    ImGui::SameLine();

    IconWithMenu settingsButton
    {
        iconCache.getIcon("gear"),
        "Scene Settings",
        "Change advanced scene settings",
        [&params, drawlist]() { DrawAdvancedParamsEditor(params, drawlist); },
    };
    settingsButton.onDraw();
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
            "Rotates the camera to focus along the +X direction (Hotkey: X)",
        };
        if (plusXbutton.onDraw())
        {
            FocusAlongX(camera);
        }

        ImGui::SameLine();

        IconWithoutMenu plusYbutton
        {
            iconCache.getIcon("plusy"),
            "Focus Camera Along +Y",
            "Rotates the camera to focus along the +Y direction (Hotkey: Y)",
        };
        if (plusYbutton.onDraw())
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
        if (plusZbutton.onDraw())
        {
            FocusAlongZ(camera);
        }

        ImGui::SameLine();
        ImGui::SameLine();

        IconWithoutMenu zoomInButton
        {
            iconCache.getIcon("zoomin"),
            "Zoom in Camera",
            "Moves the camera one step towards its focus point (Hotkey: =)",
        };
        if (zoomInButton.onDraw())
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
            "Rotates the camera to focus along the -X direction (Hotkey: Ctrl+X)",
        };
        if (minusXbutton.onDraw())
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
        if (minusYbutton.onDraw())
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
        if (minusZbutton.onDraw())
        {
            FocusAlongMinusZ(camera);
        }

        ImGui::SameLine();
        ImGui::SameLine();

        IconWithoutMenu zoomOutButton
        {
            iconCache.getIcon("zoomout"),
            "Zoom Out Camera",
            "Moves the camera one step away from its focus point (Hotkey: -)",
        };
        if (zoomOutButton.onDraw())
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
            "Try to automatically adjust the camera's zoom etc. to suit the model's dimensions (Hotkey: Ctrl+F)",
        };
        if (autoFocusButton.onDraw() && maybeSceneAABB)
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

bool osc::BeginToolbar(CStringView label, std::optional<glm::vec2> padding)
{
    if (padding)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, *padding);
    }

    float const height = ImGui::GetFrameHeight() + 2.0f*ImGui::GetStyle().WindowPadding.y;
    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    bool rv = osc::BeginMainViewportTopBar(label, height, flags);
    if (padding)
    {
        ImGui::PopStyleVar();
    }
    return rv;
}

void osc::DrawNewModelButton(ParentPtr<MainUIStateAPI> const& api)
{
    if (ImGui::Button(ICON_FA_FILE))
    {
        ActionNewModel(api);
    }
    osc::DrawTooltipIfItemHovered("New Model", "Creates a new OpenSim model in a new tab");
}

void osc::DrawOpenModelButtonWithRecentFilesDropdown(ParentPtr<MainUIStateAPI> const& api)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, 0.0f});
    if (ImGui::Button(ICON_FA_FOLDER_OPEN))
    {
        ActionOpenModel(api);
    }
    osc::DrawTooltipIfItemHovered("Open Model", "Opens an existing osim file in a new tab");
    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {1.0f, ImGui::GetStyle().FramePadding.y});
    ImGui::Button(ICON_FA_CARET_DOWN);
    osc::DrawTooltipIfItemHovered("Open Recent File", "Opens a recently-opened osim file in a new tab");
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    if (ImGui::BeginPopupContextItem("##RecentFilesMenu", ImGuiPopupFlags_MouseButtonLeft))
    {
        std::vector<RecentFile> recentFiles = App::get().getRecentFiles();
        std::reverse(recentFiles.begin(), recentFiles.end());  // sort newest -> oldest
        int imguiID = 0;

        for (RecentFile const& rf : recentFiles)
        {
            ImGui::PushID(imguiID++);
            if (ImGui::Selectable(rf.path.filename().string().c_str()))
            {
                ActionOpenModel(api, rf.path);
            }
            ImGui::PopID();
        }

        ImGui::EndPopup();
    }
}

void osc::DrawSaveModelButton(
    ParentPtr<MainUIStateAPI> const& api,
    UndoableModelStatePair& model)
{
    if (ImGui::Button(ICON_FA_SAVE))
    {
        ActionSaveModel(*api, model);
    }
    osc::DrawTooltipIfItemHovered("Save Model", "Saves the model to an osim file");
}

void osc::DrawReloadModelButton(UndoableModelStatePair& model)
{
    if (!HasInputFileName(model.getModel()))
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f * ImGui::GetStyle().Alpha);
    }

    if (ImGui::Button(ICON_FA_RECYCLE))
    {
        ActionReloadOsimFromDisk(model, *App::singleton<MeshCache>());
    }

    if (!HasInputFileName(model.getModel()))
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    osc::DrawTooltipIfItemHovered("Reload Model", "Reloads the model from its source osim file");
}

void osc::DrawUndoButton(UndoableModelStatePair& model)
{
    int itemFlagsPushed = 0;
    int styleVarsPushed = 0;
    if (!model.canUndo())
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ++itemFlagsPushed;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f * ImGui::GetStyle().Alpha);
        ++styleVarsPushed;
    }

    if (ImGui::Button(ICON_FA_UNDO))
    {
        ActionUndoCurrentlyEditedModel(model);
    }

    osc::PopItemFlags(itemFlagsPushed);
    ImGui::PopStyleVar(styleVarsPushed);

    osc::DrawTooltipIfItemHovered("Undo", "Undo the model to an earlier version");
}

void osc::DrawRedoButton(UndoableModelStatePair& model)
{
    int itemFlagsPushed = 0;
    int styleVarsPushed = 0;
    if (!model.canRedo())
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ++itemFlagsPushed;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f * ImGui::GetStyle().Alpha);
        ++styleVarsPushed;
    }

    if (ImGui::Button(ICON_FA_REDO))
    {
        ActionRedoCurrentlyEditedModel(model);
    }

    osc::PopItemFlags(itemFlagsPushed);
    ImGui::PopStyleVar(styleVarsPushed);

    osc::DrawTooltipIfItemHovered("Redo", "Redo the model to an undone version");
}

void osc::DrawUndoAndRedoButtons(UndoableModelStatePair& model)
{
    DrawUndoButton(model);
    ImGui::SameLine();
    DrawRedoButton(model);
}

void osc::DrawToggleFramesButton(UndoableModelStatePair& model, IconCache& icons)
{
    Icon const& icon = icons.getIcon(IsShowingFrames(model.getModel()) ? "frame_colored" : "frame_bw");
    if (osc::ImageButton("##toggleframes", icon.getTexture(), icon.getDimensions(), icon.getTextureCoordinates()))
    {
        ActionToggleFrames(model);
    }
    osc::DrawTooltipIfItemHovered("Toggle Rendering Frames", "Toggles whether frames (coordinate systems) within the model should be rendered in the 3D scene.");
}

void osc::DrawToggleMarkersButton(UndoableModelStatePair& model, IconCache& icons)
{
    Icon const& icon = icons.getIcon(IsShowingMarkers(model.getModel()) ? "marker_colored" : "marker");
    if (osc::ImageButton("##togglemarkers", icon.getTexture(), icon.getDimensions(), icon.getTextureCoordinates()))
    {
        ActionToggleMarkers(model);
    }
    osc::DrawTooltipIfItemHovered("Toggle Rendering Markers", "Toggles whether markers should be rendered in the 3D scene");
}

void osc::DrawToggleWrapGeometryButton(UndoableModelStatePair& model, IconCache& icons)
{
    Icon const& icon = icons.getIcon(IsShowingWrapGeometry(model.getModel()) ? "wrap_colored" : "wrap");
    if (osc::ImageButton("##togglewrapgeom", icon.getTexture(), icon.getDimensions(), icon.getTextureCoordinates()))
    {
        ActionToggleWrapGeometry(model);
    }
    osc::DrawTooltipIfItemHovered("Toggle Rendering Wrap Geometry", "Toggles whether wrap geometry should be rendered in the 3D scene.\n\nNOTE: This is a model-level property. Individual wrap geometries *within* the model may have their visibility set to 'false', which will cause them to be hidden from the visualizer, even if this is enabled.");
}

void osc::DrawToggleContactGeometryButton(UndoableModelStatePair& model, IconCache& icons)
{
    Icon const& icon = icons.getIcon(IsShowingContactGeometry(model.getModel()) ? "contact_colored" : "contact");
    if (osc::ImageButton("##togglecontactgeom", icon.getTexture(), icon.getDimensions(), icon.getTextureCoordinates()))
    {
        ActionToggleContactGeometry(model);
    }
    osc::DrawTooltipIfItemHovered("Toggle Rendering Contact Geometry", "Toggles whether contact geometry should be rendered in the 3D scene");
}

void osc::DrawAllDecorationToggleButtons(UndoableModelStatePair& model, IconCache& icons)
{
    DrawToggleFramesButton(model, icons);
    ImGui::SameLine();
    DrawToggleMarkersButton(model, icons);
    ImGui::SameLine();
    DrawToggleWrapGeometryButton(model, icons);
    ImGui::SameLine();
    DrawToggleContactGeometryButton(model, icons);
}

void osc::DrawSceneScaleFactorEditorControls(UndoableModelStatePair& model)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
    ImGui::TextUnformatted(ICON_FA_EXPAND_ALT);
    osc::DrawTooltipIfItemHovered("Scene Scale Factor", "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
    ImGui::SameLine();

    {
        float scaleFactor = model.getFixupScaleFactor();
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("0.00000").x);
        if (ImGui::InputFloat("##scaleinput", &scaleFactor))
        {
            osc::ActionSetModelSceneScaleFactorTo(model, scaleFactor);
        }
    }
    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, 0.0f});
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
    {
        osc::ActionAutoscaleSceneScaleFactor(model);
    }
    ImGui::PopStyleVar();
    osc::DrawTooltipIfItemHovered("Autoscale Scale Factor", "Try to autoscale the model's scale factor based on the current dimensions of the model");
}
