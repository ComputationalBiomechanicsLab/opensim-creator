#include "meshes_to_model_wizard_screen_step2.hpp"

#include "src/3d/cameras.hpp"
#include "src/application.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"
#include "src/log.hpp"
#include "src/constants.hpp"
#include "src/screens/model_editor_screen.hpp"

#include <imgui.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>

using namespace osc;

// a body/frame that the user has added into the model
//
// these are expressed in *absolute* (i.e. relative to ground) coordinates
// during this phase of model building. This is so that users can freely
// move them around in the scene - converting them into OpenSim's (relative)
// coordinate system happens later
struct Body_or_frame final {
    // -1 if parent is Ground; otherwise, the index of the parent
    int parent;

    // absolute position (i.e. relative to `Ground`)
    glm::vec3 pos;

    // is it a body or a frame?
    //
    // both are treated basically identically in this step of the wizard
    // and this flag essentially decides whether to emit an OpenSim::Body
    // or an OpenSim::PhysicalFrame at the end
    enum class Type { Body, Frame };
    Type type;

    // true if it is selected in the UI
    bool is_selected;

    // true if it is hovered in the UI
    bool is_hovered;
};

// type of element in the scene
enum class ElType { None, Mesh, Body, Ground };

// state associated with an open context menu
struct Context_menu final {
    // index of the element
    //
    // -1 for ground, which doesn't require an index
    int idx = -1;

    // type of element the context menu is open for
    ElType type;
};

// state associated with assigning a parent to a body, frame, or
// mesh in the scene
//
// in all cases, the "assignee" is going to be a body, frame, or
// ground
//
// this class has an "inactive" state. It is only activated when
// the user explicitly requests to assign an element in the scene
struct Parent_assignment_state final {
    // index of assigner
    //
    // <0 (usually, -1) if this state is "inactive"
    int assigner = -1;

    // true if assigner is body; otherwise, assume the assigner is
    // a mesh
    bool is_body = false;
};

struct Hovertest_result final {
    // index of the hovered element
    //
    // ignore if type == None or type == Ground
    int idx = -1;

    // type of hovered element
    ElType type = ElType::None;
};

// top-level screen state
//
// the core UI rendering loop, user interactions, etc. are maintaining this
struct osc::Meshes_to_model_wizard_screen_step2::Impl final {
    // decorative meshes loaded during step 1
    std::vector<Loaded_user_mesh> meshes;

    // the bodies/frames that the user adds during this step
    std::vector<Body_or_frame> bodies;

    // set by draw step to render's topleft location in screenspace
    glm::vec2 render_topleft_in_screen = {0.0f, 0.0f};

    // color of assigned (i.e. attached to a body/frame) meshes
    // rendered in the 3D scene
    Rgba32 assigned_mesh_color = Rgba32::from_f4(1.0f, 1.0f, 1.0f, 1.0f);

    // color of unassigned meshes rendered in the 3D scene
    Rgba32 unassigned_mesh_color = Rgba32::from_u32(0xFFE4E4FF);

    // color of ground (sphere @ 0,0,0) rendered in the 3D scene
    Rgba32 ground_color = Rgba32::from_f4(0.0f, 0.0f, 1.0f, 1.0f);

    // color of a body rendered in the 3D scene
    Rgba32 body_color = Rgba32::from_f4(1.0f, 0.0f, 0.0f, 1.0f);

    // color of a frame rendered in the 3D scene
    Rgba32 frame_color = Rgba32::from_f4(0.0f, 1.0f, 0.0f, 1.0f);

    // radius of rendered ground sphere
    float ground_sphere_radius = 0.008f;

    // radius of rendered bof spheres
    float bof_sphere_radius = 0.005f;

    // 3D rendering parameters for backend
    Render_params renderparams;

    // list of elements to render in the 3D scene
    Drawlist drawlist;

    // output targets (textures, framebuffers) for 3D scene
    Render_target rendertarg;

    // primary 3D scene camera
    Polar_perspective_camera camera;

    // context menu state
    //
    // values in this member are set when the menu is initially
    // opened by an ImGui::OpenPopup call
    Context_menu ctx_menu;

    // hovertest result
    //
    // set by the implementation if it detects the mouse is over
    // an element in the scene
    Hovertest_result hovertest_result;

    // parent assignment state
    //
    // values in this member are set when the user explicitly requests
    // that they want to assign a mesh/bof (i.e. when they want to
    // assign a parent)
    Parent_assignment_state assignment_st;

    // set to true by the implementation if mouse is over the 3D scene
    bool mouse_over_render = false;

    // set to true by the implementation if ground (0, 0, 0) is hovered
    bool ground_hovered = false;

    // true if a chequered floor should be drawn
    bool show_floor = true;

    // true if meshes should be drawn
    bool show_meshes = true;

    // true if ground should be drawn
    bool show_ground = true;

    // true if bofs should be drawn
    bool show_bofs = true;

    // true if all connection lines between entities should be
    // drawn, rather than just *hovered* entities
    bool show_all_connection_lines = false;

    // true if meshes should be drawn
    bool lock_meshes = false;

    // true if ground shouldn't be clickable in the 3D scene
    bool lock_ground = false;

    // true if BOFs shouldn't be clickable in the 3D scene
    bool lock_bofs = false;

    // issues in this Impl that prevent the user from advancing
    // (and creating an OpenSim::Model, etc.)
    std::vector<std::string> advancement_issues;

    // model created by this wizard
    //
    // `nullptr` until the model is successfully created
    std::unique_ptr<OpenSim::Model> output_model = nullptr;

    Impl(std::vector<Loaded_user_mesh> lums) : meshes{std::move(lums)} {
    }
};

// returns worldspace center of a bof
[[nodiscard]] static constexpr glm::vec3 center(Body_or_frame const& bof) noexcept {
    return bof.pos;
}

// returns worldspace center of a loaded user mesh
[[nodiscard]] static glm::vec3 center(Loaded_user_mesh const& lum) noexcept {
    glm::vec3 c = aabb_center(lum.aabb);
    return glm::vec3{lum.model_mtx * glm::vec4{c, 1.0f}};
}

// returns a mesh instance that represents a chequered floor in the scene
[[nodiscard]] static Mesh_instance create_chequered_floor_meshinstance() {
    glm::mat4 model_mtx = []() {
        glm::mat4 rv = glm::identity<glm::mat4>();

        // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
        // floor down *slightly* to prevent Z fighting from planes rendered from the
        // model itself (the contact planes, etc.)
        rv = glm::translate(rv, {0.0f, -0.0001f, 0.0f});
        rv = glm::rotate(rv, osc::pi_f / 2, {-1.0, 0.0, 0.0});
        rv = glm::scale(rv, {100.0f, 100.0f, 1.0f});

        return rv;
    }();

    Mesh_instance mi;
    mi.model_xform = model_mtx;
    mi.normal_xform = normal_matrix(mi.model_xform);
    auto& gpu_storage = Application::current().get_gpu_storage();
    mi.meshidx = gpu_storage.floor_quad_idx;
    mi.texidx = gpu_storage.chequer_idx;
    mi.flags.set_skip_shading();

    return mi;
}

// sets `is_selected` of all selectable entities in the scene
static void set_is_selected_of_all_to(Meshes_to_model_wizard_screen_step2::Impl& impl, bool v) {
    for (Loaded_user_mesh& lum : impl.meshes) {
        lum.is_selected = v;
    }
    for (Body_or_frame& bof : impl.bodies) {
        bof.is_selected = v;
    }
}

// sets `is_hovered` of all hoverable entities in the scene
static void set_is_hovered_of_all_to(Meshes_to_model_wizard_screen_step2::Impl& impl, bool v) {
    for (Loaded_user_mesh& lum : impl.meshes) {
        lum.is_hovered = v;
    }
    impl.ground_hovered = v;
    for (Body_or_frame& bof : impl.bodies) {
        bof.is_hovered = v;
    }
}

// sets all hovered elements as selected elements
//
// (and all not-hovered elements as not selected)
static void set_hovered_els_as_selected(Meshes_to_model_wizard_screen_step2::Impl& impl) {
    for (Loaded_user_mesh& lum : impl.meshes) {
        lum.is_selected = lum.is_hovered;
    }
    for (Body_or_frame& bof : impl.bodies) {
        bof.is_selected = bof.is_hovered;
    }
}

// update the scene's camera based on (ImGui's) user input
static void update_camera_from_user_input(Meshes_to_model_wizard_screen_step2::Impl& impl) {
    if (!impl.mouse_over_render) {
        return;
    }

    // handle scroll zooming
    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/5.0f;

    // handle panning/zooming/dragging with middle mouse
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {

        // in pixels, e.g. [800, 600]
        glm::vec2 screendims = impl.rendertarg.dimensions();

        // in pixels, e.g. [-80, 30]
        glm::vec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

        // as a screensize-independent ratio, e.g. [-0.1, 0.05]
        glm::vec2 relative_delta = mouse_delta / screendims;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse performs a pan
            float aspect_ratio = screendims.x / screendims.y;
            pan(impl.camera, aspect_ratio, relative_delta);
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse performs a zoom
            impl.camera.radius *= 1.0f + relative_delta.y;
        } else {
            // just middle-mouse performs a mouse drag
            drag(impl.camera, relative_delta);
        }
    }
}

// delete all selected elements
static void action_delete_selected(Meshes_to_model_wizard_screen_step2::Impl& impl) {

    // nothing refers to meshes, so they can be removed straightforwardly
    auto& meshes = impl.meshes;
    auto it = std::remove_if(meshes.begin(), meshes.end(), [](auto const& m) { return m.is_selected; });
    meshes.erase(it, meshes.end());

    // bodies/frames, and meshes, can refer to other bodies/frames (they're a tree)
    // so deletion needs to update the `assigned_body` and `parent` fields of every
    // other body/frame/mesh to be correct post-deletion

    auto& bodies = impl.bodies;

    // perform parent/assigned_body fixups
    //
    // collect a list of to-be-deleted indices, going from big to small
    std::vector<int> deleted_indices;
    for (int i = static_cast<int>(bodies.size()) - 1; i >= 0; --i) {
        Body_or_frame& b = bodies[i];
        if (b.is_selected) {
            deleted_indices.push_back(static_cast<int>(i));
        }
    }

    // for each index in the (big to small) list, fixup the entries
    // to point to a fixed-up location
    //
    // the reason it needs to be big-to-small is to prevent the sitation
    // where decrementing an index makes it point at a location that appears
    // to be equal to a to-be-deleted location
    for (int idx : deleted_indices) {
        for (Body_or_frame& b : bodies) {
            if (b.parent == idx) {
                b.parent = bodies[static_cast<size_t>(idx)].parent;
            }
            if (b.parent > idx) {
                --b.parent;
            }
        }

        for (Loaded_user_mesh& lum : meshes) {
            if (lum.assigned_body == idx) {
                lum.assigned_body = bodies[static_cast<size_t>(idx)].parent;
            }
            if (lum.assigned_body > idx) {
                --lum.assigned_body;
            }
        }
    }

    // with the fixups done, we can now just remove the selected elements as
    // normal
    auto it2 = std::remove_if(bodies.begin(), bodies.end(), [](auto const& b) { return b.is_selected; });
    bodies.erase(it2, bodies.end());
}

// add frame to model
static void action_add_frame(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, glm::vec3 pos) {
    set_is_selected_of_all_to(impl, false);

    Body_or_frame& bf = impl.bodies.emplace_back();
    bf.parent = -1;
    bf.pos = pos;
    bf.type = Body_or_frame::Type::Frame;
    bf.is_selected = true;
    bf.is_hovered = false;
}

// add body to model
static void action_add_body(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, glm::vec3 pos) {
    set_is_hovered_of_all_to(impl, false);

    Body_or_frame& bf = impl.bodies.emplace_back();
    bf.parent = -1;
    bf.pos = pos;
    bf.type = Body_or_frame::Type::Body;
    bf.is_selected = true;
    bf.is_hovered = false;
}

// update the screen state (impl) based on (ImGui's) user input
static void update_impl_from_user_input(Meshes_to_model_wizard_screen_step2::Impl& impl) {
    // DELETE: delete any selecte elements
    if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
        action_delete_selected(impl);
    }

    // B: add body to hovered element
    if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
        set_is_selected_of_all_to(impl, false);

        for (auto const& b : impl.bodies) {
            if (b.is_hovered) {
                action_add_body(impl, b.pos);
                return;
            }
        }
        if (impl.ground_hovered) {
            action_add_body(impl, {0.0f, 0.0f, 0.0f});
            return;
        }
        for (auto const& m : impl.meshes) {
            if (m.is_hovered) {
                action_add_body(impl, center(m));
                return;
            }
        }
    }

    // A: assign a parent for the hovered element
    if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
        impl.assignment_st.assigner = -1;

        for (size_t i = 0; i < impl.bodies.size(); ++i) {
            Body_or_frame const& bof = impl.bodies[i];
            if (bof.is_hovered) {
                impl.assignment_st.assigner = static_cast<int>(i);
                impl.assignment_st.is_body = true;
            }
        }
        for (size_t i = 0; i < impl.meshes.size(); ++i) {
            Loaded_user_mesh const& m = impl.meshes[i];
            if (m.is_hovered) {
                impl.assignment_st.assigner = static_cast<int>(i);
                impl.assignment_st.is_body = false;
            }
        }

        if (impl.assignment_st.assigner != -1) {
            set_hovered_els_as_selected(impl);
        }
    }

    // ESC: leave assignment state
    if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        impl.assignment_st.assigner = -1;
    }

    // CTRL+A: select all
    if ((ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
        set_is_selected_of_all_to(impl, true);
    }
}

// convert a 3D worldspace coordinate into a 2D screenspace coordinate
//
// used to draw 2D overlays for items that are in 3D
static glm::vec2 world2screen(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, glm::vec3 const& v) {
    glm::mat4 const& view = impl.renderparams.view_matrix;
    glm::mat4 const& persp = impl.renderparams.projection_matrix;

    // range: [-1,+1] for XY
    glm::vec4 clipspace_pos = persp * view * glm::vec4{v, 1.0f};

    // perspective division: 4D (affine) --> 3D
    clipspace_pos /= clipspace_pos.w;

    // range [0, +1] with Y starting in top-left
    glm::vec2 relative_screenpos{
        (clipspace_pos.x + 1.0f)/2.0f,
        -1.0f * ((clipspace_pos.y - 1.0f)/2.0f),
    };

    // current screen dimensions
    glm::vec2 screen_dimensions{
        static_cast<float>(impl.rendertarg.w),
        static_cast<float>(impl.rendertarg.h)
    };

    // range [0, w] (X) and [0, h] (Y)
    glm::vec2 screen_pos = screen_dimensions * relative_screenpos;

    return screen_pos;
}

// draw a 2D overlay line between a BOF and its parent
static void draw_bof_line_to_parent(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, Body_or_frame const& bof) {
    ImDrawList& dl = *ImGui::GetForegroundDrawList();

    glm::vec3 parent_pos;
    if (bof.parent < 0) {
        parent_pos = {0.0f, 0.0f, 0.0f};  // ground pos
    } else {
        Body_or_frame const& parent = impl.bodies.at(static_cast<size_t>(bof.parent));
        parent_pos = parent.pos;
    }

    ImVec2 p1 = world2screen(impl, bof.pos);
    ImVec2 p2 = world2screen(impl, parent_pos);
    ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

    dl.AddLine(p1, p2, color);
}

// draw 2D overlay (dotted lines between bodies, etc.)
static void draw_2d_overlay(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {

    // draw connection lines between BOFs
    for (size_t i = 0; i < impl.bodies.size(); ++i) {
        Body_or_frame const& bof = impl.bodies[i];

        // only draw connection lines if "all" requested *or* if it is
        // hovered
        if (!(impl.show_all_connection_lines || bof.is_hovered)) {
            continue;
        }

        // draw line from bof to its parent (bof/ground)
        draw_bof_line_to_parent(impl, bof);

        // draw line(s) from any other bofs connected to this one
        for (Body_or_frame const& b : impl.bodies) {
            if (b.parent == static_cast<int>(i)) {
                draw_bof_line_to_parent(impl, b);
            }
        }
    }
}

// draw hover tooltip when hovering over a user mesh
static void draw_mesh_hover_tooltip(osc::Meshes_to_model_wizard_screen_step2::Impl&, Loaded_user_mesh& m) {
    ImGui::BeginTooltip();
    ImGui::Text("filepath = %s", m.location.string().c_str());
    ImGui::TextUnformatted(m.assigned_body >= 0 ? "ASSIGNED" : "UNASSIGNED (to a body/frame)");
    ImGui::EndTooltip();
}

// draw hover tooltip when hovering over Ground
static void draw_ground_hover_tooltip(osc::Meshes_to_model_wizard_screen_step2::Impl&) {
    ImGui::BeginTooltip();
    ImGui::Text("Ground");
    ImGui::Text("(always present, and defined to be at (0, 0, 0))");
    ImGui::EndTooltip();
}

// draw hover tooltip when hovering over a BOF
static void draw_bof_hover_tooltip(osc::Meshes_to_model_wizard_screen_step2::Impl&, Body_or_frame& bof) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(bof.type == Body_or_frame::Type::Body ? "Body" : "Frame");
    ImGui::TextUnformatted(bof.parent >= 0 ? "Connected to another body/frame" : "Connected to ground");
    ImGui::EndTooltip();
}

// draw mesh context menu
static void draw_mesh_context_menu_content(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, Loaded_user_mesh& lum) {
    if (ImGui::MenuItem("add body")) {
        action_add_body(impl, center(lum));
    }

    if (ImGui::MenuItem("add frame")) {
        action_add_frame(impl, center(lum));
    }
}

// draw ground context menu
static void draw_ground_context_menu_content(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    if (ImGui::MenuItem("add body")) {
        action_add_body(impl, {0.0f, 0.0f, 0.0f});
    }

    if (ImGui::MenuItem("add frame")) {
        action_add_frame(impl, {0.0f, 0.0f, 0.0f});
    }
}

// draw bof context menu
static void draw_bof_context_menu_content(osc::Meshes_to_model_wizard_screen_step2::Impl&, Body_or_frame&) {
    ImGui::Text("(no actions available)");
}

// draw manipulation gizmos (the little handles that the user can click
// to move things in 3D)
static void draw_selection_manipulation_gizmos(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    int nselected = 0;
    glm::vec3 avg_center = {0.0f, 0.0f, 0.0f};

    for (Loaded_user_mesh const& lum : impl.meshes) {
        if (lum.is_selected) {
            avg_center += center(lum);
            ++nselected;
        }
    }
    for (Body_or_frame const& b : impl.bodies) {
        if (b.is_selected) {
            avg_center += center(b);
            ++nselected;
        }
    }
    avg_center /= static_cast<float>(nselected);

    if (nselected == 0) {
        return;  // do not draw manipulation widgets
    }

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, avg_center);
    glm::mat4 manipulated_mtx = translator;

    ImGuizmo::SetRect(
        impl.render_topleft_in_screen.x,
        impl.render_topleft_in_screen.y,
        static_cast<float>(impl.rendertarg.w),
        static_cast<float>(impl.rendertarg.h));
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

    bool manipulated = ImGuizmo::Manipulate(
        glm::value_ptr(impl.renderparams.view_matrix),
        glm::value_ptr(impl.renderparams.projection_matrix),
        ImGuizmo::TRANSLATE,
        ImGuizmo::WORLD,
        glm::value_ptr(manipulated_mtx),
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    // used to detect whether the mouse is "busy" with a gizmo
    //impl.is_using_gizmo = ImGuizmo::IsUsing();

    if (!manipulated) {
        return;
    }
    // else: apply manipulation

    glm::mat4 inverse_translator = glm::translate(glm::mat4{1.0f}, -avg_center);
    glm::mat4 raw_xform = inverse_translator * manipulated_mtx;
    glm::mat4 applied_xform = translator * raw_xform * inverse_translator;

    // update relevant positions/model matrices
    for (Loaded_user_mesh& lum : impl.meshes) {
        if (lum.is_selected) {
            lum.model_mtx = applied_xform * lum.model_mtx;
        }
    }
    for (Body_or_frame& b : impl.bodies) {
        if (b.is_selected) {
            b.pos = glm::vec3{applied_xform * glm::vec4{b.pos, 1.0f}};
        }
    }
}

// draw 3D scene into remainder of the ImGui panel's content region
static void draw_3d_scene(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    ImVec2 dims = ImGui::GetContentRegionAvail();

    // skip rendering steps if ImGui panel is too small
    if (dims.x < 1.0f || dims.y < 1.0f) {
        return;
    }

    // ensure render target dimensions match panel dimensions
    impl.rendertarg.reconfigure(
        static_cast<int>(dims.x),
        static_cast<int>(dims.y),
        Application::current().samples());

    // compute render position on the screen (needed by ImGuizmo)
    {
        glm::vec2 wp = ImGui::GetWindowPos();
        glm::vec2 cp = ImGui::GetCursorPos();
        impl.render_topleft_in_screen = wp + cp;
    }

    // populate 3D drawlist
    Drawlist& dl = impl.drawlist;
    dl.clear();

    // ID is a unique, accumulated, 1-based index into
    //
    // - meshes [1, nmeshes]
    // - ground (nmeshes, nmeshes+1]
    // - bodies/frames (nmeshes+1, nmeshes+1+nbodies]
    uint16_t id = 1;

    // add meshes to 3D scene
    if (impl.show_meshes) {
        for (Loaded_user_mesh const& um : impl.meshes) {
            Mesh_instance mi;
            mi.model_xform = um.model_mtx;
            mi.normal_xform = normal_matrix(mi.model_xform);
            mi.rgba = um.assigned_body >= 0 ? impl.assigned_mesh_color : impl.unassigned_mesh_color;
            mi.meshidx = um.gpu_meshidx;
            mi.passthrough.rim_alpha = um.is_selected ? 0xff : um.is_hovered ? 0x60 : 0x00;
            if (!impl.lock_meshes) {
                mi.passthrough.assign_u16(id);
            }
            ++id;
            dl.push_back(mi);
        }
    } else {
        id += static_cast<int>(impl.meshes.size());
    }

    // sphere data for drawing bodies/frames in 3D
    Meshidx sphereidx = Application::current().get_gpu_storage().simbody_sphere_idx;

    // add ground (defined to be at 0, 0, 0) to 3D scene
    if (impl.show_ground) {
        float r = impl.ground_sphere_radius;
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});

        Mesh_instance mi;
        mi.model_xform = scaler;
        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = impl.ground_color;
        mi.meshidx = sphereidx;
        mi.passthrough.rim_alpha = impl.ground_hovered ? 0x60 : 0x00;
        if (!impl.lock_ground) {
            mi.passthrough.assign_u16(id);
        }
        ++id;
        dl.push_back(mi);
    } else {
        ++id;
    }

    // add bodies/frames to 3D scene
    if (impl.show_bofs) {
        float r = impl.bof_sphere_radius;
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});

        for (Body_or_frame const& bf : impl.bodies) {
            Mesh_instance mi;
            mi.model_xform = glm::translate(glm::mat4{1.0f}, bf.pos) * scaler;
            mi.normal_xform = normal_matrix(mi.model_xform);
            if (bf.type == Body_or_frame::Type::Body) {
                mi.rgba = impl.body_color;
            } else {
                mi.rgba = impl.frame_color;
            }
            mi.meshidx = sphereidx;
            mi.passthrough.rim_alpha = bf.is_selected ? 0xff : bf.is_hovered ? 0x60 : 0x00;
            if (!impl.lock_bofs) {
                mi.passthrough.assign_u16(id);
            }
            ++id;
            dl.push_back(mi);
        }
    } else {
        id += static_cast<int>(impl.bodies.size());
    }

    // add chequered floor to 3D scene
    if (impl.show_floor) {
        dl.push_back(create_chequered_floor_meshinstance());
    }

    // make renderer hittest location match the mouse's location
    {
        glm::vec2 mousepos = ImGui::GetMousePos();
        glm::vec2 windowpos = ImGui::GetWindowPos();
        glm::vec2 cursor_in_window_pos = ImGui::GetCursorPos();
        glm::vec2 mouse_in_window_pos = mousepos - windowpos;
        glm::vec2 mouse_in_img_pos = mouse_in_window_pos - cursor_in_window_pos;

        impl.renderparams.hittest.x = static_cast<int>(mouse_in_img_pos.x);
        impl.renderparams.hittest.y = static_cast<int>(dims.y - mouse_in_img_pos.y);
    }

    // update renderer view + projection matrices to match scene camera
    impl.renderparams.view_matrix = view_matrix(impl.camera);
    impl.renderparams.projection_matrix = projection_matrix(impl.camera, impl.rendertarg.aspect_ratio());

    // RENDER: draw scene onto render target
    draw_scene(
        Application::current().get_gpu_storage(),
        impl.renderparams,
        impl.drawlist,
        impl.rendertarg);

    // blit rendered 3D scene to ImGui::Image
    {
        gl::Texture_2d& tex = impl.rendertarg.main();
        void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex.get()));
        ImVec2 image_dimensions{dims.x, dims.y};
        ImVec2 uv0{0.0f, 1.0f};
        ImVec2 uv1{1.0f, 0.0f};
        ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
        impl.mouse_over_render = ImGui::IsItemHovered();
    }

    // handle hovertest
    {
        auto const& meshes = impl.meshes;
        auto const& bodies = impl.bodies;

        // assumed result if nothing is hovered
        impl.hovertest_result.idx = -1;
        impl.hovertest_result.type = ElType::None;

        // set hovertest result based on hovered-over ID
        uint16_t hovered = impl.rendertarg.hittest_result.get_u16();
        if (0 < hovered && hovered <= meshes.size()) {
            // hovered over mesh
            impl.hovertest_result.idx = static_cast<int>(hovered - 1);
            impl.hovertest_result.type = ElType::Mesh;
        } else if (meshes.size() < hovered && hovered <= meshes.size()+1) {
            // hovered over ground
            impl.hovertest_result.idx = -1;
            impl.hovertest_result.type = ElType::Ground;
        } else if (meshes.size()+1 < hovered && hovered <= meshes.size()+1+bodies.size()) {
            // hovered over body
            impl.hovertest_result.idx = static_cast<int>(((hovered - meshes.size()) - 1) - 1);
            impl.hovertest_result.type = ElType::Body;
        }
    }
}

// standard event handler for the 3D scene hover-over
//
// (this differs from the event handler when *assigning*, though)
static void handle_hovertest_result(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {

    // reset all previous hover state
    set_is_hovered_of_all_to(impl, false);

    // this is set by the renderer
    Hovertest_result const& htr = impl.hovertest_result;

    if (htr.type == ElType::None) {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            !ImGuizmo::IsUsing() &&
            !(ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT))) {

            set_is_selected_of_all_to(impl, false);
        }
    } else if (htr.type == ElType::Mesh) {
        Loaded_user_mesh& lum = impl.meshes[static_cast<size_t>(htr.idx)];

        // set is_hovered
        lum.is_hovered = true;

        // draw hover tooltip
        draw_mesh_hover_tooltip(impl, lum);

        // open context menu (if applicable)
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            impl.ctx_menu.idx = htr.idx;
            impl.ctx_menu.type = ElType::Mesh;
            ImGui::OpenPopup("contextmenu");
        }

        // if left-clicked, select it
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing()) {
            // de-select everything if shift isn't down
            if (!(ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT))) {
                set_is_selected_of_all_to(impl, false);
            }

            // set clicked item as selected
            lum.is_selected = true;
        }
    } else if (htr.type == ElType::Ground) {
        // set ground_hovered
        impl.ground_hovered = true;

        // draw hover tooltip
        draw_ground_hover_tooltip(impl);

        // open context menu (if applicable)
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            impl.ctx_menu.idx = -1;
            impl.ctx_menu.type = ElType::Ground;
            ImGui::OpenPopup("contextmenu");
        }
    } else if (htr.type == ElType::Body) {
        Body_or_frame& bof = impl.bodies[static_cast<size_t>(htr.idx)];

        // set is_hovered
        bof.is_hovered = true;

        // draw hover tooltip
        draw_bof_hover_tooltip(impl, bof);

        // open context menu (if applicable)
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            impl.ctx_menu.idx = htr.idx;
            impl.ctx_menu.type = ElType::Body;
            ImGui::OpenPopup("contextmenu");
        }


        // if left-clicked, select it
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing()) {
            // de-select everything if shift isn't down
            if (!(ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT))) {
                set_is_selected_of_all_to(impl, false);
            }

            // set clicked item as selected
            bof.is_selected = true;
        }
    }
}

static void draw_scene_context_menu(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    // draw context menu (if ImGui::OpenPopup has been called)
    //
    // CARE: this should be done last, because a context menu may
    // mutate the state
    if (ImGui::BeginPopup("contextmenu")) {
        switch (impl.ctx_menu.type) {
        case ElType::Mesh:
            draw_mesh_context_menu_content(impl, impl.meshes[static_cast<size_t>(impl.ctx_menu.idx)]);
            break;
        case ElType::Ground:
            draw_ground_context_menu_content(impl);
            break;
        case ElType::Body:
            draw_bof_context_menu_content(impl, impl.bodies[static_cast<size_t>(impl.ctx_menu.idx)]);
            break;
        default:
            break;
        }
        ImGui::EndPopup();
    }
}

// draw main 3D scene viewer
static void draw_standard_3d_viewer(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    // render main 3D scene
    draw_3d_scene(impl);

    // handle any mousehover hits
    if (impl.mouse_over_render) {
        handle_hovertest_result(impl);
    }

    // draw 3D manipulation gizmos (the little user-moveable arrows etc.)
    draw_selection_manipulation_gizmos(impl);

    // draw 2D overlay (lines between items, text, etc.)
    draw_2d_overlay(impl);

    // draw context menu
    //
    // CARE: this can mutate the implementation's data (e.g. by allowing
    // the user to delete things)
    draw_scene_context_menu(impl);
}

// handle renderer hovertest result *when in assignment mode*
static void handle_hovertest_result_assignment_mode(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    if (impl.assignment_st.assigner < 0) {
        return;  // nothing being assigned right now?
    }

    // get location of thing being assigned
    glm::vec3 assigner_loc;
    if (impl.assignment_st.is_body) {
        assigner_loc = center(impl.bodies[static_cast<size_t>(impl.assignment_st.assigner)]);
    } else {
        assigner_loc = center(impl.meshes[static_cast<size_t>(impl.assignment_st.assigner)]);
    }

    // reset all previous hover state
    set_is_hovered_of_all_to(impl, false);

    // get hovertest result from rendering step
    Hovertest_result const& htr = impl.hovertest_result;

    if (htr.type == ElType::Body) {
        // bof is hovered

        Body_or_frame& bof = impl.bodies[static_cast<size_t>(htr.idx)];
        ImDrawList& dl = *ImGui::GetForegroundDrawList();
        glm::vec3 c = center(bof);

        // draw a line between the thing being assigned and this body
        ImVec2 p1 = world2screen(impl, assigner_loc);
        ImVec2 p2 = world2screen(impl, c);
        ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
        dl.AddLine(p1, p2, color);

        // if the user left-clicks, assign the hovered bof
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (impl.assignment_st.is_body) {
                impl.bodies[impl.assignment_st.assigner].parent = htr.idx;
            } else {
                impl.meshes[impl.assignment_st.assigner].assigned_body = htr.idx;
            }

            // exit assignment state
            impl.assignment_st.assigner = -1;
        }
    } else if (htr.type == ElType::Ground) {
        // ground is hovered

        ImDrawList& dl = *ImGui::GetForegroundDrawList();
        glm::vec3 c = {0.0f, 0.0f, 0.0f};

        // draw a line between the thing being assigned and the hovered ground
        ImVec2 p1 = world2screen(impl, assigner_loc);
        ImVec2 p2 = world2screen(impl, c);
        ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
        dl.AddLine(p1, p2, color);

        // if the user left-clicks, assign the ground
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (impl.assignment_st.is_body) {
                impl.bodies[impl.assignment_st.assigner].parent = htr.idx;
            } else {
                impl.meshes[impl.assignment_st.assigner].assigned_body = htr.idx;
            }

            // exit assignment state
            impl.assignment_st.assigner = -1;
        }
    }
}

// draw assignment 3D viewer
//
// this is a special 3D viewer that is presented that highlights, and
// only allows the selection of, bodies/frames in the scene
static void draw_assignment_3d_viewer(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    Rgba32 old_assigned_mesh_color = impl.assigned_mesh_color;
    Rgba32 old_unassigned_mesh_color = impl.unassigned_mesh_color;

    impl.assigned_mesh_color.a = 0x10;
    impl.unassigned_mesh_color.a = 0x10;

    draw_3d_scene(impl);

    if (impl.mouse_over_render) {
        handle_hovertest_result_assignment_mode(impl);
    }

    impl.assigned_mesh_color = old_assigned_mesh_color;
    impl.unassigned_mesh_color = old_unassigned_mesh_color;
}

// draw 3D viewer
static void wizard_step2_draw_3dviewer(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {

    // how the 3D viewer is handled depends on whether the user is assigning
    // something at the moment
    if (impl.assignment_st.assigner < 0) {
        draw_standard_3d_viewer(impl);
    } else {
        draw_assignment_3d_viewer(impl);
    }
}

// returns true if the bof is connected to ground - including whether it is
// connected to ground *via* some other bodies
//
// returns false if it is connected to bullshit (i.e. an invalid index) or
// to something that, itself, does not connect to ground (e.g. a cycle)
[[nodiscard]] static bool bof_is_connected_to_ground(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, int bofidx) {
    int jumps = 0;
    while (bofidx >= 0) {
        if (static_cast<size_t>(bofidx) >= impl.bodies.size()) {
            return false;  // out of bounds
        }

        bofidx = impl.bodies[static_cast<size_t>(bofidx)].parent;

        ++jumps;
        if (jumps > 100) {
            return false;
        }
    }

    return true;
}

// tests for issues that would prevent the scene from being transformed
// into a valid OpenSim::Model
//
// populates `impl.advancement_issues`
static void test_for_advancement_issues(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    impl.advancement_issues.clear();

    // ensure all meshes are assigned to a valid body/ground
    for (Loaded_user_mesh const& lum : impl.meshes) {
        if (lum.assigned_body < 0) {
            // assigned to ground (bad practice)
            // TODO impl.advancement_issues.push_back("a mesh is assigned to ground (it should be assigned to a body/frame)");
        } else if (static_cast<size_t>(lum.assigned_body) >= impl.bodies.size()) {
            // invalid index
            impl.advancement_issues.push_back("a mesh is assigned to an invalid body");
        }
    }

    // ensure all bodies are connected to ground *eventually*
    for (size_t i = 0; i < impl.bodies.size(); ++i) {
        Body_or_frame const& bof = impl.bodies[i];

        if (bof.parent < 0) {
            // ok: it's directly connected to ground
        } else if (static_cast<size_t>(bof.parent) >= impl.bodies.size()) {
            impl.advancement_issues.push_back("a body/frame is connected to a non-existent body/frame");
            // bad: connected to bullshit
        } else if (!bof_is_connected_to_ground(impl, static_cast<int>(i))) {
            impl.advancement_issues.push_back("a body/frame is not connected to ground");
        } else {
            // ok: it's connected to a body/frame that is connected to ground
        }
    }
}

// draw an ImGui color picker for an OSC Rgba32
static void Rgba32_ColorEdit4(char const* label, Rgba32* rgba) {
    ImVec4 col = ImGui::ColorConvertU32ToFloat4(rgba->to_u32());
    if (ImGui::ColorEdit4(label, reinterpret_cast<float*>(&col))) {
        *rgba = Rgba32::from_f4(col.x, col.y, col.z, col.w);
    }
}

// recursively add a body/frame (bof) tree to an OpenSim::Model
static void recursively_add_bof(
    osc::Meshes_to_model_wizard_screen_step2::Impl& impl,
    OpenSim::Model& model,
    int bof_idx,
    int parent_idx,
    OpenSim::PhysicalFrame& parent_pf) {

    if (bof_idx < 0) {
        // the index points to ground and can't be traversed any more
        return;
    }

    // parent position in worldspace
    glm::vec3 world_parent_pos = parent_idx < 0 ?
        glm::vec3{0.0f, 0.0f, 0.0f} :
        impl.bodies[static_cast<size_t>(parent_idx)].pos;

    // the body/frame to add in this step
    Body_or_frame const& bof = impl.bodies[static_cast<size_t>(bof_idx)];

    OpenSim::PhysicalFrame* added_pf = nullptr;

    // create body/pof and add it into the model
    if (bof.type == Body_or_frame::Type::Body) {
        // user requested a Body to be added
        //
        // use a POF to position the body correctly relative to its parent

        // joint that connects the POF to the body
        OpenSim::Joint* joint = new OpenSim::WeldJoint{};

        // the body
        OpenSim::Body* body = new OpenSim::Body{};
        body->setMass(1.0);

        // the POF that is offset from the parent physical frame
        OpenSim::PhysicalOffsetFrame* pof = new OpenSim::PhysicalOffsetFrame{};
        glm::vec3 world_bof_pos = bof.pos;

        {
            // figure out the parent's actual rotation, so that the relevant
            // vectors can be transformed into "parent space"
            model.finalizeFromProperties();
            model.finalizeConnections();
            SimTK::State s = model.initSystem();
            model.realizePosition(s);

            SimTK::Rotation rot_parent2world = parent_pf.getRotationInGround(s);
            SimTK::Rotation rot_world2parent = rot_parent2world.invert();

            // compute relevant vectors in worldspace (the screen's coordinate system)
            SimTK::Vec3 world_parent2bof = stk_vec3_from(world_bof_pos - world_parent_pos);
            SimTK::Vec3 world_bof2parent = stk_vec3_from(world_parent_pos - world_bof_pos);
            SimTK::Vec3 world_bof2parent_dir = world_bof2parent.normalize();

            SimTK::Vec3 parent_bof2parent_dir = rot_world2parent * world_bof2parent_dir;
            SimTK::Vec3 parent_y = {0.0f, 1.0f, 0.0f};  // by definition

            // create a "BOF space" that specifically points the Y axis
            // towards the parent frame (an OpenSim model building convention)
            SimTK::Transform xform_parent2bof{
                SimTK::Rotation{
                    glm::acos(SimTK::dot(parent_y, parent_bof2parent_dir)),
                    SimTK::cross(parent_y, parent_bof2parent_dir).normalize(),
                },
                SimTK::Vec3{rot_world2parent * world_parent2bof},  // translation
            };
            pof->setOffsetTransform(xform_parent2bof);
            pof->setParentFrame(parent_pf);
        }

        // link everything up
        joint->addFrame(pof);
        joint->connectSocket_parent_frame(*pof);
        joint->connectSocket_child_frame(*body);

        // add it all to the model
        model.addBody(body);
        model.addJoint(joint);

        // TODO: attach geometry
        for (Loaded_user_mesh const& lum : impl.meshes) {
            if (lum.assigned_body != bof_idx) {
                continue;
            }

            model.finalizeFromProperties();
            model.finalizeConnections();
            SimTK::State s = model.initSystem();
            model.realizePosition(s);

            SimTK::Transform xform_parent2ground = body->getTransformInGround(s);
            SimTK::Transform xform_ground2parent = xform_parent2ground.invert();

            // create a POF that attaches to the body
            //
            // this is necessary to independently transform the mesh relative
            // to the parent's transform (the mesh is currently transformed relative
            // to ground)
            OpenSim::PhysicalOffsetFrame* mesh_pof = new OpenSim::PhysicalOffsetFrame{};
            mesh_pof->setParentFrame(*body);

            // without setting `setObjectTransform`, the mesh will be subjected to
            // the POF's object-to-ground transform. so vertices in the matrix are
            // already in "object space" and we want to figure out how to transform
            // them as if they were in our current (world) space
            SimTK::Transform mmtx = to_transform(lum.model_mtx);

            mesh_pof->setOffsetTransform(xform_ground2parent * mmtx);
            body->addComponent(mesh_pof);

            // attach mesh to the POF
            OpenSim::Mesh* mesh = new OpenSim::Mesh{lum.location.string()};
            mesh_pof->attachGeometry(mesh);
        }

        // assign added_pf
        added_pf = body;
    } else if (bof.type == Body_or_frame::Type::Frame) {
        // user requested a Frame to be added

        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        SimTK::Vec3 translation = stk_vec3_from(bof.pos - world_parent_pos);
        pof->set_translation(translation);
        pof->setParentFrame(parent_pf);

        added_pf = pof.get();
        parent_pf.addComponent(pof.release());
    } else {
        throw std::runtime_error{"tried to create a body/frame from something of unknown type: this is a developer error"};
    }

    OSC_ASSERT(added_pf != nullptr);

    // ASSIGN MESHES
    //     - decompose mesh model matrix into scale, rotation, translation
    //     - figure out whether mesh needs intermediate POF for any of the above
    //     - (if it does) add POF with relevant translations etc. and attach that POF to the body/pof above
    //     - attach MESH to the intermediate POF, or the body/pof above
    for (Loaded_user_mesh const& lum : impl.meshes) {
        if (lum.assigned_body != bof_idx) {
            continue;
        }


    }

    // RECURSE (depth-first)
    for (size_t i = 0; i < impl.bodies.size(); ++i) {
        Body_or_frame const& b = impl.bodies[i];

        // if a body points to the current body/frame, recurse to it
        if (b.parent == bof_idx) {
            recursively_add_bof(impl, model, static_cast<int>(i), bof_idx, *added_pf);
        }
    }
}

// tries to create an OpenSim::Model from the current screen state
//
// will test to ensure the current screen state is actually valid, though
static void try_creating_output_model(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    test_for_advancement_issues(impl);
    if (!impl.advancement_issues.empty()) {
        log::error("cannot create an osim model: advancement issues detected");
        return;
    }

    std::unique_ptr<OpenSim::Model> m = std::make_unique<OpenSim::Model>();
    for (size_t i = 0; i < impl.bodies.size(); ++i) {
        Body_or_frame const& bof = impl.bodies[i];
        if (bof.parent == -1) {
            // it's a bof that is directly connected to Ground
            int bof_idx = static_cast<int>(i);
            int ground_idx = -1;
            recursively_add_bof(impl, *m, bof_idx, ground_idx, m->updGround());
        }
    }

    // all done: assign model
    impl.output_model = std::move(m);
}

// draw sidebar containing basic documentation and some action buttons
static void wizard_step2_draw_sidebar(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    // draw header text /w wizard explanation
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextUnformatted("Mesh Importer Wizard");
    ImGui::Separator();
    ImGui::TextWrapped("This is a specialized utlity for mapping existing mesh data into a new OpenSim model file. This wizard works best when you have a lot of mesh data from some other source and you just want to (roughly) map the mesh data onto a new OpenSim model. You can then tweak the generated model in the main OSC GUI, or an XML editor (advanced).");
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextWrapped("EXPERIMENTAL: currently under active development. Expect issues. This is shipped with OSC because, even with some bugs, it may save time in certain workflows.");
    ImGui::Dummy(ImVec2{0.0f, 5.0f});

    // draw step text /w step information
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextUnformatted("step 2: build an OpenSim model and assign meshes");
    ImGui::Separator();
    ImGui::Dummy(ImVec2{0.0f, 2.0f});
    ImGui::TextWrapped("An OpenSim `Model` is a tree of `Body`s (things with mass) and `Frame`s (things with a location) connected by `Joint`s (things with physical constraints) in a tree-like datastructure that has `Ground` as its root.\n\nIn this step, you will build the Model's tree structure by adding `Body`s and `Frame`s into the scene, followed by assigning your mesh data to them.");
    ImGui::Dummy(ImVec2{0.0f, 10.0f});

    ImGui::Text("num meshes = %zu", impl.meshes.size());
    ImGui::Text("num bodies/frames = %zu", impl.bodies.size());
    ImGui::Text("assignment_st.assigner = %i", impl.assignment_st.assigner);
    ImGui::Text("assignment_st.is_body = %s", impl.assignment_st.is_body ? "true" : "false");
    ImGui::Checkbox("show floor", &impl.show_floor);
    ImGui::Checkbox("show meshes", &impl.show_meshes);
    ImGui::Checkbox("show ground", &impl.show_ground);
    ImGui::Checkbox("show bofs", &impl.show_bofs);
    ImGui::Checkbox("lock meshes", &impl.lock_meshes);
    ImGui::Checkbox("lock ground", &impl.lock_ground);
    ImGui::Checkbox("lock bofs", &impl.lock_bofs);
    ImGui::Checkbox("show all connection lines", &impl.show_all_connection_lines);
    Rgba32_ColorEdit4("assigned mesh color", &impl.assigned_mesh_color);
    Rgba32_ColorEdit4("unassigned mesh color", &impl.unassigned_mesh_color);
    Rgba32_ColorEdit4("ground color", &impl.ground_color);
    Rgba32_ColorEdit4("body color", &impl.body_color);
    Rgba32_ColorEdit4("frame color", &impl.frame_color);

    if (ImGui::Button("add frame")) {
        action_add_frame(impl, {0.0f, 0.0f, 0.0f});
    }
    if (ImGui::Button("select all")) {
        set_is_selected_of_all_to(impl, true);
    }
    if (ImGui::Button("clear selection")) {
        set_is_hovered_of_all_to(impl, false);
    }

    test_for_advancement_issues(impl);

    if (impl.advancement_issues.empty()) {
        // next step
        if (ImGui::Button("next >>")) {
            try_creating_output_model(impl);
        }
    }

    if (!impl.advancement_issues.empty()) {
        ImGui::Text("issues (%zu):", impl.advancement_issues.size());
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        for (std::string const& s : impl.advancement_issues) {
            ImGui::TextUnformatted(s.c_str());
        }
    }
}

// SCREEN DRAW: draw wizard (step2) screen
static void wizard_step2_draw(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    ImGuizmo::BeginFrame();

    // draw sidebar in a (moveable + resizeable) ImGui panel
    if (ImGui::Begin("wizardstep2sidebar")) {
        wizard_step2_draw_sidebar(impl);
    }
    ImGui::End();

    // draw 3D viewer in a (moveable + resizeable) ImGui panel
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    if (ImGui::Begin("wizardsstep2viewer")) {
        wizard_step2_draw_3dviewer(impl);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}


// public API

osc::Meshes_to_model_wizard_screen_step2::Meshes_to_model_wizard_screen_step2(std::vector<Loaded_user_mesh> lums) :
    impl{new Impl{std::move(lums)}} {
}

osc::Meshes_to_model_wizard_screen_step2::~Meshes_to_model_wizard_screen_step2() noexcept = default;

void osc::Meshes_to_model_wizard_screen_step2::draw() {
    ::wizard_step2_draw(*impl);
}

void osc::Meshes_to_model_wizard_screen_step2::tick(float) {
    update_camera_from_user_input(*impl);
    update_impl_from_user_input(*impl);

    // if a model was produced by this step then transition into the editor
    if (impl->output_model) {
        Application::current().request_screen_transition<Model_editor_screen>(std::move(impl->output_model));
    }
}
