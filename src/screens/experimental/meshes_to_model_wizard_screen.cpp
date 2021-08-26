#include "meshes_to_model_wizard_screen.hpp"

#include "src/app.hpp"
#include "src/log.hpp"
#include "src/styling.hpp"
#include "src/3d/bvh.hpp"
#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/model.hpp"
#include "src/3d/texturing.hpp"
#include "src/simtk_bindings/stk_meshloader.hpp"
#include "src/simtk_bindings/stk_converters.hpp"
#include "src/utils/algs.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/utils/shims.hpp"
#include "src/utils/spsc.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include <nfd.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <SimTKcommon.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <variant>

using namespace osc;

// private impl details
namespace {

    // a request, made by UI thread, to load a mesh file
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
        NewMesh um;

        // AABB of the mesh data
        AABB aabb;

        // bounding sphere of the mesh data
        Sphere bounding_sphere;

        // triangle bvh of the mesh
        BVH triangle_bvh;
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

    // a response to a mesh loading request
    using Mesh_load_response = std::variant<
        Mesh_load_OK_response,
        Mesh_load_ERORR_response
    >;

    // MESH LOADER FN: respond to load request (and handle errors)
    //
    // this typically runs on a background thread
    Mesh_load_response respond_to_meshload_request(Mesh_load_request const& msg) noexcept {
        try {
            Mesh_load_OK_response rv;
            rv.id = msg.id;
            rv.filepath = msg.filepath;
            rv.um = stk_load_mesh(msg.filepath);  // can throw
            rv.aabb = aabb_from_points(rv.um.verts.data(), rv.um.verts.size());
            rv.bounding_sphere = sphere_bounds_of_points(rv.um.verts.data(), rv.um.verts.size());
            BVH_BuildFromTriangles(rv.triangle_bvh, rv.um.verts.data(), rv.um.verts.size());
            return rv;
        } catch (std::exception const& ex) {
            return Mesh_load_ERORR_response{msg.id, msg.filepath, ex.what()};
        }
    }

    // a meshloader is just a worker on a background thread that listens for requests
    using Mesh_loader = spsc::Worker<Mesh_load_request, Mesh_load_response, decltype(respond_to_meshload_request)>;

    Mesh_loader meshloader_create() {
        return Mesh_loader::create(respond_to_meshload_request);
    }

    // a fully-loaded mesh
    struct Loaded_mesh final {
        // unique mesh ID
        int id;

        // filesystem path of the mesh
        std::filesystem::path filepath;

        // CPU-side mesh data
        NewMesh meshdata;

        // AABB of the mesh data
        AABB aabb;

        // bounding sphere of the mesh data
        Sphere bounding_sphere;

        // triangle BVH of mesh data
        BVH triangle_bvh;

        // model matrix
        glm::mat4 model_mtx;

        // mesh data on GPU
        Instanceable_meshdata gpu_meshdata;

        // -1 if ground/unassigned; otherwise, a body/frame
        int parent;

        // true if the mesh is hovered by the user's mouse
        bool is_hovered;

        // true if the mesh is selected
        bool is_selected;

        // create this by stealing from an OK background response
        explicit Loaded_mesh(Mesh_load_OK_response&& tmp) :
            id{tmp.id},
            filepath{std::move(tmp.filepath)},
            meshdata{std::move(tmp.um)},
            aabb{tmp.aabb},
            bounding_sphere{tmp.bounding_sphere},
            triangle_bvh{std::move(tmp.triangle_bvh)},
            model_mtx{1.0f},
            gpu_meshdata{upload_meshdata_for_instancing(meshdata)},
            parent{-1},
            is_hovered{false},
            is_selected{false} {
        }
    };

    // descriptive alias: they contain the same information
    using Loading_mesh = Mesh_load_request;

    // a body or frame (bof) the user wants to add into the output model
    struct Body_or_frame final {
        // -1 if ground/unassigned; otherwise, a body/frame
        int parent;

        // absolute position in space
        glm::vec3 pos;

        // true if it is a frame, rather than a body
        bool is_frame;

        // true if it is selected in the UI
        bool is_selected;

        // true if it is hovered in the UI
        bool is_hovered;
    };

    // type of element in the scene
    enum class ElType { None, Mesh, Body, Ground };

    // state associated with an open context menu
    struct Context_menu_state final {
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

    // result of 3D hovertest on the scene
    struct Hovertest_result final {
        // index of the hovered element
        //
        // ignore if type == None or type == Ground
        int idx = -1;

        // type of hovered element
        ElType type = ElType::None;
    };

    // returns the worldspace center of a bof
    [[nodiscard]] constexpr glm::vec3 center(Body_or_frame const& bof) noexcept {
        return bof.pos;
    }

    // returns the worldspace center of a loaded user mesh
    [[nodiscard]] glm::vec3 center(Loaded_mesh const& lm) noexcept {
        glm::vec3 c = aabb_center(lm.aabb);
        return glm::vec3{lm.model_mtx * glm::vec4{c, 1.0f}};
    }

    // draw an ImGui color picker for an OSC Rgba32
    void Rgba32_ColorEdit4(char const* label, Rgba32* rgba) {
        ImVec4 col{
            static_cast<float>(rgba->r) / 255.0f,
            static_cast<float>(rgba->g) / 255.0f,
            static_cast<float>(rgba->b) / 255.0f,
            static_cast<float>(rgba->a) / 255.0f,
        };

        if (ImGui::ColorEdit4(label, reinterpret_cast<float*>(&col))) {
            rgba->r = static_cast<unsigned char>(col.x * 255.0f);
            rgba->g = static_cast<unsigned char>(col.y * 255.0f);
            rgba->b = static_cast<unsigned char>(col.z * 255.0f);
            rgba->a = static_cast<unsigned char>(col.w * 255.0f);
        }
    }
}

struct osc::Meshes_to_model_wizard_screen::Impl final {

    // loader that loads mesh data in a background thread
    Mesh_loader mesh_loader = meshloader_create();

    // fully-loaded meshes
    std::vector<Loaded_mesh> meshes;

    // not-yet-loaded meshes
    std::vector<Loading_mesh> loading_meshes;

    // the bodies/frames that the user adds during this step
    std::vector<Body_or_frame> bofs;

    // latest unique ID available
    //
    // this should be incremented whenever an entity in the scene
    // needs a fresh ID
    int latest_id = 1;

    // set by draw step to render's topleft location in screenspace
    glm::vec2 render_topleft_in_screen = {0.0f, 0.0f};

    // color of assigned (i.e. attached to a body/frame) meshes
    // rendered in the 3D scene
    Rgba32 assigned_mesh_color = rgba32_from_f4(1.0f, 1.0f, 1.0f, 1.0f);

    // color of unassigned meshes rendered in the 3D scene
    Rgba32 unassigned_mesh_color = rgba32_from_u32(0xFFE4E4FF);

    // color of ground (sphere @ 0,0,0) rendered in the 3D scene
    Rgba32 ground_color = rgba32_from_f4(0.0f, 0.0f, 1.0f, 1.0f);

    // color of a body rendered in the 3D scene
    Rgba32 body_color = rgba32_from_f4(1.0f, 0.0f, 0.0f, 1.0f);

    // color of a frame rendered in the 3D scene
    Rgba32 frame_color = rgba32_from_f4(0.0f, 1.0f, 0.0f, 1.0f);

    // radius of rendered ground sphere
    float ground_sphere_radius = 0.008f;

    // radius of rendered bof spheres
    float bof_sphere_radius = 0.005f;

    // scale factor for all non-mesh, non-overlay scene elements (e.g.
    // the floor, bodies)
    //
    // this is necessary because some meshes can be extremely small/large and
    // scene elements need to be scaled accordingly (e.g. without this, a body
    // sphere end up being much larger than a mesh instance). Imagine if the
    // mesh was the leg of a fly, in meters.
    float scene_scale_factor = 1.0f;

    // the transform matrix that the gizmo is manipulating (if active)
    //
    // this is set when the user initially starts interacting with a gizmo
    glm::mat4 gizmo_mtx;

    // the transformation operation that the gizmo should be doing
    ImGuizmo::OPERATION gizmo_op = ImGuizmo::TRANSLATE;

    // 3D rendering params
    osc::Render_params renderparams;

    // floor data
    std::shared_ptr<gl::Texture_2d> floor_tex = std::make_shared<gl::Texture_2d>(generate_chequered_floor_texture());
    NewMesh floor_mesh = []() {
        NewMesh rv = gen_textured_quad();
        for (auto& uv : rv.texcoords) {
            uv *= 200.0f;
        }
        return rv;
    }();
    Instanceable_meshdata floor_meshdata = upload_meshdata_for_instancing(floor_mesh);
    Instanceable_meshdata sphere_meshdata = upload_meshdata_for_instancing(gen_untextured_uv_sphere(12, 12));

    // 3D rendering instance data
    std::vector<glm::mat4x3> render_xforms;
    std::vector<glm::mat3> render_normal_matrices;
    std::vector<Rgba32> render_colors;
    std::vector<Instanceable_meshdata> render_meshes;
    std::vector<std::shared_ptr<gl::Texture_2d>> render_textures;
    std::vector<unsigned char> render_rims;

    // 3D drawlist that is rendered
    osc::Instanced_drawlist drawlist;

    // 3D renderer for the drawlist
    osc::Instanced_renderer renderer;

    // 3D scene camera
    osc::Polar_perspective_camera camera;

    // context menu state
    //
    // values in this member are set when the menu is initially
    // opened by an ImGui::OpenPopup call
    Context_menu_state ctx_menu;

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

    // set to true if the implementation thinks the user's mouse is over a gizmo
    bool mouse_over_gizmo = false;

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

    Impl() {
        camera.phi = fpi2;
    }
};
using Impl = osc::Meshes_to_model_wizard_screen::Impl;

// private Impl functions
namespace {

    // returns true if the bof is connected to ground - including whether it is
    // connected to ground *via* some other bodies
    //
    // returns false if it is connected to bullshit (i.e. an invalid index) or
    // to something that, itself, does not connect to ground (e.g. a cycle)
    [[nodiscard]] bool is_bof_connected_to_ground(Impl& impl, int bofidx) {
        int jumps = 0;
        while (bofidx >= 0) {
            if (static_cast<size_t>(bofidx) >= impl.bofs.size()) {
                return false;  // out of bounds
            }

            bofidx = impl.bofs[static_cast<size_t>(bofidx)].parent;

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
    void test_for_advancement_issues(Impl& impl) {
        impl.advancement_issues.clear();

        // ensure all meshes are assigned to a valid body/ground
        for (Loaded_mesh const& m : impl.meshes) {
            if (m.parent < 0) {
                // assigned to ground (bad practice)
                impl.advancement_issues.push_back("a mesh is assigned to ground (it should be assigned to a body/frame)");
            } else if (static_cast<size_t>(m.parent) >= impl.bofs.size()) {
                // invalid index
                impl.advancement_issues.push_back("a mesh is assigned to an invalid body");
            }
        }

        // ensure all bodies are connected to ground *eventually*
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            Body_or_frame const& bof = impl.bofs[i];

            if (bof.parent < 0) {
                // ok: it's directly connected to ground
            } else if (static_cast<size_t>(bof.parent) >= impl.bofs.size()) {
                // bad: it's connected to non-existent bs
                impl.advancement_issues.push_back("a body/frame is connected to a non-existent body/frame");
            } else if (!is_bof_connected_to_ground(impl, static_cast<int>(i))) {
                // bad: it's connected to something, but that thing doesn't connect to ground
                impl.advancement_issues.push_back("a body/frame is not connected to ground");
            } else {
                // ok: it's connected to a body/frame that is connected to ground
            }
        }
    }

    // recursively add a body/frame (bof) tree to an OpenSim::Model
    void recursively_add_bof(
        Impl& impl,
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
            impl.bofs[static_cast<size_t>(parent_idx)].pos;

        // the body/frame to add in this step
        Body_or_frame const& bof = impl.bofs[static_cast<size_t>(bof_idx)];

        OpenSim::PhysicalFrame* added_pf = nullptr;

        // create body/pof and add it into the model
        if (!bof.is_frame) {
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

            // attach geometry
            for (Loaded_mesh const& m : impl.meshes) {
                if (m.parent != bof_idx) {
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
                SimTK::Transform mmtx = stk_xform_from_mat4x3(m.model_mtx);

                mesh_pof->setOffsetTransform(xform_ground2parent * mmtx);
                body->addComponent(mesh_pof);

                // attach mesh to the POF
                OpenSim::Mesh* mesh = new OpenSim::Mesh{m.filepath.string()};
                mesh_pof->attachGeometry(mesh);
            }

            // assign added_pf
            added_pf = body;
        } else {
            // user requested a Frame to be added

            auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            SimTK::Vec3 translation = stk_vec3_from(bof.pos - world_parent_pos);
            pof->set_translation(translation);
            pof->setParentFrame(parent_pf);

            added_pf = pof.get();
            parent_pf.addComponent(pof.release());
        }

        OSC_ASSERT(added_pf != nullptr);

        // RECURSE (depth-first)
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            Body_or_frame const& b = impl.bofs[i];

            // if a body points to the current body/frame, recurse to it
            if (b.parent == bof_idx) {
                recursively_add_bof(impl, model, static_cast<int>(i), bof_idx, *added_pf);
            }
        }
    }


    // tries to create an OpenSim::Model from the current screen state
    //
    // will test to ensure the current screen state is actually valid, though
    void try_creating_output_model(Impl& impl) {
        test_for_advancement_issues(impl);
        if (!impl.advancement_issues.empty()) {
            log::error("cannot create an osim model: advancement issues detected");
            return;
        }

        std::unique_ptr<OpenSim::Model> m = std::make_unique<OpenSim::Model>();
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            Body_or_frame const& bof = impl.bofs[i];
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

    // sets `is_selected` of all selectable entities in the scene
    void set_is_selected_of_all_to(Impl& impl, bool v) {
        for (Loaded_mesh& m : impl.meshes) {
            m.is_selected = v;
        }
        for (Body_or_frame& bof : impl.bofs) {
            bof.is_selected = v;
        }
    }

    // sets `is_hovered` of all hoverable entities in the scene
    void set_is_hovered_of_all_to(Impl& impl, bool v) {
        for (Loaded_mesh& m : impl.meshes) {
            m.is_hovered = v;
        }
        impl.ground_hovered = v;
        for (Body_or_frame& bof : impl.bofs) {
            bof.is_hovered = v;
        }
    }

    // sets all hovered elements as selected elements
    //
    // (and all not-hovered elements as not selected)
    void set_hovered_els_as_selected(Impl& impl) {
        for (Loaded_mesh& m : impl.meshes) {
            m.is_selected = m.is_hovered;
        }
        for (Body_or_frame& bof : impl.bofs) {
            bof.is_selected = bof.is_hovered;
        }
    }

    // submit a meshfile load request to the mesh loader
    void submit_meshfile_load_request(Impl& impl, std::filesystem::path path) {
        int id = impl.latest_id++;
        Mesh_load_request req{id, std::move(path)};
        impl.loading_meshes.push_back(req);
        impl.mesh_loader.send(req);
    }

    // synchronously prompts the user to select multiple mesh files through
    // a native OS file dialog
    void prompt_user_to_select_multiple_mesh_files(Impl& impl) {
        nfdpathset_t s{};
        nfdresult_t result = NFD_OpenDialogMultiple("obj,vtp,stl", nullptr, &s);

        if (result == NFD_OKAY) {
            OSC_SCOPE_GUARD({ NFD_PathSet_Free(&s); });

            size_t len = NFD_PathSet_GetCount(&s);
            for (size_t i = 0; i < len; ++i) {
                submit_meshfile_load_request(impl, NFD_PathSet_GetPath(&s, i));
            }
        } else if (result == NFD_CANCEL) {
            // do nothing: the user cancelled
        } else {
            log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
        }
    }

    // update the scene's camera based on (ImGui's) user input
    void update_camera_from_user_input(Impl& impl) {

        if (!impl.mouse_over_render) {
            // ignore mouse if it isn't over the render
            return;
        }

        if (ImGuizmo::IsUsing()) {
            // ignore mouse if user currently dragging a gizmo
            return;
        }

        // handle scroll zooming
        impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

        // handle panning/zooming/dragging with middle mouse
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {  // TODO: middle mouse

            // in pixels, e.g. [800, 600]
            glm::vec2 screendims = impl.renderer.dimsf();

            // in pixels, e.g. [-80, 30]
            glm::vec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);

            // as a screensize-independent ratio, e.g. [-0.1, 0.05]
            glm::vec2 relative_delta = mouse_delta / screendims;

            if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
                // shift + middle-mouse performs a pan
                float aspect_ratio = screendims.x / screendims.y;
                impl.camera.do_pan(aspect_ratio, relative_delta);
            } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
                // shift + middle-mouse performs a zoom
                impl.camera.radius *= 1.0f + relative_delta.y;
            } else {
                // just middle-mouse performs a mouse drag
                impl.camera.do_drag(relative_delta);
            }
        }
    }

    // delete all selected elements
    void action_delete_selected(Impl& impl) {

        // nothing refers to meshes, so they can be removed straightforwardly
        auto& meshes = impl.meshes;
        osc::remove_erase(meshes, [](auto const& m) { return m.is_selected; });

        // bodies/frames, and meshes, can refer to other bodies/frames (they're a tree)
        // so deletion needs to update the `assigned_body` and `parent` fields of every
        // other body/frame/mesh to be correct post-deletion

        auto& bofs = impl.bofs;

        // perform parent/assigned_body fixups
        //
        // collect a list of to-be-deleted indices, going from big to small
        std::vector<int> deleted_indices;
        for (int i = static_cast<int>(bofs.size()) - 1; i >= 0; --i) {
            Body_or_frame& b = bofs[i];
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
            for (Body_or_frame& b : bofs) {
                if (b.parent == idx) {
                    b.parent = bofs[static_cast<size_t>(idx)].parent;
                }
                if (b.parent > idx) {
                    --b.parent;
                }
            }

            for (Loaded_mesh& m : meshes) {
                if (m.parent == idx) {
                    m.parent = bofs[static_cast<size_t>(idx)].parent;
                }
                if (m.parent > idx) {
                    --m.parent;
                }
            }
        }

        // with the fixups done, we can now just remove the selected elements as
        // normal
        osc::remove_erase(bofs, [](auto const& b) { return b.is_selected; });
    }

    // add frame to model
    void action_add_frame(Impl& impl, glm::vec3 pos) {
        set_is_selected_of_all_to(impl, false);

        Body_or_frame& bf = impl.bofs.emplace_back();
        bf.parent = -1;
        bf.pos = pos;
        bf.is_frame = true;
        bf.is_selected = true;
        bf.is_hovered = false;
    }

    // add body to model
    void action_add_body(Impl& impl, glm::vec3 pos) {
        set_is_hovered_of_all_to(impl, false);

        Body_or_frame& bf = impl.bofs.emplace_back();
        bf.parent = -1;
        bf.pos = pos;
        bf.is_frame = false;
        bf.is_selected = true;
        bf.is_hovered = false;
    }

    void action_center_camera_on(Impl& impl, Loaded_mesh const& lum) {
        impl.camera.pan = aabb_center(lum.aabb);
    }

    void action_autoscale_scene(Impl& impl) {
        if (impl.meshes.empty()) {
            impl.scene_scale_factor = 1.0f;
            return;
        }

        AABB scene_aabb{{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        for (Loaded_mesh const& mesh : impl.meshes) {
            scene_aabb = aabb_union(scene_aabb, mesh.aabb);
        }

        // we only care about the dimensions of the AABB, not its position
        glm::vec3 dims = aabb_dims(scene_aabb);

        // figure out the longest dimension, scale relative to that
        float longest_dim = vec_longest_dim(dims);

        // update relevant state
        impl.scene_scale_factor = 5.0f * longest_dim;
        impl.camera.pan = {};
        impl.camera.radius = 5.0f * longest_dim;
    }

    // polls the mesh loader's output queue for any newly-loaded meshes and pushes
    // them into the screen's state
    void pop_mesh_loader_output_queue(Impl& impl) {

        // pop anything from the mesh loader's output queue
        for (auto maybe_resp = impl.mesh_loader.poll();
             maybe_resp.has_value();
             maybe_resp = impl.mesh_loader.poll()) {

            Mesh_load_response& resp = *maybe_resp;

            if (std::holds_alternative<Mesh_load_OK_response>(resp)) {
                // handle OK message from loader

                Mesh_load_OK_response& ok = std::get<Mesh_load_OK_response>(resp);

                // remove it from the "currently loading" list
                osc::remove_erase(impl.loading_meshes, [id = ok.id](auto const& m) { return m.id == id; });

                // add it to the "loaded" mesh list
                impl.meshes.emplace_back(std::move(ok));
            } else {
                // handle ERROR message from loader

                Mesh_load_ERORR_response& err = std::get<Mesh_load_ERORR_response>(resp);

                // remove it from the "currently loading" list
                osc::remove_erase(impl.loading_meshes, [id = err.id](auto const& m) { return m.id == id; });

                // log the error (it's the best we can do for now)
                log::error("%s: error loading mesh file: %s", err.filepath.string().c_str(), err.error.c_str());
            }
        }
    }

    // update the screen state (impl) based on (ImGui's) user input
    void update_impl_from_user_input(Impl& impl) {

        // DELETE: delete any selected elements
        if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
            action_delete_selected(impl);
        }

        // B: add body to hovered element
        if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
            set_is_selected_of_all_to(impl, false);

            for (auto const& b : impl.bofs) {
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

            for (size_t i = 0; i < impl.bofs.size(); ++i) {
                Body_or_frame const& bof = impl.bofs[i];
                if (bof.is_hovered) {
                    impl.assignment_st.assigner = static_cast<int>(i);
                    impl.assignment_st.is_body = true;
                }
            }
            for (size_t i = 0; i < impl.meshes.size(); ++i) {
                Loaded_mesh const& m = impl.meshes[i];
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

        // S: set manipulation mode to "scale"
        if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
            impl.gizmo_op = ImGuizmo::SCALE;
        }

        // R: set manipulation mode to "rotate"
        if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
            impl.gizmo_op = ImGuizmo::ROTATE;
        }

        // G: set manipulation mode to "grab" (translate)
        if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
            impl.gizmo_op = ImGuizmo::TRANSLATE;
        }
    }

    // convert a 3D worldspace coordinate into a 2D screenspace (NDC) coordinate
    //
    // used to draw 2D overlays for items that are in 3D
    glm::vec2 world2ndc(Impl& impl, glm::vec3 const& v) {
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
        glm::vec2 screen_dimensions = impl.renderer.dimsf();

        // range [0, w] (X) and [0, h] (Y)
        glm::vec2 screen_pos = screen_dimensions * relative_screenpos;

        return screen_pos;
    }

    // draw a 2D overlay line between a BOF and its parent
    void draw_bof_line_to_parent(Impl& impl, Body_or_frame const& bof) {
        ImDrawList& dl = *ImGui::GetForegroundDrawList();

        glm::vec3 parent_pos;
        if (bof.parent < 0) {
            // its parent is "ground": use ground pos
            parent_pos = {0.0f, 0.0f, 0.0f};
        } else {
            Body_or_frame const& parent = impl.bofs.at(static_cast<size_t>(bof.parent));
            parent_pos = parent.pos;
        }

        auto screen_topleft = impl.render_topleft_in_screen;

        ImVec2 p1 = screen_topleft + world2ndc(impl, bof.pos);
        ImVec2 p2 = screen_topleft + world2ndc(impl, parent_pos);
        ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

        dl.AddLine(p1, p2, color);
    }

    // draw 2D overlay (dotted lines between bodies, etc.)
    void draw_2d_overlay(Impl& impl) {

        // draw connection lines between BOFs
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            Body_or_frame const& bof = impl.bofs[i];

            // only draw connection lines if "draw all connection lines" is
            // enabled, or if this bof in particular is hovered
            if (!(impl.show_all_connection_lines || bof.is_hovered)) {
                continue;
            }

            // draw line from bof to its parent (bof/ground)
            draw_bof_line_to_parent(impl, bof);

            // draw line(s) from any other bofs connected to this one
            for (Body_or_frame const& b : impl.bofs) {
                if (b.parent == static_cast<int>(i)) {
                    draw_bof_line_to_parent(impl, b);
                }
            }
        }
    }

    // draw hover tooltip when hovering over a user mesh
    void draw_mesh_hover_tooltip(Impl&, Loaded_mesh& m) {
        ImGui::BeginTooltip();
        ImGui::Text("filepath = %s", m.filepath.string().c_str());
        ImGui::TextUnformatted(m.parent >= 0 ? "ASSIGNED" : "UNASSIGNED (to a body/frame)");

        ImGui::Text("id = %i", m.id);
        ImGui::Text("filename = %s", m.filepath.filename().string().c_str());
        ImGui::Text("is_hovered = %s", m.is_hovered ? "true" : "false");
        ImGui::Text("is_selected = %s", m.is_selected ? "true" : "false");
        ImGui::Text("verts = %zu", m.meshdata.verts.size());
        ImGui::Text("elements = %zu", m.meshdata.indices.size());

        ImGui::Text("AABB.p1 = (%.2f, %.2f, %.2f)", m.aabb.min.x, m.aabb.min.y, m.aabb.min.z);
        ImGui::Text("AABB.p2 = (%.2f, %.2f, %.2f)", m.aabb.max.x, m.aabb.max.y, m.aabb.max.z);
        glm::vec3 center = (m.aabb.min + m.aabb.max) / 2.0f;
        ImGui::Text("center(AABB) = (%.2f, %.2f, %.2f)", center.x, center.y, center.z);

        ImGui::Text("sphere = O(%.2f, %.2f, %.2f), r(%.2f)",
                    m.bounding_sphere.origin.x,
                    m.bounding_sphere.origin.y,
                    m.bounding_sphere.origin.z,
                    m.bounding_sphere.radius);
        ImGui::EndTooltip();
    }

    // draw hover tooltip when hovering over Ground
    void draw_ground_hover_tooltip(Impl&) {
        ImGui::BeginTooltip();
        ImGui::Text("Ground");
        ImGui::Text("(always present, and defined to be at (0, 0, 0))");
        ImGui::EndTooltip();
    }

    // draw hover tooltip when hovering over a BOF
    void draw_bof_hover_tooltip(Impl&, Body_or_frame& bof) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(bof.is_frame ? "Frame" : "Body");
        ImGui::TextUnformatted(bof.parent >= 0 ? "Connected to another body/frame" : "Connected to ground");
        ImGui::EndTooltip();
    }

    // draw mesh context menu
    void draw_mesh_context_menu_content(Impl& impl, Loaded_mesh& lum) {
        if (ImGui::MenuItem("add body")) {
            action_add_body(impl, center(lum));
        }
        if (ImGui::MenuItem("add frame")) {
            action_add_frame(impl, center(lum));
        }
        if (ImGui::MenuItem("center camera on this mesh")) {
            action_center_camera_on(impl, lum);
        }
    }

    // draw ground context menu
    void draw_ground_context_menu_content(Impl& impl) {
        if (ImGui::MenuItem("add body")) {
            action_add_body(impl, {0.0f, 0.0f, 0.0f});
        }
        if (ImGui::MenuItem("add frame")) {
            action_add_frame(impl, {0.0f, 0.0f, 0.0f});
        }
    }

    // draw bof context menu
    void draw_bof_context_menu_content(Impl&, Body_or_frame&) {
        ImGui::Text("(no actions available)");
    }

    // draw manipulation gizmos (the little handles that the user can click
    // to move things in 3D)
    void draw_3dviewer_manipulation_gizmos(Impl& impl) {

        // if the user isn't manipulating anything, create an up-to-date
        // manipulation matrix
        if (!ImGuizmo::IsUsing()) {
            // only draw gizmos if this is >0
            int nselected = 0;

            AABB aabb{{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};

            for (Loaded_mesh const& m : impl.meshes) {
                if (m.is_selected) {
                    ++nselected;

                    aabb = aabb_union(aabb, aabb_apply_xform(m.aabb, m.model_mtx));
                }
            }
            for (Body_or_frame const& b : impl.bofs) {
                if (b.is_selected) {
                    ++nselected;
                    aabb = aabb_union(aabb, AABB{b.pos, b.pos});
                }
            }

            if (nselected == 0) {
                return;  // early exit: nothing's selected
            }

            glm::vec3 center = aabb_center(aabb);
            impl.gizmo_mtx = glm::translate(glm::mat4{1.0f}, center);
        }

        // else: is using OR nselected > 0 (so draw it)

        ImGuizmo::SetRect(
            impl.render_topleft_in_screen.x,
            impl.render_topleft_in_screen.y,
            impl.renderer.dimsf().x,
            impl.renderer.dimsf().y);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        glm::mat4 delta;
        bool manipulated = ImGuizmo::Manipulate(
            glm::value_ptr(impl.renderparams.view_matrix),
            glm::value_ptr(impl.renderparams.projection_matrix),
            impl.gizmo_op,
            ImGuizmo::WORLD,
            glm::value_ptr(impl.gizmo_mtx),
            glm::value_ptr(delta),
            nullptr,
            nullptr,
            nullptr);

        if (!manipulated) {
            return;
        }
        // else: apply manipulation

        // update relevant positions/model matrices
        for (Loaded_mesh& m : impl.meshes) {
            if (m.is_selected) {
                m.model_mtx = delta * m.model_mtx;
            }
        }
        for (Body_or_frame& b : impl.bofs) {
            if (b.is_selected) {
                b.pos = glm::vec3{delta * glm::vec4{b.pos, 1.0f}};
            }
        }
    }

    glm::mat4x3 create_floor_mmtx(Impl& impl) {
        glm::mat4 rv{1.0f};

        // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
        // floor down *slightly* to prevent Z fighting from planes rendered from the
        // model itself (the contact planes, etc.)
        rv = glm::translate(rv, {0.0f, -0.0001f, 0.0f});
        rv = glm::rotate(rv, fpi2, {-1.0, 0.0, 0.0});
        rv = glm::scale(rv, {impl.scene_scale_factor * 100.0f, impl.scene_scale_factor * 100.0f, 1.0f});

        return rv;
    }

    void perform_scene_hittest(Impl& impl) {
        glm::vec2 mousepos_in_render = glm::vec2{ImGui::GetMousePos()} - impl.render_topleft_in_screen;
        Line camera_ray =
            impl.camera.screenpos_to_world_ray(mousepos_in_render, impl.renderer.dimsf());

        // assumed result if nothing is hovered
        impl.hovertest_result.idx = -1;
        impl.hovertest_result.type = ElType::None;
        float closest = std::numeric_limits<float>::max();

        // perform ray-sphere hittest on the ground sphere
        if (!impl.lock_ground) {
            Sphere ground_sphere{{0.0f, 0.0f, 0.0f}, impl.scene_scale_factor * impl.ground_sphere_radius};
            auto res = get_ray_collision_sphere(camera_ray, ground_sphere);
            if (res.hit && res.distance < closest) {
                closest = res.distance;
                impl.hovertest_result.idx = -1;
                impl.hovertest_result.type = ElType::Ground;
            }
        }

        // perform ray-sphere hittest on the bof spheres
        if (!impl.lock_bofs) {
            float r = impl.scene_scale_factor * impl.bof_sphere_radius;
            for (size_t i = 0; i < impl.bofs.size(); ++i) {
                Body_or_frame const& bof = impl.bofs[i];
                Sphere bof_sphere{bof.pos, r};
                auto res = get_ray_collision_sphere(camera_ray, bof_sphere);
                if (res.hit && res.distance < closest) {
                    closest = res.distance;
                    impl.hovertest_result.idx = static_cast<int>(i);
                    impl.hovertest_result.type = ElType::Body;
                }
            }
        }

        // perform a *rough* ray-AABB hittest on the meshes - if there's
        // a hit, perform a more precise (BVH-accelerated) ray-triangle test
        // on the mesh
        if (!impl.lock_meshes) {
            for (size_t i = 0; i < impl.meshes.size(); ++i) {
                Loaded_mesh const& mesh = impl.meshes[i];
                AABB aabb = aabb_apply_xform(mesh.aabb, mesh.model_mtx);
                auto res = get_ray_collision_AABB(camera_ray, aabb);

                if (res.hit && res.distance < closest) {
                    // got a ray-AABB hit, now see if the ray hits a triangle in the mesh
                    Line camera_ray_modelspace = apply_xform_to_line(camera_ray, glm::inverse(mesh.model_mtx));

                    BVH_Collision collision;
                    if (BVH_get_closest_collision_triangle(mesh.triangle_bvh, mesh.meshdata.verts.data(), mesh.meshdata.verts.size(), camera_ray_modelspace, &collision)) {
                        if (collision.distance < closest) {
                            closest = collision.distance;
                            impl.hovertest_result.idx = static_cast<int>(i);
                            impl.hovertest_result.type = ElType::Mesh;
                        }
                    }
                }
            }
        }
    }

    // draw 3D scene into remainder of the ImGui panel's content region
    void draw_3dviewer_scene(Impl& impl) {
        ImVec2 dims = ImGui::GetContentRegionAvail();

        // skip rendering if the panel would be too small
        if (dims.x < 1.0f || dims.y < 1.0f) {
            return;
        }

        // populate drawlist

        // clear instance buffers
        impl.render_xforms.clear();
        impl.render_normal_matrices.clear();
        impl.render_colors.clear();
        impl.render_meshes.clear();
        impl.render_textures.clear();
        impl.render_rims.clear();

        // add floor
        if (impl.show_floor) {
            auto const& mmtx = impl.render_xforms.emplace_back(create_floor_mmtx(impl));
            impl.render_normal_matrices.push_back(normal_matrix(mmtx));
            impl.render_colors.push_back({});
            impl.render_meshes.push_back(impl.floor_meshdata);
            impl.render_textures.push_back(impl.floor_tex);
            impl.render_rims.push_back(0x00);
        }

        // add meshes
        if (impl.show_meshes) {
            for (Loaded_mesh const& m : impl.meshes) {
                auto const& mmtx = impl.render_xforms.emplace_back(m.model_mtx);
                impl.render_normal_matrices.push_back(normal_matrix(mmtx));
                impl.render_colors.push_back(m.parent >= 0 ? impl.assigned_mesh_color : impl.unassigned_mesh_color);
                impl.render_meshes.push_back(m.gpu_meshdata);
                impl.render_textures.push_back(nullptr);
                impl.render_rims.push_back(m.is_selected ? 0xff : m.is_hovered ? 0x60 : 0x00);
            }
        }

        // add ground
        if (impl.show_ground) {
            float r = impl.scene_scale_factor * impl.ground_sphere_radius;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});

            auto const& mmtx = impl.render_xforms.emplace_back(scaler);
            impl.render_normal_matrices.push_back(mmtx);
            impl.render_colors.push_back(impl.ground_color);
            impl.render_meshes.push_back(impl.sphere_meshdata);
            impl.render_textures.push_back(nullptr);
            impl.render_rims.push_back(impl.ground_hovered ? 0x60 : 0x00);
        }

        // add bodies/frames
        if (impl.show_bofs) {
            float r = impl.scene_scale_factor * impl.bof_sphere_radius;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});

            for (Body_or_frame const& bf : impl.bofs) {
                auto const& mmtx = impl.render_xforms.emplace_back(glm::translate(glm::mat4{1.0f}, bf.pos) * scaler);
                impl.render_normal_matrices.push_back(normal_matrix(mmtx));
                impl.render_colors.push_back(bf.is_frame ? impl.frame_color : impl.body_color);
                impl.render_meshes.push_back(impl.sphere_meshdata);
                impl.render_textures.push_back(nullptr);
                impl.render_rims.push_back(bf.is_selected ? 0xff : bf.is_hovered ? 0x60 : 0x00);
            }
        }

        // upload instance stripes to drawlist
        {
            OSC_ASSERT(impl.render_xforms.size() == impl.render_normal_matrices.size());
            OSC_ASSERT(impl.render_normal_matrices.size() == impl.render_colors.size());
            OSC_ASSERT(impl.render_colors.size() == impl.render_meshes.size());
            OSC_ASSERT(impl.render_meshes.size() == impl.render_textures.size());
            OSC_ASSERT(impl.render_textures.size() == impl.render_rims.size());

            Drawlist_compiler_input inp;
            inp.ninstances = impl.render_xforms.size();
            inp.model_xforms = impl.render_xforms.data();
            inp.normal_xforms = impl.render_normal_matrices.data();
            inp.colors = impl.render_colors.data();
            inp.meshes = impl.render_meshes.data();
            inp.textures = impl.render_textures.data();
            inp.rim_intensity = impl.render_rims.data();

            upload_inputs_to_drawlist(inp, impl.drawlist);
        }

        // ensure render target dimensions match panel dimensions
        impl.renderer.set_dims({static_cast<int>(dims.x), static_cast<int>(dims.y)});
        impl.renderer.set_msxaa_samples(App::cur().get_samples());

        // ensure camera clipping planes are correct for current zoom level
        impl.camera.do_znear_zfar_autoscale();

        // compute render (ImGui::Image) position on the screen (needed by ImGuizmo + hittest)
        {
            impl.render_topleft_in_screen = ImGui::GetCursorScreenPos();
        }

        // update renderer view + projection matrices to match scene camera
        impl.renderparams.view_matrix = impl.camera.view_matrix();
        impl.renderparams.projection_matrix = impl.camera.projection_matrix(impl.renderer.aspect_ratio());

        // render the scene to a texture
        impl.renderer.render(impl.renderparams, impl.drawlist);

        // blit the texture in an ImGui::Image
        {
            gl::Texture_2d& tex = impl.renderer.output_texture();
            void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex.get()));
            ImVec2 image_dimensions{dims.x, dims.y};
            ImVec2 uv0{0.0f, 1.0f};
            ImVec2 uv1{1.0f, 0.0f};
            ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
            impl.mouse_over_render = ImGui::IsItemHovered();
        }

        perform_scene_hittest(impl);
    }

    // standard event handler for the 3D scene hover-over
    void handle_3dviewer_hovertest_standard_mode(Impl& impl) {

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
            Loaded_mesh& m = impl.meshes[static_cast<size_t>(htr.idx)];

            // set is_hovered
            m.is_hovered = true;

            // draw hover tooltip
            draw_mesh_hover_tooltip(impl, m);

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
                m.is_selected = true;
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
            Body_or_frame& bof = impl.bofs[static_cast<size_t>(htr.idx)];

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

    // handle renderer hovertest result when in assignment mode
    void handle_3dviewer_hovertest_assignment_mode(Impl& impl) {

        if (impl.assignment_st.assigner < 0) {
            return;  // nothing being assigned right now?
        }

        // get location of thing being assigned
        glm::vec3 assigner_loc;
        if (impl.assignment_st.is_body) {
            assigner_loc = center(impl.bofs[static_cast<size_t>(impl.assignment_st.assigner)]);
        } else {
            assigner_loc = center(impl.meshes[static_cast<size_t>(impl.assignment_st.assigner)]);
        }

        // reset all previous hover state
        set_is_hovered_of_all_to(impl, false);

        // get hovertest result from rendering step
        Hovertest_result const& htr = impl.hovertest_result;

        if (htr.type == ElType::Body) {
            // bof is hovered

            Body_or_frame& bof = impl.bofs[static_cast<size_t>(htr.idx)];
            ImDrawList& dl = *ImGui::GetForegroundDrawList();
            glm::vec3 c = center(bof);

            // draw a line between the thing being assigned and this body
            ImVec2 p1 = impl.render_topleft_in_screen + world2ndc(impl, assigner_loc);
            ImVec2 p2 = impl.render_topleft_in_screen + world2ndc(impl, c);
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            dl.AddLine(p1, p2, color);

            // if the user left-clicks, assign the hovered bof
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (impl.assignment_st.is_body) {
                    impl.bofs[impl.assignment_st.assigner].parent = htr.idx;
                } else {
                    impl.meshes[impl.assignment_st.assigner].parent = htr.idx;
                }

                // exit assignment state
                impl.assignment_st.assigner = -1;
            }
        } else if (htr.type == ElType::Ground) {
            // ground is hovered

            ImDrawList& dl = *ImGui::GetForegroundDrawList();
            glm::vec3 c = {0.0f, 0.0f, 0.0f};

            // draw a line between the thing being assigned and the hovered ground
            ImVec2 p1 = impl.render_topleft_in_screen + world2ndc(impl, assigner_loc);
            ImVec2 p2 = impl.render_topleft_in_screen + world2ndc(impl, c);
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            dl.AddLine(p1, p2, color);

            // if the user left-clicks, assign the ground
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (impl.assignment_st.is_body) {
                    impl.bofs[impl.assignment_st.assigner].parent = htr.idx;
                } else {
                    impl.meshes[impl.assignment_st.assigner].parent = htr.idx;
                }

                // exit assignment state
                impl.assignment_st.assigner = -1;
            }
        }
    }

    // draw context menu (if ImGui::OpenPopup has been called)
    //
    // this is the menu that pops up when the user right-clicks something
    //
    // BEWARE: try to do this last, because it can modify `Impl` (e.g. by
    // letting the user delete something)
    void draw_3dviewer_context_menu(Impl& impl) {
        if (ImGui::BeginPopup("contextmenu")) {
            switch (impl.ctx_menu.type) {
            case ElType::Mesh:
                draw_mesh_context_menu_content(impl, impl.meshes[static_cast<size_t>(impl.ctx_menu.idx)]);
                break;
            case ElType::Ground:
                draw_ground_context_menu_content(impl);
                break;
            case ElType::Body:
                draw_bof_context_menu_content(impl, impl.bofs[static_cast<size_t>(impl.ctx_menu.idx)]);
                break;
            default:
                break;
            }
            ImGui::EndPopup();
        }
    }

    // draw main 3D scene viewer
    //
    // this is the "normal" 3D viewer that's shown (i.e. the one that is
    // shown when the user isn't doing something specific, like assigning
    // bodies)
    void draw_3dviewer_standard_mode(Impl& impl) {
        // render main 3D scene
        draw_3dviewer_scene(impl);

        // handle any mousehover hits
        if (impl.mouse_over_render) {
            handle_3dviewer_hovertest_standard_mode(impl);
        }

        // draw 3D manipulation gizmos (the little user-moveable arrows etc.)
        draw_3dviewer_manipulation_gizmos(impl);

        // draw 2D overlay (lines between items, text, etc.)
        draw_2d_overlay(impl);

        // draw context menu
        //
        // CARE: this can mutate the implementation's data (e.g. by allowing
        // the user to delete things)
        draw_3dviewer_context_menu(impl);
    }

    // draw assignment 3D viewer
    //
    // this is a special 3D viewer that is presented that highlights, and
    // only allows the selection of, bodies/frames in the scene
    void draw_3dviewer_assignment_mode(Impl& impl) {

        Rgba32 old_assigned_mesh_color = impl.assigned_mesh_color;
        Rgba32 old_unassigned_mesh_color = impl.unassigned_mesh_color;

        impl.assigned_mesh_color.a = 0x10;
        impl.unassigned_mesh_color.a = 0x10;

        draw_3dviewer_scene(impl);

        if (impl.mouse_over_render) {
            handle_3dviewer_hovertest_assignment_mode(impl);
        }

        impl.assigned_mesh_color = old_assigned_mesh_color;
        impl.unassigned_mesh_color = old_unassigned_mesh_color;
    }

    // draw 3D viewer
    void draw_3dviewer(Impl& impl) {

        // the 3D viewer is modal. It can either be in:
        //
        // - standard mode
        // - assignment mode (assigning bodies to eachover)

        if (impl.assignment_st.assigner < 0) {
            draw_3dviewer_standard_mode(impl);
        } else {
            draw_3dviewer_assignment_mode(impl);
        }
    }

    // draw sidebar containing basic documentation and some action buttons
    void draw_sidebar(Impl& impl) {
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

        // draw top-level stats
        ImGui::Text("num meshes = %zu", impl.meshes.size());
        ImGui::Text("num bodies/frames = %zu", impl.bofs.size());
        ImGui::Text("assignment_st.assigner = %i", impl.assignment_st.assigner);
        ImGui::Text("assignment_st.is_body = %s", impl.assignment_st.is_body ? "true" : "false");
        ImGui::Text("FPS = %.1f", ImGui::GetIO().Framerate);

        // draw editors (checkboxes, sliders, etc.)
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

        ImGui::InputFloat("scene_scale_factor", &impl.scene_scale_factor);
        if (ImGui::Button("autoscale scene_scale_factor")) {
            action_autoscale_scene(impl);
        }

        // draw actions (buttons, etc.)
        if (ImGui::Button("add frame")) {
            action_add_frame(impl, {0.0f, 0.0f, 0.0f});
        }
        if (ImGui::Button("select all")) {
            set_is_selected_of_all_to(impl, true);
        }
        if (ImGui::Button("clear selection")) {
            set_is_hovered_of_all_to(impl, false);
        }
        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLUS "Import Meshes")) {
            prompt_user_to_select_multiple_mesh_files(impl);
        }
        ImGui::PopStyleColor();

        test_for_advancement_issues(impl);
        if (impl.advancement_issues.empty()) {
            // next step
            if (ImGui::Button("next >>")) {
                try_creating_output_model(impl);
            }
        }

        // draw error list
        if (!impl.advancement_issues.empty()) {
            ImGui::Text("issues (%zu):", impl.advancement_issues.size());
            ImGui::Separator();
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            for (std::string const& s : impl.advancement_issues) {
                ImGui::TextUnformatted(s.c_str());
            }
        }
    }

    // SCREEN DRAW: draw the screen
    void meshes2model_draw(Impl& impl) {

        // must be called at the start of each frame
        ImGuizmo::BeginFrame();

        // draw sidebar in a (moveable + resizeable) ImGui panel
        if (ImGui::Begin("wizardstep2sidebar")) {
            draw_sidebar(impl);
        }
        ImGui::End();

        // draw 3D viewer in a (moveable + resizeable) ImGui panel
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
        if (ImGui::Begin("wizardsstep2viewer")) {
            draw_3dviewer(impl);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // SCREEN TICK: update the screen's state
    void meshes2model_tick(Impl& impl) {

        pop_mesh_loader_output_queue(impl);
        update_camera_from_user_input(impl);
        update_impl_from_user_input(impl);

        // if a model was produced by this step then transition into the editor
        if (impl.output_model) {
            // TODO
            //auto mes = std::make_shared<Main_editor_state>();
            //mes->tabs.emplace_back(std::make_unique<Undoable_ui_model>(std::move(impl.output_model)));
            //mes->cur_tab = static_cast<int>(mes->tabs.size()) - 1;
            //Application::current().request_transition<Model_editor_screen>(mes);
        }
    }
}

// public API

osc::Meshes_to_model_wizard_screen::Meshes_to_model_wizard_screen() :
    impl{new Impl{}} {
}

osc::Meshes_to_model_wizard_screen::Meshes_to_model_wizard_screen(
        std::vector<std::filesystem::path> paths) :

    impl{new Impl{}} {

    for (std::filesystem::path const& p : paths) {
        submit_meshfile_load_request(*impl, p);
    }
}

osc::Meshes_to_model_wizard_screen::~Meshes_to_model_wizard_screen() noexcept = default;

void osc::Meshes_to_model_wizard_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Meshes_to_model_wizard_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Meshes_to_model_wizard_screen::on_event(SDL_Event const& e) {
    osc::ImGuiOnEvent(e);
}

void osc::Meshes_to_model_wizard_screen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);
    ::meshes2model_draw(*impl);
    osc::ImGuiRender();
}

void osc::Meshes_to_model_wizard_screen::tick(float) {
    ::meshes2model_tick(*impl);
}
