#include "BasicWidgets.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Documents/OutputExtractors/ComponentOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/ForceRecordOutputExtractor.h>
#include <libopensimcreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libopensimcreator/Documents/Simulation/IntegratorMethod.h>
#include <libopensimcreator/Documents/Simulation/SimulationModelStatePair.h>
#include <libopensimcreator/Graphics/CustomRenderingOptions.h>
#include <libopensimcreator/Graphics/ModelRendererParams.h>
#include <libopensimcreator/Graphics/MuscleDecorationStyle.h>
#include <libopensimcreator/Graphics/MuscleSizingStyle.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
#include <libopensimcreator/Graphics/OpenSimDecorationOptions.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/Platform/RecentFile.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/Utils/ParamBlock.h>
#include <libopensimcreator/Utils/ParamValue.h>
#include <libopensimcreator/Utils/SimTKConverters.h>

#include <liboscar/Formats/DAE.h>
#include <liboscar/Formats/OBJ.h>
#include <liboscar/Formats/STL.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppMetadata.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/os.h>
#include <liboscar/UI/IconCache.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Widgets/CameraViewAxes.h>
#include <liboscar/UI/Widgets/IconWithMenu.h>
#include <liboscar/Utils/StringHelpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <SimTKcommon/basics.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

using namespace osc::literals;
using namespace osc;

// export utils
namespace
{
    // prompts the user for a save location and then exports a DAE file containing the 3D scene
    void TryPromptUserToSaveAsDAE(std::span<const SceneDecoration> scene)
    {
        App::upd().prompt_user_to_save_file_with_extension_async([scene = std::vector(scene.begin(), scene.end())](std::optional<std::filesystem::path> p)
        {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            std::ofstream outfile{*p};
            if (not outfile) {
                log_error("cannot save to %s: IO error", p->string().c_str());
                return;
            }

            DAEMetadata daeMetadata{
                App::get().human_readable_name(),
                App::get().application_name_with_version_and_buildid(),
            };

            write_as_dae(outfile, scene, daeMetadata);
            log_info("wrote scene as a DAE file to %s", p->string().c_str());
        }, "dae");
    }

    void DrawOutputTooltip(const OpenSim::AbstractOutput& o)
    {
        ui::begin_tooltip();
        ui::draw_text_disabled(o.getTypeName());
        ui::end_tooltip();
    }

    bool DrawOutputWithSubfieldsMenu(
        const OpenSim::AbstractOutput& o,
        const std::function<void(OutputExtractor)>& onUserSelection)
    {
        bool outputAdded = false;
        ComponentOutputSubfields supportedSubfields = GetSupportedSubfields(o);

        // can plot suboutputs
        if (ui::begin_menu(("  " + o.getName())))
        {
            for (ComponentOutputSubfield f : GetAllSupportedOutputSubfields())
            {
                if (f & supportedSubfields)
                {
                    if (auto label = GetOutputSubfieldLabel(f); label && ui::draw_menu_item(*label))
                    {
                        onUserSelection(OutputExtractor{ComponentOutputExtractor{o, f}});
                        outputAdded = true;
                    }
                }
            }
            ui::end_menu();
        }

        if (ui::is_item_hovered())
        {
            DrawOutputTooltip(o);
        }

        return outputAdded;
    }

    bool DrawOutputWithNoSubfieldsMenuItem(
        const OpenSim::AbstractOutput& o,
        const std::function<void(OutputExtractor)>& onUserSelection)
    {
        // can only plot top-level of output

        bool outputAdded = false;

        if (ui::draw_menu_item(("  " + o.getName())))
        {
            onUserSelection(OutputExtractor{ComponentOutputExtractor{o}});
            outputAdded = true;
        }

        if (ui::is_item_hovered())
        {
            DrawOutputTooltip(o);
        }

        return outputAdded;
    }

    void DrawSimulationParamValue(const ParamValue& v)
    {
        if (std::holds_alternative<double>(v))
        {
            ui::draw_text("%f", static_cast<float>(std::get<double>(v)));
        }
        else if (std::holds_alternative<IntegratorMethod>(v))
        {
            ui::draw_text(std::get<IntegratorMethod>(v).label());
        }
        else if (std::holds_alternative<int>(v))
        {
            ui::draw_text("%i", std::get<int>(v));
        }
        else
        {
            ui::draw_text("(unknown value type)");
        }
    }

    Transform CalcTransformWithRespectTo(
        const OpenSim::Mesh& mesh,
        const OpenSim::Frame& frame,
        const SimTK::State& state)
    {
        auto rv = to<Transform>(mesh.getFrame().findTransformBetween(state, frame));
        rv.scale = to<Vec3>(mesh.get_scale_factors());
        return rv;
    }

    void ActionReexportMeshOBJWithRespectTo(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const OpenSim::Mesh& openSimMesh,
        const OpenSim::Frame& frame)
    {
        // Pre-write mesh data in-memory so that the asynchronous callback isn't dependent
        // on a bunch of state.
        std::stringstream ss;
        {
            // load raw mesh data into an osc mesh for processing
            Mesh oscMesh = ToOscMesh(model, state, openSimMesh);

            // bake transform into mesh data
            oscMesh.transform_vertices(CalcTransformWithRespectTo(openSimMesh, frame, state));

            const ObjMetadata objMetadata{
                App::get().application_name_with_version_and_buildid(),
            };

            write_as_obj(ss, oscMesh, objMetadata, ObjWriterFlag::NoWriteNormals);
        }

        // Asynchronously prompt the user and write the data
        App::upd().prompt_user_to_save_file_with_extension_async([content = std::move(ss).str()](std::optional<std::filesystem::path> p)
        {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            // write transformed mesh to output
            try {
                std::ofstream ofs;
                ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                ofs.open(*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

                ofs << content;
            }
            catch (const std::exception& e) {
                log_error("error saving obj output to %s: %s", p->string().c_str(), e.what());
            }
        }, "obj");
    }

    void ActionReexportMeshSTLWithRespectTo(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const OpenSim::Mesh& openSimMesh,
        const OpenSim::Frame& frame)
    {
        // Pre-write the mesh data in-memory so that the asynchronous callback isn't dependent
        // on a bunch of state.
        std::stringstream ss;
        {
            // load raw mesh data into an osc mesh for processing
            Mesh oscMesh = ToOscMesh(model, state, openSimMesh);

            // bake transform into mesh data
            oscMesh.transform_vertices(CalcTransformWithRespectTo(openSimMesh, frame, state));

            const StlMetadata stlMetadata{
                App::get().application_name_with_version_and_buildid(),
            };

            write_as_stl(ss, oscMesh, stlMetadata);
        }

        // Asynchronously prompt the user for a save location and write the content to it.
        App::upd().prompt_user_to_save_file_with_extension_async([content = std::move(ss).str()](std::optional<std::filesystem::path> p)
        {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            // write transformed mesh to output
            try {
                std::ofstream ofs;
                ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                ofs.open(*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

                ofs << content;
            }
            catch (const std::exception& e) {
                log_error("error saving obj output to %s: %s", p->string().c_str(), e.what());
            }
        }, "stl");
    }

    void drawTooltipOrContextMenuContentText(const OpenSim::Component& c)
    {
        ui::draw_text(truncate_with_ellipsis(c.getName(), 15));
        ui::same_line();
        ui::begin_disabled();
        ui::draw_text(c.getConcreteClassName());
        ui::same_line();
        ui::draw_text(IconFor(c));
        ui::end_disabled();
    }
}


// public API

CStringView osc::IconFor(const OpenSim::Component& c)
{
    if (dynamic_cast<const OpenSim::Muscle*>(&c)) {
        return OSC_ICON_MUSCLE;
    }
    else if (dynamic_cast<const OpenSim::Coordinate*>(&c)) {
        return OSC_ICON_COORDINATE;
    }
    else if (dynamic_cast<const OpenSim::WrapObject*>(&c)) {
        return OSC_ICON_WRAP;
    }
    else if (dynamic_cast<const OpenSim::Probe*>(&c)) {
        return OSC_ICON_PROBE;
    }
    else if (dynamic_cast<const OpenSim::Joint*>(&c)) {
        return OSC_ICON_JOINT;
    }
    else if (dynamic_cast<const OpenSim::Geometry*>(&c)) {
        return OSC_ICON_MESH;
    }
    else if (dynamic_cast<const OpenSim::Body*>(&c)) {
        return OSC_ICON_BODY;
    }
    else if (dynamic_cast<const OpenSim::ContactGeometry*>(&c)) {
        return OSC_ICON_CONTACT;
    }
    else if (dynamic_cast<const OpenSim::Station*>(&c) or dynamic_cast<const OpenSim::PathPoint*>(&c)) {
        return OSC_ICON_MARKER;
    }
    else if (dynamic_cast<const OpenSim::Constraint*>(&c)) {
        return OSC_ICON_CONSTRAINT;
    }
    else if (dynamic_cast<const OpenSim::Function*>(&c)) {
        return OSC_ICON_SPLINE;
    }
    else if (dynamic_cast<const OpenSim::Frame*>(&c)) {
        return OSC_ICON_FRAME;
    }
    else if (dynamic_cast<const OpenSim::Model*>(&c)) {
        return OSC_ICON_MODEL;
    }
    else {
        return OSC_ICON_COMPONENT;
    }
}

void osc::DrawNothingRightClickedContextMenuHeader()
{
    ui::draw_text_disabled("(nothing selected)");
}

void osc::DrawContextMenuHeader(CStringView title, CStringView subtitle)
{
    ui::draw_text(title);
    ui::same_line();
    ui::draw_text_disabled(subtitle);
}

void osc::DrawRightClickedComponentContextMenuHeader(const OpenSim::Component& c)
{
    drawTooltipOrContextMenuContentText(c);
}

void osc::DrawContextMenuSeparator()
{
    ui::draw_separator();
    ui::draw_vertical_spacer(3.0f/15.0f);
}

void osc::DrawComponentHoverTooltip(const OpenSim::Component& hovered)
{
    ui::begin_tooltip();
    drawTooltipOrContextMenuContentText(hovered);
    ui::end_tooltip();
}

void osc::DrawSelectOwnerMenu(IModelStatePair& model, const OpenSim::Component& selected)
{
    if (ui::begin_menu("Select Owner"))
    {
        model.setHovered(nullptr);

        for (
            const OpenSim::Component* owner = GetOwner(selected);
            owner != nullptr;
            owner = GetOwner(*owner))
        {
            const std::string menuLabel = [&owner]()
            {
                std::stringstream ss;
                ss << owner->getName() << '(' << owner->getConcreteClassName() << ')';
                return std::move(ss).str();
            }();

            if (ui::draw_menu_item(menuLabel))
            {
                model.setSelected(owner);
            }
            if (ui::is_item_hovered())
            {
                model.setHovered(owner);
            }
        }

        ui::end_menu();
    }
}

bool osc::DrawRequestOutputMenuOrMenuItem(
    const OpenSim::AbstractOutput& o,
    const std::function<void(OutputExtractor)>& onUserSelection)
{
    if (GetSupportedSubfields(o) == ComponentOutputSubfield::None)
    {
        return DrawOutputWithNoSubfieldsMenuItem(o, onUserSelection);
    }
    else
    {
        return DrawOutputWithSubfieldsMenu(o, onUserSelection);
    }
}

bool osc::DrawWatchOutputMenu(
    const OpenSim::Component& c,
    const std::function<void(OutputExtractor)>& onUserSelection)
{
    bool outputAdded = false;

    if (ui::begin_menu("Watch Output")) {
        int entriesDrawn = 0;
        for (const auto& [name, output] : c.getOutputs()) {
            ui::push_id(entriesDrawn++);
            if (DrawRequestOutputMenuOrMenuItem(*output, onUserSelection)) {
                outputAdded = true;
            }
            ui::pop_id();
        }

        // Edge-case: `Force`s have record-based outputs, which should also be exposed
        if (const auto* f = dynamic_cast<const OpenSim::Force*>(&c)) {
            const OpenSim::Array<std::string> labels = f->getRecordLabels();
            for (int i = 0; i < labels.size(); ++i) {
                ui::push_id(entriesDrawn++);
                if (ui::draw_menu_item("  " + labels[i])) {
                    onUserSelection(OutputExtractor{ForceRecordOutputExtractor{*f, i}});
                    outputAdded = true;
                }
                ui::pop_id();
            }
        }

        if (entriesDrawn == 0) {
            ui::draw_text_disabled("%s has no outputs", c.getName().c_str());
        }
        ui::end_menu();
    }

    return outputAdded;
}

void osc::DrawSimulationParams(const ParamBlock& params)
{
    ui::draw_vertical_spacer(1.0f/15.0f);
    ui::draw_text("parameters:");
    ui::same_line();
    ui::draw_help_marker("The parameters used when this simulation was launched. These must be set *before* running the simulation");
    ui::draw_separator();
    ui::draw_vertical_spacer(2.0f/15.0f);

    ui::set_num_columns(2);
    for (int i = 0, len = params.size(); i < len; ++i)
    {
        const std::string& name = params.getName(i);
        const std::string& description = params.getDescription(i);
        const ParamValue& value = params.getValue(i);

        ui::draw_text(name);
        ui::same_line();
        ui::draw_help_marker(name, description);
        ui::next_column();

        DrawSimulationParamValue(value);
        ui::next_column();
    }
    ui::set_num_columns();
}

void osc::DrawSearchBar(std::string& out)
{
    ui::push_style_var(ui::StyleVar::FrameRounding, 5.0f);
    ui::draw_string_input_with_hint("##hirarchtsearchbar", OSC_ICON_SEARCH " search...",  out);
    ui::pop_style_var();
}

void osc::DrawOutputNameColumn(
    const IOutputExtractor& output,
    bool centered,
    SimulationModelStatePair* maybeActiveSate)
{
    if (centered)
    {
        ui::draw_text_centered(output.getName());
    }
    else
    {
        ui::draw_text(output.getName());
    }

    // if it's specifically a component ouptut, then hover/clicking the text should
    // propagate to the rest of the UI
    //
    // (e.g. if the user mouses over the name of a component output it should make
    // the associated component the current hover to provide immediate feedback to
    // the user)
    if (const auto* co = dynamic_cast<const ComponentOutputExtractor*>(&output); co && maybeActiveSate)
    {
        if (ui::is_item_hovered())
        {
            maybeActiveSate->setHovered(FindComponent(maybeActiveSate->getModel(), co->getComponentAbsPath()));
        }

        if (ui::is_item_clicked(ui::MouseButton::Left))
        {
            maybeActiveSate->setSelected(FindComponent(maybeActiveSate->getModel(), co->getComponentAbsPath()));
        }
    }

    if (!output.getDescription().empty())
    {
        ui::same_line();
        ui::draw_help_marker(output.getName(), output.getDescription());
    }
}

void osc::DrawWithRespectToMenuContainingMenuPerFrame(
    const OpenSim::Component& root,
    const std::function<void(const OpenSim::Frame&)>& onFrameMenuOpened,
    const OpenSim::Frame* maybeParent)
{
    ui::draw_text_disabled("With Respect to:");
    ui::draw_separator();

    int imguiID = 0;

    if (maybeParent) {
        ui::push_id(imguiID++);
        std::stringstream ss;
        ss << "Parent (" << maybeParent->getName() << ')';
        if (ui::begin_menu(std::move(ss).str())) {
            onFrameMenuOpened(*maybeParent);
            ui::end_menu();
        }
        ui::pop_id();
        ui::draw_separator();
    }

    for (const OpenSim::Frame& frame : root.getComponentList<OpenSim::Frame>())
    {
        ui::push_id(imguiID++);
        if (ui::begin_menu(frame.getName()))
        {
            onFrameMenuOpened(frame);
            ui::end_menu();
        }
        ui::pop_id();
    }
}

void osc::DrawWithRespectToMenuContainingMenuItemPerFrame(
    const OpenSim::Component& root,
    const std::function<void(const OpenSim::Frame&)>& onFrameMenuItemClicked,
    const OpenSim::Frame* maybeParent = nullptr)
{
    ui::draw_text_disabled("With Respect to:");
    ui::draw_separator();

    int imguiID = 0;

    if (maybeParent) {
        ui::push_id(imguiID++);
        if (ui::draw_menu_item("parent")) {
            onFrameMenuItemClicked(*maybeParent);
        }
        ui::pop_id();
    }

    for (const OpenSim::Frame& frame : root.getComponentList<OpenSim::Frame>())
    {
        ui::push_id(imguiID++);
        if (ui::draw_menu_item(frame.getName()))
        {
            onFrameMenuItemClicked(frame);
        }
        ui::pop_id();
    }
}

void osc::DrawPointTranslationInformationWithRespectTo(
    const OpenSim::Frame& frame,
    const SimTK::State& state,
    Vec3 locationInGround)
{
    const SimTK::Transform groundToFrame = frame.getTransformInGround(state).invert();
    Vec3 position = to<Vec3>(groundToFrame * to<SimTK::Vec3>(locationInGround));

    ui::draw_text("translation");
    ui::same_line();
    ui::draw_help_marker("translation", "Translational offset (in meters) of the point expressed in the chosen frame");
    ui::same_line();
    ui::draw_vec3_input("##translation", position, "%.6f", ui::TextInputFlag::ReadOnly);
}

void osc::DrawDirectionInformationWithRepsectTo(
    const OpenSim::Frame& frame,
    const SimTK::State& state,
    Vec3 directionInGround)
{
    const SimTK::Transform groundToFrame = frame.getTransformInGround(state).invert();
    Vec3 direction = to<Vec3>(groundToFrame.xformBaseVecToFrame(to<SimTK::Vec3>(directionInGround)));

    ui::draw_text("direction");
    ui::same_line();
    ui::draw_help_marker("direction", "a unit vector expressed in the given frame");
    ui::same_line();
    ui::draw_vec3_input("##direction", direction, "%.6f", ui::TextInputFlag::ReadOnly);
}

void osc::DrawFrameInformationExpressedIn(
    const OpenSim::Frame& parent,
    const SimTK::State& state,
    const OpenSim::Frame& otherFrame)
{
    const SimTK::Transform xform = parent.findTransformBetween(state, otherFrame);
    Vec3 position = to<Vec3>(xform.p());
    Vec3 rotationEulers = to<Vec3>(xform.R().convertRotationToBodyFixedXYZ());

    ui::draw_text("translation");
    ui::same_line();
    ui::draw_help_marker("translation", "Translational offset (in meters) of the frame's origin expressed in the chosen frame");
    ui::same_line();
    ui::draw_vec3_input("##translation", position, "%.6f", ui::TextInputFlag::ReadOnly);

    ui::draw_text("orientation");
    ui::same_line();
    ui::draw_help_marker("orientation", "Orientation offset (in radians) of the frame, expressed in the chosen frame as a frame-fixed x-y-z rotation sequence");
    ui::same_line();
    ui::draw_vec3_input("##orientation", rotationEulers, "%.6f", ui::TextInputFlag::ReadOnly);
}

bool osc::BeginCalculateMenu(CalculateMenuFlags flags)
{
    const CStringView label = flags & CalculateMenuFlags::NoCalculatorIcon ?
        "Calculate" :
        OSC_ICON_CALCULATOR " Calculate";
    return ui::begin_menu(label);
}

void osc::EndCalculateMenu()
{
    ui::end_menu();
}

void osc::DrawCalculatePositionMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Point& point,
    const OpenSim::Frame* maybeParent)
{
    if (ui::begin_menu("Position"))
    {
        const auto onFrameMenuOpened = [&state, &point](const OpenSim::Frame& frame)
        {
            DrawPointTranslationInformationWithRespectTo(
                frame,
                state,
                to<Vec3>(point.getLocationInGround(state))
            );
        };

        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, maybeParent);
        ui::end_menu();
    }
}

void osc::DrawCalculateMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Station& station,
    CalculateMenuFlags flags)
{
    if (BeginCalculateMenu(flags))
    {
        DrawCalculatePositionMenu(root, state, station, &station.getParentFrame());
        EndCalculateMenu();
    }
}

void osc::DrawCalculateMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Point& point,
    CalculateMenuFlags flags)
{
    if (BeginCalculateMenu(flags))
    {
        DrawCalculatePositionMenu(root, state, point, nullptr);
        EndCalculateMenu();
    }
}

void osc::DrawCalculateTransformMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Frame& frame)
{
    if (ui::begin_menu("Transform"))
    {
        const auto onFrameMenuOpened = [&state, &frame](const OpenSim::Frame& otherFrame)
        {
            DrawFrameInformationExpressedIn(frame, state, otherFrame);
        };
        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, TryGetParentFrame(frame));
        ui::end_menu();
    }
}

void osc::DrawCalculateAxisDirectionsMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Frame& frame)
{
    if (ui::begin_menu("Axis Directions")) {
        const auto onFrameMenuOpened = [&state, &frame](const OpenSim::Frame& other)
        {
            Vec3 x = to<Vec3>(frame.expressVectorInAnotherFrame(state, {1.0, 0.0, 0.0}, other));
            Vec3 y = to<Vec3>(frame.expressVectorInAnotherFrame(state, {0.0, 1.0, 0.0}, other));
            Vec3 z = to<Vec3>(frame.expressVectorInAnotherFrame(state, {0.0, 0.0, 1.0}, other));

            ui::draw_text("x axis");
            ui::same_line();
            ui::draw_vec3_input("##xdir", x, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("y axis");
            ui::same_line();
            ui::draw_vec3_input("##ydir", y, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("z axis");
            ui::same_line();
            ui::draw_vec3_input("##zdir", z, "%.6f", ui::TextInputFlag::ReadOnly);
        };
        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, TryGetParentFrame(frame));
        ui::end_menu();
    }
}

void osc::DrawCalculateOriginMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Frame& frame)
{
    if (ui::begin_menu("Origin")) {
        const auto onFrameMenuOpened = [&state, &frame](const OpenSim::Frame& otherFrame)
        {
            auto v = to<Vec3>(frame.findStationLocationInAnotherFrame(state, {0.0f, 0.0f, 0.0f}, otherFrame));
            ui::draw_text("origin");
            ui::same_line();
            ui::draw_vec3_input("##origin", v, "%.6f", ui::TextInputFlag::ReadOnly);
        };
        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, TryGetParentFrame(frame));
        ui::end_menu();
    }
}

void osc::DrawCalculateMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Frame& frame,
    CalculateMenuFlags flags)
{
    if (BeginCalculateMenu(flags))
    {
        DrawCalculateTransformMenu(root, state, frame);
        DrawCalculateOriginMenu(root, state, frame);
        DrawCalculateAxisDirectionsMenu(root, state, frame);
        EndCalculateMenu();
    }
}

void osc::DrawCalculateOriginMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Sphere& sphere)
{
    if (ui::begin_menu("Origin"))
    {
        const Vec3 posInGround = to<Vec3>(sphere.getFrame().getPositionInGround(state));
        const auto onFrameMenuOpened = [&state, posInGround](const OpenSim::Frame& otherFrame)
        {
            DrawPointTranslationInformationWithRespectTo(otherFrame, state, posInGround);
        };
        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, TryGetParentFrame(sphere.getFrame()));

        ui::end_menu();
    }
}

void osc::DrawCalculateRadiusMenu(
    const OpenSim::Component&,
    const SimTK::State&,
    const OpenSim::Sphere& sphere)
{
    if (ui::begin_menu("Radius"))
    {
        double d = sphere.get_radius();
        ui::draw_double_input("radius", &d);
        ui::end_menu();
    }
}

void osc::DrawCalculateVolumeMenu(
    const OpenSim::Component&,
    const SimTK::State&,
    const OpenSim::Sphere& sphere)
{
    if (ui::begin_menu("Volume"))
    {
        const double r = sphere.get_radius();
        double v = 4.0/3.0 * SimTK::Pi * r*r*r;
        ui::draw_double_input("volume", &v, 0.0, 0.0, "%.6f", ui::TextInputFlag::ReadOnly);
        ui::end_menu();
    }
}

void osc::DrawCalculateMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Geometry& geom,
    CalculateMenuFlags flags)
{
    if (BeginCalculateMenu(flags))
    {
        if (const auto* spherePtr = dynamic_cast<const OpenSim::Sphere*>(&geom))
        {
            DrawCalculateOriginMenu(root, state, *spherePtr);
            DrawCalculateRadiusMenu(root, state, *spherePtr);
            DrawCalculateVolumeMenu(root, state, *spherePtr);
        }
        else
        {
            DrawCalculateTransformMenu(root, state, geom.getFrame());
            DrawCalculateOriginMenu(root, state, geom.getFrame());
            DrawCalculateAxisDirectionsMenu(root, state, geom.getFrame());
        }
        EndCalculateMenu();
    }
}

void osc::TryDrawCalculateMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Component& selected,
    CalculateMenuFlags flags)
{
    if (const auto* const frame = dynamic_cast<const OpenSim::Frame*>(&selected))
    {
        DrawCalculateMenu(root, state, *frame, flags);
    }
    else if (const auto* const point = dynamic_cast<const OpenSim::Point*>(&selected))
    {
        DrawCalculateMenu(root, state, *point, flags);
    }
}

void osc::DrawCalculateOriginMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Ellipsoid& ellipsoid)
{
    if (ui::begin_menu("Origin")) {
        const Vec3 posInGround = to<Vec3>(ellipsoid.getFrame().getPositionInGround(state));
        const auto onFrameMenuOpened = [&state, posInGround](const OpenSim::Frame& otherFrame)
        {
            DrawPointTranslationInformationWithRespectTo(otherFrame, state, posInGround);
        };
        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, TryGetParentFrame(ellipsoid.getFrame()));

        ui::end_menu();
    }
}

void osc::DrawCalculateRadiiMenu(
    const OpenSim::Component&,
    const SimTK::State&,
    const OpenSim::Ellipsoid& ellipsoid)
{
    if (ui::begin_menu("Radii")) {
        auto v = to<Vec3>(ellipsoid.get_radii());
        ui::draw_text("radii");
        ui::same_line();
        ui::draw_vec3_input("##radii", v, "%.6f", ui::TextInputFlag::ReadOnly);
        ui::end_menu();
    }
}

void osc::DrawCalculateRadiiDirectionsMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Ellipsoid& ellipsoid)
{
    DrawCalculateAxisDirectionsMenu(root, state, ellipsoid.getFrame());
}

void osc::DrawCalculateScaledRadiiDirectionsMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Ellipsoid& ellipsoid)
{
    if (ui::begin_menu("Axis Directions (Scaled by Radii)")) {
        const auto onFrameMenuOpened = [&state, &ellipsoid](const OpenSim::Frame& other)
        {
            const auto& radii = ellipsoid.get_radii();
            Vec3 x = to<Vec3>(radii[0] * ellipsoid.getFrame().expressVectorInAnotherFrame(state, {1.0, 0.0, 0.0}, other));
            Vec3 y = to<Vec3>(radii[1] * ellipsoid.getFrame().expressVectorInAnotherFrame(state, {0.0, 1.0, 0.0}, other));
            Vec3 z = to<Vec3>(radii[2] * ellipsoid.getFrame().expressVectorInAnotherFrame(state, {0.0, 0.0, 1.0}, other));

            ui::draw_text("x axis");
            ui::same_line();
            ui::draw_vec3_input("##xdir", x, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("y axis");
            ui::same_line();
            ui::draw_vec3_input("##ydir", y, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("z axis");
            ui::same_line();
            ui::draw_vec3_input("##zdir", z, "%.6f", ui::TextInputFlag::ReadOnly);
        };
        DrawWithRespectToMenuContainingMenuPerFrame(root, onFrameMenuOpened, TryGetParentFrame(ellipsoid.getFrame()));
        ui::end_menu();
    }
}

void osc::DrawCalculateMenu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Ellipsoid& ellipsoid,
    CalculateMenuFlags flags)
{
    if (BeginCalculateMenu(flags)) {
        DrawCalculateOriginMenu(root, state, ellipsoid);
        DrawCalculateRadiiMenu(root, state, ellipsoid);
        DrawCalculateRadiiDirectionsMenu(root, state, ellipsoid);
        DrawCalculateScaledRadiiDirectionsMenu(root, state, ellipsoid);
        EndCalculateMenu();
    }
}

bool osc::DrawMuscleRenderingOptionsRadioButtions(OpenSimDecorationOptions& opts)
{
    const MuscleDecorationStyle currentStyle = opts.getMuscleDecorationStyle();
    bool edited = false;
    for (const auto& metadata : GetAllMuscleDecorationStyleMetadata())
    {
        if (ui::draw_radio_button(metadata.label, metadata.value == currentStyle))
        {
            opts.setMuscleDecorationStyle(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::DrawMuscleSizingOptionsRadioButtons(OpenSimDecorationOptions& opts)
{
    const MuscleSizingStyle currentStyle = opts.getMuscleSizingStyle();
    bool edited = false;
    for (const auto& metadata : GetAllMuscleSizingStyleMetadata())
    {
        if (ui::draw_radio_button(metadata.label, metadata.value == currentStyle))
        {
            opts.setMuscleSizingStyle(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::DrawMuscleColorSourceOptionsRadioButtons(OpenSimDecorationOptions& opts)
{
    const MuscleColorSource currentStyle = opts.getMuscleColorSource();
    bool edited = false;
    for (const auto& metadata : GetAllPossibleMuscleColoringSourcesMetadata())
    {
        if (ui::draw_radio_button(metadata.label, metadata.value == currentStyle))
        {
            opts.setMuscleColorSource(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::DrawMuscleColorScalingOptionsRadioButtons(OpenSimDecorationOptions& opts)
{
    const MuscleColorSourceScaling currentStyle = opts.getMuscleColorSourceScaling();
    bool edited = false;
    for (const auto& metadata : GetAllPossibleMuscleColorSourceScalingMetadata()) {
        if (ui::draw_radio_button(metadata.label, metadata.value == currentStyle)) {
            opts.setMuscleColorSourceScaling(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::DrawMuscleDecorationOptionsEditor(OpenSimDecorationOptions& opts)
{
    int id = 0;
    bool edited = false;

    ui::push_id(id++);
    ui::draw_text_disabled("Rendering");
    edited = DrawMuscleRenderingOptionsRadioButtions(opts) || edited;
    ui::pop_id();

    ui::draw_vertical_spacer(0.25f);
    ui::push_id(id++);
    ui::draw_text_disabled("Sizing");
    edited = DrawMuscleSizingOptionsRadioButtons(opts) || edited;
    ui::pop_id();

    ui::draw_vertical_spacer(0.25f);
    ui::push_id(id++);
    ui::draw_text_disabled("Color Source");
    edited = DrawMuscleColorSourceOptionsRadioButtons(opts) || edited;
    ui::pop_id();

    ui::draw_vertical_spacer(0.25f);
    ui::push_id(id++);
    ui::draw_text_disabled("Color Scaling");
    edited = DrawMuscleColorScalingOptionsRadioButtons(opts) || edited;
    ui::pop_id();

    return edited;
}

bool osc::DrawRenderingOptionsEditor(CustomRenderingOptions& opts)
{
    bool edited = false;
    ui::draw_text_disabled("Rendering");
    for (size_t i = 0; i < opts.getNumOptions(); ++i)
    {
        bool value = opts.getOptionValue(i);
        if (ui::draw_checkbox(opts.getOptionLabel(i), &value))
        {
            opts.setOptionValue(i, value);
            edited = true;
        }
    }
    return edited;
}

bool osc::DrawOverlayOptionsEditor(OverlayDecorationOptions& opts)
{
    std::optional<CStringView> lastGroupLabel;
    bool edited = false;
    for (size_t i = 0; i < opts.getNumOptions(); ++i)
    {
        // print header, if necessary
        const CStringView groupLabel = opts.getOptionGroupLabel(i);
        if (groupLabel != lastGroupLabel)
        {
            if (lastGroupLabel)
            {
                ui::draw_vertical_spacer(0.25f);
            }
            ui::draw_text_disabled(groupLabel);
            lastGroupLabel = groupLabel;
        }

        bool value = opts.getOptionValue(i);
        if (ui::draw_checkbox(opts.getOptionLabel(i), &value))
        {
            opts.setOptionValue(i, value);
            edited = true;
        }
    }
    return edited;
}

bool osc::DrawCustomDecorationOptionCheckboxes(OpenSimDecorationOptions& opts)
{
    int imguiID = 0;
    bool edited = false;
    for (size_t i = 0; i < opts.getNumOptions(); ++i)
    {
        ui::push_id(imguiID++);

        bool v = opts.getOptionValue(i);
        if (ui::draw_checkbox(opts.getOptionLabel(i), &v))
        {
            opts.setOptionValue(i, v);
            edited = true;
        }
        if (std::optional<CStringView> description = opts.getOptionDescription(i))
        {
            ui::same_line();
            ui::draw_help_marker(*description);
        }

        ui::pop_id();
    }
    return edited;
}

bool osc::DrawAdvancedParamsEditor(
    ModelRendererParams& params,
    std::span<const SceneDecoration> drawlist)
{
    bool edited = false;

    if (ui::draw_button("Export to .dae"))
    {
        TryPromptUserToSaveAsDAE(drawlist);
    }
    ui::draw_tooltip_body_only_if_item_hovered("Try to export the 3D scene to a portable DAE file, so that it can be viewed in 3rd-party modelling software, such as Blender");

    ui::draw_vertical_spacer(10.0f/15.0f);
    ui::draw_text("advanced camera properties:");
    ui::draw_separator();
    edited = ui::draw_float_meters_slider("radius", params.camera.radius, 0.0f, 10.0f) || edited;
    edited = ui::draw_angle_slider("theta", params.camera.theta, 0_deg, 360_deg) || edited;
    edited = ui::draw_angle_slider("phi", params.camera.phi, 0_deg, 360_deg) || edited;
    edited = ui::draw_angle_slider("vertial FoV", params.camera.vertical_field_of_view, 0_deg, 360_deg) || edited;
    edited = ui::draw_float_meters_input("znear", params.camera.znear) || edited;
    edited = ui::draw_float_meters_input("zfar", params.camera.zfar) || edited;
    ui::start_new_line();
    edited = ui::draw_float_meters_slider("pan_x", params.camera.focus_point.x, -100.0f, 100.0f) || edited;
    edited = ui::draw_float_meters_slider("pan_y", params.camera.focus_point.y, -100.0f, 100.0f) || edited;
    edited = ui::draw_float_meters_slider("pan_z", params.camera.focus_point.z, -100.0f, 100.0f) || edited;

    ui::draw_vertical_spacer(10.0f/15.0f);
    ui::draw_text("advanced scene properties:");
    ui::draw_separator();
    edited = ui::draw_rgb_color_editor("light_color", params.lightColor) || edited;
    edited = ui::draw_rgb_color_editor("background color", params.backgroundColor) || edited;
    edited = ui::draw_float3_meters_input("floor location", params.floorLocation) || edited;
    ui::draw_tooltip_body_only_if_item_hovered("Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");

    return edited;
}

bool osc::DrawVisualAidsContextMenuContent(ModelRendererParams& params)
{
    bool edited = false;

    // generic rendering options
    edited = DrawRenderingOptionsEditor(params.renderingOptions) || edited;

    // overlay options
    edited = DrawOverlayOptionsEditor(params.overlayOptions) || edited;

    // OpenSim-specific extra rendering options
    ui::draw_vertical_spacer(0.25f);
    ui::draw_text_disabled("OpenSim");
    edited = DrawCustomDecorationOptionCheckboxes(params.decorationOptions) || edited;

    return edited;
}

bool osc::DrawViewerTopButtonRow(
    ModelRendererParams& params,
    std::span<const SceneDecoration>,
    IconCache& iconCache,
    const std::function<bool()>& drawExtraElements)
{
    bool edited = false;

    IconWithMenu muscleStylingButton
    {
        iconCache.find_or_throw("muscle_coloring"),
        "Muscle Styling",
        "Affects how muscles appear in this visualizer panel",
        [&params]() { return DrawMuscleDecorationOptionsEditor(params.decorationOptions); },
    };
    edited = muscleStylingButton.on_draw() || edited;
    ui::same_line();

    IconWithMenu vizAidsButton
    {
        iconCache.find_or_throw("viz_aids"),
        "Visual Aids",
        "Affects what's shown in the 3D scene",
        [&params]() { return DrawVisualAidsContextMenuContent(params); },
    };
    edited = vizAidsButton.on_draw() || edited;

    ui::same_line();
    ui::draw_vertical_separator();
    ui::same_line();

    // caller-provided extra buttons (usually, context-dependent)
    edited = drawExtraElements() || edited;

    return edited;
}

bool osc::DrawCameraControlButtons(
    ModelRendererParams& params,
    std::span<const SceneDecoration> drawlist,
    const Rect& viewerScreenRect,
    const std::optional<AABB>& maybeSceneAABB,
    IconCache& iconCache,
    Vec2 desiredTopCentroid)
{
    IconWithoutMenu zoomOutButton{
        iconCache.find_or_throw("zoomout"),
        "Zoom Out Camera",
        "Moves the camera one step away from its focus point (Hotkey: -)",
    };
    IconWithoutMenu zoomInButton{
        iconCache.find_or_throw("zoomin"),
        "Zoom in Camera",
        "Moves the camera one step towards its focus point (Hotkey: =)",
    };
    IconWithoutMenu autoFocusButton{
        iconCache.find_or_throw("zoomauto"),
        "Auto-Focus Camera",
        "Try to automatically adjust the camera's zoom etc. to suit the model's dimensions (Hotkey: Ctrl+F)",
    };
    IconWithMenu sceneSettingsButton{
        iconCache.find_or_throw("gear"),
        "Scene Settings",
        "Change advanced scene settings",
        [&params, drawlist]() { return DrawAdvancedParamsEditor(params, drawlist); },
    };

    auto c = ui::get_style_color(ui::ColorVar::Button);
    c.a *= 0.9f;
    ui::push_style_color(ui::ColorVar::Button, c);

    const float spacing = ui::get_style_item_spacing().x;
    float width = zoomOutButton.dimensions().x + spacing + zoomInButton.dimensions().x + spacing + autoFocusButton.dimensions().x;
    const Vec2 topleft = {desiredTopCentroid.x - 0.5f*width, desiredTopCentroid.y + 2.0f*ui::get_style_item_spacing().y};
    ui::set_cursor_ui_pos(topleft);

    bool edited = false;
    if (zoomOutButton.on_draw()) {
        zoom_out(params.camera);
        edited = true;
    }
    ui::same_line();
    if (zoomInButton.on_draw()) {
        zoom_in(params.camera);
        edited = true;
    }
    ui::same_line();
    if (autoFocusButton.on_draw() && maybeSceneAABB) {
        auto_focus(params.camera, *maybeSceneAABB, aspect_ratio_of(viewerScreenRect));
        edited = true;
    }

    // next line (centered)
    {
        const Vec2 tl = {
            desiredTopCentroid.x - 0.5f*sceneSettingsButton.dimensions().x,
            ui::get_cursor_ui_pos().y,
        };
        ui::set_cursor_ui_pos(tl);
        if (sceneSettingsButton.on_draw()) {
            edited = true;
        }
    }

    ui::pop_style_color();

    return edited;
}

bool osc::DrawViewerImGuiOverlays(
    ModelRendererParams& params,
    std::span<const SceneDecoration> drawlist,
    std::optional<AABB> maybeSceneAABB,
    const Rect& renderRect,
    IconCache& iconCache,
    const std::function<bool()>& drawExtraElementsInTop)
{
    bool edited = false;

    // draw top-left buttons
    const Vec2 windowPadding = ui::get_style_panel_padding();
    ui::set_cursor_ui_pos(renderRect.ypd_top_left() + windowPadding);
    edited = DrawViewerTopButtonRow(params, drawlist, iconCache, drawExtraElementsInTop) || edited;

    // draw top-right camera manipulators
    CameraViewAxes axes;
    const Vec2 renderDims = renderRect.dimensions();
    const Vec2 axesDims = axes.dimensions();
    const Vec2 axesTopLeft = {
        renderRect.ypd_top_left().x + renderDims.x - windowPadding.x - axesDims.x,
        renderRect.ypd_top_left().y + windowPadding.y,
    };

    // draw the bottom overlays
    ui::set_cursor_ui_pos(axesTopLeft);
    edited = axes.draw(params.camera) || edited;

    const Vec2 cameraButtonsTopLeft = axesTopLeft + Vec2{0.0f, axesDims.y};
    ui::set_cursor_ui_pos(cameraButtonsTopLeft);
    edited = DrawCameraControlButtons(
        params,
        drawlist,
        renderRect,
        maybeSceneAABB,
        iconCache,
        {axesTopLeft.x + 0.5f*axesDims.x, axesTopLeft.y + axesDims.y}
    ) || edited;

    return edited;
}

bool osc::BeginToolbar(CStringView label, std::optional<Vec2> padding)
{
    if (padding)
    {
        ui::push_style_var(ui::StyleVar::PanelPadding, *padding);
    }

    const float height = ui::get_frame_height() + 2.0f*ui::get_style_panel_padding().y;
    const ui::PanelFlags flags = {ui::PanelFlag::NoScrollbar, ui::PanelFlag::NoSavedSettings};
    bool open = ui::begin_main_window_top_bar(label, height, flags);
    if (padding)
    {
        ui::pop_style_var();
    }
    return open;
}

void osc::DrawNewModelButton(Widget& api)
{
    if (ui::draw_button(OSC_ICON_FILE))
    {
        ActionNewModel(api);
    }
    ui::draw_tooltip_if_item_hovered("New Model", "Creates a new OpenSim model in a new tab");
}

void osc::DrawOpenModelButtonWithRecentFilesDropdown(
    const std::function<void(std::optional<std::filesystem::path>)>& onUserClickedOpenOrSelectedFile)
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});
    if (ui::draw_button(OSC_ICON_FOLDER_OPEN))
    {
        onUserClickedOpenOrSelectedFile(std::nullopt);
    }
    ui::draw_tooltip_if_item_hovered("Open Model", "Opens an existing osim file in a new tab");
    ui::same_line();
    ui::push_style_var(ui::StyleVar::FramePadding, {1.0f, ui::get_style_frame_padding().y});
    ui::draw_button(OSC_ICON_CARET_DOWN);
    ui::draw_tooltip_if_item_hovered("Open Recent File", "Opens a recently-opened osim file in a new tab");
    ui::pop_style_var();
    ui::pop_style_var();

    if (ui::begin_popup_context_menu("##RecentFilesMenu", ui::PopupFlag::MouseButtonLeft))
    {
        const auto recentFiles = App::singleton<RecentFiles>();
        int imguiID = 0;

        for (const RecentFile& rf : *recentFiles)
        {
            ui::push_id(imguiID++);
            if (ui::draw_selectable(rf.path.filename().string()))
            {
                onUserClickedOpenOrSelectedFile(rf.path);
            }
            ui::pop_id();
        }

        ui::end_popup();
    }
}

void osc::DrawOpenModelButtonWithRecentFilesDropdown(Widget& api)
{
    DrawOpenModelButtonWithRecentFilesDropdown([&api](auto maybeFile)
    {
        if (maybeFile) {
            ActionOpenModel(api, *maybeFile);
        }
        else {
            ActionOpenModel(api);
        }
    });
}

void osc::DrawSaveModelButton(const std::shared_ptr<IModelStatePair>& model)
{
    if (ui::draw_button(OSC_ICON_SAVE)) {
        ActionSaveModelAsync(model);
    }
    ui::draw_tooltip_if_item_hovered("Save Model", "Saves the model to an osim file");
}

void osc::DrawReloadModelButton(UndoableModelStatePair& model)
{
    const bool disable = model.isReadonly() or not HasInputFileName(model.getModel());

    if (disable) {
        ui::begin_disabled();
    }
    if (ui::draw_button(OSC_ICON_RECYCLE)) {
        ActionReloadOsimFromDisk(model, *App::singleton<SceneCache>());
    }
    if (disable) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Reload Model", "Reloads the model from its source osim file");
}

void osc::DrawUndoButton(IModelStatePair& model)
{
    auto* undoable = dynamic_cast<UndoableModelStatePair*>(&model);
    const bool disable = not (undoable != nullptr and undoable->canUndo());

    if (disable) {
        ui::begin_disabled();
    }
    if (ui::draw_button(OSC_ICON_UNDO) and undoable != nullptr) {
        undoable->doUndo();
    }
    if (disable) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Undo", "Undo the model to an earlier version");
}

void osc::DrawRedoButton(IModelStatePair& model)
{
    auto* undoable = dynamic_cast<UndoableModelStatePair*>(&model);
    const bool disable = not (undoable != nullptr and undoable->canRedo());

    if (disable) {
        ui::begin_disabled();
    }
    if (ui::draw_button(OSC_ICON_REDO) and undoable != nullptr) {
        undoable->doRedo();
    }
    if (disable) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Redo", "Redo the model to an undone version");
}

void osc::DrawUndoAndRedoButtons(IModelStatePair& model)
{
    DrawUndoButton(model);
    ui::same_line();
    DrawRedoButton(model);
}

void osc::DrawToggleFramesButton(IModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(IsShowingFrames(model.getModel()) ? "frame_colored" : "frame_bw");

    if (model.isReadonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button("##toggleframes", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        ActionToggleFrames(model);
    }
    if (model.isReadonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Toggle Rendering Frames", "Toggles whether frames (coordinate systems) within the model should be rendered in the 3D scene.");
}

void osc::DrawToggleMarkersButton(IModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(IsShowingMarkers(model.getModel()) ? "marker_colored" : "marker");
    if (model.isReadonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button("##togglemarkers", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        ActionToggleMarkers(model);
    }
    if (model.isReadonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Toggle Rendering Markers", "Toggles whether markers should be rendered in the 3D scene");
}

void osc::DrawToggleWrapGeometryButton(IModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(IsShowingWrapGeometry(model.getModel()) ? "wrap_colored" : "wrap");
    if (model.isReadonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button("##togglewrapgeom", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        ActionToggleWrapGeometry(model);
    }
    if (model.isReadonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Toggle Rendering Wrap Geometry", "Toggles whether wrap geometry should be rendered in the 3D scene.\n\nNOTE: This is a model-log_level_ property. Individual wrap geometries *within* the model may have their visibility set to 'false', which will cause them to be hidden from the visualizer, even if this is enabled.");
}

void osc::DrawToggleContactGeometryButton(IModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(IsShowingContactGeometry(model.getModel()) ? "contact_colored" : "contact");
    if (model.isReadonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button("##togglecontactgeom", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        ActionToggleContactGeometry(model);
    }
    if (model.isReadonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Toggle Rendering Contact Geometry", "Toggles whether contact geometry should be rendered in the 3D scene");
}

void osc::DrawToggleForcesButton(IModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(IsShowingForces(model.getModel()) ? "forces_colored" : "forces_bw");
    if (model.isReadonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button("##toggleforces", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        ActionToggleForces(model);
    }
    if (model.isReadonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Toggle Rendering Forces", "Toggles whether forces should be rendered in the 3D scene.\n\nNOTE: this is a model-level property that only applies to forces in OpenSim that actually check this flag. OpenSim Creator's visualizers also offer custom overlays for forces, muscles, etc. separately to this mechanism.");
}

void osc::DrawAllDecorationToggleButtons(IModelStatePair& model, IconCache& icons)
{
    DrawToggleFramesButton(model, icons);
    ui::same_line();
    DrawToggleMarkersButton(model, icons);
    ui::same_line();
    DrawToggleWrapGeometryButton(model, icons);
    ui::same_line();
    DrawToggleContactGeometryButton(model, icons);
    ui::same_line();
    DrawToggleForcesButton(model, icons);
}

void osc::DrawSceneScaleFactorEditorControls(IModelStatePair& model)
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
    ui::draw_text(OSC_ICON_EXPAND_ALT);
    ui::draw_tooltip_if_item_hovered("Scene Scale Factor", "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
    ui::same_line();

    {
        float scaleFactor = model.getFixupScaleFactor();
        ui::set_next_item_width(ui::calc_text_size("0.00000").x);
        if (ui::draw_float_input("##scaleinput", &scaleFactor)) {
            model.setFixupScaleFactor(scaleFactor);
        }
    }
    ui::pop_style_var();

    ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});
    ui::same_line();
    if (ui::draw_button(OSC_ICON_EXPAND_ARROWS_ALT)) {
        ActionAutoscaleSceneScaleFactor(model);
    }
    ui::pop_style_var();
    ui::draw_tooltip_if_item_hovered("Autoscale Scale Factor", "Try to autoscale the model's scale factor based on the current dimensions of the model");
}

void osc::DrawMeshExportContextMenuContent(
    const IModelStatePair& model,
    const OpenSim::Mesh& mesh)
{
    ui::draw_text_disabled("Format:");
    ui::draw_separator();

    if (ui::begin_menu(".obj")) {
        const auto onFrameMenuItemClicked = [&model, &mesh](const OpenSim::Frame& frame)
        {
            ActionReexportMeshOBJWithRespectTo(
                model.getModel(),
                model.getState(),
                mesh,
                frame
            );
        };

        DrawWithRespectToMenuContainingMenuItemPerFrame(model.getModel(), onFrameMenuItemClicked);
        ui::end_menu();
    }

    if (ui::begin_menu(".stl")) {
        const auto onFrameMenuItemClicked = [&model, &mesh](const OpenSim::Frame& frame)
        {
            ActionReexportMeshSTLWithRespectTo(
                model.getModel(),
                model.getState(),
                mesh,
                frame
            );
        };

        DrawWithRespectToMenuContainingMenuItemPerFrame(model.getModel(), onFrameMenuItemClicked);
        ui::end_menu();
    }
}
