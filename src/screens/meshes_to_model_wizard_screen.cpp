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

#include <imgui.h>
#include <nfd.h>
#include "third_party/IconsFontAwesome5.h"
#include <filesystem>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>

using namespace osc;

// mesh data loaded in background (file loading, IO, etc.)
struct Background_loaded_mesh final {

    // CPU-side mesh data
    Untextured_mesh um;

    // AABB of the mesh data
    AABB aabb;

    // bounding sphere of the mesh data
    Sphere bounding_sphere;

    // immediately start loading the mesh on this thread
    explicit Background_loaded_mesh(std::filesystem::path const& p) :
        um{[&p]() {
            Untextured_mesh rv;
            load_mesh_file_with_simtk_backend(p, rv);
            return rv;
        }()},
        aabb{aabb_from_mesh(um)},
        bounding_sphere{bounding_sphere_from_mesh(um)} {
    }
};

// mesh data loaded in foreground (GPU buffers, etc.)
struct Foreground_loaded_mesh final {

    // index of mesh, loaded into GPU_storage
    Meshidx idx;

    // immediately start uploading the mesh to the GPU on this thread
    explicit Foreground_loaded_mesh(Background_loaded_mesh const& blm) {
        GPU_storage& gs = Application::current().get_gpu_storage();
        idx = Meshidx::from_index(gs.meshes.size());
        gs.meshes.emplace_back(blm.um);
    }
};

// a loading/loaded user mesh
struct User_mesh final {
    // unique ID for the mesh
    int id;

    // location of the mesh file on disk
    std::filesystem::path location;

    // any errors during the loading/GPU transfer process
    std::string error;

    // in-UI transforms to the mesh
    //
    // this is modified when the user edits the mesh's location/orientation in
    // the wizard itself
    glm::mat4 model_mtx{1.0f};

    // mesh data loaded by background thread
    //
    // `nullptr` if not loaded by the background thread or
    // there was an error
    std::unique_ptr<Background_loaded_mesh> bgdata;

    // GPU data transferred by UI thread (required by OpenGL API)
    //
    // `nullptr` if the mesh has not been loaded by the foreground
    // thread or there was an error
    std::unique_ptr<Foreground_loaded_mesh> fgdata;

    // is this mesh hovered by the user?
    bool is_hovered;

    // is this mesh selected by the user?
    bool is_selected;

    explicit User_mesh(int _id, std::filesystem::path _loc) :
        id{_id},
        location{std::move(_loc)},
        error{},
        bgdata{nullptr},
        fgdata{nullptr},
        is_hovered{false},
        is_selected{false} {
    }
};

// top-level function for mesh loader thread (declaration)
static int mesh_loader_thread_main(
        osc::stop_token tok,
        osc::Meshes_to_model_wizard_screen::Impl*);

// data associated with the screen
struct osc::Meshes_to_model_wizard_screen::Impl final {

    // loading/loaded meshes
    //
    // mutex guarded because the mesh loader thread can
    // concurrently mutate this
    Mutex_guarded<std::vector<User_mesh>> meshes;

    // used to provide a unique ID to each mesh so that
    // the worker thread can (fail to) locate a mesh in
    // `meshes` after it has finished loading a mesh
    int latest_id = 1;

    // condition variable for waking up the worker thread
    //
    // used to communicate if the worker is cancelled or
    // there are new meshes
    std::condition_variable worker_cv;

    // worker thread that loads meshes
    //
    // starts immediately on constructionz
    osc::jthread mesh_loader_thread{mesh_loader_thread_main, this};

    // rendering params
    osc::Render_params renderparams;

    // drawlist that is rendered
    osc::Drawlist drawlist;

    // output render target from renderer
    osc::Render_target render_target;

    // scene camera
    osc::Polar_perspective_camera camera;

    // true if the implementation thinks the user's mouse is over the render
    bool mouse_over_render = true;

    // true if the implementation thinks the user's mouse is over a gizmo
    bool mouse_over_gizmo = false;

    // true if renderer should draw AABBs
    bool draw_aabbs = false;

    // true if renderer should draw bounding spheres
    bool draw_bounding_spheres = false;

    ~Impl() {
        // on destruction, signal the worker to stop and wake it up
        // so that it has a chance to stop
        mesh_loader_thread.request_stop();
        worker_cv.notify_all();
    }
};

// returns true if the a user mesh needs data to be loaded
[[nodiscard]] static bool needs_mesh_data(User_mesh const& um) {
    return um.bgdata.get() == nullptr;
}

// returns true if any user mesh needs mesh data to be loaded
[[nodiscard]] static bool contains_meshes_that_need_to_load_data(std::vector<User_mesh> const& ums) {
    return std::any_of(ums.begin(), ums.end(), needs_mesh_data);
}

// WORKER THREAD: step
//
// the worker thread continually does this in a loop while the
// screen meshes screen is open
static void mesh_loader_step(osc::stop_token const& tok, osc::Meshes_to_model_wizard_screen::Impl& impl) {
    int id_to_load;
    std::filesystem::path location_to_load;

    {
        // wait for meshes to arrive, or cancellation
        auto guard = impl.meshes.unique_lock();
        impl.worker_cv.wait(guard.raw_guard(), [&]() {
            return tok.stop_requested() ||
                   contains_meshes_that_need_to_load_data(*guard);
        });

        // cancellation requested
        if (tok.stop_requested()) {
            return;
        }

        // else: find the mesh to load
        auto it = std::find_if(guard->begin(), guard->end(), needs_mesh_data);

        // edge-case that shouldn't happen
        if (it == guard->end()) {
            return;
        }

        id_to_load = it->id;
        location_to_load = it->location;

        // drop mutex guard here
        //
        // the worker thread will independently load the mesh etc. without
        // blocking the UI, followed by reacquiring the mutex, checking to
        // ensure the UI didn't delete the User_mesh in the meantime, etc.
    }

    OSC_ASSERT(id_to_load != -1);
    OSC_ASSERT(!location_to_load.empty());


    // try loading the data in this thread with no mutex acquired

    std::unique_ptr<Background_loaded_mesh> loaded = nullptr;
    std::string error;

    try {
        loaded = std::make_unique<Background_loaded_mesh>(location_to_load);
    } catch (std::exception const& ex) {
        error = ex.what();
    }

    // then acquire the mutex, so we can send the update to the UI

    auto guard = impl.meshes.lock();
    auto it = std::find_if(guard->begin(), guard->end(), [id_to_load](User_mesh const& um) {
        return um.id == id_to_load;
    });

    if (it == guard->end()) {
        // edge-case: the user deleted the element while this
        // thread loaded it
        return;
    }

    User_mesh& um = *it;

    if (!error.empty()) {
        // edge-case: there was an error, update the user mesh to reflect this
        um.error = std::move(error);
        return;
    }

    OSC_ASSERT(loaded.get() != nullptr);

    // assign the data to the user model, so the UI thread can see the update
    um.bgdata = std::move(loaded);

    // mutex drops here
}


// WORKER THREAD: main
//
// see step (above) for implementation details
static int mesh_loader_thread_main(osc::stop_token tok,
                            osc::Meshes_to_model_wizard_screen::Impl* impl) {
    try {
        while (!tok.stop_requested()) {
            mesh_loader_step(tok, *impl);
        }
    } catch (std::exception const& ex) {
        log::error("exception thrown in the meshloader background thread: %s", ex.what());
        throw;
    } catch (...) {
        log::error("exception of unknown type thrown in the meshloader background thread");
        throw;
    }

    return 0;
}

// synchronously prompt the user to select multiple mesh files through
// a native OS file dialog
static void prompt_user_to_select_multiple_mesh_files(Meshes_to_model_wizard_screen::Impl& impl) {
    nfdpathset_t s{};
    nfdresult_t result = NFD_OpenDialogMultiple("obj,vtp,stl", nullptr, &s);

    if (result == NFD_OKAY) {
        OSC_SCOPE_GUARD({ NFD_PathSet_Free(&s); });

        size_t len = NFD_PathSet_GetCount(&s);
        auto guard = impl.meshes.lock();
        for (size_t i = 0; i < len; ++i) {
            nfdchar_t* path = NFD_PathSet_GetPath(&s, i);
            guard->emplace_back(impl.latest_id++, std::filesystem::path{path});
        }
        impl.worker_cv.notify_all();
    } else if (result == NFD_CANCEL) {
        // do nothing
    } else {
        log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
    }
}

// DRAW popover tooltip that shows a mesh's details
static void draw_usermesh_tooltip(User_mesh const& um) {
    ImGui::BeginTooltip();
    ImGui::Text("id = %i", um.id);
    ImGui::Text("filename = %s", um.location.filename().string().c_str());
    ImGui::Text("is_hovered = %s", um.is_hovered ? "true" : "false");
    ImGui::Text("is_selected = %s", um.is_selected ? "true" : "false");
    if (um.bgdata) {
        Background_loaded_mesh const& b = *um.bgdata;
        ImGui::Text("verts = %zu", b.um.verts.size());
        ImGui::Text("elements = %zu", b.um.indices.size());

        ImGui::Text("AABB.p1 = (%.2f, %.2f, %.2f)", b.aabb.p1.x, b.aabb.p1.y, b.aabb.p1.z);
        ImGui::Text("AABB.p2 = (%.2f, %.2f, %.2f)", b.aabb.p2.x, b.aabb.p2.y, b.aabb.p2.z);
        glm::vec3 center = (b.aabb.p1 + b.aabb.p2) / 2.0f;
        ImGui::Text("center(AABB) = (%.2f, %.2f, %.2f)", center.x, center.y, center.z);

        ImGui::Text("sphere = O(%.2f, %.2f, %.2f), r(%.2f)", b.bounding_sphere.origin.x, b.bounding_sphere.origin.y, b.bounding_sphere.origin.z, b.bounding_sphere.radius);
    }
    ImGui::EndTooltip();
}

// create a Loaded_user_mesh by stealing data from a User_mesh
[[nodiscard]] static Loaded_user_mesh pilfer_loaded_mesh_from(User_mesh&& um) {
    OSC_ASSERT(um.bgdata != nullptr);
    OSC_ASSERT(um.fgdata != nullptr);

    Loaded_user_mesh rv;
    rv.location = std::move(um.location);
    rv.meshdata = std::move(um.bgdata->um);
    rv.aabb = std::move(um.bgdata->aabb);
    rv.bounding_sphere = std::move(um.bgdata->bounding_sphere);
    rv.gpu_meshidx = std::move(um.fgdata->idx);
    rv.model_mtx = um.model_mtx;
    rv.is_hovered = um.is_hovered;
    rv.is_selected = um.is_selected;
    rv.assigned_body = -1;
    return rv;
}

// DRAW meshlist
//
// returns nonempty vector if the user clicks "Next step" in the UI
static std::vector<Loaded_user_mesh> draw_meshlist_panel_content(Meshes_to_model_wizard_screen::Impl& impl) {

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

    auto guard = impl.meshes.lock();
    auto is_fully_loaded = [](User_mesh const& um) {
        return um.fgdata && um.bgdata;
    };

    if (!guard->empty() && std::all_of(guard->begin(), guard->end(), is_fully_loaded)) {
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_RIGHT "Next step (body assignment)")) {
            // reorgnaize the various pieces of mesh data into the input
            // for the next screen
            std::vector<Loaded_user_mesh> loaded_meshes;
            loaded_meshes.reserve(guard->size());
            for (User_mesh& um : *guard) {
                loaded_meshes.push_back(pilfer_loaded_mesh_from(std::move(um)));
            }
            return loaded_meshes;
        }
    }

    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::Text("meshes (%zu):", guard->size());
    ImGui::Separator();

    if (guard->empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.6f, 0.6f, 0.6f, 1.0f});
        ImGui::TextUnformatted("  (no meshes added yet)");
        ImGui::PopStyleColor();
    }

    for (int i = 0; i < static_cast<int>(guard->size()); ++i) {
        User_mesh& mesh = (*guard)[i];

        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.6f, 0.0f, 0.0f, 1.0f});
        if (ImGui::Button("X")) {
            guard->erase(guard->begin() + i);
            --i;
            ImGui::PopStyleColor();
            continue;
        }
        ImGui::PopStyleColor();
        ImGui::PopID();

        std::string filename = mesh.location.filename().string();
        ImGui::SameLine();
        ImGui::Text("%s%s", filename.c_str(), mesh.fgdata ? "" : mesh.bgdata ? "(rendering)" : "(loading)");

        mesh.is_hovered = ImGui::IsItemHovered();
        if (mesh.is_hovered) {
            draw_usermesh_tooltip(mesh);
        }

        if (ImGui::IsItemClicked()) {
            bool lshift_down = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT);
            bool rshift_down = ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            if (!(lshift_down || rshift_down)) {
                for (auto& um : *guard) {
                    um.is_selected = false;
                }
            }

            mesh.is_selected = true;
        }
    }

    return {};
}

// DRAW 3d viewer
static void draw_3d_viewer_panel_content(Meshes_to_model_wizard_screen::Impl& impl) {
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
    auto guard = impl.meshes.lock();

    decltype(guard->end()) it;
    if (hovered_meshid == 0) {
        it = guard->end();
    } else {
        it = std::find_if(guard->begin(), guard->end(), [hovered_meshid](User_mesh const& um) {
                return um.id == hovered_meshid;
            });
    }

    // handle mouse
    if (impl.mouse_over_gizmo) {
        // mouse is being handled by ImGuizmo

    } else if (it != guard->end()) {
        // mouse is over a piece of 3D geometry in the scene

        it->is_hovered = true;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            bool lshift_down = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT);
            bool rshift_down = ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            if (!(lshift_down || rshift_down)) {
                for (auto& um : *guard) {
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
                for (auto& um : *guard) {
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
    if (it != guard->end()) {
        draw_usermesh_tooltip(*it);
    }

    // draw manipulation gizmos
    {
        int nselected = 0;
        glm::vec3 avg_center = {0.0f, 0.0f, 0.0f};
        for (User_mesh const& um : *guard) {
            if (!um.is_selected) {
                continue;
            }

            if (!um.bgdata) {
                continue;
            }

            glm::vec3 raw_center = aabb_center(um.bgdata->aabb);
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

                for (User_mesh& um : *guard) {
                    if (!um.is_selected) {
                        continue;
                    }

                    if (!um.bgdata) {
                        continue;
                    }

                    um.model_mtx = applied_xform * um.model_mtx;
                }
            }
        }

        impl.mouse_over_gizmo = ImGuizmo::IsOver();
    }
}

// DRAW wizard screen
static void draw_meshes_to_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl) {
    ImGuizmo::BeginFrame();

    if (ImGui::Begin("Mesh list")) {
        auto maybe_meshes = draw_meshlist_panel_content(impl);
        if (!maybe_meshes.empty()) {
            ImGui::End();
            Application::current().request_screen_transition<Meshes_to_model_wizard_screen_step2>(std::move(maybe_meshes));
            return;
        }
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    if (ImGui::Begin("some3dviewer")) {
        draw_3d_viewer_panel_content(impl);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// update the camera
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

// tick: perform any UI updates (e.g. uploading things to the GPU)
static void tick_meshes_to_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl) {

    GPU_storage const& s = Application::current().get_gpu_storage();

    auto guard = impl.meshes.lock();

    // upload any background-loaded (CPU-side) mesh data to the GPU
    for (auto& mesh : *guard) {
        if (mesh.bgdata && !mesh.fgdata) {
            mesh.fgdata = std::make_unique<Foreground_loaded_mesh>(*mesh.bgdata);
        }
    }

    // generate a rendering drawlist that reflects the UI state
    impl.drawlist.clear();
    for (auto& mesh : *guard) {


        if (!mesh.bgdata || !mesh.fgdata) {
            continue;  // skip meshes that haven't loaded yet
        }
        auto const& bgdata = *mesh.bgdata;
        auto const& fgdata = *mesh.fgdata;

        // draw the geometry
        {
            Mesh_instance mi;
            mi.model_xform = mesh.model_mtx;
            mi.normal_xform = normal_matrix(mi.model_xform);
            mi.rgba = Rgba32::from_vec4({1.0f, 1.0f, 1.0f, 1.0f});
            mi.meshidx = fgdata.idx;
            mi.passthrough.rim_alpha = mesh.is_selected ? 0xff : mesh.is_hovered ? 0x60 : 0x00;
            mi.passthrough.assign_u16(static_cast<uint16_t>(mesh.id));
            impl.drawlist.push_back(mi);
        }

        // also, draw the aabb
        if (impl.draw_aabbs) {
            Mesh_instance mi2;
            AABB const& aabb = bgdata.aabb;
            glm::vec3 o = (aabb.p1 + aabb.p2)/2.0f;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, o - aabb.p1);
            glm::mat4 translate = glm::translate(glm::mat4{1.0f}, o);
            mi2.model_xform = mesh.model_mtx * translate * scaler;
            mi2.normal_xform = normal_matrix(mi2.model_xform);
            mi2.rgba = Rgba32::from_vec4({1.0f, 0.0f, 0.0f, 1.0f});
            mi2.meshidx = s.cube_lines_idx;
            mi2.flags.set_draw_lines();
            impl.drawlist.push_back(mi2);
        }

        // also, draw the bounding sphere
        if (impl.draw_bounding_spheres) {
            Mesh_instance mi2;
            Sphere const& sph = bgdata.bounding_sphere;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {sph.radius, sph.radius, sph.radius});
            glm::mat4 translate = glm::translate(glm::mat4{1.0f}, sph.origin);
            mi2.model_xform = mesh.model_mtx * translate * scaler;
            mi2.normal_xform = normal_matrix(mi2.model_xform);
            mi2.rgba = Rgba32::from_vec4({0.0f, 1.0f, 0.0f, 0.3f});
            mi2.meshidx = s.simbody_sphere_idx;
            mi2.flags.set_draw_lines();
            impl.drawlist.push_back(mi2);
        }
    }

    update_camera_from_user_input(impl);

    // perform deletions, if necessary
    if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
        auto it = std::remove_if(guard->begin(), guard->end(), [](User_mesh const& um) {
            return um.is_selected;
        });
        guard->erase(it, guard->end());
    }
}

// public API

osc::Meshes_to_model_wizard_screen::Meshes_to_model_wizard_screen() :
    impl{new Impl} {
}

osc::Meshes_to_model_wizard_screen::~Meshes_to_model_wizard_screen() noexcept = default;

void osc::Meshes_to_model_wizard_screen::tick(float) {
    ::tick_meshes_to_model_wizard_screen(*impl);
}

void osc::Meshes_to_model_wizard_screen::draw() {
    ::draw_meshes_to_model_wizard_screen(*impl);
}
