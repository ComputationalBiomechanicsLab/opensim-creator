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
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
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
//   existing one. You can't (safely) stuff an arbitrary integer into an ID type. This
//   prevents IDs being "mixed around" from various (unsafe) sources
namespace {

    class ID {
        friend ID GenerateID() noexcept;
        friend constexpr int64_t UnwrapID(ID const&) noexcept;

    protected:
        explicit constexpr ID(int64_t value) noexcept : m_Value{value} {}

    private:
        int64_t m_Value;
    };

    // strongly-typed version of the above
    //
    // adds compile-time type checking to IDs
    template<typename T>
    class IDT : public ID {
        template<typename U>
        friend IDT<U> GenerateIDT() noexcept;

        template<typename U>
        friend constexpr IDT<U> DowncastID(ID const&) noexcept;

    private:
        explicit constexpr IDT(ID id) : ID{id} {}
    };

    ID GenerateID() noexcept
    {
        static std::atomic<int64_t> g_NextID = 1;
        return ID{g_NextID++};
    }

    template<typename T>
    IDT<T> GenerateIDT() noexcept
    {
        return IDT<T>{GenerateID()};
    }

    constexpr int64_t UnwrapID(ID const& id) noexcept
    {
        return id.m_Value;
    }

    std::ostream& operator<<(std::ostream& o, ID const& id) { return o << UnwrapID(id); }
    constexpr bool operator==(ID const& lhs, ID const& rhs) noexcept { return UnwrapID(lhs) == UnwrapID(rhs); }
    constexpr bool operator!=(ID const& lhs, ID const& rhs) noexcept { return UnwrapID(lhs) != UnwrapID(rhs); }

    template<typename T>
    constexpr IDT<T> DowncastID(ID const& id) noexcept {
        return IDT<T>{id};
    }

    // senteniel values used in this codebase
    class BodyEl;
    IDT<BodyEl> const g_GroundID = GenerateIDT<BodyEl>();
    ID const g_EmptyID = GenerateID();
    ID const g_GroundGroupID = GenerateID();
    ID const g_MeshGroupID = GenerateID();
    ID const g_BodyGroupID = GenerateID();
    ID const g_JointGroupID = GenerateID();
    ID const g_PivotID = GenerateID();
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
namespace std {

    template<>
    struct hash<ID> {
        size_t operator()(ID const& id) const { return static_cast<size_t>(UnwrapID(id)); }
    };

    template<typename T>
    struct hash<IDT<T>> {
        size_t operator()(ID const& id) const { return static_cast<size_t>(UnwrapID(id)); }
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
        IDT<BodyEl> PreferredAttachmentPoint;
        std::filesystem::path Path;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        IDT<BodyEl> PreferredAttachmentPoint;
        std::filesystem::path Path;
        std::shared_ptr<Mesh> mesh;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        IDT<BodyEl> PreferredAttachmentPoint;
        std::filesystem::path Path;
        std::string Error;
    };

    // an OK/ERROR response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // function that's used by the meshloader to respond to a mesh loading request
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest msg) noexcept
    {
        try {
            auto mesh = std::make_shared<Mesh>(SimTKLoadMesh(msg.Path));
            auto resp = MeshLoadOKResponse{msg.PreferredAttachmentPoint, msg.Path, std::move(mesh)};
            App::cur().requestRedraw();  // TODO: HACK: try to make the UI thread redraw around the time this is sent
            return resp;
        } catch (std::exception const& ex) {
            return MeshLoadErrorResponse{msg.PreferredAttachmentPoint, msg.Path, ex.what()};
        }
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
        Ras(glm::vec3 rot_, glm::vec3 shift_) : rot{rot_}, shift{shift_} {}
    };

    // returns the average of two rotations-and-shifts
    Ras AverageRas(Ras const& a, Ras const& b)
    {
        glm::vec3 avgShift = (a.shift + b.shift) / 2.0f;
        glm::vec3 avgRot = (a.rot + b.rot) / 2.0f;

        return Ras{avgRot, avgShift};
    }

    // print to an output stream
    std::ostream& operator<<(std::ostream& o, Ras const& to)
    {
        using osc::operator<<;
        return o << "Ras(rot = " << to.rot << ", shift = " << to.shift << ')';
    }

    // returns a transform matrix that maps quantities expressed in the Ras
    // (i.e. "a poor man's frame") into its base
    glm::mat4 XFormFromRas(Ras const& frame)
    {
        glm::mat4 mtx = EulerAnglesToMat(frame.rot);
        mtx[3] = glm::vec4{frame.shift, 1.0f};
        return mtx;
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
        MeshEl(IDT<MeshEl> id,
               IDT<BodyEl> attachment,  // can be ground
               std::shared_ptr<Mesh> meshData,
               std::filesystem::path const& path) :

            ID{id},
            Attachment{attachment},
            MeshData{meshData},
            Path{path}
        {
        }

        IDT<MeshEl> ID;
        IDT<BodyEl> Attachment;  // can be ground
        Ras Xform = {};
        glm::vec3 ScaleFactors = {1.0f, 1.0f, 1.0f};
        std::shared_ptr<Mesh> MeshData;
        std::filesystem::path Path;
        std::string Name = FileNameWithoutExtension(Path);
    };

    // returns a transform matrix that maps quantities expressed in mesh (model) space to groundspace
    glm::mat4 GetModelMatrix(MeshEl const& mesh)
    {
        glm::mat4 translateAndRotate = XFormFromRas(mesh.Xform);
        glm::mat4 rescale = glm::scale(glm::mat4{1.0f}, mesh.ScaleFactors);
        return translateAndRotate * rescale;
    }

    // returns the groundspace bounds of the mesh
    AABB GetGroundspaceBounds(Mesh const& mesh, glm::mat4 const& modelMtx)
    {
        AABB modelspaceBounds = mesh.getAABB();
        return AABBApplyXform(modelspaceBounds, modelMtx);
    }

    // returns the groundspace bounds center point of the mesh
    glm::vec3 GetAABBCenterPointInGround(MeshEl const& mesh)
    {
        AABB bounds = GetGroundspaceBounds(*mesh.MeshData, GetModelMatrix(mesh));
        return AABBCenter(bounds);
    }

    // returns a unique, generated body name
    std::string GenerateBodyName()
    {
        static std::atomic<int> g_LatestBodyIdx = 0;

        std::stringstream ss;
        ss << "body" << g_LatestBodyIdx++;
        return std::move(ss).str();
    }

    // a body scene element
    //
    // In this mesh importer, bodies are positioned + oriented in ground (see MeshEl for explanation of why).
    class BodyEl final {
    public:
        BodyEl(IDT<BodyEl> id,
               std::string name,
               Ras xform) :

            ID{id},
            Name{std::move(name)},
            Xform{xform}
        {
        }

        IDT<BodyEl> ID;
        std::string Name;
        Ras Xform;
        double Mass = 1.0f;  // required: OpenSim goes bananas if a body has a mass <= 0
    };

    // one "side" (attachment) of a joint
    //
    // In OpenSim, a Joint fixes two frames together. If the frames of the two things being joined
    // (e.g. bodies) need to be oriented + translated away from the actual joint center, it's
    // assumed that the model designer knows to add `OpenSim::PhysicalOffsetFrame`s that have a
    // translation + orientation from the thing being joined in the model.
    //
    // In this model editor, a joint always links two bodies (or a body+ground) together. It is assumed
    // that the joint attachment *always* requires some degree of translation+reorientation on each end
    // of the joint - even if it's just zero. The implementation will *always* emit two
    // `OpenSim::PhysicalOffsetFrame`s at each end of the joint - even if they are technically
    // unnecessary in "pure" OpenSim (e.g. the zero case).
    class JointAttachment final {
    public:
        JointAttachment(IDT<BodyEl> bodyID) : BodyID{bodyID} {}
        JointAttachment(IDT<BodyEl> bodyID, Ras xform) : BodyID{bodyID}, Xform{xform} {}

        IDT<BodyEl> BodyID;
        Ras Xform = {};
    };

    // a joint scene element
    //
    // see `JointAttachment` comment for an explanation of why it's designed this way.
    class JointEl final {
    public:
        JointEl(IDT<JointEl> id,
                size_t jointTypeIdx,
                std::string userAssignedName,  // can be empty
                JointAttachment parent,
                JointAttachment child) :

            ID{id},
            JointTypeIndex{jointTypeIdx},
            UserAssignedName{std::move(userAssignedName)},
            Parent{parent},
            Child{child}
        {
        }

        IDT<JointEl> ID;
        size_t JointTypeIndex;
        std::string UserAssignedName;
        JointAttachment Parent;  // can be ground
        JointAttachment Child;  // can't be ground
    };

    // returns a human-readable typename for the joint
    std::string const& GetJointTypeName(JointEl const& joint)
    {
        return JointRegistry::nameStrings()[joint.JointTypeIndex];
    }

    // returns a human-readable name for the joint
    std::string const& GetJointLabel(JointEl const& joint)
    {
        return joint.UserAssignedName.empty() ? GetJointTypeName(joint) : joint.UserAssignedName;
    }

    // returns `true` if body is either the parent or the child attachment of `joint`
    bool IsAttachedToJoint(JointEl const& joint, BodyEl const& body)
    {
        return joint.Parent.BodyID == body.ID || joint.Child.BodyID == body.ID;
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
        std::unordered_map<IDT<MeshEl>, MeshEl> const& GetMeshes() const { return m_Meshes; }
        std::unordered_map<IDT<BodyEl>, BodyEl> const& GetBodies() const { return m_Bodies; }
        std::unordered_map<IDT<JointEl>, JointEl> const& GetJoints() const { return m_Joints; }
        std::unordered_set<ID> const& GetSelected() const { return m_Selected; }

        bool ContainsMeshEl(ID id) const { return TryGetMeshElByID(id) != nullptr; }
        bool ContainsBodyEl(ID id) const { return TryGetBodyElByID(id) != nullptr; }
        bool ContainsJointEl(ID id) const { return TryGetJointElByID(id) != nullptr; }

        MeshEl const* TryGetMeshElByID(ID id) const { return const_cast<ModelGraph&>(*this).TryUpdMeshElByID(id); }
        BodyEl const* TryGetBodyElByID(ID id) const { return const_cast<ModelGraph&>(*this).TryUpdBodyElByID(id); }
        JointEl const* TryGetJointElByID(ID id) const { return const_cast<ModelGraph&>(*this).TryUpdJointElByID(id); }

        MeshEl const& GetMeshByIDOrThrow(ID id) const { return const_cast<ModelGraph&>(*this).UpdMeshByIDOrThrow(id); }
        BodyEl const& GetBodyByIDOrThrow(ID id) const { return const_cast<ModelGraph&>(*this).UpdBodyByIDOrThrow(id); }
        JointEl const& GetJointByIDOrThrow(ID id) const { return const_cast<ModelGraph&>(*this).UpdJointByIDOrThrow(id); }

        IDT<BodyEl> AddBody(std::string name, glm::vec3 const& position, glm::vec3 const& orientation)
        {
            IDT<BodyEl> id = GenerateIDT<BodyEl>();
            return m_Bodies.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, name, Ras{orientation, position})).first->first;
        }

        IDT<MeshEl> AddMesh(std::shared_ptr<Mesh> mesh, IDT<BodyEl> attachment, std::filesystem::path const& path)
        {
            if (attachment != g_GroundID && !ContainsBodyEl(attachment)) {
                throw std::runtime_error{"implementation error: tried to assign a body to a mesh, but the body does not exist"};
            }

            IDT<MeshEl> id = GenerateIDT<MeshEl>();
            return m_Meshes.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, attachment, mesh, path)).first->first;
        }

        IDT<JointEl> AddJoint(size_t jointTypeIdx, std::string maybeName, JointAttachment parentAttachment, JointAttachment childAttachment)
        {
            IDT<JointEl> id = GenerateIDT<JointEl>();
            return m_Joints.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, jointTypeIdx, maybeName, parentAttachment, childAttachment)).first->first;
        }

        void SetMeshAttachmentPoint(IDT<MeshEl> meshID, IDT<BodyEl> bodyID)
        {
            UpdMeshByIDOrThrow(meshID).Attachment = bodyID;
        }

        void UnsetMeshAttachmentPoint(IDT<MeshEl> meshID)
        {
            UpdMeshByIDOrThrow(meshID).Attachment = g_GroundID;
        }

        void SetMeshXform(IDT<MeshEl> meshID, Ras const& newXform)
        {
            UpdMeshByIDOrThrow(meshID).Xform = newXform;
        }

        void SetMeshScaleFactors(IDT<MeshEl> meshID, glm::vec3 const& newScaleFactors)
        {
            UpdMeshByIDOrThrow(meshID).ScaleFactors = newScaleFactors;
        }

        void SetMeshName(IDT<MeshEl> meshID, std::string_view newName)
        {
            UpdMeshByIDOrThrow(meshID).Name = newName;
        }

        void SetBodyName(IDT<BodyEl> bodyID, std::string_view newName)
        {
            UpdBodyByIDOrThrow(bodyID).Name = newName;
        }

        void SetBodyXform(IDT<BodyEl> bodyID, Ras const& newXform)
        {
            UpdBodyByIDOrThrow(bodyID).Xform = newXform;
        }

        void SetBodyMass(IDT<BodyEl> bodyID, double newMass)
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

        void DeleteMeshElByID(ID id)
        {
            auto it = m_Meshes.find(DowncastID<MeshEl>(id));
            if (it != m_Meshes.end()) {
                DeleteMesh(it);
            }
        }

        void DeleteBodyElByID(ID id)
        {
            auto it = m_Bodies.find(DowncastID<BodyEl>(id));
            if (it != m_Bodies.end()) {
                DeleteBody(it);
            }
        }

        void DeleteJointElByID(ID id)
        {
            auto it = m_Joints.find(DowncastID<JointEl>(id));
            if (it != m_Joints.end()) {
                DeleteJoint(it);
            }
        }

        void DeleteElementByID(ID id)
        {
            DeleteMeshElByID(id);
            DeleteBodyElByID(id);
            DeleteJointElByID(id);
        }

        void ApplyTranslation(ID id, glm::vec3 const& translation)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                meshPtr->Xform.shift += translation;
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                bodyPtr->Xform.shift += translation;
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                jointPtr->Parent.Xform.shift += translation;
                jointPtr->Child.Xform.shift += translation;
            }
        }

        void ApplyRotation(ID id, glm::vec3 const& eulerAngles)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                meshPtr->Xform.rot = EulerCompose(meshPtr->Xform.rot, eulerAngles);
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                bodyPtr->Xform.rot = EulerCompose(bodyPtr->Xform.rot, eulerAngles);
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                jointPtr->Parent.Xform.rot = EulerCompose(jointPtr->Parent.Xform.rot, eulerAngles);
                jointPtr->Child.Xform.rot = EulerCompose(jointPtr->Child.Xform.rot, eulerAngles);
            }
        }

        void ApplyScale(ID id, glm::vec3 const& scaleFactors)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                meshPtr->ScaleFactors *= scaleFactors;
            }
        }

        glm::vec3 GetTranslationInGround(ID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return meshPtr->Xform.shift;
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return bodyPtr->Xform.shift;
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                return (jointPtr->Parent.Xform.shift + jointPtr->Child.Xform.shift) / 2.0f;
            } else {
                throw std::runtime_error{"GetTranslation(): cannot find element by ID"};
            }
        }

        glm::vec3 GetOrientationInGround(ID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return meshPtr->Xform.rot;
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return bodyPtr->Xform.rot;
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                return (jointPtr->Parent.Xform.rot + jointPtr->Child.Xform.rot) / 2.0f;
            } else {
                throw std::runtime_error{"GetOrientationInground(): cannot find element by ID"};
            }
        }

        Ras GetTranslationOrientationInGround(ID id) const
        {
            return {GetOrientationInGround(id), GetTranslationInGround(id)};
        }

        // returns empty AABB at point if a point-like element (e.g. mesh, joint pivot)
        AABB GetBounds(ID id) const
        {
            if (id == g_GroundID) {
                return {};
            } else if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                AABB boundsInModelSpace = meshPtr->MeshData->getAABB();
                return AABBApplyXform(boundsInModelSpace, GetModelMatrix(*meshPtr));
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return AABB{bodyPtr->Xform.shift, bodyPtr->Xform.shift};
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                glm::vec3 points[] = {jointPtr->Parent.Xform.shift, jointPtr->Child.Xform.shift};
                return AABBFromVerts(points, 2);
            } else {
                throw std::runtime_error{"GetBounds(): could not find supplied ID"};
            }
        }

        void SelectAll()
        {
            auto addIDToSelectionSet = [this](ID id) { m_Selected.insert(id); };
            ForEachSceneElID(addIDToSelectionSet);
        }

        void DeSelectAll()
        {
            m_Selected.clear();
        }

        void Select(ID id)
        {
            m_Selected.insert(id);
        }

        void DeSelect(ID id)
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

        bool IsSelected(ID id) const
        {
            return m_Selected.find(id) != m_Selected.end();
        }

        void DeleteSelected()
        {
            auto selected = m_Selected;  // copy to ensure iterator invalidation doesn't screw us
            for (ID id : selected) {
                DeleteElementByID(id);
            }
            m_Selected.clear();
        }

    private:
        MeshEl* TryUpdMeshElByID(ID id)
        {
            auto it = m_Meshes.find(DowncastID<MeshEl>(id));
            return it != m_Meshes.end() ? &it->second : nullptr;
        }

        BodyEl* TryUpdBodyElByID(ID id)
        {
            auto it = m_Bodies.find(DowncastID<BodyEl>(id));
            return it != m_Bodies.end() ? &it->second : nullptr;
        }

        JointEl* TryUpdJointElByID(ID id)
        {
            auto it = m_Joints.find(DowncastID<JointEl>(id));
            return it != m_Joints.end() ? &it->second : nullptr;
        }

        MeshEl& UpdMeshByIDOrThrow(ID id)
        {
            MeshEl* meshEl = TryUpdMeshElByID(id);
            if (!meshEl) {
                throw std::runtime_error{"could not find a mesh"};
            }
            return *meshEl;
        }

        BodyEl& UpdBodyByIDOrThrow(ID id)
        {
            BodyEl* bodyEl = TryUpdBodyElByID(id);
            if (!bodyEl) {
                throw std::runtime_error{"could not find a body"};
            }
            return *bodyEl;
        }

        JointEl& UpdJointByIDOrThrow(ID id)
        {
            JointEl* jointEl = TryUpdJointElByID(id);
            if (!jointEl) {
                throw std::runtime_error{"could not find a joint"};
            }
            return *jointEl;
        }

        void DeleteMesh(std::unordered_map<IDT<MeshEl>, MeshEl>::iterator it)
        {
            DeSelect(it->first);
            m_Meshes.erase(it);
        }

        void DeleteBody(std::unordered_map<IDT<BodyEl>, BodyEl>::iterator it)
        {
            auto const& [bodyID, bodyEl] = *it;

            // delete any joints that reference the body
            for (auto jointIt = m_Joints.begin(); jointIt != m_Joints.end(); ++jointIt) {
                if (IsAttachedToJoint(jointIt->second, bodyEl)) {
                    DeSelect(jointIt->first);
                    jointIt = m_Joints.erase(jointIt);
                }
            }

            // delete any meshes attached to the body
            for (auto meshIt = m_Meshes.begin(); meshIt != m_Meshes.end(); ++meshIt) {
                if (meshIt->second.Attachment == bodyID) {
                    DeSelect(meshIt->first);
                    meshIt = m_Meshes.erase(meshIt);
                }
            }

            // delete the body
            DeSelect(it->first);
            m_Bodies.erase(it);
        }

        void DeleteJoint(std::unordered_map<IDT<JointEl>, JointEl>::iterator it)
        {
            DeSelect(it->first);
            m_Joints.erase(it);
        }

        std::unordered_map<IDT<MeshEl>, MeshEl> m_Meshes;
        std::unordered_map<IDT<BodyEl>, BodyEl> m_Bodies;
        std::unordered_map<IDT<JointEl>, JointEl> m_Joints;
        std::unordered_set<ID> m_Selected;
    };

    // try to find the parent body of the given joint
    //
    // returns nullptr if the joint's parent has an invalid id or is attached to ground
    BodyEl const* TryGetParentBody(ModelGraph const& modelGraph, JointEl const& joint)
    {   
        if (joint.Parent.BodyID == g_GroundID) {
            return nullptr;
        } else {
            return modelGraph.TryGetBodyElByID(joint.Parent.BodyID);
        }
    }

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(ModelGraph const& modelGraph, BodyEl const& body)
    {
        auto IsBodyAttachedToJointAsChild = [&body](auto const& pair)
        {
            return pair.second.Child.BodyID == body.ID;
        };

        return AnyOf(modelGraph.GetJoints(), IsBodyAttachedToJointAsChild);
    }

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(ModelGraph const& modelGraph, JointEl const& jointEl)
    {
        if (jointEl.Child.BodyID == g_GroundID) {
            return true;  // ground cannot be a child in a joint
        }

        if (jointEl.Parent.BodyID == jointEl.Child.BodyID) {
            return true;  // is directly attached to itself
        }

        if (jointEl.Parent.BodyID != g_GroundID && !modelGraph.ContainsBodyEl(jointEl.Parent.BodyID)) {
             return true;  // has a parent ID that's invalid for this model graph
        }

        if (!modelGraph.ContainsBodyEl(jointEl.Child.BodyID)) {
            return true;  // has a child ID that's invalid for this model graph
        }

        return false;
    }

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
                                BodyEl const& body,
                                std::unordered_set<ID>& previouslyVisitedJoints);

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(ModelGraph const& modelGraph,
                                 JointEl const& joint,
                                 std::unordered_set<ID>& previousVisits)
    {
        OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, joint));

        BodyEl const* parent = TryGetParentBody(modelGraph, joint);

        if (!parent) {
            return true;  // joints use nullptr as senteniel for Ground
        }

        return IsBodyAttachedToGround(modelGraph, *parent, previousVisits);
    }

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
                                BodyEl const& body,
                                std::unordered_set<ID>& previouslyVisitedJoints)
    {
        for (auto const& [jointID, jointEl] : modelGraph.GetJoints()) {
            OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, jointEl));

            if (jointEl.Child.BodyID == body.ID) {
                bool alreadyVisited = !previouslyVisitedJoints.emplace(jointEl.ID).second;
                if (alreadyVisited) {
                    return false;  // cycle detected
                } else {
                    // body participates as a child in a joint - check if the joint indirectly connects to ground
                    return IsJointAttachedToGround(modelGraph, jointEl, previouslyVisitedJoints);
                }
            }
        }
        return true;  // non-participating bodies will be attached to ground automatically
    }

    // returns `true` if `modelGraph` contains issues
    bool GetModelGraphIssues(ModelGraph const& modelGraph, std::vector<std::string>& issuesOut)
    {
        issuesOut.clear();

        for (auto const& [id, joint] : modelGraph.GetJoints()) {
            if (IsGarbageJoint(modelGraph, joint)) {
                std::stringstream ss;
                ss << GetJointLabel(joint) << ": joint is garbage (this is an implementation error)";
                throw std::runtime_error{std::move(ss).str()};
            }
        }

        for (auto const& [id, body] : modelGraph.GetBodies()) {
            std::unordered_set<ID> previouslyVisitedJoints;
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
                             Ras const& parentTranslationAndOrientationInGround,
                             OpenSim::PhysicalFrame& parentPhysFrame)
    {
        // create a POF that attaches to the body
        auto meshPhysOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
        meshPhysOffsetFrame->setName(meshEl.Name + "_offset");

        // re-express the transform matrix in the parent's frame
        glm::mat4 parent2ground = XFormFromRas(parentTranslationAndOrientationInGround);
        glm::mat4 ground2parent = glm::inverse(parent2ground);
        glm::mat4 mesh2ground = XFormFromRas(meshEl.Xform);
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
                                                      std::unordered_map<ID, OpenSim::Body*>& visitedBodies,
                                                      IDT<BodyEl> elID)
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

    // returns the translation and orientation of the given body in ground
    //
    // if the supplied ID is empty, returns the translation and orientation of ground itself (i.e. 0.0...)
    Ras GetBodyTranslationAndOrientationInGround(ModelGraph const& mg, IDT<BodyEl> bodyID)
    {
        if (bodyID == g_GroundID) {
            return Ras{};
        }

        BodyEl const* body = mg.TryGetBodyElByID(bodyID);

        if (!body) {
            throw std::runtime_error{"cannot get the position of this body: the ID is invalid"};
        }

        return body->Xform;
    }

    // returns a `TranslationOrientation` that can reorient things expressed in base to things expressed in parent
    Ras TranslationOrientationInParent(Ras const& parentInBase, Ras const& childInBase)
    {
        glm::mat4 parent2base = XFormFromRas(parentInBase);
        glm::mat4 base2parent = glm::inverse(parent2base);
        glm::mat4 child2base = XFormFromRas(childInBase);
        glm::mat4 child2parent = base2parent * child2base;

        glm::vec3 translation = base2parent * glm::vec4{childInBase.shift, 1.0f};
        glm::vec3 orientation = MatToEulerAngles(child2parent);

        return Ras{orientation, translation};
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
            std::stringstream msg;
            msg << "GetDegreesOfFreedom: coordinate fixups for joint type '" << proto->getConcreteClassName() << "' not yet supported: needs to be implemented";
            throw std::runtime_error{std::move(msg).str()};
        }
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

    // returns true if the value is effectively zero
    bool IsEffectivelyZero(float v)
    {
        return std::fabs(v) < 1e-6;
    }

    // sets joint coordinate default values based on the difference between its two attached frames
    //
    // throws if the joint does not contain a relevant coordinate to rectify the difference
    void SetJointCoordinatesBasedOnFrameDifferences(OpenSim::Joint& joint,
                                                    Ras const& parentPofInGround,
                                                    Ras const& childPofInGround)
    {
        size_t jointTypeIdx = *JointRegistry::indexOf(joint);

        std::cerr << "parentPofInGround = " << parentPofInGround << '\n';
        std::cerr << "childPofInGround = " << childPofInGround << '\n';

        Ras toInParent = TranslationOrientationInParent(parentPofInGround, childPofInGround);
        std::cerr << toInParent << '\n';

        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(jointTypeIdx);

        // handle translations
        for (int i = 0; i < 3; ++i) {
            if (dofs.translation[i] == -1) {
                if (!IsEffectivelyZero(toInParent.shift[i])) {
                    throw std::runtime_error{"invalid POF translation: joint offset frames have a nonzero translation but the joint doesn't have a coordinate along the necessary DoF"};
                }
            } else {
                joint.upd_coordinates(dofs.translation[i]).setDefaultValue(toInParent.shift[i]);
            }
        }

        // handle orientations
        for (int i = 0; i < 3; ++i) {
            if (dofs.orientation[i] == -1) {
                if (!IsEffectivelyZero(toInParent.rot[i])) {
                    throw std::runtime_error{"invalid POF rotation: joint offset frames have a nonzero rotation but the joint doesn't have a coordinate along the necessary DoF"};
                }
            } else {
                joint.upd_coordinates(dofs.orientation[i]).setDefaultValue(toInParent.rot[i]);
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
                              std::unordered_map<ID, OpenSim::Body*>& visitedBodies,
                              std::unordered_set<ID>& visitedJoints)
    {
        if (auto const& [it, wasInserted] = visitedJoints.emplace(joint.ID); !wasInserted) {
            return;  // graph cycle detected: joint was already previously visited and shouldn't be traversed again
        }

        // lookup each side of the joint, creating the bodies if necessary
        JointAttachmentCachedLookupResult parent = LookupPhysFrame(mg, model, visitedBodies, joint.Parent.BodyID);
        JointAttachmentCachedLookupResult child = LookupPhysFrame(mg, model, visitedBodies, joint.Child.BodyID);

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parentPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parentPOF->setName(parent.physicalFrame->getName() + "_offset");
        parentPOF->setParentFrame(*parent.physicalFrame);
        Ras toParentInGround = GetBodyTranslationAndOrientationInGround(mg, joint.Parent.BodyID);
        Ras toParentPofInGround = joint.Parent.Xform;
        Ras toParentPofInParent = TranslationOrientationInParent(toParentInGround, toParentPofInGround);
        parentPOF->set_translation(SimTKVec3FromV3(toParentPofInParent.shift));
        parentPOF->set_orientation(SimTKVec3FromV3(toParentPofInParent.rot));

        // create the child OpenSim::PhysicalOffsetFrame
        auto childPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        childPOF->setName(child.physicalFrame->getName() + "_offset");
        childPOF->setParentFrame(*child.physicalFrame);
        Ras toChildInGround = GetBodyTranslationAndOrientationInGround(mg, joint.Child.BodyID);
        Ras toChildPofInGround = joint.Child.Xform;
        Ras toChildPofInChild = TranslationOrientationInParent(toChildInGround, toChildPofInGround);
        childPOF->set_translation(SimTKVec3FromV3(toChildPofInChild.shift));
        childPOF->set_orientation(SimTKVec3FromV3(toChildPofInChild.rot));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto jointUniqPtr = ConstructOpenSimJointFromTypeIndex(joint.JointTypeIndex);

        // set its name
        jointUniqPtr->setName(CalcJointName(joint, *parent.physicalFrame, *child.physicalFrame));

        // set its default coordinate values based on the differences between the user-specified offset frames
        //
        // e.g. if the two ground-defined offset frames are separated by (0, 1, 0), then the joint should set
        //      its y translation to +1 (if possible). If it isn't possible to translate/rotate the joint then
        //      we have a UI error (the UI should only allow the user to change the frame locations based on
        //      the joint type)
        SetJointCoordinatesBasedOnFrameDifferences(*jointUniqPtr, toParentPofInGround, toChildPofInGround);

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
            model.addBody(child.createdBody.release());
        }

        // add the joint to the model
        model.addJoint(jointUniqPtr.release());

        // recurse by finding where the child of this joint is the parent of some other joint
        OSC_ASSERT_ALWAYS(child.bodyEl != nullptr && "child should always be an identifiable body element");
        for (auto const& [otherJointID, otherJoint] : mg.GetJoints()) {
            if (otherJoint.Parent.BodyID == child.bodyEl->ID) {
                AttachJointRecursive(mg, model, otherJoint, visitedBodies, visitedJoints);
            }
        }
    }

    // attaches `BodyEl` into `model` by directly attaching it to ground
    void AttachBodyDirectlyToGround(ModelGraph const& mg,
                                    OpenSim::Model& model,
                                    BodyEl const& bodyEl,
                                    std::unordered_map<ID, OpenSim::Body*>& visitedBodies)
    {
        auto addedBody = CreateDetatchedBody(mg, bodyEl);
        auto joint = std::make_unique<OpenSim::FreeJoint>();

        // set joint name
        joint->setName(bodyEl.Name + "_to_ground");

        // set joint coordinate names
        SetJointCoordinateNames(*joint, bodyEl.Name);

        // set joint's default location of the body's position in the ground
        SetJointCoordinatesBasedOnFrameDifferences(*joint, Ras{}, bodyEl.Xform);

        // connect joint from ground to the body
        joint->connectSocket_parent_frame(model.getGround());
        joint->connectSocket_child_frame(*addedBody);

        // populate it in the "already visited bodies" cache
        visitedBodies[bodyEl.ID] = addedBody.get();

        // add the body + joint to the output model
        model.addBody(addedBody.release());
        model.addJoint(joint.release());
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
        for (auto const& [meshID, mesh] : mg.GetMeshes()) {
            if (mesh.Attachment == g_GroundID) {
                AttachMeshElToFrame(mesh, Ras{}, model->updGround());
            }
        }

        // keep track of any bodies/joints already visited (there might be cycles)
        std::unordered_map<ID, OpenSim::Body*> visitedBodies;
        std::unordered_set<ID> visitedJoints;

        // add any bodies that participate in no joints into the model with a freejoint
        for (auto const& [bodyID, body] : mg.GetBodies()) {
            if (!IsAChildAttachmentInAnyJoint(mg, body)) {
                AttachBodyDirectlyToGround(mg, *model, body, visitedBodies);
            }
        }

        // add bodies that do participate in joints into the model
        //
        // note: these bodies may use the non-participating bodies (above) as parents
        for (auto const& [jointID, joint] : mg.GetJoints()) {
            if (joint.Parent.BodyID == g_GroundID || ContainsKey(visitedBodies, joint.Parent.BodyID)) {
                AttachJointRecursive(mg, *model, joint, visitedBodies, visitedJoints);
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
        ID id = g_EmptyID;
        ID groupId = g_EmptyID;
        std::shared_ptr<Mesh> mesh;
        glm::mat4x3 modelMatrix;
        glm::mat3x3 normalMatrix;
        glm::vec4 color;
        float rimColor;
        std::shared_ptr<gl::Texture2D> maybeDiffuseTex;
    };

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

        glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};

        glm::mat4 projMat = camera.getProjMtx(VecAspectRatio(dims));
        glm::mat4 viewMat = camera.getViewMtx();
        glm::vec3 viewPos = camera.getPos();

        auto samples = App::cur().getSamples();

        gl::RenderBuffer sceneRBO = MultisampledRenderBuffer(samples, GL_RGBA, dims);
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
                    gl::Uniform(shader.uSampler0, gl::textureIndex<GL_TEXTURE0>());
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
            gl::Uniform(eds.uRimRgba,  glm::vec4{1.0f, 0.4f, 0.0f, 0.85f});
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
        Hover(ID id_, glm::vec3 pos_) : ID{id_}, Pos{pos_} {}
        operator bool () const noexcept { return ID != g_EmptyID; }

        ID ID;
        glm::vec3 Pos;
    };

    class SharedData final {
    public:
        SharedData() = default;

        SharedData(std::vector<std::filesystem::path> meshFiles)
        {
            for (auto const& meshFile : meshFiles) {
                PushMeshLoadRequest(meshFile);
            }
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
            m_MaybeOutputModel = CreateOpenSimModelFromModelGraph(GetCurrentModelGraph(), m_Issues);
        }

        ModelGraph const& GetCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.Current();
        }

        ModelGraph& UpdCurrentModelGraph()
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

        std::unordered_set<ID> const& GetCurrentSelection() const
        {
            return GetCurrentModelGraph().GetSelected();
        }

        void SelectAll()
        {
            UpdCurrentModelGraph().SelectAll();
        }

        void DeSelectAll()
        {
            UpdCurrentModelGraph().DeSelectAll();
        }

        void Select(ID id)
        {
            UpdCurrentModelGraph().Select(id);
        }

        void DeSelect(ID id)
        {
            UpdCurrentModelGraph().DeSelect(id);
        }

        bool HasSelection() const
        {
            return GetCurrentModelGraph().HasSelection();
        }

        bool IsSelected(ID id) const
        {
            return GetCurrentModelGraph().IsSelected(id);
        }

        void DeleteSelected()
        {
            if (!HasSelection()) {
                return;
            }
            UpdCurrentModelGraph().DeleteSelected();
            CommitCurrentModelGraph("deleted selection");
        }

        IDT<BodyEl> AddBody(std::string const& name, glm::vec3 const& pos, glm::vec3 const& orientation)
        {
            auto id = UpdCurrentModelGraph().AddBody(name, pos, orientation);
            UpdCurrentModelGraph().DeSelectAll();
            UpdCurrentModelGraph().Select(id);
            CommitCurrentModelGraph(std::string{"added "} + name);
            return id;
        }

        IDT<BodyEl> AddBody(glm::vec3 const& pos)
        {
            return AddBody(GenerateBodyName(), pos, {});
        }

        void PushMeshLoadRequest(std::filesystem::path const& meshFilePath, IDT<BodyEl> bodyToAttachTo)
        {
            m_MeshLoader.send(MeshLoadRequest{bodyToAttachTo, meshFilePath});
        }

        void PushMeshLoadRequest(std::filesystem::path const& meshFilePath)
        {
            m_MeshLoader.send(MeshLoadRequest{g_GroundID, meshFilePath});
        }

        // called when the mesh loader responds with a fully-loaded mesh
        void PopMeshLoader_OnOKResponse(MeshLoadOKResponse& ok)
        {
            ModelGraph& mg = UpdCurrentModelGraph();

            auto meshID = mg.AddMesh(ok.mesh, ok.PreferredAttachmentPoint, ok.Path);

            auto const* maybeBody = mg.TryGetBodyElByID(ok.PreferredAttachmentPoint);
            if (maybeBody) {
                mg.SetMeshXform(meshID, maybeBody->Xform);
            }

            std::stringstream commitMsgSS;
            commitMsgSS << "loaded " << ok.Path.filename();
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
            for (auto const& meshFile : PromptUserForMeshFiles()) {
                PushMeshLoadRequest(meshFile);
            }
        }

        glm::vec2 WorldPosToScreenPos(glm::vec3 const& worldPos) const
        {
            return GetCamera().projectOntoScreenRect(worldPos, Get3DSceneRect());
        }

        void DrawConnectionLine(ImU32 color, glm::vec2 parent, glm::vec2 child) const
        {
            // the line
            ImGui::GetWindowDrawList()->AddLine(parent, child, color, 2.0f);

            // triangle indicating connection directionality
            glm::vec2 midpoint = (parent + child) / 2.0f;
            glm::vec2 direction = glm::normalize(parent - child);
            glm::vec2 normal = {-direction.y, direction.x};

            constexpr float triangleWidth = 10.0f;
            glm::vec2 p1 = midpoint + (triangleWidth/2.0f)*normal;
            glm::vec2 p2 = midpoint - (triangleWidth/2.0f)*normal;
            glm::vec2 p3 = midpoint + triangleWidth*direction;

            ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, color);
        }

        void DrawConnectionLine(MeshEl const& meshEl, ImU32 color) const
        {
            glm::vec3 meshLoc = meshEl.Xform.shift;
            glm::vec3 otherLoc = GetCurrentModelGraph().GetTranslationInGround(meshEl.Attachment);

            DrawConnectionLine(color, WorldPosToScreenPos(otherLoc), WorldPosToScreenPos(meshLoc));
        }

        void DrawConnectionLineToGround(BodyEl const& bodyEl, ImU32 color) const
        {
            glm::vec3 bodyLoc = bodyEl.Xform.shift;
            glm::vec3 otherLoc = {};

            DrawConnectionLine(color, WorldPosToScreenPos(otherLoc), WorldPosToScreenPos(bodyLoc));
        }

        void DrawConnectionLine(JointEl const& jointEl, ImU32 color, ID excludeID = g_EmptyID) const
        {
            if (jointEl.ID == excludeID) {
                return;
            }

            if (jointEl.Child.BodyID != excludeID) {
                glm::vec3 childLoc = GetCurrentModelGraph().GetTranslationInGround(jointEl.Child.BodyID);
                glm::vec3 childPivotLoc = jointEl.Child.Xform.shift;
                DrawConnectionLine(color, WorldPosToScreenPos(childPivotLoc), WorldPosToScreenPos(childLoc));
            }

            if (jointEl.Parent.BodyID != excludeID) {
                glm::vec3 parentLoc = GetCurrentModelGraph().GetTranslationInGround(jointEl.Parent.BodyID);
                glm::vec3 parentPivotLoc = jointEl.Parent.Xform.shift;
                DrawConnectionLine(color, WorldPosToScreenPos(parentLoc), WorldPosToScreenPos(parentPivotLoc));
            }
        }

        void DrawConnectionLines() const
        {
            DrawConnectionLines({0.0f, 0.0f, 0.0f, 0.33f});
        }

        void DrawConnectionLines(ImVec4 colorVec, ID excludeID = g_EmptyID) const
        {
            ModelGraph const& mg = GetCurrentModelGraph();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);

            // draw each mesh's connection line
            for (auto const& [meshID, meshEl] : mg.GetMeshes()) {
                if (meshID == excludeID) {
                    continue;
                }

                DrawConnectionLine(meshEl, color);
            }

            // draw connection lines for bodies that have a direct (implicit) connection to ground
            // because they do not participate as a child of any joint
            for (auto const& [bodyID, bodyEl] : mg.GetBodies()) {
                if (bodyID == excludeID) {
                    continue;
                }

                if (IsAChildAttachmentInAnyJoint(mg, bodyEl)) {
                    continue;  // will be handled during joint drawing
                }

                DrawConnectionLineToGround(bodyEl, color);
            }

            // draw connection lines for each joint
            for (auto const& [jointID, jointEl] : mg.GetJoints()) {
                if (jointID == excludeID) {
                    continue;
                }

                DrawConnectionLine(jointEl, color, excludeID);
            }
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
                GetBgColor(),
                drawables,
                UpdSceneTex());

            // send texture to ImGui
            DrawTextureAsImGuiImage(UpdSceneTex(), RectDims(Get3DSceneRect()));

            // handle hittesting, etc.
            SetIsRenderHovered(ImGui::IsItemHovered());
        }

        bool IsShowingMeshes() const { return m_IsShowingMeshes; }
        bool IsShowingBodies() const { return m_IsShowingBodies; }
        bool IsShowingGround() const { return m_IsShowingGround; }
        bool IsShowingFloor() const { return m_IsShowingFloor; }
        bool IsShowingAllConnectionLines() const { return m_IsShowingAllConnectionLines; }
        bool IsMeshesInteractable() const { return m_IsMeshesInteractable; }
        bool IsBodiesInteractable() const { return m_IsBodiesInteractable; }
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

        glm::vec4 const& GetBgColor() const { return m_3DSceneBgColor; }
        gl::Texture2D& UpdSceneTex() { return m_3DSceneTex; }

        glm::vec4 const& GetMeshColor() const { return m_Colors.mesh; }
        glm::vec4 const& GetBodyColor() const { return m_Colors.body; }
        glm::vec4 const& GetGroundColor() const { return m_Colors.ground; }

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
            dt.color = {0.0f, 0.0f, 0.0f, 1.0f};  // doesn't matter: it's textured
            dt.rimColor = 0.0f;
            dt.maybeDiffuseTex = m_FloorChequerTex;
            return dt;
        }

        DrawableThing GenerateMeshElDrawable(MeshEl const& meshEl, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = meshEl.ID;
            rv.groupId = g_MeshGroupID;
            rv.mesh = meshEl.MeshData;
            rv.modelMatrix = GetModelMatrix(meshEl);
            rv.normalMatrix = NormalMatrix(rv.modelMatrix);
            rv.color = color;
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        float GetSphereRadius() const
        {
            return 0.0125f * m_SceneScaleFactor;
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

        void AppendAsFrame(ID logicalID,
                           ID groupID,
                           Ras const& translationOrientation,
                           std::vector<DrawableThing>& appendOut,
                           float alpha = 1.0f) const
        {
            // stolen from SceneGeneratorNew.cpp

            glm::vec3 origin = translationOrientation.shift;
            glm::mat3 rotation = EulerAnglesToMat(translationOrientation.rot);

            // emit origin sphere
            {
                Sphere centerSphere{origin, GetSphereRadius()};

                DrawableThing& sphere = appendOut.emplace_back();
                sphere.id = logicalID;
                sphere.groupId = groupID;
                sphere.mesh = m_SphereMesh;
                sphere.modelMatrix = SphereMeshToSceneSphereXform(centerSphere);
                sphere.normalMatrix = NormalMatrix(sphere.modelMatrix);
                sphere.color = {1.0f, 1.0f, 1.0f, alpha};
                sphere.rimColor = 0.0f;
                sphere.maybeDiffuseTex = nullptr;
            }

            // emit "legs"
            Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
            for (int i = 0; i < 3; ++i) {
                glm::vec3 dir = {0.0f, 0.0f, 0.0f};
                dir[i] = 4.0f * GetSphereRadius();
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
                se.rimColor = 0.0f;
                se.maybeDiffuseTex = nullptr;
            }
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

            ID closestID = g_EmptyID;
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

        // scene colors
        struct {
            glm::vec4 mesh = {1.0f, 1.0f, 1.0f, 1.0f};
            glm::vec4 ground = {0.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 body = {0.0f, 0.0f, 0.0f, 1.0f};
        } m_Colors;

        glm::vec4 m_3DSceneBgColor = {0.89f, 0.89f, 0.89f, 1.0f};

        // scale factor for all non-mesh, non-overlay scene elements (e.g.
        // the floor, bodies)
        //
        // this is necessary because some meshes can be extremely small/large and
        // scene elements need to be scaled accordingly (e.g. without this, a body
        // sphere end up being much larger than a mesh instance). Imagine if the
        // mesh was the leg of a fly
        float m_SceneScaleFactor = 1.0f;

        // buffer containing issues found in the modelgraph
        std::vector<std::string> m_Issues;

        // model created by this wizard
        //
        // `nullptr` until the model is successfully created
        std::unique_ptr<OpenSim::Model> m_MaybeOutputModel = nullptr;

        // true if a chequered floor should be drawn
        bool m_IsShowingFloor = true;

        // true if meshes should be drawn
        bool m_IsShowingMeshes = true;

        // true if ground should be drawn
        bool m_IsShowingGround = true;

        // true if bodies should be drawn
        bool m_IsShowingBodies = true;

        // true if all connection lines between entities should be
        // drawn, rather than just *hovered* entities
        bool m_IsShowingAllConnectionLines = true;

        // true if meshes shouldn't be hoverable/clickable in the 3D scene
        bool m_IsMeshesInteractable = true;

        // true if bodies shouldn't be hoverable/clickable in the 3D scene
        bool m_IsBodiesInteractable = true;

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
        MWState(SharedData& sharedData) : m_SharedData{sharedData} {}
        virtual ~MWState() noexcept = default;

        virtual std::unique_ptr<MWState> onEvent(SDL_Event const&) = 0;
        virtual std::unique_ptr<MWState> draw() = 0;
        virtual std::unique_ptr<MWState> tick(float) = 0;

    protected:
        SharedData& m_SharedData;
    };

    // forward-declaration so that subscreens can transition to the standard top-level state
    std::unique_ptr<MWState> CreateStandardState(SharedData&);

    // "mesh assignment" state
    //
    // this is what's shown when the user is trying to assign a mesh to some other
    // body in the scene
    class AssignMeshMWState final : public MWState {
    public:
        AssignMeshMWState(SharedData& sharedData, IDT<MeshEl> meshID) :
            MWState{sharedData},
            m_MeshID{meshID}
        {
        }

        bool IsHoveringABody() const
        {
            return m_Hover && m_Hover.ID != m_MeshID && m_Hover.ID != g_GroundID;
        }

        void UpdateFromImGuiKeyboardState()
        {
            if (ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
                // ESC: transition back to main UI
                m_MaybeNextState = CreateStandardState(m_SharedData);
            }
        }

        void DrawMeshHoverTooltip() const
        {
            if (m_Hover.ID != m_MeshID) {
                return;
            }

            // user is hovering the mesh being assigned
            MeshEl const& meshEl = m_SharedData.GetCurrentModelGraph().GetMeshByIDOrThrow(m_MeshID);

            ImGui::BeginTooltip();
            ImGui::TextUnformatted(meshEl.Name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(click to assign the mesh to ground)");
            ImGui::EndTooltip();
        }

        void DrawBodyHoverTooltip() const
        {
            if (!IsHoveringABody()) {
                return;
            }

            // user is hovering a body that the mesh could attach to
            BodyEl const& bodyEl = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(m_Hover.ID);

            ImGui::BeginTooltip();
            ImGui::TextUnformatted(bodyEl.Name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(click to assign the mesh to this body)");
            ImGui::EndTooltip();
        }

        void DrawGroundHoverTooltip() const
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("ground");
            ImGui::SameLine();
            ImGui::TextDisabled("(click to assign the mesh to ground)");
            ImGui::EndTooltip();
        }

        void DrawHoverTooltip() const
        {
            if (!m_Hover) {
                return;
            } else if (m_Hover.ID == m_MeshID) {
                DrawMeshHoverTooltip();
            } else if (IsHoveringABody()) {
                DrawBodyHoverTooltip();
            } else if (m_Hover.ID == g_GroundID) {
                DrawGroundHoverTooltip();
            }
        }

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            if (!m_SharedData.IsRenderHovered()) {
                m_Hover = Hover{};
                return;
            }

            m_Hover = m_SharedData.Hovertest(drawables);
        }

        void DrawConnectionLines()
        {
            ImVec4 faintColor = {0.0f, 0.0f, 0.0f, 0.2f};
            ImVec4 strongColor = {0.0f, 0.0f, 0.0f, 1.0f};

            if (!m_Hover) {
                // draw all existing connection lines faintly
                m_SharedData.DrawConnectionLines(faintColor);
                return;
            }

            // otherwise, draw all *other* connection lines faintly and then handle
            // the hovertest line drawing
            m_SharedData.DrawConnectionLines(faintColor, m_MeshID);

            // draw hover connection line strongly
            auto meshLoc = m_SharedData.GetCurrentModelGraph().GetMeshByIDOrThrow(m_MeshID).Xform.shift;
            auto strongColorU32 = ImGui::ColorConvertFloat4ToU32(strongColor);
            if (IsHoveringABody()) {
                auto bodyLoc = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(m_Hover.ID).Xform.shift;
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(bodyLoc),
                                                m_SharedData.WorldPosToScreenPos(meshLoc));
            } else if (m_Hover.ID == m_MeshID || m_Hover.ID == g_GroundID) {
                auto groundLoc = glm::vec3{0.0f, 0.0f, 0.0f};
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(groundLoc),
                                                m_SharedData.WorldPosToScreenPos(meshLoc));
            }
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose a body to assign the mesh to (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetWindowDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void Draw3DViewer()
        {
            MeshEl const& meshEl = m_SharedData.GetCurrentModelGraph().GetMeshByIDOrThrow(m_MeshID);

            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            // draw mesh slightly faded and with a rim highlight, but non-clickable
            {
                DrawableThing& meshDrawable = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                meshDrawable.color.a = 0.25f;
                meshDrawable.rimColor = 0.25f;
                meshDrawable.id = g_EmptyID;
                meshDrawable.groupId = g_EmptyID;
            }

            // draw bodies as spheres the user can click
            for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                sceneEls.push_back(m_SharedData.GenerateBodyElSphere(bodyEl, {1.0f, 0.0f, 0.0f, 1.0f}));
            }

            // draw ground as sphere the user can click
            sceneEls.push_back(m_SharedData.GenerateGroundSphere({0.0f, 0.0f, 0.0f, 1.0f}));

            // draw floor
            sceneEls.push_back(m_SharedData.GenerateFloorDrawable());

            // hovertest generated geometry
            DoHovertest(sceneEls);

            // hovertest side-effects
            if (m_Hover) {

                DrawHoverTooltip();

                // handle user clicks
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {

                    if (m_Hover.ID == m_MeshID || m_Hover.ID == g_GroundID) {
                        // user clicked on the mesh: assign mesh to ground (un-assign it)
                        m_SharedData.UpdCurrentModelGraph().UnsetMeshAttachmentPoint(m_MeshID);
                        m_SharedData.CommitCurrentModelGraph("assigned mesh to ground");
                        m_MaybeNextState = CreateStandardState(m_SharedData);  // transition to main UI
                        App::cur().requestRedraw();
                    } else if (IsHoveringABody()) {
                        // user clicked on a body: assign the mesh to the body
                        auto bodyID = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(m_Hover.ID).ID;

                        m_SharedData.UpdCurrentModelGraph().SetMeshAttachmentPoint(m_MeshID, bodyID);
                        m_SharedData.CommitCurrentModelGraph("assigned mesh to body");
                        m_MaybeNextState = CreateStandardState(m_SharedData);  // transition to main UI
                        App::cur().requestRedraw();
                    }
                }
            }

            m_SharedData.DrawScene(sceneEls);
            DrawConnectionLines();
            DrawHeaderText();
        }

        std::unique_ptr<MWState> onEvent(SDL_Event const&) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();

            if (isRenderHovered) {
                UpdateFromImGuiKeyboardState();
            }

            return std::move(m_MaybeNextState);
        }


        std::unique_ptr<MWState> tick(float) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();

            if (isRenderHovered) {
                UpdatePolarCameraFromImGuiUserInput(m_SharedData.Get3DSceneDims(), m_SharedData.UpdCamera());
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
        // the mesh being assigned by this state
        IDT<MeshEl> m_MeshID;

        // (maybe) next state to transition to
        std::unique_ptr<MWState> m_MaybeNextState = nullptr;

        // (maybe) user mouse hover
        Hover m_Hover;
    };

    Ras CalcAverageTranslationOrientation(ModelGraph const& mg, ID parentID, IDT<BodyEl> childID)
    {
        Ras parentTO = mg.GetTranslationOrientationInGround(parentID);
        Ras childTO = mg.GetTranslationOrientationInGround(childID);
        return AverageRas(parentTO, childTO);
    }

    class FreeJointFirstPivotPlacementState final : public MWState {
    public:
        FreeJointFirstPivotPlacementState(
                SharedData& sharedData,
                IDT<BodyEl> parentID,  // can be ground
                IDT<BodyEl> childBodyID) :

            MWState{sharedData},
            m_MaybeNextState{nullptr},
            m_ParentOrGroundID{parentID},
            m_ChildBodyID{childBodyID},
            m_PivotTranslationOrientation{CalcAverageTranslationOrientation(m_SharedData.GetCurrentModelGraph(), m_ParentOrGroundID, m_ChildBodyID)}
        {
        }

    private:

        glm::vec3 GetParentTranslationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetTranslationInGround(m_ParentOrGroundID);
        }

        glm::vec3 GetParentOrientationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetOrientationInGround(m_ParentOrGroundID);
        }

        glm::vec3 GetChildTranslationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetTranslationInGround(m_ChildBodyID);
        }

        glm::vec3 GetChildOrientationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetOrientationInGround(m_ChildBodyID);
        }

        glm::vec3 const& GetPivotCenter() const
        {
            return m_PivotTranslationOrientation.shift;
        }

        glm::vec3 const& GetPivotOrientation() const
        {
            return m_PivotTranslationOrientation.rot;
        }

        void AddFreeJointToModelGraphFromCurrentState()
        {
            JointAttachment parentAttachment{m_ParentOrGroundID, m_PivotTranslationOrientation};
            JointAttachment childAttachment{m_ChildBodyID, m_PivotTranslationOrientation};

            auto pinjointIdx = *JointRegistry::indexOf(OpenSim::FreeJoint{});

            m_SharedData.UpdCurrentModelGraph().AddJoint(pinjointIdx, "", parentAttachment, childAttachment);
            m_SharedData.CommitCurrentModelGraph("added FreeJoint");
        }

        void TransitionToStandardScreen()
        {
            m_MaybeNextState = CreateStandardState(m_SharedData);
        }

        void UpdateFromImGuiKeyboardState()
        {
            if (ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
                m_MaybeNextState = CreateStandardState(m_SharedData);
                return;
            }

            // R: set manipulation mode to "rotate"
            if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
                if (m_ImGuizmoState.op == ImGuizmo::ROTATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::ROTATE;
            }

            // G: set manipulation mode to "grab" (translate)
            if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
                if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
            }
        }

        void DrawConnectionLines() const
        {
            glm::vec2 childScreenPos = m_SharedData.WorldPosToScreenPos(GetChildTranslationInGround());
            glm::vec2 pivotScreenPos = m_SharedData.WorldPosToScreenPos(m_PivotTranslationOrientation.shift);
            glm::vec2 parentScreenPos = m_SharedData.WorldPosToScreenPos(GetParentTranslationInGround());

            ImU32 blackColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

            m_SharedData.DrawConnectionLine(blackColor, pivotScreenPos, childScreenPos);
            m_SharedData.DrawConnectionLine(blackColor, parentScreenPos, pivotScreenPos);
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose joint location (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetWindowDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void DrawPivot3DManipulators()
        {
            if (!ImGuizmo::IsUsing()) {
                m_ImGuizmoState.mtx = XFormFromRas(m_PivotTranslationOrientation);
            }

            Rect sceneRect = m_SharedData.Get3DSceneRect();
            ImGuizmo::SetRect(sceneRect.p1.x, sceneRect.p1.y, RectDims(sceneRect).x, RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::AllowAxisFlip(false);

            glm::mat4 viewMatrix = m_SharedData.GetCamera().getViewMtx();
            glm::mat4 projMatrix = m_SharedData.GetCamera().getProjMtx(RectAspectRatio(sceneRect));
            glm::mat4 delta;

            bool wasManipulated = ImGuizmo::Manipulate(
                glm::value_ptr(viewMatrix),
                glm::value_ptr(projMatrix),
                m_ImGuizmoState.op,
                m_ImGuizmoState.mode,
                glm::value_ptr(m_ImGuizmoState.mtx),
                glm::value_ptr(delta),
                nullptr,
                nullptr,
                nullptr);

            if (!wasManipulated) {
                return;
            }

            glm::vec3 translation;
            glm::vec3 rotation;
            glm::vec3 scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(delta), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
            rotation = glm::radians(rotation);

            if (m_ImGuizmoState.op == ImGuizmo::ROTATE) {
                m_PivotTranslationOrientation.rot = EulerCompose(m_PivotTranslationOrientation.rot, rotation);
            } else if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                m_PivotTranslationOrientation.shift += translation;
            }
        }

        void Draw3DViewer()
        {
            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            // draw pivot point as a single frame
            m_SharedData.AppendAsFrame(g_PivotID, g_PivotID, m_PivotTranslationOrientation, sceneEls);

            float const faintAlpha = 0.1f;

            // draw other bodies faintly and non-clickable
            for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                m_SharedData.AppendAsFrame(g_EmptyID, g_EmptyID, bodyEl.Xform, sceneEls, faintAlpha);
            }

            // draw meshes faintly and non-clickable
            for (auto const& [meshID, meshEl] : m_SharedData.GetCurrentModelGraph().GetMeshes()) {
                DrawableThing& dt = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                dt.id = g_EmptyID;
                dt.groupId = g_EmptyID;
                dt.color.a = faintAlpha;
            }

            // draw floor
            sceneEls.push_back(m_SharedData.GenerateFloorDrawable());

            m_SharedData.DrawScene(sceneEls);
            DrawPivot3DManipulators();
            DrawConnectionLines();
            DrawHeaderText();
        }

    public:
        std::unique_ptr<MWState> onEvent(SDL_Event const&) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();

            if (isRenderHovered) {
                UpdateFromImGuiKeyboardState();
            }

            return std::move(m_MaybeNextState);
        }


        std::unique_ptr<MWState> tick(float) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (isRenderHovered && !isUsingGizmo) {
                UpdatePolarCameraFromImGuiUserInput(m_SharedData.Get3DSceneDims(), m_SharedData.UpdCamera());
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> draw() override
        {
            ImGuizmo::BeginFrame();

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
        // (maybe) next state that should be transitioned to
        std::unique_ptr<MWState> m_MaybeNextState = nullptr;

        // ID of the parent-side of the FreeJoint (ground or body)
        IDT<BodyEl> m_ParentOrGroundID;

        // ID of the child-side of the FreeJoint (always a body)
        IDT<BodyEl> m_ChildBodyID;

        // translation+orientation of the FreeJoint's joint location in ground
        Ras m_PivotTranslationOrientation;

        // ImGuizmo state for 3D manipulators
        struct {
            bool wasUsingLastFrame = false;
            glm::mat4 mtx{};
            ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
            ImGuizmo::MODE mode = ImGuizmo::WORLD;
        } m_ImGuizmoState;
    };

    class PinJointPivotPlacementState final : public MWState {
    public:
        PinJointPivotPlacementState(SharedData& sharedData, IDT<BodyEl> parentID, IDT<BodyEl> childBodyID) :
            MWState{sharedData},
            m_MaybeNextState{nullptr},
            m_ParentOrGroundID{parentID},
            m_ChildBodyID{childBodyID},
            m_PivotTranslationOrientation{CalcAverageTranslationOrientation(sharedData.GetCurrentModelGraph(), parentID, childBodyID)}
        {
        }

    private:

        glm::vec3 GetParentTranslationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetTranslationInGround(m_ParentOrGroundID);
        }

        glm::vec3 GetParentOrientationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetOrientationInGround(m_ParentOrGroundID);
        }

        glm::vec3 GetChildTranslationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetTranslationInGround(m_ChildBodyID);
        }

        glm::vec3 GetChildOrientationInGround() const
        {
            return m_SharedData.GetCurrentModelGraph().GetOrientationInGround(m_ChildBodyID);
        }

        glm::vec3 const& GetPivotCenter() const
        {
            return m_PivotTranslationOrientation.shift;
        }

        glm::vec3 const& GetPivotOrientation() const
        {
            return m_PivotTranslationOrientation.rot;
        }

        void CreatePinjointFromCurrentState()
        {
            JointAttachment parentAttachment{m_ParentOrGroundID, m_PivotTranslationOrientation};
            JointAttachment childAttachment{m_ChildBodyID, m_PivotTranslationOrientation};

            auto pinjointIdx = *JointRegistry::indexOf(OpenSim::PinJoint{});

            m_SharedData.UpdCurrentModelGraph().AddJoint(pinjointIdx, "pinjoint", parentAttachment, childAttachment);
            m_SharedData.CommitCurrentModelGraph("added pin joint");
        }

        void TransitionToStandardScreen()
        {
            m_MaybeNextState = CreateStandardState(m_SharedData);
        }

        void UpdateFromImGuiKeyboardState()
        {
            if (ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
                m_MaybeNextState = CreateStandardState(m_SharedData);
                App::cur().requestRedraw();
            }

            // R: set manipulation mode to "rotate"
            if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
                if (m_ImGuizmoState.op == ImGuizmo::ROTATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::ROTATE;
            }

            // G: set manipulation mode to "grab" (translate)
            if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
                if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
            }
        }

        void DrawConnectionLines() const
        {
            glm::vec2 childScreenPos = m_SharedData.WorldPosToScreenPos(GetChildTranslationInGround());
            glm::vec2 pivotScreenPos = m_SharedData.WorldPosToScreenPos(m_PivotTranslationOrientation.shift);
            glm::vec2 parentScreenPos = m_SharedData.WorldPosToScreenPos(GetParentTranslationInGround());

            ImU32 blackColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

            m_SharedData.DrawConnectionLine(blackColor, pivotScreenPos, childScreenPos);
            m_SharedData.DrawConnectionLine(blackColor, parentScreenPos, pivotScreenPos);
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose pivot location + orientation (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetWindowDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void DrawPivot3DManipulators()
        {
            if (!ImGuizmo::IsUsing()) {
                m_ImGuizmoState.mtx = XFormFromRas(m_PivotTranslationOrientation);
            }

            Rect sceneRect = m_SharedData.Get3DSceneRect();
            ImGuizmo::SetRect(sceneRect.p1.x, sceneRect.p1.y, RectDims(sceneRect).x, RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::AllowAxisFlip(false);

            glm::mat4 viewMatrix = m_SharedData.GetCamera().getViewMtx();
            glm::mat4 projMatrix = m_SharedData.GetCamera().getProjMtx(RectAspectRatio(sceneRect));
            glm::mat4 delta;

            bool wasManipulated = ImGuizmo::Manipulate(
                glm::value_ptr(viewMatrix),
                glm::value_ptr(projMatrix),
                m_ImGuizmoState.op,
                m_ImGuizmoState.mode,
                glm::value_ptr(m_ImGuizmoState.mtx),
                glm::value_ptr(delta),
                nullptr,
                nullptr,
                nullptr);

            if (!wasManipulated) {
                return;
            }

            glm::vec3 translation;
            glm::vec3 rotation;
            glm::vec3 scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(delta), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
            rotation = glm::radians(rotation);

            if (m_ImGuizmoState.op == ImGuizmo::ROTATE) {
                m_PivotTranslationOrientation.rot = EulerCompose(m_PivotTranslationOrientation.rot, rotation);
            } else if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                m_PivotTranslationOrientation.shift += translation;
            }
        }

        float GetAngleBetweenParentAndChild() const
        {
            glm::vec3 pivot = m_PivotTranslationOrientation.shift;
            glm::vec3 pivot2parent = GetParentTranslationInGround() - pivot;
            glm::vec3 pivot2child = GetChildTranslationInGround() - pivot;
            glm::vec3 pivot2parentDir = glm::normalize(pivot2parent);
            glm::vec3 pivot2childDir = glm::normalize(pivot2child);
            float cosTheta = glm::dot(pivot2parentDir, pivot2childDir);
            return glm::acos(cosTheta);
        }

        void Draw3DViewer()
        {
            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            // draw pivot point as a single frame
            m_SharedData.AppendAsFrame(g_PivotID, g_PivotID, m_PivotTranslationOrientation, sceneEls);

            float const faintAlpha = 0.1f;

            // draw other bodies faintly and non-clickable
            for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                m_SharedData.AppendAsFrame(g_EmptyID, g_EmptyID, bodyEl.Xform, sceneEls, faintAlpha);
            }

            // draw meshes faintly and non-clickable
            for (auto const& [meshID, meshEl] : m_SharedData.GetCurrentModelGraph().GetMeshes()) {
                DrawableThing& dt = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                dt.id = g_EmptyID;
                dt.groupId = g_EmptyID;
                dt.color.a = faintAlpha;
            }

            // draw floor
            sceneEls.push_back(m_SharedData.GenerateFloorDrawable());

            m_SharedData.DrawScene(sceneEls);
            DrawPivot3DManipulators();
            DrawConnectionLines();
            DrawHeaderText();
        }

        void DrawPlacementTools()
        {
            {
                float sf = m_SharedData.GetSceneScaleFactor();
                if (ImGui::InputFloat("scene scale factor", &sf)) {
                    m_SharedData.SetSceneScaleFactor(sf);
                }
            }

            ImGui::Dummy({0.0f, 2.0f});
            ImGui::Text("joint stats:");
            ImGui::Dummy({0.0f, 5.0f});

            {
                float l = glm::length(GetPivotCenter()-GetParentTranslationInGround());
                ImGui::Text("parent to pivot length: %f", l);
            }

            {
                float l = glm::length(GetPivotCenter()-GetChildTranslationInGround());
                ImGui::Text("child to pivot length: %f", l);
            }

            {
                float l = glm::length(GetParentTranslationInGround()-GetChildTranslationInGround());
                ImGui::Text("child to parent length: %f", l);
            }

            {
                float ang = GetAngleBetweenParentAndChild();
                ImGui::Text("pivot angle: %f", glm::degrees(ang));
            }


            ImGui::Dummy({0.0f, 2.0f});
            ImGui::Text("orientation tools:");
            ImGui::Dummy({0.0f, 5.0f});

            {
                glm::vec3 v = glm::degrees(m_PivotTranslationOrientation.rot);
                if (ImGui::InputFloat3("orientation in ground", glm::value_ptr(v))) {
                    m_PivotTranslationOrientation.rot = glm::radians(v);
                }
            }

            if (ImGui::Button("Auto-orient with X pointing towards parent")) {
                glm::vec3 parentPos = GetParentTranslationInGround();
                glm::vec3 childPos = GetChildTranslationInGround();
                glm::vec3 pivotPos = GetPivotCenter();
                glm::vec3 pivot2parent = parentPos - pivotPos;
                glm::vec3 pivot2child = childPos - pivotPos;
                glm::vec3 pivot2parentDir = glm::normalize(pivot2parent);
                glm::vec3 pivot2childDir = glm::normalize(pivot2child);

                glm::vec3 xAxis = pivot2parentDir;  // user requested this
                glm::vec3 zAxis = glm::normalize(glm::cross(pivot2parentDir, pivot2childDir));  // from the triangle
                glm::vec3 yAxis = glm::normalize(glm::cross(zAxis, xAxis));

                glm::mat3 m{xAxis, yAxis, zAxis};
                m_PivotTranslationOrientation.rot = MatToEulerAngles(m);
            }

            if (ImGui::Button("Auto-orient with X pointing towards child")) {
                glm::vec3 parentPos = GetParentTranslationInGround();
                glm::vec3 childPos = GetChildTranslationInGround();
                glm::vec3 pivotPos = GetPivotCenter();
                glm::vec3 pivot2parent = parentPos - pivotPos;
                glm::vec3 pivot2child = childPos - pivotPos;
                glm::vec3 pivot2parentDir = glm::normalize(pivot2parent);
                glm::vec3 pivot2childDir = glm::normalize(pivot2child);

                glm::vec3 xAxis = pivot2childDir;  // user requested this
                glm::vec3 zAxis = glm::normalize(glm::cross(pivot2parentDir, pivot2childDir));  // from the triangle
                glm::vec3 yAxis = glm::normalize(glm::cross(zAxis, xAxis));

                glm::mat3 m{xAxis, yAxis, zAxis};
                m_PivotTranslationOrientation.rot = MatToEulerAngles(m);
            }

            if (ImGui::Button("Reverse Z by rotating X")) {
                m_PivotTranslationOrientation.rot = EulerCompose({fpi, 0.0f, 0.0f}, m_PivotTranslationOrientation.rot);
            }

            if (ImGui::Button("Reverse Z by rotating Y")) {
                m_PivotTranslationOrientation.rot = EulerCompose({0.0f, fpi, 0.0f}, m_PivotTranslationOrientation.rot);
            }

            if (ImGui::Button("Use parent's orientation")) {
                m_PivotTranslationOrientation.rot = GetParentOrientationInGround();
            }

            if (ImGui::Button("Use child's orientation")) {
                m_PivotTranslationOrientation.rot = GetChildOrientationInGround();
            }

            if (ImGui::Button("Use parent's and child's orientation (average)")) {
                glm::vec3 parentOrientation = GetParentOrientationInGround();
                glm::vec3 childOrientation = GetChildOrientationInGround();
                m_PivotTranslationOrientation.rot = (parentOrientation+childOrientation)/2.0f;
            }

            if (ImGui::Button("Orient along global axes")) {
                m_PivotTranslationOrientation.rot = {};
            }

            ImGui::Dummy({0.0f, 10.0f});
            ImGui::Text("translation tools:");
            ImGui::Dummy({0.0f, 5.0f});

            ImGui::InputFloat3("translation in ground", glm::value_ptr(m_PivotTranslationOrientation.shift));

            if (ImGui::Button("Use parent's translation")) {
                m_PivotTranslationOrientation.shift = GetParentTranslationInGround();
            }

            if (ImGui::Button("Use child's translation")) {
                m_PivotTranslationOrientation.shift = GetChildTranslationInGround();
            }

            if (ImGui::Button("Use midpoint translation ((parent+child)/2)")) {
                glm::vec3 parentTranslation = GetParentTranslationInGround();
                glm::vec3 childTranslation = GetChildTranslationInGround();
                m_PivotTranslationOrientation.shift = (parentTranslation+childTranslation)/2.0f;
            }

            ImGui::Dummy({0.0f, 10.0f});
            ImGui::Text("finishing:");
            ImGui::Dummy({0.0f, 5.0f});

            if (ImGui::Button("finish (add the joint)")) {
                CreatePinjointFromCurrentState();
                TransitionToStandardScreen();
            }
        }

    public:
        std::unique_ptr<MWState> onEvent(SDL_Event const&) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();

            if (isRenderHovered) {
                UpdateFromImGuiKeyboardState();
            }

            return std::move(m_MaybeNextState);
        }


        std::unique_ptr<MWState> tick(float) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (isRenderHovered && !isUsingGizmo) {
                UpdatePolarCameraFromImGuiUserInput(m_SharedData.Get3DSceneDims(), m_SharedData.UpdCamera());
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> draw() override
        {
            ImGuizmo::BeginFrame();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("wizardsstep2viewer")) {
                ImGui::PopStyleVar();
                Draw3DViewer();
            } else {
                ImGui::PopStyleVar();
            }
            ImGui::End();

            if (ImGui::Begin("placement tools")) {
                DrawPlacementTools();
            }
            ImGui::End();

            return std::move(m_MaybeNextState);
        }

    private:
        // (maybe) next state that should be transitioned to
        std::unique_ptr<MWState> m_MaybeNextState;

        // ID of the parent-side of the joint (ground/body)
        IDT<BodyEl> m_ParentOrGroundID;

        // ID of the child-side of the joint (always a body)
        IDT<BodyEl> m_ChildBodyID;

        // translation+orientation of the pinjoint's pivot
        Ras m_PivotTranslationOrientation;

        // ImGuizmo state for 3D manipulators
        struct {
            bool wasUsingLastFrame = false;
            glm::mat4 mtx{};
            ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
            ImGuizmo::MODE mode = ImGuizmo::WORLD;
        } m_ImGuizmoState;
    };

    // joint assignment: step 1: choose body to join to
    //
    // this state is entered when the user clicks "join to" in the UI, indicating
    // that they want to create a joint in the scene
    class JointAssignmentStep1State final : public MWState {
    public:
        JointAssignmentStep1State(SharedData& sharedData, IDT<BodyEl> childBodyID) :
            MWState{sharedData},
            m_ChildBodyID{childBodyID}
        {
        }

    private:

        bool IsHoveringAnotherBody() const
        {
            return m_Hover && m_Hover.ID != g_GroundID && m_Hover.ID != m_ChildBodyID;
        }

        BodyEl const& GetChildBodyEl() const
        {
            return m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(m_ChildBodyID);
        }

        void UpdateFromImGuiKeyboardState()
        {
            if (ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
                // ESC: transition back to main UI
                m_MaybeNextState = CreateStandardState(m_SharedData);
            }
        }

        void DrawChildHoverTooltip() const
        {
            if (m_Hover.ID != m_ChildBodyID) {
                return;
            }

            BodyEl const& body = GetChildBodyEl();

            ImGui::BeginTooltip();
            ImGui::Text("%s", body.Name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(you cannot join bodies to themselves)");
            ImGui::EndTooltip();
        }

        void DrawGroundHoverTooltip() const
        {
            if (m_Hover.ID != g_GroundID) {
                return;
            }

            ImGui::BeginTooltip();
            ImGui::Text("ground");
            ImGui::SameLine();
            ImGui::TextDisabled("(click to join the body to ground)");
            ImGui::EndTooltip();
        }

        void DrawOtherBodyTooltip() const
        {
            if (!IsHoveringAnotherBody()) {
                return;
            }

            BodyEl const& otherBody = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(m_Hover.ID);

            ImGui::BeginTooltip();
            ImGui::Text("%s", otherBody.Name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(click to join)");
            ImGui::EndTooltip();
        }

        void DrawHoverTooltip() const
        {
            if (!m_Hover) {
                return;
            } else if (m_Hover.ID == m_ChildBodyID) {
                DrawChildHoverTooltip();
            } else if (m_Hover.ID == g_GroundID) {
                DrawGroundHoverTooltip();
            } else if (IsHoveringAnotherBody()) {
                DrawOtherBodyTooltip();
            }
        }

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            if (!m_SharedData.IsRenderHovered()) {
                m_Hover = Hover{};
                return;
            }

            m_Hover = m_SharedData.Hovertest(drawables);
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose a body (or ground) to join to (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetWindowDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void DrawConnectionLines() const
        {
            ImVec4 faintColor = {0.0f, 0.0f, 0.0f, 0.2f};
            ImVec4 strongColor = {0.0f, 0.0f, 0.0f, 1.0f};
            auto strongColorU32 = ImGui::ColorConvertFloat4ToU32(strongColor);

            if (!m_Hover) {
                // draw all existing connection lines faintly
                m_SharedData.DrawConnectionLines(faintColor);
                return;
            }

            // otherwise, draw all *other* connection lines faintly and then handle
            // the hovertest line drawing
            m_SharedData.DrawConnectionLines(faintColor, m_ChildBodyID);

            glm::vec3 const& childBodyLoc = GetChildBodyEl().Xform.shift;

            if (IsHoveringAnotherBody()) {
                glm::vec3 const& otherBodyLoc = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(m_Hover.ID).Xform.shift;
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(otherBodyLoc),
                                                m_SharedData.WorldPosToScreenPos(childBodyLoc));
            } else if (m_Hover.ID == g_GroundID) {
                glm::vec3 groundLoc = {0.0f, 0.0f, 0.0f};
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(groundLoc),
                                                m_SharedData.WorldPosToScreenPos(childBodyLoc));
            }
        }

        void HandleHovertestSideEffects()
        {
            if (!m_Hover) {
                return;
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // user has clicked what they want, so present them with the joint type
                // popup modal

                m_MaybeUserParentChoice = m_Hover.ID;
                ImGui::OpenPopup(m_JointTypePopupName);
                App::cur().requestRedraw();
            }
        }

        void DrawJointTypeSelectionPopupIfUserHasSelectedSomething()
        {
            if (m_MaybeUserParentChoice == g_EmptyID) {
                return;
            }

            // otherwise: try to draw the popup (should be opened by this point)

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(512, 0));
            if (!ImGui::BeginPopupModal(m_JointTypePopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                return;  // not showing modal popup
            }

            if (ImGui::Button("PinJoint")) {
                m_MaybeNextState = std::make_unique<PinJointPivotPlacementState>(m_SharedData, DowncastID<BodyEl>(m_MaybeUserParentChoice), m_ChildBodyID);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("FreeJoint")) {
                m_MaybeNextState = std::make_unique<FreeJointFirstPivotPlacementState>(m_SharedData, DowncastID<BodyEl>(m_MaybeUserParentChoice), m_ChildBodyID);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        void Draw3DViewer()
        {
            // ensure render is sized correctly
            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            // draw bodies as spheres the user can click
            for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                DrawableThing& dt = sceneEls.emplace_back(m_SharedData.GenerateBodyElSphere(bodyEl, {1.0f, 0.0f, 0.0f, 1.0f}));
                if (bodyID == m_ChildBodyID) {
                    dt.color.a = 0.1f;  // can't self-assign, so fade it a bit
                }
            }

            // draw meshes faintly and non-clickable
            for (auto const& [meshID, meshEl] : m_SharedData.GetCurrentModelGraph().GetMeshes()) {
                DrawableThing& dt = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, {1.0f, 1.0f, 1.0f, 0.1f}));
                dt.id = g_EmptyID;
                dt.groupId = g_EmptyID;
            }

            // draw ground as clickable sphere
            sceneEls.push_back(m_SharedData.GenerateGroundSphere({0.0f, 0.0f, 0.0f, 1.0f}));

            // draw floor normally
            sceneEls.push_back(m_SharedData.GenerateFloorDrawable());

            // hovertest generated geometry
            DoHovertest(sceneEls);

            // handle any hovertest side effects (state transitions, etc.)
            HandleHovertestSideEffects();

            // draw everything
            m_SharedData.DrawScene(sceneEls);
            DrawHoverTooltip();
            DrawConnectionLines();
            DrawHeaderText();
            DrawJointTypeSelectionPopupIfUserHasSelectedSomething();
        }

    public:
        std::unique_ptr<MWState> onEvent(SDL_Event const&) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();

            if (isRenderHovered) {
                UpdateFromImGuiKeyboardState();
            }

            return std::move(m_MaybeNextState);
        }


        std::unique_ptr<MWState> tick(float) override
        {
            bool isRenderHovered = m_SharedData.IsRenderHovered();

            if (isRenderHovered) {
                UpdatePolarCameraFromImGuiUserInput(m_SharedData.Get3DSceneDims(), m_SharedData.UpdCamera());
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
        // the body that the user clicked "join to" on
        IDT<BodyEl> m_ChildBodyID;

        // (maybe) next state to transition to
        std::unique_ptr<MWState> m_MaybeNextState = nullptr;

        // (maybe) hover + worldspace location of the user's mouse hover
        Hover m_Hover;

        // (maybe) the body/ground the user selected
        ID m_MaybeUserParentChoice = g_EmptyID;

        // name of the ImGui popup that lets the user select a joint type
        char const* m_JointTypePopupName = "select joint type";
    };


    // "standard" UI state
    //
    // this is what the user is typically interacting with when the UI loads
    class StandardMWState final : public MWState {
    public:

        StandardMWState(SharedData& sharedData) : MWState{sharedData}
        {
        }

        bool IsHovered(ID id) const
        {
            return m_Hover.ID == id;
        }

        void SelectHover()
        {
            if (!m_Hover) {
                return;
            }

            m_SharedData.Select(m_Hover.ID);
        }

        float RimIntensity(ID id)
        {
            if (id == g_EmptyID) {
                return 0.0f;
            } else if (m_SharedData.IsSelected(id)) {
                return 0.9f;
            } else if (IsHovered(id)) {
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

            m_SharedData.AddBody(m_Hover.Pos);
        }

        void TransitionToAssigningMeshNextFrame(MeshEl const& meshEl)
        {
            // request a state transition
            m_MaybeNextState = std::make_unique<AssignMeshMWState>(m_SharedData, meshEl.ID);
        }

        void TryTransitionToAssigningHoveredMeshNextFrame()
        {
            if (!m_Hover) {
                return;
            }

            MeshEl const* maybeMesh = m_SharedData.UpdCurrentModelGraph().TryGetMeshElByID(m_Hover.ID);

            if (!maybeMesh) {
                return;  // not hovering a mesh
            }

            TransitionToAssigningMeshNextFrame(*maybeMesh);
        }

        void OpenHoverContextMenu()
        {
            if (!m_Hover) {
                return;
            }

            m_MaybeOpenedContextMenu = m_Hover;
            ImGui::OpenPopup(m_ContextMenuName);
            App::cur().requestRedraw();
        }

        void UpdateFromImGuiKeyboardState()
        {
            bool superPressed = ImGui::IsKeyDown(SDL_SCANCODE_LGUI) || ImGui::IsKeyDown(SDL_SCANCODE_RGUI);
            bool ctrlPressed = ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL);
            bool shiftPressed = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            bool ctrlOrSuperPressed =  superPressed || ctrlPressed;

            if (ctrlOrSuperPressed && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                // Ctrl+A: select all
                m_SharedData.SelectAll();
            } else if (ctrlOrSuperPressed && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                // Ctrl+Z: undo
                m_SharedData.UndoCurrentModelGraph();
            } else if (ctrlOrSuperPressed && shiftPressed && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                // Ctrl+Shift+Z: redo
                m_SharedData.RedoCurrentModelGraph();
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE) || ImGui::IsKeyPressed(SDL_SCANCODE_BACKSPACE)) {
                // DELETE/BACKSPACE: delete any selected elements
                m_SharedData.DeleteSelected();
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
                // B: add body to hovered element
                AddBodyToHoveredElement();
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                // A: assign a parent for the hovered element
                TryTransitionToAssigningHoveredMeshNextFrame();
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
                // S: set manipulation mode to "scale"
                if (m_ImGuizmoState.op == ImGuizmo::SCALE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::SCALE;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
                // R: set manipulation mode to "rotate"
                if (m_ImGuizmoState.op == ImGuizmo::ROTATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::ROTATE;
            } else if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
                // G: set manipulation mode to "grab" (translate)
                if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
            }
        }

        void DrawBodyContextMenu(BodyEl const& bodyEl)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                if (ImGui::MenuItem("focus camera on this")) {
                    m_SharedData.FocusCameraOn(bodyEl.Xform.shift);
                }

                if (ImGui::MenuItem("join to")) {
                    m_MaybeNextState = std::make_unique<JointAssignmentStep1State>(m_SharedData, bodyEl.ID);
                }

                if (ImGui::MenuItem("attach mesh to this")) {
                    IDT<BodyEl> bodyID = bodyEl.ID;
                    for (auto const& meshFile : m_SharedData.PromptUserForMeshFiles()) {
                        m_SharedData.PushMeshLoadRequest(meshFile, bodyID);
                    }
                }

                if (ImGui::MenuItem("delete")) {
                    std::string name = bodyEl.Name;
                    m_SharedData.UpdCurrentModelGraph().DeleteBodyElByID(bodyEl.ID);
                    m_Hover = Hover{};
                    m_MaybeOpenedContextMenu = Hover{};
                    m_SharedData.CommitCurrentModelGraph("deleted " + name);
                }

                // draw name editor
                {
                    char buf[256];
                    std::strcpy(buf, bodyEl.Name.c_str());
                    if (ImGui::InputText("name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        m_SharedData.UpdCurrentModelGraph().SetBodyName(bodyEl.ID, buf);
                        m_SharedData.CommitCurrentModelGraph("changed body name");
                    }
                }

                // pos editor
                {
                    glm::vec3 translation = bodyEl.Xform.shift;
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        Ras to = bodyEl.Xform;
                        to.shift = translation;
                        m_SharedData.UpdCurrentModelGraph().SetBodyXform(bodyEl.ID, to);
                        m_SharedData.CommitCurrentModelGraph("changed body translation");
                    }
                }

                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(bodyEl.Xform.rot);
                    if (ImGui::InputFloat3("orientation", glm::value_ptr(orientationDegrees), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        Ras to = bodyEl.Xform;
                        to.rot = glm::radians(orientationDegrees);
                        m_SharedData.UpdCurrentModelGraph().SetBodyXform(bodyEl.ID, to);
                        m_SharedData.CommitCurrentModelGraph("changed body orientation");
                    }
                }
                ImGui::EndPopup();
            }
        }

        void DrawMeshContextMenu(MeshEl const& meshEl, glm::vec3 const& clickPos)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {

                if (ImGui::MenuItem("add body at click location")) {
                    m_SharedData.AddBody(clickPos);
                }

                if (ImGui::MenuItem("add body at mesh origin")) {
                    m_SharedData.AddBody(meshEl.Xform.shift);
                }

                if (ImGui::MenuItem("add body at mesh bounds center")) {
                    m_SharedData.AddBody(AABBCenter(GetGroundspaceBounds(*meshEl.MeshData, GetModelMatrix(meshEl))));
                }

                if (ImGui::MenuItem("focus camera on this")) {
                    m_SharedData.FocusCameraOn(meshEl.Xform.shift);
                }

                if (ImGui::MenuItem("assign to body")) {
                    TransitionToAssigningMeshNextFrame(meshEl);
                }

                if (ImGui::MenuItem("delete")) {
                    std::string name = meshEl.Name;
                    m_SharedData.UpdCurrentModelGraph().DeleteMeshElByID(meshEl.ID);
                    m_SharedData.CommitCurrentModelGraph("deleted " + name);
                }

                ImGui::Dummy({0.0f, 5.0f});
                ImGui::Separator();
                ImGui::Dummy({0.0f, 5.0f});

                // draw name editor
                {
                    char buf[256];
                    std::strcpy(buf, meshEl.Name.c_str());
                    if (ImGui::InputText("name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        m_SharedData.UpdCurrentModelGraph().SetMeshName(meshEl.ID, buf);
                        m_SharedData.CommitCurrentModelGraph("changed mesh name");
                    }
                }

                // pos editor
                {
                    glm::vec3 translation = meshEl.Xform.shift;
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        Ras to = meshEl.Xform;
                        to.shift = translation;
                        m_SharedData.UpdCurrentModelGraph().SetMeshXform(meshEl.ID, to);
                        m_SharedData.CommitCurrentModelGraph("changed mesh translation");
                    }
                }

                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(meshEl.Xform.rot);
                    if (ImGui::InputFloat3("orientation", glm::value_ptr(orientationDegrees), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        Ras to = meshEl.Xform;
                        to.rot = glm::radians(orientationDegrees);
                        m_SharedData.UpdCurrentModelGraph().SetMeshXform(meshEl.ID, to);
                        m_SharedData.CommitCurrentModelGraph("changed mesh orientation");
                    }
                }

                // scale factor editor
                {
                    glm::vec3 scaleFactors = meshEl.ScaleFactors;
                    if (ImGui::InputFloat3("scale factors", glm::value_ptr(scaleFactors), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        m_SharedData.UpdCurrentModelGraph().SetMeshScaleFactors(meshEl.ID, scaleFactors);
                        m_SharedData.CommitCurrentModelGraph("changed mesh scale factors");
                    }
                }
                ImGui::EndPopup();
            }
        }

        void DrawJointContextMenu(JointEl const& jointEl)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                if (ImGui::MenuItem("do joint stuff")) {
                }

                ImGui::EndPopup();
            }
        }

        void DrawContextMenu()
        {
            if (!m_MaybeOpenedContextMenu) {
                return;
            }

            ModelGraph const& mg = m_SharedData.GetCurrentModelGraph();

            if (MeshEl const* meshEl = mg.TryGetMeshElByID(m_MaybeOpenedContextMenu.ID)) {
                DrawMeshContextMenu(*meshEl, m_MaybeOpenedContextMenu.Pos);
            } else if (BodyEl const* bodyEl = mg.TryGetBodyElByID(m_MaybeOpenedContextMenu.ID)) {
                DrawBodyContextMenu(*bodyEl);
            } else if (JointEl const* jointEl = mg.TryGetJointElByID(m_MaybeOpenedContextMenu.ID)) {
                DrawJointContextMenu(*jointEl);
            }
        }

        void DrawHistory()
        {
            auto const& snapshots = m_SharedData.GetModelGraphSnapshots();
            size_t currentSnapshot = m_SharedData.GetModelGraphIsBasedOn();
            for (size_t i = 0; i < snapshots.size(); ++i) {
                ModelGraphSnapshot const& snapshot = snapshots[i];

                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(snapshot.GetCommitMessage().c_str(), i == currentSnapshot)) {
                    m_SharedData.UseModelGraphSnapshot(i);
                }
                ImGui::PopID();
            }
        }

        void DrawSidebar()
        {
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
            if (ImGui::Button(ICON_FA_PLUS "Import Meshes")) {
                m_SharedData.PromptUserForMeshFilesAndPushThemOntoMeshLoader();
            }
            if (ImGui::Button(ICON_FA_PLUS "Add Body")) {
                m_SharedData.AddBody({0.0f, 0.0f, 0.0f});
            }
            ImGui::PopStyleColor();

            if (ImGui::Button(ICON_FA_ARROW_RIGHT " Convert to OpenSim model")) {
                m_SharedData.TryCreateOutputModel();
            }
        }

        char const* BodyOrGroundString(IDT<BodyEl> id) const
        {
            return id == g_GroundID ? "ground" : m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(id).Name.c_str();
        }

        void DrawMeshHoverTooltip(MeshEl const& meshEl) const
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", meshEl.Name.c_str());
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
            ImGui::Text("%s", GetJointLabel(jointEl).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(%s, %s --> %s)", GetJointTypeName(jointEl).c_str(), BodyOrGroundString(jointEl.Parent.BodyID), BodyOrGroundString(jointEl.Child.BodyID));
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

            ModelGraph const& mg = m_SharedData.GetCurrentModelGraph();

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
            if (!m_SharedData.HasSelection()) {
                return;  // can only manipulate selected stuff
            }

            // if the user isn't manipulating anything, create an up-to-date
            // manipulation matrix
            //
            // this is so that ImGuizmo can *show* the manipulation axes, and
            // because the user might start manipulating during this frame
            if (!ImGuizmo::IsUsing()) {

                auto it = m_SharedData.GetCurrentSelection().begin();
                auto end = m_SharedData.GetCurrentSelection().end();

                if (it == end) {
                    return;
                }

                ModelGraph const& mg = m_SharedData.GetCurrentModelGraph();

                int n = 0;

                glm::vec3 translation = mg.GetTranslationInGround(*it);
                glm::vec3 orientation = mg.GetOrientationInGround(*it);
                ++it;
                ++n;

                while (it != end) {
                    translation += mg.GetTranslationInGround(*it);  // TODO: numerically unstable
                    orientation += mg.GetOrientationInGround(*it);  // TODO: numerically unstable
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

            Rect sceneRect = m_SharedData.Get3DSceneRect();

            ImGuizmo::SetRect(
                sceneRect.p1.x,
                sceneRect.p1.y,
                RectDims(sceneRect).x,
                RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::AllowAxisFlip(false);

            glm::mat4 delta;
            bool manipulated = ImGuizmo::Manipulate(
                glm::value_ptr(m_SharedData.GetCamera().getViewMtx()),
                glm::value_ptr(m_SharedData.GetCamera().getProjMtx(RectAspectRatio(sceneRect))),
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
                m_SharedData.CommitCurrentModelGraph("manipulated selection");
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

            for (ID id : m_SharedData.GetCurrentSelection()) {
                switch (m_ImGuizmoState.op) {
                case ImGuizmo::SCALE:
                    m_SharedData.UpdCurrentModelGraph().ApplyScale(id, scale);
                    break;
                case ImGuizmo::ROTATE:
                    m_SharedData.UpdCurrentModelGraph().ApplyRotation(id, rotation);
                    break;
                case ImGuizmo::TRANSLATE:
                    m_SharedData.UpdCurrentModelGraph().ApplyTranslation(id, translation);
                    break;
                default:
                    break;
                }
            }
        }

        bool MouseWasReleasedWithoutDragging(ImGuiMouseButton btn) const
        {
            using osc::operator<<;

            if (!ImGui::IsMouseReleased(btn)) {
                return false;
            }

            glm::vec2 dragDelta = ImGui::GetMouseDragDelta(btn);
            return glm::length(dragDelta) < 5.0f;
        }

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            if (!m_SharedData.IsRenderHovered() || ImGuizmo::IsUsing()) {
                m_Hover = Hover{};
                return;
            }

            m_Hover = m_SharedData.Hovertest(drawables);

            bool lcClicked = MouseWasReleasedWithoutDragging(ImGuiMouseButton_Left);
            bool rcClicked = MouseWasReleasedWithoutDragging(ImGuiMouseButton_Right);
            bool shiftDown = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (!m_Hover && lcClicked && !isUsingGizmo && !shiftDown) {
                m_SharedData.DeSelectAll();
                return;
            }

            if (m_Hover && lcClicked && !isUsingGizmo) {
                if (!shiftDown) {
                    m_SharedData.DeSelectAll();
                }
                SelectHover();
                return;
            }

            if (m_Hover && rcClicked && !isUsingGizmo) {
                OpenHoverContextMenu();
            }

            if (m_Hover && !ImGui::IsPopupOpen(m_ContextMenuName)) {
                DrawHoverTooltip();
            }
        }

        void Draw3DViewer()
        {
            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            if (m_SharedData.IsShowingMeshes()) {
                for (auto const& [meshID, meshEl] : m_SharedData.GetCurrentModelGraph().GetMeshes()) {
                    sceneEls.push_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                }
            }

            if (m_SharedData.IsShowingBodies()) {
                for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                    m_SharedData.AppendBodyElAsFrame(bodyEl, sceneEls);
                }
            }

            if (m_SharedData.IsShowingGround()) {
                sceneEls.push_back(m_SharedData.GenerateGroundSphere(m_SharedData.GetGroundColor()));
            }

            for (auto const& [jointID, jointEl] : m_SharedData.GetCurrentModelGraph().GetJoints()) {
                // first pivot
                m_SharedData.AppendAsFrame(jointID, g_EmptyID, jointEl.Parent.Xform, sceneEls);
                m_SharedData.AppendAsFrame(jointID, g_EmptyID, jointEl.Child.Xform, sceneEls);
            }

            if (m_SharedData.IsShowingFloor()) {
                sceneEls.push_back(m_SharedData.GenerateFloorDrawable());
            }

            // hovertest the generated geometry
            DoHovertest(sceneEls);

            // assign rim highlights based on hover
            for (DrawableThing& dt : sceneEls) {
                dt.rimColor = RimIntensity(dt.id);
            }

            // draw 3D scene (effectively, as an ImGui::Image)
            m_SharedData.DrawScene(sceneEls);

            // draw overlays/gizmos
            DrawSelection3DManipulators();
            DrawContextMenu();
            m_SharedData.DrawConnectionLines();
        }

        std::unique_ptr<MWState> onEvent(SDL_Event const&) override
        {
            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> tick(float) override
        {
            UpdateFromImGuiKeyboardState();

            bool isRenderHovered = m_SharedData.IsRenderHovered();
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (isRenderHovered && !isUsingGizmo) {
                UpdatePolarCameraFromImGuiUserInput(m_SharedData.Get3DSceneDims(), m_SharedData.UpdCamera());
            }

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<MWState> draw() override
        {
            ImGuizmo::BeginFrame();

            if (ImGui::Begin("wizardstep2sidebar")) {
                DrawSidebar();
            }
            ImGui::End();

            if (ImGui::Begin("history")) {
                DrawHistory();
            }
            ImGui::End();

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
        // (maybe) hover + worldspace location of the hover
        Hover m_Hover;

        // (maybe) the scene element the user opened a context menu for
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

    std::unique_ptr<MWState> CreateStandardState(SharedData& sd)
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
        m_SharedData{},
        m_CurrentState{std::make_unique<StandardMWState>(m_SharedData)}
    {
    }

    Impl(std::vector<std::filesystem::path> meshPaths) :
        m_SharedData{std::move(meshPaths)},
        m_CurrentState{std::make_unique<StandardMWState>(m_SharedData)}
    {
    }

    void LoadPaths(std::vector<std::filesystem::path> meshPaths)
    {
        for (std::filesystem::path const& p : meshPaths) {
            m_SharedData.PushMeshLoadRequest(p);
        }
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

    void onEvent(SDL_Event const& e)
    {
        if (osc::ImGuiOnEvent(e)) {
            m_ShouldRequestRedraw = true;
            return;
        }

        // pump event into top-level shared state for state-independent handling
        if (m_SharedData.onEvent(e)) {
            return;
        }

        // pump the event into the currently-active state and handle any state transitions
        auto maybeNewState = m_CurrentState->onEvent(e);
        if (maybeNewState) {
            m_CurrentState = std::move(maybeNewState);
        }
    }

    void tick(float dt)
    {
        // tick the top-level state for any state-independent handling
        m_SharedData.tick(dt);

        // tick the currently-active state and handle any state transitions
        auto maybeNewState = m_CurrentState->tick(dt);
        if (maybeNewState) {
            m_CurrentState = std::move(maybeNewState);
        }
    }

    void draw()
    {
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        osc::ImGuiNewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

        // draw the currently-active state and handle any state transitions
        auto maybeNewState = m_CurrentState->draw();
        if (maybeNewState) {
            m_CurrentState = std::move(maybeNewState);
        }

        osc::ImGuiRender();

        if (m_ShouldRequestRedraw) {
            App::cur().requestRedraw();
            m_ShouldRequestRedraw = false;
        }
    }

private:
    SharedData m_SharedData;
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
osc::MeshesToModelWizardScreen::Impl* GetModelWizardScreenGLOBAL()
{
    static osc::MeshesToModelWizardScreen::Impl* g_ModelImpoterScreenState = new osc::MeshesToModelWizardScreen::Impl{};
    return g_ModelImpoterScreenState;
}

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen() :
    m_Impl{GetModelWizardScreenGLOBAL()}
{
}

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen(std::vector<std::filesystem::path> paths) :
    m_Impl{GetModelWizardScreenGLOBAL()}
{
    m_Impl->LoadPaths(std::move(paths));
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
