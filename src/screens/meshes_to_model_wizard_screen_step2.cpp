#include "meshes_to_model_wizard_screen_step2.hpp"

#include "src/3d/cameras.hpp"
#include "src/application.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"
#include "src/log.hpp"
#include "src/constants.hpp"

#include <OpenSim.h>
#include <imgui.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>

using namespace osc;

// a body/frame that the user has added into the model
//
// these are expressed in *absolute* (i.e. relative to ground) coordinates
// during this phase of model building so that users can intuitively move
// the bodies/frames around in the scene. Subsequent steps put these into
// OpenSim's relative coordinate system
struct Body_or_frame final {

    // -1 if parent is Ground; otherwise, the index of the parent
    int parent;

    // absolute position (i.e. relative to `Ground`)
    glm::vec3 pos;

    // is it a body or a frame?
    //
    // both are treated basically identically in this step of the wizard
    // but they do differ in the output OpenSim::Model
    enum class Type { Body, Frame };
    Type type;

    // true if it is selected in the UI
    bool is_selected;

    // true if it is hovered in the UI
    bool is_hovered;
};

// type of element in the scene
enum class ElType { None, Mesh, Body, Ground };

// screen state
struct osc::Meshes_to_model_wizard_screen_step2::Impl final {

    // decorative meshes loaded during step 1
    std::vector<Loaded_user_mesh> meshes;

    // user-assigned body locations, with tree-like topology
    //
    // these are what eventually get transformed into `OpenSim::Body`s
    // and `OpenSim::PhysicalFrame`s in the output model
    std::vector<Body_or_frame> bodies;

    // 3D stuff
    glm::vec2 render_topleft_in_screen = {0.0f, 0.0f};
    Render_params renderparams;
    Drawlist drawlist;
    Render_target rendertarg;
    Polar_perspective_camera camera;

    // true if the implementation detects that the mouse is over the
    // render
    bool mouse_over_render = false;

    // true if mouse is over a manipulation gizmo
    bool mouse_over_gizmo = false;

    // true if a chequered floor should be drawn
    bool show_floor = true;

    // true if ground (0,0,0) is hovered
    bool ground_hovered = false;

    // holds state between frames for the scene's context menu
    //
    // this is necessary because the user can arbitrarily move their mouse
    // around while a context menu is opened, so the hovertest will keep
    // changing during that time
    struct Context_menu final {
        int idx = -1;
        ElType type;
    } ctx_menu;

    Impl(std::vector<Loaded_user_mesh> lums) :
        meshes{std::move(lums)} {
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

// returns mesh instance representing a chequered floor in the scene
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

// update the scene's camera based on (ImGui's) user input
static void update_camera_from_user_input(Meshes_to_model_wizard_screen_step2::Impl& impl) {
    if (!impl.mouse_over_render) {
        return;
    }

    // handle scroll zooming
    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/5.0f;

    // handle panning/zooming/dragging with middle mouse
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImVec2 screendims = impl.rendertarg.dimensions();
        float aspect_ratio = screendims.x / screendims.y;
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

        // relative vectors
        float rdx = delta.x/screendims.x;
        float rdy = delta.y/screendims.y;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse: pan
            pan(impl.camera, aspect_ratio, {rdx, rdy});
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse: zoom
            impl.camera.radius *= 1.0f + rdy;
        } else {
            // middle: mouse drag
            drag(impl.camera, {rdx, rdy});
        }
    }
}

// draw 2D overlay (dotted lines between bodies, etc.)
static void draw_2d_overlay(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    /*
    ImU32 black = ImGui::ColorConvertFloat4ToU32(ImVec4{1.0f, 0.0f, 0.0f, 1.0f});

    OpenSim::Model const& m = impl.model;
    OpenSim::Ground const& g = m.getGround();
    SimTK::Transform t = g.getTransformInGround(impl.st);

    // model --> world
    glm::mat4 model = to_mat4(t);

    // world --> view
    glm::mat4 view = view_matrix(impl.camera);

    // view --> clip
    glm::mat4 proj = projection_matrix(impl.camera, impl.rendertarg.aspect_ratio());

    glm::vec4 modelpos = {0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 worldpos = model * modelpos;


    // NDC pos (pre-perspective divide)
    glm::vec4 ndcpos = proj * view * worldpos;

    // perspective divide
    ndcpos.x /= ndcpos.w;
    ndcpos.y /= ndcpos.w;
    ndcpos.z /= ndcpos.w;
    ndcpos.w = 1.0f;

    // NDC (centered, -1.0f to +1.0f) --> screen coords (top-left)
    glm::vec2 screenpos{
        ((ndcpos.x + 1.0f)/2.0f)*impl.rendertarg.w,
        ((ndcpos.y - 1.0f)/2.0f)*impl.rendertarg.h * -1.0f,
    };

    ImGui::SetCursorPos(ImVec2{0.0f, 0.0f});
    glm::vec2 base_scrpos = ImGui::GetCursorScreenPos();
    ImDrawList& l = *ImGui::GetForegroundDrawList();

    l.AddCircleFilled(base_scrpos + screenpos, 2.0f, black);
    char buf[512];
    std::snprintf(buf, sizeof(buf), "Ground (always %.0f, %.0f, %.0f)", worldpos.x, worldpos.y, worldpos.z);
    l.AddText(base_scrpos + screenpos, black, buf);

    for (Loaded_user_mesh const& m : impl.meshes) {
        glm::vec3 v3 = (m.aabb.p1 + m.aabb.p2) / 2.0f;
        glm::vec4 v4{v3.x, v3.y, v3.z, 1.0f};
        glm::vec4 ndc = proj * view * m.model_mtx * v4;
        ndc.x /= ndc.w;
        ndc.y /= ndc.w;
        ndc.z /= ndc.w;
        ndc.w = 1.0f;
        glm::vec2 sp{
            ((ndc.x + 1.0f)/2.0f)*impl.rendertarg.w,
            ((ndc.y - 1.0f)/2.0f)*impl.rendertarg.h * -1.0f,
        };
        glm::vec2 panelpos = base_scrpos + sp;

        l.AddCircleFilled(panelpos, 2.0f, black);
    }
    */
}

// draw hover tooltip when hovering over a user mesh
static void draw_mesh_hover_tooltip(osc::Meshes_to_model_wizard_screen_step2::Impl&, Loaded_user_mesh&) {
    ImGui::BeginTooltip();
    ImGui::Text("mesh");
    ImGui::EndTooltip();
}

// draw hover tooltip when hovering over Ground
static void draw_ground_hover_tooltip(osc::Meshes_to_model_wizard_screen_step2::Impl&) {
    ImGui::BeginTooltip();
    ImGui::Text("ground");
    ImGui::EndTooltip();
}

// draw hover tooltip when hovering over a BOF
static void draw_bof_hover_tooltip(osc::Meshes_to_model_wizard_screen_step2::Impl&, Body_or_frame&) {
    ImGui::BeginTooltip();
    ImGui::Text("bof");
    ImGui::EndTooltip();
}

// draw mesh context menu
static void draw_mesh_context_menu_content(osc::Meshes_to_model_wizard_screen_step2::Impl& impl, Loaded_user_mesh& lum) {
    if (ImGui::MenuItem("add body here")) {
        Body_or_frame& bf = impl.bodies.emplace_back();
        bf.parent = -1;
        bf.pos = center(lum);
        bf.type = Body_or_frame::Type::Frame;
        bf.is_selected = false;
        bf.is_hovered = false;
    }
}

// draw ground context menu
static void draw_ground_context_menu_content(osc::Meshes_to_model_wizard_screen_step2::Impl&) {

}

// draw bof context menu
static void draw_bof_context_menu_content(osc::Meshes_to_model_wizard_screen_step2::Impl&, Body_or_frame&) {

}

// draw manipulation gizmos (the little handles that move things in 3D)
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

    if (!manipulated) {
        return;
    }

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

    // used to detect whether the mouse is "busy" with a gizmo
    impl.mouse_over_gizmo = ImGuizmo::IsOver();
}

// draw main 3D scene viewer
static void wizard_step2_draw_3dviewer(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    ImVec2 dims = ImGui::GetContentRegionAvail();

    // skip rendering steps if ImGui panel is too small
    if (dims.x < 1.0f || dims.y < 1.0f) {
        return;
    }

    // ensure render target dimensions match panel dimensions
    impl.rendertarg.reconfigure(
        static_cast<int>(dims.x), static_cast<int>(dims.y), Application::current().samples());

    // compute render position on the screen (needed by ImGuizmo)
    {
        glm::vec2 wp = ImGui::GetWindowPos();
        glm::vec2 cp = ImGui::GetCursorPos();
        impl.render_topleft_in_screen = wp + cp;
    }

    // populate 3D drawlist
    Drawlist& dl = impl.drawlist;
    dl.clear();

    // unique ID for each mesh instance in the scene, used for hit testing
    uint16_t id = 1;

    // ID is an accumulated 1-based index into
    //
    // - meshes [1, nmeshes]
    // - ground (nmeshes, nmeshes+1]
    // - bodies/frames (nmeshes+1, nmeshes+1+nbodies]

    // add meshes to 3D scene
    for (Loaded_user_mesh const& um : impl.meshes) {
        Mesh_instance mi;
        mi.model_xform = um.model_mtx;
        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = Rgba32::from_vec4({1.0f, 1.0f, 1.0f, 1.0f});
        mi.meshidx = um.gpu_meshidx;
        mi.passthrough.rim_alpha = um.is_selected ? 0xff : um.is_hovered ? 0x60 : 0x00;
        mi.passthrough.assign_u16(id++);
        dl.push_back(mi);
    }

    // sphere data for drawing bodies/frames in 3D
    Meshidx sphereidx = Application::current().get_gpu_storage().simbody_sphere_idx;
    glm::mat4 sphere_scaler = glm::scale(glm::mat4{1.0f}, {0.01f, 0.01f, 0.01f});

    // add ground (defined to be at 0, 0, 0) to 3D scene
    {
        Mesh_instance mi;
        mi.model_xform = glm::scale(glm::mat4{1.0f}, {0.008f, 0.008f, 0.008f});
        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = Rgba32::from_vec4({0.0f, 0.0f, 1.0f, 1.0f});
        mi.meshidx = sphereidx;
        mi.passthrough.rim_alpha = impl.ground_hovered ? 0x60 : 0x00;
        mi.passthrough.assign_u16(id++);
        dl.push_back(mi);
    }

    // add bodies/frames to 3D scene
    for (Body_or_frame const& bf : impl.bodies) {
        Mesh_instance mi;
        mi.model_xform = glm::translate(glm::mat4{1.0f}, bf.pos) * sphere_scaler;
        mi.normal_xform = normal_matrix(mi.model_xform);
        if (bf.type == Body_or_frame::Type::Body) {
            mi.rgba = Rgba32::from_vec4({1.0f, 0.0f, 0.0f, 1.0f});
        } else {
            mi.rgba = Rgba32::from_vec4({0.0f, 1.0f, 0.0f, 1.0f});
        }
        mi.meshidx = sphereidx;
        mi.passthrough.rim_alpha = bf.is_selected ? 0xff : bf.is_hovered ? 0x60 : 0x00;
        mi.passthrough.assign_u16(id++);
        dl.push_back(mi);
    }

    // (if applicable) add chequered floor to 3D scene
    if (impl.show_floor) {
        dl.push_back(create_chequered_floor_meshinstance());
    }

    // make renderer hittest location match mouse location
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

    draw_selection_manipulation_gizmos(impl);

    // handle mouse hovers
    //
    // this code is relatively ugly because the scene supports a range of
    // hovering behavior
    if (impl.mouse_over_render) {

        // reset all previous hover state
        set_is_hovered_of_all_to(impl, false);

        // set hover based on latest hittest result
        uint16_t hovered_id = impl.rendertarg.hittest_result.get_u16();

        if (hovered_id == 0) {
            // hovered over nothing
        } else if (hovered_id <= impl.meshes.size()) {
            // hovered over mesh

            int idx = static_cast<int>(hovered_id - 1);

            Loaded_user_mesh& lum = impl.meshes[static_cast<size_t>(idx)];

            // set is_hovered
            lum.is_hovered = true;

            // draw hover tooltip
            draw_mesh_hover_tooltip(impl, lum);

            // open context menu (if applicable)
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.ctx_menu.idx = idx;
                impl.ctx_menu.type = ElType::Mesh;
                ImGui::OpenPopup("contextmenu");
            }

            // if left-clicked, select it
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // de-select everything
                set_is_selected_of_all_to(impl, false);

                // set clicked item as selected
                lum.is_selected = true;
            }
        } else if (hovered_id <= impl.meshes.size()+1) {
            // hovered over ground

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
        } else if (hovered_id <= impl.meshes.size()+1+impl.bodies.size()) {
            // hovered over body

            int idx = static_cast<int>(((hovered_id - impl.meshes.size()) - 1) - 1);

            Body_or_frame& bof = impl.bodies[static_cast<size_t>(idx)];

            // set is_hovered
            bof.is_hovered = true;

            // draw hover tooltip
            draw_bof_hover_tooltip(impl, bof);

            // open context menu (if applicable)
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.ctx_menu.idx = idx;
                impl.ctx_menu.type = ElType::Body;
                ImGui::OpenPopup("contextmenu");
            }


            // if left-clicked, select it
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // de-select everything
                set_is_selected_of_all_to(impl, false);

                // set clicked item as selected
                bof.is_selected = true;
            }
        }
    }

    // draw context menu (if necessary)
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

    // draw 2D overlays
    draw_2d_overlay(impl);
}

// draw sidebar
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

    // draw action buttons
    if (ImGui::Button("add frame")) {
        Body_or_frame& bf = impl.bodies.emplace_back();
        bf.parent = -1;
        bf.pos = {0.0f, 0.0f, 0.0f};
        bf.type = Body_or_frame::Type::Frame;
        bf.is_selected = false;
        bf.is_hovered = false;
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
}
