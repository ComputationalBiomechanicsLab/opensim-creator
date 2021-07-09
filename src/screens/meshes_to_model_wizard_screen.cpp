#include "meshes_to_model_wizard_screen.hpp"

#include "src/log.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/utils/shims.hpp"
#include "src/assertions.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/cameras.hpp"
#include "src/application.hpp"
#include "src/screens/meshes_to_model_wizard_screen_step2.hpp"
#include "src/utils/algs.hpp"

#include <imgui.h>
#include <nfd.h>
#include "third_party/IconsFontAwesome5.h"
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>

#include <filesystem>
#include <condition_variable>
#include <mutex>
#include <string>
#include <memory>
#include <variant>

using namespace osc;

// a request (usually made by UI thread) to load some mesh data
struct Mesh_load_request final {
    // unique mesh ID
    int id;

    // filesystem path of the mesh
    std::filesystem::path filepath;
};

// an OK response to a mesh loading request
struct Mesh_load_OK_response final {
    // unique mesh ID
    int id;

    // filesystem path of the mesh
    std::filesystem::path filepath;

    // CPU-side mesh data
    Untextured_mesh um;

    // AABB of the mesh data
    AABB aabb;

    // bounding sphere of the mesh data
    Sphere bounding_sphere;
};

// an ERROR response to a mesh loading request
struct Mesh_load_ERORR_response final {
    // unique mesh ID
    int id;

    // filesystem path of the mesh (that errored while loading)
    std::filesystem::path filepath;

    // the error
    std::string error;
};

// the response from a mesh loading request
using Mesh_load_response = std::variant<Mesh_load_OK_response, Mesh_load_ERORR_response>;

// MESH LOADER THREAD: respond to load request (and handle errors)
static Mesh_load_response create_response(Mesh_load_request const& msg) {
    try {
        Mesh_load_OK_response rv;
        rv.id = msg.id;
        rv.filepath = msg.filepath;
        load_mesh_file_with_simtk_backend(msg.filepath, rv.um);  // can throw
        rv.aabb = aabb_from_mesh(rv.um);
        rv.bounding_sphere = bounding_sphere_from_mesh(rv.um);
        return rv;
    } catch (std::exception const& ex) {
        return Mesh_load_ERORR_response{msg.id, msg.filepath, ex.what()};
    }
}

// MESH LOADER THREAD: implementation code
static int mesh_loader_thread_main(osc::stop_token, spsc::Receiver<Mesh_load_request> rx, spsc::Sender<Mesh_load_response> tx) {
    while (!tx.is_receiver_hung_up()) {
        std::optional<Mesh_load_request> msg = rx.recv();

        if (!msg) {
            return 0;  // sender hung up
        }

        tx.send(create_response(*msg));
    }
    return 0;
}

// state associated with the mesh loader
struct Mesh_loader final {
    // worker (background thread)
    osc::jthread worker;

    // request transmitter (UI thread)
    spsc::Sender<Mesh_load_request> tx;

    // response receiver (UI thread)
    spsc::Receiver<Mesh_load_response> rx;

    // create, and immediately start, the mesh loader
    static Mesh_loader create() {
        auto [req_tx, req_rx] = spsc::channel<Mesh_load_request>();
        auto [resp_tx, resp_rx] = spsc::channel<Mesh_load_response>();
        osc::jthread worker{mesh_loader_thread_main, std::move(req_rx), std::move(resp_tx)};
        return Mesh_loader{std::move(worker), std::move(req_tx), std::move(resp_rx)};
    }

    // send a request to the mesh loader
    void send(Mesh_load_request req) {
        tx.send(std::move(req));
    }

    // poll for any new responses from the mesh loader
    std::optional<Mesh_load_response> poll() {
        return rx.try_recv();
    }
};

// a fully-loaded mesh
//
// this differs from the `Mesh_load_OK_response` in that it contains GPU data
// that *must* be loaded in the main thread (OpenGL limitation)
struct Loaded_mesh final {
    // unique mesh ID
    int id;

    // filesystem path of the mesh
    std::filesystem::path filepath;

    // CPU-side mesh data
    Untextured_mesh um;

    // AABB of the mesh data
    AABB aabb;

    // bounding sphere of the mesh data
    Sphere bounding_sphere;

    // model matrix
    glm::mat4 model_mtx;

    // index of mesh, loaded into GPU_storage
    Meshidx idx;

    // true if the mesh is hovered by the user's mouse
    bool is_hovered;

    // true if the mesh is selected
    bool is_selected;

    // create this by stealing from an OK background response
    explicit Loaded_mesh(Mesh_load_OK_response&& tmp) :
        id{tmp.id},
        filepath{std::move(tmp.filepath)},
        um{std::move(tmp.um)},
        aabb{tmp.aabb},
        bounding_sphere{tmp.bounding_sphere},
        model_mtx{1.0f},
        idx{[this]() {
            GPU_storage& gs = Application::current().get_gpu_storage();
            Meshidx rv = Meshidx::from_index(gs.meshes.size());
            gs.meshes.emplace_back(um);
            return rv;
        }()},
        is_hovered{false},
        is_selected{false} {
    }
};

// top-level state associated with the whole screen
struct osc::Meshes_to_model_wizard_screen::Impl final {
    // loader that loads mesh data in a background thread
    Mesh_loader mesh_loader = Mesh_loader::create();

    // loaded meshes
    std::vector<Loaded_mesh> meshes;

    // meshes queued up to be loaded by the background worker
    std::vector<Mesh_load_request> loading_meshes;

    // latest mesh ID - should be incremented each time a new
    // mesh is loaded so that all meshes get a unique ID
    int latest_id = 1;

    // 3D rendering params
    osc::Render_params renderparams;

    // 3D drawlist that is rendered
    osc::Drawlist drawlist;

    // 3D output render target from renderer
    osc::Render_target render_target;

    // 3D scene camera
    osc::Polar_perspective_camera camera;

    // true if the implementation thinks the user's mouse is over the render
    bool mouse_over_render = true;

    // true if the implementation thinks the user's mouse is over a gizmo
    bool mouse_over_gizmo = false;

    // true if renderer should draw AABBs
    bool draw_aabbs = false;

    // true if renderer should draw bounding spheres
    bool draw_bounding_spheres = false;
};

// synchronously prompt the user to select multiple mesh files through
// a native OS file dialog
static void prompt_user_to_select_multiple_mesh_files(Meshes_to_model_wizard_screen::Impl& impl) {
    nfdpathset_t s{};
    nfdresult_t result = NFD_OpenDialogMultiple("obj,vtp,stl", nullptr, &s);

    if (result == NFD_OKAY) {
        OSC_SCOPE_GUARD({ NFD_PathSet_Free(&s); });

        size_t len = NFD_PathSet_GetCount(&s);

        for (size_t i = 0; i < len; ++i) {
            int id = impl.latest_id++;
            std::filesystem::path fspath{NFD_PathSet_GetPath(&s, i)};

            Mesh_load_request req{id, fspath};
            impl.loading_meshes.push_back(req);
            impl.mesh_loader.send(req);
        }
    } else if (result == NFD_CANCEL) {
        // do nothing: the user cancelled
    } else {
        log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
    }
}

// DRAW popover tooltip that shows a mesh's details
static void draw_tooltip(Loaded_mesh const& um) {
    ImGui::BeginTooltip();
    ImGui::Text("id = %i", um.id);
    ImGui::Text("filename = %s", um.filepath.filename().string().c_str());
    ImGui::Text("is_hovered = %s", um.is_hovered ? "true" : "false");
    ImGui::Text("is_selected = %s", um.is_selected ? "true" : "false");
    ImGui::Text("verts = %zu", um.um.verts.size());
    ImGui::Text("elements = %zu", um.um.indices.size());

    ImGui::Text("AABB.p1 = (%.2f, %.2f, %.2f)", um.aabb.p1.x, um.aabb.p1.y, um.aabb.p1.z);
    ImGui::Text("AABB.p2 = (%.2f, %.2f, %.2f)", um.aabb.p2.x, um.aabb.p2.y, um.aabb.p2.z);
    glm::vec3 center = (um.aabb.p1 + um.aabb.p2) / 2.0f;
    ImGui::Text("center(AABB) = (%.2f, %.2f, %.2f)", center.x, center.y, center.z);

    ImGui::Text("sphere = O(%.2f, %.2f, %.2f), r(%.2f)", um.bounding_sphere.origin.x, um.bounding_sphere.origin.y, um.bounding_sphere.origin.z, um.bounding_sphere.radius);
    ImGui::EndTooltip();
}

// create a Loaded_user_mesh by stealing data from a User_mesh
[[nodiscard]] static Loaded_user_mesh pilfer_loaded_mesh_from(Loaded_mesh&& um) {
    Loaded_user_mesh rv;
    rv.location = std::move(um.filepath);
    rv.meshdata = std::move(um.um);
    rv.aabb = std::move(um.aabb);
    rv.bounding_sphere = std::move(um.bounding_sphere);
    rv.gpu_meshidx = std::move(um.idx);
    rv.model_mtx = um.model_mtx;
    rv.is_hovered = um.is_hovered;
    rv.is_selected = um.is_selected;
    rv.assigned_body = -1;
    return rv;
}

// DRAW meshlist
//
// returns nonempty vector if the user clicks "Next step" in the UI
enum class Meshlist_panel_response { Ok, ShouldTransition };
static Meshlist_panel_response draw_meshlist_panel_content(Meshes_to_model_wizard_screen::Impl& impl) {

    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextUnformatted("Mesh Importer Wizard");
    ImGui::Separator();
    ImGui::TextWrapped("This is a specialized utlity for mapping existing mesh data into a new OpenSim model file. This wizard works best when you have a lot of mesh data from some other source and you just want to (roughly) map the mesh data onto a new OpenSim model. You can then tweak the generated model in the main OSC GUI, or an XML editor (advanced).");
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextWrapped("EXPERIMENTAL: currently under active development: expect issues. This is shipped with OSC because, even with some bugs, it may save time in certain workflows.");
    ImGui::Dummy(ImVec2{0.0f, 5.0f});

    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextUnformatted("step 1: Import raw mesh data");
    ImGui::Separator();
    ImGui::Dummy(ImVec2{0.0f, 2.0f});
    ImGui::TextWrapped("Import the mesh data that you want to map into an OpenSim model. You can make minor adjustments here, but the next screen (body assignment) has additional options");
    ImGui::Dummy(ImVec2{0.0f, 10.0f});

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.6f, 0.0f, 1.0f});
    if (ImGui::Button(ICON_FA_PLUS "Import Meshes")) {
        prompt_user_to_select_multiple_mesh_files(impl);
    }
    ImGui::PopStyleColor();

    if (ImGui::Button(ICON_FA_ARROW_RIGHT "Next step (body assignment)")) {
        // caller should handle the transition
        return Meshlist_panel_response::ShouldTransition;
    }

    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::Text("meshes (%zu):", impl.meshes.size());
    ImGui::Separator();

    if (impl.meshes.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.6f, 0.6f, 0.6f, 1.0f});
        ImGui::TextUnformatted("  (no meshes added yet)");
        ImGui::PopStyleColor();
    }

    for (int i = 0; i < static_cast<int>(impl.meshes.size()); ++i) {
        Loaded_mesh& m = impl.meshes[static_cast<size_t>(i)];


        // perform deletion, if the user requests it
        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.6f, 0.0f, 0.0f, 1.0f});
        if (ImGui::Button("X")) {
            impl.meshes.erase(impl.meshes.begin() + i);
            --i;
            ImGui::PopStyleColor();
            continue;
        }
        ImGui::PopStyleColor();
        ImGui::PopID();

        // show mesh filename
        std::string filename = m.filepath.filename().string();
        ImGui::SameLine();
        ImGui::Text("%s", filename.c_str());

        // show tooltip if user hovers filename
        m.is_hovered = ImGui::IsItemHovered();
        if (m.is_hovered) {
            draw_tooltip(m);
        }

        // select mesh if user clicks filename
        if (ImGui::IsItemClicked()) {
            bool lshift_down = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT);
            bool rshift_down = ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);

            if (!(lshift_down || rshift_down)) {
                for (Loaded_mesh& lm : impl.meshes) {
                    lm.is_selected = false;
                }
            }

            m.is_selected = true;
        }
    }

    return Meshlist_panel_response::Ok;
}

// populate the screen's drawlist with the meshes
static void populate_3d_drawlist(Meshes_to_model_wizard_screen::Impl& impl) {
    GPU_storage const& storage = Application::current().get_gpu_storage();

    impl.drawlist.clear();
    for (auto const& mesh : impl.meshes) {

        // draw the geometry
        {
            Mesh_instance mi;
            mi.model_xform = mesh.model_mtx;
            mi.normal_xform = normal_matrix(mi.model_xform);
            mi.rgba = Rgba32::from_vec4({1.0f, 1.0f, 1.0f, 1.0f});
            mi.meshidx = mesh.idx;
            mi.passthrough.rim_alpha = mesh.is_selected ? 0xff : mesh.is_hovered ? 0x60 : 0x00;
            mi.passthrough.assign_u16(static_cast<uint16_t>(mesh.id));
            impl.drawlist.push_back(mi);
        }

        // also, draw the aabb
        if (impl.draw_aabbs) {
            Mesh_instance mi2;
            AABB const& aabb = mesh.aabb;
            glm::vec3 o = (aabb.p1 + aabb.p2)/2.0f;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, o - aabb.p1);
            glm::mat4 translate = glm::translate(glm::mat4{1.0f}, o);
            mi2.model_xform = mesh.model_mtx * translate * scaler;
            mi2.normal_xform = normal_matrix(mi2.model_xform);
            mi2.rgba = Rgba32::from_vec4({1.0f, 0.0f, 0.0f, 1.0f});
            mi2.meshidx = storage.cube_lines_idx;
            mi2.flags.set_draw_lines();
            impl.drawlist.push_back(mi2);
        }

        // also, draw the bounding sphere
        if (impl.draw_bounding_spheres) {
            Mesh_instance mi2;
            Sphere const& sph = mesh.bounding_sphere;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {sph.radius, sph.radius, sph.radius});
            glm::mat4 translate = glm::translate(glm::mat4{1.0f}, sph.origin);
            mi2.model_xform = mesh.model_mtx * translate * scaler;
            mi2.normal_xform = normal_matrix(mi2.model_xform);
            mi2.rgba = Rgba32::from_vec4({0.0f, 1.0f, 0.0f, 0.3f});
            mi2.meshidx = storage.simbody_sphere_idx;
            mi2.flags.set_draw_lines();
            impl.drawlist.push_back(mi2);
        }
    }
}

// DRAW 3d viewer
static void draw_3d_viewer_panel_content(Meshes_to_model_wizard_screen::Impl& impl) {
    populate_3d_drawlist(impl);

    ImGui::Checkbox("draw aabbs", &impl.draw_aabbs);
    ImGui::SameLine();
    ImGui::Checkbox("draw bounding spheres", &impl.draw_bounding_spheres);
    ImGui::SameLine();
    ImGui::CheckboxFlags("wireframe mode", &impl.renderparams.flags, DrawcallFlags_WireframeMode);
    ImGui::SameLine();
    ImGui::Text("FPS: %.0f", static_cast<double>(ImGui::GetIO().Framerate));

    // render will fill remaining content region
    ImVec2 dims = ImGui::GetContentRegionAvail();

    if (dims.x < 1.0f || dims.y < 1.0f) {
        // edge case: no space left (e.g. user minimized it *a lot*)
        return;
    }

    // reconfigure render buffers to match current panel size (this can be
    // resized by the user at any time)
    impl.render_target.reconfigure(
        static_cast<int>(dims.x), static_cast<int>(dims.y), Application::current().samples());

    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 cp = ImGui::GetCursorPos();
    ImVec2 imgstart{wp.x + cp.x, wp.y + cp.y};

    // update hovertest location
    {
        ImVec2 mp = ImGui::GetMousePos();
        impl.renderparams.hittest.x = static_cast<int>((mp.x - wp.x) - cp.x);
        impl.renderparams.hittest.y = static_cast<int>(dims.y - ((mp.y - wp.y) - cp.y));
    }

    // set params to use latest camera state
    impl.renderparams.view_matrix = view_matrix(impl.camera);
    impl.renderparams.projection_matrix = projection_matrix(impl.camera, impl.render_target.aspect_ratio());

    // do drawcall
    auto& app = Application::current();
    GPU_storage& gpu =  app.get_gpu_storage();
    draw_scene(gpu, impl.renderparams, impl.drawlist, impl.render_target);

    // update UI state from hittest, if there was a hit
    int hovered_meshid = static_cast<int>(impl.render_target.hittest_result.get_u16());

    std::vector<Loaded_mesh>& meshes = impl.meshes;

    decltype(meshes.end()) it;
    if (hovered_meshid == 0) {
        it = meshes.end();
    } else {
        it = std::find_if(meshes.begin(), meshes.end(), [hovered_meshid](Loaded_mesh const& um) {
                return um.id == hovered_meshid;
            });
    }

    // handle mouse
    if (impl.mouse_over_gizmo) {
        // mouse is being handled by ImGuizmo

    } else if (it != meshes.end()) {
        // mouse is over a piece of 3D geometry in the scene

        it->is_hovered = true;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            bool lshift_down = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT);
            bool rshift_down = ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            if (!(lshift_down || rshift_down)) {
                for (Loaded_mesh& um : meshes) {
                    um.is_selected = false;
                }
            }

            it->is_selected = true;
        }
    } else {
        // mouse is over nothing

        if (impl.mouse_over_render && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            bool lshift_down = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT);
            bool rshift_down = ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            if (!(lshift_down || rshift_down)) {
                for (Loaded_mesh& um : meshes) {
                    um.is_selected = false;
                }
            }
        }
    }

    // blit output texture to an ImGui::Image
    gl::Texture_2d& tex = impl.render_target.main();
    void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex.get()));
    ImVec2 image_dimensions{dims.x, dims.y};
    ImVec2 uv0{0.0f, 1.0f};
    ImVec2 uv1{1.0f, 0.0f};
    ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
    impl.mouse_over_render = ImGui::IsItemHovered();

    // draw hover-over tooltips
    if (it != meshes.end()) {
        draw_tooltip(*it);
    }

    // draw manipulation gizmos
    {
        int nselected = 0;
        glm::vec3 avg_center = {0.0f, 0.0f, 0.0f};
        for (Loaded_mesh const& um : meshes) {
            if (!um.is_selected) {
                continue;
            }

            glm::vec3 raw_center = aabb_center(um.aabb);
            glm::vec3 center = glm::vec3{um.model_mtx * glm::vec4{raw_center, 1.0f}};

            avg_center += center;
            ++nselected;
        }
        avg_center /= static_cast<float>(nselected);

        if (nselected > 0) {
            glm::mat4 translator = glm::translate(glm::mat4{1.0f}, avg_center);
            glm::mat4 manipulated_mtx = translator;

            ImGuizmo::SetRect(imgstart.x, imgstart.y, static_cast<float>(impl.render_target.w), static_cast<float>(impl.render_target.h));
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

            if (manipulated) {
                glm::mat4 inv_translator = glm::translate(glm::mat4{1.0f}, -avg_center);
                glm::mat4 raw_xform = inv_translator * manipulated_mtx;
                glm::mat4 applied_xform = translator * raw_xform * inv_translator;

                for (Loaded_mesh& um : meshes) {
                    if (!um.is_selected) {
                        continue;
                    }

                    um.model_mtx = applied_xform * um.model_mtx;
                }
            }
        }

        impl.mouse_over_gizmo = ImGuizmo::IsOver();
    }
}

[[nodiscard]] static std::vector<Loaded_user_mesh> pilfer_meshes_from(std::vector<Loaded_mesh>&& ms) {
    std::vector<Loaded_user_mesh> rv;
    rv.reserve(ms.size());
    for (Loaded_mesh& m : ms) {
        rv.push_back(pilfer_loaded_mesh_from(std::move(m)));
    }
    return rv;
}

// SCREEN DRAW
static void draw_meshes_to_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl) {

    // ImGuizmo: requires this be called each frame to enable 3D manipulation handlers
    ImGuizmo::BeginFrame();

    // draw the mesh list (a thin panel containing buttons, etc.)
    if (ImGui::Begin("Mesh list")) {
        if (draw_meshlist_panel_content(impl) == Meshlist_panel_response::ShouldTransition) {
            ImGui::End();
            Application::current().request_screen_transition<Meshes_to_model_wizard_screen_step2>(pilfer_meshes_from(std::move(impl.meshes)));
            return;
        }
    }
    ImGui::End();

    // draw 3D viewer
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    if (ImGui::Begin("some3dviewer")) {
        draw_3d_viewer_panel_content(impl);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// update the 3D scene camera based on any (ImGui-polled) user input
static void update_camera_from_user_input(Meshes_to_model_wizard_screen::Impl& impl) {
    if (!impl.mouse_over_render) {
        return;
    }

    // handle scroll zooming
    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/5.0f;

    if (impl.mouse_over_render && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImVec2 screendims = impl.render_target.dimensions();
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

// SCREEN TICK
static void tick_meshes_to_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl) {

    // pop anything from the mesh loader's output queue
    for (auto maybe_resp = impl.mesh_loader.poll(); maybe_resp; maybe_resp = impl.mesh_loader.poll()) {
        Mesh_load_response& resp = *maybe_resp;

        if (std::holds_alternative<Mesh_load_OK_response>(resp)) {
            Mesh_load_OK_response& ok = std::get<Mesh_load_OK_response>(resp);

            // remove it from the "currently loading" list
            osc::remove_erase(impl.loading_meshes, [id = ok.id](auto const& m) { return m.id == id; });

            // add it to the "loaded" mesh list
            impl.meshes.emplace_back(std::move(ok));
        } else {
            Mesh_load_ERORR_response& err = std::get<Mesh_load_ERORR_response>(resp);

            // remove it from the "currently loading" list
            osc::remove_erase(impl.loading_meshes, [id = err.id](auto const& m) { return m.id == id; });

            // log the error (it's the best we can do for now)
            log::error("%s: error loading mesh file: %s", err.filepath.string().c_str(), err.error.c_str());
        }
    }

    // update the camera
    update_camera_from_user_input(impl);

    // handle keyboard input
    if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
        osc::remove_erase(impl.meshes, [](Loaded_mesh const& m) { return m.is_selected; });
    }
}

// public API

osc::Meshes_to_model_wizard_screen::Meshes_to_model_wizard_screen() :
    impl{new Impl{}} {
}

osc::Meshes_to_model_wizard_screen::~Meshes_to_model_wizard_screen() noexcept = default;

void osc::Meshes_to_model_wizard_screen::tick(float) {
    ::tick_meshes_to_model_wizard_screen(*impl);
}

void osc::Meshes_to_model_wizard_screen::draw() {
    ::draw_meshes_to_model_wizard_screen(*impl);
}
