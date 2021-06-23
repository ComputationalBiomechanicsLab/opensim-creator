#include "meshes_to_model_wizard_screen.hpp"

#include "src/log.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/utils/shims.hpp"
#include "src/assertions.hpp"
#include "src/simtk_bindings/simtk_bindings.hpp"
#include "src/3d/3d.hpp"
#include "src/3d/cameras.hpp"
#include "src/application.hpp"

#include <imgui.h>
#include <nfd.h>
#include "third_party/IconsFontAwesome5.h"
#include <filesystem>
#include <SimTKcommon/internal/DecorativeGeometry.h>

using namespace osc;

// mesh data loaded in background (file loading, IO, etc.)
struct Background_loaded_mesh final {

    // CPU-side mesh data
    Untextured_mesh um;

    Background_loaded_mesh(std::filesystem::path const& p) {
        load_mesh_file_with_simtk_backend(p, um);
    }
};

// mesh data loaded in foreground (GPU buffers, etc.)
struct Foreground_loaded_mesh final {

    // index of mesh, loaded into GPU_storage
    Meshidx idx;

    Foreground_loaded_mesh(Background_loaded_mesh const& blm) {
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

    explicit User_mesh(int _id, std::filesystem::path _loc) :
        id{_id},
        location{std::move(_loc)},
        error{},
        bgdata{nullptr},
        fgdata{nullptr} {
    }
};

// top-level function for mesh loader thread
static int mesh_loader_thread_main(
        osc::stop_token tok,
        osc::Meshes_to_model_wizard_screen::Impl*);

// data associated with the screen
struct osc::Meshes_to_model_wizard_screen::Impl final {

    // loading/loaded meshes
    //
    // mutex guarded because the mesh loader can mutate
    // this also
    Mutex_guarded<std::vector<User_mesh>> meshes{};

    // used to provide a unique ID to each mesh so that
    // the worker thread can (fail to) locate a mesh in
    // `meshes` after it has finished loading a mesh
    int latest_id = 0;

    // condition variable for waking up the worker thread
    //
    // used to communicate if the worker is cancelled or
    // there are new meshes
    std::condition_variable worker_cv{};

    // worker thread that loads meshes
    //
    // starts immediately on construction
    osc::jthread mesh_loader_thread{mesh_loader_thread_main, this};

    // rendering params
    osc::Render_params renderparams;

    // drawlist that is rendered
    osc::Drawlist drawlist;

    // output render target from renderer
    osc::Render_target render_target;

    // scene camera
    osc::Polar_perspective_camera camera;

    // true if implementation thinks mouse is over the render
    bool mouse_over_render = true;

    ~Impl() {
        // on destruction, signal the worker to stop and wake it up
        // so that it has a chance to stop
        mesh_loader_thread.request_stop();
        worker_cv.notify_all();
    }
};

// returns true if the mesh needs data
[[nodiscard]] static bool needs_mesh_data(User_mesh const& um) {
    return um.bgdata.get() == nullptr;
}

// returns true if any mesh needs mesh data
[[nodiscard]] static bool contains_meshes_that_need_to_load_data(std::vector<User_mesh> const& ums) {
    return std::any_of(ums.begin(), ums.end(), needs_mesh_data);
}

// single step of the worker thread
//
// the worker thread continually does this in a loop while the
// screen is open
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

// synchronously prompt the user to select mesh files in
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
            User_mesh& um = guard->emplace_back(impl.latest_id++, std::filesystem::path{path});
        }
        impl.worker_cv.notify_all();
    } else if (result == NFD_CANCEL) {
        // do nothing
    } else {
        log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
    }
}

// draw the UI
static void draw_meshes_to_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl) {

    ImGui::Begin("Mesh list");

    if (ImGui::Button(ICON_FA_PLUS "Add Meshes")) {
        prompt_user_to_select_multiple_mesh_files(impl);
    }

    auto guard = impl.meshes.lock();

    ImGui::Columns(2);
    for (auto const& mesh : *guard) {
        std::string filename = mesh.location.filename().string();

        ImGui::TextUnformatted(filename.c_str());
        ImGui::NextColumn();
        if (mesh.fgdata) {
            ImGui::Text("GPU loaded (%i)", mesh.fgdata->idx.get());
        } else if (mesh.bgdata) {
            int verts = static_cast<int>(mesh.bgdata->um.verts.size());
            int els = static_cast<int>(mesh.bgdata->um.indices.size());
            ImGui::Text("loaded (%i verts, %i els)", verts, els);
        } else {
            ImGui::TextUnformatted("loading");
        }
        ImGui::NextColumn();
    }
    ImGui::Columns();

    ImGui::End();

    // 3d viewer
    ImGui::Begin("some3dviewer");
    ImGui::Text("%.0f", static_cast<double>(ImGui::GetIO().Framerate));

    // render scene to render target
    {

        // render will fill remaining content region
        ImVec2 dims = ImGui::GetContentRegionAvail();
        impl.render_target.reconfigure(
            static_cast<int>(dims.x), static_cast<int>(dims.y), Application::current().samples());

        // set params to use latest camera state
        impl.renderparams.view_matrix = view_matrix(impl.camera);
        impl.renderparams.projection_matrix = projection_matrix(impl.camera, impl.render_target.aspect_ratio());

        // do drawcall
        auto& app = Application::current();
        app.disable_vsync();
        GPU_storage& gpu =  app.get_gpu_storage();
        draw_scene(gpu, impl.renderparams, impl.drawlist, impl.render_target);

        gl::Texture_2d& tex = impl.render_target.main();

        void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex.get()));
        ImVec2 image_dimensions{dims.x, dims.y};
        ImVec2 uv0{0, 1};
        ImVec2 uv1{1, 0};
        ImGui::Image(texture_handle, image_dimensions, uv0, uv1);

    }

    ImGui::End();
}

static void update_camera_from_user_input(Meshes_to_model_wizard_screen::Impl& impl) {
    // update camera
    //
    // At the moment, all camera updates happen via the middle-mouse. The mouse
    // input is designed to mirror Blender fairly closely (because, imho, it has
    // decent UX for this problem space)
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

static bool action_step_zoom_in(osc::Polar_perspective_camera& camera) {
    if (camera.radius >= 0.1f) {
        camera.radius *= 0.9f;
        return true;
    } else {
        return false;
    }
}

static bool action_step_zoom_out(osc::Polar_perspective_camera& camera) {
    if (camera.radius < 100.0f) {
        camera.radius /= 0.9f;
        return true;
    } else {
        return false;
    }
}

static bool on_event_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl, SDL_Event const& e) {
    if (e.type == SDL_MOUSEWHEEL) {
        return e.wheel.y > 0 ? action_step_zoom_in(impl.camera) : action_step_zoom_out(impl.camera);
    } else {
        return false;
    }
}

// tick: perform any UI updates (e.g. uploading things to the GPU)
static void tick_meshes_to_model_wizard_screen(Meshes_to_model_wizard_screen::Impl& impl) {

    // upload any background-loaded (CPU-side) mesh data to the GPU
    auto guard = impl.meshes.lock();
    for (auto& mesh : *guard) {
        if (mesh.bgdata && !mesh.fgdata) {
            mesh.fgdata = std::make_unique<Foreground_loaded_mesh>(*mesh.bgdata);
        }
    }

    // update the drawlist
    impl.drawlist.clear();
    for (auto& mesh : *guard) {
        if (mesh.fgdata) {
            Mesh_instance mi;
            mi.model_xform = glm::mat4{1.0f};
            mi.normal_xform = glm::mat4{1.0f};
            mi.rgba = Rgba32::from_vec4({1.0f, 1.0f, 1.0f, 1.0f});
            mi.meshidx = mesh.fgdata->idx;
            impl.drawlist.push_back(mi);
        }
    }

    update_camera_from_user_input(impl);
    ImGui::CaptureMouseFromApp(false);
}


// public API

osc::Meshes_to_model_wizard_screen::Meshes_to_model_wizard_screen() :
    impl{new Impl} {
}

osc::Meshes_to_model_wizard_screen::~Meshes_to_model_wizard_screen() noexcept = default;

bool osc::Meshes_to_model_wizard_screen::on_event(SDL_Event const& e) {
    return ::on_event_model_wizard_screen(*impl, e);
}

void osc::Meshes_to_model_wizard_screen::tick(float) {
    ::tick_meshes_to_model_wizard_screen(*impl);
}

void osc::Meshes_to_model_wizard_screen::draw() {
    ::draw_meshes_to_model_wizard_screen(*impl);
}
