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
    std::string OrientationString(glm::vec3 const& eulerAngles)
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

// logical ID support
//
// The model graph contains internal cross-references. E.g. a joint in the model may
// cross-reference bodies that are somewhere else in the model. Those references are
// looked up at runtime using associative lookups.
//
// Associative lookups are preferred over direct pointers, shared pointers, array indices,
// etc. here because the model graph can be moved in memory, copied (undo/redo), and be
// heavily edited by the user at runtime. We want the *overall* UI datastructure to have
// value, rather than reference, semantics to make handling that easier.
namespace {

    // logical IDs are used for weak scene-element_to_scene-element relationships
    //
    // external code cannot set these to arbitrary values. They're either empty, copied
    // from some other ID, or "created" by pulling the next ID from a global ID pool.
    class LogicalID {
    private:
        static constexpr int64_t g_EmptyId = -1;

    public:
        static LogicalID next()
        {
            static std::atomic<int64_t> g_NextId = 1;
            return LogicalID{g_NextId++};
        }

        static LogicalID empty() { return LogicalID{g_EmptyId}; }

        LogicalID() : m_Value{g_EmptyId} {}
    protected:
        LogicalID(int64_t value) : m_Value{value} {}

    public:
        bool IsEmpty() const noexcept { return m_Value == g_EmptyId; }
        operator bool () const noexcept { return !IsEmpty(); }
        int64_t Unwrap() const noexcept { return m_Value; }  // handy for printing, hashing, etc.

    private:
        int64_t m_Value;
    };

    std::ostream& operator<<(std::ostream& o, LogicalID const& id) { return o << id.Unwrap(); }
    bool operator==(LogicalID const& lhs, LogicalID const& rhs) { return lhs.Unwrap() == rhs.Unwrap(); }
    bool operator!=(LogicalID const& lhs, LogicalID const& rhs) { return lhs.Unwrap() != rhs.Unwrap(); }

    // strongly-typed version of the above. Purely a development convenience that helps
    // prevent code from trying to use IDs "locked" to one type in contexts where they are
    // strictly "locked" to some other type.
    //
    // Typed IDs "decompose" into untyped LogicalIDs where necessary. Untyped IDs can be explicitly
    // "downcasted" to a typed one by using the explicit constructor. Regardless of type assignment,
    // LogicalIDs are always globally unique.
    template<typename T>
    class LogicalIDT : public LogicalID {
    public:
        static LogicalIDT<T> next() { return LogicalIDT<T>{LogicalID::next()}; }
        static LogicalIDT<T> empty() { return LogicalIDT<T>{LogicalID::empty()}; }
        static LogicalIDT<T> downcast(LogicalID id) { return LogicalIDT<T>{id}; }

    private:
        explicit LogicalIDT(LogicalID id) : LogicalID{id} {}
    };
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
namespace std {

    template<>
    struct hash<LogicalID> {
        size_t operator()(LogicalID const& id) const { return static_cast<size_t>(id.Unwrap()); }
    };

    template<typename T>
    struct hash<LogicalIDT<T>> {
        size_t operator()(LogicalID const& id) const { return static_cast<size_t>(id.Unwrap()); }
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
        LogicalIDT<BodyEl> PreferredAttachmentPoint;
        std::filesystem::path Path;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        LogicalIDT<BodyEl> PreferredAttachmentPoint;
        std::filesystem::path Path;
        std::shared_ptr<Mesh> mesh;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        LogicalIDT<BodyEl> PreferredAttachmentPoint;
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

    // composable declaration of a translation + orientation
    struct TranslationOrientation final {
        glm::vec3 translation;
        glm::vec3 orientation;  // Euler angles
    };

    // returns the translation and orientation of ground in ground
    TranslationOrientation GroundTranslationOrientation()
    {
        return {{}, {}};
    }

    // returns the average of two translations + orientations
    TranslationOrientation Average(TranslationOrientation const& a, TranslationOrientation const& b)
    {
        glm::vec3 translation = (a.translation + b.translation) / 2.0f;
        glm::vec3 orientation = (a.orientation + b.orientation) / 2.0f;
        return TranslationOrientation{translation, orientation};
    }

    // print a `TranslationOrientation` to an output stream
    std::ostream& operator<<(std::ostream& o, TranslationOrientation const& to)
    {
        using osc::operator<<;
        return o << "TranslationOrientation(translation = " << to.translation << ", orientation = " << to.orientation << ')';
    }

    // returns a transform matrix that maps quantities expressed in the TranslationOrientation
    // (i.e. "a poor man's frame") into its base
    glm::mat4 TranslationOrientationToBase(TranslationOrientation const& frame)
    {
        glm::mat4 mtx = EulerAnglesToMat(frame.orientation);
        mtx[3] = glm::vec4{frame.translation, 1.0f};
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
        MeshEl(LogicalIDT<MeshEl> logicalID,
               LogicalIDT<BodyEl> maybeAttachedBody,  // not strictly required: empty implies "attached to ground"
               std::shared_ptr<Mesh> mesh,
               std::filesystem::path const& path) :

            m_LogicalID{std::move(logicalID)},
            m_MaybeAttachedBody{maybeAttachedBody},
            m_TranslationOrientationInGround{},
            m_ScaleFactors{1.0f, 1.0f, 1.0f},
            m_Mesh{std::move(mesh)},
            m_Path{path},
            m_Name{FileNameWithoutExtension(m_Path)}
        {
        }

        LogicalIDT<MeshEl> GetLogicalID() const { return m_LogicalID; }

        LogicalIDT<BodyEl> GetAttachmentPoint() const { return m_MaybeAttachedBody; }
        void SetAttachmentPoint(LogicalIDT<BodyEl> newAttachmentID) { m_MaybeAttachedBody = newAttachmentID; }

        TranslationOrientation const& GetTranslationOrientationInGround() const { return m_TranslationOrientationInGround; }
        void SetTranslationOrientationInGround(TranslationOrientation const& newFrame) { m_TranslationOrientationInGround = newFrame; }

        glm::vec3 const& GetTranslationInGround() const { return m_TranslationOrientationInGround.translation; }
        void SetTranslationInGround(glm::vec3 const& newTranslation) { m_TranslationOrientationInGround.translation = newTranslation; }

        glm::vec3 const& GetOrientationInGround() const { return m_TranslationOrientationInGround.orientation; }
        void SetOrientationInGround(glm::vec3 const& newOrientation) { m_TranslationOrientationInGround.orientation = newOrientation; }

        glm::vec3 const& GetScaleFactors() const { return m_ScaleFactors; }
        void SetScaleFactors(glm::vec3 const& newScaleFactors) { m_ScaleFactors = newScaleFactors; }

        std::shared_ptr<Mesh> const& GetMesh() const { return m_Mesh; }

        std::filesystem::path const& GetPath() const { return m_Path; }

        std::string const& GetName() const { return m_Name; }
        void SetName(std::string_view newName) { m_Name = newName; }

    private:
        LogicalIDT<MeshEl> m_LogicalID;
        LogicalIDT<BodyEl> m_MaybeAttachedBody;
        TranslationOrientation m_TranslationOrientationInGround;
        glm::vec3 m_ScaleFactors;
        std::shared_ptr<Mesh> m_Mesh;
        std::filesystem::path m_Path;
        std::string m_Name;
    };

    // returns `true` if the mesh is directly attached to ground
    bool IsAttachedToGround(MeshEl const& mesh)
    {
        return mesh.GetAttachmentPoint().IsEmpty();
    }

    // returns `true` if the mesh is attached to some body (i.e. not ground)
    bool IsAttachedToABody(MeshEl const& mesh)
    {
        return !mesh.GetAttachmentPoint().IsEmpty();
    }

    // returns a transform matrix that maps quantities expressed in mesh (model) space to groundspace
    glm::mat4 GetModelMatrix(MeshEl const& mesh)
    {
        glm::mat4 translateAndRotate = TranslationOrientationToBase(mesh.GetTranslationOrientationInGround());
        glm::mat4 rescale = glm::scale(glm::mat4{1.0f}, mesh.GetScaleFactors());
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
        return AABBCenter(GetGroundspaceBounds(*mesh.GetMesh(), GetModelMatrix(mesh)));
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
        BodyEl(LogicalIDT<BodyEl> logicalID,
               std::string name,
               glm::vec3 const& position,
               glm::vec3 const& orientation) :

            m_LogicalID{logicalID},
            m_Name{std::move(name)},
            m_TranslationOrientationInGround{position, orientation},
            m_Mass{1.0}  // required: OpenSim goes bananas if a body has a mass <= 0
        {
        }

        LogicalIDT<BodyEl> GetLogicalID() const { return m_LogicalID; }

        std::string const& GetName() const { return m_Name; }
        void SetName(std::string_view newName) { m_Name = newName; }

        TranslationOrientation const& GetTranslationOrientationInGround() const { return m_TranslationOrientationInGround; }
        void SetTranslationOrientationInGround(TranslationOrientation const& newTranslationOrientation) { m_TranslationOrientationInGround = newTranslationOrientation; }

        glm::vec3 const& GetTranslationInGround() const { return m_TranslationOrientationInGround.translation; }
        void SetTranslationInGround(glm::vec3 const& newTranslation) { m_TranslationOrientationInGround.translation = newTranslation; }

        glm::vec3 const& GetOrientationInGround() const { return m_TranslationOrientationInGround.orientation; }
        void SetOrientationInGround(glm::vec3 const& newOrientation) { m_TranslationOrientationInGround.orientation = newOrientation; }

        double GetMass() const { return m_Mass; }
        void SetMass(double newMass) { m_Mass = newMass; }

    private:
        LogicalIDT<BodyEl> m_LogicalID;
        std::string m_Name;
        TranslationOrientation m_TranslationOrientationInGround;
        double m_Mass;
    };

    // compute a joint's name based on whether it has a user-defined name or not
    std::string const& CalcJointName(size_t jointTypeIndex, std::string const& maybeName)
    {
        if (!maybeName.empty()) {
            return maybeName;
        }

        return JointRegistry::nameStrings()[jointTypeIndex];
    }

    // returns `true` if `mesh` is directly attached to `body`
    bool IsMeshAttachedToBody(MeshEl const& mesh, BodyEl const& body)
    {
        return mesh.GetAttachmentPoint() == body.GetLogicalID();
    }

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
        JointAttachment(LogicalIDT<BodyEl> bodyID) :
            m_BodyID{bodyID},
            m_TranslationOrientationInGround{}
        {
        }

        JointAttachment(LogicalIDT<BodyEl> bodyID, TranslationOrientation const& translationOrientationInGround) :
            m_BodyID{bodyID},
            m_TranslationOrientationInGround{translationOrientationInGround}
        {
        }

        LogicalIDT<BodyEl> GetBodyID() const { return m_BodyID; }
        void SetBodyID(LogicalIDT<BodyEl> newBodyID) { m_BodyID = std::move(newBodyID); }

        TranslationOrientation const& GetTranslationOrientationInGround() const { return m_TranslationOrientationInGround; }
        void SetTranslationOrientationInGround(TranslationOrientation const& newTranslationOrientation) { m_TranslationOrientationInGround = newTranslationOrientation; }

        glm::vec3 const& GetTranslationInGround() const { return m_TranslationOrientationInGround.translation; }
        void SetTranslationInGround(glm::vec3 const& newTranslation) { m_TranslationOrientationInGround.translation = newTranslation; }

        glm::vec3 const& GetOrientationInGround() const { return m_TranslationOrientationInGround.orientation; }
        void SetOrientationInGround(glm::vec3 const& newOrientation) { m_TranslationOrientationInGround.orientation = newOrientation; }

    private:
        LogicalIDT<BodyEl> m_BodyID;
        TranslationOrientation m_TranslationOrientationInGround;
    };

    // a joint scene element
    //
    // see `JointAttachment` comment for an explanation of why it's designed this way.
    class JointEl final {
    public:
        JointEl(LogicalIDT<JointEl> logicalID,
                size_t jointTypeIdx,
                std::string maybeName,
                JointAttachment parentAttachment,
                JointAttachment childAttachment) :

            m_LogicalID{logicalID},
            m_JointTypeIndex{jointTypeIdx},
            m_MaybeUserAssignedName{std::move(maybeName)},
            m_ParentAttachment{parentAttachment},
            m_ChildAttachment{childAttachment}
        {
        }

        LogicalIDT<JointEl> GetLogicalID() const { return m_LogicalID; }

        size_t GetJointTypeIndex() const { return m_JointTypeIndex; }
        void SetJointTypeIndex(size_t newJointTypeIndex) { m_JointTypeIndex = newJointTypeIndex; }

        bool HasUserAssignedName() const { return !m_MaybeUserAssignedName.empty(); }
        std::string const& GetUserAssignedName() const { return m_MaybeUserAssignedName; }
        void SetUserAssignedName(std::string_view newName) { m_MaybeUserAssignedName = newName; }

        LogicalIDT<BodyEl> GetParentBodyID() const { return m_ParentAttachment.GetBodyID(); }
        void SetParentBodyID(LogicalIDT<BodyEl> newParentBodyID) { m_ParentAttachment.SetBodyID(std::move(newParentBodyID)); }

        TranslationOrientation const& GetParentAttachmentTranslationOrientationInGround() const { return m_ParentAttachment.GetTranslationOrientationInGround(); }
        void SetParentAttachmentTranslationOrientationInGround(TranslationOrientation const& newTranslationOrientation) { m_ParentAttachment.SetTranslationOrientationInGround(newTranslationOrientation); }

        glm::vec3 const& GetParentAttachmentTranslationInGround() const { return m_ParentAttachment.GetTranslationInGround(); }
        void SetParentAttachmentTranslationInGround(glm::vec3 const& newTranslation) { m_ParentAttachment.SetTranslationInGround(newTranslation); }

        glm::vec3 const& GetParentAttachmentOrientationInGround() const { return m_ParentAttachment.GetOrientationInGround(); }
        void SetParentAttachmentOrientationInGround(glm::vec3 const& newOrientation) { m_ParentAttachment.SetOrientationInGround(newOrientation); }

        LogicalIDT<BodyEl> GetChildBodyID() const { return m_ChildAttachment.GetBodyID(); }
        void SetChildBodyID(LogicalIDT<BodyEl> newChildBodyID) { m_ChildAttachment.SetBodyID(std::move(newChildBodyID)); }

        TranslationOrientation const& GetChildAttachmentTranslationOrientationInGround() const { return m_ChildAttachment.GetTranslationOrientationInGround(); }
        void SetChildAttachmentTranslationOrientationInGround(TranslationOrientation const& newTranslationOrientation) { m_ChildAttachment.SetTranslationOrientationInGround(newTranslationOrientation); }

        glm::vec3 const& GetChildAttachmentTranslationInGround() const { return m_ChildAttachment.GetTranslationInGround(); }
        void SetChildAttachmentTranslationInGround(glm::vec3 const& newTranslation) { m_ChildAttachment.SetTranslationInGround(newTranslation); }

        glm::vec3 const& GetChildAttachmentOrientationInGround() const { return m_ChildAttachment.GetOrientationInGround(); }
        void SetChildAttachmentOrientationInGround(glm::vec3 const& newOrientation) { m_ChildAttachment.SetOrientationInGround(newOrientation); }

    private:
        LogicalIDT<JointEl> m_LogicalID;
        size_t m_JointTypeIndex;
        std::string m_MaybeUserAssignedName;
        JointAttachment m_ParentAttachment;
        JointAttachment m_ChildAttachment;
    };

    // returns a human-readable name for the joint
    std::string const& GetName(JointEl const& joint)
    {
        if (joint.HasUserAssignedName()) {
            return joint.GetUserAssignedName();
        } else {
            return JointRegistry::nameStrings()[joint.GetJointTypeIndex()];
        }
    }

    // returns `true` if `body` is `joint`'s parent
    bool IsParentOfJoint(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetParentBodyID() == body.GetLogicalID();
    }

    // returns `true` if `body` is `joint`'s child
    bool IsChildOfJoint(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetChildBodyID() == body.GetLogicalID();
    }

    // returns `true` if `body` is the child attachment of `joint`
    bool IsAttachedToJointAsChild(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetChildBodyID() == body.GetLogicalID();
    }

    // returns `true` if `body` is the parent attachment of `joint`
    bool IsAttachedToJointAsParent(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetParentBodyID() == body.GetLogicalID();
    }

    // returns `true` if body is either the parent or the child attachment of `joint`
    bool IsAttachedToJoint(JointEl const& joint, BodyEl const& body)
    {
        return IsAttachedToJointAsChild(joint, body) || IsAttachedToJointAsParent(joint, body);
    }

    // returns `true` is a joint's parent is ground
    bool IsJointAttachedToGroundAsParent(JointEl const& joint)
    {
        return joint.GetParentBodyID().IsEmpty();
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
        std::unordered_map<LogicalIDT<MeshEl>, MeshEl> const& GetMeshes() const { return m_Meshes; }
        std::unordered_map<LogicalIDT<BodyEl>, BodyEl> const& GetBodies() const { return m_Bodies; }
        std::unordered_map<LogicalIDT<JointEl>, JointEl> const& GetJoints() const { return m_Joints; }
        std::unordered_set<LogicalID> const& GetSelected() const { return m_Selected; }

        MeshEl const* TryGetMeshElByID(LogicalID id) const { return const_cast<ModelGraph&>(*this).TryUpdMeshElByID(id); }
        BodyEl const* TryGetBodyElByID(LogicalID id) const { return const_cast<ModelGraph&>(*this).TryUpdBodyElByID(id); }
        JointEl const* TryGetJointElByID(LogicalID id) const { return const_cast<ModelGraph&>(*this).TryUpdJointElByID(id); }

        MeshEl const& GetMeshByIDOrThrow(LogicalID id) const { return const_cast<ModelGraph&>(*this).UpdMeshByIDOrThrow(id); }
        BodyEl const& GetBodyByIDOrThrow(LogicalID id) const { return const_cast<ModelGraph&>(*this).UpdBodyByIDOrThrow(id); }
        JointEl const& GetJointByIDOrThrow(LogicalID id) const { return const_cast<ModelGraph&>(*this).UpdJointByIDOrThrow(id); }

        LogicalIDT<BodyEl> AddBody(std::string name, glm::vec3 const& position, glm::vec3 const& orientation)
        {
            LogicalIDT<BodyEl> id = LogicalIDT<BodyEl>::next();
            return m_Bodies.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, name, position, orientation)).first->first;
        }

        LogicalIDT<MeshEl> AddMesh(std::shared_ptr<Mesh> mesh, LogicalIDT<BodyEl> maybeAttachedBody, std::filesystem::path const& path)
        {
            if (maybeAttachedBody) {
                OSC_ASSERT_ALWAYS(TryGetBodyElByID(maybeAttachedBody) != nullptr && "invalid attachment ID");
            }


            LogicalIDT<MeshEl> id = LogicalIDT<MeshEl>::next();
            return m_Meshes.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, maybeAttachedBody, mesh, path)).first->first;
        }

        LogicalIDT<JointEl> AddJoint(size_t jointTypeIdx, std::string maybeName, JointAttachment parentAttachment, JointAttachment childAttachment)
        {
            // TODO: validate joint attachment for this particular type of joint

            LogicalIDT<JointEl> id = LogicalIDT<JointEl>::next();
            return m_Joints.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(id, jointTypeIdx, maybeName, parentAttachment, childAttachment)).first->first;
        }

        void SetMeshAttachmentPoint(LogicalIDT<MeshEl> meshID, LogicalIDT<BodyEl> bodyID)
        {
            UpdMeshByIDOrThrow(meshID).SetAttachmentPoint(bodyID);
        }

        void UnsetMeshAttachmentPoint(LogicalIDT<MeshEl> meshID)
        {
            UpdMeshByIDOrThrow(meshID).SetAttachmentPoint(LogicalIDT<BodyEl>::empty());
        }

        void SetMeshTranslationOrientationInGround(LogicalIDT<MeshEl> meshID, TranslationOrientation const& newTO)
        {
            UpdMeshByIDOrThrow(meshID).SetTranslationOrientationInGround(newTO);
        }

        void SetMeshScaleFactors(LogicalIDT<MeshEl> meshID, glm::vec3 const& newScaleFactors)
        {
            UpdMeshByIDOrThrow(meshID).SetScaleFactors(newScaleFactors);
        }

        void SetMeshName(LogicalIDT<MeshEl> meshID, std::string_view newName)
        {
            UpdMeshByIDOrThrow(meshID).SetName(newName);
        }

        void SetBodyName(LogicalIDT<BodyEl> bodyID, std::string_view newName)
        {
            UpdBodyByIDOrThrow(bodyID).SetName(newName);
        }

        void SetBodyTranslationOrientationInGround(LogicalIDT<BodyEl> bodyID, TranslationOrientation const& newTO)
        {
            UpdBodyByIDOrThrow(bodyID).SetTranslationOrientationInGround(newTO);
        }

        void SetBodyMass(LogicalIDT<BodyEl> bodyID, double newMass)
        {
            UpdBodyByIDOrThrow(bodyID).SetMass(newMass);
        }

        template<typename Consumer>
        void ForEachSceneElID(Consumer idConsumer) const
        {
            for (auto const& [meshID, mesh] : GetMeshes()) { idConsumer(meshID); }
            for (auto const& [bodyID, body] : GetBodies()) { idConsumer(bodyID); }
            for (auto const& [jointID, joint] : GetJoints()) { idConsumer(jointID); }
        }

        void DeleteMeshElByID(LogicalID id)
        {
            auto it = m_Meshes.find(LogicalIDT<MeshEl>::downcast(id));
            if (it != m_Meshes.end()) {
                DeleteMesh(it);
            }
        }

        void DeleteBodyElByID(LogicalID id)
        {
            auto it = m_Bodies.find(LogicalIDT<BodyEl>::downcast(id));
            if (it != m_Bodies.end()) {
                DeleteBody(it);
            }
        }

        void DeleteJointElByID(LogicalID id)
        {
            auto it = m_Joints.find(LogicalIDT<JointEl>::downcast(id));
            if (it != m_Joints.end()) {
                DeleteJoint(it);
            }
        }

        void DeleteElementByID(LogicalID id)
        {
            DeleteMeshElByID(id);
            DeleteBodyElByID(id);
            DeleteJointElByID(id);
        }

        void ApplyTranslation(LogicalID id, glm::vec3 const& translation)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                auto newTranslation = meshPtr->GetTranslationInGround() + translation;
                meshPtr->SetTranslationInGround(newTranslation);
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                auto newTranslation = bodyPtr->GetTranslationInGround() + translation;
                bodyPtr->SetTranslationInGround(newTranslation);
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                auto newParentTranslation = jointPtr->GetParentAttachmentTranslationInGround();
                jointPtr->SetParentAttachmentTranslationInGround(newParentTranslation);
                auto newChildTranslation = jointPtr->GetChildAttachmentTranslationInGround() + translation;
                jointPtr->SetChildAttachmentTranslationInGround(newChildTranslation);
            }
        }

        void ApplyRotation(LogicalID id, glm::vec3 const& eulerAngles)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                glm::vec3 newOrientation = EulerCompose(meshPtr->GetOrientationInGround(), eulerAngles);
                meshPtr->SetOrientationInGround(newOrientation);
            } else if (BodyEl* bodyPtr = TryUpdBodyElByID(id)) {
                glm::vec3 newOrientation = EulerCompose(bodyPtr->GetOrientationInGround(), eulerAngles);
                bodyPtr->SetOrientationInGround(newOrientation);
            } else if (JointEl* jointPtr = TryUpdJointElByID(id)) {
                glm::vec3 newParentOrientation = EulerCompose(jointPtr->GetParentAttachmentOrientationInGround(), eulerAngles);
                jointPtr->SetParentAttachmentOrientationInGround(newParentOrientation);
                glm::vec3 newChildOrientation = EulerCompose(jointPtr->GetChildAttachmentOrientationInGround(), eulerAngles);
                jointPtr->SetChildAttachmentOrientationInGround(newChildOrientation);
            }
        }

        void ApplyScale(LogicalID id, glm::vec3 const& scaleFactors)
        {
            if (MeshEl* meshPtr = TryUpdMeshElByID(id)) {
                glm::vec3 newSfs = meshPtr->GetScaleFactors() * scaleFactors;
                meshPtr->SetScaleFactors(newSfs);
            }
        }

        glm::vec3 GetTranslationInGround(LogicalID id) const
        {
            if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return meshPtr->GetTranslationInGround();
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return bodyPtr->GetTranslationInGround();
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                glm::vec3 components[] = {
                    GetTranslationInGround(jointPtr->GetParentBodyID()),
                    GetTranslationInGround(jointPtr->GetChildBodyID()),
                    jointPtr->GetParentAttachmentTranslationInGround(),
                    jointPtr->GetChildAttachmentTranslationInGround(),
                };
                return VecNumericallyStableAverage(components, 4);
            } else {
                throw std::runtime_error{"GetTranslation(): cannot find element by ID"};
            }
        }

        glm::vec3 GetOrientationInGround(LogicalID id) const
        {
            if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return meshPtr->GetOrientationInGround();
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                return bodyPtr->GetOrientationInGround();
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                glm::vec3 components[] = {
                    GetOrientationInGround(jointPtr->GetParentBodyID()),
                    GetOrientationInGround(jointPtr->GetChildBodyID()),
                    jointPtr->GetParentAttachmentTranslationInGround(),
                    jointPtr->GetChildAttachmentTranslationInGround()
                };
                return VecNumericallyStableAverage(components, 4);
            } else {
                throw std::runtime_error{"GetOrientationInground(): cannot find element by ID"};
            }
        }

        TranslationOrientation GetTranslationOrientationInGround(LogicalID id) const
        {
            return {GetTranslationInGround(id), GetOrientationInGround(id)};
        }

        // returns empty AABB at point if a point-like element (e.g. mesh, joint pivot)
        AABB GetBounds(LogicalID id) const
        {
            if (MeshEl const* meshPtr = TryGetMeshElByID(id)) {
                return AABBApplyXform(meshPtr->GetMesh()->getAABB(), GetModelMatrix(*meshPtr));
            } else if (BodyEl const* bodyPtr = TryGetBodyElByID(id)) {
                glm::vec3 loc = bodyPtr->GetTranslationInGround();
                return AABB{loc, loc};
            } else if (JointEl const* jointPtr = TryGetJointElByID(id)) {
                glm::vec3 points[] = {
                    GetTranslationInGround(jointPtr->GetParentBodyID()),
                    GetTranslationInGround(jointPtr->GetChildBodyID()),
                    jointPtr->GetParentAttachmentTranslationInGround(),
                    jointPtr->GetChildAttachmentTranslationInGround()
                };
                return AABBFromVerts(points, 4);
            } else {
                throw std::runtime_error{"GetBounds(): could not find supplied ID"};
            }
        }

        void SelectAll()
        {
            auto addIDToSelectionSet = [this](LogicalID id) { m_Selected.insert(id); };
            ForEachSceneElID(addIDToSelectionSet);
        }

        void DeSelectAll()
        {
            m_Selected.clear();
        }

        void Select(LogicalID id)
        {
            m_Selected.insert(id);
        }

        void DeSelect(LogicalID id)
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

        bool IsSelected(LogicalID id) const
        {
            return m_Selected.find(id) != m_Selected.end();
        }

        void DeleteSelected()
        {
            auto selected = m_Selected;
            for (LogicalID id : selected) {
                DeleteElementByID(id);
            }
            m_Selected.clear();
        }

    private:
        MeshEl* TryUpdMeshElByID(LogicalID id)
        {
            auto it = m_Meshes.find(LogicalIDT<MeshEl>::downcast(id));
            return it != m_Meshes.end() ? &it->second : nullptr;
        }

        BodyEl* TryUpdBodyElByID(LogicalID id)
        {
            auto it = m_Bodies.find(LogicalIDT<BodyEl>::downcast(id));
            return it != m_Bodies.end() ? &it->second : nullptr;
        }

        JointEl* TryUpdJointElByID(LogicalID id)
        {
            auto it = m_Joints.find(LogicalIDT<JointEl>::downcast(id));
            return it != m_Joints.end() ? &it->second : nullptr;
        }

        MeshEl& UpdMeshByIDOrThrow(LogicalID id)
        {
            MeshEl* meshEl = TryUpdMeshElByID(id);
            if (!meshEl) {
                throw std::runtime_error{"could not find a mesh"};
            }
            return *meshEl;
        }

        BodyEl& UpdBodyByIDOrThrow(LogicalID id)
        {
            BodyEl* bodyEl = TryUpdBodyElByID(id);
            if (!bodyEl) {
                throw std::runtime_error{"could not find a body"};
            }
            return *bodyEl;
        }

        JointEl& UpdJointByIDOrThrow(LogicalID id)
        {
            JointEl* jointEl = TryUpdJointElByID(id);
            if (!jointEl) {
                throw std::runtime_error{"could not find a joint"};
            }
            return *jointEl;
        }

        void DeleteMesh(std::unordered_map<LogicalIDT<MeshEl>, MeshEl>::iterator it)
        {
            DeSelect(it->first);
            m_Meshes.erase(it);
        }

        void DeleteBody(std::unordered_map<LogicalIDT<BodyEl>, BodyEl>::iterator it)
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
                if (meshIt->second.GetAttachmentPoint() == bodyID) {
                    DeSelect(meshIt->first);
                    meshIt = m_Meshes.erase(meshIt);
                }
            }

            // delete the body
            DeSelect(it->first);
            m_Bodies.erase(it);
        }

        void DeleteJoint(std::unordered_map<LogicalIDT<JointEl>, JointEl>::iterator it)
        {
            DeSelect(it->first);
            m_Joints.erase(it);
        }

        std::unordered_map<LogicalIDT<MeshEl>, MeshEl> m_Meshes;
        std::unordered_map<LogicalIDT<BodyEl>, BodyEl> m_Bodies;
        std::unordered_map<LogicalIDT<JointEl>, JointEl> m_Joints;
        std::unordered_set<LogicalID> m_Selected;
    };

    // try to find the parent body of the given joint
    //
    // returns nullptr if the joint's parent has an invalid id or is attached to ground
    BodyEl const* TryGetParentBody(ModelGraph const& modelGraph, JointEl const& joint)
    {
        auto maybeId = joint.GetParentBodyID();

        if (!maybeId) {
            return nullptr;
        }

        return modelGraph.TryGetBodyElByID(maybeId);
    }

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(ModelGraph const& modelGraph, BodyEl const& body)
    {
        return AnyOf(modelGraph.GetJoints(), [&](auto const& pair) { return IsAttachedToJointAsChild(pair.second, body); });
    }

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(ModelGraph const& modelGraph, JointEl const& jointEl)
    {
        auto parentBodyID = jointEl.GetParentBodyID();
        auto childBodyID = jointEl.GetChildBodyID();

        if (parentBodyID == childBodyID) {
            return true;  // is directly attached to itself
        }

        if (parentBodyID && !modelGraph.TryGetBodyElByID(parentBodyID)) {
             return true;  // has a parent ID that's invalid for this model graph
        }

        if (childBodyID && !modelGraph.TryGetBodyElByID(childBodyID)) {
            return true;  // has a child ID that's invalid for this model graph
        }

        return false;
    }

    // returns `true` if a body is indirectly or directly attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
                                BodyEl const& body,
                                std::unordered_set<LogicalID>& previouslyVisitedJoints);

    // returns `true` if `joint` is indirectly or directly attached to ground via its parent
    bool IsJointAttachedToGround(ModelGraph const& modelGraph,
                                 JointEl const& joint,
                                 std::unordered_set<LogicalID>& previousVisits)
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
                                std::unordered_set<LogicalID>& previouslyVisitedJoints)
    {
        for (auto const& [jointID, jointEl] : modelGraph.GetJoints()) {
            OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, jointEl));

            if (IsChildOfJoint(jointEl, body)) {
                bool alreadyVisited = !previouslyVisitedJoints.emplace(jointEl.GetLogicalID()).second;
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
                ss << GetName(joint) << ": joint is garbage (this is an implementation error)";
                throw std::runtime_error{std::move(ss).str()};
            }
        }

        for (auto const& [id, body] : modelGraph.GetBodies()) {
            std::unordered_set<LogicalID> previouslyVisitedJoints;
            if (!IsBodyAttachedToGround(modelGraph, body, previouslyVisitedJoints)) {
                std::stringstream ss;
                ss << body.GetName() << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
                issuesOut.push_back(std::move(ss).str());
            }
        }

        return !issuesOut.empty();
    }

    // attaches a mesh to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
    void AttachMeshElToFrame(MeshEl const& meshEl,
                             TranslationOrientation const& parentTranslationAndOrientationInGround,
                             OpenSim::PhysicalFrame& parentPhysFrame)
    {
        // create a POF that attaches to the body
        auto meshPhysOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
        meshPhysOffsetFrame->setName(meshEl.GetName() + "_offset");

        // re-express the transform matrix in the parent's frame
        glm::mat4 parent2ground = TranslationOrientationToBase(parentTranslationAndOrientationInGround);
        glm::mat4 ground2parent = glm::inverse(parent2ground);
        glm::mat4 mesh2ground = TranslationOrientationToBase(meshEl.GetTranslationOrientationInGround());
        glm::mat4 mesh2parent = ground2parent * mesh2ground;

        // set it as the transform
        meshPhysOffsetFrame->setOffsetTransform(SimTKTransformFromMat4x3(mesh2parent));

        // attach mesh to the POF
        auto mesh = std::make_unique<OpenSim::Mesh>(meshEl.GetPath().string());
        mesh->setName(meshEl.GetName());
        mesh->set_scale_factors(SimTKVec3FromV3(meshEl.GetScaleFactors()));
        meshPhysOffsetFrame->attachGeometry(mesh.release());

        parentPhysFrame.addComponent(meshPhysOffsetFrame.release());
    }

    // create a body for the `model`, but don't add it to the model yet
    //
    // *may* add any attached meshes to the model, though
    std::unique_ptr<OpenSim::Body> CreateDetatchedBody(ModelGraph const& mg, BodyEl const& bodyEl)
    {
        auto addedBody = std::make_unique<OpenSim::Body>();
        addedBody->setMass(bodyEl.GetMass());
        addedBody->setName(bodyEl.GetName());

        for (auto const& [meshID, mesh] : mg.GetMeshes()) {
            if (IsMeshAttachedToBody(mesh, bodyEl)) {
                AttachMeshElToFrame(mesh, bodyEl.GetTranslationOrientationInGround(), *addedBody);
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
                                                      std::unordered_map<LogicalID, OpenSim::Body*>& visitedBodies,
                                                      LogicalIDT<BodyEl> elID)
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
        if (jointEl.HasUserAssignedName()) {
            return jointEl.GetUserAssignedName();
        } else {
            return childFrame.getName() + "_to_" + parentFrame.getName();
        }
    }

    // returns the translation and orientation of the given body in ground
    //
    // if the supplied ID is empty, returns the translation and orientation of ground itself (i.e. 0.0...)
    TranslationOrientation GetBodyTranslationAndOrientationInGround(ModelGraph const& mg, LogicalIDT<BodyEl> bodyID)
    {
        if (bodyID.IsEmpty()) {
            return TranslationOrientation{};  // ground
        }

        BodyEl const* body = mg.TryGetBodyElByID(bodyID);

        if (!body) {
            throw std::runtime_error{"cannot get the position of this body: the ID is invalid"};
        }

        return body->GetTranslationOrientationInGround();
    }

    // returns a `TranslationOrientation` that can reorient things expressed in base to things expressed in parent
    TranslationOrientation TranslationOrientationInParent(TranslationOrientation const& parentInBase, TranslationOrientation const& childInBase)
    {
        glm::mat4 parent2base = TranslationOrientationToBase(parentInBase);
        glm::mat4 base2parent = glm::inverse(parent2base);
        glm::mat4 child2base = TranslationOrientationToBase(childInBase);
        glm::mat4 child2parent = base2parent * child2base;

        glm::vec3 translation = base2parent * glm::vec4{childInBase.translation, 1.0f};
        glm::vec3 orientation = MatToEulerAngles(child2parent);

        return TranslationOrientation{translation, orientation};
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
        constexpr std::array<char const*, 3> translationNames = {"_tx", "_ty", "_tz"};
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
                                                    TranslationOrientation const& parentPofInGround,
                                                    TranslationOrientation const& childPofInGround)
    {
        size_t jointTypeIdx = *JointRegistry::indexOf(joint);

        TranslationOrientation toInParent = TranslationOrientationInParent(parentPofInGround, childPofInGround);

        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(jointTypeIdx);

        // handle translations
        for (int i = 0; i < 3; ++i) {
            if (dofs.translation[i] == -1) {
                if (!IsEffectivelyZero(toInParent.translation[i])) {
                    throw std::runtime_error{"invalid POF translation: joint offset frames have a nonzero translation but the joint doesn't have a coordinate along the necessary DoF"};
                }
            } else {
                joint.upd_coordinates(dofs.translation[i]).setDefaultValue(toInParent.translation[i]);
            }
        }

        // handle orientations
        for (int i = 0; i < 3; ++i) {
            if (dofs.orientation[i] == -1) {
                if (!IsEffectivelyZero(toInParent.orientation[i])) {
                    throw std::runtime_error{"invalid POF rotation: joint offset frames have a nonzero rotation but the joint doesn't have a coordinate along the necessary DoF"};
                }
            } else {
                joint.upd_coordinates(dofs.orientation[i]).setDefaultValue(toInParent.orientation[i]);
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
                              std::unordered_map<LogicalID, OpenSim::Body*>& visitedBodies,
                              std::unordered_set<LogicalID>& visitedJoints)
    {
        LogicalID jointID = joint.GetLogicalID();

        if (auto const& [it, wasInserted] = visitedJoints.emplace(jointID); !wasInserted) {
            return;  // graph cycle detected: joint was already previously visited and shouldn't be traversed again
        }

        // lookup each side of the joint, creating the bodies if necessary
        JointAttachmentCachedLookupResult parent = LookupPhysFrame(mg, model, visitedBodies, joint.GetParentBodyID());
        JointAttachmentCachedLookupResult child = LookupPhysFrame(mg, model, visitedBodies, joint.GetChildBodyID());

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parentPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parentPOF->setName(parent.physicalFrame->getName() + "_offset");
        parentPOF->setParentFrame(*parent.physicalFrame);
        TranslationOrientation toParentInGround = GetBodyTranslationAndOrientationInGround(mg, joint.GetParentBodyID());
        TranslationOrientation toParentPofInGround = joint.GetParentAttachmentTranslationOrientationInGround();
        TranslationOrientation toParentPofInParent = TranslationOrientationInParent(toParentInGround, toParentPofInGround);
        parentPOF->set_translation(SimTKVec3FromV3(toParentPofInParent.translation));
        parentPOF->set_orientation(SimTKVec3FromV3(toParentPofInParent.orientation));

        // create the child OpenSim::PhysicalOffsetFrame
        auto childPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        childPOF->setName(child.physicalFrame->getName() + "_offset");
        childPOF->setParentFrame(*child.physicalFrame);
        TranslationOrientation toChildInGround = GetBodyTranslationAndOrientationInGround(mg, joint.GetChildBodyID());
        TranslationOrientation toChildPofInGround = joint.GetChildAttachmentTranslationOrientationInGround();
        TranslationOrientation toChildPofInChild = TranslationOrientationInParent(toChildInGround, toChildPofInGround);
        childPOF->set_translation(SimTKVec3FromV3(toChildPofInChild.translation));
        childPOF->set_orientation(SimTKVec3FromV3(toChildPofInChild.orientation));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto jointUniqPtr = ConstructOpenSimJointFromTypeIndex(joint.GetJointTypeIndex());

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
            if (IsAttachedToJointAsParent(otherJoint, *child.bodyEl)) {
                AttachJointRecursive(mg, model, otherJoint, visitedBodies, visitedJoints);
            }
        }
    }

    // attaches `BodyEl` into `model` by directly attaching it to ground
    void AttachBodyDirectlyToGround(ModelGraph const& mg,
                                    OpenSim::Model& model,
                                    BodyEl const& bodyEl,
                                    std::unordered_map<LogicalID, OpenSim::Body*>& visitedBodies)
    {
        auto addedBody = CreateDetatchedBody(mg, bodyEl);
        auto joint = std::make_unique<OpenSim::FreeJoint>();

        // set joint name
        joint->setName(bodyEl.GetName() + "_to_ground");

        // set joint coordinate names
        SetJointCoordinateNames(*joint, bodyEl.GetName());

        // set joint's default location of the body's position in the ground
        SetJointCoordinatesBasedOnFrameDifferences(*joint, TranslationOrientation{}, bodyEl.GetTranslationOrientationInGround());

        // connect joint from ground to the body
        joint->connectSocket_parent_frame(model.getGround());
        joint->connectSocket_child_frame(*addedBody);

        // populate it in the "already visited bodies" cache
        visitedBodies[bodyEl.GetLogicalID()] = addedBody.get();

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
            if (IsAttachedToGround(mesh)) {
                AttachMeshElToFrame(mesh, TranslationOrientation{}, model->updGround());
            }
        }

        // keep track of any bodies/joints already visited (there might be cycles)
        std::unordered_map<LogicalID, OpenSim::Body*> visitedBodies;
        std::unordered_set<LogicalID> visitedJoints;

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

            auto parentID = joint.GetParentBodyID();

            if (IsJointAttachedToGroundAsParent(joint) || ContainsKey(visitedBodies, parentID)) {
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
        LogicalID id;
        LogicalID groupId;
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
                   glm::vec3 const& lightDir,
                   glm::vec3 const& lightCol,
                   glm::vec4 const& bgCol,
                   nonstd::span<DrawableThing const> drawables,
                   gl::Texture2D& outSceneTex)
    {
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

                gl::Uniform(scs.uColor, d.rimColor * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
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
            gl::Uniform(eds.uRimRgba, {1.0f, 0.0f, 0.0f, 1.0f});
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

    // senteniel IDs
    //
    // these come in handy when (e.g.) an implementation needs
    // to differentiate between "empty" and "ground"

    LogicalID const g_GroundID = LogicalID::next();

    LogicalID const g_BodyGroupID = LogicalID::next();
    LogicalID const g_MeshGroupID = LogicalID::next();
    LogicalID const g_GroundGroupID = LogicalID::next();

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

        std::unordered_set<LogicalID> const& GetCurrentSelection() const
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

        void Select(LogicalID id)
        {
            UpdCurrentModelGraph().Select(id);
        }

        void DeSelect(LogicalID id)
        {
            UpdCurrentModelGraph().DeSelect(id);
        }

        bool HasSelection() const
        {
            return GetCurrentModelGraph().HasSelection();
        }

        bool IsSelected(LogicalID id) const
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

        LogicalIDT<BodyEl> AddBody(std::string const& name, glm::vec3 const& pos, glm::vec3 const& orientation)
        {
            auto id = UpdCurrentModelGraph().AddBody(name, pos, orientation);
            UpdCurrentModelGraph().DeSelectAll();
            UpdCurrentModelGraph().Select(id);
            CommitCurrentModelGraph(std::string{"added "} + name);
            return id;
        }

        LogicalIDT<BodyEl> AddBody(glm::vec3 const& pos)
        {
            return AddBody(GenerateBodyName(), pos, {});
        }

        void PushMeshLoadRequest(std::filesystem::path const& meshFilePath, LogicalIDT<BodyEl> bodyToAttachTo)
        {
            m_MeshLoader.send(MeshLoadRequest{bodyToAttachTo, meshFilePath});
        }

        void PushMeshLoadRequest(std::filesystem::path const& meshFilePath)
        {
            m_MeshLoader.send(MeshLoadRequest{LogicalIDT<BodyEl>::empty(), meshFilePath});
        }

        // called when the mesh loader responds with a fully-loaded mesh
        void PopMeshLoader_OnOKResponse(MeshLoadOKResponse& ok)
        {
            ModelGraph& mg = UpdCurrentModelGraph();

            auto meshID = mg.AddMesh(ok.mesh, ok.PreferredAttachmentPoint, ok.Path);

            auto const* maybeBody = mg.TryGetBodyElByID(ok.PreferredAttachmentPoint);
            if (maybeBody) {
                mg.SetMeshTranslationOrientationInGround(meshID, maybeBody->GetTranslationOrientationInGround());
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
            ImGui::GetForegroundDrawList()->AddLine(parent, child, color, 2.0f);

            // triangle indicating connection directionality
            glm::vec2 midpoint = (parent + child) / 2.0f;
            glm::vec2 direction = glm::normalize(parent - child);
            glm::vec2 normal = {-direction.y, direction.x};

            constexpr float triangleWidth = 10.0f;
            glm::vec2 p1 = midpoint + (triangleWidth/2.0f)*normal;
            glm::vec2 p2 = midpoint - (triangleWidth/2.0f)*normal;
            glm::vec2 p3 = midpoint + triangleWidth*direction;

            ImGui::GetForegroundDrawList()->AddTriangleFilled(p1, p2, p3, color);
        }

        void DrawConnectionLine(MeshEl const& meshEl, ImU32 color) const
        {
            glm::vec3 meshLoc = meshEl.GetTranslationInGround();
            glm::vec3 otherLoc{};

            if (IsAttachedToABody(meshEl)) {
                BodyEl const& body = GetCurrentModelGraph().GetBodyByIDOrThrow(meshEl.GetAttachmentPoint());
                otherLoc = body.GetTranslationInGround();
            }

            DrawConnectionLine(color, WorldPosToScreenPos(otherLoc), WorldPosToScreenPos(meshLoc));
        }

        void DrawConnectionLineToGround(BodyEl const& bodyEl, ImU32 color) const
        {
            glm::vec3 bodyLoc = bodyEl.GetTranslationInGround();
            glm::vec3 otherLoc = {};

            DrawConnectionLine(color, WorldPosToScreenPos(otherLoc), WorldPosToScreenPos(bodyLoc));
        }

        void DrawConnectionLine(JointEl const& jointEl, ImU32 color, LogicalID excludeID = LogicalID::empty()) const
        {
            if (jointEl.GetLogicalID() == excludeID) {
                return;
            }

            auto childID = jointEl.GetChildBodyID();
            if (childID != excludeID) {
                glm::vec3 childLoc = childID && childID != g_GroundID ? GetCurrentModelGraph().GetBodyByIDOrThrow(childID).GetTranslationInGround() : glm::vec3{};
                glm::vec3 childPivotLoc = jointEl.GetChildAttachmentTranslationInGround();
                DrawConnectionLine(color, WorldPosToScreenPos(childPivotLoc), WorldPosToScreenPos(childLoc));
            }

            auto parentID = jointEl.GetParentBodyID();
            if (parentID != excludeID) {
                glm::vec3 parentLoc = parentID && parentID != g_GroundID ? GetCurrentModelGraph().GetBodyByIDOrThrow(parentID).GetTranslationInGround() : glm::vec3{};
                glm::vec3 parentPivotLoc = jointEl.GetParentAttachmentTranslationInGround();
                DrawConnectionLine(color, WorldPosToScreenPos(parentLoc), WorldPosToScreenPos(parentPivotLoc));
            }
        }

        void DrawConnectionLines() const
        {
            DrawConnectionLines({0.0f, 0.0f, 0.0f, 0.33f});
        }

        void DrawConnectionLines(ImVec4 colorVec, LogicalID excludeID = LogicalID::empty()) const
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
                GetLightDir(),
                GetLightColor(),
                GetBgColor(),
                drawables,
                UpdSceneTex());

            // send texture to ImGui
            DrawTextureAsImGuiImage(UpdSceneTex(), RectDims(Get3DSceneRect()));

            // handle hittesting, etc.
            SetIsRenderHovered(ImGui::IsItemHovered());
            if (ImGui::IsItemHovered()) {
                ImGui::CaptureKeyboardFromApp(false);
                ImGui::CaptureMouseFromApp(false);
            }
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

        glm::vec3 const& GetLightDir() const { return m_3DSceneLightDir; }
        glm::vec3 const& GetLightColor() const { return m_3DSceneLightColor; }
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
            dt.id = LogicalID::empty();
            dt.groupId = LogicalID::empty();
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
            rv.id = meshEl.GetLogicalID();
            rv.groupId = g_MeshGroupID;
            rv.mesh = meshEl.GetMesh();
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
            rv.id = bodyEl.GetLogicalID();
            rv.groupId = g_BodyGroupID;
            rv.mesh = m_SphereMesh;
            rv.modelMatrix = SphereMeshToSceneSphereXform(SphereAtTranslation(bodyEl.GetTranslationInGround()));
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

        void AppendAsFrame(LogicalID logicalID,
                           LogicalID groupID,
                           TranslationOrientation const& translationOrientation,
                           std::vector<DrawableThing>& appendOut,
                           float alpha = 1.0f) const
        {
            // stolen from SceneGeneratorNew.cpp

            glm::vec3 origin = translationOrientation.translation;
            glm::mat3 rotation = EulerAnglesToMat(translationOrientation.orientation);

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
                dir[i] = 3.0f * GetSphereRadius();
                Segment axisline{origin, origin + rotation*dir};

                float frameAxisThickness = GetSphereRadius()/4.0f;
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
            AppendAsFrame(bodyEl.GetLogicalID(), g_BodyGroupID, bodyEl.GetTranslationOrientationInGround(), appendOut);
        }

        std::pair<LogicalID, glm::vec3> Hovertest(std::vector<DrawableThing> const& drawables) const
        {
            Rect sceneRect = Get3DSceneRect();
            glm::vec2 mousePos = ImGui::GetMousePos();

            if (!PointIsInRect(sceneRect, mousePos)) {
                return {};
            }

            glm::vec2 sceneDims = RectDims(sceneRect);
            glm::vec2 relMousePos = mousePos - sceneRect.p1;

            Line ray = GetCamera().unprojectTopLeftPosToWorldRay(relMousePos, sceneDims);
            bool hittestMeshes = IsMeshesInteractable();
            bool hittestBodies = IsBodiesInteractable();

            LogicalID closestID;
            float closestDist = std::numeric_limits<float>::max();

            for (DrawableThing const& drawable : drawables) {
                if (!drawable.id) {
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

            glm::vec3 hitPos = closestID ? ray.origin + closestDist*ray.dir : glm::vec3{};

            return {closestID, hitPos};
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

        void tick(float dt)
        {
            // pop any background-loaded meshes
            PopMeshLoader();

            // update animation percentage thing
            {
                float dotMotionsPerSecond = 0.35f;
                float ignoreMe;
                m_AnimationPercentage = std::modf(m_AnimationPercentage + std::modf(dotMotionsPerSecond * dt, &ignoreMe), &ignoreMe);
            }

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
            glm::vec4 ground = {0.0f, 0.0f, 1.0f, 1.0f};
            glm::vec4 body = {1.0f, 0.0f, 0.0f, 1.0f};
        } m_Colors;

        glm::vec3 m_3DSceneLightDir = {-0.34f, -0.25f, 0.05f};
        glm::vec3 m_3DSceneLightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 m_3DSceneBgColor = {0.89f, 0.89f, 0.89f, 1.0f};

        // senteniel ID for ground: used for some hittests
        LogicalID m_GroundID = LogicalID::next();

        // scale factor for all non-mesh, non-overlay scene elements (e.g.
        // the floor, bodies)
        //
        // this is necessary because some meshes can be extremely small/large and
        // scene elements need to be scaled accordingly (e.g. without this, a body
        // sphere end up being much larger than a mesh instance). Imagine if the
        // mesh was the leg of a fly
        float m_SceneScaleFactor = 1.0f;

        // looping animation X
        float m_AnimationPercentage = 0.0f;

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
        AssignMeshMWState(SharedData& sharedData, LogicalIDT<MeshEl> meshID) :
            MWState{sharedData},
            m_MeshID{meshID}
        {
        }

        LogicalID GetHoverID() const
        {
            return m_MaybeHover.first;
        }

        bool IsHoveringSomething() const
        {
            return GetHoverID();
        }

        bool IsHoveringMesh() const
        {
            return GetHoverID() == m_MeshID;
        }

        bool IsHoveringGround() const
        {
            return GetHoverID() == g_GroundID;
        }

        bool IsHoveringABody() const
        {
            return IsHoveringSomething() && !IsHoveringMesh() && !IsHoveringGround();
        }

        void ClearHover()
        {
            m_MaybeHover.first = LogicalID::empty();
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
            if (!IsHoveringSomething() || !IsHoveringMesh()) {
                return;
            }

            // user is hovering the mesh being assigned
            MeshEl const& meshEl = m_SharedData.GetCurrentModelGraph().GetMeshByIDOrThrow(m_MeshID);

            ImGui::BeginTooltip();
            ImGui::TextUnformatted(meshEl.GetName().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(click to assign the mesh to ground)");
            ImGui::EndTooltip();
        }

        void DrawBodyHoverTooltip() const
        {
            if (!IsHoveringSomething() || !IsHoveringABody()) {
                return;
            }

            // user is hovering a body that the mesh could attach to
            BodyEl const& bodyEl = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(GetHoverID());

            ImGui::BeginTooltip();
            ImGui::TextUnformatted(bodyEl.GetName().c_str());
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
            if (!IsHoveringSomething()) {
                return;
            } else if (IsHoveringMesh()) {
                DrawMeshHoverTooltip();
            } else if (IsHoveringABody()) {
                DrawBodyHoverTooltip();
            } else if (IsHoveringGround()) {
                DrawGroundHoverTooltip();
            }
        }

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            if (!m_SharedData.IsRenderHovered()) {
                ClearHover();
                return;
            }

            m_MaybeHover = m_SharedData.Hovertest(drawables);
        }

        void DrawConnectionLines()
        {
            ImVec4 faintColor = {0.0f, 0.0f, 0.0f, 0.2f};
            ImVec4 strongColor = {0.0f, 0.0f, 0.0f, 1.0f};

            if (!IsHoveringSomething()) {
                // draw all existing connection lines faintly
                m_SharedData.DrawConnectionLines(faintColor);
                return;
            }

            // otherwise, draw all *other* connection lines faintly and then handle
            // the hovertest line drawing
            m_SharedData.DrawConnectionLines(faintColor, m_MeshID);

            // draw hover connection line strongly
            auto meshLoc = m_SharedData.GetCurrentModelGraph().GetMeshByIDOrThrow(m_MeshID).GetTranslationInGround();
            auto strongColorU32 = ImGui::ColorConvertFloat4ToU32(strongColor);
            if (IsHoveringABody()) {
                auto bodyLoc = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(GetHoverID()).GetTranslationInGround();
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(bodyLoc),
                                                m_SharedData.WorldPosToScreenPos(meshLoc));
            } else if (IsHoveringMesh() || IsHoveringGround()) {
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
            ImGui::GetForegroundDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void Draw3DViewer()
        {
            MeshEl const& meshEl = m_SharedData.GetCurrentModelGraph().GetMeshByIDOrThrow(m_MeshID);

            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            // draw mesh slightly faded and with a rim highlight
            {
                DrawableThing& meshDrawable = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                meshDrawable.color.a = 0.25f;
                meshDrawable.rimColor = 0.25f;
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
            if (IsHoveringSomething()) {

                DrawHoverTooltip();

                // handle user clicks
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {

                    if (IsHoveringMesh() || IsHoveringGround()) {
                        // user clicked on the mesh: assign mesh to ground (un-assign it)
                        m_SharedData.UpdCurrentModelGraph().UnsetMeshAttachmentPoint(m_MeshID);
                        m_SharedData.CommitCurrentModelGraph("assigned mesh to ground");
                        m_MaybeNextState = CreateStandardState(m_SharedData);  // transition to main UI
                        App::cur().requestRedraw();
                    } else if (IsHoveringABody()) {
                        // user clicked on a body: assign the mesh to the body
                        auto hoverID = GetHoverID();
                        auto bodyID = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(hoverID).GetLogicalID();

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
        LogicalIDT<MeshEl> m_MeshID;

        // (maybe) next state to transition to
        std::unique_ptr<MWState> m_MaybeNextState;

        // (maybe) hover + worldspace location of the hover
        std::pair<LogicalID, glm::vec3> m_MaybeHover;
    };

    TranslationOrientation CalcAverageTranslationOrientation(ModelGraph const& mg, LogicalID parentID, LogicalIDT<BodyEl> childID)
    {
        TranslationOrientation parentTO = (parentID && parentID != g_GroundID) ? mg.GetTranslationOrientationInGround(parentID) : GroundTranslationOrientation();
        TranslationOrientation childTO = mg.GetTranslationOrientationInGround(childID);
        return Average(parentTO, childTO);
    }

    class FreeJointFirstPivotPlacementState final : public MWState {
    public:
        FreeJointFirstPivotPlacementState(SharedData& sharedData, LogicalID parentID, LogicalIDT<BodyEl> childBodyID) :
            MWState{sharedData},
            m_MaybeNextState{nullptr},
            m_ParentID{parentID},
            m_ChildBodyID{childBodyID},
            m_PivotTranslationOrientation{CalcAverageTranslationOrientation(m_SharedData.GetCurrentModelGraph(), m_ParentID, m_ChildBodyID)},
            m_PivotID{LogicalID::next()}
        {
        }

    private:

        glm::vec3 GetParentTranslationInGround() const
        {
            if (m_ParentID && m_ParentID != g_GroundID) {
                return m_SharedData.GetCurrentModelGraph().GetTranslationInGround(m_ParentID);
            } else {
                return {};  // ground
            }
        }

        glm::vec3 GetParentOrientationInGround() const
        {
            if (m_ParentID && m_ParentID != g_GroundID) {
                return m_SharedData.GetCurrentModelGraph().GetOrientationInGround(m_ParentID);
            } else {
                return {};  // ground
            }
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
            return m_PivotTranslationOrientation.translation;
        }

        glm::vec3 const& GetPivotOrientation() const
        {
            return m_PivotTranslationOrientation.orientation;
        }

        void AddFreeJointToModelGraphFromCurrentState()
        {
            LogicalIDT<BodyEl> parentID = m_ParentID && m_ParentID != g_GroundID ? LogicalIDT<BodyEl>::downcast(m_ParentID) : LogicalIDT<BodyEl>::empty();
            JointAttachment parentAttachment{parentID, m_PivotTranslationOrientation};
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
            glm::vec2 pivotScreenPos = m_SharedData.WorldPosToScreenPos(m_PivotTranslationOrientation.translation);
            glm::vec2 parentScreenPos = m_SharedData.WorldPosToScreenPos(GetParentTranslationInGround());

            ImU32 blackColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

            m_SharedData.DrawConnectionLine(blackColor, pivotScreenPos, childScreenPos);
            m_SharedData.DrawConnectionLine(blackColor, parentScreenPos, pivotScreenPos);
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose joint location (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetForegroundDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void DrawPivot3DManipulators()
        {
            if (!ImGuizmo::IsUsing()) {
                m_ImGuizmoState.mtx = TranslationOrientationToBase(m_PivotTranslationOrientation);
            }

            Rect sceneRect = m_SharedData.Get3DSceneRect();
            ImGuizmo::SetRect(sceneRect.p1.x, sceneRect.p1.y, RectDims(sceneRect).x, RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

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
                m_PivotTranslationOrientation.orientation = EulerCompose(m_PivotTranslationOrientation.orientation, rotation);
            } else if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                m_PivotTranslationOrientation.translation += translation;
            }
        }

        void Draw3DViewer()
        {
            m_SharedData.SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing> sceneEls;

            // draw pivot point as a single frame
            m_SharedData.AppendAsFrame(m_PivotID, m_PivotID, m_PivotTranslationOrientation, sceneEls);

            float const faintAlpha = 0.1f;

            // draw other bodies faintly and non-clickable
            for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                m_SharedData.AppendAsFrame({}, {}, bodyEl.GetTranslationOrientationInGround(), sceneEls, faintAlpha);
            }

            // draw meshes faintly and non-clickable
            for (auto const& [meshID, meshEl] : m_SharedData.GetCurrentModelGraph().GetMeshes()) {
                DrawableThing& dt = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                dt.id = {};
                dt.groupId = {};
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
        LogicalID m_ParentID;

        // ID of the child-side of the FreeJoint (always a body)
        LogicalIDT<BodyEl> m_ChildBodyID;

        // translation+orientation of the FreeJoint's joint location in ground
        TranslationOrientation m_PivotTranslationOrientation;

        // senteniel ID for the joint location, so that hittesting can "see" it
        LogicalID m_PivotID;

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
        PinJointPivotPlacementState(SharedData& sharedData, LogicalID parentID, LogicalIDT<BodyEl> childBodyID) :
            MWState{sharedData},
            m_MaybeNextState{nullptr},
            m_ParentID{parentID},
            m_ChildBodyID{childBodyID},
            m_PivotTranslationOrientation{CalcAverageTranslationOrientation(sharedData.GetCurrentModelGraph(), parentID, childBodyID)},
            m_PivotID{LogicalID::next()}
        {
        }

    private:

        glm::vec3 GetParentTranslationInGround() const
        {
            if (m_ParentID && m_ParentID != g_GroundID) {
                return m_SharedData.GetCurrentModelGraph().GetTranslationInGround(m_ParentID);
            } else {
                return {};  // ground
            }
        }

        glm::vec3 GetParentOrientationInGround() const
        {
            if (m_ParentID && m_ParentID != g_GroundID) {
                return m_SharedData.GetCurrentModelGraph().GetOrientationInGround(m_ParentID);
            } else {
                return {};  // ground
            }
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
            return m_PivotTranslationOrientation.translation;
        }

        glm::vec3 const& GetPivotOrientation() const
        {
            return m_PivotTranslationOrientation.orientation;
        }

        void CreatePinjointFromCurrentState()
        {
            LogicalIDT<BodyEl> parentID = m_ParentID && m_ParentID != g_GroundID ? LogicalIDT<BodyEl>::downcast(m_ParentID) : LogicalIDT<BodyEl>::empty();
            JointAttachment parentAttachment{parentID, m_PivotTranslationOrientation};
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
            glm::vec2 pivotScreenPos = m_SharedData.WorldPosToScreenPos(m_PivotTranslationOrientation.translation);
            glm::vec2 parentScreenPos = m_SharedData.WorldPosToScreenPos(GetParentTranslationInGround());

            ImU32 blackColor = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});

            m_SharedData.DrawConnectionLine(blackColor, pivotScreenPos, childScreenPos);
            m_SharedData.DrawConnectionLine(blackColor, parentScreenPos, pivotScreenPos);
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose pivot location + orientation (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetForegroundDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void DrawPivot3DManipulators()
        {
            if (!ImGuizmo::IsUsing()) {
                m_ImGuizmoState.mtx = TranslationOrientationToBase(m_PivotTranslationOrientation);
            }

            Rect sceneRect = m_SharedData.Get3DSceneRect();
            ImGuizmo::SetRect(sceneRect.p1.x, sceneRect.p1.y, RectDims(sceneRect).x, RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

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
                m_PivotTranslationOrientation.orientation = EulerCompose(m_PivotTranslationOrientation.orientation, rotation);
            } else if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE) {
                m_PivotTranslationOrientation.translation += translation;
            }
        }

        float GetAngleBetweenParentAndChild() const
        {
            glm::vec3 pivot = m_PivotTranslationOrientation.translation;
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
            m_SharedData.AppendAsFrame(m_PivotID, m_PivotID, m_PivotTranslationOrientation, sceneEls);

            float const faintAlpha = 0.1f;

            // draw other bodies faintly and non-clickable
            for (auto const& [bodyID, bodyEl] : m_SharedData.GetCurrentModelGraph().GetBodies()) {
                m_SharedData.AppendAsFrame({}, {}, bodyEl.GetTranslationOrientationInGround(), sceneEls, faintAlpha);
            }

            // draw meshes faintly and non-clickable
            for (auto const& [meshID, meshEl] : m_SharedData.GetCurrentModelGraph().GetMeshes()) {
                DrawableThing& dt = sceneEls.emplace_back(m_SharedData.GenerateMeshElDrawable(meshEl, m_SharedData.GetMeshColor()));
                dt.id = {};
                dt.groupId = {};
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
                glm::vec3 v = glm::degrees(m_PivotTranslationOrientation.orientation);
                if (ImGui::InputFloat3("orientation in ground", glm::value_ptr(v))) {
                    m_PivotTranslationOrientation.orientation = glm::radians(v);
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
                m_PivotTranslationOrientation.orientation = MatToEulerAngles(m);
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
                m_PivotTranslationOrientation.orientation = MatToEulerAngles(m);
            }

            if (ImGui::Button("Reverse Z by rotating X")) {
                m_PivotTranslationOrientation.orientation = EulerCompose({fpi, 0.0f, 0.0f}, m_PivotTranslationOrientation.orientation);
            }

            if (ImGui::Button("Reverse Z by rotating Y")) {
                m_PivotTranslationOrientation.orientation = EulerCompose({0.0f, fpi, 0.0f}, m_PivotTranslationOrientation.orientation);
            }

            if (ImGui::Button("Use parent's orientation")) {
                m_PivotTranslationOrientation.orientation = GetParentOrientationInGround();
            }

            if (ImGui::Button("Use child's orientation")) {
                m_PivotTranslationOrientation.orientation = GetChildOrientationInGround();
            }

            if (ImGui::Button("Use parent's and child's orientation (average)")) {
                glm::vec3 parentOrientation = GetParentOrientationInGround();
                glm::vec3 childOrientation = GetChildOrientationInGround();
                m_PivotTranslationOrientation.orientation = (parentOrientation+childOrientation)/2.0f;
            }

            if (ImGui::Button("Orient along global axes")) {
                m_PivotTranslationOrientation.orientation = {};
            }

            ImGui::Dummy({0.0f, 10.0f});
            ImGui::Text("translation tools:");
            ImGui::Dummy({0.0f, 5.0f});

            ImGui::InputFloat3("translation in ground", glm::value_ptr(m_PivotTranslationOrientation.translation));

            if (ImGui::Button("Use parent's translation")) {
                m_PivotTranslationOrientation.translation = GetParentTranslationInGround();
            }

            if (ImGui::Button("Use child's translation")) {
                m_PivotTranslationOrientation.translation = GetChildTranslationInGround();
            }

            if (ImGui::Button("Use midpoint translation ((parent+child)/2)")) {
                glm::vec3 parentTranslation = GetParentTranslationInGround();
                glm::vec3 childTranslation = GetChildTranslationInGround();
                m_PivotTranslationOrientation.translation = (parentTranslation+childTranslation)/2.0f;
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
        LogicalID m_ParentID;

        // ID of the child-side of the joint (always a body)
        LogicalIDT<BodyEl> m_ChildBodyID;

        // translation+orientation of the pinjoint's pivot
        TranslationOrientation m_PivotTranslationOrientation;

        // senteniel ID for the pivot, so that hittesting can "see" it
        LogicalID m_PivotID;

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
        JointAssignmentStep1State(SharedData& sharedData, LogicalIDT<BodyEl> childBodyID) :
            MWState{sharedData},
            m_ChildBodyID{childBodyID}
        {
        }

    private:
        LogicalID GetHoverID() const
        {
            return m_MaybeHover.first;
        }

        bool IsHoveringSomething() const
        {
            return GetHoverID();
        }

        bool IsHoveringGround() const
        {
            return GetHoverID() == g_GroundID;
        }

        bool IsHoveringChildBody() const
        {
            return GetHoverID() == m_ChildBodyID;
        }

        bool IsHoveringAnotherBody() const
        {
            return IsHoveringSomething() && !IsHoveringGround() && !IsHoveringChildBody();
        }

        void ClearHover()
        {
            m_MaybeHover.first = LogicalID::empty();
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
            if (!IsHoveringChildBody()) {
                return;
            }

            BodyEl const& body = GetChildBodyEl();

            ImGui::BeginTooltip();
            ImGui::Text("%s", body.GetName().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(you cannot join bodies to themselves)");
            ImGui::EndTooltip();
        }

        void DrawGroundHoverTooltip() const
        {
            if (!IsHoveringGround()) {
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

            BodyEl const& otherBody = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(GetHoverID());

            ImGui::BeginTooltip();
            ImGui::Text("%s", otherBody.GetName().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(click to join)");
            ImGui::EndTooltip();
        }

        void DrawHoverTooltip() const
        {
            if (!IsHoveringSomething()) {
                return;
            } else if (IsHoveringChildBody()) {
                DrawChildHoverTooltip();
            } else if (IsHoveringGround()) {
                DrawGroundHoverTooltip();
            } else if (IsHoveringAnotherBody()) {
                DrawOtherBodyTooltip();
            }
        }

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            if (!m_SharedData.IsRenderHovered()) {
                ClearHover();
                return;
            }

            m_MaybeHover = m_SharedData.Hovertest(drawables);
        }

        void DrawHeaderText() const
        {
            char const* const text = "choose a body (or ground) to join to (ESC to cancel)";
            ImU32 color = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f});
            ImGui::GetForegroundDrawList()->AddText({10.0f, 10.0f}, color, text);
        }

        void DrawConnectionLines() const
        {
            ImVec4 faintColor = {0.0f, 0.0f, 0.0f, 0.2f};
            ImVec4 strongColor = {0.0f, 0.0f, 0.0f, 1.0f};
            auto strongColorU32 = ImGui::ColorConvertFloat4ToU32(strongColor);

            if (!IsHoveringSomething()) {
                // draw all existing connection lines faintly
                m_SharedData.DrawConnectionLines(faintColor);
                return;
            }

            // otherwise, draw all *other* connection lines faintly and then handle
            // the hovertest line drawing
            m_SharedData.DrawConnectionLines(faintColor, m_ChildBodyID);

            glm::vec3 const& childBodyLoc = GetChildBodyEl().GetTranslationInGround();

            if (IsHoveringAnotherBody()) {
                glm::vec3 const& otherBodyLoc = m_SharedData.GetCurrentModelGraph().GetBodyByIDOrThrow(GetHoverID()).GetTranslationInGround();
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(otherBodyLoc),
                                                m_SharedData.WorldPosToScreenPos(childBodyLoc));
            } else if (IsHoveringGround()) {
                glm::vec3 groundLoc = {0.0f, 0.0f, 0.0f};
                m_SharedData.DrawConnectionLine(strongColorU32,
                                                m_SharedData.WorldPosToScreenPos(groundLoc),
                                                m_SharedData.WorldPosToScreenPos(childBodyLoc));
            }
        }

        void HandleHovertestSideEffects()
        {
            if (!IsHoveringSomething()) {
                return;
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // user has clicked what they want, so present them with the joint type
                // popup modal

                m_MaybeUserParentChoice = GetHoverID();
                ImGui::OpenPopup(m_JointTypePopupName);
                App::cur().requestRedraw();
            }
        }

        void DrawJointTypeSelectionPopupIfUserHasSelectedSomething()
        {
            if (!m_MaybeUserParentChoice) {
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
                m_MaybeNextState = std::make_unique<PinJointPivotPlacementState>(m_SharedData, m_MaybeUserParentChoice, m_ChildBodyID);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("FreeJoint")) {
                m_MaybeNextState = std::make_unique<FreeJointFirstPivotPlacementState>(m_SharedData, m_MaybeUserParentChoice, m_ChildBodyID);
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
                dt.id = LogicalID::empty();
                dt.groupId = LogicalID::empty();
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
        LogicalIDT<BodyEl> m_ChildBodyID;

        // (maybe) next state to transition to
        std::unique_ptr<MWState> m_MaybeNextState;

        // (maybe) hover + worldspace location of the user's mouse hover
        std::pair<LogicalID, glm::vec3> m_MaybeHover;

        // (maybe) the body/ground the user selected
        LogicalID m_MaybeUserParentChoice = LogicalID::empty();

        // name of the ImGui popup that lets the user select a joint type
        char const* m_JointTypePopupName = "select joint type";
    };


    // "standard" UI state
    //
    // this is what the user is typically interacting with when the UI loads
    class StandardMWState final : public MWState {
    public:

        StandardMWState(SharedData& sharedData) :
            MWState{sharedData},
            m_MaybeHover{},
            m_MaybeOpenedContextMenu{}
        {
        }

        LogicalID HoverID() const
        {
            return m_MaybeHover.first;
        }

        glm::vec3 HoverPos() const
        {
            return m_MaybeHover.second;
        }

        bool HasHover() const
        {
            return !HoverID().IsEmpty();
        }

        bool IsHovered(LogicalID id) const
        {
            return m_MaybeHover.first == id;
        }

        void ClearHover()
        {
            m_MaybeHover.first = LogicalID::empty();
        }

        void SelectHover()
        {
            LogicalID maybeHoverID = m_MaybeHover.first;

            if (!maybeHoverID) {
                return;
            }

            m_SharedData.Select(maybeHoverID);
        }

        float RimIntensity(LogicalID id)
        {
            if (!id) {
                return 0.0f;
            } else if (m_SharedData.IsSelected(id)) {
                return 1.0f;
            } else if (IsHovered(id)) {
                return 0.5f;
            } else {
                return 0.0f;
            }
        }

        void AddBodyToHoveredElement()
        {
            if (!HasHover()) {
                return;
            }

            m_SharedData.AddBody(HoverPos());
        }

        void TransitionToAssigningMeshNextFrame(MeshEl const& meshEl)
        {
            // request a state transition
            m_MaybeNextState = std::make_unique<AssignMeshMWState>(m_SharedData, meshEl.GetLogicalID());
        }

        void TryTransitionToAssigningHoveredMeshNextFrame()
        {
            if (!HasHover()) {
                return;
            }

            MeshEl const* maybeMesh = m_SharedData.UpdCurrentModelGraph().TryGetMeshElByID(HoverID());

            if (!maybeMesh) {
                return;  // not hovering a mesh
            }

            TransitionToAssigningMeshNextFrame(*maybeMesh);
        }

        void OpenHoverContextMenu()
        {
            if (!HasHover()) {
                return;
            }

            m_MaybeOpenedContextMenu = m_MaybeHover;
            ImGui::OpenPopup(m_ContextMenuName);
            App::cur().requestRedraw();
        }

        void UpdateFromImGuiKeyboardState()
        {
            // DELETE/BACKSPACE: delete any selected elements
            if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE) || ImGui::IsKeyPressed(SDL_SCANCODE_BACKSPACE)) {
                m_SharedData.DeleteSelected();
            }

            // B: add body to hovered element
            if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
                AddBodyToHoveredElement();
            }

            // A: assign a parent for the hovered element
            if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                TryTransitionToAssigningHoveredMeshNextFrame();
            }

            // S: set manipulation mode to "scale"
            if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
                if (m_ImGuizmoState.op == ImGuizmo::SCALE) {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::SCALE;
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

            // modified keys

            bool superPressed = ImGui::IsKeyDown(SDL_SCANCODE_LGUI) || ImGui::IsKeyDown(SDL_SCANCODE_RGUI);
            bool ctrlPressed = ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL);
            bool shiftPressed = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            bool ctrlOrSuperPressed =  superPressed || ctrlPressed;

            // Ctrl+A: select all
            if (ctrlOrSuperPressed && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                m_SharedData.SelectAll();
            }

            // Ctrl+Z: undo
            if (ctrlOrSuperPressed && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                m_SharedData.UndoCurrentModelGraph();
            }

            // Ctrl+Shift+Z: redo
            if (ctrlOrSuperPressed && shiftPressed && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                m_SharedData.RedoCurrentModelGraph();
            }
        }

        void DrawBodyContextMenu(BodyEl const& bodyEl)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                if (ImGui::MenuItem("focus camera on this")) {
                    m_SharedData.FocusCameraOn(bodyEl.GetTranslationInGround());
                }

                if (ImGui::MenuItem("join to")) {
                    m_MaybeNextState = std::make_unique<JointAssignmentStep1State>(m_SharedData, bodyEl.GetLogicalID());
                }

                if (ImGui::MenuItem("attach mesh to this")) {
                    LogicalIDT<BodyEl> bodyID = bodyEl.GetLogicalID();
                    for (auto const& meshFile : m_SharedData.PromptUserForMeshFiles()) {
                        m_SharedData.PushMeshLoadRequest(meshFile, bodyID);
                    }
                }

                if (ImGui::MenuItem("delete")) {
                    std::string name = bodyEl.GetName();
                    m_SharedData.UpdCurrentModelGraph().DeleteBodyElByID(bodyEl.GetLogicalID());
                    ClearHover();
                    m_MaybeOpenedContextMenu = {};
                    m_SharedData.CommitCurrentModelGraph("deleted " + name);
                }

                // draw name editor
                {
                    char buf[256];
                    std::strcpy(buf, bodyEl.GetName().c_str());
                    if (ImGui::InputText("name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        m_SharedData.UpdCurrentModelGraph().SetBodyName(bodyEl.GetLogicalID(), buf);
                        m_SharedData.CommitCurrentModelGraph("changed body name");
                    }
                }

                // pos editor
                {
                    glm::vec3 translation = bodyEl.GetTranslationInGround();
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        TranslationOrientation to = bodyEl.GetTranslationOrientationInGround();
                        to.translation = translation;
                        m_SharedData.UpdCurrentModelGraph().SetBodyTranslationOrientationInGround(bodyEl.GetLogicalID(), to);
                        m_SharedData.CommitCurrentModelGraph("changed body translation");
                    }
                }

                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(bodyEl.GetOrientationInGround());
                    if (ImGui::InputFloat3("orientation", glm::value_ptr(orientationDegrees), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        TranslationOrientation to = bodyEl.GetTranslationOrientationInGround();
                        to.orientation = glm::radians(orientationDegrees);
                        m_SharedData.UpdCurrentModelGraph().SetBodyTranslationOrientationInGround(bodyEl.GetLogicalID(), to);
                        m_SharedData.CommitCurrentModelGraph("changed body orientation");
                    }
                }

            }
            ImGui::EndPopup();
        }

        void DrawMeshContextMenu(MeshEl const& meshEl, glm::vec3 const& clickPos)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {

                if (ImGui::MenuItem("add body at click location")) {
                    m_SharedData.AddBody(clickPos);
                }

                if (ImGui::MenuItem("add body at mesh origin")) {
                    m_SharedData.AddBody(meshEl.GetTranslationInGround());
                }

                if (ImGui::MenuItem("add body at mesh bounds center")) {
                    m_SharedData.AddBody(AABBCenter(GetGroundspaceBounds(*meshEl.GetMesh(), GetModelMatrix(meshEl))));
                }

                if (ImGui::MenuItem("focus camera on this")) {
                    m_SharedData.FocusCameraOn(meshEl.GetTranslationInGround());
                }

                if (ImGui::MenuItem("assign to body")) {
                    TransitionToAssigningMeshNextFrame(meshEl);
                }

                if (ImGui::MenuItem("delete")) {
                    std::string name = meshEl.GetName();
                    m_SharedData.UpdCurrentModelGraph().DeleteMeshElByID(meshEl.GetLogicalID());
                    m_SharedData.CommitCurrentModelGraph("deleted " + name);
                }

                ImGui::Dummy({0.0f, 5.0f});
                ImGui::Separator();
                ImGui::Dummy({0.0f, 5.0f});

                // draw name editor
                {
                    char buf[256];
                    std::strcpy(buf, meshEl.GetName().c_str());
                    if (ImGui::InputText("name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        m_SharedData.UpdCurrentModelGraph().SetMeshName(meshEl.GetLogicalID(), buf);
                        m_SharedData.CommitCurrentModelGraph("changed mesh name");
                    }
                }

                // pos editor
                {
                    glm::vec3 translation = meshEl.GetTranslationInGround();
                    if (ImGui::InputFloat3("translation", glm::value_ptr(translation), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        TranslationOrientation to = meshEl.GetTranslationOrientationInGround();
                        to.translation = translation;
                        m_SharedData.UpdCurrentModelGraph().SetMeshTranslationOrientationInGround(meshEl.GetLogicalID(), to);
                        m_SharedData.CommitCurrentModelGraph("changed mesh translation");
                    }
                }

                // rotation editor
                {
                    glm::vec3 orientationDegrees = glm::degrees(meshEl.GetOrientationInGround());
                    if (ImGui::InputFloat3("orientation", glm::value_ptr(orientationDegrees), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        TranslationOrientation to = meshEl.GetTranslationOrientationInGround();
                        to.orientation = glm::radians(orientationDegrees);
                        m_SharedData.UpdCurrentModelGraph().SetMeshTranslationOrientationInGround(meshEl.GetLogicalID(), to);
                        m_SharedData.CommitCurrentModelGraph("changed mesh orientation");
                    }
                }

                // scale factor editor
                {
                    glm::vec3 scaleFactors = meshEl.GetScaleFactors();
                    if (ImGui::InputFloat3("scale factors", glm::value_ptr(scaleFactors), "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                        m_SharedData.UpdCurrentModelGraph().SetMeshScaleFactors(meshEl.GetLogicalID(), scaleFactors);
                        m_SharedData.CommitCurrentModelGraph("changed mesh scale factors");
                    }
                }

            }
            ImGui::EndPopup();
        }

        void DrawJointContextMenu(JointEl const& jointEl)
        {
            if (ImGui::BeginPopup(m_ContextMenuName)) {
                if (ImGui::MenuItem("do joint stuff")) {
                }
            }
            ImGui::EndPopup();
        }

        void DrawContextMenu()
        {
            if (!m_MaybeOpenedContextMenu.first) {
                return;
            }

            auto id = m_MaybeOpenedContextMenu.first;
            ModelGraph const& mg = m_SharedData.GetCurrentModelGraph();

            if (MeshEl const* meshEl = mg.TryGetMeshElByID(id)) {
                DrawMeshContextMenu(*meshEl, m_MaybeOpenedContextMenu.second);
            } else if (BodyEl const* bodyEl = mg.TryGetBodyElByID(id)) {
                DrawBodyContextMenu(*bodyEl);
            } else if (JointEl const* jointEl = mg.TryGetJointElByID(id)) {
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

        void DrawMeshHoverTooltip(MeshEl const& meshEl)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Imported Mesh");
            ImGui::Indent();
            ImGui::Text("Name = %s", meshEl.GetName().c_str());
            ImGui::Text("Filename = %s", meshEl.GetPath().filename().string().c_str());

            auto pos = GetAABBCenterPointInGround(meshEl);
            ImGui::Text("Center = (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
            ImGui::Unindent();
            ImGui::EndTooltip();
        }

        void DrawBodyHoverTooltip(BodyEl const& bodyEl)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Body");
            ImGui::Indent();
            ImGui::Text("Name = %s", bodyEl.GetName().c_str());
            ImGui::Text("Pos = %s", PosString(bodyEl.GetTranslationInGround()).c_str());
            ImGui::Text("Orientation = %s", OrientationString(bodyEl.GetOrientationInGround()).c_str());
            ImGui::Unindent();
            ImGui::EndTooltip();
        }

        void DrawJointHoverTooltip(JointEl const& jointEl)
        {
            ImGui::BeginTooltip();
            ImGui::Text("joint");
            ImGui::EndTooltip();
        }

        void DrawHoverTooltip()
        {
            if (!HasHover()) {
                return;
            }

            LogicalID hoverID = HoverID();
            ModelGraph const& mg = m_SharedData.GetCurrentModelGraph();

            if (MeshEl const* meshEl = mg.TryGetMeshElByID(hoverID)) {
                DrawMeshHoverTooltip(*meshEl);
            } else if (BodyEl const* bodyEl = mg.TryGetBodyElByID(hoverID)) {
                DrawBodyHoverTooltip(*bodyEl);
            } else if (JointEl const* jointEl = mg.TryGetJointElByID(hoverID)) {
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
            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

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

            for (LogicalID id : m_SharedData.GetCurrentSelection()) {
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

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            if (!m_SharedData.IsRenderHovered() || ImGuizmo::IsUsing()) {
                ClearHover();
                return;
            }

            m_MaybeHover = m_SharedData.Hovertest(drawables);

            bool lcReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            bool rcReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
            bool shiftDown = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (!HasHover() && lcReleased && !isUsingGizmo && !shiftDown) {
                m_SharedData.DeSelectAll();
                return;
            }

            if (HasHover() && lcReleased && !isUsingGizmo) {
                if (!shiftDown) {
                    m_SharedData.DeSelectAll();
                }
                SelectHover();
                return;
            }

            if (HasHover() && rcReleased && !isUsingGizmo) {
                OpenHoverContextMenu();
            }

            if (HasHover() && !ImGui::IsPopupOpen(m_ContextMenuName)) {
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

            for (auto const& [jointID, jointEl] : m_SharedData.GetCurrentModelGraph().GetJoints()) {
                // first pivot
                m_SharedData.AppendAsFrame(jointID, {}, jointEl.GetParentAttachmentTranslationOrientationInGround(), sceneEls);
                m_SharedData.AppendAsFrame(jointID, {}, jointEl.GetChildAttachmentTranslationOrientationInGround(), sceneEls);
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
            if (m_SharedData.IsRenderHovered()) {
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
        std::pair<LogicalID, glm::vec3> m_MaybeHover;

        // (maybe) the scene element the user opened a context menu for
        std::pair<LogicalID, glm::vec3> m_MaybeOpenedContextMenu;

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

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen() :
    m_Impl{new Impl{}}
{
}

osc::MeshesToModelWizardScreen::MeshesToModelWizardScreen(std::vector<std::filesystem::path> paths) :
    m_Impl{new Impl{paths}}
{
}

osc::MeshesToModelWizardScreen::~MeshesToModelWizardScreen() noexcept
{
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
