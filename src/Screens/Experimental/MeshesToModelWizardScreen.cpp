#include "MeshesToModelWizardScreen.hpp"

#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/MainEditorState.hpp"
#include "src/Styling.hpp"
#include "src/3D/BVH.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/InstancedRenderer.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/Texturing.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/Spsc.hpp"

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
    struct MeshLoadRequest final {
        // unique mesh ID
        int id;

        // filesystem path of the mesh
        std::filesystem::path filepath;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        // unique mesh ID
        int id;

        // filesystem path of the mesh
        std::filesystem::path filepath;

        // CPU-side mesh data
        CPUMesh um;

        // AABB of the mesh data
        AABB aabb;

        // bounding sphere of the mesh data
        Sphere boundingSphere;

        // triangle bvh of the mesh
        BVH triangleBVH;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        // unique mesh ID
        int id;

        // filesystem path of the mesh (that errored while loading)
        std::filesystem::path filepath;

        // the error
        std::string error;
    };

    // a response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // MESH LOADER FN: respond to load request (and handle errors)
    //
    // this typically runs on a background thread
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest const& msg) noexcept {
        try {
            MeshLoadOKResponse rv;
            rv.id = msg.id;
            rv.filepath = msg.filepath;
            rv.um = SimTKLoadMesh(msg.filepath);  // can throw
            rv.aabb = AABBFromVerts(rv.um.verts.data(), rv.um.verts.size());
            rv.boundingSphere = BoundingSphereFromVerts(rv.um.verts.data(), rv.um.verts.size());
            BVH_BuildFromTriangles(rv.triangleBVH, rv.um.verts.data(), rv.um.verts.size());
            return rv;
        } catch (std::exception const& ex) {
            return MeshLoadErrorResponse{msg.id, msg.filepath, ex.what()};
        }
    }

    // a meshloader is just a worker on a background thread that listens for requests
    using Mesh_loader = spsc::Worker<MeshLoadRequest, MeshLoadResponse, decltype(respondToMeshloadRequest)>;

    Mesh_loader createMeshloaderWorker() {
        return Mesh_loader::create(respondToMeshloadRequest);
    }

    // a fully-loaded mesh
    struct LoadedMesh final {
        // unique mesh ID
        int id;

        // filesystem path of the mesh
        std::filesystem::path filepath;

        // CPU-side mesh data
        CPUMesh meshdata;

        // AABB of the mesh data
        AABB aabb;

        // bounding sphere of the mesh data
        Sphere boundingSphere;

        // triangle BVH of mesh data
        BVH triangleBVH;

        // model matrix
        glm::mat4 modelMtx;

        // mesh data on GPU
        InstanceableMeshdata gpuMeshdata;

        // -1 if ground/unassigned; otherwise, a body/frame
        int parent;

        // true if the mesh is hovered by the user's mouse
        bool isHovered;

        // true if the mesh is selected
        bool isSelected;

        // create this by stealing from an OK background response
        explicit LoadedMesh(MeshLoadOKResponse&& tmp) :
            id{tmp.id},
            filepath{std::move(tmp.filepath)},
            meshdata{std::move(tmp.um)},
            aabb{tmp.aabb},
            boundingSphere{tmp.boundingSphere},
            triangleBVH{std::move(tmp.triangleBVH)},
            modelMtx{1.0f},
            gpuMeshdata{uploadMeshdataForInstancing(meshdata)},
            parent{-1},
            isHovered{false},
            isSelected{false} {
        }
    };

    // descriptive alias: they contain the same information
    using LoadingMesh = MeshLoadRequest;

    // a body or frame (bof) the user wants to add into the output model
    struct BodyOrFrame final {
        // -1 if ground/unassigned; otherwise, a body/frame
        int parent;

        // absolute position in space
        glm::vec3 pos;

        // true if it is a frame, rather than a body
        bool isFrame;

        // true if it is selected in the UI
        bool isSelected;

        // true if it is hovered in the UI
        bool isHovered;
    };

    // type of element in the scene
    enum class ElType { None, Mesh, Body, Ground };

    // state associated with an open context menu
    struct ContextMenuState final {
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
    struct ParentAssignmentState final {
        // index of assigner
        //
        // <0 (usually, -1) if this state is "inactive"
        int assigner = -1;

        // true if assigner is body; otherwise, assume the assigner is
        // a mesh
        bool isBody = false;
    };

    // result of 3D hovertest on the scene
    struct HovertestResult final {
        // index of the hovered element
        //
        // ignore if type == None or type == Ground
        int idx = -1;

        // type of hovered element
        ElType type = ElType::None;
    };

    // returns the worldspace center of a bof
    [[nodiscard]] constexpr glm::vec3 center(BodyOrFrame const& bof) noexcept {
        return bof.pos;
    }

    // returns the worldspace center of a loaded user mesh
    [[nodiscard]] glm::vec3 center(LoadedMesh const& lm) noexcept {
        glm::vec3 c = AABBCenter(lm.aabb);
        return glm::vec3{lm.modelMtx * glm::vec4{c, 1.0f}};
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

struct osc::MeshesToModelWizardScreen::Impl final {

    // loader that loads mesh data in a background thread
    Mesh_loader meshLoader = createMeshloaderWorker();

    // fully-loaded meshes
    std::vector<LoadedMesh> meshes;

    // not-yet-loaded meshes
    std::vector<LoadingMesh> loadingMeshes;

    // the bodies/frames that the user adds during this step
    std::vector<BodyOrFrame> bofs;

    // latest unique ID available
    //
    // this should be incremented whenever an entity in the scene
    // needs a fresh ID
    int latestId = 1;

    // set by draw step to render's topleft location in screenspace
    glm::vec2 renderTopleftInScreen = {0.0f, 0.0f};

    // color of assigned (i.e. attached to a body/frame) meshes
    // rendered in the 3D scene
    Rgba32 assignedMeshColor = Rgba32FromF4(1.0f, 1.0f, 1.0f, 1.0f);

    // color of unassigned meshes rendered in the 3D scene
    Rgba32 unassignedMeshColor = Rgba32FromU32(0xFFE4E4FF);

    // color of ground (sphere @ 0,0,0) rendered in the 3D scene
    Rgba32 groundColor = Rgba32FromF4(0.0f, 0.0f, 1.0f, 1.0f);

    // color of a body rendered in the 3D scene
    Rgba32 bodyColor = Rgba32FromF4(1.0f, 0.0f, 0.0f, 1.0f);

    // color of a frame rendered in the 3D scene
    Rgba32 frameColor = Rgba32FromF4(0.0f, 1.0f, 0.0f, 1.0f);

    // radius of rendered ground sphere
    float groundSphereRadius = 0.008f;

    // radius of rendered bof spheres
    float bofSphereRadius = 0.005f;

    // scale factor for all non-mesh, non-overlay scene elements (e.g.
    // the floor, bodies)
    //
    // this is necessary because some meshes can be extremely small/large and
    // scene elements need to be scaled accordingly (e.g. without this, a body
    // sphere end up being much larger than a mesh instance). Imagine if the
    // mesh was the leg of a fly, in meters.
    float sceneScaleFactor = 1.0f;

    // the transform matrix that the gizmo is manipulating (if active)
    //
    // this is set when the user initially starts interacting with a gizmo
    glm::mat4 gizmoMtx;

    // the transformation operation that the gizmo should be doing
    ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;

    // 3D rendering params
    osc::InstancedRendererParams renderParams;

    // floor data
    std::shared_ptr<gl::Texture2D> floorTex = std::make_shared<gl::Texture2D>(genChequeredFloorTexture());
    CPUMesh floorMesh = []() {
        CPUMesh rv = GenTexturedQuad();
        for (auto& uv : rv.texcoords) {
            uv *= 200.0f;
        }
        return rv;
    }();
    InstanceableMeshdata floorMeshdata = uploadMeshdataForInstancing(floorMesh);
    InstanceableMeshdata sphereMeshdata = uploadMeshdataForInstancing(GenUntexturedUVSphere(12, 12));

    // 3D rendering instance data
    std::vector<glm::mat4x3> renderXforms;
    std::vector<glm::mat3> renderNormalMatrices;
    std::vector<Rgba32> renderColors;
    std::vector<InstanceableMeshdata> renderMeshes;
    std::vector<std::shared_ptr<gl::Texture2D>> renderTextures;
    std::vector<unsigned char> renderRims;

    // 3D drawlist that is rendered
    osc::InstancedDrawlist drawlist;

    // 3D renderer for the drawlist
    osc::InstancedRenderer renderer;

    // 3D scene camera
    osc::PolarPerspectiveCamera camera;

    // context menu state
    //
    // values in this member are set when the menu is initially
    // opened by an ImGui::OpenPopup call
    ContextMenuState contextMenu;

    // hovertest result
    //
    // set by the implementation if it detects the mouse is over
    // an element in the scene
    HovertestResult hovertestResult;

    // parent assignment state
    //
    // values in this member are set when the user explicitly requests
    // that they want to assign a mesh/bof (i.e. when they want to
    // assign a parent)
    ParentAssignmentState assignmentState;

    // set to true by the implementation if mouse is over the 3D scene
    bool mouseOverRender = false;

    // set to true if the implementation thinks the user's mouse is over a gizmo
    bool mouseOverGizmo = false;

    // set to true by the implementation if ground (0, 0, 0) is hovered
    bool groundHovered = false;

    // true if a chequered floor should be drawn
    bool showFloor = true;

    // true if meshes should be drawn
    bool showMeshes = true;

    // true if ground should be drawn
    bool showGround = true;

    // true if bofs should be drawn
    bool showBofs = true;

    // true if all connection lines between entities should be
    // drawn, rather than just *hovered* entities
    bool showAllConnectionLines = false;

    // true if meshes should be drawn
    bool lockMeshes = false;

    // true if ground shouldn't be clickable in the 3D scene
    bool lockGround = false;

    // true if BOFs shouldn't be clickable in the 3D scene
    bool lockBOFs = false;

    // issues in this Impl that prevent the user from advancing
    // (and creating an OpenSim::Model, etc.)
    std::vector<std::string> advancementIssues;

    // model created by this wizard
    //
    // `nullptr` until the model is successfully created
    std::unique_ptr<OpenSim::Model> outputModel = nullptr;

    Impl() {
        camera.phi = fpi2;
    }
};
using Impl = osc::MeshesToModelWizardScreen::Impl;

// private Impl functions
namespace {

    // returns true if the bof is connected to ground - including whether it is
    // connected to ground *via* some other bodies
    //
    // returns false if it is connected to bullshit (i.e. an invalid index) or
    // to something that, itself, does not connect to ground (e.g. a cycle)
    [[nodiscard]] bool isBOFConnectedToGround(Impl& impl, int bofidx) {
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
    void testForAdvancementIssues(Impl& impl) {
        impl.advancementIssues.clear();

        // ensure all meshes are assigned to a valid body/ground
        for (LoadedMesh const& m : impl.meshes) {
            if (m.parent < 0) {
                // assigned to ground (bad practice)
                impl.advancementIssues.push_back("a mesh is assigned to ground (it should be assigned to a body/frame)");
            } else if (static_cast<size_t>(m.parent) >= impl.bofs.size()) {
                // invalid index
                impl.advancementIssues.push_back("a mesh is assigned to an invalid body");
            }
        }

        // ensure all bodies are connected to ground *eventually*
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            BodyOrFrame const& bof = impl.bofs[i];

            if (bof.parent < 0) {
                // ok: it's directly connected to ground
            } else if (static_cast<size_t>(bof.parent) >= impl.bofs.size()) {
                // bad: it's connected to non-existent bs
                impl.advancementIssues.push_back("a body/frame is connected to a non-existent body/frame");
            } else if (!isBOFConnectedToGround(impl, static_cast<int>(i))) {
                // bad: it's connected to something, but that thing doesn't connect to ground
                impl.advancementIssues.push_back("a body/frame is not connected to ground");
            } else {
                // ok: it's connected to a body/frame that is connected to ground
            }
        }
    }

    // recursively add a body/frame (bof) tree to an OpenSim::Model
    void recursivelyAddBOFToModel(
        Impl& impl,
        OpenSim::Model& model,
        int bofIndex,
        int parentIndex,
        OpenSim::PhysicalFrame& parentPhysFrame) {

        if (bofIndex < 0) {
            // the index points to ground and can't be traversed any more
            return;
        }

        // parent position in worldspace
        glm::vec3 worldParentPos = parentIndex < 0 ?
            glm::vec3{0.0f, 0.0f, 0.0f} :
            impl.bofs[static_cast<size_t>(parentIndex)].pos;

        // the body/frame to add in this step
        BodyOrFrame const& bof = impl.bofs[static_cast<size_t>(bofIndex)];

        OpenSim::PhysicalFrame* addedPhysFrame = nullptr;

        // create body/pof and add it into the model
        if (!bof.isFrame) {
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
            glm::vec3 worldBOFPos = bof.pos;

            {
                // figure out the parent's actual rotation, so that the relevant
                // vectors can be transformed into "parent space"
                model.finalizeFromProperties();
                model.finalizeConnections();
                SimTK::State s = model.initSystem();
                model.realizePosition(s);

                SimTK::Rotation rotateParentToWorld = parentPhysFrame.getRotationInGround(s);
                SimTK::Rotation rotateWorldToParent = rotateParentToWorld.invert();

                // compute relevant vectors in worldspace (the screen's coordinate system)
                SimTK::Vec3 worldParentToBOF = SimTKVec3FromV3(worldBOFPos - worldParentPos);
                SimTK::Vec3 worldBOFToParent = SimTKVec3FromV3(worldParentPos - worldBOFPos);
                SimTK::Vec3 worldBOFToParentDir = worldBOFToParent.normalize();

                SimTK::Vec3 parentBOFToParentDir = rotateWorldToParent * worldBOFToParentDir;
                SimTK::Vec3 parentY = {0.0f, 1.0f, 0.0f};  // by definition

                // create a "BOF space" that specifically points the Y axis
                // towards the parent frame (an OpenSim model building convention)
                SimTK::Transform parentToBOFMtx{
                    SimTK::Rotation{
                        glm::acos(SimTK::dot(parentY, parentBOFToParentDir)),
                        SimTK::cross(parentY, parentBOFToParentDir).normalize(),
                    },
                    SimTK::Vec3{rotateWorldToParent * worldParentToBOF},  // translation
                };
                pof->setOffsetTransform(parentToBOFMtx);
                pof->setParentFrame(parentPhysFrame);
            }

            // link everything up
            joint->addFrame(pof);
            joint->connectSocket_parent_frame(*pof);
            joint->connectSocket_child_frame(*body);

            // add it all to the model
            model.addBody(body);
            model.addJoint(joint);

            // attach geometry
            for (LoadedMesh const& m : impl.meshes) {
                if (m.parent != bofIndex) {
                    continue;
                }

                model.finalizeFromProperties();
                model.finalizeConnections();
                SimTK::State s = model.initSystem();
                model.realizePosition(s);

                SimTK::Transform parentToGroundMtx = body->getTransformInGround(s);
                SimTK::Transform groundToParentMtx = parentToGroundMtx.invert();

                // create a POF that attaches to the body
                //
                // this is necessary to independently transform the mesh relative
                // to the parent's transform (the mesh is currently transformed relative
                // to ground)
                OpenSim::PhysicalOffsetFrame* meshPhysOffsetFrame = new OpenSim::PhysicalOffsetFrame{};
                meshPhysOffsetFrame->setParentFrame(*body);

                // without setting `setObjectTransform`, the mesh will be subjected to
                // the POF's object-to-ground transform. so vertices in the matrix are
                // already in "object space" and we want to figure out how to transform
                // them as if they were in our current (world) space
                SimTK::Transform mmtx = SimTKTransformFromMat4x3(m.modelMtx);

                meshPhysOffsetFrame->setOffsetTransform(groundToParentMtx * mmtx);
                body->addComponent(meshPhysOffsetFrame);

                // attach mesh to the POF
                OpenSim::Mesh* mesh = new OpenSim::Mesh{m.filepath.string()};
                meshPhysOffsetFrame->attachGeometry(mesh);
            }

            // assign added_pf
            addedPhysFrame = body;
        } else {
            // user requested a Frame to be added

            auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            SimTK::Vec3 translation = SimTKVec3FromV3(bof.pos - worldParentPos);
            pof->set_translation(translation);
            pof->setParentFrame(parentPhysFrame);

            addedPhysFrame = pof.get();
            parentPhysFrame.addComponent(pof.release());
        }

        OSC_ASSERT(addedPhysFrame != nullptr);

        // RECURSE (depth-first)
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            BodyOrFrame const& b = impl.bofs[i];

            // if a body points to the current body/frame, recurse to it
            if (b.parent == bofIndex) {
                recursivelyAddBOFToModel(impl, model, static_cast<int>(i), bofIndex, *addedPhysFrame);
            }
        }
    }


    // tries to create an OpenSim::Model from the current screen state
    //
    // will test to ensure the current screen state is actually valid, though
    void tryCreatingOutputModel(Impl& impl) {
        testForAdvancementIssues(impl);
        if (!impl.advancementIssues.empty()) {
            log::error("cannot create an osim model: advancement issues detected");
            return;
        }

        std::unique_ptr<OpenSim::Model> m = std::make_unique<OpenSim::Model>();
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            BodyOrFrame const& bof = impl.bofs[i];
            if (bof.parent == -1) {
                // it's a bof that is directly connected to Ground
                int bofIndex = static_cast<int>(i);
                int groundIndex = -1;
                recursivelyAddBOFToModel(impl, *m, bofIndex, groundIndex, m->updGround());
            }
        }

        // all done: assign model
        impl.outputModel = std::move(m);
    }

    // sets `is_selected` of all selectable entities in the scene
    void setIsSelectedOfAll(Impl& impl, bool v) {
        for (LoadedMesh& m : impl.meshes) {
            m.isSelected = v;
        }
        for (BodyOrFrame& bof : impl.bofs) {
            bof.isSelected = v;
        }
    }

    // sets `is_hovered` of all hoverable entities in the scene
    void setIsHoveredOfAll(Impl& impl, bool v) {
        for (LoadedMesh& m : impl.meshes) {
            m.isHovered = v;
        }
        impl.groundHovered = v;
        for (BodyOrFrame& bof : impl.bofs) {
            bof.isHovered = v;
        }
    }

    // sets all hovered elements as selected elements
    //
    // (and all not-hovered elements as not selected)
    void setCurrentlyHoveredElsAsSelected(Impl& impl) {
        for (LoadedMesh& m : impl.meshes) {
            m.isSelected = m.isHovered;
        }
        for (BodyOrFrame& bof : impl.bofs) {
            bof.isSelected = bof.isHovered;
        }
    }

    // submit a meshfile load request to the mesh loader
    void submitMeshfileLoadRequestToMeshloader(Impl& impl, std::filesystem::path path) {
        int id = impl.latestId++;
        MeshLoadRequest req{id, std::move(path)};
        impl.loadingMeshes.push_back(req);
        impl.meshLoader.send(req);
    }

    // synchronously prompts the user to select multiple mesh files through
    // a native OS file dialog
    void promptUserToSelectMultipleMeshFiles(Impl& impl) {
        nfdpathset_t s{};
        nfdresult_t result = NFD_OpenDialogMultiple("obj,vtp,stl", nullptr, &s);

        if (result == NFD_OKAY) {
            OSC_SCOPE_GUARD({ NFD_PathSet_Free(&s); });

            size_t len = NFD_PathSet_GetCount(&s);
            for (size_t i = 0; i < len; ++i) {
                submitMeshfileLoadRequestToMeshloader(impl, NFD_PathSet_GetPath(&s, i));
            }
        } else if (result == NFD_CANCEL) {
            // do nothing: the user cancelled
        } else {
            log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
        }
    }

    // update the scene's camera based on (ImGui's) user input
    void UpdatePolarCameraFromImGuiUserInput(Impl& impl) {

        if (!impl.mouseOverRender) {
            // ignore mouse if it isn't over the render
            return;
        }

        if (ImGuizmo::IsUsing()) {
            // ignore mouse if user currently dragging a gizmo
            return;
        }

        UpdatePolarCameraFromImGuiUserInput(impl.renderer.getDimsf(), impl.camera);
    }

    // delete all selected elements
    void actionDeleteSelectedEls(Impl& impl) {

        // nothing refers to meshes, so they can be removed straightforwardly
        auto& meshes = impl.meshes;
        osc::RemoveErase(meshes, [](auto const& m) { return m.isSelected; });

        // bodies/frames, and meshes, can refer to other bodies/frames (they're a tree)
        // so deletion needs to update the `assigned_body` and `parent` fields of every
        // other body/frame/mesh to be correct post-deletion

        auto& bofs = impl.bofs;

        // perform parent/assigned_body fixups
        //
        // collect a list of to-be-deleted indices, going from big to small
        std::vector<int> deletedIndices;
        for (int i = static_cast<int>(bofs.size()) - 1; i >= 0; --i) {
            BodyOrFrame& b = bofs[i];
            if (b.isSelected) {
                deletedIndices.push_back(static_cast<int>(i));
            }
        }

        // for each index in the (big to small) list, fixup the entries
        // to point to a fixed-up location
        //
        // the reason it needs to be big-to-small is to prevent the sitation
        // where decrementing an index makes it point at a location that appears
        // to be equal to a to-be-deleted location
        for (int idx : deletedIndices) {
            for (BodyOrFrame& b : bofs) {
                if (b.parent == idx) {
                    b.parent = bofs[static_cast<size_t>(idx)].parent;
                }
                if (b.parent > idx) {
                    --b.parent;
                }
            }

            for (LoadedMesh& m : meshes) {
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
        osc::RemoveErase(bofs, [](auto const& b) { return b.isSelected; });
    }

    // add frame to model
    void actionAddFrame(Impl& impl, glm::vec3 pos) {
        setIsSelectedOfAll(impl, false);

        BodyOrFrame& bf = impl.bofs.emplace_back();
        bf.parent = -1;
        bf.pos = pos;
        bf.isFrame = true;
        bf.isSelected = true;
        bf.isHovered = false;
    }

    // add body to model
    void actionAddBody(Impl& impl, glm::vec3 pos) {
        setIsHoveredOfAll(impl, false);

        BodyOrFrame& bf = impl.bofs.emplace_back();
        bf.parent = -1;
        bf.pos = pos;
        bf.isFrame = false;
        bf.isSelected = true;
        bf.isHovered = false;
    }

    void actionSetCameraFocus(Impl& impl, LoadedMesh const& lum) {
        impl.camera.focusPoint = AABBCenter(lum.aabb);
    }

    void actionAutoscaleScene(Impl& impl) {
        if (impl.meshes.empty()) {
            impl.sceneScaleFactor = 1.0f;
            return;
        }

        AABB sceneAABB{{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        for (LoadedMesh const& mesh : impl.meshes) {
            sceneAABB = AABBUnion(sceneAABB, mesh.aabb);
        }

        // we only care about the dimensions of the AABB, not its position
        glm::vec3 dims = AABBDims(sceneAABB);

        // figure out the longest dimension, scale relative to that
        float longestDim = VecLongestDimVal(dims);

        // update relevant state
        impl.sceneScaleFactor = 5.0f * longestDim;
        impl.camera.focusPoint = {};
        impl.camera.radius = 5.0f * longestDim;
    }

    // polls the mesh loader's output queue for any newly-loaded meshes and pushes
    // them into the screen's state
    void popMeshloaderOutputQueue(Impl& impl) {

        // pop anything from the mesh loader's output queue
        for (auto maybeResponse = impl.meshLoader.poll();
             maybeResponse.has_value();
             maybeResponse = impl.meshLoader.poll()) {

            MeshLoadResponse& resp = *maybeResponse;

            if (std::holds_alternative<MeshLoadOKResponse>(resp)) {
                // handle OK message from loader

                MeshLoadOKResponse& ok = std::get<MeshLoadOKResponse>(resp);

                // remove it from the "currently loading" list
                osc::RemoveErase(impl.loadingMeshes, [id = ok.id](auto const& m) { return m.id == id; });

                // add it to the "loaded" mesh list
                impl.meshes.emplace_back(std::move(ok));
            } else {
                // handle ERROR message from loader

                MeshLoadErrorResponse& err = std::get<MeshLoadErrorResponse>(resp);

                // remove it from the "currently loading" list
                osc::RemoveErase(impl.loadingMeshes, [id = err.id](auto const& m) { return m.id == id; });

                // log the error (it's the best we can do for now)
                log::error("%s: error loading mesh file: %s", err.filepath.string().c_str(), err.error.c_str());
            }
        }
    }

    // update the screen state (impl) based on (ImGui's) user input
    void updateImplFromUserInput(Impl& impl) {

        // DELETE: delete any selected elements
        if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
            actionDeleteSelectedEls(impl);
        }

        // B: add body to hovered element
        if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
            setIsSelectedOfAll(impl, false);

            for (auto const& b : impl.bofs) {
                if (b.isHovered) {
                    actionAddBody(impl, b.pos);
                    return;
                }
            }
            if (impl.groundHovered) {
                actionAddBody(impl, {0.0f, 0.0f, 0.0f});
                return;
            }
            for (auto const& m : impl.meshes) {
                if (m.isHovered) {
                    actionAddBody(impl, center(m));
                    return;
                }
            }
        }

        // A: assign a parent for the hovered element
        if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
            impl.assignmentState.assigner = -1;

            for (size_t i = 0; i < impl.bofs.size(); ++i) {
                BodyOrFrame const& bof = impl.bofs[i];
                if (bof.isHovered) {
                    impl.assignmentState.assigner = static_cast<int>(i);
                    impl.assignmentState.isBody = true;
                }
            }
            for (size_t i = 0; i < impl.meshes.size(); ++i) {
                LoadedMesh const& m = impl.meshes[i];
                if (m.isHovered) {
                    impl.assignmentState.assigner = static_cast<int>(i);
                    impl.assignmentState.isBody = false;
                }
            }

            if (impl.assignmentState.assigner != -1) {
                setCurrentlyHoveredElsAsSelected(impl);
            }
        }

        // ESC: leave assignment state
        if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
            impl.assignmentState.assigner = -1;
        }

        // CTRL+A: select all
        if ((ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
            setIsSelectedOfAll(impl, true);
        }

        // S: set manipulation mode to "scale"
        if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
            impl.gizmoOp = ImGuizmo::SCALE;
        }

        // R: set manipulation mode to "rotate"
        if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
            impl.gizmoOp = ImGuizmo::ROTATE;
        }

        // G: set manipulation mode to "grab" (translate)
        if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
            impl.gizmoOp = ImGuizmo::TRANSLATE;
        }
    }

    // convert a 3D worldspace coordinate into a 2D screenspace (NDC) coordinate
    //
    // used to draw 2D overlays for items that are in 3D
    glm::vec2 worldCoordToScreenNDC(Impl& impl, glm::vec3 const& v) {
        glm::mat4 const& view = impl.renderParams.viewMtx;
        glm::mat4 const& persp = impl.renderParams.projMtx;

        // range: [-1,+1] for XY
        glm::vec4 clipspacePos = persp * view * glm::vec4{v, 1.0f};

        // perspective division: 4D (affine) --> 3D
        clipspacePos /= clipspacePos.w;

        // range [0, +1] with Y starting in top-left
        glm::vec2 relativeScreenpos{
            (clipspacePos.x + 1.0f)/2.0f,
            -1.0f * ((clipspacePos.y - 1.0f)/2.0f),
        };

        // current screen dimensions
        glm::vec2 screenDimensions = impl.renderer.getDimsf();

        // range [0, w] (X) and [0, h] (Y)
        glm::vec2 screenPos = screenDimensions * relativeScreenpos;

        return screenPos;
    }

    // draw a 2D overlay line between a BOF and its parent
    void drawBOFLineToParent(Impl& impl, BodyOrFrame const& bof) {
        ImDrawList& dl = *ImGui::GetForegroundDrawList();

        glm::vec3 parentPos;
        if (bof.parent < 0) {
            // its parent is "ground": use ground pos
            parentPos = {0.0f, 0.0f, 0.0f};
        } else {
            BodyOrFrame const& parent = impl.bofs.at(static_cast<size_t>(bof.parent));
            parentPos = parent.pos;
        }

        auto screenTopLeft = impl.renderTopleftInScreen;

        ImVec2 p1 = screenTopLeft + worldCoordToScreenNDC(impl, bof.pos);
        ImVec2 p2 = screenTopLeft + worldCoordToScreenNDC(impl, parentPos);
        ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

        dl.AddLine(p1, p2, color);
    }

    // draw 2D overlay (dotted lines between bodies, etc.)
    void draw2DOverlay(Impl& impl) {

        // draw connection lines between BOFs
        for (size_t i = 0; i < impl.bofs.size(); ++i) {
            BodyOrFrame const& bof = impl.bofs[i];

            // only draw connection lines if "draw all connection lines" is
            // enabled, or if this bof in particular is hovered
            if (!(impl.showAllConnectionLines || bof.isHovered)) {
                continue;
            }

            // draw line from bof to its parent (bof/ground)
            drawBOFLineToParent(impl, bof);

            // draw line(s) from any other bofs connected to this one
            for (BodyOrFrame const& b : impl.bofs) {
                if (b.parent == static_cast<int>(i)) {
                    drawBOFLineToParent(impl, b);
                }
            }
        }
    }

    // draw hover tooltip when hovering over a user mesh
    void drawMeshHoverTooltip(Impl&, LoadedMesh& m) {
        ImGui::BeginTooltip();
        ImGui::Text("filepath = %s", m.filepath.string().c_str());
        ImGui::TextUnformatted(m.parent >= 0 ? "ASSIGNED" : "UNASSIGNED (to a body/frame)");

        ImGui::Text("id = %i", m.id);
        ImGui::Text("filename = %s", m.filepath.filename().string().c_str());
        ImGui::Text("is_hovered = %s", m.isHovered ? "true" : "false");
        ImGui::Text("is_selected = %s", m.isSelected ? "true" : "false");
        ImGui::Text("verts = %zu", m.meshdata.verts.size());
        ImGui::Text("elements = %zu", m.meshdata.indices.size());

        ImGui::Text("AABB.p1 = (%.2f, %.2f, %.2f)", m.aabb.min.x, m.aabb.min.y, m.aabb.min.z);
        ImGui::Text("AABB.p2 = (%.2f, %.2f, %.2f)", m.aabb.max.x, m.aabb.max.y, m.aabb.max.z);
        glm::vec3 center = (m.aabb.min + m.aabb.max) / 2.0f;
        ImGui::Text("center(AABB) = (%.2f, %.2f, %.2f)", center.x, center.y, center.z);

        ImGui::Text("sphere = O(%.2f, %.2f, %.2f), r(%.2f)",
                    m.boundingSphere.origin.x,
                    m.boundingSphere.origin.y,
                    m.boundingSphere.origin.z,
                    m.boundingSphere.radius);
        ImGui::EndTooltip();
    }

    // draw hover tooltip when hovering over Ground
    void drawGroundHoverTooltip(Impl&) {
        ImGui::BeginTooltip();
        ImGui::Text("Ground");
        ImGui::Text("(always present, and defined to be at (0, 0, 0))");
        ImGui::EndTooltip();
    }

    // draw hover tooltip when hovering over a BOF
    void drawBOFHoverTooltip(Impl&, BodyOrFrame& bof) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(bof.isFrame ? "Frame" : "Body");
        ImGui::TextUnformatted(bof.parent >= 0 ? "Connected to another body/frame" : "Connected to ground");
        ImGui::EndTooltip();
    }

    // draw mesh context menu
    void drawMeshContextMenuContent(Impl& impl, LoadedMesh& lum) {
        if (ImGui::MenuItem("add body")) {
            actionAddBody(impl, center(lum));
        }
        if (ImGui::MenuItem("add frame")) {
            actionAddFrame(impl, center(lum));
        }
        if (ImGui::MenuItem("center camera on this mesh")) {
            actionSetCameraFocus(impl, lum);
        }
    }

    // draw ground context menu
    void drawGroundContextMenuContent(Impl& impl) {
        if (ImGui::MenuItem("add body")) {
            actionAddBody(impl, {0.0f, 0.0f, 0.0f});
        }
        if (ImGui::MenuItem("add frame")) {
            actionAddFrame(impl, {0.0f, 0.0f, 0.0f});
        }
    }

    // draw bof context menu
    void drawBOFContextMenuContent(Impl&, BodyOrFrame&) {
        ImGui::Text("(no actions available)");
    }

    // draw manipulation gizmos (the little handles that the user can click
    // to move things in 3D)
    void draw3DViewerManipulationGizmos(Impl& impl) {

        // if the user isn't manipulating anything, create an up-to-date
        // manipulation matrix
        if (!ImGuizmo::IsUsing()) {
            // only draw gizmos if this is >0
            int nselected = 0;

            AABB aabb{{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};

            for (LoadedMesh const& m : impl.meshes) {
                if (m.isSelected) {
                    ++nselected;

                    aabb = AABBUnion(aabb, AABBApplyXform(m.aabb, m.modelMtx));
                }
            }
            for (BodyOrFrame const& b : impl.bofs) {
                if (b.isSelected) {
                    ++nselected;
                    aabb = AABBUnion(aabb, AABB{b.pos, b.pos});
                }
            }

            if (nselected == 0) {
                return;  // early exit: nothing's selected
            }

            glm::vec3 center = AABBCenter(aabb);
            impl.gizmoMtx = glm::translate(glm::mat4{1.0f}, center);
        }

        // else: is using OR nselected > 0 (so draw it)

        ImGuizmo::SetRect(
            impl.renderTopleftInScreen.x,
            impl.renderTopleftInScreen.y,
            impl.renderer.getDimsf().x,
            impl.renderer.getDimsf().y);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        glm::mat4 delta;
        bool manipulated = ImGuizmo::Manipulate(
            glm::value_ptr(impl.renderParams.viewMtx),
            glm::value_ptr(impl.renderParams.projMtx),
            impl.gizmoOp,
            ImGuizmo::WORLD,
            glm::value_ptr(impl.gizmoMtx),
            glm::value_ptr(delta),
            nullptr,
            nullptr,
            nullptr);

        if (!manipulated) {
            return;
        }
        // else: apply manipulation

        // update relevant positions/model matrices
        for (LoadedMesh& m : impl.meshes) {
            if (m.isSelected) {
                m.modelMtx = delta * m.modelMtx;
            }
        }
        for (BodyOrFrame& b : impl.bofs) {
            if (b.isSelected) {
                b.pos = glm::vec3{delta * glm::vec4{b.pos, 1.0f}};
            }
        }
    }

    glm::mat4x3 createFloorMMtx(Impl& impl) {
        glm::mat4 rv{1.0f};

        // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
        // floor down *slightly* to prevent Z fighting from planes rendered from the
        // model itself (the contact planes, etc.)
        rv = glm::translate(rv, {0.0f, -0.0001f, 0.0f});
        rv = glm::rotate(rv, fpi2, {-1.0, 0.0, 0.0});
        rv = glm::scale(rv, {impl.sceneScaleFactor * 100.0f, impl.sceneScaleFactor * 100.0f, 1.0f});

        return rv;
    }

    void performSceneHittest(Impl& impl) {
        glm::vec2 mouseposInRender = glm::vec2{ImGui::GetMousePos()} - impl.renderTopleftInScreen;
        Line cameraRay =
            impl.camera.unprojectScreenposToWorldRay(mouseposInRender, impl.renderer.getDimsf());

        // assumed result if nothing is hovered
        impl.hovertestResult.idx = -1;
        impl.hovertestResult.type = ElType::None;
        float closest = std::numeric_limits<float>::max();

        // perform ray-sphere hittest on the ground sphere
        if (!impl.lockGround) {
            Sphere groundSphere{{0.0f, 0.0f, 0.0f}, impl.sceneScaleFactor * impl.groundSphereRadius};
            auto res = GetRayCollisionSphere(cameraRay, groundSphere);
            if (res.hit && res.distance < closest) {
                closest = res.distance;
                impl.hovertestResult.idx = -1;
                impl.hovertestResult.type = ElType::Ground;
            }
        }

        // perform ray-sphere hittest on the bof spheres
        if (!impl.lockBOFs) {
            float r = impl.sceneScaleFactor * impl.bofSphereRadius;
            for (size_t i = 0; i < impl.bofs.size(); ++i) {
                BodyOrFrame const& bof = impl.bofs[i];
                Sphere bofSphere{bof.pos, r};
                auto res = GetRayCollisionSphere(cameraRay, bofSphere);
                if (res.hit && res.distance < closest) {
                    closest = res.distance;
                    impl.hovertestResult.idx = static_cast<int>(i);
                    impl.hovertestResult.type = ElType::Body;
                }
            }
        }

        // perform a *rough* ray-AABB hittest on the meshes - if there's
        // a hit, perform a more precise (BVH-accelerated) ray-triangle test
        // on the mesh
        if (!impl.lockMeshes) {
            for (size_t i = 0; i < impl.meshes.size(); ++i) {
                LoadedMesh const& mesh = impl.meshes[i];
                AABB aabb = AABBApplyXform(mesh.aabb, mesh.modelMtx);
                auto res = GetRayCollisionAABB(cameraRay, aabb);

                if (res.hit && res.distance < closest) {
                    // got a ray-AABB hit, now see if the ray hits a triangle in the mesh
                    Line cameraRayModelspace = LineApplyXform(cameraRay, glm::inverse(mesh.modelMtx));

                    BVHCollision collision;
                    if (BVH_GetClosestRayTriangleCollision(mesh.triangleBVH, mesh.meshdata.verts.data(), mesh.meshdata.verts.size(), cameraRayModelspace, &collision)) {
                        if (collision.distance < closest) {
                            closest = collision.distance;
                            impl.hovertestResult.idx = static_cast<int>(i);
                            impl.hovertestResult.type = ElType::Mesh;
                        }
                    }
                }
            }
        }
    }

    // draw 3D scene into remainder of the ImGui panel's content region
    void draw3DViewerScene(Impl& impl) {
        ImVec2 dims = ImGui::GetContentRegionAvail();

        // skip rendering if the panel would be too small
        if (dims.x < 1.0f || dims.y < 1.0f) {
            return;
        }

        // populate drawlist

        // clear instance buffers
        impl.renderXforms.clear();
        impl.renderNormalMatrices.clear();
        impl.renderColors.clear();
        impl.renderMeshes.clear();
        impl.renderTextures.clear();
        impl.renderRims.clear();

        // add floor
        if (impl.showFloor) {
            auto const& mmtx = impl.renderXforms.emplace_back(createFloorMMtx(impl));
            impl.renderNormalMatrices.push_back(NormalMatrix(mmtx));
            impl.renderColors.push_back({});
            impl.renderMeshes.push_back(impl.floorMeshdata);
            impl.renderTextures.push_back(impl.floorTex);
            impl.renderRims.push_back(0x00);
        }

        // add meshes
        if (impl.showMeshes) {
            for (LoadedMesh const& m : impl.meshes) {
                auto const& mmtx = impl.renderXforms.emplace_back(m.modelMtx);
                impl.renderNormalMatrices.push_back(NormalMatrix(mmtx));
                impl.renderColors.push_back(m.parent >= 0 ? impl.assignedMeshColor : impl.unassignedMeshColor);
                impl.renderMeshes.push_back(m.gpuMeshdata);
                impl.renderTextures.push_back(nullptr);
                impl.renderRims.push_back(m.isSelected ? 0xff : m.isHovered ? 0x60 : 0x00);
            }
        }

        // add ground
        if (impl.showGround) {
            float r = impl.sceneScaleFactor * impl.groundSphereRadius;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});

            auto const& mmtx = impl.renderXforms.emplace_back(scaler);
            impl.renderNormalMatrices.push_back(mmtx);
            impl.renderColors.push_back(impl.groundColor);
            impl.renderMeshes.push_back(impl.sphereMeshdata);
            impl.renderTextures.push_back(nullptr);
            impl.renderRims.push_back(impl.groundHovered ? 0x60 : 0x00);
        }

        // add bodies/frames
        if (impl.showBofs) {
            float r = impl.sceneScaleFactor * impl.bofSphereRadius;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {r, r, r});

            for (BodyOrFrame const& bf : impl.bofs) {
                auto const& mmtx = impl.renderXforms.emplace_back(glm::translate(glm::mat4{1.0f}, bf.pos) * scaler);
                impl.renderNormalMatrices.push_back(NormalMatrix(mmtx));
                impl.renderColors.push_back(bf.isFrame ? impl.frameColor : impl.bodyColor);
                impl.renderMeshes.push_back(impl.sphereMeshdata);
                impl.renderTextures.push_back(nullptr);
                impl.renderRims.push_back(bf.isSelected ? 0xff : bf.isHovered ? 0x60 : 0x00);
            }
        }

        // upload instance stripes to drawlist
        {
            OSC_ASSERT(impl.renderXforms.size() == impl.renderNormalMatrices.size());
            OSC_ASSERT(impl.renderNormalMatrices.size() == impl.renderColors.size());
            OSC_ASSERT(impl.renderColors.size() == impl.renderMeshes.size());
            OSC_ASSERT(impl.renderMeshes.size() == impl.renderTextures.size());
            OSC_ASSERT(impl.renderTextures.size() == impl.renderRims.size());

            DrawlistCompilerInput inp;
            inp.ninstances = impl.renderXforms.size();
            inp.modelMtxs = impl.renderXforms.data();
            inp.normalMtxs = impl.renderNormalMatrices.data();
            inp.colors = impl.renderColors.data();
            inp.meshes = impl.renderMeshes.data();
            inp.textures = impl.renderTextures.data();
            inp.rimIntensities = impl.renderRims.data();

            uploadInputsToDrawlist(inp, impl.drawlist);
        }

        // ensure render target dimensions match panel dimensions
        impl.renderer.setDims({static_cast<int>(dims.x), static_cast<int>(dims.y)});
        impl.renderer.setMsxaaSamples(App::cur().getSamples());

        // ensure camera clipping planes are correct for current zoom level
        impl.camera.rescaleZNearAndZFarBasedOnRadius();

        // compute render (ImGui::Image) position on the screen (needed by ImGuizmo + hittest)
        {
            impl.renderTopleftInScreen = ImGui::GetCursorScreenPos();
        }

        // update renderer view + projection matrices to match scene camera
        impl.renderParams.viewMtx = impl.camera.getViewMtx();
        impl.renderParams.projMtx = impl.camera.getProjMtx(impl.renderer.getAspectRatio());

        // render the scene to a texture
        impl.renderer.render(impl.renderParams, impl.drawlist);

        // blit the texture in an ImGui::Image
        {
            gl::Texture2D& tex = impl.renderer.getOutputTexture();
            void* textureHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex.get()));
            ImVec2 image_dimensions{dims.x, dims.y};
            ImVec2 uv0{0.0f, 1.0f};
            ImVec2 uv1{1.0f, 0.0f};
            ImGui::Image(textureHandle, image_dimensions, uv0, uv1);
            impl.mouseOverRender = ImGui::IsItemHovered();
        }

        performSceneHittest(impl);
    }

    // standard event handler for the 3D scene hover-over
    void handle3DViewerHovertestStandardMode(Impl& impl) {

        // reset all previous hover state
        setIsHoveredOfAll(impl, false);

        // this is set by the renderer
        HovertestResult const& htr = impl.hovertestResult;

        if (htr.type == ElType::None) {
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                !ImGuizmo::IsUsing() &&
                !(ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT))) {

                setIsSelectedOfAll(impl, false);
            }
        } else if (htr.type == ElType::Mesh) {
            LoadedMesh& m = impl.meshes[static_cast<size_t>(htr.idx)];

            // set is_hovered
            m.isHovered = true;

            // draw hover tooltip
            drawMeshHoverTooltip(impl, m);

            // open context menu (if applicable)
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.contextMenu.idx = htr.idx;
                impl.contextMenu.type = ElType::Mesh;
                ImGui::OpenPopup("contextmenu");
            }

            // if left-clicked, select it
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing()) {
                // de-select everything if shift isn't down
                if (!(ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT))) {
                    setIsSelectedOfAll(impl, false);
                }

                // set clicked item as selected
                m.isSelected = true;
            }
        } else if (htr.type == ElType::Ground) {
            // set ground_hovered
            impl.groundHovered = true;

            // draw hover tooltip
            drawGroundHoverTooltip(impl);

            // open context menu (if applicable)
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.contextMenu.idx = -1;
                impl.contextMenu.type = ElType::Ground;
                ImGui::OpenPopup("contextmenu");
            }
        } else if (htr.type == ElType::Body) {
            BodyOrFrame& bof = impl.bofs[static_cast<size_t>(htr.idx)];

            // set is_hovered
            bof.isHovered = true;

            // draw hover tooltip
            drawBOFHoverTooltip(impl, bof);

            // open context menu (if applicable)
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.contextMenu.idx = htr.idx;
                impl.contextMenu.type = ElType::Body;
                ImGui::OpenPopup("contextmenu");
            }

            // if left-clicked, select it
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing()) {
                // de-select everything if shift isn't down
                if (!(ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT))) {
                    setIsSelectedOfAll(impl, false);
                }

                // set clicked item as selected
                bof.isSelected = true;
            }
        }
    }

    // handle renderer hovertest result when in assignment mode
    void handle3DViewerHovertestAssignmentMode(Impl& impl) {

        if (impl.assignmentState.assigner < 0) {
            return;  // nothing being assigned right now?
        }

        // get location of thing being assigned
        glm::vec3 assignerLoc;
        if (impl.assignmentState.isBody) {
            assignerLoc = center(impl.bofs[static_cast<size_t>(impl.assignmentState.assigner)]);
        } else {
            assignerLoc = center(impl.meshes[static_cast<size_t>(impl.assignmentState.assigner)]);
        }

        // reset all previous hover state
        setIsHoveredOfAll(impl, false);

        // get hovertest result from rendering step
        HovertestResult const& htr = impl.hovertestResult;

        if (htr.type == ElType::Body) {
            // bof is hovered

            BodyOrFrame& bof = impl.bofs[static_cast<size_t>(htr.idx)];
            ImDrawList& dl = *ImGui::GetForegroundDrawList();
            glm::vec3 c = center(bof);

            // draw a line between the thing being assigned and this body
            ImVec2 p1 = impl.renderTopleftInScreen + worldCoordToScreenNDC(impl, assignerLoc);
            ImVec2 p2 = impl.renderTopleftInScreen + worldCoordToScreenNDC(impl, c);
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            dl.AddLine(p1, p2, color);

            // if the user left-clicks, assign the hovered bof
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (impl.assignmentState.isBody) {
                    impl.bofs[impl.assignmentState.assigner].parent = htr.idx;
                } else {
                    impl.meshes[impl.assignmentState.assigner].parent = htr.idx;
                }

                // exit assignment state
                impl.assignmentState.assigner = -1;
            }
        } else if (htr.type == ElType::Ground) {
            // ground is hovered

            ImDrawList& dl = *ImGui::GetForegroundDrawList();
            glm::vec3 c = {0.0f, 0.0f, 0.0f};

            // draw a line between the thing being assigned and the hovered ground
            ImVec2 p1 = impl.renderTopleftInScreen + worldCoordToScreenNDC(impl, assignerLoc);
            ImVec2 p2 = impl.renderTopleftInScreen + worldCoordToScreenNDC(impl, c);
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            dl.AddLine(p1, p2, color);

            // if the user left-clicks, assign the ground
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (impl.assignmentState.isBody) {
                    impl.bofs[impl.assignmentState.assigner].parent = htr.idx;
                } else {
                    impl.meshes[impl.assignmentState.assigner].parent = htr.idx;
                }

                // exit assignment state
                impl.assignmentState.assigner = -1;
            }
        }
    }

    // draw context menu (if ImGui::OpenPopup has been called)
    //
    // this is the menu that pops up when the user right-clicks something
    //
    // BEWARE: try to do this last, because it can modify `Impl` (e.g. by
    // letting the user delete something)
    void draw3DViewerContextMenu(Impl& impl) {
        if (ImGui::BeginPopup("contextmenu")) {
            switch (impl.contextMenu.type) {
            case ElType::Mesh:
                drawMeshContextMenuContent(impl, impl.meshes[static_cast<size_t>(impl.contextMenu.idx)]);
                break;
            case ElType::Ground:
                drawGroundContextMenuContent(impl);
                break;
            case ElType::Body:
                drawBOFContextMenuContent(impl, impl.bofs[static_cast<size_t>(impl.contextMenu.idx)]);
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
    void draw3DViewerStandardMode(Impl& impl) {
        // render main 3D scene
        draw3DViewerScene(impl);

        // handle any mousehover hits
        if (impl.mouseOverRender) {
            handle3DViewerHovertestStandardMode(impl);
        }

        // draw 3D manipulation gizmos (the little user-moveable arrows etc.)
        draw3DViewerManipulationGizmos(impl);

        // draw 2D overlay (lines between items, text, etc.)
        draw2DOverlay(impl);

        // draw context menu
        //
        // CARE: this can mutate the implementation's data (e.g. by allowing
        // the user to delete things)
        draw3DViewerContextMenu(impl);
    }

    // draw assignment 3D viewer
    //
    // this is a special 3D viewer that is presented that highlights, and
    // only allows the selection of, bodies/frames in the scene
    void draw3DViewerAssignmentMode(Impl& impl) {

        Rgba32 oldAssignedMeshColor = impl.assignedMeshColor;
        Rgba32 oldUnassignedMeshColor = impl.unassignedMeshColor;

        impl.assignedMeshColor.a = 0x10;
        impl.unassignedMeshColor.a = 0x10;

        draw3DViewerScene(impl);

        if (impl.mouseOverRender) {
            handle3DViewerHovertestAssignmentMode(impl);
        }

        impl.assignedMeshColor = oldAssignedMeshColor;
        impl.unassignedMeshColor = oldUnassignedMeshColor;
    }

    // draw 3D viewer
    void draw3DViewer(Impl& impl) {

        // the 3D viewer is modal. It can either be in:
        //
        // - standard mode
        // - assignment mode (assigning bodies to eachover)

        if (impl.assignmentState.assigner < 0) {
            draw3DViewerStandardMode(impl);
        } else {
            draw3DViewerAssignmentMode(impl);
        }
    }

    // draw sidebar containing basic documentation and some action buttons
    void drawSidebar(Impl& impl) {
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
        ImGui::Text("assignment_st.assigner = %i", impl.assignmentState.assigner);
        ImGui::Text("assignment_st.is_body = %s", impl.assignmentState.isBody ? "true" : "false");
        ImGui::Text("FPS = %.1f", ImGui::GetIO().Framerate);

        // draw editors (checkboxes, sliders, etc.)
        ImGui::Checkbox("show floor", &impl.showFloor);
        ImGui::Checkbox("show meshes", &impl.showMeshes);
        ImGui::Checkbox("show ground", &impl.showGround);
        ImGui::Checkbox("show bofs", &impl.showBofs);
        ImGui::Checkbox("lock meshes", &impl.lockMeshes);
        ImGui::Checkbox("lock ground", &impl.lockGround);
        ImGui::Checkbox("lock bofs", &impl.lockBOFs);
        ImGui::Checkbox("show all connection lines", &impl.showAllConnectionLines);
        Rgba32_ColorEdit4("assigned mesh color", &impl.assignedMeshColor);
        Rgba32_ColorEdit4("unassigned mesh color", &impl.unassignedMeshColor);
        Rgba32_ColorEdit4("ground color", &impl.groundColor);
        Rgba32_ColorEdit4("body color", &impl.bodyColor);
        Rgba32_ColorEdit4("frame color", &impl.frameColor);

        ImGui::InputFloat("scene_scale_factor", &impl.sceneScaleFactor);
        if (ImGui::Button("autoscale scene_scale_factor")) {
            actionAutoscaleScene(impl);
        }

        // draw actions (buttons, etc.)
        if (ImGui::Button("add frame")) {
            actionAddFrame(impl, {0.0f, 0.0f, 0.0f});
        }
        if (ImGui::Button("select all")) {
            setIsSelectedOfAll(impl, true);
        }
        if (ImGui::Button("clear selection")) {
            setIsHoveredOfAll(impl, false);
        }
        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLUS "Import Meshes")) {
            promptUserToSelectMultipleMeshFiles(impl);
        }
        ImGui::PopStyleColor();

        testForAdvancementIssues(impl);
        if (impl.advancementIssues.empty()) {
            // next step
            if (ImGui::Button("next >>")) {
                tryCreatingOutputModel(impl);
            }
        }

        // draw error list
        if (!impl.advancementIssues.empty()) {
            ImGui::Text("issues (%zu):", impl.advancementIssues.size());
            ImGui::Separator();
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            for (std::string const& s : impl.advancementIssues) {
                ImGui::TextUnformatted(s.c_str());
            }
        }
    }
}

// public API

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen() :
    m_Impl{new Impl{}} {
}

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen(
        std::vector<std::filesystem::path> paths) :

    m_Impl{new Impl{}} {

    for (std::filesystem::path const& p : paths) {
        submitMeshfileLoadRequestToMeshloader(*m_Impl, p);
    }
}

osc::MeshesToModelWizardScreen::~MeshesToModelWizardScreen() noexcept = default;

void osc::MeshesToModelWizardScreen::onMount() {
    osc::ImGuiInit();
}

void osc::MeshesToModelWizardScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::MeshesToModelWizardScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }
}

void osc::MeshesToModelWizardScreen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

    // must be called at the start of each frame
    ImGuizmo::BeginFrame();

    // draw sidebar in a (moveable + resizeable) ImGui panel
    if (ImGui::Begin("wizardstep2sidebar")) {
        drawSidebar(*m_Impl);
    }
    ImGui::End();

    // draw 3D viewer in a (moveable + resizeable) ImGui panel
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    if (ImGui::Begin("wizardsstep2viewer")) {
        draw3DViewer(*m_Impl);
    }
    ImGui::End();
    ImGui::PopStyleVar();
    osc::ImGuiRender();
}

void osc::MeshesToModelWizardScreen::tick(float) {
    popMeshloaderOutputQueue(*m_Impl);
    ::UpdatePolarCameraFromImGuiUserInput(*m_Impl);
    updateImplFromUserInput(*m_Impl);

    // if a model was produced by this step then transition into the editor
    if (m_Impl->outputModel) {
        auto mes = std::make_shared<MainEditorState>(std::move(m_Impl->outputModel));
        App::cur().requestTransition<ModelEditorScreen>(mes);
    }
}
