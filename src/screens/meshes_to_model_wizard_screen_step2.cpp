#include "meshes_to_model_wizard_screen_step2.hpp"

#include "src/3d/cameras.hpp"
#include "src/application.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"
#include "src/log.hpp"

#include <OpenSim.h>
#include <imgui.h>
#include <glm/gtx/transform.hpp>

using namespace osc;

struct osc::Meshes_to_model_wizard_screen_step2::Impl final {
    // mesh data loaded during step 1 of the wizard
    std::vector<Loaded_user_mesh> meshes;

    // the OpenSim model that the user is building against the meshes
    OpenSim::Model model;
    SimTK::State st;

    // 3D stuff
    osc::Render_params renderparams;
    osc::Drawlist drawlist;
    osc::Render_target rendertarg;
    osc::Polar_perspective_camera camera;

    bool mouse_over_render = true;

    Impl(std::vector<Loaded_user_mesh> lums) :
        meshes{std::move(lums)},
        model{},
        st{[this]() {
            model.finalizeFromProperties();
            SimTK::State rv = model.initSystem();
            return rv;
        }()} {
        camera.radius = 10.0f;
    }
};

// update the camera
static void update_camera_from_user_input(Meshes_to_model_wizard_screen_step2::Impl& impl) {
    if (!impl.mouse_over_render) {
        return;
    }

    // handle scroll zooming
    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/5.0f;

    if (impl.mouse_over_render && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
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

static void draw_2d_overlay(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
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
    std::snprintf(buf, sizeof(buf), "ground @ (%.2f, %.2f, %.2f)", worldpos.x, worldpos.y, worldpos.z);
    l.AddText(base_scrpos + screenpos, black, buf);
}

static void wizard_step2_draw_3dviewer(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    ImVec2 dims = ImGui::GetContentRegionAvail();

    if (dims.x < 1.0f || dims.y < 1.0f) {
        return;
    }

    // ensure render target dimensions match panel dimensions
    impl.rendertarg.reconfigure(
        static_cast<int>(dims.x), static_cast<int>(dims.y), Application::current().samples());

    // populate 3D drawlist with the scene meshes etc.
    Drawlist& dl = impl.drawlist;
    dl.clear();
    for (Loaded_user_mesh const& um : impl.meshes) {
        Mesh_instance mi;
        mi.model_xform = glm::mat4{1.0f};
        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = Rgba32::from_vec4({1.0f, 1.0f, 1.0f, 1.0f});
        mi.meshidx = um.gpu_meshidx;
        mi.passthrough.rim_alpha = um.is_selected ? 0xff : um.is_hovered ? 0x60 : 0x00;
        mi.passthrough.assign_u16(0);
        dl.push_back(mi);
    }

    // update 3D render params
    impl.renderparams.view_matrix = view_matrix(impl.camera);
    impl.renderparams.projection_matrix = projection_matrix(impl.camera, impl.rendertarg.aspect_ratio());

    // do 3D drawcall
    GPU_storage& gpu =  Application::current().get_gpu_storage();
    draw_scene(gpu, impl.renderparams, impl.drawlist, impl.rendertarg);

    // blit rendered (2D) output render to an ImGui::Image
    gl::Texture_2d& tex = impl.rendertarg.main();
    void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex.get()));
    ImVec2 image_dimensions{dims.x, dims.y};
    ImVec2 uv0{0.0f, 1.0f};
    ImVec2 uv1{1.0f, 0.0f};
    ImGui::Image(texture_handle, image_dimensions, uv0, uv1);

    // draw 2D overlays
    draw_2d_overlay(impl);
}

static void wizard_step2_draw_sidebar(osc::Meshes_to_model_wizard_screen_step2::Impl&) {
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextUnformatted("Mesh Importer Wizard");
    ImGui::Separator();
    ImGui::TextWrapped("This is a specialized utlity for mapping existing mesh data into a new OpenSim model file. This wizard works best when you have a lot of mesh data from some other source and you just want to (roughly) map the mesh data onto a new OpenSim model. You can then tweak the generated model in the main OSC GUI, or an XML editor (advanced).");
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextWrapped("EXPERIMENTAL: currently under active development: expect issues. This is shipped with OSC because, even with some bugs, it may save time in certain workflows.");
    ImGui::Dummy(ImVec2{0.0f, 5.0f});

    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextUnformatted("step 2: assign OpenSim bodys/joints to the mesh");
    ImGui::Separator();
    ImGui::Dummy(ImVec2{0.0f, 2.0f});
    ImGui::TextWrapped("OpenSim simulates bodies attached to eachover via joints, rather than pure meshes. In this step, you need to create a body tree, where every body is joined to X etc.");
    ImGui::Dummy(ImVec2{0.0f, 10.0f});
}

static void wizard_step2_draw(osc::Meshes_to_model_wizard_screen_step2::Impl& impl) {
    if (ImGui::Begin("Sidebar")) {
        wizard_step2_draw_sidebar(impl);
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    if (ImGui::Begin("another3dviewer")) {
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
