#include "MeshesToModelWizardScreen.hpp"

#include "src/3D/Shaders/InstancedGouraudColorShader.hpp"
#include "src/3D/Shaders/EdgeDetectionShader.hpp"
#include "src/3D/Shaders/GouraudShader.hpp"
#include "src/3D/Shaders/SolidColorShader.hpp"
#include "src/3D/BVH.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/Texturing.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/UI/MainMenu.hpp"
#include "src/UI/LogViewer.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/Spsc.hpp"
#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/MainEditorState.hpp"
#include "src/Styling.hpp"
#include "src/os.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <SimTKcommon.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <unordered_set>
#include <stdexcept>
#include <string.h>
#include <string>
#include <vector>
#include <variant>

using namespace osc;

#define OSC_BODY_DESC "Bodies are active elements in the model. They define a frame (location + orientation) with a mass. Other properties (e.g. inertia) can be edited in the main OpenSim Creator editor after you have converted the model into an OpenSim model."
#define OSC_TRANSLATION_DESC  "Translation of the component in ground. OpenSim defines this as 'unitless'; however, models conventionally use meters."
#define OSC_GROUND_DESC "Ground is an inertial reference frame in which the motion of all Frames and points may conveniently and efficiently be expressed."
#define OSC_MESH_DESC "Meshes are purely decorational elements in the model. They can be translated, rotated, and scaled. Typically, meshes are 'attached' to other elements in the model, such as bodies. When meshes are 'attached' to something, they will translate/rotate whenever the thing they are attached to translates/rotates"
#define OSC_FLOAT_INPUT_FORMAT "%.6f"

// generic helpers
namespace {

    // returns a string representation of a spatial position (e.g. (0.0, 1.0, 3.0))
    std::string PosString(glm::vec3 const& pos)
    {
        std::stringstream ss;
        ss.precision(4);
        ss << '(' << pos.x << ", " << pos.y << ", " << pos.z << ')';
        return std::move(ss).str();
    }

    // returns a string representation of euler angles in degrees
    std::string EulerAnglesString(glm::vec3 const& eulerAngles)
    {
        glm::vec3 degrees = glm::degrees(eulerAngles);
        std::stringstream ss;
        ss << '(' << degrees.x << ", " << degrees.y << ", " << degrees.z << ')';
        return std::move(ss).str();
    }

    // returns an orthogonal rotation matrix representation of the given Euler angles
    glm::mat4 EulerAnglesToMat(glm::vec3 const& eulerAngles)
    {
        return glm::eulerAngleXYZ(eulerAngles.x, eulerAngles.y, eulerAngles.z);
    }

    // returns a Euler-angle representation of the provided rotation matrix
    glm::vec3 MatToEulerAngles(glm::mat3 const& mat)
    {
        glm::mat4 m{mat};
        glm::vec3 rv;
        glm::extractEulerAngleXYZ(m, rv.x, rv.y, rv.z);
        return rv;
    }

    // composes two euler angle rotations together
    glm::vec3 EulerCompose(glm::vec3 const& a, glm::vec3 const& b)
    {
        glm::mat3 am = EulerAnglesToMat(a);
        glm::mat3 bm = EulerAnglesToMat(b);
        glm::mat3 comp = bm * am;
        return MatToEulerAngles(comp);
    }
}

// global ID support
//
// The model graph contains internal cross-references (e.g. a joint in the model may
// cross-reference bodies that are somewhere else in the model). Those references are
// looked up at runtime using associative lookups.
//
// Associative lookups are preferred over direct pointers, shared pointers, array indices,
// etc. because the model graph can be moved in memory, copied (undo/redo), and be
// heavily edited by the user at runtime. We want the *overall* UI datastructure to have
// value, rather than reference, semantics to aid those use-cases.
//
// - IDs are global, so that there is no chance of an ID collision between different data
//   types in the system (e.g. if we're storing a variety of types)
//
// - IDs can only be "generated" or "copied". Either you generate a new ID or you copy an
//   existing one. You can't implicitly convert an arbitrary integer into an ID type. This
//   prevents IDs being "mixed around" from various (unsafe) sources
namespace {

    class UID {
        friend UID GenerateID() noexcept;
        friend constexpr int64_t UnwrapID(UID const&) noexcept;

    protected:
        explicit constexpr UID(int64_t value) noexcept : m_Value{value} {}

    private:
        int64_t m_Value;
    };

    // strongly-typed version of the above
    //
    // adds compile-time type checking to IDs
    template<typename T>
    class UIDT : public UID {
        template<typename U>
        friend UIDT<U> GenerateIDT() noexcept;

        template<typename U>
        friend constexpr UIDT<U> DowncastID(UID const&) noexcept;

    private:
        explicit constexpr UIDT(UID id) : UID{id} {}
    };

    UID GenerateID() noexcept
    {
        static std::atomic<int64_t> g_NextID = 1;
        return UID{g_NextID++};
    }

    template<typename T>
    UIDT<T> GenerateIDT() noexcept
    {
        return UIDT<T>{GenerateID()};
    }

    constexpr int64_t UnwrapID(UID const& id) noexcept
    {
        return id.m_Value;
    }

    std::ostream& operator<<(std::ostream& o, UID const& id) { return o << UnwrapID(id); }
    constexpr bool operator==(UID const& lhs, UID const& rhs) noexcept { return UnwrapID(lhs) == UnwrapID(rhs); }
    constexpr bool operator!=(UID const& lhs, UID const& rhs) noexcept { return UnwrapID(lhs) != UnwrapID(rhs); }

    template<typename T>
    constexpr UIDT<T> DowncastID(UID const& id) noexcept {
        return UIDT<T>{id};
    }

    // senteniel values used in this codebase
    class BodyEl;
    UIDT<BodyEl> const g_GroundID = GenerateIDT<BodyEl>();
    UID const g_EmptyID = GenerateID();
    UID const g_RightClickedNothingID = GenerateID();
    UID const g_GroundGroupID = GenerateID();
    UID const g_MeshGroupID = GenerateID();
    UID const g_BodyGroupID = GenerateID();
    UID const g_JointGroupID = GenerateID();
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
namespace std {

    template<>
    struct hash<UID> {
        size_t operator()(UID const& id) const { return static_cast<size_t>(UnwrapID(id)); }
    };

    template<typename T>
    struct hash<UIDT<T>> {
        size_t operator()(UID const& id) const { return static_cast<size_t>(UnwrapID(id)); }
    };
}

// background mesh loading support
//
// loading mesh files can be slow, so all mesh loading is done on a background worker
// that:
//
//   - receives a mesh loading request
//   - loads the mesh
//   - sends the loaded mesh (or error) as a response
//
// the main (UI) thread then regularly polls the response channel and handles the (loaded)
// mesh appropriately
namespace {

    class BodyEl;

    // a mesh loading request
    struct MeshLoadRequest final {
        UIDT<BodyEl> PreferredAttachmentPoint;
        std::vector<std::filesystem::path> Paths;
    };

    struct LoadedMesh final {
        std::filesystem::path Path;
        std::shared_ptr<Mesh> Mesh;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        UIDT<BodyEl> PreferredAttachmentPoint;
        std::vector<LoadedMesh> Meshes;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        UIDT<BodyEl> PreferredAttachmentPoint;
        std::filesystem::path Path;
        std::string Error;
    };

    // an OK/ERROR response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // function that's used by the meshloader to respond to a mesh loading request
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest msg) noexcept
    {
        std::vector<LoadedMesh> loadedMeshes;
        for (std::filesystem::path const& path : msg.Paths) {
            try {
                std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(SimTKLoadMesh(path));
                loadedMeshes.push_back(LoadedMesh{path, mesh});
            } catch (std::exception const& ex) {
                return MeshLoadErrorResponse{msg.PreferredAttachmentPoint, path, ex.what()};
            }
        }
        App::cur().requestRedraw();  // TODO: HACK: try to make the UI thread redraw around the time this is sent
        return MeshLoadOKResponse{msg.PreferredAttachmentPoint, std::move(loadedMeshes)};
    }

    // top-level MeshLoader class that the UI thread can safely poll
    class MeshLoader final {
        using Worker = spsc::Worker<MeshLoadRequest, MeshLoadResponse, decltype(respondToMeshloadRequest)>;

    public:
        MeshLoader() : m_Worker{Worker::create(respondToMeshloadRequest)}
        {
        }

        void send(MeshLoadRequest req) { m_Worker.send(std::move(req)); }
        std::optional<MeshLoadResponse> poll() { return m_Worker.poll(); }

    private:
        spsc::Worker<MeshLoadRequest, MeshLoadResponse, decltype(respondToMeshloadRequest)> m_Worker;
    };
}

// modelgraph support
//
// This code adds support for an editor-specific "modelgraph". This modelgraph is specifically
// designed for:
//
//   - Freeform UI manipulation. Everything is defined "in ground", rather than relative to the
//     current model topography. This lets users drag, rotate, reassign, etc. things around without
//     having to constantly worry so much about joint attachment topology, frame fixups, etc.
//
//   - UI integration. Uses `glm` vector types, floats, supports raycasting, has reference-counted
//     access to ready-to-render meshes, etc.
//
//   - Value semantics. Datatypes do not contain pointer-based crossreferences (exception: mesh
//     data), which means that independent copies of the modelgraph can be cheaply made at runtime (for
//     undo/redo and snapshot support)
//
//   - Direct (eventual) mapping to an OpenSim::Model. So that the user-edited freeform modelgraph can
//     be exported directly into the main (OpenSim::Model-manipulating) UI
namespace {

    // rotate-and-shift transform
    struct Ras final {
        glm::vec3 rot;  // Euler angles
        glm::vec3 shift;

        Ras() : rot{}, shift{} {}  // effectively, default-constructs to ground location
        Ras(glm::vec3 const& rot_, glm::vec3 const& shift_) : rot{rot_}, shift{shift_} {}
    };

    // print to an output stream
    std::ostream& operator<<(std::ostream& o, Ras const& to)
    {
        using osc::operator<<;
        return o << "Ras(rot = " << to.rot << ", shift = " << to.shift << ')';
    }

    glm::mat4 CalcXformMatrix(Ras const& ras)
    {
        glm::mat4 mtx = EulerAnglesToMat(ras.rot);
        mtx[3] = glm::vec4{ras.shift, 1.0f};
        return mtx;
    }

    // returns a `Ras` that can reorient things expressed in base to things expressed in parent
    Ras RasInParent(Ras const& parentInBase, Ras const& childInBase)
    {
        glm::mat4 parent2base = CalcXformMatrix(parentInBase);
        glm::mat4 base2parent = glm::inverse(parent2base);
        glm::mat4 child2base = CalcXformMatrix(childInBase);
        glm::mat4 child2parent = base2parent * child2base;

        glm::vec3 shift = base2parent * glm::vec4{childInBase.shift, 1.0f};
        glm::vec3 rotation = MatToEulerAngles(child2parent);

        return Ras{rotation, shift};
    }

    void ApplyTranslation(Ras& ras, glm::vec3 const& translation)
    {
        ras.shift += translation;
    }

    void ApplyRotation(Ras& ras, glm::vec3 const& eulerAngles)
    {
        ras.rot = EulerCompose(ras.rot, eulerAngles);
    }

    // a mesh in the scene
    //
    // In this mesh importer, meshes are always positioned + oriented in ground. At OpenSim::Model generation
    // time, the implementation does necessary maths to attach the meshes into the Model in the relevant relative
    // coordinate system.
    //
    // The reason the editor uses ground-based coordinates is so that users have freeform control over where
    // the mesh will be positioned in the model, and so that the user can freely re-attach the mesh and freely
    // move meshes/bodies/joints in the mesh importer without everything else in the scene moving around (which
    // is what would happen in a relative topology-sensitive attachment graph).
    class BodyEl;
    class MeshEl final {
    public:
        MeshEl(UIDT<MeshEl> id,
               UIDT<BodyEl> attachment,  // can be g_GroundID
               std::shared_ptr<Mesh> meshData,
               std::filesystem::path const& path) :

            ID{id},
            Attachment{attachment},
            MeshData{meshData},
            Path{path}
        {
        }

        UIDT<MeshEl> ID;
        UIDT<BodyEl> Attachment;  // can be g_GroundID
        Ras Xform;
        glm::vec3 ScaleFactors{1.0f, 1.0f, 1.0f};
        std::shared_ptr<Mesh> MeshData;
        std::filesystem::path Path;
        std::string Name{FileNameWithoutExtension(Path)};
    };

    // write MeshEl to output stream in a human-readable format
    std::ostream& operator<<(std::ostream& o, MeshEl const& mesh)
    {
        using osc::operator<<;

        return o << "MeshEl(ID = " << mesh.ID
                 << ", Attachment = " << mesh.Attachment
                 << ", Xform = " << mesh.Xform
                 << ", ScaleFactors = " << mesh.ScaleFactors
                 << ", MeshData = " << mesh.MeshData.get()
                 << ", Path = " << mesh.Path
                 << ", Name = " << mesh.Name
                 << ')';
    }

    // returns unique ID for the mesh element
    UID GetID(MeshEl const& mesh)
    {
        return mesh.ID;
    }

    // returns human-readable label for the mesh
    std::string const& GetLabel(MeshEl const& mesh)
    {
        return mesh.Name;
    }

    glm::vec3 GetShift(MeshEl const& mesh)
    {
        return mesh.Xform.shift;
    }

    glm::vec3 GetRotation(MeshEl const& mesh)
    {
        return mesh.Xform.rot;
    }

    Ras GetRas(MeshEl const& mesh)
    {
        return mesh.Xform;
    }

    // returns transform matrix for the mesh (transforms origin-centered meshes into worldspace)
    glm::mat4 CalcXformMatrix(MeshEl const& mesh)
    {
        glm::mat4 baseXform = CalcXformMatrix(mesh.Xform);
        glm::mat4 scalingXform = glm::scale(glm::mat4{1.0f}, mesh.ScaleFactors);
        return baseXform * scalingXform;
    }

    // returns the groundspace bounds of the mesh
    AABB CalcBounds(Mesh const& mesh, glm::mat4 const& modelMtx)
    {
        AABB modelspaceBounds = mesh.getAABB();
        return AABBApplyXform(modelspaceBounds, modelMtx);
    }

    // returns the groundspace bounds of the mesh
    AABB CalcBounds(MeshEl const& mesh)
    {
        return CalcBounds(*mesh.MeshData, CalcXformMatrix(mesh));
    }

    // returns the groundspace bounds center point of the mesh
    glm::vec3 CalcBoundsCenter(MeshEl const& mesh)
    {
        return AABBCenter(CalcBounds(mesh));
    }

    // returns a unique, generated body name
    std::string GenerateBodyName()
    {
        static std::atomic<int> g_LatestBodyIdx = 0;

        std::stringstream ss;
        ss << "body" << g_LatestBodyIdx++;
        return std::move(ss).str();
    }

    void ApplyTranslation(MeshEl& mesh, glm::vec3 const& translation)
    {
        ApplyTranslation(mesh.Xform, translation);
    }

    void ApplyRotation(MeshEl& mesh, glm::vec3 const& eulerAngles)
    {
        ApplyRotation(mesh.Xform, eulerAngles);
    }

    void ApplyScale(MeshEl& mesh, glm::vec3 const& scaleFactors)
    {
        mesh.ScaleFactors *= scaleFactors;
    }

    // a body scene element
    //
    // In this mesh importer, bodies are positioned + oriented in ground (see MeshEl for explanation of why).
    class BodyEl final {
    public:
        BodyEl(UIDT<BodyEl> id,
               std::string const& name,
               Ras xform) :

            ID{id},
            Name{name},
            Xform{xform}
        {
        }

        UIDT<BodyEl> ID;
        std::string Name;
        Ras Xform;
        double Mass{1.0f};  // OpenSim goes bananas if a body has a mass <= 0
    };

    std::ostream& operator<<(std::ostream& o, BodyEl const& b)
    {
        return o << "BodyEl(ID = " << b.ID
                 << ", Name = " << b.Name
                 << ", Xform = " << b.Xform
                 << ", Mass = " << b.Mass
                 << ')';
    }

    // returns unique ID for the body element
    UID GetID(BodyEl const& body)
    {
        return body.ID;
    }

    // returns human-readable label for the body element
    std::string const& GetLabel(BodyEl const& body)
    {
        return body.Name;
    }

    glm::vec3 GetShift(BodyEl const& body)
    {
        return body.Xform.shift;
    }

    glm::vec3 GetRotation(BodyEl const& body)
    {
        return body.Xform.rot;
    }

    Ras GetRas(BodyEl const& body)
    {
        return body.Xform;
    }

    // returns transform matrix for the body element
    glm::mat4 CalcXformMatrix(BodyEl const& body)
    {
        return CalcXformMatrix(body.Xform);
    }

    // returns groundspace bounds of the body element (volume == 0)
    AABB CalcBounds(BodyEl const& body)
    {
        return AABB{body.Xform.shift, body.Xform.shift};
    }

    void ApplyTranslation(BodyEl& body, glm::vec3 const& translation)
    {
        ApplyTranslation(body.Xform, translation);
    }

    void ApplyRotation(BodyEl& body, glm::vec3 const& eulerAngles)
    {
        ApplyRotation(body.Xform, eulerAngles);
    }

    void ApplyScale(BodyEl&, glm::vec3 const&)
    {
        return;  // can't scale a body
    }

    // a joint scene element
    //
    // see `JointAttachment` comment for an explanation of why it's designed this way.
    class JointEl final {
    public:
        JointEl(UIDT<JointEl> id,
                size_t jointTypeIdx,
                std::string userAssignedName,  // can be empty
                UID parent,
                UIDT<BodyEl> child,
                Ras pos) :

            ID{std::move(id)},
            JointTypeIndex{std::move(jointTypeIdx)},
            UserAssignedName{std::move(userAssignedName)},
            Parent{std::move(parent)},
            Child{std::move(child)},
            Center{std::move(pos)}
        {
        }

        UIDT<JointEl> ID;
        size_t JointTypeIndex;
        std::string UserAssignedName;
        UID Parent;  // can be ground
        UIDT<BodyEl> Child;
        Ras Center;
    };

    std::ostream& operator<<(std::ostream& o, JointEl const& j)
    {
        return o << "JointEl(ID = " << j.ID
                 << ", JointTypeIndex = " << j.JointTypeIndex
                 << ", UserAssignedName = " << j.UserAssignedName
                 << ", Parent = " << j.Parent
                 << ", Child = " << j.Child
                 << ", Center = " << j.Center
                 << ')';
    }

    // returns unique ID for the joint element
    UID GetID(JointEl const& joint)
    {
        return joint.ID;
    }

    // returns a human-readable typename for the joint
    std::string const& GetJointTypeName(JointEl const& joint)
    {
        return JointRegistry::nameStrings()[joint.JointTypeIndex];
    }

    // returns human-readable label for the joint element
    std::string const& GetLabel(JointEl const& joint)
    {
        return joint.UserAssignedName.empty() ? GetJointTypeName(joint) : joint.UserAssignedName;
    }

    glm::vec3 GetShift(JointEl const& joint)
    {
        return joint.Center.shift;
    }

    glm::vec3 GetRotation(JointEl const& joint)
    {
        return joint.Center.rot;
    }

    Ras GetRas(JointEl const& joint)
    {
        return joint.Center;
    }

    // returns transform matrix for the joint center
    glm::mat4 CalcXformMatrix(JointEl const& joint)
    {
        return CalcXformMatrix(joint.Center);
    }

    // returns groundspace bounds of the joint center (volume == 0)
    AABB CalcBounds(JointEl const& joint)
    {
        return AABB{joint.Center.shift, joint.Center.shift};
    }

    void ApplyTranslation(JointEl& joint, glm::vec3 const& translation)
    {
        ApplyTranslation(joint.Center, translation);
    }

    void ApplyRotation(JointEl& joint, glm::vec3 const& eulerAngles)
    {
        ApplyRotation(joint.Center, eulerAngles);
    }

    void ApplyScale(JointEl&, glm::vec3 const&)
    {
        return;  // can't scale a joint center
    }

    // returns `true` if body is either the parent or the child attachment of `joint`
    bool IsAttachedToJoint(JointEl const& joint, BodyEl const& body)
    {
        return joint.Parent == body.ID || joint.Child == body.ID;
    }

    // returns an OpenSim::Joint that has the specified type index (from the type registry)
    std::unique_ptr<OpenSim::Joint> ConstructOpenSimJointFromTypeIndex(size_t typeIndex)
    {
        return std::unique_ptr<OpenSim::Joint>(JointRegistry::prototypes()[typeIndex]->clone());
    }

    // top-level model structure
    //
    // - must have value semantics, so that other code can copy this to (e.g.) an
    //   undo/redo buffer without worrying about references into the copy later
    //   internally tainting the buffer
    //
    // - associative lookups need to be fairly fast, because that's how the rest
    //   of the system "references" things in this graph (rather than using pointers,
    //   which don't play well with copying, multiple versions, etc.)
    class ModelGraph final {
    public:
        std::unordered_map<UIDT<MeshEl>, MeshEl> const& GetMeshes() const { return m_Meshes; }
        std::unordered_map<UIDT<BodyEl>, BodyEl> const& GetBodies() const { return m_Bodies; }
        std::unordered_map<UIDT<JointEl>, JointEl> const& GetJoints() const { return m_Joints; }
        std::unordered_set<UID> const& GetSelected() const { return m_Selected; }

        bool ContainsMeshEl(UID id) const { return TryGetMeshElByID(id) != nullptr; }
        bool ContainsBodyEl(UID id) const { return TryGetBodyElByID(id) != nullptr; }
        bool ContainsJointEl(UID id) const { return TryGetJointElByID(id) != nullptr; }

        MeshEl const* TryGetMeshElByID(UID id) const { return const_cast<ModelGraph&>(*this).TryUpdMeshElByID(id); }
        BodyEl const* TryGetBodyElByID(UID id) const { return const_cast<ModelGraph&>(*this).TryUpdBodyElByID(id); }
        JointEl const* TryGetJointElByID(UID id) const { return const_cast<ModelGraph&>(*this).TryUpdJointElByID(id); }

        MeshEl const& GetMeshByIDOrThrow(UID id) const { return const_cast<ModelGraph&>(*this).UpdMeshByIDOrThrow(id); }
        BodyEl const& GetBodyByIDOrThrow(UID id) const { return const_cast<ModelGraph&>(*this).UpdBodyByIDOrThrow(id); }
        JointEl const& GetJointByIDOrThrow(UID id) const { return const_cast<ModelGraph&>(*this).UpdJointByIDOrThrow(id); }

        UIDT<BodyEl> AddBody(std::string name, Ras const& ras)
        {
            UIDT<BodyEl> id = GenerateIDT<BodyEl>();
            return m_Bodies.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, name, ras)).first->first;
        }

        UIDT<MeshEl> AddMesh(std::shared_ptr<Mesh> mesh, UIDT<BodyEl> attachment, std::filesystem::path const& path)
        {
            if (attachment != g_GroundID && !ContainsBodyEl(attachment)) {
                throw std::runtime_error{"implementation error: tried to assign a body to a mesh, but the body does not exist"};
            }

            UIDT<MeshEl> id = GenerateIDT<MeshEl>();
            return m_Meshes.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, attachment, mesh, path)).first->first;
        }

        UIDT<JointEl> AddJoint(size_t jointTypeIdx, std::string maybeName, UID parent, UIDT<BodyEl> child, Ras center)
        {
            UIDT<JointEl> id = GenerateIDT<JointEl>();
            return m_Joints.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, jointTypeIdx, maybeName, parent, child, center)).first->first;
        }

        void SetMeshAttachmentPoint(UIDT<MeshEl> meshID, UIDT<BodyEl> bodyID)
        {
            UpdMeshByIDOrThrow(meshID).Attachment = bodyID;
        }

        void UnsetMeshAttachmentPoint(UIDT<MeshEl> meshID)
        {
            UpdMeshByIDOrThrow(meshID).Attachment = g_GroundID;
        }

        void SetMeshXform(UIDT<MeshEl> meshID, Ras const& newXform)
        {
            UpdMeshByIDOrThrow(meshID).Xform = newXform;
        }

        void SetMeshScaleFactors(UIDT<MeshEl> meshID, glm::vec3 const& newScaleFactors)
        {
            UpdMeshByIDOrThrow(meshID).ScaleFactors = newScaleFactors;
        }

        void SetMeshName(UIDT<MeshEl> meshID, std::string_view newName)
        {
            UpdMeshByIDOrThrow(meshID).Name = newName;
        }

        void SetBodyName(UIDT<BodyEl> bodyID, std::string_view newName)
        {
            UpdBodyByIDOrThrow(bodyID).Name = newName;
        }

        void SetJointName(UIDT<JointEl> jointID, std::string_view newName)
        {
            UpdJointByIDOrThrow(jointID).UserAssignedName = newName;
        }

        void SetBodyXform(UIDT<BodyEl> bodyID, Ras const& newXform)
        {
            UpdBodyByIDOrThrow(bodyID).Xform = newXform;
        }

        void SetJointCenter(UIDT<JointEl> jointID, Ras const& newCenter)
        {
            UpdJointByIDOrThrow(jointID).Center = newCenter;
        }

        void SetJointTypeIdx(UIDT<JointEl> jointID, size_t newIdx)
        {
            UpdJointByIDOrThrow(jointID).JointTypeIndex = newIdx;
        }

        void SetBodyMass(UIDT<BodyEl> bodyID, double newMass)
        {
            UpdBodyByIDOrThrow(bodyID).Mass = newMass;
        }

        template<typename Consumer>
        void ForEachSceneElID(Consumer idConsumer) const
        {
            for (auto const& [meshID, mesh] : GetMeshes()) { idConsumer(meshID); }
            for (auto const& [bodyID, body] : GetBodies()) { idConsumer(bodyID); }
            for (auto const& [jointID, joint] : GetJoints()) { idConsumer(jointID); }
        }

        void DeleteMeshElByID(UID id)
        {
            auto it = m_Meshes.find(DowncastID<MeshEl>(id));

            if (it == m_Meshes.end()) {
                return;  // ID doesn't exist in mesh collection
            }

            DeSelect(it->first);
            m_Meshes.erase(it);
        }

        void DeleteBodyElByID(UID id)
        {
            auto it = m_Bodies.find(DowncastID<BodyEl>(id));

            if (it == m_Bodies.end()) {
                return;  // ID doesn't exist in body collection
            }

            // delete any joints that reference the body
            for (auto jointIt = m_Joints.begin(); jointIt != m_Joints.end();) {

                if (IsAttachedToJoint(jointIt->second, it->second)) {
                    DeSelect(jointIt->first);
                    jointIt = m_Joints.erase(jointIt);
                } else {
                    ++jointIt;
                }
            }

            // delete any meshes attached to the body
            for (auto meshIt = m_Meshes.begin(); meshIt != m_Meshes.end();) {

                if (meshIt->second.Attachment == it->first) {
                    DeSelect(meshIt->first);
                    meshIt = m_Meshes.erase(meshIt);
                } else {
                    ++meshIt;
                }
            }

            // delete the body
            DeSelect(it->first);
            m_Bodies.erase(it);
        }

        void DeleteJointElByID(UID id)
        {
            auto it = m_Joints.find(DowncastID<JointEl>(id));

            if (it == m_Joints.end()) {
                return;
            }

            DeSelect(it->first);
            m_Joints.erase(it);
        }

        void DeleteElementByID(UID id)
        {
            DeleteMeshElByID(id);
            DeleteBodyElByID(id);
            DeleteJointElByID(id);
        }

        void ApplyTranslation(UID id, glm::vec3 const& translation)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                ::ApplyTranslation(*meshPtr, translation);
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                ::ApplyTranslation(*bodyPtr, translation);
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                ::ApplyTranslation(*jointPtr, translation);
            }
        }

        void ApplyRotation(UID id, glm::vec3 const& eulerAngles)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                ::ApplyRotation(*meshPtr, eulerAngles);
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                ::ApplyRotation(*bodyPtr, eulerAngles);
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                ::ApplyRotation(*jointPtr, eulerAngles);
            }
        }

        void ApplyScale(UID id, glm::vec3 const& scaleFactors)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                ::ApplyScale(*meshPtr, scaleFactors);
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                ::ApplyScale(*bodyPtr, scaleFactors);
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                ::ApplyScale(*jointPtr, scaleFactors);
            }
        }

        glm::vec3 GetShiftInGround(UID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return GetShift(*meshPtr);
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return GetShift(*bodyPtr);
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                return GetShift(*jointPtr);
            } else {
                throw std::runtime_error{"GetShiftInGround(): cannot find element by ID"};
            }
        }

        glm::vec3 GetRotationInGround(UID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return GetRotation(*meshPtr);
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return GetRotation(*bodyPtr);
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                return GetRotation(*jointPtr);
            } else {
                throw std::runtime_error{"GetRotationInGround(): cannot find element by ID"};
            }
        }

        Ras GetRasInGround(UID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return GetRas(*meshPtr);
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return GetRas(*bodyPtr);
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                return GetRas(*jointPtr);
            } else {
                throw std::runtime_error{"GetRasInGround(): cannot find element by ID"};
            }
        }

        // returns empty AABB at point if a point-like element (e.g. mesh, joint pivot)
        AABB GetBounds(UID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return CalcBounds(*meshPtr);
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return CalcBounds(*bodyPtr);
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                return CalcBounds(*jointPtr);
            } else {
                throw std::runtime_error{"GetBounds(): could not find supplied ID"};
            }
        }

        void SelectAll()
        {
            auto addIDToSelectionSet = [this](UID id) { m_Selected.insert(id); };
            ForEachSceneElID(addIDToSelectionSet);
        }

        void DeSelectAll()
        {
            m_Selected.clear();
        }

        void Select(UID id)
        {
            if (id != g_EmptyID && id != g_GroundID) {
                m_Selected.insert(id);
            }
        }

        void DeSelect(UID id)
        {
            auto it = m_Selected.find(id);
            if (it != m_Selected.end()) {
                m_Selected.erase(it);
            }
        }

        bool HasSelection() const
        {
            return !m_Selected.empty();
        }

        bool IsSelected(UID id) const
        {
            return m_Selected.find(id) != m_Selected.end();
        }

        void DeleteSelected()
        {
            auto selected = m_Selected;  // copy to ensure iterator invalidation doesn't screw us
            for (UID id : selected) {
                DeleteElementByID(id);
            }
            m_Selected.clear();
        }

    private:
        MeshEl* TryUpdMeshElByID(UID id)
        {
            auto it = m_Meshes.find(DowncastID<MeshEl>(id));
            return it != m_Meshes.end() ? &it->second : nullptr;
        }

        BodyEl* TryUpdBodyElByID(UID id)
        {
            auto it = m_Bodies.find(DowncastID<BodyEl>(id));
            return it != m_Bodies.end() ? &it->second : nullptr;
        }

        JointEl* TryUpdJointElByID(UID id)
        {
            auto it = m_Joints.find(DowncastID<JointEl>(id));
            return it != m_Joints.end() ? &it->second : nullptr;
        }

        MeshEl& UpdMeshByIDOrThrow(UID id)
        {
            MeshEl* meshEl = TryUpdMeshElByID(id);
            if (!meshEl) {
                throw std::runtime_error{"could not find a mesh"};
            }
            return *meshEl;
        }

        BodyEl& UpdBodyByIDOrThrow(UID id)
        {
            BodyEl* bodyEl = TryUpdBodyElByID(id);
            if (!bodyEl) {
                throw std::runtime_error{"could not find a body"};
            }
            return *bodyEl;
        }

        JointEl& UpdJointByIDOrThrow(UID id)
        {
            JointEl* jointEl = TryUpdJointElByID(id);
            if (!jointEl) {
                throw std::runtime_error{"could not find a joint"};
            }
            return *jointEl;
        }

        std::unordered_map<UIDT<MeshEl>, MeshEl> m_Meshes;
        std::unordered_map<UIDT<BodyEl>, BodyEl> m_Bodies;
        std::unordered_map<UIDT<JointEl>, JointEl> m_Joints;
        std::unordered_set<UID> m_Selected;
    };

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(ModelGraph const& modelGraph, BodyEl const& body)
    {
        auto IsBodyAttachedToJointAsChild = [&body](auto const& pair)
        {
            return pair.second.Child == body.ID;
        };

        return AnyOf(modelGraph.GetJoints(), IsBodyAttachedToJointAsChild);
    }

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(ModelGraph const& modelGraph, JointEl const& jointEl)
    {
        if (jointEl.Child == g_GroundID) {
            return true;  // ground cannot be a child in a joint
        }

        if (jointEl.Parent == jointEl.Child) {
            return true;  // is directly attached to itself
        }

        if (jointEl.Parent != g_GroundID && !modelGraph.ContainsBodyEl(jointEl.Parent)) {
             return true;  // has a parent ID that's invalid for this model graph
        }

        if (!modelGraph.ContainsBodyEl(jointEl.Child)) {
            return true;  // has a child ID that's invalid for this model graph
        }

        return false;
    }

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
                                BodyEl const& body,
                                std::unordered_set<UID>& previouslyVisitedJoints);

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(ModelGraph const& modelGraph,
                                 JointEl const& joint,
                                 std::unordered_set<UID>& previousVisits)
    {
        OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, joint));

        if (joint.Parent == g_GroundID) {
            return true;
        }

        BodyEl const* parent = modelGraph.TryGetBodyElByID(joint.Parent);

        if (!parent) {
            return false;  // joint's parent is garbage
        }

        return IsBodyAttachedToGround(modelGraph, *parent, previousVisits);
    }

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
                                BodyEl const& body,
                                std::unordered_set<UID>& previouslyVisitedJoints)
    {
        bool childInAtLeastOneJoint = false;

        for (auto const& [jointID, jointEl] : modelGraph.GetJoints()) {
            OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, jointEl));

            if (jointEl.Child == body.ID) {

                childInAtLeastOneJoint = true;

                bool alreadyVisited = !previouslyVisitedJoints.emplace(jointEl.ID).second;
                if (alreadyVisited) {
                    continue;  // skip this joint: was previously visited
                }

                if (IsJointAttachedToGround(modelGraph, jointEl, previouslyVisitedJoints)) {
                    return true;
                }
            }
        }

        if (childInAtLeastOneJoint) {
            return false;  // particiaptes as a child in at least one joint but doesn't ultimately join to ground
        } else {
            return true;
        }
    }

    // returns `true` if `modelGraph` contains issues
    bool GetModelGraphIssues(ModelGraph const& modelGraph, std::vector<std::string>& issuesOut)
    {
        issuesOut.clear();

        for (auto const& [id, joint] : modelGraph.GetJoints()) {
            if (IsGarbageJoint(modelGraph, joint)) {
                std::stringstream ss;
                ss << GetLabel(joint) << ": joint is garbage (this is an implementation error)";
                throw std::runtime_error{std::move(ss).str()};
            }
        }

        for (auto const& [id, body] : modelGraph.GetBodies()) {
            std::unordered_set<UID> previouslyVisitedJoints;
            if (!IsBodyAttachedToGround(modelGraph, body, previouslyVisitedJoints)) {
                std::stringstream ss;
                ss << body.Name << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
                issuesOut.push_back(std::move(ss).str());
            }
        }

        return !issuesOut.empty();
    }

    // attaches a mesh to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
    void AttachMeshElToFrame(MeshEl const& meshEl,
                             Ras const& parentRas,
                             OpenSim::PhysicalFrame& parentPhysFrame)
    {
        // create a POF that attaches to the body
        auto meshPhysOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
        meshPhysOffsetFrame->setName(meshEl.Name + "_offset");

        // re-express the transform matrix in the parent's frame
        glm::mat4 parent2ground = CalcXformMatrix(parentRas);
        glm::mat4 ground2parent = glm::inverse(parent2ground);
        glm::mat4 mesh2ground = CalcXformMatrix(meshEl.Xform);
        glm::mat4 mesh2parent = ground2parent * mesh2ground;

        // set it as the transform
        meshPhysOffsetFrame->setOffsetTransform(SimTKTransformFromMat4x3(mesh2parent));

        // attach mesh to the POF
        auto mesh = std::make_unique<OpenSim::Mesh>(meshEl.Path.string());
        mesh->setName(meshEl.Name);
        mesh->set_scale_factors(SimTKVec3FromV3(meshEl.ScaleFactors));
        meshPhysOffsetFrame->attachGeometry(mesh.release());

        parentPhysFrame.addComponent(meshPhysOffsetFrame.release());
    }

    // create a body for the `model`, but don't add it to the model yet
    //
    // *may* add any attached meshes to the model, though
    std::unique_ptr<OpenSim::Body> CreateDetatchedBody(ModelGraph const& mg, BodyEl const& bodyEl)
    {
        auto addedBody = std::make_unique<OpenSim::Body>();
        addedBody->setMass(bodyEl.Mass);
        addedBody->setName(bodyEl.Name);

        for (auto const& [meshID, mesh] : mg.GetMeshes()) {
            if (mesh.Attachment == bodyEl.ID) {
                AttachMeshElToFrame(mesh, bodyEl.Xform, *addedBody);
            }
        }

        return addedBody;
    }

    // result of a lookup for (effectively) a physicalframe
    struct JointAttachmentCachedLookupResult {
        BodyEl const* bodyEl;  // can be nullptr (indicating Ground)
        std::unique_ptr<OpenSim::Body> createdBody;  // can be nullptr (indicating ground/cache hit)
        OpenSim::PhysicalFrame* physicalFrame;  // always != nullptr, can point to `createdBody`, or an existing body from the cache, or Ground
    };

    // cached lookup of a physical frame
    //
    // if the frame/body doesn't exist yet, constructs it
    JointAttachmentCachedLookupResult LookupPhysFrame(ModelGraph const& mg,
                                                      OpenSim::Model& model,
                                                      std::unordered_map<UID, OpenSim::Body*>& visitedBodies,
                                                      UID elID)
    {
        // figure out what the parent body is. There's 3 possibilities:
        //
        // - null (ground)
        // - found, visited before (get it, but don't make it or add it to the model)
        // - found, not visited before (make it, add it to the model, cache it)

        JointAttachmentCachedLookupResult rv;
        rv.bodyEl = mg.TryGetBodyElByID(elID);
        rv.createdBody = nullptr;
        rv.physicalFrame = nullptr;

        if (rv.bodyEl) {
            auto it = visitedBodies.find(elID);
            if (it == visitedBodies.end()) {
                // haven't visited the body before
                rv.createdBody = CreateDetatchedBody(mg, *rv.bodyEl);
                rv.physicalFrame = rv.createdBody.get();

                // add it to the cache
                visitedBodies.emplace(elID, rv.createdBody.get());
            } else {
                // visited the body before, use cached result
                rv.createdBody = nullptr;  // it's not this function's responsibility to add it
                rv.physicalFrame = it->second;
            }
        } else {
            // the element is connected to ground
            rv.createdBody = nullptr;
            rv.physicalFrame = &model.updGround();
        }

        return rv;
    }

    // compute the name of a joint from its attached frames
    std::string CalcJointName(JointEl const& jointEl,
                              OpenSim::PhysicalFrame const& parentFrame,
                              OpenSim::PhysicalFrame const& childFrame)
    {
        if (!jointEl.UserAssignedName.empty()) {
            return jointEl.UserAssignedName;
        } else {
            return childFrame.getName() + "_to_" + parentFrame.getName();
        }
    }

    // returns true if the given element (ID) is in the "selection group" of
    bool IsInSelectionGroupOf(ModelGraph const& mg, UID parent, UID id)
    {
        if (id == g_EmptyID || parent == g_EmptyID) {
            return false;
        }

        if (id == parent) {
            return true;
        }

        BodyEl const* bodyEl = nullptr;

        if (BodyEl const* be = mg.TryGetBodyElByID(parent)) {
            bodyEl = be;
        } else if (MeshEl const* me = mg.TryGetMeshElByID(parent)) {
            bodyEl = mg.TryGetBodyElByID(me->Attachment);
        }

        if (!bodyEl) {
            return false;  // parent isn't attached to any body (or isn't a body)
        }

        if (BodyEl const* be = mg.TryGetBodyElByID(id)) {
            return be->ID == bodyEl->ID;
        } else if (MeshEl const* me = mg.TryGetMeshElByID(id)) {
            return me->Attachment == bodyEl->ID;
        } else {
            return false;
        }
    }

    template<typename Consumer>
    void ForEachIDInSelectionGroup(ModelGraph const& mg, UID parent, Consumer f)
    {
        for (auto const& [meshID, meshEl] : mg.GetMeshes()) {
            if (IsInSelectionGroupOf(mg, parent, meshID)) {
                f(meshID);
            }
        }
        for (auto const& [bodyID, bodyEl] : mg.GetBodies()) {
            if (IsInSelectionGroupOf(mg, parent, bodyID)) {
                f(bodyID);
            }
        }
        for (auto const& [jointID, jointEl] : mg.GetJoints()) {
            if (IsInSelectionGroupOf(mg, parent, jointID)) {
                f(jointID);
            }
        }
    }

    // expresses if a joint has a degree of freedom (i.e. != -1) and the coordinate index of
    // that degree of freedom
    struct JointDegreesOfFreedom final {
        std::array<int, 3> orientation = {-1, -1, -1};
        std::array<int, 3> translation = {-1, -1, -1};
    };

    // returns the indices of each degree of freedom that the joint supports
    JointDegreesOfFreedom GetDegreesOfFreedom(size_t jointTypeIdx)
    {
        OpenSim::Joint const* proto = JointRegistry::prototypes()[jointTypeIdx].get();
        size_t typeHash = typeid(*proto).hash_code();

        if (typeHash == typeid(OpenSim::FreeJoint).hash_code()) {
            return JointDegreesOfFreedom{{0, 1, 2}, {3, 4, 5}};
        } else if (typeHash == typeid(OpenSim::PinJoint).hash_code()) {
            return JointDegreesOfFreedom{{-1, -1, 0}, {-1, -1, -1}};
        } else {
            return JointDegreesOfFreedom{};  // unknown joint type
        }
    }

    glm::vec3 GetJointAxisLengths(JointEl const& joint)
    {
        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(joint.JointTypeIndex);

        glm::vec3 rv;
        for (int i = 0; i < 3; ++i) {
            rv[i] = dofs.orientation[static_cast<size_t>(i)] == -1 ? 0.6f : 1.0f;
        }
        return rv;
    }

    // sets the names of a joint's coordinates
    void SetJointCoordinateNames(OpenSim::Joint& joint, std::string const& prefix)
    {
        constexpr std::array<char const*, 3> const translationNames = {"_tx", "_ty", "_tz"};
        constexpr std::array<char const*, 3> const rotationNames = {"_rx", "_ry", "_rz"};

        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(*JointRegistry::indexOf(joint));

        // translations
        for (int i = 0; i < 3; ++i) {
            if (dofs.translation[i] != -1) {
                joint.upd_coordinates(dofs.translation[i]).setName(prefix + translationNames[i]);
            }
        }

        for (int i = 0; i < 3; ++i) {
            if (dofs.orientation[i] != -1) {
                joint.upd_coordinates(dofs.orientation[i]).setName(prefix + rotationNames[i]);
            }
        }
    }

    // recursively attaches `joint` to `model` by:
    //
    // - adding child bodies, if necessary
    // - adding an offset frames for each side of the joint
    // - computing relevant offset values for the offset frames, to ensure the bodies/joint-center end up in the right place
    // - setting the joint's default coordinate values based on any differences
    // - RECURSING by figuring out which joints have this joint's child as a parent
    void AttachJointRecursive(ModelGraph const& mg,
                              OpenSim::Model& model,
                              JointEl const& joint,
                              std::unordered_map<UID, OpenSim::Body*>& visitedBodies,
                              std::unordered_set<UID>& visitedJoints)
    {
        if (auto const& [it, wasInserted] = visitedJoints.emplace(joint.ID); !wasInserted) {
            return;  // graph cycle detected: joint was already previously visited and shouldn't be traversed again
        }

        // lookup each side of the joint, creating the bodies if necessary
        JointAttachmentCachedLookupResult parent = LookupPhysFrame(mg, model, visitedBodies, joint.Parent);
        JointAttachmentCachedLookupResult child = LookupPhysFrame(mg, model, visitedBodies, joint.Child);

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parentPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parentPOF->setName(parent.physicalFrame->getName() + "_offset");
        parentPOF->setParentFrame(*parent.physicalFrame);
        Ras toParentPofInParent = RasInParent(mg.GetRasInGround(joint.Parent), joint.Center);
        parentPOF->set_translation(SimTKVec3FromV3(toParentPofInParent.shift));
        parentPOF->set_orientation(SimTKVec3FromV3(toParentPofInParent.rot));

        // create the child OpenSim::PhysicalOffsetFrame
        auto childPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        childPOF->setName(child.physicalFrame->getName() + "_offset");
        childPOF->setParentFrame(*child.physicalFrame);
        Ras toChildPofInChild = RasInParent(mg.GetRasInGround(joint.Child), joint.Center);
        childPOF->set_translation(SimTKVec3FromV3(toChildPofInChild.shift));
        childPOF->set_orientation(SimTKVec3FromV3(toChildPofInChild.rot));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto jointUniqPtr = ConstructOpenSimJointFromTypeIndex(joint.JointTypeIndex);

        // set its name
        std::string jointName = CalcJointName(joint, *parent.physicalFrame, *child.physicalFrame);
        jointUniqPtr->setName(jointName);

        // set joint coordinate names
        SetJointCoordinateNames(*jointUniqPtr, jointName);

        // add + connect the joint to the POFs
        jointUniqPtr->addFrame(parentPOF.get());
        jointUniqPtr->addFrame(childPOF.get());
        jointUniqPtr->connectSocket_parent_frame(*parentPOF);
        jointUniqPtr->connectSocket_child_frame(*childPOF);
        parentPOF.release();
        childPOF.release();

        // if a child body was created during this step (e.g. because it's not a cyclic connection)
        // then add it to the model
        OSC_ASSERT_ALWAYS(parent.createdBody == nullptr && "at this point in the algorithm, all parents should have already been created");
        if (child.createdBody) {
            model.addBody(child.createdBody.release());  // add created body to model
        }

        // add the joint to the model
        model.addJoint(jointUniqPtr.release());

        // recurse by finding where the child of this joint is the parent of some other joint
        OSC_ASSERT_ALWAYS(child.bodyEl != nullptr && "child should always be an identifiable body element");
        for (auto const& [otherJointID, otherJoint] : mg.GetJoints()) {
            if (otherJoint.Parent == child.bodyEl->ID) {
                AttachJointRecursive(mg, model, otherJoint, visitedBodies, visitedJoints);
            }
        }
    }

    // attaches `BodyEl` into `model` by directly attaching it to ground with a FreeJoint
    void AttachBodyDirectlyToGround(ModelGraph const& mg,
                                    OpenSim::Model& model,
                                    BodyEl const& bodyEl,
                                    std::unordered_map<UID, OpenSim::Body*>& visitedBodies)
    {
        auto addedBody = CreateDetatchedBody(mg, bodyEl);
        auto freeJoint = std::make_unique<OpenSim::FreeJoint>();

        // set joint name
        freeJoint->setName(bodyEl.Name + "_to_ground");

        // set joint coordinate names
        SetJointCoordinateNames(*freeJoint, bodyEl.Name);

        // set joint's default location of the body's xform in ground
        freeJoint->upd_coordinates(0).setDefaultValue(static_cast<double>(bodyEl.Xform.rot[0]));
        freeJoint->upd_coordinates(1).setDefaultValue(static_cast<double>(bodyEl.Xform.rot[1]));
        freeJoint->upd_coordinates(2).setDefaultValue(static_cast<double>(bodyEl.Xform.rot[2]));
        freeJoint->upd_coordinates(3).setDefaultValue(static_cast<double>(bodyEl.Xform.shift[0]));
        freeJoint->upd_coordinates(4).setDefaultValue(static_cast<double>(bodyEl.Xform.shift[1]));
        freeJoint->upd_coordinates(5).setDefaultValue(static_cast<double>(bodyEl.Xform.shift[2]));

        // connect joint from ground to the body
        freeJoint->connectSocket_parent_frame(model.getGround());
        freeJoint->connectSocket_child_frame(*addedBody);

        // populate it in the "already visited bodies" cache
        visitedBodies[bodyEl.ID] = addedBody.get();

        // add the body + joint to the output model
        model.addBody(addedBody.release());
        model.addJoint(freeJoint.release());
    }

    // if there are no issues, returns a new OpenSim::Model created from the Modelgraph
    //
    // otherwise, returns nullptr and issuesOut will be populated with issue messages
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromModelGraph(ModelGraph const& mg,
                                                                     std::vector<std::string>& issuesOut)
    {
        if (GetModelGraphIssues(mg, issuesOut)) {
            log::error("cannot create an osim model: issues detected");
            for (std::string const& issue : issuesOut) {
                log::error("issue: %s", issue.c_str());
            }
            return nullptr;
        }

        // create the output model
        auto model = std::make_unique<OpenSim::Model>();
        model->updDisplayHints().upd_show_frames() = true;

        // add any meshes that are directly connected to ground (i.e. meshes that are not attached to a body)
        for (auto const& [meshID, meshEl] : mg.GetMeshes()) {
            if (meshEl.Attachment == g_GroundID) {
                AttachMeshElToFrame(meshEl, Ras{}, model->updGround());
            }
        }

        // keep track of any bodies/joints already visited (there might be cycles)
        std::unordered_map<UID, OpenSim::Body*> visitedBodies;
        std::unordered_set<UID> visitedJoints;

        // directly connect any bodies that participate in no joints into the model with a freejoint
        for (auto const& [bodyID, bodyEl] : mg.GetBodies()) {
            if (!IsAChildAttachmentInAnyJoint(mg, bodyEl)) {
                AttachBodyDirectlyToGround(mg, *model, bodyEl, visitedBodies);
            }
        }

        // add bodies that do participate in joints into the model
        //
        // note: these bodies may use the non-participating bodies (above) as parents
        for (auto const& [jointID, jointEl] : mg.GetJoints()) {
            if (jointEl.Parent == g_GroundID || ContainsKey(visitedBodies, jointEl.Parent)) {
                AttachJointRecursive(mg, *model, jointEl, visitedBodies, visitedJoints);
            }
        }

        return model;
    }
}

// undo/redo/snapshot support
//
// the editor has to support undo/redo/snapshots, because it's feasible that the user
// will want to undo a change they make.
//
// this implementation leans on the fact that the modelgraph (above) tries to follow value
// semantics, so copying an entire modelgraph into a buffer results in an independent copy
// that can't be indirectly mutated via references from other copies
namespace {

    // a single immutable and independent snapshot of the model, with a commit message + time
    // explaining what the snapshot "is" (e.g. "loaded file", "rotated body") and when it was
    // created
    class ModelGraphSnapshot {
    public:
        ModelGraphSnapshot(ModelGraph const& modelGraph,
                           std::string_view commitMessage) :

            m_ModelGraph{modelGraph},
            m_CommitMessage{commitMessage},
            m_CommitTime{std::chrono::system_clock::now()}
        {
        }

        ModelGraph const& GetModelGraph() const { return m_ModelGraph; }
        std::string const& GetCommitMessage() const { return m_CommitMessage; }
        std::chrono::system_clock::time_point const& GetCommitTime() const { return m_CommitTime; }

    private:
        ModelGraph m_ModelGraph;
        std::string m_CommitMessage;
        std::chrono::system_clock::time_point m_CommitTime;
    };


    // undoable model graph storage
    class SnapshottableModelGraph final {
    public:
        SnapshottableModelGraph() :
            m_Current{},
            m_Snapshots{ModelGraphSnapshot{m_Current, "created model"}},
            m_CurrentIsBasedOn{0}
        {
        }

        ModelGraph& Current() { return m_Current; }
        ModelGraph const& Current() const { return m_Current; }

        void CreateSnapshot(ModelGraph const& src, std::string_view commitMessage)
        {
            m_Snapshots.emplace_back(src, commitMessage);
            m_CurrentIsBasedOn = m_Snapshots.size()-1;
        }

        void CommitCurrent(std::string_view commitMessage)
        {
            CreateSnapshot(m_Current, commitMessage);
        }

        std::vector<ModelGraphSnapshot> const& GetSnapshots() const { return m_Snapshots; }

        size_t GetCurrentIsBasedOnIdx() const { return m_CurrentIsBasedOn; }

        void UseSnapshot(size_t i)
        {
            m_Current = m_Snapshots.at(i).GetModelGraph();
            m_CurrentIsBasedOn = i;
        }

        bool CanUndo() const
        {
            return m_CurrentIsBasedOn > 0;
        }

        void Undo()
        {
            if (m_Snapshots.empty()) {
                return;  // this shouldn't happen, but I'm paranoid
            }

            m_CurrentIsBasedOn = m_CurrentIsBasedOn <= 0 ? 0 : m_CurrentIsBasedOn-1;
            m_Current = m_Snapshots.at(m_CurrentIsBasedOn).GetModelGraph();
        }

        bool CanRedo() const
        {
            if (m_Snapshots.empty()) {
                return false;
            }

            return m_CurrentIsBasedOn < m_Snapshots.size()-1;
        }

        void Redo()
        {
            if (m_Snapshots.empty()) {
                return;  // this shouldn't happen, but I'm paranoid
            }

            size_t lastSnapshot = m_Snapshots.size()-1;

            m_CurrentIsBasedOn = m_CurrentIsBasedOn >= lastSnapshot ? lastSnapshot : m_CurrentIsBasedOn+1;
            m_Current = m_Snapshots.at(m_CurrentIsBasedOn).GetModelGraph();
        }

    private:
        ModelGraph m_Current;  // mutatable staging area from which commits can be made
        std::vector<ModelGraphSnapshot> m_Snapshots;  // frozen snapshots of previously-made states
        size_t m_CurrentIsBasedOn;
    };
}

// rendering support
//
// this code exists to make the modelgraph, and any other decorations (lines, hovers, selections, etc.)
// renderable in the UI
namespace {

    // returns a transform that maps a sphere mesh (defined to be @ 0,0,0 with radius 1)
    // to some sphere in the scene (e.g. a body/ground)
    glm::mat4 SphereMeshToSceneSphereXform(Sphere const& sceneSphere)
    {
        Sphere sphereMesh{{0.0f, 0.0f, 0.0f}, 1.0f};
        return SphereToSphereXform(sphereMesh, sceneSphere);
    }


    // returns a quad used for rendering the chequered floor
    Mesh generateFloorMesh()
    {
        Mesh m{GenTexturedQuad()};
        m.scaleTexCoords(200.0f);
        return m;
    }

    // returns a multiampled render buffer with the given format + dimensions
    gl::RenderBuffer MultisampledRenderBuffer(int samples, GLenum format, glm::ivec2 dims)
    {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, dims.x, dims.y);
        return rv;
    }

    // returns a non-multisampled render buffer with the given format + dimensions
    gl::RenderBuffer RenderBuffer(GLenum format, glm::ivec2 dims)
    {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorage(GL_RENDERBUFFER, format, dims.x, dims.y);
        return rv;
    }

    // sets the supplied texture with the appropriate dimensions, parameters, etc. to be used as a scene texture
    void SetTextureAsSceneTextureTex(gl::Texture2D& out, GLint level, GLint internalFormat, glm::ivec2 dims, GLenum format, GLenum type)
    {
        gl::BindTexture(out);
        gl::TexImage2D(out.type, level, internalFormat, dims.x, dims.y, 0, format, type, nullptr);
        gl::TexParameteri(out.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(out.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(out.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(out.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(out.type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl::BindTexture();
    }

    // returns a texture as a scene texture (specific params, etc.) with the given format, dims, etc.
    gl::Texture2D SceneTex(GLint level, GLint internalFormat, glm::ivec2 dims, GLenum format, GLenum type)
    {
        gl::Texture2D rv;
        SetTextureAsSceneTextureTex(rv, level, internalFormat, dims, format, type);
        return rv;
    }

    // declares a type that can bind an OpenGL buffer type to an FBO in the current OpenGL context
    struct FboBinding {
        virtual ~FboBinding() noexcept = default;
        virtual void Bind() = 0;
    };

    // defines a way of binding to a render buffer to the current FBO
    struct RboBinding final : FboBinding {
        GLenum attachment;
        gl::RenderBuffer& rbo;

        RboBinding(GLenum attachment_, gl::RenderBuffer& rbo_) :
            attachment{attachment_}, rbo{rbo_}
        {
        }

        void Bind() override
        {
            gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, rbo);
        }
    };

    // defines a way of binding to a texture buffer to the current FBO
    struct TexBinding final : FboBinding {
        GLenum attachment;
        gl::Texture2D& tex;
        GLint level;

        TexBinding(GLenum attachment_, gl::Texture2D& tex_, GLint level_) :
            attachment{attachment_}, tex{tex_}, level{level_}
        {
        }

        void Bind() override
        {
            gl::FramebufferTexture2D(GL_FRAMEBUFFER, attachment, tex, level);
        }
    };

    // returns an OpenGL framebuffer that is bound to the specified `FboBinding`s
    template<typename... Binding>
    gl::FrameBuffer FrameBufferWithBindings(Binding... bindings)
    {
        gl::FrameBuffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        (bindings.Bind(), ...);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
        return rv;
    }

    // something that is being drawn in the scene
    struct DrawableThing final {
        UID id = g_EmptyID;
        UID groupId = g_EmptyID;
        std::shared_ptr<Mesh> mesh;
        glm::mat4x3 modelMatrix;
        glm::mat3x3 normalMatrix;
        glm::vec4 color;
        float rimColor;
        std::shared_ptr<gl::Texture2D> maybeDiffuseTex;
    };

    AABB CalcBounds(DrawableThing const& dt)
    {
        return CalcBounds(*dt.mesh, dt.modelMatrix);
    }

    // an instance of something that is being drawn, once uploaded to the GPU
    struct SceneGPUInstanceData final {
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 rgba;
    };

    // a predicate used for drawcall ordering
    bool OptimalDrawOrder(DrawableThing const& a, DrawableThing const& b)
    {
        if (a.color.a != b.color.a) {
            return a.color.a > b.color.a;  // alpha descending
        } else {
            return a.mesh < b.mesh;
        }
    }

    // draws the drawables to the output texture
    //
    // effectively, this is the main top-level rendering function
    void DrawScene(glm::ivec2 dims,
                   PolarPerspectiveCamera const& camera,
                   glm::vec4 const& bgCol,
                   nonstd::span<DrawableThing const> drawables,
                   gl::Texture2D& outSceneTex)
    {
        glm::vec3 lightDir;
        {
            glm::vec3 p = glm::normalize(-camera.focusPoint - camera.getPos());
            glm::vec3 up = {0.0f, 1.0f, 0.0f};
            glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.25f * fpi4, up) * glm::vec4{p, 0.0f};
            lightDir = glm::normalize(mp + -up);
        }

        glm::vec3 lightCol = {1.0f, 1.0f, 1.0f};

        glm::mat4 projMat = camera.getProjMtx(VecAspectRatio(dims));
        glm::mat4 viewMat = camera.getViewMtx();
        glm::vec3 viewPos = camera.getPos();

        auto samples = App::cur().getSamples();

        gl::RenderBuffer sceneRBO = MultisampledRenderBuffer(samples, GL_RGB, dims);
        gl::RenderBuffer sceneDepth24Stencil8RBO = MultisampledRenderBuffer(samples, GL_DEPTH24_STENCIL8, dims);
        gl::FrameBuffer sceneFBO = FrameBufferWithBindings(
            RboBinding{GL_COLOR_ATTACHMENT0, sceneRBO},
            RboBinding{GL_DEPTH_STENCIL_ATTACHMENT, sceneDepth24Stencil8RBO});

        gl::Viewport(0, 0, dims.x, dims.y);

        gl::BindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        gl::ClearColor(bgCol);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the scene to the scene FBO
        if (true) {
            GouraudShader& shader = App::cur().getShaderCache().getShader<GouraudShader>();

            gl::UseProgram(shader.program);
            gl::Uniform(shader.uProjMat, projMat);
            gl::Uniform(shader.uViewMat, viewMat);
            gl::Uniform(shader.uLightDir, lightDir);
            gl::Uniform(shader.uLightColor, lightCol);
            gl::Uniform(shader.uViewPos, viewPos);
            for (auto const& d : drawables) {
                gl::Uniform(shader.uModelMat, d.modelMatrix);
                gl::Uniform(shader.uNormalMat, d.normalMatrix);
                gl::Uniform(shader.uDiffuseColor, d.color);
                if (d.maybeDiffuseTex) {
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(*d.maybeDiffuseTex);
                    gl::Uniform(shader.uIsTextured, true);
                    gl::Uniform(shader.uSampler0, GL_TEXTURE0-GL_TEXTURE0);
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }
                gl::BindVertexArray(d.mesh->GetVertexArray());
                d.mesh->Draw();
                gl::BindVertexArray();
            }
        }

        // blit it to the (non-MSXAAed) output texture

        SetTextureAsSceneTextureTex(outSceneTex, 0, GL_RGBA, dims, GL_RGBA, GL_UNSIGNED_BYTE);
        gl::FrameBuffer outputFBO = FrameBufferWithBindings(
            TexBinding{GL_COLOR_ATTACHMENT0, outSceneTex, 0}
        );

        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFBO);
        gl::BlitFramebuffer(0, 0, dims.x, dims.y, 0, 0, dims.x, dims.y, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // draw rims directly over the output texture
        if (true) {
            gl::Texture2D rimsTex;
            SetTextureAsSceneTextureTex(rimsTex, 0, GL_RED, dims, GL_RED, GL_UNSIGNED_BYTE);
            gl::FrameBuffer rimsFBO = FrameBufferWithBindings(
                TexBinding{GL_COLOR_ATTACHMENT0, rimsTex, 0}
            );

            gl::BindFramebuffer(GL_FRAMEBUFFER, rimsFBO);
            gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT);

            SolidColorShader& scs = App::cur().getShaderCache().getShader<SolidColorShader>();
            gl::UseProgram(scs.program);
            gl::Uniform(scs.uProjection, projMat);
            gl::Uniform(scs.uView, viewMat);

            gl::Disable(GL_DEPTH_TEST);
            for (auto const& d : drawables) {
                if (d.rimColor <= 0.05f) {
                    continue;
                }

                gl::Uniform(scs.uColor, {d.rimColor, 0.0f, 0.0f, 1.0f});
                gl::Uniform(scs.uModel, d.modelMatrix);
                gl::BindVertexArray(d.mesh->GetVertexArray());
                d.mesh->Draw();
                gl::BindVertexArray();
            }
            gl::Enable(GL_DEPTH_TEST);


            gl::BindFramebuffer(GL_FRAMEBUFFER, outputFBO);
            EdgeDetectionShader& eds = App::cur().getShaderCache().getShader<EdgeDetectionShader>();
            gl::UseProgram(eds.program);
            gl::Uniform(eds.uMVP, gl::identity);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(rimsTex);
            gl::Uniform(eds.uSampler0, gl::textureIndex<GL_TEXTURE0>());
            gl::Uniform(eds.uRimRgba,  glm::vec4{0.4f, 0.4f, 0.4f, 0.65f});
            gl::Uniform(eds.uRimThickness, 1.0f / VecLongestDimVal(dims));
            auto quadMesh = App::meshes().getTexturedQuadMesh();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            gl::Enable(GL_BLEND);
            gl::BindVertexArray(quadMesh->GetVertexArray());
            quadMesh->Draw();
            gl::BindVertexArray();
        }

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }
}

// shared data support
//
// Data that's shared between multiple UI states.
namespace {

    class Hover final {
    public:
        Hover() : ID{g_EmptyID}, Pos{} {}
        Hover(UID id_, glm::vec3 pos_) : ID{id_}, Pos{pos_} {}
        operator bool () const noexcept { return ID != g_EmptyID; }
        void reset() { *this = Hover{}; }

        UID ID;
        glm::vec3 Pos;
    };

    class SharedData final {
    public:
        SharedData() = default;

        SharedData(std::vector<std::filesystem::path> meshFiles)
        {
            PushMeshLoadRequests(meshFiles);
        }

        bool HasOutputModel() const
        {
            return m_MaybeOutputModel.get() != nullptr;
        }

        std::unique_ptr<OpenSim::Model>& UpdOutputModel()
        {
            return m_MaybeOutputModel;
        }

        void TryCreateOutputModel()
        {
            m_MaybeOutputModel = CreateOpenSimModelFromModelGraph(GetModelGraph(), m_IssuesBuffer);
        }

        ModelGraph const& GetModelGraph() const
        {
            return m_ModelGraphSnapshots.Current();
        }

        ModelGraph& UpdModelGraph()
        {
            return m_ModelGraphSnapshots.Current();
        }

        void CommitCurrentModelGraph(std::string_view commitMsg)
        {
            m_ModelGraphSnapshots.CommitCurrent(commitMsg);
        }

        std::vector<ModelGraphSnapshot> const& GetModelGraphSnapshots() const
        {
            return m_ModelGraphSnapshots.GetSnapshots();
        }

        size_t GetModelGraphIsBasedOn() const
        {
            return m_ModelGraphSnapshots.GetCurrentIsBasedOnIdx();
        }

        void UseModelGraphSnapshot(size_t i)
        {
            m_ModelGraphSnapshots.UseSnapshot(i);
        }

        bool CanUndoCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.CanUndo();
        }

        void UndoCurrentModelGraph()
        {
            m_ModelGraphSnapshots.Undo();
        }

        bool CanRedoCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.CanRedo();
        }

        void RedoCurrentModelGraph()
        {
            m_ModelGraphSnapshots.Redo();
        }

        void ResetModelGraph()
        {
            m_ModelGraphSnapshots = SnapshottableModelGraph{};
        }

        std::unordered_set<UID> const& GetCurrentSelection() const
        {
            return GetModelGraph().GetSelected();
        }

        void SelectAll()
        {
            UpdModelGraph().SelectAll();
        }

        void DeSelectAll()
        {
            UpdModelGraph().DeSelectAll();
        }

        void Select(UID id)
        {
            UpdModelGraph().Select(id);
        }

        void DeSelect(UID id)
        {
            UpdModelGraph().DeSelect(id);
        }

        bool HasSelection() const
        {
            return GetModelGraph().HasSelection();
        }

        bool IsSelected(UID id) const
        {
            return GetModelGraph().IsSelected(id);
        }

        void DeleteSelected()
        {
            if (!HasSelection()) {
                return;
            }
            UpdModelGraph().DeleteSelected();
            CommitCurrentModelGraph("deleted selection");
        }

        UIDT<BodyEl> AddBody(std::string const& name, glm::vec3 const& shift, glm::vec3 const& rot)
        {
            auto id = UpdModelGraph().AddBody(name, Ras{rot, shift});
            UpdModelGraph().DeSelectAll();
            UpdModelGraph().Select(id);
            CommitCurrentModelGraph(std::string{"added "} + name);
            return id;
        }

        UIDT<BodyEl> AddBody(glm::vec3 const& pos)
        {
            return AddBody(GenerateBodyName(), pos, {});
        }

        void UnassignMesh(MeshEl const& me)
        {
            UpdModelGraph().UnsetMeshAttachmentPoint(me.ID);
            std::stringstream ss;
            ss << "unassigned '" << me.Name << "' back to ground";
            CommitCurrentModelGraph(std::move(ss).str());
        }

        void PushMeshLoadRequests(UIDT<BodyEl> bodyToAttachTo, std::vector<std::filesystem::path> paths)
        {
            m_MeshLoader.send(MeshLoadRequest{bodyToAttachTo, std::move(paths)});
        }

        void PushMeshLoadRequests(std::vector<std::filesystem::path> paths)
        {
            PushMeshLoadRequests(g_GroundID, std::move(paths));
        }

        void PushMeshLoadRequest(UIDT<BodyEl> bodyToAttachTo, std::filesystem::path const& path)
        {
            PushMeshLoadRequests(bodyToAttachTo, std::vector<std::filesystem::path>{path});
        }

        void PushMeshLoadRequest(std::filesystem::path const& meshFilePath)
        {
            PushMeshLoadRequest(g_GroundID, meshFilePath);
        }

        // called when the mesh loader responds with a fully-loaded mesh
        void PopMeshLoader_OnOKResponse(MeshLoadOKResponse& ok)
        {
            ModelGraph& mg = UpdModelGraph();

            mg.DeSelectAll();
            for (LoadedMesh const& lm : ok.Meshes) {
                auto meshID = mg.AddMesh(lm.Mesh, ok.PreferredAttachmentPoint, lm.Path);

                auto const* maybeBody = mg.TryGetBodyElByID(ok.PreferredAttachmentPoint);
                if (maybeBody) {
                    mg.Select(maybeBody->ID);
                    mg.SetMeshXform(meshID, maybeBody->Xform);
                }

                mg.Select(meshID);
            }

            std::stringstream commitMsgSS;
            if (ok.Meshes.empty()) {
                commitMsgSS << "loaded 0 meshes";
            } else if (ok.Meshes.size() == 1) {
                commitMsgSS << "loaded " << ok.Meshes[0].Path.filename();
            } else {
                commitMsgSS << "loaded " << ok.Meshes.size() << " meshes";
            }

            CommitCurrentModelGraph(std::move(commitMsgSS).str());
        }

        // called when the mesh loader responds with a mesh loading error
        void PopMeshLoader_OnErrorResponse(MeshLoadErrorResponse& err)
        {
            log::error("%s: error loading mesh file: %s", err.Path.string().c_str(), err.Error.c_str());
        }

        void PopMeshLoader()
        {
            for (auto maybeResponse = m_MeshLoader.poll(); maybeResponse.has_value(); maybeResponse = m_MeshLoader.poll()) {
                MeshLoadResponse& meshLoaderResp = *maybeResponse;

                if (std::holds_alternative<MeshLoadOKResponse>(meshLoaderResp)) {
                    PopMeshLoader_OnOKResponse(std::get<MeshLoadOKResponse>(meshLoaderResp));
                } else {
                    PopMeshLoader_OnErrorResponse(std::get<MeshLoadErrorResponse>(meshLoaderResp));
                }
            }
        }

        std::vector<std::filesystem::path> PromptUserForMeshFiles() const
        {
            return PromptUserForFiles("obj,vtp,stl");
        }

        void PromptUserForMeshFilesAndPushThemOntoMeshLoader()
        {
            PushMeshLoadRequests(PromptUserForMeshFiles());
        }

        glm::vec2 WorldPosToScreenPos(glm::vec3 const& worldPos) const
        {
            return GetCamera().projectOntoScreenRect(worldPos, Get3DSceneRect());
        }

        void DrawConnectionLine(ImU32 color, glm::vec2 parent, glm::vec2 child) const
        {
            // triangle indicating connection directionality
            constexpr float lineWidth = 1.0f;
            constexpr float triangleWidth = 6.0f * lineWidth;
            constexpr float triangleWidthSquared = triangleWidth*triangleWidth;

            // the line
            ImGui::GetWindowDrawList()->AddLine(parent, child, color, lineWidth);

            glm::vec2 child2parent = parent - child;
            if (glm::dot(child2parent, child2parent) > triangleWidthSquared) {
                glm::vec2 midpoint = (parent + child) / 2.0f;
                glm::vec2 direction = glm::normalize(child2parent);
                glm::vec2 normal = {-direction.y, direction.x};

                glm::vec2 p1 = midpoint + (triangleWidth/2.0f)*normal;
                glm::vec2 p2 = midpoint - (triangleWidth/2.0f)*normal;
                glm::vec2 p3 = midpoint + triangleWidth*direction;

                ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, color);
            }
        }

        void DrawConnectionLine(MeshEl const& meshEl, ImU32 color) const
        {
            glm::vec3 meshLoc = meshEl.Xform.shift;
            glm::vec3 otherLoc = GetModelGraph().GetShiftInGround(meshEl.Attachment);

            DrawConnectionLine(color, WorldPosToScreenPos(otherLoc), WorldPosToScreenPos(meshLoc));
        }

        void DrawConnectionLineToGround(BodyEl const& bodyEl, ImU32 color) const
        {
            glm::vec3 bodyLoc = bodyEl.Xform.shift;
            glm::vec3 otherLoc = {};

            DrawConnectionLine(color, WorldPosToScreenPos(otherLoc), WorldPosToScreenPos(bodyLoc));
        }

        void DrawConnectionLine(JointEl const& jointEl, ImU32 color, UID excludeID = g_EmptyID) const
        {
            if (jointEl.ID == excludeID) {
                return;
            }

            glm::vec3 const& pivotLoc = jointEl.Center.shift;

            if (jointEl.Child != excludeID) {
                glm::vec3 childLoc = GetModelGraph().GetShiftInGround(jointEl.Child);
                DrawConnectionLine(color, WorldPosToScreenPos(pivotLoc), WorldPosToScreenPos(childLoc));
            }

            if (jointEl.Parent != excludeID) {
                glm::vec3 parentLoc = GetModelGraph().GetShiftInGround(jointEl.Parent);
                DrawConnectionLine(color, WorldPosToScreenPos(parentLoc), WorldPosToScreenPos(pivotLoc));
            }
        }


        void DrawConnectionLines(ImVec4 colorVec, UID excludeID = g_EmptyID) const
        {
            ModelGraph const& mg = GetModelGraph();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);

            // draw each mesh's connection line
            if (IsShowingMeshConnectionLines()) {
                for (auto const& [meshID, meshEl] : mg.GetMeshes()) {
                    if (meshID == excludeID) {
                        continue;
                    }

                    DrawConnectionLine(meshEl, color);
                }
            }

            // draw connection lines for bodies that have a direct (implicit) connection to ground
            // because they do not participate as a child of any joint
            if (IsShowingBodyConnectionLines()) {
                for (auto const& [bodyID, bodyEl] : mg.GetBodies()) {
                    if (bodyID == excludeID) {
                        continue;
                    }

                    if (IsAChildAttachmentInAnyJoint(mg, bodyEl)) {
                        continue;  // will be handled during joint drawing
                    }

                    DrawConnectionLineToGround(bodyEl, color);
                }
            }

            // draw connection lines for each joint
            if (IsShowingJointConnectionLines()) {
                for (auto const& [jointID, jointEl] : mg.GetJoints()) {
                    if (jointID == excludeID) {
                        continue;
                    }

                    DrawConnectionLine(jointEl, color, excludeID);
                }
            }
        }

        void DrawConnectionLines() const
        {
            DrawConnectionLines(m_Colors.FaintConnection);
        }


        void SetContentRegionAvailAsSceneRect()
        {
            Set3DSceneRect(ContentRegionAvailScreenRect());
        }

        void DrawScene(nonstd::span<DrawableThing> drawables)
        {
            // sort for (potentially) instanced rendering
            Sort(drawables, OptimalDrawOrder);

            // draw 3D scene to texture
            ::DrawScene(
                RectDims(Get3DSceneRect()),
                GetCamera(),
                GetColorSceneBackground(),
                drawables,
                UpdSceneTex());

            // send texture to ImGui
            DrawTextureAsImGuiImage(UpdSceneTex(), RectDims(Get3DSceneRect()));

            // handle hittesting, etc.
            SetIsRenderHovered(ImGui::IsItemHovered());
        }

        bool IsRenderHovered() const { return m_IsRenderHovered; }

        void SetIsRenderHovered(bool newIsHovered) { m_IsRenderHovered = newIsHovered; }

        Rect const& Get3DSceneRect() const { return m_3DSceneRect; }
        void Set3DSceneRect(Rect const& newRect) { m_3DSceneRect = newRect; }

        glm::vec2 Get3DSceneDims() const { return RectDims(m_3DSceneRect); }

        PolarPerspectiveCamera const& GetCamera() const { return m_3DSceneCamera; }
        PolarPerspectiveCamera& UpdCamera() { return m_3DSceneCamera; }
        void FocusCameraOn(glm::vec3 const& focusPoint)
        {
            m_3DSceneCamera.focusPoint = -focusPoint;
        }

        gl::Texture2D& UpdSceneTex() { return m_3DSceneTex; }



        // COLOR METHODS
        nonstd::span<glm::vec4 const> GetColors() const
        {
            static_assert(alignof(decltype(m_Colors)) == alignof(glm::vec4));
            static_assert(sizeof(m_Colors) % sizeof(glm::vec4) == 0);
            glm::vec4 const* start = reinterpret_cast<glm::vec4 const*>(&m_Colors);
            return {start, start + sizeof(m_Colors)/sizeof(glm::vec4)};
        }

        void SetColor(size_t i, glm::vec4 const& newColorValue)
        {
            reinterpret_cast<glm::vec4*>(&m_Colors)[i] = newColorValue;
        }

        nonstd::span<char const* const> GetColorLabels() const { return g_ColorNames; }

        glm::vec4 const& GetColorSceneBackground() const { return m_Colors.SceneBackground; }

        glm::vec4 const& GetColorMesh() const { return m_Colors.Mesh; }
        void SetColorMesh(glm::vec4 const& newColor) { m_Colors.Mesh = newColor; }

        glm::vec4 const& GetColorUnassignedMesh() const { return m_Colors.UnassignedMesh; }
        void GetColorUnassignedMesh(glm::vec4 const& newColor) { m_Colors.UnassignedMesh = newColor; }

        glm::vec4 const& GetColorBody() const { return m_Colors.Body; }
        glm::vec4 const& GetColorGround() const { return m_Colors.Ground; }

        glm::vec4 const& GetColorSolidConnectionLine() const { return m_Colors.SolidConnection; }
        void SetColorSolidConnectionLine(glm::vec4 const& newColor) { m_Colors.SolidConnection = newColor; }

        glm::vec4 const& GetColorTransparentFaintConnectionLine() const { return m_Colors.TransparentFaintConnection; }
        void SetColorTransparentFaintConnectionLine(glm::vec4 const& newColor) { m_Colors.TransparentFaintConnection = newColor; }

        glm::vec4 const& GetColorJointFrameCore() const { return m_Colors.JointFrameCore; }
        void SetColorJointFrameCore(glm::vec4 const& newColor) { m_Colors.JointFrameCore = newColor; }



        // VISIBILITY METHODS
        nonstd::span<bool const> GetVisibilityFlags() const
        {
            static_assert(alignof(decltype(m_VisibilityFlags)) == alignof(bool));
            static_assert(sizeof(m_VisibilityFlags) % sizeof(bool) == 0);
            bool const* start = reinterpret_cast<bool const*>(&m_VisibilityFlags);
            return {start, start + sizeof(m_VisibilityFlags)/sizeof(bool)};
        }

        void SetVisibilityFlag(size_t i, bool newVisibilityValue)
        {
            reinterpret_cast<bool*>(&m_VisibilityFlags)[i] = newVisibilityValue;
        }

        nonstd::span<char const* const> GetVisibilityFlagLabels() const { return g_VisibilityFlagNames; }

        bool IsShowingMeshes() const { return m_VisibilityFlags.Meshes; }
        void SetIsShowingMeshes(bool newIsShowing) { m_VisibilityFlags.Meshes = newIsShowing; }

        bool IsShowingBodies() const { return m_VisibilityFlags.Bodies; }
        void SetIsShowingBodies(bool newIsShowing) { m_VisibilityFlags.Bodies = newIsShowing; }

        bool IsShowingJointCenters() const { return m_VisibilityFlags.JointCenters; }
        void SetIsShowingJointCenters(bool newIsShowing) { m_VisibilityFlags.JointCenters = newIsShowing; }

        bool IsShowingGround() const { return m_VisibilityFlags.Ground; }
        void SetIsShowingGround(bool newIsShowing) { m_VisibilityFlags.Ground = newIsShowing; }

        bool IsShowingFloor() const { return m_VisibilityFlags.Floor; }
        void SetIsShowingFloor(bool newIsShowing) { m_VisibilityFlags.Floor = newIsShowing; }

        bool IsShowingJointConnectionLines() const { return m_VisibilityFlags.JointConnectionLines; }
        void SetIsShowingJointConnectionLines(bool newIsShowing) { m_VisibilityFlags.JointConnectionLines = newIsShowing; }

        bool IsShowingMeshConnectionLines() const { return m_VisibilityFlags.MeshConnectionLines; }
        void SetIsShowingMeshConnectionLines(bool newIsShowing) { m_VisibilityFlags.MeshConnectionLines = newIsShowing; }

        bool IsShowingBodyConnectionLines() const { return m_VisibilityFlags.BodyToGroundConnectionLines; }
        void SetIsShowingBodyConnectionLines(bool newIsShowing) { m_VisibilityFlags.BodyToGroundConnectionLines = newIsShowing; }


        // LOCKING/INTERACTIVITY METHODS
        nonstd::span<bool const> GetIneractivityFlags() const
        {
            static_assert(alignof(decltype(m_InteractivityFlags)) == alignof(bool));
            static_assert(sizeof(m_InteractivityFlags) % sizeof(bool) == 0);
            bool const* start = reinterpret_cast<bool const*>(&m_InteractivityFlags);
            return {start, start + sizeof(m_InteractivityFlags)/sizeof(bool)};
        }

        void SetInteractivityFlag(size_t i, bool newInteractivityValue)
        {
            reinterpret_cast<bool*>(&m_InteractivityFlags)[i] = newInteractivityValue;
        }

        nonstd::span<char const* const> GetInteractivityFlagLabels() const
        {
            return g_InteractivityFlagNames;
        }

        bool IsMeshesInteractable() const { return m_InteractivityFlags.Meshes; }
        void SetIsMeshesInteractable(bool newIsInteractable) { m_InteractivityFlags.Meshes = newIsInteractable; }

        bool IsBodiesInteractable() const { return m_InteractivityFlags.Bodies; }
        void SetIsBodiesInteractable(bool newIsInteractable) { m_InteractivityFlags.Bodies = newIsInteractable; }

        bool IsJointCentersInteractable() const { return m_InteractivityFlags.JointCenters; }
        void SetIsJointCentersInteractable(bool newIsInteractable) { m_InteractivityFlags.JointCenters = newIsInteractable; }

        bool IsGroundInteractable() const { return m_InteractivityFlags.Ground; }
        void SetIsGroundInteractable(bool newIsInteractable) { m_InteractivityFlags.Ground = newIsInteractable; }

        float GetSceneScaleFactor() const { return m_SceneScaleFactor; }
        void SetSceneScaleFactor(float newScaleFactor) { m_SceneScaleFactor = newScaleFactor; }

        glm::mat4 GetFloorModelMtx() const
        {
            glm::mat4 rv{1.0f};

            // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
            // floor down *slightly* to prevent Z fighting from planes rendered from the
            // model itself (the contact planes, etc.)
            rv = glm::translate(rv, {0.0f, -0.0001f, 0.0f});
            rv = glm::rotate(rv, fpi2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {m_SceneScaleFactor * 100.0f, m_SceneScaleFactor * 100.0f, 1.0f});

            return rv;
        }

        DrawableThing GenerateFloorDrawable() const
        {
            DrawableThing dt;
            dt.id = g_EmptyID;
            dt.groupId = g_EmptyID;
            dt.mesh = m_FloorMesh;
            dt.modelMatrix = GetFloorModelMtx();
            dt.normalMatrix = NormalMatrix(dt.modelMatrix);
            dt.color = m_Colors.FloorTint;
            dt.rimColor = 0.0f;
            dt.maybeDiffuseTex = m_FloorChequerTex;
            return dt;
        }

        DrawableThing GenerateMeshElDrawable(MeshEl const& meshEl) const
        {
            DrawableThing rv;
            rv.id = meshEl.ID;
            rv.groupId = g_MeshGroupID;
            rv.mesh = meshEl.MeshData;
            rv.modelMatrix = CalcXformMatrix(meshEl);
            rv.normalMatrix = NormalMatrix(rv.modelMatrix);
            rv.color = meshEl.Attachment == g_GroundID || meshEl.Attachment == g_EmptyID ? GetColorUnassignedMesh() : GetColorMesh();
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        float GetSphereRadius() const
        {
            return 0.02f * m_SceneScaleFactor;
        }

        Sphere SphereAtTranslation(glm::vec3 const& translation) const
        {
            return Sphere{translation, GetSphereRadius()};
        }

        DrawableThing GenerateBodyElSphere(BodyEl const& bodyEl, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = bodyEl.ID;
            rv.groupId = g_BodyGroupID;
            rv.mesh = m_SphereMesh;
            rv.modelMatrix = SphereMeshToSceneSphereXform(SphereAtTranslation(bodyEl.Xform.shift));
            rv.normalMatrix = NormalMatrix(rv.modelMatrix);
            rv.color = color;
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        DrawableThing GenerateGroundSphere(glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = g_GroundID;
            rv.groupId = g_GroundGroupID;
            rv.mesh = m_SphereMesh;
            rv.modelMatrix = SphereMeshToSceneSphereXform(SphereAtTranslation({0.0f, 0.0f, 0.0f}));
            rv.normalMatrix = NormalMatrix(rv.modelMatrix);
            rv.color = color;
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        void AppendAsFrame(UID logicalID,
                           UID groupID,
                           Ras const& xform,
                           std::vector<DrawableThing>& appendOut,
                           float alpha = 1.0f,
                           float rimAlpha = 0.0f,
                           glm::vec3 legLen = {1.0f, 1.0f, 1.0f},
                           glm::vec3 coreColor = {1.0f, 1.0f, 1.0f}) const
        {
            // stolen from SceneGeneratorNew.cpp

            glm::vec3 origin = xform.shift;
            glm::mat3 rotation = EulerAnglesToMat(xform.rot);

            // emit origin sphere
            {
                Sphere centerSphere{origin, GetSphereRadius()};

                DrawableThing& sphere = appendOut.emplace_back();
                sphere.id = logicalID;
                sphere.groupId = groupID;
                sphere.mesh = m_SphereMesh;
                sphere.modelMatrix = SphereMeshToSceneSphereXform(centerSphere);
                sphere.normalMatrix = NormalMatrix(sphere.modelMatrix);
                sphere.color = {coreColor.r, coreColor.g, coreColor.b, alpha};
                sphere.rimColor = rimAlpha;
                sphere.maybeDiffuseTex = nullptr;
            }

            // emit "legs"
            Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
            for (int i = 0; i < 3; ++i) {
                glm::vec3 dir = {0.0f, 0.0f, 0.0f};
                dir[i] = 4.0f * legLen[i] * GetSphereRadius();
                Segment axisline{origin, origin + rotation*dir};

                float frameAxisThickness = GetSphereRadius()/2.0f;
                glm::vec3 prescale = {frameAxisThickness, 1.0f, frameAxisThickness};
                glm::mat4 prescaleMtx = glm::scale(glm::mat4{1.0f}, prescale);
                glm::vec4 color{0.0f, 0.0f, 0.0f, alpha};
                color[i] = 1.0f;

                DrawableThing& se = appendOut.emplace_back();
                se.id = logicalID;
                se.groupId = groupID;
                se.mesh = m_CylinderMesh;
                se.modelMatrix = SegmentToSegmentXform(cylinderline, axisline) * prescaleMtx;
                se.normalMatrix = NormalMatrix(se.modelMatrix);
                se.color = color;
                se.rimColor = rimAlpha;
                se.maybeDiffuseTex = nullptr;
            }
        }

        void AppendAsCubeThing(UID logicalID,
                               UID groupID,
                               Ras const& xform,
                               std::vector<DrawableThing>& appendOut,
                               float alpha = 1.0f,
                               float rimAlpha = 0.0f,
                               glm::vec3 legLen = {1.0f, 1.0f, 1.0f},
                               glm::vec3 coreColor = {1.0f, 1.0f, 1.0f}) const
        {
            glm::mat4 baseMmtx = CalcXformMatrix(xform);

            float halfWidths = 1.5f * GetSphereRadius();
            glm::vec3 scaleFactors = halfWidths * glm::vec3{1.0f, 1.0f, 1.0f};

            glm::mat4 mmtx = baseMmtx * glm::scale(glm::mat4{1.0f}, glm::vec3{scaleFactors});

            {
                DrawableThing& originCube = appendOut.emplace_back();
                originCube.id = logicalID;
                originCube.groupId = groupID;
                originCube.mesh = App::cur().meshes().getBrickMesh();
                originCube.modelMatrix = mmtx;
                originCube.normalMatrix = NormalMatrix(mmtx);
                originCube.color = glm::vec4{coreColor, alpha};
                originCube.rimColor = rimAlpha;
                originCube.maybeDiffuseTex = nullptr;
            }

            // stretch origin cube for legs
            for (int i = 0; i < 3; ++i) {
                Segment coneLine{glm::vec3{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
                Segment outputLine{};
                outputLine.p1[i] = halfWidths;
                outputLine.p2[i] = 1.75f * halfWidths * legLen[i];

                glm::mat4 xform = SegmentToSegmentXform(coneLine, outputLine);
                xform = baseMmtx * xform * glm::scale(glm::mat4{1.0f}, glm::vec3{halfWidths/2.0f, 1.0f, halfWidths/2.0f});

                glm::vec4 color = {0.0f, 0.0f, 0.0f, alpha};
                color[i] = 1.0f;

                DrawableThing& legCube = appendOut.emplace_back();
                legCube.id = logicalID;
                legCube.groupId = groupID;
                legCube.mesh = App::cur().meshes().getConeMesh();
                legCube.modelMatrix = xform;
                legCube.normalMatrix = NormalMatrix(xform);
                legCube.color = color;
                legCube.rimColor = rimAlpha;
                legCube.maybeDiffuseTex = nullptr;
            }
        }

        void AppendBodyElAsCubeThing(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            AppendAsCubeThing(bodyEl.ID, g_BodyGroupID, bodyEl.Xform, appendOut);
        }

        void AppendBodyElAsFrame(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            AppendAsFrame(bodyEl.ID, g_BodyGroupID, bodyEl.Xform, appendOut);
        }

        Hover Hovertest(std::vector<DrawableThing> const& drawables) const
        {
            Rect sceneRect = Get3DSceneRect();
            glm::vec2 mousePos = ImGui::GetMousePos();

            if (!PointIsInRect(sceneRect, mousePos)) {
                return Hover{};
            }

            glm::vec2 sceneDims = RectDims(sceneRect);
            glm::vec2 relMousePos = mousePos - sceneRect.p1;

            Line ray = GetCamera().unprojectTopLeftPosToWorldRay(relMousePos, sceneDims);
            bool hittestMeshes = IsMeshesInteractable();
            bool hittestBodies = IsBodiesInteractable();
            bool hittestJointCenters = IsJointCentersInteractable();
            bool hittestGround = IsGroundInteractable();

            UID closestID = g_EmptyID;
            float closestDist = std::numeric_limits<float>::max();

            for (DrawableThing const& drawable : drawables) {
                if (drawable.id == g_EmptyID) {
                    continue;  // no hittest data
                }

                if (drawable.groupId == g_BodyGroupID && !hittestBodies) {
                    continue;
                }

                if (drawable.groupId == g_MeshGroupID && !hittestMeshes) {
                    continue;
                }

                if (drawable.groupId == g_JointGroupID && !hittestJointCenters) {
                    continue;
                }

                if (drawable.groupId == g_GroundGroupID && !hittestGround) {
                    continue;
                }

                RayCollision rc = drawable.mesh->getRayMeshCollisionInWorldspace(drawable.modelMatrix, ray);
                if (rc.hit && rc.distance < closestDist) {
                    closestID = drawable.id;
                    closestDist = rc.distance;
                }
            }

            glm::vec3 hitPos = closestID != g_EmptyID ? ray.origin + closestDist*ray.dir : glm::vec3{};

            return Hover{closestID, hitPos};
        }

        bool onEvent(SDL_Event const& e)
        {
            // if the user drags + drops a file into the window, assume it's a meshfile
            if (e.type == SDL_DROPFILE && e.drop.file != nullptr) {
                PushMeshLoadRequest(std::filesystem::path{e.drop.file});
                return true;
            }

            return false;
        }

        void tick(float)
        {
            // pop any background-loaded meshes
            PopMeshLoader();

            // if some screen generated an OpenSim::Model, transition to the main editor
            if (HasOutputModel()) {
                auto mainEditorState = std::make_shared<MainEditorState>(std::move(UpdOutputModel()));
                mainEditorState->editedModel.setFixupScaleFactor(mainEditorState->editedModel.getFixupScaleFactor());
                for (auto& viewerPtr : mainEditorState->viewers) {
                    if (viewerPtr) {
                        viewerPtr->requestAutoFocus();
                    }
                }

                App::cur().requestTransition<ModelEditorScreen>(mainEditorState);
            }
        }

    private:
        // model graph (snapshots) the user is working on
        SnapshottableModelGraph m_ModelGraphSnapshots;

        // loads meshes in a background thread
        MeshLoader m_MeshLoader;

        // sphere mesh used by various scene elements
        std::shared_ptr<Mesh> m_SphereMesh = std::make_shared<Mesh>(GenUntexturedUVSphere(12, 12));

        // cylinder mesh used by various scene elements
        std::shared_ptr<Mesh> m_CylinderMesh = std::make_shared<Mesh>(GenUntexturedSimbodyCylinder(16));

        // quad mesh used for chequered floor
        std::shared_ptr<Mesh> m_FloorMesh = std::make_shared<Mesh>(generateFloorMesh());

        // chequered floor texture
        std::shared_ptr<gl::Texture2D> m_FloorChequerTex = std::make_shared<gl::Texture2D>(genChequeredFloorTexture());

        // main 3D scene camera
        PolarPerspectiveCamera m_3DSceneCamera = []() {
            PolarPerspectiveCamera rv;
            rv.phi = fpi4;
            rv.theta = fpi4;
            rv.radius = 5.0f;
            return rv;
        }();

        // screenspace rect where the 3D scene is currently being drawn to
        Rect m_3DSceneRect = {};

        // texture the 3D scene is being rendered to
        //
        // CAREFUL: must survive beyond the end of the drawcall because ImGui needs it to be
        //          alive during rendering
        gl::Texture2D m_3DSceneTex;


        // COLORS
        //
        // these are runtime-editable color values for things in the scene
        struct {
            glm::vec4 Mesh{1.0f, 1.0f, 1.0f, 1.0f};
            glm::vec4 UnassignedMesh{1.0f, 0.95f, 0.95, 1.0f};
            glm::vec4 Ground{0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 Body{0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 FaintConnection{0.6f, 0.6f, 0.6f, 1.0f};
            glm::vec4 SolidConnection{0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 TransparentFaintConnection{0.0f, 0.0f, 0.0f, 0.2f};
            glm::vec4 SceneBackground{0.89f, 0.89f, 0.89f, 1.0f};
            glm::vec4 JointFrameCore{0.8f, 0.8f, 0.8f, 1.0f};
            glm::vec4 FloorTint{1.0f, 1.0f, 1.0f, 0.4f};
        } m_Colors;
        static constexpr std::array<char const*, 10> g_ColorNames = {
            "mesh",
            "unassigned mesh",
            "ground",
            "body",
            "faint connection line",
            "solid connection line",
            "transparent faint connection line",
            "scene background",
            "joint frame core",
            "floor tint",
        };
        static_assert(sizeof(decltype(m_Colors))/sizeof(glm::vec4) == g_ColorNames.size());


        // VISIBILITY
        //
        // these are runtime-editable visibility flags for things in the scene
        struct {
            bool Floor = true;
            bool Meshes = true;
            bool Ground = true;
            bool Bodies = true;
            bool JointCenters = true;
            bool JointConnectionLines = true;
            bool MeshConnectionLines = true;
            bool BodyToGroundConnectionLines = true;
        } m_VisibilityFlags;
        static constexpr std::array<char const*, 8> g_VisibilityFlagNames = {
            "floor",
            "meshes",
            "ground",
            "bodies",
            "joint centers",
            "joint connection lines",
            "mesh connection lines",
            "body-to-ground connection lines",
        };
        static_assert(sizeof(decltype(m_VisibilityFlags))/sizeof(bool) == g_VisibilityFlagNames.size());

        // LOCKING
        //
        // these are runtime-editable flags that dictate what gets hit-tested
        struct {
            bool Meshes = true;
            bool Bodies = true;
            bool JointCenters = true;
            bool Ground = true;
        } m_InteractivityFlags;
        static constexpr std::array<char const*, 4> g_InteractivityFlagNames = {
            "meshes",
            "bodies",
            "joint centers",
            "ground",
        };
        static_assert(sizeof(decltype(m_InteractivityFlags))/sizeof(bool) == g_InteractivityFlagNames.size());

    public:
        // WINDOWS
        //
        // these are runtime-editable flags that dictate which panels are open
        std::array<bool, 3> m_PanelStates{false, true, true};
        static constexpr std::array<char const*, 3> g_OpenedPanelNames = {
            "History",
            "Hierarchy",
            "Log",
        };
        enum PanelIndex_ {
            PanelIndex_History = 0,
            PanelIndex_Hierarchy,
            PanelIndex_Log,
            PanelIndex_COUNT,
        };
        LogViewer m_Logviewer;
    private:

        // scale factor for all non-mesh, non-overlay scene elements (e.g.
        // the floor, bodies)
        //
        // this is necessary because some meshes can be extremely small/large and
        // scene elements need to be scaled accordingly (e.g. without this, a body
        // sphere end up being much larger than a mesh instance). Imagine if the
        // mesh was the leg of a fly
        float m_SceneScaleFactor = 1.0f;

        // buffer containing issues found in the modelgraph
        std::vector<std::string> m_IssuesBuffer;

        // model created by this wizard
        //
        // `nullptr` until the model is successfully created
        std::unique_ptr<OpenSim::Model> m_MaybeOutputModel = nullptr;

        // set to true after drawing the ImGui::Image
        bool m_IsRenderHovered = false;
    };
}

// state pattern support
//
// The mesh importer UI isn't always in one particular state with one particular update->tick->draw
// algorithm. The UI could be in:
//
// - "standard editing mode"           showing the scene, handling hovering, tooltips, move things around, etc.
// - "assigning mesh mode"             shows the scene differently: only assignable things will be highlighted, etc.
// - "assigning joint mode (step 1)"   shows the scene differently: can only click other bodies to join to
// - "assigning joint mode (step 2)"   (e.g.) might be asking the user what joint type they want, etc.
// - (etc.)
//
// The UI needs to be able to handle these states without resorting to a bunch of `if` statements littering
// every method. The states also need to be able to transition into arbitrary other states because state
// transitions aren't always linear. For example, the user might start assigning a joint, but cancel out
// of doing that midway by pressing ESC.
namespace {

    // base class that declares the API that each state must implement
    class MWState {
    public:
        virtual ~MWState() noexcept = default;
        virtual std::unique_ptr<MWState> onEvent(SDL_Event const&) = 0;
        virtual std::unique_ptr<MWState> tick(float) = 0;
        virtual std::unique_ptr<MWState> draw() = 0;
    };

    // forward-declaration so that subscreens can transition to the standard top-level state
    std::unique_ptr<MWState> CreateStandardState(std::shared_ptr<SharedData>);


    // options for when the UI transitions into "choose two mesh points" mode
    struct SelectTwoMeshPointsOptions final {
        std::function<std::unique_ptr<MWState>(std::shared_ptr<SharedData>, glm::vec3, glm::vec3)> OnTwoPointsChosen =
            [](std::shared_ptr<SharedData> shared, glm::vec3, glm::vec3) { return CreateStandardState(shared); };
        std::function<std::unique_ptr<MWState>(std::shared_ptr<SharedData>)> OnUserCancelled =
                [](std::shared_ptr<SharedData> shared) { return CreateStandardState(shared); };
        std::string Header = "choose first (left-click) and second (right click) mesh positions (ESC to cancel)";
    };

    class SelectTwoMeshPointsState final : public MWState {
    public:
        SelectTwoMeshPointsState(std::shared_ptr<SharedData> shared, SelectTwoMeshPointsOptions options) :
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:

        void HandlePossibleTransitionToNextStep()
        {
            if (!m_MaybeFirstLocation) {
                return;
            }

            if (!m_MaybeSecondLocation) {
                return;
            }

            m_MaybeNextState = m_Options.OnTwoPointsChosen(m_Shared, *m_MaybeFirstLocation, *m_MaybeSecondLocation);
        }

        void HandleHovertestSideEffects()
        {
            if (!m_MaybeCurrentHover) {
                return;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // LEFT CLICK: set first mouse location
                m_MaybeFirstLocation = m_MaybeCurrentHover.Pos;
                HandlePossibleTransitionToNextStep();
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                // RIGHT CLICK: set second mouse location
                m_MaybeSecondLocation = m_MaybeCurrentHover.Pos;
                HandlePossibleTransitionToNextStep();
            }
        }

        std::vector<DrawableThing>& GenerateDrawables()
        {
            ModelGraph const& mg = m_Shared->GetModelGraph();

            for (auto const& [meshID, meshEl] : mg.GetMeshes()) {
                m_DrawablesBuffer.emplace_back(m_Shared->GenerateMeshElDrawable(meshEl));
            }

            m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());

            return m_DrawablesBuffer;
        }

        void DrawHoverTooltip()
        {
            if (!m_MaybeCurrentHover) {
                return;
            }

            ImGui::BeginTooltip();
            ImGui::Text("%s", PosString(m_MaybeCurrentHover.Pos).c_str());
            ImGui::TextDisabled("(left-click to assign as first point, right-click to assign as second point)");
            ImGui::EndTooltip();
        }

        void DrawOverlay()
        {
            if (!m_MaybeFirstLocation && !m_MaybeSecondLocation) {
                return;
            }

            glm::vec3 clickedWorldPos = m_MaybeFirstLocation ? *m_MaybeFirstLocation : *m_MaybeSecondLocation;
            glm::vec2 clickedScrPos = m_Shared->WorldPosToScreenPos(clickedWorldPos);

            auto color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddCircleFilled(clickedScrPos, 5.0f, color);

            if (!m_MaybeCurrentHover) {
                return;
            }

            glm::vec2 hoverScrPos = m_Shared->WorldPosToScreenPos(m_MaybeCurrentHover.Pos);

            dl->AddCircleFilled(hoverScrPos, 5.0f, color);
            dl->AddLine(clickedScrPos, hoverScrPos, color, 5.0f);
        }

        void DrawHeaderText() const
        {
            if (m_Options.Header.empty()) {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetWindowDrawList()->AddText({10.0f, 10.0f}, color, m_Options.Header.c_str());
        }

        void Draw3DViewer()
        {
            m_Shared->SetContentRegionAvailAsSceneRect();
            std::vector<DrawableThing>& drawables = GenerateDrawables();
            m_MaybeCurrentHover = m_Shared->Hovertest(drawables);
            HandleHovertestSideEffects();

            m_Shared->DrawScene(drawables);
            DrawOverlay();
            DrawHoverTooltip();
            DrawHeaderText();
        }

    public:
        std::unique_ptr<MWState> onEvent(SDL_Event const& e) override
        {
            m_Shared->onEvent(e);
            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> tick(float dt) override
        {
            m_Shared->tick(dt);

            if (!ImGui::GetIO().WantCaptureKeyboard && ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
                // ESC: user cancelled out
                m_MaybeNextState = m_Options.OnUserCancelled(m_Shared);
            }

            bool isRenderHovered = m_Shared->IsRenderHovered();

            if (isRenderHovered) {
                UpdatePolarCameraFromImGuiUserInput(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> draw() override
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("wizardsstep2viewer")) {
                ImGui::PopStyleVar();
                Draw3DViewer();
            } else {
                ImGui::PopStyleVar();
            }
            ImGui::End();

            return std::move(m_MaybeNextState);
        }

    private:
        // data that's shared between other UI states
        std::shared_ptr<SharedData> m_Shared;

        // options for this state
        SelectTwoMeshPointsOptions m_Options;

        // (maybe) next state to transition to
        std::unique_ptr<MWState> m_MaybeNextState = nullptr;

        // (maybe) user mouse hover
        Hover m_MaybeCurrentHover;

        // (maybe) first mesh location
        std::optional<glm::vec3> m_MaybeFirstLocation;

        // (maybe) second mesh location
        std::optional<glm::vec3> m_MaybeSecondLocation;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;
    };



    // options for when the UI transitions into "choose something" mode
    struct ChooseSomethingOptions final {
        bool CanChooseBodies = true;
        bool CanChooseGround = true;
        bool CanChooseMeshes = true;
        bool CanChooseJoints = true;
        UID MaybeElAttachingTo = g_EmptyID;
        bool IsAttachingTowardEl = true;  // false implies "away from"
        UID MaybeElBeingReplacedByChoice = g_EmptyID;
        std::function<std::unique_ptr<MWState>(std::shared_ptr<SharedData>, UID)> OnUserChoice =
                [](std::shared_ptr<SharedData> shared, UID) { return CreateStandardState(shared); };
        std::function<std::unique_ptr<MWState>(std::shared_ptr<SharedData>)> OnUserCancelled =
                [](std::shared_ptr<SharedData> shared) { return CreateStandardState(shared); };
        std::string Header = "choose something";
    };

    // "choose something" UI state
    //
    // this is what's drawn when the user's being prompted to choose something
    // else in the scene
    class ChooseSomethingMWState final : public MWState {
    public:
        ChooseSomethingMWState(std::shared_ptr<SharedData> shared, ChooseSomethingOptions options) :
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:
        void DrawMeshHoverTooltip(MeshEl const& meshEl) const
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(GetLabel(meshEl).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(Mesh, click to choose)");
            ImGui::EndTooltip();
        }

        void DrawBodyHoverTooltip(BodyEl const& bodyEl) const
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(GetLabel(bodyEl).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(Body, click to choose)");
            ImGui::EndTooltip();
        }

        void DrawJointHoverTooltip(JointEl const& jointEl) const
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(GetLabel(jointEl).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(Joint, click to choose)");
            ImGui::EndTooltip();
        }

        void DrawGroundHoverTooltip() const
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("ground");
            ImGui::SameLine();
            ImGui::TextDisabled("(click to choose)");
            ImGui::EndTooltip();
        }

        void DrawHoverTooltip() const
        {
            if (!m_MaybeHover) {
                return;
            } else if (m_MaybeHover.ID == g_GroundID) {
                DrawGroundHoverTooltip();
            } else if (MeshEl const* meshEl = m_Shared->GetModelGraph().TryGetMeshElByID(m_MaybeHover.ID)) {
                DrawMeshHoverTooltip(*meshEl);
            } else if (BodyEl const* bodyEl = m_Shared->GetModelGraph().TryGetBodyElByID(m_MaybeHover.ID)) {
                DrawBodyHoverTooltip(*bodyEl);
            } else if (JointEl const* jointEl = m_Shared->GetModelGraph().TryGetJointElByID(m_MaybeHover.ID)) {
                DrawJointHoverTooltip(*jointEl);
            }
        }

        void DrawConnectionLines() const
        {
            if (!m_MaybeHover) {
                // user isn't hovering anything, so just draw all existing connection
                // lines faintly
                m_Shared->DrawConnectionLines(m_Shared->GetColorTransparentFaintConnectionLine());
                return;
            }

            // else: user is hovering *something*

            // draw all other connection lines but exclude the thing being assigned (if any)
            m_Shared->DrawConnectionLines(m_Shared->GetColorTransparentFaintConnectionLine(), m_Options.MaybeElBeingReplacedByChoice);

            if (m_Options.MaybeElAttachingTo == g_EmptyID) {
                return;  // we don't know what the user's choice is ultimately attaching to
            }

            // draw strong connection line between the thing being attached to and the hover
            glm::vec2 parentScrPos = m_Shared->WorldPosToScreenPos(m_Shared->GetModelGraph().GetShiftInGround(m_Options.MaybeElAttachingTo));
            glm::vec2 childScrPos = m_Shared->WorldPosToScreenPos(m_Shared->GetModelGraph().GetShiftInGround(m_MaybeHover.ID));

            if (!m_Options.IsAttachingTowardEl) {
                std::swap(parentScrPos, childScrPos);
            }

            ImU32 strongColorU2 = ImGui::ColorConvertFloat4ToU32(m_Shared->GetColorSolidConnectionLine());

            m_Shared->DrawConnectionLine(strongColorU2, parentScrPos, childScrPos);
        }

        void DrawHeaderText() const
        {
            if (m_Options.Header.empty()) {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetWindowDrawList()->AddText({10.0f, 10.0f}, color, m_Options.Header.c_str());
        }

        // generates 3D scene
        std::vector<DrawableThing>& GenerateDrawables()
        {
            m_DrawablesBuffer.clear();

            ModelGraph const& mg = m_Shared->GetModelGraph();

            float fadedAlpha = 0.2f;

            // meshes
            for (auto const& [meshID, meshEl] : mg.GetMeshes()) {
                DrawableThing& d = m_DrawablesBuffer.emplace_back(m_Shared->GenerateMeshElDrawable(meshEl));

                if (meshID == m_MaybeHover.ID) {
                    d.rimColor = 0.8f;
                }

                bool isSelectable = meshID != m_Options.MaybeElAttachingTo && m_Options.CanChooseMeshes;

                if (!isSelectable) {
                    d.color.a = fadedAlpha;
                    d.id = g_EmptyID;
                    d.groupId = g_EmptyID;
                }
            }

            // bodies
            for (auto const& [bodyID, bodyEl] : mg.GetBodies()) {
                bool isSelectable = bodyID != m_Options.MaybeElAttachingTo && m_Options.CanChooseBodies;

                UID id = isSelectable ? bodyID : g_EmptyID;
                UID groupId = isSelectable ? g_BodyGroupID : g_EmptyID;
                float alpha = isSelectable ? 1.0f : 0.2f;
                float rimAlpha = bodyID == m_MaybeHover.ID ? 0.8f: 0.0f;

                m_Shared->AppendAsCubeThing(id, groupId, bodyEl.Xform, m_DrawablesBuffer, alpha, rimAlpha);
            }

            // joints
            for (auto const& [jointID, jointEl] : mg.GetJoints()) {
                bool isSelectable = jointID != m_Options.MaybeElAttachingTo && m_Options.CanChooseJoints;

                UID id = isSelectable? jointID : g_EmptyID;
                UID groupId = isSelectable ? g_JointGroupID : g_EmptyID;
                float alpha = isSelectable ? 1.0f : 0.2f;
                float rimAlpha = jointID == m_MaybeHover.ID ? 0.8f : 0.0f;
                glm::vec3 axisLengths = GetJointAxisLengths(jointEl);

                m_Shared->AppendAsFrame(id, groupId, jointEl.Center, m_DrawablesBuffer, alpha, rimAlpha, axisLengths, m_Shared->GetColorJointFrameCore());
            }

            // ground
            {
                bool isSelectable = g_GroundID != m_Options.MaybeElAttachingTo && m_Options.CanChooseGround;

                DrawableThing& d = m_DrawablesBuffer.emplace_back(m_Shared->GenerateGroundSphere({0.0f, 0.0f, 0.0f, 1.0f}));
                d.id = isSelectable ? g_GroundID : g_EmptyID;
                d.groupId = isSelectable ? g_GroundGroupID : g_EmptyID;
                d.color.a = isSelectable ? 1.0f : 0.2f;
                d.rimColor = g_GroundID == m_MaybeHover.ID ? 0.8f : 0.0f;
            }

            // floor
            m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());

            return m_DrawablesBuffer;
        }

        void HandleHovertestSideEffects()
        {
            if (!m_MaybeHover) {
                return;
            }

            DrawHoverTooltip();

            // if user clicks on hovered element, then they are trying to select it
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_MaybeNextState = m_Options.OnUserChoice(m_Shared, m_MaybeHover.ID);
            }
        }

        // draws 3D scene into an ImGui::Image and performs any hittesting etc.
        void Draw3DViewer()
        {
            m_Shared->SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing>& drawables = GenerateDrawables();

            m_MaybeHover = m_Shared->Hovertest(drawables);
            HandleHovertestSideEffects();

            m_Shared->DrawScene(drawables);
            DrawConnectionLines();
            DrawHeaderText();
        }

    public:
        std::unique_ptr<MWState> onEvent(SDL_Event const& e) override
        {
            m_Shared->onEvent(e);
            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> tick(float dt) override
        {
            m_Shared->tick(dt);

            if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
                // ESC: user cancelled out
                m_MaybeNextState = m_Options.OnUserCancelled(m_Shared);
            }

            bool isRenderHovered = m_Shared->IsRenderHovered();

            if (isRenderHovered) {
                UpdatePolarCameraFromImGuiUserInput(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> draw() override
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("wizardsstep2viewer")) {
                ImGui::PopStyleVar();
                Draw3DViewer();
            } else {
                ImGui::PopStyleVar();
            }
            ImGui::End();

            return std::move(m_MaybeNextState);
        }


    private:
        // data that's shared between other UI states
        std::shared_ptr<SharedData> m_Shared;

        // options for this state
        ChooseSomethingOptions m_Options;

        // (maybe) next state to transition to
        std::unique_ptr<MWState> m_MaybeNextState = nullptr;

        // (maybe) user mouse hover
        Hover m_MaybeHover;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;
    };

    // "standard" UI state
    //
    // this is what the user is typically interacting with when the UI loads
    class StandardMWState final : public MWState {
    public:

        StandardMWState(std::shared_ptr<SharedData> shared) : m_Shared{std::move(shared)}
        {
        }

        // try to select *only* what is currently hovered
        void SelectJustHover()
        {
            if (!m_Hover) {
                return;
            }

            m_Shared->UpdModelGraph().Select(m_Hover.ID);
        }

        // try to select what is currently hovered *and* anything that is "grouped"
        // with the hovered item
        //
        // "grouped" here specifically means other meshes connected to the same body
        void SelectAnythingGroupedWithHover()
        {
            if (!m_Hover) {
                return;
            }

            ModelGraph& mg = m_Shared->UpdModelGraph();
            ForEachIDInSelectionGroup(mg, m_Hover.ID, [&mg](UID el) {
                mg.Select(el);
            });
        }

        // returns recommended rim intensity for the provided scene element
        float RimIntensity(UID id) const
        {
            if (id == g_EmptyID) {
                return 0.0f;
            } else if (m_Shared->IsSelected(id)) {
                return 0.9f;
            } else if (IsInSelectionGroupOf(m_Shared->GetModelGraph(), m_Hover.ID, id)) {
                return 0.4f;
            } else {
                return 0.0f;
            }
        }

        void AddBodyToHoveredElement()
        {
            if (!m_Hover) {
                return;
            }

            m_Shared->AddBody(m_Hover.Pos);
        }

        void TransitionToAssigningMeshNextFrame(MeshEl meshEl)
        {
            ChooseSomethingOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = false;
            opts.CanChooseMeshes = false;
            opts.MaybeElAttachingTo = meshEl.ID;
            opts.IsAttachingTowardEl = false;
            opts.MaybeElBeingReplacedByChoice = meshEl.Attachment;
            opts.Header = "choose mesh attachment point (ESC to cancel)";
            opts.OnUserChoice = [meshID = meshEl.ID](std::shared_ptr<SharedData> shared, UID choice) {
                if (choice == meshID || choice == g_GroundID) {
                    shared->UpdModelGraph().UnsetMeshAttachmentPoint(meshID);
                    shared->CommitCurrentModelGraph("assigned mesh to ground");
                } else if (shared->GetModelGraph().ContainsBodyEl(choice)) {
                    shared->UpdModelGraph().SetMeshAttachmentPoint(meshID, DowncastID<BodyEl>(choice));
                    shared->CommitCurrentModelGraph("assigned mesh to body");
                }
                return CreateStandardState(shared);
            };

            // request a state transition
            m_MaybeNextState = std::make_unique<ChooseSomethingMWState>(m_Shared, opts);
        }

        void TryTransitionToAssigningHoveredMeshNextFrame()
        {
            if (!m_Hover) {
                return;
            }

            MeshEl const* maybeMesh = m_Shared->UpdModelGraph().TryGetMeshElByID(m_Hover.ID);

            if (!maybeMesh) {
                return;  // not hovering a mesh
            }

            TransitionToAssigningMeshNextFrame(*maybeMesh);
        }

        void OpenHoverContextMenu()
        {
            m_MaybeOpenedContextMenu = m_Hover ? m_Hover : Hover{g_RightClickedNothingID, {}};
            ImGui::OpenPopup(m_ContextMenuName);
            App::cur().requestRedraw();
        }

        void DeleteSelected()
        {
            if (Contains(m_Shared->GetModelGraph().GetSelected(), m_Hover.ID)) {
                m_Hover.reset();
            }

            if (Contains(m_Shared->GetModelGraph().GetSelected(), m_MaybeOpenedContextMenu.ID)) {
                m_MaybeOpenedContextMenu.reset();
            }

            m_Shared->DeleteSelected();
        }

        void DeleteEl(UID elID)
        {
            if (m_Hover.ID == elID) {
                m_Hover.reset();
            }

            if (m_MaybeOpenedContextMenu.ID == elID) {
                m_MaybeOpenedContextMenu.reset();
            }

            m_Shared->UpdModelGraph().DeleteElementByID(elID);
        }

        bool UpdateFromImGuiKeyboardState()
        {
            if (ImGui::GetIO().WantCaptureKeyboard) {
                return false;
            }

            bool shiftDown = osc::IsShiftDown();
            bool ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

            if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_N)) {
                // Ctrl+N: new scene
                m_Shared->ResetModelGraph();
                return true;
            } else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_Q)) {
                // Ctrl+Q: quit application
                App::cur().requestQuit();
                return true;
            } else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                // Ctrl+A: select all
                m_Shared->SelectAll();
                return true;
            } else if (ctrlOrSuperDown && shiftDown && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                // Ctrl+Shift+Z: redo
                m_Shared->RedoCurrentModelGraph();
                return true;
            } else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                // Ctrl+Z: undo
                m_Shared->UndoCurrentModelGraph();
                return true;
            } else if (osc::IsAnyKeyDown({ SDL_SCANCODE_DELETE, SDL_SCANCODE_BACKSPACE})) {
                // Delete/Backspace: delete any selected elements
                DeleteSelected();
                return true;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
                // B: add body to hovered element
                AddBodyToHoveredElement();
                return true;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                // A: assign a parent for the hovered element
                TryTransitionToAssigningHoveredMeshNextFrame();
                return true;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
                // R: set manipulation mode to "rotate"
                if (m_ImGuizmoState.op == ImGuizmo::ROTATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::ROTATE;
                return true;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
                // G: set manipulation mode to "grab" (translate)
                if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
                return true;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
                // S: set manipulation mode to "scale"
                if (m_ImGuizmoState.op == ImGuizmo::SCALE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::SCALE;
                return true;
            } else {
                return false;
            }
        }

        void TransitionToChoosingJointParent(UIDT<BodyEl> childID)
        {
            ChooseSomethingOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = false;
            opts.CanChooseMeshes = false;
            opts.Header = "choose joint parent (ESC to cancel)";
            opts.MaybeElAttachingTo = childID;
            opts.IsAttachingTowardEl = false;  // away from the body
            opts.OnUserChoice = [childID](std::shared_ptr<SharedData> shared, UID parentID) {
                size_t freejointIdx = *JointRegistry::indexOf(OpenSim::FreeJoint{});
                glm::vec3 parentPos = shared->GetModelGraph().GetShiftInGround(parentID);
                glm::vec3 childPos = shared->GetModelGraph().GetShiftInGround(childID);
                glm::vec3 midPoint = (parentPos + childPos) / 2.0f;
                shared->UpdModelGraph().AddJoint(freejointIdx, "", parentID, childID, Ras{{}, midPoint});
                shared->CommitCurrentModelGraph("added joint");
                return CreateStandardState(shared);
            };
            m_MaybeNextState = std::make_unique<ChooseSomethingMWState>(m_Shared, opts);
        }

        void DrawGroundContextMenu()
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                ImGui::Text("ground");
                ImGui::SameLine();
                ImGui::TextDisabled("(scene origin)");
                ImGui::SameLine();
                DrawHelpMarker("Ground", OSC_GROUND_DESC);

                ImGui::Dummy({0.0f, 5.0f});

                if (ImGui::MenuItem(ICON_FA_CAMERA " focus camera on this")) {
                    m_Shared->FocusCameraOn({});
                }
                if (ImGui::MenuItem(ICON_FA_CUBE " add mesh(es)")) {
                    m_Shared->PushMeshLoadRequests(m_Shared->PromptUserForMeshFiles());
                }

                ImGui::EndPopup();
            }
        }

        void DrawRightClickedNothingContextMenu()
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                ImGui::Text("actions");

                ImGui::Separator();

                if (ImGui::MenuItem("import meshes")) {
                    m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
                }
                if (ImGui::MenuItem("add body")) {
                    m_Shared->AddBody({0.0f, 0.0f, 0.0f});
                }
                if (ImGui::MenuItem(ICON_FA_ARROW_RIGHT " convert to OpenSim model")) {
                    m_Shared->TryCreateOutputModel();
                }
                ImGui::EndPopup();
            }
        }

        void DrawBodyContextMenu(BodyEl bodyEl)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                // title
                ImGui::Text(ICON_FA_CIRCLE" %s", bodyEl.Name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("(body)");
                ImGui::SameLine();
                DrawHelpMarker("Body", OSC_BODY_DESC);

                ImGui::Dummy({0.0f, 5.0f});

                // editors
                // name editor
                {
                    char buf[256];
                    std::strcpy(buf, bodyEl.Name.c_str());
                    if (ImGui::InputText("name", buf, sizeof(buf))) {
                        m_Shared->UpdModelGraph().SetBodyName(bodyEl.ID, buf);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed body name");
                    }
                }
                // mass editor
                {
                    float curMass = static_cast<float>(bodyEl.Mass);
                    if (ImGui::InputFloat("mass", &curMass, 0.0f, 0.0f, OSC_FLOAT_INPUT_FORMAT)) {
                        m_Shared->UpdModelGraph().SetBodyMass(bodyEl.ID, static_cast<double>(curMass));
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed body mass");
                    }
                    ImGui::SameLine();
                    DrawHelpMarker("Mass", "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
                }
                // pos editor
                {
                    glm::vec3 translation = bodyEl.Xform.shift;
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), OSC_FLOAT_INPUT_FORMAT)) {
                        Ras to = bodyEl.Xform;
                        to.shift = translation;
                        m_Shared->UpdModelGraph().SetBodyXform(bodyEl.ID, to);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed body translation");
                    }
                    ImGui::SameLine();
                    DrawHelpMarker("Translation", OSC_TRANSLATION_DESC);
                }
                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(bodyEl.Xform.rot);
                    if (ImGui::InputFloat3("orientation (deg)", glm::value_ptr(orientationDegrees), OSC_FLOAT_INPUT_FORMAT)) {
                        Ras to = bodyEl.Xform;
                        to.rot = glm::radians(orientationDegrees);
                        m_Shared->UpdModelGraph().SetBodyXform(bodyEl.ID, to);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("cFMehanged body orientation");
                    }
                }

                ImGui::Dummy({0.0f, 5.0f});

                // actions
                if (ImGui::MenuItem(ICON_FA_CAMERA " focus camera on this")) {
                    m_Shared->FocusCameraOn(bodyEl.Xform.shift);
                }
                if (ImGui::MenuItem(ICON_FA_LINK " join to")) {
                    TransitionToChoosingJointParent(bodyEl.ID);
                }
                DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a freejoint between the body and ground.");
                if (ImGui::MenuItem(ICON_FA_CUBE " attach mesh to this")) {
                    UIDT<BodyEl> bodyID = bodyEl.ID;
                    m_Shared->PushMeshLoadRequests(bodyID, m_Shared->PromptUserForMeshFiles());
                }

                DrawReorientMenu(bodyEl);

                if (ImGui::MenuItem(ICON_FA_TRASH " delete")) {
                    std::string name = bodyEl.Name;
                    DeleteEl(bodyEl.ID);
                    m_Shared->CommitCurrentModelGraph("deleted " + name);
                    ImGui::EndPopup();
                    return;
                }

                ImGui::EndPopup();

                if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
                    m_MaybeOpenedContextMenu.reset();
                }
            }
        }

        void DrawMeshContextMenu(MeshEl meshEl, glm::vec3 clickPos)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                // title
                ImGui::Text(ICON_FA_CUBE " %s", GetLabel(meshEl).c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("(mesh)");
                ImGui::SameLine();
                DrawHelpMarker("Mesh", OSC_MESH_DESC);

                ImGui::Dummy({0.0f, 5.0f});

                // editors
                // draw name editor
                {
                    char buf[256];
                    std::strcpy(buf, meshEl.Name.c_str());
                    ImGui::InputText("name", buf, sizeof(buf));
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->UpdModelGraph().SetMeshName(meshEl.ID, buf);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed mesh name");
                    }
                }
                // pos editor
                {
                    glm::vec3 translation = meshEl.Xform.shift;
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), OSC_FLOAT_INPUT_FORMAT)) {
                        Ras to = meshEl.Xform;
                        to.shift = translation;
                        m_Shared->UpdModelGraph().SetMeshXform(meshEl.ID, to);
                    }

                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed mesh translation");
                    }

                    ImGui::SameLine();
                    DrawHelpMarker("Translation", OSC_TRANSLATION_DESC);
                }
                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(meshEl.Xform.rot);
                    if (ImGui::InputFloat3("orientation", glm::value_ptr(orientationDegrees), OSC_FLOAT_INPUT_FORMAT)) {
                        Ras to = meshEl.Xform;
                        to.rot = glm::radians(orientationDegrees);
                        m_Shared->UpdModelGraph().SetMeshXform(meshEl.ID, to);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed mesh orientation");
                    }
                }
                // scale factor editor
                {
                    glm::vec3 scaleFactors = meshEl.ScaleFactors;
                    if (ImGui::InputFloat3("scale", glm::value_ptr(scaleFactors), OSC_FLOAT_INPUT_FORMAT)) {
                        m_Shared->UpdModelGraph().SetMeshScaleFactors(meshEl.ID, scaleFactors);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed mesh scale factors");
                    }
                }

                // actions
                ImGui::Dummy({0.0f, 5.0f});

                if (ImGui::MenuItem(ICON_FA_CAMERA " focus camera on this")) {
                    m_Shared->FocusCameraOn(meshEl.Xform.shift);
                }

                if (ImGui::BeginMenu(ICON_FA_CIRCLE " add body")) {
                    if (ImGui::MenuItem("at click location")) {
                        std::string bodyName = GenerateBodyName();
                        UIDT<BodyEl> bodyID = m_Shared->UpdModelGraph().AddBody(bodyName, Ras{{}, clickPos});
                        m_Shared->UpdModelGraph().DeSelectAll();
                        m_Shared->UpdModelGraph().Select(bodyID);
                        if (meshEl.Attachment == g_GroundID || meshEl.Attachment == g_EmptyID) {
                            m_Shared->UpdModelGraph().SetMeshAttachmentPoint(meshEl.ID, bodyID);
                        }
                        m_Shared->CommitCurrentModelGraph(std::string{"added "} + bodyName);
                    }

                    if (ImGui::MenuItem("at mesh origin")) {
                        std::string bodyName = GenerateBodyName();
                        UIDT<BodyEl> bodyID = m_Shared->UpdModelGraph().AddBody(bodyName, Ras{{}, meshEl.Xform.shift});
                        m_Shared->UpdModelGraph().DeSelectAll();
                        m_Shared->UpdModelGraph().Select(bodyID);
                        if (meshEl.Attachment == g_GroundID || meshEl.Attachment == g_EmptyID) {
                            m_Shared->UpdModelGraph().SetMeshAttachmentPoint(meshEl.ID, bodyID);
                        }
                        m_Shared->CommitCurrentModelGraph(std::string{"added "} + bodyName);
                    }

                    if (ImGui::MenuItem("at mesh bounds center")) {
                        std::string bodyName = GenerateBodyName();
                        UIDT<BodyEl> bodyID = m_Shared->UpdModelGraph().AddBody(bodyName, Ras{{}, CalcBoundsCenter(meshEl)});
                        m_Shared->UpdModelGraph().DeSelectAll();
                        m_Shared->UpdModelGraph().Select(bodyID);
                        if (meshEl.Attachment == g_GroundID || meshEl.Attachment == g_EmptyID) {
                            m_Shared->UpdModelGraph().SetMeshAttachmentPoint(meshEl.ID, bodyID);
                        }
                        m_Shared->CommitCurrentModelGraph(std::string{"added "} + bodyName);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("assign to body")) {
                    TransitionToAssigningMeshNextFrame(meshEl);
                }

                if (ImGui::MenuItem("unassign from body", NULL, false, !(meshEl.Attachment == g_EmptyID || meshEl.Attachment == g_GroundID))) {
                    m_Shared->UnassignMesh(meshEl);
                }

                if (ImGui::MenuItem(ICON_FA_TRASH " delete")) {
                    std::string name = meshEl.Name;
                    DeleteEl(meshEl.ID);
                    m_Shared->CommitCurrentModelGraph("deleted " + name);
                    m_MaybeOpenedContextMenu.reset();
                    ImGui::EndPopup();
                    return;
                }

                ImGui::EndPopup();

                if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
                    m_MaybeOpenedContextMenu.reset();
                }
            }
        }

        void ActionPointBodyTowards(UIDT<BodyEl> bodyID, int axis)
        {
            ChooseSomethingOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = true;
            opts.CanChooseMeshes = false;
            opts.Header = "choose what to point towards (ESC to cancel)";
            opts.OnUserChoice = [bodyID, axis](std::shared_ptr<SharedData> shared, UID userChoice) {
                glm::vec3 choicePos = shared->GetModelGraph().GetShiftInGround(userChoice);
                glm::vec3 sourcePos = shared->GetModelGraph().GetShiftInGround(bodyID);
                glm::vec3 choice2source = choicePos - sourcePos;
                glm::vec3 choice2sourceDir = glm::normalize(choice2source);
                glm::vec3 axisDir{};
                axisDir[axis] = 1.0f;

                float cosAng = glm::dot(choice2sourceDir, axisDir);
                if (std::fabs(cosAng) < 0.999f) {
                    glm::vec3 axisVec = glm::cross(axisDir, choice2sourceDir);
                    glm::mat4 rot = glm::rotate(glm::mat4{1.0f}, glm::acos(cosAng), axisVec);
                    glm::vec3 euler = MatToEulerAngles(rot);
                    Ras bodyXform = shared->GetModelGraph().GetRasInGround(bodyID);
                    shared->UpdModelGraph().SetBodyXform(bodyID, Ras{euler, bodyXform.shift});
                }

                shared->CommitCurrentModelGraph("reoriented body");
                return CreateStandardState(shared);
            };
            m_MaybeNextState = std::make_unique<ChooseSomethingMWState>(m_Shared, opts);
        }

        void DrawReorientMenu(BodyEl bodyEl)
        {
            if (ImGui::BeginMenu(ICON_FA_REDO " reorient")) {
                if (ImGui::MenuItem("Point X towards")) {
                    ActionPointBodyTowards(bodyEl.ID, 0);
                }
                if (ImGui::MenuItem("Point Y towards")) {
                    ActionPointBodyTowards(bodyEl.ID, 1);
                }
                if (ImGui::MenuItem("Point Z towards")) {
                    ActionPointBodyTowards(bodyEl.ID, 2);
                }
                if (ImGui::MenuItem("Reset")) {
                    Ras newCenter = Ras{{}, bodyEl.Xform.shift};
                    m_Shared->UpdModelGraph().SetBodyXform(bodyEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reset " + bodyEl.Name + " orientation");
                }

                ImGui::EndMenu();
            }
        }

        void DrawReorientMenu(JointEl jointEl)
        {
            ModelGraph const& mg = m_Shared->GetModelGraph();

            if (ImGui::BeginMenu(ICON_FA_REDO " reorient")) {

                if (ImGui::MenuItem("Point X towards parent")) {
                    glm::vec3 parentPos = mg.GetShiftInGround(jointEl.Parent);
                    glm::vec3 childPos = mg.GetShiftInGround(jointEl.Child);
                    glm::vec3 pivotPos = jointEl.Center.shift;
                    glm::vec3 pivot2parent = parentPos - pivotPos;
                    glm::vec3 pivot2child = childPos - pivotPos;
                    glm::vec3 pivot2parentDir = glm::normalize(pivot2parent);
                    glm::vec3 pivot2childDir = glm::normalize(pivot2child);

                    glm::vec3 xAxis = pivot2parentDir;  // user requested this
                    glm::vec3 zAxis = glm::normalize(glm::cross(pivot2parentDir, pivot2childDir));  // from the triangle
                    glm::vec3 yAxis = glm::normalize(glm::cross(zAxis, xAxis));

                    glm::mat3 m{xAxis, yAxis, zAxis};
                    Ras newCenter{MatToEulerAngles(m), jointEl.Center.shift};

                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Point X towards child")) {
                    glm::vec3 parentPos = mg.GetShiftInGround(jointEl.Parent);
                    glm::vec3 childPos = mg.GetShiftInGround(jointEl.Child);
                    glm::vec3 pivotPos = jointEl.Center.shift;
                    glm::vec3 pivot2parent = parentPos - pivotPos;
                    glm::vec3 pivot2child = childPos - pivotPos;
                    glm::vec3 pivot2parentDir = glm::normalize(pivot2parent);
                    glm::vec3 pivot2childDir = glm::normalize(pivot2child);

                    glm::vec3 xAxis = pivot2childDir;  // user requested this
                    glm::vec3 zAxis = glm::normalize(glm::cross(pivot2parentDir, pivot2childDir));  // from the triangle
                    glm::vec3 yAxis = glm::normalize(glm::cross(zAxis, xAxis));

                    glm::mat3 m{xAxis, yAxis, zAxis};
                    Ras newCenter{MatToEulerAngles(m), jointEl.Center.shift};

                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Orient Z along two mesh points")) {
                    SelectTwoMeshPointsOptions opts;
                    opts.OnTwoPointsChosen = [jointEl](std::shared_ptr<SharedData> shared, glm::vec3 a, glm::vec3 b) {
                        glm::vec3 aTobDir = glm::normalize(a-b);
                        glm::mat4 currentXform = CalcXformMatrix(jointEl);
                        glm::vec3 currentZ = currentXform * glm::vec4{0.0f, 0.0f, 1.0f, 0.0f};

                        float cosAng = glm::dot(aTobDir, currentZ);
                        if (std::fabs(cosAng) < 0.999f) {
                            glm::vec3 axis = glm::cross(aTobDir, currentZ);
                            glm::mat4 xform = glm::rotate(glm::mat4{1.0f}, glm::acos(cosAng), axis);
                            glm::mat4 overallXform = xform * currentXform;
                            glm::vec3 newEuler = MatToEulerAngles(overallXform);
                            Ras newRas = Ras{newEuler, jointEl.Center.shift};
                            shared->UpdModelGraph().SetJointCenter(jointEl.ID, newRas);
                            shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                        }
                        return CreateStandardState(shared);
                    };

                    m_MaybeNextState = std::make_unique<SelectTwoMeshPointsState>(m_Shared, opts);
                }

                if (ImGui::MenuItem("Rotate X 180 degrees")) {
                    Ras newCenter = {EulerCompose({fpi, 0.0f, 0.0f}, jointEl.Center.rot), jointEl.Center.shift};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Rotate Y 180 degrees")) {
                    Ras newCenter = {EulerCompose({0.0f, fpi, 0.0f}, jointEl.Center.rot), jointEl.Center.shift};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Rotate Z 180 degrees")) {
                    Ras newCenter = {EulerCompose({0.0f, 0.0f, fpi}, jointEl.Center.rot), jointEl.Center.shift};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Use parent's orientation")) {
                    Ras newCenter = Ras{mg.GetRotationInGround(jointEl.Parent), jointEl.Center.shift};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Use child's orientation")) {
                    Ras newCenter = Ras{mg.GetRotationInGround(jointEl.Child), jointEl.Center.shift};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reoriented " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Reset")) {
                    Ras newCenter = Ras{{}, jointEl.Center.shift};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("reset " + GetLabel(jointEl) + " orientation");
                }

                ImGui::EndMenu();
            }
        }

        void DrawTranslateMenu(JointEl jointEl)
        {
            ModelGraph const& mg = m_Shared->GetModelGraph();

            if (ImGui::BeginMenu(ICON_FA_ARROWS_ALT " translate")) {

                if (ImGui::MenuItem("Translate to midpoint")) {
                    glm::vec3 parentPos = mg.GetShiftInGround(jointEl.Parent);
                    glm::vec3 childPos = mg.GetShiftInGround(jointEl.Child);
                    glm::vec3 centerPos = (parentPos + childPos)/2.0f;
                    Ras newCenter = {jointEl.Center.rot, centerPos};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("moved " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Use parent's translation")) {
                    glm::vec3 parentPos = mg.GetShiftInGround(jointEl.Parent);
                    Ras newCenter = {jointEl.Center.rot, parentPos};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("moved " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Use child's translation")) {
                    glm::vec3 childPos = mg.GetShiftInGround(jointEl.Child);
                    Ras newCenter = {jointEl.Center.rot, childPos};
                    m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, newCenter);
                    m_Shared->CommitCurrentModelGraph("moved " + GetLabel(jointEl));
                }

                if (ImGui::MenuItem("Between two mesh points")) {
                    SelectTwoMeshPointsOptions opts;
                    opts.OnTwoPointsChosen = [jointEl](std::shared_ptr<SharedData> shared, glm::vec3 a, glm::vec3 b) {
                        glm::vec3 midpoint = (a+b)/2.0f;
                        Ras newRas = {shared->GetModelGraph().GetRotationInGround(jointEl.ID), midpoint};
                        shared->UpdModelGraph().SetJointCenter(jointEl.ID, newRas);
                        shared->CommitCurrentModelGraph("translated " + GetLabel(jointEl));
                        return CreateStandardState(shared);
                    };

                    m_MaybeNextState = std::make_unique<SelectTwoMeshPointsState>(m_Shared, opts);
                }

                ImGui::EndMenu();
            }
        }

        void DrawJointContextMenu(JointEl jointEl)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                // title
                ImGui::Text(ICON_FA_LINK " %s", GetLabel(jointEl).c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", GetJointTypeName(jointEl).c_str());
                ImGui::SameLine();
                DrawHelpMarker("Joints", "Joints connect two PhysicalFrames (body/ground) together and specifies their relative permissible motion.");

                ImGui::Dummy({0.0f, 5.0f});

                // editors
                // name editor
                {
                    char buf[256];
                    std::strcpy(buf, jointEl.UserAssignedName.c_str());
                    if (ImGui::InputText("name", buf, sizeof(buf))) {
                        m_Shared->UpdModelGraph().SetJointName(jointEl.ID, buf);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed joint name");
                    }
                }
                // pos editor
                {
                    glm::vec3 translation = jointEl.Center.shift;
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), OSC_FLOAT_INPUT_FORMAT)) {
                        Ras to = jointEl.Center;
                        to.shift = translation;
                        m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, to);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed joint translation");
                    }
                    ImGui::SameLine();
                    DrawHelpMarker("Translation", OSC_TRANSLATION_DESC);
                }
                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(jointEl.Center.rot);
                    if (ImGui::InputFloat3("orientation", glm::value_ptr(orientationDegrees), OSC_FLOAT_INPUT_FORMAT)) {
                        Ras to = jointEl.Center;
                        to.rot = glm::radians(orientationDegrees);
                        m_Shared->UpdModelGraph().SetJointCenter(jointEl.ID, to);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_Shared->CommitCurrentModelGraph("changed joint orientation");
                    }
                }
                // joint type editor
                {
                    int currentIdx = static_cast<int>(jointEl.JointTypeIndex);
                    nonstd::span<char const* const> labels = JointRegistry::nameCStrings();
                    if (ImGui::Combo("joint type", &currentIdx, labels.data(), static_cast<int>(labels.size()))) {
                        m_Shared->UpdModelGraph().SetJointTypeIdx(jointEl.ID, static_cast<size_t>(currentIdx));
                        m_Shared->CommitCurrentModelGraph("changed joint type");
                    }
                }

                ImGui::Dummy({0.0f, 5.0f});

                // actions
                if (ImGui::MenuItem(ICON_FA_CAMERA " focus camera on this")) {
                    m_Shared->FocusCameraOn(jointEl.Center.shift);
                }
                DrawReorientMenu(jointEl);
                DrawTranslateMenu(jointEl);
                if (ImGui::MenuItem(ICON_FA_TRASH " delete")) {
                    std::string name = GetLabel(jointEl);
                    DeleteEl(jointEl.ID);
                    m_Shared->CommitCurrentModelGraph("deleted " + name);
                    ImGui::EndPopup();
                    return;
                }

                ImGui::EndPopup();

                if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
                    m_MaybeOpenedContextMenu.reset();
                }
            }
        }

        void DrawContextMenu()
        {
            if (!m_MaybeOpenedContextMenu) {
                return;  // draw nothing
            } else if (m_MaybeOpenedContextMenu.ID == g_GroundID) {
                DrawGroundContextMenu();
            } else if (m_MaybeOpenedContextMenu.ID == g_RightClickedNothingID) {
                DrawRightClickedNothingContextMenu();
            } else if (MeshEl const* meshEl = m_Shared->GetModelGraph().TryGetMeshElByID(m_MaybeOpenedContextMenu.ID)) {
                DrawMeshContextMenu(*meshEl, m_MaybeOpenedContextMenu.Pos);
            } else if (BodyEl const* bodyEl = m_Shared->GetModelGraph().TryGetBodyElByID(m_MaybeOpenedContextMenu.ID)) {
                DrawBodyContextMenu(*bodyEl);
            } else if (JointEl const* jointEl = m_Shared->GetModelGraph().TryGetJointElByID(m_MaybeOpenedContextMenu.ID)) {
                DrawJointContextMenu(*jointEl);
            }
        }

        void DrawHistory()
        {
            auto const& snapshots = m_Shared->GetModelGraphSnapshots();
            size_t currentSnapshot = m_Shared->GetModelGraphIsBasedOn();
            for (size_t i = 0; i < snapshots.size(); ++i) {
                ModelGraphSnapshot const& snapshot = snapshots[i];

                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(snapshot.GetCommitMessage().c_str(), i == currentSnapshot)) {
                    m_Shared->UseModelGraphSnapshot(i);
                }
                ImGui::PopID();
            }
        }

        void DrawHierarchy()
        {
            ImGui::Text("Bodies");
            ImGui::Indent();
            for (auto const& [bodyID, bodyEl] : m_Shared->GetModelGraph().GetBodies()) {
                int styles = 0;

                if (m_Shared->IsSelected(bodyID)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
                    ++styles;
                }

                ImGui::Text("%s", bodyEl.Name.c_str());

                ImGui::PopStyleColor(styles);

                if (ImGui::IsItemHovered()) {
                    m_Hover = {bodyID, {}};
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (!IsShiftDown()) {
                        m_Shared->UpdModelGraph().DeSelectAll();
                    }
                    m_Shared->UpdModelGraph().Select(bodyID);
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    m_MaybeOpenedContextMenu = Hover{bodyID, {}};
                    ImGui::OpenPopup(m_ContextMenuName);
                    App::cur().requestRedraw();
                }
            }
            ImGui::Unindent();

            ImGui::Text("Joints");
            ImGui::Indent();
            for (auto const& [jointID, jointEl] : m_Shared->GetModelGraph().GetJoints()) {
                int styles = 0;

                if (m_Shared->IsSelected(jointID)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
                    ++styles;
                }

                ImGui::Text("%s", GetLabel(jointEl).c_str());

                ImGui::PopStyleColor(styles);

                if (ImGui::IsItemHovered()) {
                    m_Hover = {jointID, {}};
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (!IsShiftDown()) {
                        m_Shared->UpdModelGraph().DeSelectAll();
                    }
                    m_Shared->UpdModelGraph().Select(jointID);
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    m_MaybeOpenedContextMenu = Hover{jointID, {}};
                    ImGui::OpenPopup(m_ContextMenuName);
                    App::cur().requestRedraw();
                }
            }
            ImGui::Unindent();

            ImGui::Text("Meshes");
            ImGui::Indent();
            for (auto const& [meshID, meshEl] : m_Shared->GetModelGraph().GetMeshes()) {
                int styles = 0;

                if (m_Shared->IsSelected(meshID)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
                    ++styles;
                }

                ImGui::Text("%s", meshEl.Name.c_str());

                ImGui::PopStyleColor(styles);

                if (ImGui::IsItemHovered()) {
                    m_Hover = {meshID, {}};
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (!IsShiftDown()) {
                        m_Shared->UpdModelGraph().DeSelectAll();
                    }
                    m_Shared->UpdModelGraph().Select(meshID);
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    m_MaybeOpenedContextMenu = Hover{meshID, {}};
                    ImGui::OpenPopup(m_ContextMenuName);
                    App::cur().requestRedraw();
                }
            }
            ImGui::Unindent();
        }

        void Draw3DViewerOverlay()
        {
            int imguiID = 0;

            if (ImGui::Button(ICON_FA_CUBE " Add Meshes")) {
                m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
            }
            DrawTooltipIfItemHovered("Add Mesh(es) to the model", "Meshes are purely decorative scene elements that can be attached to bodies, or the ground. When attached to a body, the mesh's transformation will be linked to the body's.");

            ImGui::SameLine();

            ImGui::Button(ICON_FA_PLUS " Add Other");
            DrawTooltipIfItemHovered("Add components to the model");

            if (ImGui::BeginPopupContextItem("##additemtoscenepopup", ImGuiPopupFlags_MouseButtonLeft)) {
                if (ImGui::MenuItem(ICON_FA_CUBE " Mesh(es)")) {
                    m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
                }
                DrawTooltipIfItemHovered("Add Mesh(es) to the model", "Meshes are purely decorative scene elements that can be attached to bodies, or the ground. When attached to a body, the mesh's transformation will be linked to the body's.");

                if (ImGui::MenuItem(ICON_FA_CIRCLE " Body")) {
                    m_Shared->AddBody({0.0f, 0.0f, 0.0f});
                }
                DrawTooltipIfItemHovered("Add Body at Ground Location", OSC_BODY_DESC);
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::Button(ICON_FA_PAINT_ROLLER " Colors");
            DrawTooltipIfItemHovered("Change scene display colors", "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

            if (ImGui::BeginPopupContextItem("##addpainttoscenepopup", ImGuiPopupFlags_MouseButtonLeft)) {
                nonstd::span<glm::vec4 const> colors = m_Shared->GetColors();
                nonstd::span<char const* const> labels = m_Shared->GetColorLabels();
                OSC_ASSERT(colors.size() == labels.size() && "every color should have a label");

                for (size_t i = 0; i < colors.size(); ++i) {
                    glm::vec4 colorVal = colors[i];
                    ImGui::PushID(imguiID++);
                    if (ImGui::ColorEdit4(labels[i], glm::value_ptr(colorVal))) {
                        m_Shared->SetColor(i, colorVal);
                    }
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::Button(ICON_FA_EYE " Visibility");
            DrawTooltipIfItemHovered("Change what's visible in the 3D scene", "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

            if (ImGui::BeginPopupContextItem("##changevisibilitypopup", ImGuiPopupFlags_MouseButtonLeft)) {
                nonstd::span<bool const> visibilities = m_Shared->GetVisibilityFlags();
                nonstd::span<char const* const> labels = m_Shared->GetVisibilityFlagLabels();
                OSC_ASSERT(visibilities.size() == labels.size() && "every visibility flag should have a label");

                for (size_t i = 0; i < visibilities.size(); ++i) {
                    bool v = visibilities[i];
                    ImGui::PushID(imguiID++);
                    if (ImGui::Checkbox(labels[i], &v)) {
                        m_Shared->SetVisibilityFlag(i, v);
                    }
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::Button(ICON_FA_LOCK " Interactivity");
            DrawTooltipIfItemHovered("Change what your mouse can interact with in the 3D scene", "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

            if (ImGui::BeginPopupContextItem("##changeinteractionlockspopup", ImGuiPopupFlags_MouseButtonLeft)) {
                nonstd::span<bool const> interactables = m_Shared->GetIneractivityFlags();
                nonstd::span<char const* const> labels =  m_Shared->GetInteractivityFlagLabels();
                OSC_ASSERT(interactables.size() == labels.size());

                for (size_t i = 0; i < interactables.size(); ++i) {
                    bool v = interactables[i];
                    ImGui::PushID(imguiID++);
                    if (ImGui::Checkbox(labels[i], &v)) {
                        m_Shared->SetInteractivityFlag(i, v);
                    }
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            // translate/rotate/scale dropdown
            {
                char const* modes[] = {"translate", "rotate", "scale"};
                ImGuizmo::OPERATION ops[] = {ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE};
                int currentOp = static_cast<int>(std::distance(std::begin(ops), std::find(std::begin(ops), std::end(ops), m_ImGuizmoState.op)));

                ImGui::SetNextItemWidth(ImGui::CalcTextSize(modes[0]).x + 40.0f);
                if (ImGui::Combo("##opselect", &currentOp, modes, IM_ARRAYSIZE(modes))) {
                    m_ImGuizmoState.op = ops[static_cast<size_t>(currentOp)];
                }
                char const* const tooltipTitle = "Manipulation Mode";
                char const* const tooltipDesc = "This affects which manipulation gizmos are shown over the selected object.\n\nYou can also use keybinds to flip between these:\n    G    translate\n    R    rotate\n    S    scale";
                DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
            }

            ImGui::SameLine();

            // local/global dropdown
            {
                char const* modeLabels[] = {"local", "global"};
                ImGuizmo::MODE modes[] = {ImGuizmo::LOCAL, ImGuizmo::WORLD};
                int currentMode = static_cast<int>(std::distance(std::begin(modes), std::find(std::begin(modes), std::end(modes), m_ImGuizmoState.mode)));

                ImGui::SetNextItemWidth(ImGui::CalcTextSize(modeLabels[0]).x + 40.0f);
                if (ImGui::Combo("##modeselect", &currentMode, modeLabels, IM_ARRAYSIZE(modeLabels))) {
                    m_ImGuizmoState.mode = modes[static_cast<size_t>(currentMode)];
                }
                char const* const tooltipTitle = "Manipulation coordinate system";
                char const* const tooltipDesc = "This affects whether manipulations (such as the arrow gizmos that you can use to translate things) are performed relative to the global coordinate system or the selection's (local) one. Local manipulations can be handy when translating/rotating something that's already rotated.";
                DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
            }

            ImGui::SameLine();

            // scale factor
            {
                char const* const tooltipTitle = "Change scene scale factor";
                char const* const tooltipDesc = "This rescales *some* elements in the scene. Specifically, the ones that have no 'size', such as body frames, joint frames, and the chequered floor texture.\n\nChanging this is handy if you are working on smaller or larger models, where the size of the (decorative) frames and floor are too large/small compared to the model you are working on.\n\nThis is purely decorative and does not affect the exported OpenSim model in any way.";

                float sf = m_Shared->GetSceneScaleFactor();
                ImGui::SetNextItemWidth(ImGui::CalcTextSize("1000.00").x);
                if (ImGui::InputFloat("##", &sf)) {
                    m_Shared->SetSceneScaleFactor(sf);
                }
                DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f}));
                ImGui::Text("Scene Scale Factor");
                ImGui::PopStyleColor();
                DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
            }

            // bottom-left axes overlay
            DrawAlignmentAxesOverlayInBottomRightOf(m_Shared->GetCamera().getViewMtx(), m_Shared->Get3DSceneRect());

            // zoom in/out buttons?
            {
                Rect sceneRect = m_Shared->Get3DSceneRect();
                glm::vec2 trPos = {sceneRect.p1.x + 100.0f, sceneRect.p2.y - 55.0f};
                ImGui::SetCursorScreenPos(trPos);

                if (ImGui::Button(ICON_FA_SEARCH_MINUS)) {
                    m_Shared->UpdCamera().radius *= 1.2f;
                }
                DrawTooltipIfItemHovered("Zoom Out");

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_SEARCH_PLUS)) {
                    m_Shared->UpdCamera().radius *= 0.8f;
                }
                DrawTooltipIfItemHovered("Zoom In");

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT)) {
                    auto it = m_DrawablesBuffer.begin();
                    bool containsAtLeastOne = false;
                    AABB aabb;
                    while (it != m_DrawablesBuffer.end()) {
                        if (it->id != g_EmptyID) {
                            aabb = CalcBounds(*it);
                            it++;
                            containsAtLeastOne = true;
                            break;
                        }
                        it++;
                    }
                    if (containsAtLeastOne) {
                        while (it != m_DrawablesBuffer.end()) {
                            if (it->id != g_EmptyID) {
                                aabb = AABBUnion(aabb, CalcBounds(*it));
                            }
                            ++it;
                        }
                        m_Shared->UpdCamera().focusPoint = -AABBCenter(aabb);
                        m_Shared->UpdCamera().radius = 2.0f * AABBLongestDim(aabb);
                    }
                }
                DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_CAMERA)) {
                    m_Shared->UpdCamera().reset();
                    m_Shared->UpdCamera().phi = fpi4;
                    m_Shared->UpdCamera().theta = fpi4;
                    m_Shared->UpdCamera().radius = 5.0f;
                }
                DrawTooltipIfItemHovered("Reset camera");
            }

            // bottom-right "finish" button
            {
                char const* const text = "Convert to OpenSim Model " ICON_FA_ARROW_RIGHT;

                glm::vec2 framePad = {10.0f, 10.0f};
                glm::vec2 margin = {25.0f, 25.0f};
                Rect sceneRect = m_Shared->Get3DSceneRect();
                glm::vec2 textDims = ImGui::CalcTextSize(text);

                ImGui::SetCursorScreenPos(sceneRect.p2 - textDims - framePad - margin);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePad);
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                if (ImGui::Button(text)) {
                    m_Shared->TryCreateOutputModel();
                }
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
                DrawTooltipIfItemHovered("Convert current scene to an OpenSim Model", "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nThe converter will take what you have laid out on this screen and (internally) convert it into an equivalent OpenSim::Model. The conversion process is one-way: you can't edit the OpenSim model and go back to this screen. However, your progress on this screen is saved. You can return to the mesh importer screen, which will 'remember' its last state, if you want to make any additional changes/edits.");
            }
        }

        char const* BodyOrGroundString(UID id) const
        {
            return id == g_GroundID ? "ground" : m_Shared->GetModelGraph().GetBodyByIDOrThrow(id).Name.c_str();
        }

        void DrawMeshHoverTooltip(MeshEl const& meshEl) const
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", GetLabel(meshEl).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(%s, attached to %s)", meshEl.Path.filename().string().c_str(), BodyOrGroundString(meshEl.Attachment));
            ImGui::EndTooltip();
        }

        void DrawBodyHoverTooltip(BodyEl const& bodyEl) const
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", bodyEl.Name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(body)");
            ImGui::EndTooltip();
        }

        void DrawJointHoverTooltip(JointEl const& jointEl) const
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", GetLabel(jointEl).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(%s, %s --> %s)", GetJointTypeName(jointEl).c_str(), BodyOrGroundString(jointEl.Child), BodyOrGroundString(jointEl.Parent));
            ImGui::EndTooltip();
        }

        void DrawGroundHoverTooltip()
        {
            ImGui::BeginTooltip();
            ImGui::Text("ground");
            ImGui::SameLine();
            ImGui::TextDisabled("(scene origin)");
            ImGui::EndTooltip();
        }

        void DrawHoverTooltip()
        {
            if (!m_Hover) {
                return;
            }

            ModelGraph const& mg = m_Shared->GetModelGraph();

            if (m_Hover.ID == g_GroundID) {
                DrawGroundHoverTooltip();
            } else if (MeshEl const* meshEl = mg.TryGetMeshElByID(m_Hover.ID)) {
                DrawMeshHoverTooltip(*meshEl);
            } else if (BodyEl const* bodyEl = mg.TryGetBodyElByID(m_Hover.ID)) {
                DrawBodyHoverTooltip(*bodyEl);
            } else if (JointEl const* jointEl = mg.TryGetJointElByID(m_Hover.ID)) {
                DrawJointHoverTooltip(*jointEl);
            }
        }

        // draws 3D manipulator overlays (drag handles, etc.)
        void DrawSelection3DManipulators()
        {
            if (!m_Shared->HasSelection()) {
                return;  // can only manipulate selected stuff
            }

            // if the user isn't manipulating anything, create an up-to-date
            // manipulation matrix
            //
            // this is so that ImGuizmo can *show* the manipulation axes, and
            // because the user might start manipulating during this frame
            if (!ImGuizmo::IsUsing()) {

                auto it = m_Shared->GetCurrentSelection().begin();
                auto end = m_Shared->GetCurrentSelection().end();

                if (it == end) {
                    return;
                }

                ModelGraph const& mg = m_Shared->GetModelGraph();

                int n = 0;

                glm::vec3 translation = mg.GetShiftInGround(*it);
                glm::vec3 orientation = mg.GetRotationInGround(*it);
                ++it;
                ++n;

                while (it != end) {
                    translation += mg.GetShiftInGround(*it);  // TODO: numerically unstable
                    orientation += mg.GetRotationInGround(*it);  // TODO: numerically unstable
                    ++it;
                    ++n;
                }

                orientation /= static_cast<float>(n);
                translation /= static_cast<float>(n);

                glm::mat4 rotationMtx = EulerAnglesToMat(orientation);
                glm::mat4 translationMtx = glm::translate(glm::mat4{1.0f}, translation);

                m_ImGuizmoState.mtx = translationMtx * rotationMtx;
            }

            // else: is using OR nselected > 0 (so draw it)

            Rect sceneRect = m_Shared->Get3DSceneRect();

            ImGuizmo::SetRect(
                sceneRect.p1.x,
                sceneRect.p1.y,
                RectDims(sceneRect).x,
                RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::AllowAxisFlip(false);

            glm::mat4 delta;
            bool manipulated = ImGuizmo::Manipulate(
                glm::value_ptr(m_Shared->GetCamera().getViewMtx()),
                glm::value_ptr(m_Shared->GetCamera().getProjMtx(RectAspectRatio(sceneRect))),
                m_ImGuizmoState.op,
                m_ImGuizmoState.mode,
                glm::value_ptr(m_ImGuizmoState.mtx),
                glm::value_ptr(delta),
                nullptr,
                nullptr,
                nullptr);

            bool isUsingThisFrame = ImGuizmo::IsUsing();
            bool wasUsingLastFrame = m_ImGuizmoState.wasUsingLastFrame;
            m_ImGuizmoState.wasUsingLastFrame = isUsingThisFrame;  // for the next frame

            // if the user was manipulating something last frame, and isn't manipulaitng
            // this frame, then they just finished a manipulation and it should be
            // snapshotted for undo/redo support
            if (wasUsingLastFrame && !isUsingThisFrame) {
                m_Shared->CommitCurrentModelGraph("manipulated selection");
                App::cur().requestRedraw();
            }

            // if no manipulation happened this frame, exit early
            if (!manipulated) {
                return;
            }

            glm::vec3 translation;
            glm::vec3 rotation;
            glm::vec3 scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(delta), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
            rotation = glm::radians(rotation);

            for (UID id : m_Shared->GetCurrentSelection()) {
                switch (m_ImGuizmoState.op) {
                case ImGuizmo::ROTATE:
                    m_Shared->UpdModelGraph().ApplyRotation(id, rotation);
                    break;
                case ImGuizmo::TRANSLATE:
                    m_Shared->UpdModelGraph().ApplyTranslation(id, translation);
                    break;
                case ImGuizmo::SCALE:
                    m_Shared->UpdModelGraph().ApplyScale(id, scale);
                    break;
                default:
                    break;
                }
            }
        }

        Hover HovertestScene(std::vector<DrawableThing> const& drawables)
        {
            if (!m_Shared->IsRenderHovered()) {
                return m_Hover;
            }

            if (ImGuizmo::IsUsing()) {
                return Hover{};
            }

            return m_Shared->Hovertest(drawables);
        }

        void HandleCurrentHover()
        {
            if (!m_Shared->IsRenderHovered()) {
                return;
            }

            bool lcClicked = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
            bool rcClicked = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
            bool shiftDown = osc::IsShiftDown();
            bool altDown = osc::IsAltDown();
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (!m_Hover && lcClicked && !isUsingGizmo && !shiftDown) {
                // user clicked in some empty part of the screen: clear selection
                m_Shared->DeSelectAll();
            } else if (m_Hover && lcClicked && !isUsingGizmo) {
                // user clicked hovered thing: select hovered thing
                if (!shiftDown) {
                    // user wasn't holding SHIFT, so clear selection
                    m_Shared->DeSelectAll();
                }

                if (altDown) {
                    // ALT: only select the thing the mouse is over
                    SelectJustHover();
                } else {
                    // NO ALT: select the "grouped items"
                    SelectAnythingGroupedWithHover();
                }
            } else if (rcClicked && !isUsingGizmo) {  // user can also right click nothing
                OpenHoverContextMenu();
            } else if (m_Hover && !ImGui::IsPopupOpen(m_ContextMenuName)) {
                DrawHoverTooltip();
            }
        }

        std::vector<DrawableThing>& GenerateDrawables()
        {
            m_DrawablesBuffer.clear();

            if (m_Shared->IsShowingMeshes()) {
                for (auto const& [meshID, meshEl] : m_Shared->GetModelGraph().GetMeshes()) {
                    m_DrawablesBuffer.push_back(m_Shared->GenerateMeshElDrawable(meshEl));
                }
            }

            if (m_Shared->IsShowingBodies()) {
                for (auto const& [bodyID, bodyEl] : m_Shared->GetModelGraph().GetBodies()) {
                    m_Shared->AppendBodyElAsCubeThing(bodyEl, m_DrawablesBuffer);
                }
            }

            if (m_Shared->IsShowingGround()) {
                m_DrawablesBuffer.push_back(m_Shared->GenerateGroundSphere(m_Shared->GetColorGround()));
            }

            if (m_Shared->IsShowingJointCenters()) {
                for (auto const& [jointID, jointEl] : m_Shared->GetModelGraph().GetJoints()) {
                    m_Shared->AppendAsFrame(jointID, g_JointGroupID, jointEl.Center, m_DrawablesBuffer, 1.0f, 0.0f, GetJointAxisLengths(jointEl), m_Shared->GetColorJointFrameCore());
                }
            }

            if (m_Shared->IsShowingFloor()) {
                m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());
            }

            return m_DrawablesBuffer;
        }

        void Draw3DViewer()
        {
            m_Shared->SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing>& sceneEls = GenerateDrawables();

            // hovertest the generated geometry
            m_Hover = HovertestScene(sceneEls);
            HandleCurrentHover();

            // assign rim highlights based on hover
            for (DrawableThing& dt : sceneEls) {
                dt.rimColor = RimIntensity(dt.id);
            }

            // draw 3D scene (effectively, as an ImGui::Image)
            m_Shared->DrawScene(sceneEls);

            // draw overlays/gizmos
            DrawSelection3DManipulators();
            m_Shared->DrawConnectionLines();
        }

        std::unique_ptr<MWState> onEvent(SDL_Event const& e) override
        {
            if (m_Shared->onEvent(e)) {
                return std::move(m_MaybeNextState);
            }

            if (UpdateFromImGuiKeyboardState()) {
                return std::move(m_MaybeNextState);
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> tick(float dt) override
        {
            m_Shared->tick(dt);

            if (m_Shared->IsRenderHovered() && !ImGuizmo::IsUsing()) {
                UpdatePolarCameraFromImGuiUserInput(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> draw() override
        {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem(ICON_FA_FILE " New Scene", "Ctrl+N")) {
                        m_Shared->ResetModelGraph();
                    }

                    if (ImGui::MenuItem(ICON_FA_CUBE " Add Meshes")) {
                        m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
                    }
                    if (ImGui::MenuItem(ICON_FA_ARROW_LEFT " Back to experiments screen")) {
                        App::cur().requestTransition<ExperimentsScreen>();
                    }
                    if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q")) {
                        App::cur().requestQuit();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Shared->CanUndoCurrentModelGraph())) {
                        m_Shared->UndoCurrentModelGraph();
                    }
                    if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Shared->CanRedoCurrentModelGraph())) {
                        m_Shared->RedoCurrentModelGraph();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Window")) {
                    for (int i = 0; i < SharedData::PanelIndex_COUNT; ++i) {
                        if (ImGui::MenuItem(SharedData::g_OpenedPanelNames[i], nullptr, m_Shared->m_PanelStates[i])) {
                            m_Shared->m_PanelStates[i] = !m_Shared->m_PanelStates[i];
                        }
                    }
                    ImGui::EndMenu();
                }

                MainMenuAboutTab{}.draw();

                ImGui::EndMainMenuBar();
            }

            ImGuizmo::BeginFrame();

            if (m_Shared->m_PanelStates[SharedData::PanelIndex_History]) {
                if (ImGui::Begin("history", &m_Shared->m_PanelStates[SharedData::PanelIndex_History])) {
                    DrawHistory();
                }
                ImGui::End();
            }

            if (m_Shared->m_PanelStates[SharedData::PanelIndex_Hierarchy]) {
                if (ImGui::Begin("hierarchy", &m_Shared->m_PanelStates[SharedData::PanelIndex_Hierarchy])) {
                    DrawHierarchy();
                    DrawContextMenu();
                }
                ImGui::End();
            }

            if (m_Shared->m_PanelStates[SharedData::PanelIndex_Log]) {
                if (ImGui::Begin("log", &m_Shared->m_PanelStates[SharedData::PanelIndex_Log])) {
                    m_Shared->m_Logviewer.draw();
                }
                ImGui::End();
            }

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("wizardsstep2viewer")) {
                ImGui::PopStyleVar();
                Draw3DViewer();

                ImGui::SetCursorPos({10.0f, 10.0f});
                Draw3DViewerOverlay();

                DrawContextMenu();
            } else {
                ImGui::PopStyleVar();
            }
            ImGui::End();

            return std::move(m_MaybeNextState);
        }

    private:
        // data shared between states
        std::shared_ptr<SharedData> m_Shared;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;

        // (maybe) hover + worldspace location of the hover
        Hover m_Hover;

        // (maybe) the scene element that the user opened a context menu for
        Hover m_MaybeOpenedContextMenu;

        // (maybe) the next state the host screen should transition to
        std::unique_ptr<MWState> m_MaybeNextState;

        // ImGuizmo state
        struct {
            bool wasUsingLastFrame = false;
            glm::mat4 mtx{};
            ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
            ImGuizmo::MODE mode = ImGuizmo::WORLD;
        } m_ImGuizmoState;

        static constexpr char const* const m_ContextMenuName = "standardmodecontextmenu";
    };

    std::unique_ptr<MWState> CreateStandardState(std::shared_ptr<SharedData> sd)
    {
        return std::make_unique<StandardMWState>(sd);
    }
}

// top-level screen implementation
//
// this effectively just feeds the underlying state machine pattern established by
// the `ModelWizardState` class
struct osc::MeshesToModelWizardScreen::Impl final {
public:
    Impl() :
        m_CurrentState{std::make_unique<StandardMWState>(std::make_shared<SharedData>())}
    {
    }

    Impl(std::vector<std::filesystem::path> meshPaths) :
        m_CurrentState{std::make_unique<StandardMWState>(std::make_shared<SharedData>(meshPaths))}
    {
    }

    void onMount()
    {
        osc::ImGuiInit();
        App::cur().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
        App::cur().makeMainEventLoopPolling();
    }

    // handle any potential new states returned by an MWState method
    void HandleStateReturn(std::unique_ptr<MWState> maybeNewState)
    {
        if (maybeNewState) {
            m_CurrentState = std::move(maybeNewState);
            m_ShouldRequestRedraw = true;
        }
    }

    void onEvent(SDL_Event const& e)
    {
        if (osc::ImGuiOnEvent(e)) {
            m_ShouldRequestRedraw = true;
        }

        HandleStateReturn(m_CurrentState->onEvent(e));
    }

    void tick(float dt)
    {
        HandleStateReturn(m_CurrentState->tick(dt));
    }

    void draw()
    {
        // clear the whole screen (it's a full redraw)
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set up ImGui's internal datastructures
        osc::ImGuiNewFrame();

        // enable panel docking
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

        // draw current state
        HandleStateReturn(m_CurrentState->draw());

        // draw ImGui
        osc::ImGuiRender();

        // request another draw (e.g. because the state changed during this draw)
        if (m_ShouldRequestRedraw) {
            App::cur().requestRedraw();
            m_ShouldRequestRedraw = false;
        }
    }

private:
    std::unique_ptr<MWState> m_CurrentState;
    bool m_ShouldRequestRedraw = false;
};


// public API

// HACK: save this screen's state globally, so that users can "go back" to the screen if the
//       model import fails
//
//       ideally, the screen would launch into a separate tab for the export, but the main UI
//       doesn't support a tab interface at the moment, so this is the best we've got
//
//       DRAGONS: globally allocating a screen like this is bad form because `atexit` is going
//                to be called *after* the app has shutdown the window, OpenGL context, etc.
//                so I'm using a non-unique_ptr, because I don't want the screen's destructor
//                to crash the `atexit` phase, which cleans up all static globals.
osc::MeshesToModelWizardScreen::Impl* GetModelWizardScreenGLOBAL(std::vector<std::filesystem::path> paths)
{
    static osc::MeshesToModelWizardScreen::Impl* g_ModelImpoterScreenState = new osc::MeshesToModelWizardScreen::Impl{paths};
    return g_ModelImpoterScreenState;
}

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen() :
    m_Impl{GetModelWizardScreenGLOBAL({})}
{
}

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen(std::vector<std::filesystem::path> paths) :
    m_Impl{GetModelWizardScreenGLOBAL(paths)}
{
}

osc::MeshesToModelWizardScreen::~MeshesToModelWizardScreen() noexcept
{
    // HACK: don't delete Impl, because we're sharing it globally
}

void osc::MeshesToModelWizardScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MeshesToModelWizardScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MeshesToModelWizardScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MeshesToModelWizardScreen::draw()
{
    m_Impl->draw();
}

void osc::MeshesToModelWizardScreen::tick(float dt)
{
    m_Impl->tick(dt);
}
