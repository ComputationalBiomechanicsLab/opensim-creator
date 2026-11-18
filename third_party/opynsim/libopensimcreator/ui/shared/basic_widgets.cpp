#include "basic_widgets.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/documents/simulation/simulation_model_state_pair.h>
#include <libopensimcreator/documents/param_block.h>
#include <libopensimcreator/documents/param_value.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/recent_file.h>
#include <libopensimcreator/platform/recent_files.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/output_extractors/component_output_extractor.h>
#include <libopynsim/documents/output_extractors/force_record_output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/graphics/custom_rendering_options.h>
#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/muscle_decoration_style.h>
#include <libopynsim/graphics/muscle_sizing_style.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/solvers/integrator_method.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/formats/dae.h>
#include <liboscar/formats/obj.h>
#include <liboscar/formats/stl.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/widgets/camera_view_axes.h>
#include <liboscar/ui/widgets/icon_with_menu.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Point.h>

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
    void try_prompt_user_to_save_as_dae(std::span<const SceneDecoration> scene)
    {
        App::upd().prompt_user_to_save_file_with_extension_async(
            [scene =
                 std::vector(scene.begin(), scene.end())](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                std::ofstream outfile{*p};
                if (not outfile) {
                    log_error("cannot save to {}: IO error", p->string());
                    return;
                }

                DAEMetadata dae_metadata{
                    App::get().human_readable_name(),
                    App::get().application_name_with_version_and_buildid(),
                };

                DAE::write(outfile, scene, dae_metadata);
                log_info("wrote scene as a DAE file to {}", p->string());
            },
            "dae");
    }

    void draw_output_tooltip(const OpenSim::AbstractOutput& o)
    {
        ui::begin_tooltip();
        ui::draw_text_disabled(o.getTypeName());
        ui::end_tooltip();
    }

    bool draw_output_with_subfields_menu(
        const OpenSim::AbstractOutput& o,
        const std::function<void(opyn::SharedOutputExtractor)>& on_user_selection)
    {
        bool output_added = false;
        opyn::ComponentOutputSubfields supported_subfields = opyn::get_supported_subfields(o);

        // can plot suboutputs
        if (ui::begin_menu(("  " + o.getName()))) {
            for (opyn::ComponentOutputSubfield f : opyn::get_all_supported_output_subfields()) {
                if (f & supported_subfields) {
                    if (auto label = opyn::get_output_subfield_label(f);
                        label and ui::draw_menu_item(*label)) {

                        on_user_selection(
                            opyn::make_output_extractor<opyn::ComponentOutputExtractor>(o, f));
                        output_added = true;
                    }
                }
            }
            ui::end_menu();
        }

        if (ui::is_item_hovered()) {
            draw_output_tooltip(o);
        }

        return output_added;
    }

    bool draw_output_with_no_subfields_menu_item(
        const OpenSim::AbstractOutput& o,
        const std::function<void(opyn::SharedOutputExtractor)>& on_user_selection)
    {
        // can only plot top-level of output

        bool output_added = false;

        if (ui::draw_menu_item(("  " + o.getName()))) {
            on_user_selection(opyn::make_output_extractor<opyn::ComponentOutputExtractor>(o));
            output_added = true;
        }

        if (ui::is_item_hovered()) {
            draw_output_tooltip(o);
        }

        return output_added;
    }

    void draw_simulation_param_value(const ParamValue& v)
    {
        if (std::holds_alternative<double>(v)) {
            ui::draw_text("{}", std::get<double>(v));
        } else if (std::holds_alternative<opyn::IntegratorMethod>(v)) {
            ui::draw_text(opyn::label_for(std::get<opyn::IntegratorMethod>(v)));
        } else if (std::holds_alternative<int>(v)) {
            ui::draw_text("{}", std::get<int>(v));
        } else {
            ui::draw_text("(unknown value type)");
        }
    }

    Transform calc_transform_with_respect_to(
        const OpenSim::Mesh& mesh, const OpenSim::Frame& frame, const SimTK::State& state)
    {
        auto rv = to<Transform>(mesh.getFrame().findTransformBetween(state, frame));
        rv.scale = to<Vector3>(mesh.get_scale_factors());
        return rv;
    }

    void action_reexport_mesh_obj_with_respect_to(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const OpenSim::Mesh& open_sim_mesh,
        const OpenSim::Frame& frame)
    {
        // Pre-write mesh data in-memory so that the asynchronous callback isn't dependent
        // on a bunch of state.
        std::stringstream ss;
        {
            // load raw mesh data into an osc mesh for processing
            Mesh osc_mesh = opyn::to_osc_mesh(model, state, open_sim_mesh);

            // bake transform into mesh data
            osc_mesh.transform_vertices(
                calc_transform_with_respect_to(open_sim_mesh, frame, state));

            const OBJMetadata obj_metadata{
                App::get().application_name_with_version_and_buildid(),
            };

            OBJ::write(ss, osc_mesh, obj_metadata, OBJWriterFlag::NoWriteNormals);
        }

        // Asynchronously prompt the user and write the data
        App::upd().prompt_user_to_save_file_with_extension_async(
            [content = std::move(ss).str()](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                // write transformed mesh to output
                try {
                    std::ofstream ofs;
                    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                    ofs.open(*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

                    ofs << content;
                } catch (const std::exception& e) {
                    log_error("error saving obj output to {}: {}", p->string(), e.what());
                }
            },
            "obj");
    }

    void action_reexport_mesh_stl_with_respect_to(
        const OpenSim::Model& model,
        const SimTK::State& state,
        const OpenSim::Mesh& open_sim_mesh,
        const OpenSim::Frame& frame)
    {
        // Pre-write the mesh data in-memory so that the asynchronous callback isn't dependent
        // on a bunch of state.
        std::stringstream ss;
        {
            // load raw mesh data into an osc mesh for processing
            Mesh osc_mesh = opyn::to_osc_mesh(model, state, open_sim_mesh);

            // bake transform into mesh data
            osc_mesh.transform_vertices(
                calc_transform_with_respect_to(open_sim_mesh, frame, state));

            const STLMetadata stl_metadata{
                App::get().application_name_with_version_and_buildid(),
            };

            STL::write(ss, osc_mesh, stl_metadata);
        }

        // Asynchronously prompt the user for a save location and write the content to it.
        App::upd().prompt_user_to_save_file_with_extension_async(
            [content = std::move(ss).str()](std::optional<std::filesystem::path> p) {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }

                // write transformed mesh to output
                try {
                    std::ofstream ofs;
                    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                    ofs.open(*p, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

                    ofs << content;
                } catch (const std::exception& e) {
                    log_error("error saving obj output to {}: {}", p->string(), e.what());
                }
            },
            "stl");
    }

    void draw_tooltip_or_context_menu_content_text(const OpenSim::Component& c)
    {
        ui::draw_text(truncate_with_ellipsis(c.getName(), 15));
        ui::same_line();
        ui::begin_disabled();
        ui::draw_text(c.getConcreteClassName());
        ui::same_line();
        ui::draw_text(icon_for(c));
        ui::end_disabled();
    }
}

CStringView osc::icon_for(const OpenSim::Component& c)
{
    if (dynamic_cast<const OpenSim::Muscle*>(&c)) {
        return MSMICONS_MUSCLE;
    } else if (dynamic_cast<const OpenSim::Coordinate*>(&c)) {
        return MSMICONS_COORDINATE;
    } else if (dynamic_cast<const OpenSim::WrapObject*>(&c)) {
        return MSMICONS_WRAP;
    } else if (dynamic_cast<const OpenSim::Probe*>(&c)) {
        return MSMICONS_PROBE;
    } else if (dynamic_cast<const OpenSim::Joint*>(&c)) {
        return MSMICONS_JOINT;
    } else if (dynamic_cast<const OpenSim::Geometry*>(&c)) {
        return MSMICONS_MESH;
    } else if (dynamic_cast<const OpenSim::Body*>(&c)) {
        return MSMICONS_BODY;
    } else if (dynamic_cast<const OpenSim::ContactGeometry*>(&c)) {
        return MSMICONS_CONTACT;
    } else if (
        dynamic_cast<const OpenSim::Station*>(&c) or dynamic_cast<const OpenSim::PathPoint*>(&c)) {

        return MSMICONS_MARKER;
    } else if (dynamic_cast<const OpenSim::Constraint*>(&c)) {
        return MSMICONS_CONSTRAINT;
    } else if (dynamic_cast<const OpenSim::Function*>(&c)) {
        return MSMICONS_SPLINE;
    } else if (dynamic_cast<const OpenSim::Frame*>(&c)) {
        return MSMICONS_FRAME;
    } else if (dynamic_cast<const OpenSim::Model*>(&c)) {
        return MSMICONS_MODEL;
    } else {
        return MSMICONS_COMPONENT;
    }
}

void osc::draw_nothing_right_clicked_context_menu_header()
{
    ui::draw_text_disabled("(nothing selected)");
}

void osc::draw_context_menu_header(CStringView title, CStringView subtitle)
{
    ui::draw_text(title);
    ui::same_line();
    ui::draw_text_disabled(subtitle);
}

void osc::draw_right_clicked_component_context_menu_header(const OpenSim::Component& c)
{
    draw_tooltip_or_context_menu_content_text(c);
}

void osc::draw_context_menu_separator()
{
    ui::draw_separator();
    ui::draw_vertical_spacer(3.0f / 15.0f);
}

void osc::draw_component_hover_tooltip(const OpenSim::Component& hovered)
{
    ui::begin_tooltip();
    draw_tooltip_or_context_menu_content_text(hovered);
    ui::end_tooltip();
}

void osc::draw_select_owner_menu(opyn::ModelStatePair& model, const OpenSim::Component& selected)
{
    if (ui::begin_menu("Select Owner")) {
        model.set_hovered(nullptr);

        for (const OpenSim::Component* owner = opyn::get_owner(selected); owner != nullptr;
             owner = opyn::get_owner(*owner)) {

            const std::string menu_label = [&owner] {
                std::stringstream ss;
                ss << owner->getName() << '(' << owner->getConcreteClassName() << ')';
                return std::move(ss).str();
            }();

            if (ui::draw_menu_item(menu_label)) {
                model.set_selected(owner);
            }
            if (ui::is_item_hovered()) {
                model.set_hovered(owner);
            }
        }

        ui::end_menu();
    }
}

bool osc::draw_request_output_menu_or_menu_item(
    const OpenSim::AbstractOutput& o,
    const std::function<void(opyn::SharedOutputExtractor)>& on_user_selection)
{
    if (opyn::get_supported_subfields(o) == opyn::ComponentOutputSubfield::None) {
        return draw_output_with_no_subfields_menu_item(o, on_user_selection);
    } else {
        return draw_output_with_subfields_menu(o, on_user_selection);
    }
}

bool osc::draw_watch_output_menu(
    const OpenSim::Component& c,
    const std::function<void(opyn::SharedOutputExtractor)>& on_user_selection)
{
    bool output_added = false;

    if (ui::begin_menu("Watch Output")) {
        int entries_drawn = 0;
        for (const auto& [name, output] : c.getOutputs()) {
            ui::push_id(entries_drawn++);
            if (draw_request_output_menu_or_menu_item(*output, on_user_selection)) {
                output_added = true;
            }
            ui::pop_id();
        }

        // Edge-case: `Force`s have record-based outputs, which should also be exposed
        if (const auto* f = dynamic_cast<const OpenSim::Force*>(&c)) {
            const OpenSim::Array<std::string> labels = f->getRecordLabels();
            for (int i = 0; i < labels.size(); ++i) {
                ui::push_id(entries_drawn++);
                if (ui::draw_menu_item("  " + labels[i])) {
                    on_user_selection(
                        opyn::make_output_extractor<opyn::ForceRecordOutputExtractor>(*f, i));
                    output_added = true;
                }
                ui::pop_id();
            }
        }

        if (entries_drawn == 0) {
            ui::draw_text_disabled("{} has no outputs", c.getName());
        }
        ui::end_menu();
    }

    return output_added;
}

void osc::draw_simulation_params(const ParamBlock& params)
{
    ui::draw_vertical_spacer(1.0f / 15.0f);
    ui::draw_text("parameters:");
    ui::same_line();
    ui::draw_help_marker(
        "The parameters used when this simulation was launched. These must be set *before* running the simulation");
    ui::draw_separator();
    ui::draw_vertical_spacer(2.0f / 15.0f);

    ui::set_num_columns(2);
    for (int i = 0, len = params.size(); i < len; ++i) {
        const std::string& name = params.get_name(i);
        const std::string& description = params.get_description(i);
        const ParamValue& value = params.get_value(i);

        ui::draw_text(name);
        ui::same_line();
        ui::draw_help_marker(name, description);
        ui::next_column();

        draw_simulation_param_value(value);
        ui::next_column();
    }
    ui::set_num_columns();
}

void osc::draw_search_bar(std::string& out)
{
    ui::push_style_var(ui::StyleVar::FrameRounding, 5.0f);
    ui::draw_string_input_with_hint("##hirarchtsearchbar", MSMICONS_SEARCH " search...", out);
    ui::pop_style_var();
}

void osc::draw_output_name_column(
    const opyn::OutputExtractor& output, bool centered, SimulationModelStatePair* maybe_active_sate)
{
    if (centered) {
        ui::draw_text_centered(output.name());
    } else {
        ui::draw_text(output.name());
    }

    // if it's specifically a component ouptut, then hover/clicking the text should
    // propagate to the rest of the UI
    //
    // (e.g. if the user mouses over the name of a component output it should make
    // the associated component the current hover to provide immediate feedback to
    // the user)
    if (const auto* co = dynamic_cast<const opyn::ComponentOutputExtractor*>(&output);
        co && maybe_active_sate) {

        if (ui::is_item_hovered()) {
            maybe_active_sate->set_hovered(
                opyn::find_component(maybe_active_sate->get_model(), co->component_abs_path()));
        }

        if (ui::is_item_clicked(MouseButton::Left)) {
            maybe_active_sate->set_selected(
                opyn::find_component(maybe_active_sate->get_model(), co->component_abs_path()));
        }
    }

    if (not output.description().empty()) {
        ui::same_line();
        ui::draw_help_marker(output.name(), output.description());
    }
}

void osc::draw_with_respect_to_menu_containing_menu_per_frame(
    const OpenSim::Component& root,
    const std::function<void(const OpenSim::Frame&)>& on_frame_menu_opened,
    const OpenSim::Frame* maybe_parent)
{
    ui::draw_text_disabled("With Respect to:");
    ui::draw_separator();

    int imgui_id = 0;

    if (maybe_parent) {
        ui::push_id(imgui_id++);
        std::stringstream ss;
        ss << "Parent (" << maybe_parent->getName() << ')';
        if (ui::begin_menu(std::move(ss).str())) {
            on_frame_menu_opened(*maybe_parent);
            ui::end_menu();
        }
        ui::pop_id();
        ui::draw_separator();
    }

    for (const OpenSim::Frame& frame : root.getComponentList<OpenSim::Frame>()) {
        ui::push_id(imgui_id++);
        if (ui::begin_menu(frame.getName())) {
            on_frame_menu_opened(frame);
            ui::end_menu();
        }
        ui::pop_id();
    }
}

void osc::draw_with_respect_to_menu_containing_menu_item_per_frame(
    const OpenSim::Component& root,
    const std::function<void(const OpenSim::Frame&)>& on_frame_menu_item_clicked,
    const OpenSim::Frame* maybe_parent = nullptr)
{
    ui::draw_text_disabled("With Respect to:");
    ui::draw_separator();

    int imgui_id = 0;

    if (maybe_parent) {
        ui::push_id(imgui_id++);
        if (ui::draw_menu_item("parent")) {
            on_frame_menu_item_clicked(*maybe_parent);
        }
        ui::pop_id();
    }

    for (const OpenSim::Frame& frame : root.getComponentList<OpenSim::Frame>()) {
        ui::push_id(imgui_id++);
        if (ui::draw_menu_item(frame.getName())) {
            on_frame_menu_item_clicked(frame);
        }
        ui::pop_id();
    }
}

void osc::draw_point_translation_information_with_respect_to(
    const OpenSim::Frame& frame, const SimTK::State& state, Vector3 location_in_ground)
{
    const SimTK::Transform ground_to_frame = frame.getTransformInGround(state).invert();
    auto translation = to<Vector3>(ground_to_frame * to<SimTK::Vec3>(location_in_ground));

    ui::draw_text("translation");
    ui::same_line();
    ui::draw_help_marker(
        "translation",
        "Translational offset (in meters) of the point expressed in the chosen frame");
    ui::same_line();
    ui::draw_vector3_input("##translation", translation, "%.6f", ui::TextInputFlag::ReadOnly);
}

void osc::draw_direction_information_with_repsect_to(
    const OpenSim::Frame& frame, const SimTK::State& state, Vector3 direction_in_ground)
{
    const SimTK::Transform ground_to_frame = frame.getTransformInGround(state).invert();
    auto direction =
        to<Vector3>(ground_to_frame.xformBaseVecToFrame(to<SimTK::Vec3>(direction_in_ground)));

    ui::draw_text("direction");
    ui::same_line();
    ui::draw_help_marker("direction", "a unit vector expressed in the given frame");
    ui::same_line();
    ui::draw_vector3_input("##direction", direction, "%.6f", ui::TextInputFlag::ReadOnly);
}

void osc::draw_frame_information_expressed_in(
    const OpenSim::Frame& parent, const SimTK::State& state, const OpenSim::Frame& other_frame)
{
    const SimTK::Transform xform = parent.findTransformBetween(state, other_frame);
    auto translation = to<Vector3>(xform.p());
    auto rotation_eulers = to<Vector3>(xform.R().convertRotationToBodyFixedXYZ());

    ui::draw_text("translation");
    ui::same_line();
    ui::draw_help_marker(
        "translation",
        "Translational offset (in meters) of the frame's origin expressed in the chosen frame");
    ui::same_line();
    ui::draw_vector3_input("##translation", translation, "%.6f", ui::TextInputFlag::ReadOnly);

    ui::draw_text("orientation");
    ui::same_line();
    ui::draw_help_marker(
        "orientation",
        "Orientation offset (in radians) of the frame, expressed in the chosen frame as a frame-fixed x-y-z rotation sequence");
    ui::same_line();
    ui::draw_vector3_input("##orientation", rotation_eulers, "%.6f", ui::TextInputFlag::ReadOnly);
}

bool osc::begin_calculate_menu(CalculateMenuFlags flags)
{
    const CStringView label = flags & CalculateMenuFlags::NoCalculatorIcon ? "Calculate"
                                                                           : MSMICONS_CALCULATOR
                                  " Calculate";
    return ui::begin_menu(label);
}

void osc::end_calculate_menu()
{
    ui::end_menu();
}

void osc::draw_calculate_position_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Point& point,
    const OpenSim::Frame* maybe_parent)
{
    if (ui::begin_menu("Position")) {
        const auto on_frame_menu_opened = [&state, &point](const OpenSim::Frame& frame) {
            draw_point_translation_information_with_respect_to(
                frame, state, to<Vector3>(point.getLocationInGround(state)));
        };

        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_frame_menu_opened, maybe_parent);
        ui::end_menu();
    }
}

void osc::draw_calculate_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Station& station,
    CalculateMenuFlags flags)
{
    if (begin_calculate_menu(flags)) {
        draw_calculate_position_menu(root, state, station, &station.getParentFrame());
        end_calculate_menu();
    }
}

void osc::draw_calculate_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Point& point,
    CalculateMenuFlags flags)
{
    if (begin_calculate_menu(flags)) {
        draw_calculate_position_menu(root, state, point, nullptr);
        end_calculate_menu();
    }
}

void osc::draw_calculate_transform_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Frame& frame)
{
    if (ui::begin_menu("Transform")) {
        const auto on_frame_menu_opened = [&state, &frame](const OpenSim::Frame& other_frame) {
            draw_frame_information_expressed_in(frame, state, other_frame);
        };
        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_frame_menu_opened, opyn::try_get_parent_frame(frame));
        ui::end_menu();
    }
}

void osc::draw_calculate_axis_directions_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Frame& frame)
{
    if (ui::begin_menu("Axis Directions")) {
        const auto on_frame_menu_opened = [&state, &frame](const OpenSim::Frame& other) {
            auto x = to<Vector3>(frame.expressVectorInAnotherFrame(state, {1.0, 0.0, 0.0}, other));
            auto y = to<Vector3>(frame.expressVectorInAnotherFrame(state, {0.0, 1.0, 0.0}, other));
            auto z = to<Vector3>(frame.expressVectorInAnotherFrame(state, {0.0, 0.0, 1.0}, other));

            ui::draw_text("x axis");
            ui::same_line();
            ui::draw_vector3_input("##xdir", x, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("y axis");
            ui::same_line();
            ui::draw_vector3_input("##ydir", y, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("z axis");
            ui::same_line();
            ui::draw_vector3_input("##zdir", z, "%.6f", ui::TextInputFlag::ReadOnly);
        };
        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_frame_menu_opened, opyn::try_get_parent_frame(frame));
        ui::end_menu();
    }
}

void osc::draw_calculate_origin_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Frame& frame)
{
    if (ui::begin_menu("Origin")) {
        const auto on_frame_menu_opened = [&state, &frame](const OpenSim::Frame& other_frame) {
            auto v = to<Vector3>(
                frame.findStationLocationInAnotherFrame(state, {0.0f, 0.0f, 0.0f}, other_frame));
            ui::draw_text("origin");
            ui::same_line();
            ui::draw_vector3_input("##origin", v, "%.6f", ui::TextInputFlag::ReadOnly);
        };
        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_frame_menu_opened, opyn::try_get_parent_frame(frame));
        ui::end_menu();
    }
}

void osc::draw_calculate_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Frame& frame,
    CalculateMenuFlags flags)
{
    if (begin_calculate_menu(flags)) {
        draw_calculate_transform_menu(root, state, frame);
        draw_calculate_origin_menu(root, state, frame);
        draw_calculate_axis_directions_menu(root, state, frame);
        end_calculate_menu();
    }
}

void osc::draw_calculate_origin_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Sphere& sphere)
{
    if (ui::begin_menu("Origin")) {
        auto pos_in_ground = to<Vector3>(sphere.getFrame().getPositionInGround(state));
        const auto on_opened = [&state, pos_in_ground](const OpenSim::Frame& other_frame) {
            draw_point_translation_information_with_respect_to(other_frame, state, pos_in_ground);
        };
        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_opened, opyn::try_get_parent_frame(sphere.getFrame()));

        ui::end_menu();
    }
}

void osc::draw_calculate_radius_menu(
    const OpenSim::Component&, const SimTK::State&, const OpenSim::Sphere& sphere)
{
    if (ui::begin_menu("Radius")) {
        double d = sphere.get_radius();
        ui::draw_double_input("radius", &d);
        ui::end_menu();
    }
}

void osc::draw_calculate_volume_menu(
    const OpenSim::Component&, const SimTK::State&, const OpenSim::Sphere& sphere)
{
    if (ui::begin_menu("Volume")) {
        const double r = sphere.get_radius();
        double v = 4.0 / 3.0 * SimTK::Pi * r * r * r;
        ui::draw_double_input("volume", &v, 0.0, 0.0, "%.6f", ui::TextInputFlag::ReadOnly);
        ui::end_menu();
    }
}

void osc::draw_calculate_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Geometry& geom,
    CalculateMenuFlags flags)
{
    if (begin_calculate_menu(flags)) {
        if (const auto* sphere_ptr = dynamic_cast<const OpenSim::Sphere*>(&geom)) {
            draw_calculate_origin_menu(root, state, *sphere_ptr);
            draw_calculate_radius_menu(root, state, *sphere_ptr);
            draw_calculate_volume_menu(root, state, *sphere_ptr);
        } else {
            draw_calculate_transform_menu(root, state, geom.getFrame());
            draw_calculate_origin_menu(root, state, geom.getFrame());
            draw_calculate_axis_directions_menu(root, state, geom.getFrame());
        }
        end_calculate_menu();
    }
}

void osc::try_draw_calculate_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Component& selected,
    CalculateMenuFlags flags)
{
    if (const auto* const frame = dynamic_cast<const OpenSim::Frame*>(&selected)) {
        draw_calculate_menu(root, state, *frame, flags);
    } else if (const auto* const point = dynamic_cast<const OpenSim::Point*>(&selected)) {
        draw_calculate_menu(root, state, *point, flags);
    }
}

void osc::draw_calculate_origin_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Ellipsoid& ellipsoid)
{
    if (ui::begin_menu("Origin")) {
        auto pos_in_ground = to<Vector3>(ellipsoid.getFrame().getPositionInGround(state));
        const auto on_opened = [&state, pos_in_ground](const OpenSim::Frame& other_frame) {
            draw_point_translation_information_with_respect_to(other_frame, state, pos_in_ground);
        };
        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_opened, opyn::try_get_parent_frame(ellipsoid.getFrame()));

        ui::end_menu();
    }
}

void osc::draw_calculate_radii_menu(
    const OpenSim::Component&, const SimTK::State&, const OpenSim::Ellipsoid& ellipsoid)
{
    if (ui::begin_menu("Radii")) {
        auto v = to<Vector3>(ellipsoid.get_radii());
        ui::draw_text("radii");
        ui::same_line();
        ui::draw_vector3_input("##radii", v, "%.6f", ui::TextInputFlag::ReadOnly);
        ui::end_menu();
    }
}

void osc::draw_calculate_radii_directions_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Ellipsoid& ellipsoid)
{
    draw_calculate_axis_directions_menu(root, state, ellipsoid.getFrame());
}

void osc::draw_calculate_scaled_radii_directions_menu(
    const OpenSim::Component& root, const SimTK::State& state, const OpenSim::Ellipsoid& ellipsoid)
{
    if (ui::begin_menu("Axis Directions (Scaled by Radii)")) {
        const auto on_frame_menu_opened = [&state, &ellipsoid](const OpenSim::Frame& other) {
            const auto& radii = ellipsoid.get_radii();
            auto x = to<Vector3>(
                radii[0] *
                ellipsoid.getFrame().expressVectorInAnotherFrame(state, {1.0, 0.0, 0.0}, other));
            auto y = to<Vector3>(
                radii[1] *
                ellipsoid.getFrame().expressVectorInAnotherFrame(state, {0.0, 1.0, 0.0}, other));
            auto z = to<Vector3>(
                radii[2] *
                ellipsoid.getFrame().expressVectorInAnotherFrame(state, {0.0, 0.0, 1.0}, other));

            ui::draw_text("x axis");
            ui::same_line();
            ui::draw_vector3_input("##xdir", x, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("y axis");
            ui::same_line();
            ui::draw_vector3_input("##ydir", y, "%.6f", ui::TextInputFlag::ReadOnly);

            ui::draw_text("z axis");
            ui::same_line();
            ui::draw_vector3_input("##zdir", z, "%.6f", ui::TextInputFlag::ReadOnly);
        };
        draw_with_respect_to_menu_containing_menu_per_frame(
            root, on_frame_menu_opened, opyn::try_get_parent_frame(ellipsoid.getFrame()));
        ui::end_menu();
    }
}

void osc::draw_calculate_menu(
    const OpenSim::Component& root,
    const SimTK::State& state,
    const OpenSim::Ellipsoid& ellipsoid,
    CalculateMenuFlags flags)
{
    if (begin_calculate_menu(flags)) {
        draw_calculate_origin_menu(root, state, ellipsoid);
        draw_calculate_radii_menu(root, state, ellipsoid);
        draw_calculate_radii_directions_menu(root, state, ellipsoid);
        draw_calculate_scaled_radii_directions_menu(root, state, ellipsoid);
        end_calculate_menu();
    }
}

bool osc::draw_muscle_rendering_options_radio_buttions(opyn::OpenSimDecorationOptions& opts)
{
    const opyn::MuscleDecorationStyle current_style = opts.get_muscle_decoration_style();
    bool edited = false;
    for (const auto& metadata : opyn::get_all_muscle_decoration_style_metadata()) {
        if (ui::draw_radio_button(metadata.label, metadata.value == current_style)) {
            opts.set_muscle_decoration_style(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::draw_muscle_sizing_options_radio_buttons(opyn::OpenSimDecorationOptions& opts)
{
    const opyn::MuscleSizingStyle current_style = opts.get_muscle_sizing_style();
    bool edited = false;
    for (const auto& metadata : opyn::get_all_muscle_sizing_style_metadata()) {
        if (ui::draw_radio_button(metadata.label, metadata.value == current_style)) {
            opts.set_muscle_sizing_style(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::draw_muscle_color_source_options_radio_buttons(opyn::OpenSimDecorationOptions& opts)
{
    const opyn::MuscleColorSource current_style = opts.get_muscle_color_source();
    bool edited = false;
    for (const auto& metadata : opyn::get_all_possible_muscle_coloring_sources_metadata()) {
        if (ui::draw_radio_button(metadata.label, metadata.value == current_style)) {
            opts.set_muscle_color_source(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::draw_muscle_color_scaling_options_radio_buttons(opyn::OpenSimDecorationOptions& opts)
{
    const opyn::MuscleColorSourceScaling current_style = opts.get_muscle_color_source_scaling();
    bool edited = false;
    for (const auto& metadata : opyn::get_all_possible_muscle_color_source_scaling_metadata()) {
        if (ui::draw_radio_button(metadata.label, metadata.value == current_style)) {
            opts.set_muscle_color_source_scaling(metadata.value);
            edited = true;
        }
    }
    return edited;
}

bool osc::draw_muscle_decoration_options_editor(opyn::OpenSimDecorationOptions& opts)
{
    int id = 0;
    bool edited = false;

    ui::push_id(id++);
    ui::draw_text_disabled("Rendering");
    edited = draw_muscle_rendering_options_radio_buttions(opts) || edited;
    ui::pop_id();

    ui::draw_vertical_spacer(0.25f);
    ui::push_id(id++);
    ui::draw_text_disabled("Sizing");
    edited = draw_muscle_sizing_options_radio_buttons(opts) || edited;
    ui::pop_id();

    ui::draw_vertical_spacer(0.25f);
    ui::push_id(id++);
    ui::draw_text_disabled("Color Source");
    edited = draw_muscle_color_source_options_radio_buttons(opts) || edited;
    ui::pop_id();

    ui::draw_vertical_spacer(0.25f);
    ui::push_id(id++);
    ui::draw_text_disabled("Color Scaling");
    edited = draw_muscle_color_scaling_options_radio_buttons(opts) || edited;
    ui::pop_id();

    return edited;
}

bool osc::draw_rendering_options_editor(opyn::CustomRenderingOptions& opts)
{
    bool edited = false;
    ui::draw_text_disabled("Rendering");
    for (size_t i = 0; i < opts.get_num_options(); ++i) {
        bool value = opts.get_option_value(i);
        if (ui::draw_checkbox(opts.get_option_label(i), &value)) {
            opts.set_option_value(i, value);
            edited = true;
        }
    }
    return edited;
}

bool osc::draw_overlay_options_editor(opyn::OverlayDecorationOptions& opts)
{
    std::optional<CStringView> last_group_label;
    bool edited = false;
    for (size_t i = 0; i < opts.get_num_options(); ++i) {
        // print header, if necessary
        const CStringView group_label = opts.get_option_group_label(i);
        if (group_label != last_group_label) {
            if (last_group_label) {
                ui::draw_vertical_spacer(0.25f);
            }
            ui::draw_text_disabled(group_label);
            last_group_label = group_label;
        }

        bool value = opts.get_option_value(i);
        if (ui::draw_checkbox(opts.get_option_label(i), &value)) {
            opts.set_option_value(i, value);
            edited = true;
        }
    }
    return edited;
}

bool osc::draw_custom_decoration_option_checkboxes(opyn::OpenSimDecorationOptions& opts)
{
    int imgui_id = 0;
    bool edited = false;
    for (size_t i = 0; i < opts.get_num_options(); ++i) {
        ui::push_id(imgui_id++);

        bool v = opts.get_option_value(i);
        if (ui::draw_checkbox(opts.get_option_label(i), &v)) {
            opts.set_option_value(i, v);
            edited = true;
        }
        if (std::optional<CStringView> description = opts.get_option_description(i)) {
            ui::same_line();
            ui::draw_help_marker(*description);
        }

        ui::pop_id();
    }
    return edited;
}

bool osc::draw_orbit_camera_controller_editor(OrbitCameraController& orbit_camera_controller)
{
    bool edited = false;
    edited = ui::draw_float_meters_slider(
                 "pan_x", orbit_camera_controller.focus_point.x(), -100.0f, 100.0f) or
             edited;
    edited = ui::draw_float_meters_slider(
                 "pan_y", orbit_camera_controller.focus_point.y(), -100.0f, 100.0f) or
             edited;
    edited = ui::draw_float_meters_slider(
                 "pan_z", orbit_camera_controller.focus_point.z(), -100.0f, 100.0f) or
             edited;
    edited = ui::draw_float_meters_slider("radius", orbit_camera_controller.radius, 0.0f, 10.0f) or
             edited;
    edited =
        ui::draw_angle_slider("theta", orbit_camera_controller.theta, 0_deg, 360_deg) or edited;
    edited = ui::draw_angle_slider("phi", orbit_camera_controller.phi, 0_deg, 360_deg) or edited;
    return edited;
}

bool osc::draw_advanced_params_editor(
    opyn::ModelRendererParams& params,
    OrbitCameraController& orbit_camera_controller,
    std::span<const SceneDecoration> drawlist)
{
    bool edited = false;

    if (ui::draw_button("Export to .dae")) {
        try_prompt_user_to_save_as_dae(drawlist);
    }
    ui::draw_tooltip_body_only_if_item_hovered(
        "Try to export the 3D scene to a portable DAE file, so that it can be viewed in 3rd-party modelling software, such as Blender");

    ui::draw_vertical_spacer(10.0f / 15.0f);
    ui::draw_text("advanced camera properties:");
    ui::draw_separator();

    if (draw_orbit_camera_controller_editor(orbit_camera_controller)) {
        orbit_camera_controller.update_camera(params.camera);
        edited = true;
    }
    if (auto fov = params.camera.vertical_field_of_view();
        ui::draw_angle_slider("vertial FoV", fov, 0_deg, 360_deg)) {

        params.camera.set_vertical_field_of_view(fov);
        edited = true;
    }
    if (auto znear = params.camera.near_clipping_plane();
        ui::draw_float_meters_input("znear", znear)) {

        params.camera.set_near_clipping_plane(znear);
        edited = true;
    }
    if (auto zfar = params.camera.far_clipping_plane(); ui::draw_float_meters_input("zfar", zfar)) {
        params.camera.set_far_clipping_plane(zfar);
        edited = true;
    }
    if (ui::draw_float3_meters_input("target", orbit_camera_controller.focus_point)) {
        orbit_camera_controller.update_camera(params.camera);
        edited = true;
    }
    if (Vector3 position = orbit_camera_controller.position();
        ui::draw_float3_meters_input("position", position)) {

        orbit_camera_controller.set_position(position);
        params.camera.set_position(position);
        edited = true;
    }

    ui::draw_vertical_spacer(10.0f / 15.0f);
    ui::draw_text("advanced scene properties:");
    ui::draw_separator();
    edited = ui::draw_rgb_color_editor("light_color", params.light_color) || edited;
    edited = ui::draw_rgb_color_editor("background color", params.background_color) || edited;
    edited = ui::draw_float3_meters_input("floor location", params.floor_location) || edited;
    ui::draw_tooltip_body_only_if_item_hovered(
        "Set the origin location of the scene's chequered floor. This is handy if you are working on smaller models, or models that need a floor somewhere else");

    return edited;
}

bool osc::draw_visual_aids_context_menu_content(opyn::ModelRendererParams& params)
{
    bool edited = false;

    // generic rendering options
    edited = draw_rendering_options_editor(params.rendering_options) || edited;

    // overlay options
    edited = draw_overlay_options_editor(params.overlay_options) || edited;

    // OpenSim-specific extra rendering options
    ui::draw_vertical_spacer(0.25f);
    ui::draw_text_disabled("OpenSim");
    edited = draw_custom_decoration_option_checkboxes(params.decoration_options) || edited;

    return edited;
}

bool osc::draw_viewer_top_button_row(
    opyn::ModelRendererParams& params,
    std::span<const SceneDecoration>,
    IconCache& icon_cache,
    const std::function<bool()>& draw_extra_elements)
{
    bool edited = false;

    IconWithMenu muscle_styling_button{
        icon_cache.find_or_throw("muscle_coloring"),
        "Muscle Styling",
        "Affects how muscles appear in this visualizer panel",
        [&params] { return draw_muscle_decoration_options_editor(params.decoration_options); },
    };
    edited = muscle_styling_button.on_draw() || edited;
    ui::same_line();

    IconWithMenu viz_aids_button{
        icon_cache.find_or_throw("viz_aids"),
        "Visual Aids",
        "Affects what's shown in the 3D scene",
        [&params] { return draw_visual_aids_context_menu_content(params); },
    };
    edited = viz_aids_button.on_draw() || edited;

    ui::same_line();
    ui::draw_vertical_separator();
    ui::same_line();

    // caller-provided extra buttons (usually, context-dependent)
    edited = draw_extra_elements() || edited;

    return edited;
}

bool osc::draw_camera_control_buttons(
    opyn::ModelRendererParams& params,
    OrbitCameraController& orbit_camera_controller,
    std::span<const SceneDecoration> drawlist,
    const Rect& viewer_screen_rect,
    const std::optional<AABB>& maybe_scene_aabb,
    IconCache& icon_cache,
    Vector2 desired_top_centroid)
{
    IconWithoutMenu zoom_out_button{
        icon_cache.find_or_throw("zoomout"),
        "Zoom Out Camera",
        "Moves the camera one step away from its focus point (Hotkey: -)",
    };
    IconWithoutMenu zoom_in_button{
        icon_cache.find_or_throw("zoomin"),
        "Zoom in Camera",
        "Moves the camera one step towards its focus point (Hotkey: =)",
    };
    IconWithoutMenu auto_focus_button{
        icon_cache.find_or_throw("zoomauto"),
        "Auto-Focus Camera",
        "Try to automatically adjust the camera's zoom etc. to suit the model's dimensions (Hotkey: Ctrl+F)",
    };
    IconWithMenu scene_settings_button{
        icon_cache.find_or_throw("gear"),
        "Scene Settings",
        "Change advanced scene settings",
        [&params, &orbit_camera_controller, drawlist] {
            return draw_advanced_params_editor(params, orbit_camera_controller, drawlist);
        },
    };

    auto c = ui::get_style_color(ui::ColorVar::Button);
    c.a *= 0.9f;
    ui::push_style_color(ui::ColorVar::Button, c);

    const float spacing = ui::get_style_item_spacing().x();
    float width = zoom_out_button.dimensions().x() + spacing + zoom_in_button.dimensions().x() +
                  spacing + auto_focus_button.dimensions().x();
    const Vector2 topleft = {
        desired_top_centroid.x() - (0.5f * width),
        desired_top_centroid.y() + (2.0f * ui::get_style_item_spacing().y())};
    ui::set_cursor_ui_position(topleft);

    bool edited = false;
    if (zoom_out_button.on_draw()) {
        orbit_camera_controller.zoom_out();
        orbit_camera_controller.update_camera(params.camera);
        edited = true;
    }
    ui::same_line();
    if (zoom_in_button.on_draw()) {
        orbit_camera_controller.zoom_in();
        orbit_camera_controller.update_camera(params.camera);
        edited = true;
    }
    ui::same_line();
    if (auto_focus_button.on_draw() && maybe_scene_aabb) {
        orbit_camera_controller.focus_on(
            *maybe_scene_aabb, params.camera, aspect_ratio_of(viewer_screen_rect));
        orbit_camera_controller.update_camera(params.camera);
        edited = true;
    }

    // next line (centered)
    {
        const Vector2 tl = {
            desired_top_centroid.x() - 0.5f * scene_settings_button.dimensions().x(),
            ui::get_cursor_ui_position().y(),
        };
        ui::set_cursor_ui_position(tl);
        if (scene_settings_button.on_draw()) {
            edited = true;
        }
    }

    ui::pop_style_color();

    return edited;
}

bool osc::draw_viewer_im_gui_overlays(
    opyn::ModelRendererParams& params,
    OrbitCameraController& orbit_camera_controller,
    std::span<const SceneDecoration> drawlist,
    std::optional<AABB> maybe_scene_aabb,
    const Rect& render_rect,
    IconCache& icon_cache,
    const std::function<bool()>& draw_extra_elements_in_top)
{
    bool edited = false;

    // draw top-left buttons
    const Vector2 window_padding = ui::get_style_panel_padding();
    ui::set_cursor_ui_position(render_rect.ypd_top_left() + window_padding);
    edited = draw_viewer_top_button_row(params, drawlist, icon_cache, draw_extra_elements_in_top) or
             edited;

    // draw top-right camera manipulators
    CameraViewAxes axes;
    const Vector2 render_dims = render_rect.dimensions();
    const Vector2 axes_dims = axes.dimensions();
    const Vector2 axes_top_left = {
        render_rect.left() + render_dims.x() - window_padding.x() - axes_dims.x(),
        render_rect.ypd_top() + window_padding.y(),
    };

    // draw the bottom overlays
    ui::set_cursor_ui_position(axes_top_left);
    if (axes.draw(orbit_camera_controller, params.camera)) {
        orbit_camera_controller.update_camera(params.camera);
        edited = true;
    }

    const Vector2 camera_buttons_top_left = axes_top_left + Vector2{0.0f, axes_dims.y()};
    ui::set_cursor_ui_position(camera_buttons_top_left);
    edited = draw_camera_control_buttons(
                 params,
                 orbit_camera_controller,
                 drawlist,
                 render_rect,
                 maybe_scene_aabb,
                 icon_cache,
                 {axes_top_left.x() + 0.5f * axes_dims.x(), axes_top_left.y() + axes_dims.y()}) or
             edited;

    return edited;
}

bool osc::begin_toolbar(CStringView label, std::optional<Vector2> padding)
{
    if (padding) {
        ui::push_style_var(ui::StyleVar::PanelPadding, *padding);
    }

    const float height = ui::get_frame_height() + 2.0f * ui::get_style_panel_padding().y();
    const ui::PanelFlags flags = {ui::PanelFlag::NoScrollbar, ui::PanelFlag::NoSavedSettings};
    bool open = ui::begin_main_window_top_bar(label, height, flags);
    if (padding) {
        ui::pop_style_var();
    }
    return open;
}

void osc::draw_new_model_button(Widget& api)
{
    if (ui::draw_button(MSMICONS_FILE)) {
        action_new_model(api);
    }
    ui::draw_tooltip_if_item_hovered("New Model", "Creates a new OpenSim model in a new tab");
}

void osc::draw_open_model_button_with_recent_files_dropdown(
    const std::function<void(std::optional<std::filesystem::path>)>&
        on_user_clicked_open_or_selected_file)
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});
    if (ui::draw_button(MSMICONS_FOLDER_OPEN)) {
        on_user_clicked_open_or_selected_file(std::nullopt);
    }
    ui::draw_tooltip_if_item_hovered("Open Model", "Opens an existing osim file in a new tab");
    ui::same_line();
    ui::push_style_var(ui::StyleVar::FramePadding, {1.0f, ui::get_style_frame_padding().y()});
    ui::draw_button(MSMICONS_CARET_DOWN);
    ui::draw_tooltip_if_item_hovered(
        "Open Recent File", "Opens a recently-opened osim file in a new tab");
    ui::pop_style_var();
    ui::pop_style_var();

    if (ui::begin_popup_context_menu("##RecentFilesMenu", ui::PopupFlag::MouseButtonLeft)) {
        const auto recent_files = App::singleton<RecentFiles>();
        int imgui_id = 0;

        for (const RecentFile& rf : *recent_files) {
            ui::push_id(imgui_id++);
            if (ui::draw_selectable(rf.path.filename().string())) {
                on_user_clicked_open_or_selected_file(rf.path);
            }
            ui::pop_id();
        }

        ui::end_popup();
    }
}

void osc::draw_open_model_button_with_recent_files_dropdown(Widget& api)
{
    draw_open_model_button_with_recent_files_dropdown([&api](auto maybe_file) {
        if (maybe_file) {
            action_open_model(api, *maybe_file);
        } else {
            action_open_model(api);
        }
    });
}

void osc::draw_save_model_button(const std::shared_ptr<opyn::ModelStatePair>& model)
{
    if (ui::draw_button(MSMICONS_SAVE)) {
        action_save_model_async(model);
    }
    ui::draw_tooltip_if_item_hovered("Save Model", "Saves the model to an osim file");
}

void osc::draw_reload_model_button(UndoableModelStatePair& model)
{
    const bool disable = model.is_readonly() or not opyn::has_input_file_name(model.get_model());

    if (disable) {
        ui::begin_disabled();
    }
    if (ui::draw_button(MSMICONS_RECYCLE)) {
        action_reload_osim_from_disk(model, *App::singleton<SceneCache>());
    }
    if (disable) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Reload Model", "Reloads the model from its source osim file");
}

void osc::draw_undo_button(opyn::ModelStatePair& model)
{
    auto* undoable = dynamic_cast<UndoableModelStatePair*>(&model);
    const bool disable = not(undoable != nullptr and undoable->can_undo());

    if (disable) {
        ui::begin_disabled();
    }
    if (ui::draw_button(MSMICONS_UNDO) and undoable != nullptr) {
        undoable->do_undo();
    }
    if (disable) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Undo", "Undo the model to an earlier version");
}

void osc::draw_redo_button(opyn::ModelStatePair& model)
{
    auto* undoable = dynamic_cast<UndoableModelStatePair*>(&model);
    const bool disable = not(undoable != nullptr and undoable->can_redo());

    if (disable) {
        ui::begin_disabled();
    }
    if (ui::draw_button(MSMICONS_REDO) and undoable != nullptr) {
        undoable->do_redo();
    }
    if (disable) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered("Redo", "Redo the model to an undone version");
}

void osc::draw_undo_and_redo_buttons(opyn::ModelStatePair& model)
{
    draw_undo_button(model);
    ui::same_line();
    draw_redo_button(model);
}

void osc::draw_toggle_frames_button(opyn::ModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(
        opyn::is_showing_frames(model.get_model()) ? "frame_colored" : "frame_bw");

    if (model.is_readonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button(
            "##toggleframes", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        action_toggle_frames(model);
    }
    if (model.is_readonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered(
        "Toggle Rendering Frames",
        "Toggles whether frames (coordinate systems) within the model should be rendered in the 3D scene.");
}

void osc::draw_toggle_markers_button(opyn::ModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(
        opyn::is_showing_markers(model.get_model()) ? "marker_colored" : "marker");
    if (model.is_readonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button(
            "##togglemarkers", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        action_toggle_markers(model);
    }
    if (model.is_readonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered(
        "Toggle Rendering Markers", "Toggles whether markers should be rendered in the 3D scene");
}

void osc::draw_toggle_wrap_geometry_button(opyn::ModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(
        opyn::is_showing_wrap_geometry(model.get_model()) ? "wrap_colored" : "wrap");
    if (model.is_readonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button(
            "##togglewrapgeom", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        action_toggle_wrap_geometry(model);
    }
    if (model.is_readonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered(
        "Toggle Rendering Wrap Geometry",
        "Toggles whether wrap geometry should be rendered in the 3D scene.\n\nNOTE: This is a model-log_level_ property. Individual wrap geometries *within* the model may have their visibility set to 'false', which will cause them to be hidden from the visualizer, even if this is enabled.");
}

void osc::draw_toggle_contact_geometry_button(opyn::ModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(
        opyn::is_showing_contact_geometry(model.get_model()) ? "contact_colored" : "contact");
    if (model.is_readonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button(
            "##togglecontactgeom", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        action_toggle_contact_geometry(model);
    }
    if (model.is_readonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered(
        "Toggle Rendering Contact Geometry",
        "Toggles whether contact geometry should be rendered in the 3D scene");
}

void osc::draw_toggle_forces_button(opyn::ModelStatePair& model, IconCache& icons)
{
    const Icon& icon = icons.find_or_throw(
        opyn::is_showing_forces(model.get_model()) ? "forces_colored" : "forces_bw");
    if (model.is_readonly()) {
        ui::begin_disabled();
    }
    if (ui::draw_image_button(
            "##toggleforces", icon.texture(), icon.dimensions(), icon.texture_coordinates())) {
        action_toggle_forces(model);
    }
    if (model.is_readonly()) {
        ui::end_disabled();
    }
    ui::draw_tooltip_if_item_hovered(
        "Toggle Rendering Forces",
        "Toggles whether forces should be rendered in the 3D scene.\n\nNOTE: this is a model-level property that only applies to forces in OpenSim that actually check this flag. OpenSim Creator's visualizers also offer custom overlays for forces, muscles, etc. separately to this mechanism.");
}

void osc::draw_all_decoration_toggle_buttons(opyn::ModelStatePair& model, IconCache& icons)
{
    draw_toggle_frames_button(model, icons);
    ui::same_line();
    draw_toggle_markers_button(model, icons);
    ui::same_line();
    draw_toggle_wrap_geometry_button(model, icons);
    ui::same_line();
    draw_toggle_contact_geometry_button(model, icons);
    ui::same_line();
    draw_toggle_forces_button(model, icons);
}

void osc::draw_scene_scale_factor_editor_controls(opyn::ModelStatePair& model)
{
    ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
    ui::draw_text(MSMICONS_EXPAND_ALT);
    ui::draw_tooltip_if_item_hovered(
        "Scene Scale Factor",
        "Rescales decorations in the model by this amount. Changing this can be handy when working on extremely small/large models.");
    ui::same_line();

    {
        float scale_factor = model.get_fixup_scale_factor();
        ui::set_next_item_width(ui::calc_text_size("0.00000").x());
        if (ui::draw_float_input("##scaleinput", &scale_factor)) {
            model.set_fixup_scale_factor(scale_factor);
        }
    }
    ui::pop_style_var();

    ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});
    ui::same_line();
    if (ui::draw_button(MSMICONS_EXPAND_ARROWS_ALT)) {
        action_autoscale_scene_scale_factor(model);
    }
    ui::pop_style_var();
    ui::draw_tooltip_if_item_hovered(
        "Autoscale Scale Factor",
        "Try to autoscale the model's scale factor based on the current dimensions of the model");
}

void osc::draw_mesh_export_context_menu_content(
    const opyn::ModelStatePair& model, const OpenSim::Mesh& mesh)
{
    ui::draw_text_disabled("Format:");
    ui::draw_separator();

    if (ui::begin_menu(".obj")) {
        const auto on_frame_menu_item_clicked = [&model, &mesh](const OpenSim::Frame& frame) {
            action_reexport_mesh_obj_with_respect_to(
                model.get_model(), model.get_state(), mesh, frame);
        };

        draw_with_respect_to_menu_containing_menu_item_per_frame(
            model.get_model(), on_frame_menu_item_clicked);
        ui::end_menu();
    }

    if (ui::begin_menu(".stl")) {
        const auto on_frame_menu_item_clicked = [&model, &mesh](const OpenSim::Frame& frame) {
            action_reexport_mesh_stl_with_respect_to(
                model.get_model(), model.get_state(), mesh, frame);
        };

        draw_with_respect_to_menu_containing_menu_item_per_frame(
            model.get_model(), on_frame_menu_item_clicked);
        ui::end_menu();
    }
}
