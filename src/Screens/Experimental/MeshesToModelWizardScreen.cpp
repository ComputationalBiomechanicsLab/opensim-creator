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

    std::string PosString(glm::vec3 const& pos)
    {
        std::stringstream ss;
        ss.precision(4);
        ss << '(' << pos.x << ", " << pos.y << ", " << pos.z << ')';
        return std::move(ss).str();
    }

    std::string OrientationString(glm::vec3 const& eulerAngles)
    {
        glm::vec3 degrees = glm::degrees(eulerAngles);
        std::stringstream ss;
        ss << '(' << degrees.x << ", " << degrees.y << ", " << degrees.z << ')';
        return std::move(ss).str();
    }
}

// logical ID support
//
// The model graph contains internal cross-references. E.g. a joint in the model may
// cross-reference bodies that are somewhere else in the model. Those references are
// looked up at runtime using associative lookups. Associative lookups are preferred
// over direct pointers, shared pointers, array indices, etc. here because the model
// graph can be moved in memory, copied (undo/redo), and be heavily edited by the user
// at runtime. We want the *overall* UI datastructure to have value, rather than
// reference, semantics to make handling that easier.
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

        explicit LogicalIDT(LogicalID id) : LogicalID{id} {}
    };
}

// hashing support for LogicalIDs
//
// lets them be used as associative lookup keys, etc.
namespace std {

    template<>
    struct hash<LogicalID> {
        size_t operator()(LogicalID const& id) const { return id.Unwrap(); }
    };

    template<typename T>
    struct hash<LogicalIDT<T>> {
        size_t operator()(LogicalID const& id) const { return id.Unwrap(); }
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
            return MeshLoadOKResponse{msg.PreferredAttachmentPoint, msg.Path, std::move(mesh)};
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
        glm::mat4 mtx = glm::eulerAngleXYZ(frame.orientation.x, frame.orientation.y, frame.orientation.z);
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
               LogicalIDT<BodyEl> maybeAttachedBody,  // not strictly required: mesh can also be attached to ground
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

        glm::vec3 const& GetScaleFactors() const { return m_ScaleFactors; }
        void SetScaleFactors(glm::vec3 const& newScaleFactors) { m_ScaleFactors = newScaleFactors; }

        std::shared_ptr<Mesh> const& GetMesh() const { return m_Mesh; }

        std::filesystem::path const& GetPath() const { return m_Path; }

        std::string const& GetName() const { return m_Name; }
        void SetName(std::string const& newName) { m_Name = newName; }

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

    // generates a placeholder body name
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
            m_Mass{1.0}  // required: OpenSim goes bananas if it's <= 0
        {
        }

        LogicalIDT<BodyEl> GetLogicalID() const { return m_LogicalID; }

        std::string const& GetName() const { return m_Name; }
        void SetName(std::string const& newName) { m_Name = newName; }

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
    std::string const& ComputeJointName(size_t jointTypeIndex, std::string const& maybeName)
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
    //
    // The reason the editor sticks to these different-from-OpenSim semantics is that:
    //
    // - OpenSim is too flexible. You can directly attach the two bodies to a joint, or one attachment
    //   could be a `PhysicalOffsetFrame`, or both attachments could be, or `PhysicalOffsetFrame`s could be
    //   attached in a daisy chain of `PhysicalOffsetFrame`s that eventually connect to the joint. They
    //   are all valid topologies in OpenSim.
    //
    // - That makes it difficult to design joint UX. If the user wants to join two bodies in the mesh
    //   importer, what flow should be shown?
    //
    //     + Should the mesh importer expose the ability to add `PhysicalOffsetFrame`s manually? Should it
    //       immediately (rudely) "snap" the two frames together, potentially reorienting the child, if the
    //       user didn't handle their frames correctly? That flow would *probably* piss off a designer -
    //       especially if they just spent 5 minutes freely orienting their meshes in 3D space *before* handling
    //       bodies/PoFs/joints
    //
    //     + Should the mesh importer ask "do you want an offset frame?", "where do you want it?", "do you want a
    //       second offset frame attached to the other side of the joint?", "where do you want that?", "Same
    //       location and orientation as the first offset frame?", "it looks like the position of your two
    //       offset frames are different, I'll just go ahead and snap your whole model into place if the joint
    //       doesn't offer a translation coordinate". That flow would enable a wide range of functionality in a
    //       step-by-step manner, but would *probably* piss off a designer because it's kind of like going to a
    //       restaurant where the waiter incessantly asks you what curvature of bowl you want your soup in and
    //       how many ice cubes you'd like in your diet coke.
    //
    // - The joint UX we *want* is:
    //
    //     + User right-clicks a body, clicks "add joint"
    //     + UI prompts which other body/ground to join to, user clicks one
    //     + UI asks which joint type (defaults to something sensible, like a PinJoint)
    //     + UI uses its knowledge of that joint type to ask the user for the bare-minimum amount of information:
    //
    //       + WeldJoint: pick + orient a single joint location
    //       + PinJoint: pick+orient a single joint location, rotate along pin axis to set the default angle, automatically compute default rotation coordinate based on initial separation
    //       + FreeJoint: pick + orient parent joint location, pick + orient child joint location, automatically compute default coordinates from the separation between those two
    //       +
    //       + Any other joint: pick + orient parent joint location, pick + orient child joint location, hope for the best when the OpenSim import happens
    //
    // - Doing this predictably is the difficult part. There's *a lot* of joint types and it's possible for
    //   the user to produce an arrangement of joints+offsets that cause a "snap" to happen when the modelgraph is
    //   finally imported into an OpenSim::Model. However, this constrained+simplified datastructure is here to
    //   make it easier for the UI to figure out "what it's working with"
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
    // see `JointAttachment` comment for an extremely long explanation of why it's designed this way.
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
        void SetUserAssignedName(std::string newName) { m_MaybeUserAssignedName = std::move(newName); }

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
    bool HasParent(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetParentBodyID() == body.GetLogicalID();
    }

    // returns `true` if `body` is `joint`'s child
    bool HasChild(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetChildBodyID() == body.GetLogicalID();
    }

    // returns `true` if `body` is the child attachment of `joint`
    bool IsAttachedToAsChild(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetChildBodyID() == body.GetLogicalID();
    }

    // returns `true` if `body` is the parent attachment of `joint`
    bool IsAttachedToAsParent(JointEl const& joint, BodyEl const& body)
    {
        return joint.GetParentBodyID() == body.GetLogicalID();
    }

    // returns `true` if body is either the parent or the child attachment of `joint`
    bool IsAttachedTo(JointEl const& joint, BodyEl const& body)
    {
        return IsAttachedToAsChild(joint, body) || IsAttachedToAsParent(joint, body);
    }

    // returns `true` is a joint's parent is ground
    bool IsAttachedToGroundAsParent(JointEl const& joint)
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

        void SetMeshTranslationOrientationInGround(LogicalIDT<MeshEl> meshID, TranslationOrientation const& newTO)
        {
            UpdMeshByIDOrThrow(meshID).SetTranslationOrientationInGround(newTO);
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
            auto it = m_Meshes.find(LogicalIDT<MeshEl>{id});
            if (it != m_Meshes.end()) {
                DeleteMesh(it);
            }
        }

        void DeleteBodyElByID(LogicalID id)
        {
            auto it = m_Bodies.find(LogicalIDT<BodyEl>{id});
            if (it != m_Bodies.end()) {
                DeleteBody(it);
            }
        }

        void DeleteJointElByID(LogicalID id)
        {
            auto it = m_Joints.find(LogicalIDT<JointEl>{id});
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

    private:
        MeshEl* TryUpdMeshElByID(LogicalID id)
        {
            auto it = m_Meshes.find(LogicalIDT<MeshEl>{id});
            return it != m_Meshes.end() ? &it->second : nullptr;
        }

        BodyEl* TryUpdBodyElByID(LogicalID id)
        {
            auto it = m_Bodies.find(LogicalIDT<BodyEl>{id});
            return it != m_Bodies.end() ? &it->second : nullptr;
        }

        JointEl* TryUpdJointElByID(LogicalID id)
        {
            auto it = m_Joints.find(LogicalIDT<JointEl>{id});
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
            m_Meshes.erase(it);
        }

        void DeleteBody(std::unordered_map<LogicalIDT<BodyEl>, BodyEl>::iterator it)
        {
            auto const& [bodyID, bodyEl] = *it;

            // delete any joints that reference the body
            for (auto jointIt = m_Joints.begin(); jointIt != m_Joints.end(); ++jointIt) {
                if (IsAttachedTo(jointIt->second, bodyEl)) {
                    jointIt = m_Joints.erase(jointIt);
                }
            }

            // delete any meshes attached to the body
            for (auto meshIt = m_Meshes.begin(); meshIt != m_Meshes.end(); ++meshIt) {
                if (meshIt->second.GetAttachmentPoint() == bodyID) {
                    meshIt = m_Meshes.erase(meshIt);
                }
            }

            // delete the body
            m_Bodies.erase(it);
        }

        void DeleteJoint(std::unordered_map<LogicalIDT<JointEl>, JointEl>::iterator it)
        {
            m_Joints.erase(it);
        }

        std::unordered_map<LogicalIDT<MeshEl>, MeshEl> m_Meshes;
        std::unordered_map<LogicalIDT<BodyEl>, BodyEl> m_Bodies;
        std::unordered_map<LogicalIDT<JointEl>, JointEl> m_Joints;
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
    bool IsChildAttachmentOfAnyJoint(ModelGraph const& modelGraph, BodyEl const& body)
    {
        return AnyOf(modelGraph.GetJoints(), [&](auto const& pair) { return IsAttachedToAsChild(pair.second, body); });
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
                                std::unordered_set<LogicalID>& previousVisits);

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

        auto [it, wasInserted] = previousVisits.emplace(parent->GetLogicalID());

        if (!wasInserted) {
            return false;  // cycle detected
        }

        return IsBodyAttachedToGround(modelGraph, *parent, previousVisits);
    }

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
                                BodyEl const& body,
                                std::unordered_set<LogicalID>& previousVisits)
    {
        for (auto const& [id, joint] : modelGraph.GetJoints()) {
            OSC_ASSERT_ALWAYS(!IsGarbageJoint(modelGraph, joint));

            if (HasChild(joint, body)) {
                // body participates as a child in a joint - check if the joint indirectly connects to ground
                return IsJointAttachedToGround(modelGraph, joint, previousVisits);
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

        std::unordered_set<LogicalID> previousVisits;

        for (auto const& [id, body] : modelGraph.GetBodies()) {
            if (!IsBodyAttachedToGround(modelGraph, body, previousVisits)) {
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

        TranslationOrientation meshInGround = meshEl.GetTranslationOrientationInGround();

        // re-express the transform matrix in the parent's frame
        glm::mat4 parent2ground = TranslationOrientationToBase(parentTranslationAndOrientationInGround);
        glm::mat4 ground2parent = glm::inverse(parent2ground);
        glm::mat4 mesh2ground = TranslationOrientationToBase(meshEl.GetTranslationOrientationInGround());
        glm::mat4 mesh2parent = ground2parent * mesh2ground;

        // set it as the transform
        meshPhysOffsetFrame->setOffsetTransform(SimTKTransformFromMat4x3(mesh2parent));

        // attach mesh to the POF
        auto mesh = std::make_unique<OpenSim::Mesh>(meshEl.GetPath());
        mesh->setName(meshEl.GetName());
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
    JointAttachmentCachedLookupResult lookupPF(ModelGraph const& mg,
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
    std::string ComputeJointName(JointEl const& jointEl,
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

        glm::vec3 orientation;
        glm::extractEulerAngleXYZ(child2parent, orientation.x, orientation.y, orientation.z);

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
        return std::fabs(v) < 1e-7;
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
        JointAttachmentCachedLookupResult parent = lookupPF(mg, model, visitedBodies, joint.GetParentBodyID());
        JointAttachmentCachedLookupResult child = lookupPF(mg, model, visitedBodies, joint.GetChildBodyID());

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
        jointUniqPtr->setName(ComputeJointName(joint, *parent.physicalFrame, *child.physicalFrame));

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
            if (IsAttachedToAsParent(otherJoint, *child.bodyEl)) {
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
            return nullptr;
        }

        // create the output model
        auto model = std::make_unique<OpenSim::Model>();

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
            if (!IsChildAttachmentOfAnyJoint(mg, body)) {
                AttachBodyDirectlyToGround(mg, *model, body, visitedBodies);
            }
        }

        // add bodies that do participate in joints into the model
        //
        // note: these bodies may use the non-participating bodies (above) as parents
        for (auto const& [jointID, joint] : mg.GetJoints()) {

            auto parentID = joint.GetParentBodyID();

            if (IsAttachedToGroundAsParent(joint) || ContainsKey(visitedBodies, parentID)) {
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

        void CommitCurrent(std::string_view commitMessage)
        {
            m_Snapshots.emplace_back(m_Current, commitMessage);
            m_CurrentIsBasedOn = m_Snapshots.size() - 1;
        }

        std::vector<ModelGraphSnapshot> const& GetSnapshots() const { return m_Snapshots; }

        size_t GetCurrentIsBasedOn() const { return m_CurrentIsBasedOn; }

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
            m_CurrentIsBasedOn = m_CurrentIsBasedOn == 0 ? 0 : m_CurrentIsBasedOn-1;
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

            m_CurrentIsBasedOn = m_CurrentIsBasedOn == lastSnapshot ? lastSnapshot : m_CurrentIsBasedOn+1;
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

    LogicalID const g_BodyGroupID = LogicalID::next();
    LogicalID const g_MeshGroupID = LogicalID::next();

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
        return std::tie(b.color.a, b.mesh) < std::tie(a.color.a, a.mesh);
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

        GouraudShader& shader = App::cur().getShaderCache().getShader<GouraudShader>();

        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, projMat);
        gl::Uniform(shader.uViewMat, viewMat);
        gl::Uniform(shader.uLightDir, lightDir);
        gl::Uniform(shader.uLightColor, lightCol);
        gl::Uniform(shader.uViewPos, viewPos);
        gl::Uniform(shader.uIsTextured, false);
        for (auto const& d : drawables) {
            gl::Uniform(shader.uModelMat, d.modelMatrix);
            gl::Uniform(shader.uNormalMat, d.normalMatrix);
            gl::Uniform(shader.uDiffuseColor, d.color);
            if (d.maybeDiffuseTex) {
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(*d.maybeDiffuseTex);
                gl::Uniform(shader.uIsTextured, true);
                gl::Uniform(shader.uSampler0, GL_TEXTURE0);
            } else {
                gl::Uniform(shader.uIsTextured, false);
            }
            gl::BindVertexArray(d.mesh->GetVertexArray());
            d.mesh->Draw();
            gl::BindVertexArray();
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
            gl::Uniform(eds.uSampler0, GL_TEXTURE0);
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

    class SharedData final {
    public:
        SharedData() = default;

        SharedData(std::vector<std::filesystem::path> meshFiles)
        {
            for (auto const& meshFile : meshFiles) {
                PushMeshLoadRequest(meshFile);
            }
        }

        bool HasOutputModel() const { return m_MaybeOutputModel.get() != nullptr; }
        std::unique_ptr<OpenSim::Model>& UpdOutputModel() { return m_MaybeOutputModel; }

        ModelGraph const& GetCurrentModelGraph() const { return m_ModelGraphSnapshots.Current(); }
        ModelGraph& UpdCurrentModelGraph() { return m_ModelGraphSnapshots.Current(); }
        void CommitCurrentModelGraph(std::string_view commitMsg) { m_ModelGraphSnapshots.CommitCurrent(commitMsg); }
        std::vector<ModelGraphSnapshot> const& GetModelGraphSnapshots() const { return m_ModelGraphSnapshots.GetSnapshots(); }
        size_t GetModelGraphIsBasedOn() const { return m_ModelGraphSnapshots.GetCurrentIsBasedOn(); }
        void UseModelGraphSnapshot(size_t i) { m_ModelGraphSnapshots.UseSnapshot(i); }
        bool CanUndoCurrentModelGraph() const { return m_ModelGraphSnapshots.CanUndo(); }
        void UndoCurrentModelGraph() { m_ModelGraphSnapshots.Undo(); }
        bool CanRedoCurrentModelGraph() const { return m_ModelGraphSnapshots.CanRedo(); }
        void RedoCurrentModelGraph() { m_ModelGraphSnapshots.Redo(); }

        // pushes a mesh file load request to the background mesh loader
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

        void PromptUserForMeshFilesAndPushThemOntoMeshLoader()
        {
            for (auto const& meshFile : PromptUserForFiles("obj,vtp,stl")) {
                PushMeshLoadRequest(meshFile);
            }
        }

        float GetAnimationPercentage() const { return m_AnimationPercentage; }
        void UpdateAnimationPercentage(float dt)
        {
            float dotMotionsPerSecond = 0.35f;
            float ignoreMe;
            m_AnimationPercentage = std::modf(m_AnimationPercentage + std::modf(dotMotionsPerSecond * dt, &ignoreMe), &ignoreMe);
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

        glm::vec3 const& GetLightDir() const { return m_3DSceneLightDir; }
        glm::vec3 const& GetLightColor() const { return m_3DSceneLightColor; }
        glm::vec4 const& GetBgColor() const { return m_3DSceneBgColor; }
        gl::Texture2D& UpdSceneTex() { return m_3DSceneTex; }

        glm::vec4 const& GetMeshColor() const { return m_Colors.mesh; }
        glm::vec4 const& GetBodyColor() const { return m_Colors.body; }
        glm::vec4 const& GetGroundColor() const { return m_Colors.ground; }

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
            dt.mesh = m_Floor.mesh;
            dt.modelMatrix = GetFloorModelMtx();
            dt.normalMatrix = NormalMatrix(dt.modelMatrix);
            dt.color = {0.0f, 0.0f, 0.0f, 1.0f};  // doesn't matter: it's textured
            dt.rimColor = 0.0f;
            dt.maybeDiffuseTex = m_Floor.texture;
            return dt;
        }

        DrawableThing GenerateMeshElDrawable(MeshEl const& meshEl, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = meshEl.GetLogicalID();
            rv.groupId = g_MeshGroupID;
            rv.mesh = meshEl.GetMesh();
            rv.modelMatrix = GetModelMatrix(meshEl) * glm::scale(glm::mat4{1.0f}, glm::vec3{m_SceneScaleFactor, m_SceneScaleFactor, m_SceneScaleFactor});
            rv.normalMatrix = NormalMatrix(rv.modelMatrix);
            rv.color = color;
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        float GetSphereRadius() const
        {
            return 0.025f;
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

        void AppendBodyElAsFrame(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            // stolen from SceneGeneratorNew.cpp

            glm::vec3 origin = bodyEl.GetTranslationInGround();
            glm::vec3 rotationEuler = bodyEl.GetOrientationInGround();
            glm::mat3 rotation = glm::eulerAngleXYX(rotationEuler.x, rotationEuler.y, rotationEuler.z);

            static constexpr float g_FrameAxisLengthRescale = 0.25f;
            static constexpr float g_FrameAxisThickness = 0.0025f;

            // emit origin sphere
            {
                Sphere centerSphere{origin, 0.05f * g_FrameAxisLengthRescale};

                DrawableThing& sphere = appendOut.emplace_back();
                sphere.id = bodyEl.GetLogicalID();
                sphere.groupId = g_BodyGroupID;
                sphere.mesh = m_SphereMesh;
                sphere.modelMatrix = SphereMeshToSceneSphereXform(centerSphere);
                sphere.normalMatrix = NormalMatrix(sphere.modelMatrix);
                sphere.color = {1.0f, 1.0f, 1.0f, 1.0f};
                sphere.rimColor = 0.0f;
                sphere.maybeDiffuseTex = nullptr;
            }

            // emit "legs"
            Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
            for (int i = 0; i < 3; ++i) {
                glm::vec3 dir = {0.0f, 0.0f, 0.0f};
                dir[i] = 0.2f * g_FrameAxisLengthRescale;
                Segment axisline{origin, origin + rotation*dir};

                glm::vec3 prescale = {g_FrameAxisThickness, 1.0f, g_FrameAxisThickness};
                glm::mat4 prescaleMtx = glm::scale(glm::mat4{1.0f}, prescale);
                glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
                color[i] = 1.0f;

                DrawableThing& se = appendOut.emplace_back();
                se.id = bodyEl.GetLogicalID();
                se.groupId = g_BodyGroupID;
                se.mesh = m_CylinderMesh;
                se.modelMatrix = SegmentToSegmentXform(cylinderline, axisline) * prescaleMtx;
                se.normalMatrix = NormalMatrix(se.modelMatrix);
                se.color = color;
                se.rimColor = 0.0f;
                se.maybeDiffuseTex = nullptr;
            }
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
            UpdateAnimationPercentage(dt);

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

        // scale factor for all non-mesh, non-overlay scene elements (e.g.
        // the floor, bodies)
        //
        // this is necessary because some meshes can be extremely small/large and
        // scene elements need to be scaled accordingly (e.g. without this, a body
        // sphere end up being much larger than a mesh instance). Imagine if the
        // mesh was the leg of a fly
        float m_SceneScaleFactor = 1.0f;

        // floor data
        //
        // this is for the chequered floor
        struct {
            std::shared_ptr<gl::Texture2D> texture = std::make_shared<gl::Texture2D>(genChequeredFloorTexture());
            std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(generateFloorMesh());
        } m_Floor;

        // model created by this wizard
        //
        // `nullptr` until the model is successfully created
        std::unique_ptr<OpenSim::Model> m_MaybeOutputModel = nullptr;

        // clamped to [0, 1] - used for any active animations, like the dots on the connection lines
        float m_AnimationPercentage = 0.0f;

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
    class ModelWizardState {
    public:
        ModelWizardState(SharedData& sharedData) : m_SharedData{sharedData} {}
        virtual ~ModelWizardState() noexcept = default;

        SharedData& UpdSharedData() { return m_SharedData; }
        SharedData const& GetSharedData() const { return m_SharedData; }

        virtual std::unique_ptr<ModelWizardState> onEvent(SDL_Event const&) = 0;
        virtual std::unique_ptr<ModelWizardState> draw() = 0;
        virtual std::unique_ptr<ModelWizardState> tick(float) = 0;

    private:
        SharedData& m_SharedData;
    };

    // UI state for when a user is assigning something in the UI
    class AssignmentModeState final : public ModelWizardState {
        using ModelWizardState::ModelWizardState;

        std::unique_ptr<ModelWizardState> onEvent(SDL_Event const&) override
        {
            return nullptr;
        }

        std::unique_ptr<ModelWizardState> draw() override
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("assign")) {
                ImGui::PopStyleVar();
            } else {
                ImGui::PopStyleVar();
            }
            ImGui::End();
            return nullptr;
        }

        std::unique_ptr<ModelWizardState> tick(float) override
        {
            return nullptr;
        }

    private:
        // (maybe) hover + worldspace location of the hover
        std::pair<LogicalID, glm::vec3> m_MaybeHover;
    };

    // "standard" UI state: it's what's shown when the user is browsing around the main UI
    class StandardModeState final : public ModelWizardState {
    public:

        StandardModeState(SharedData& sharedData) :
            ModelWizardState{sharedData},
            m_Selected{},
            m_MaybeHover{},
            m_MaybeOpenedContextMenu{}
        {
        }

        void Select(LogicalID elID)
        {
            m_Selected.insert(elID);
        }

        void SelectAll()
        {
            auto addIDToSelectionSet = [this](LogicalID id) { m_Selected.insert(id); };
            GetSharedData().GetCurrentModelGraph().ForEachSceneElID(addIDToSelectionSet);
        }

        void DeSelectAll()
        {
            m_Selected.clear();
        }

        bool IsSelected(LogicalID id) const
        {
            return m_Selected.find(id) != m_Selected.end();
        }

        void DeleteSelected()
        {
            if (m_Selected.empty()) {
                return;
            }

            SharedData& shared = UpdSharedData();
            ModelGraph& mg = shared.UpdCurrentModelGraph();

            for (LogicalID id : m_Selected) {
                mg.DeleteElementByID(id);
            }
            shared.CommitCurrentModelGraph("deleted selection");
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
            if (!HasHover()) {
                return;
            }

            m_Selected.insert(m_MaybeHover.first);
        }

        float RimIntensity(LogicalID id)
        {
            if (!id) {
                return 0.0f;
            } else if (IsSelected(id)) {
                return 1.0f;
            } else if (IsHovered(id)) {
                return 0.5f;
            } else {
                return 0.0f;
            }
        }

        LogicalIDT<BodyEl> AddBody(glm::vec3 const& pos)
        {
            SharedData& shared = UpdSharedData();
            ModelGraph& mg = shared.UpdCurrentModelGraph();

            std::string name = GenerateBodyName();
            glm::vec3 orientation{};

            auto id = mg.AddBody(name, pos, orientation);
            shared.CommitCurrentModelGraph(std::string{"added "} + name);

            return id;
        }

        void AddBodyToHoveredElement()
        {
            if (!HasHover()) {
                return;
            }

            DeSelectAll();
            auto bodyID = AddBody(HoverPos());
            Select(bodyID);
        }

        void TryStartAssigningHover()
        {
            if (!HasHover()) {
                return;
            }

            // request a state transition
            m_MaybeNextState = std::make_unique<AssignmentModeState>(UpdSharedData());
        }

        void UpdateFromImGuiKeyboardState()
        {
            // DELETE: delete any selected elements
            if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
                DeleteSelected();
            }

            // B: add body to hovered element
            if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
                AddBodyToHoveredElement();
            }

            // A: assign a parent for the hovered element
            if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                TryStartAssigningHover();
            }

            // S: set manipulation mode to "scale"
            if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
                m_ImGuizmoState.op = ImGuizmo::SCALE;
            }

            // R: set manipulation mode to "rotate"
            if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
                m_ImGuizmoState.op = ImGuizmo::ROTATE;
            }

            // G: set manipulation mode to "grab" (translate)
            if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
                m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
            }


            bool ctrlPressed = ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL);
            bool shiftPressed = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);

            // Ctrl+A: select all
            if (ctrlPressed && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
                SelectAll();
            }

            // Ctrl+Z: undo
            if (ctrlPressed && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                UpdSharedData().UndoCurrentModelGraph();
            }

            // Ctrl+Shift+Z: redo
            if (ctrlPressed && shiftPressed && ImGui::IsKeyPressed(SDL_SCANCODE_Z)) {
                UpdSharedData().RedoCurrentModelGraph();
            }
        }

        void DrawHistory()
        {
            auto const& snapshots = GetSharedData().GetModelGraphSnapshots();
            size_t currentSnapshot = GetSharedData().GetModelGraphIsBasedOn();
            for (size_t i = 0; i < snapshots.size(); ++i) {
                ModelGraphSnapshot const& snapshot = snapshots[i];
                ImGui::Text("%s", snapshot.GetCommitMessage().c_str());
                if (ImGui::IsItemClicked()) {
                    UpdSharedData().UseModelGraphSnapshot(i);
                }
            }
        }

        void DrawSidebar()
        {
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
            if (ImGui::Button(ICON_FA_PLUS "Import Meshes")) {
                UpdSharedData().PromptUserForMeshFilesAndPushThemOntoMeshLoader();
            }
            if (ImGui::Button(ICON_FA_PLUS "Add Body")) {
                AddBody({0.0f, 0.0f, 0.0f});
            }

            ImGui::PopStyleColor();

            ImGui::Text("hover = %i, pos = (%.2f, %.2f, %.2f)", static_cast<int>(m_MaybeHover.first.Unwrap()), m_MaybeHover.second[0], m_MaybeHover.second[1], m_MaybeHover.second[2]);

            DrawHistory();
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

        void DrawHoverTooltip()
        {
            if (!HasHover()) {
                return;
            }

            LogicalID hoverID = HoverID();
            ModelGraph const& mg = GetSharedData().GetCurrentModelGraph();

            if (MeshEl const* meshEl = mg.TryGetMeshElByID(hoverID)) {
                DrawMeshHoverTooltip(*meshEl);
            } else if (BodyEl const* bodyEl = mg.TryGetBodyElByID(hoverID)) {
                DrawBodyHoverTooltip(*bodyEl);
            }
        }

        void Draw3DManipulators(std::vector<DrawableThing> const& drawables)
        {
            // if the user isn't manipulating anything, create an up-to-date
            // manipulation matrix
            if (!ImGuizmo::IsUsing()) {
                if (m_Selected.empty()) {
                    return;  // nothing's selected
                }

                auto it = drawables.begin();
                auto end = drawables.end();

                AABB aabb;
                bool foundAABB = false;
                while (!foundAABB && it != end) {
                    if (IsSelected(it->id)) {
                        aabb = AABBApplyXform(it->mesh->getAABB(), it->modelMatrix);
                        foundAABB = true;
                    }
                    ++it;
                }

                if (!foundAABB) {
                    return;  // nothing's selected
                }

                for (; it != end; ++it) {
                    aabb = AABBUnion(aabb, AABBApplyXform(it->mesh->getAABB(), it->modelMatrix));
                }

                m_ImGuizmoState.mtx = glm::translate(glm::mat4{1.0f}, AABBCenter(aabb));
            }

            // else: is using OR nselected > 0 (so draw it)

            Rect sceneRect = GetSharedData().Get3DSceneRect();

            ImGuizmo::SetRect(
                sceneRect.p1.x,
                sceneRect.p1.y,
                RectDims(sceneRect).x,
                RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

            glm::mat4 delta;
            bool manipulated = ImGuizmo::Manipulate(
                glm::value_ptr(GetSharedData().GetCamera().getViewMtx()),
                glm::value_ptr(GetSharedData().GetCamera().getProjMtx(RectAspectRatio(sceneRect))),
                m_ImGuizmoState.op,
                ImGuizmo::LOCAL,
                glm::value_ptr(m_ImGuizmoState.mtx),
                glm::value_ptr(delta),
                nullptr,
                nullptr,
                nullptr);

            if (!manipulated) {
                return;
            }

            // TODO: apply

            /*
            for (auto& node : m_SelectedNodes) {
                switch (m_ImGuizmoState.op) {
                case ImGuizmo::SCALE:
                    node->ApplyScale(delta);
                    break;
                case ImGuizmo::ROTATE:
                    node->ApplyRotation(delta);
                    break;
                case ImGuizmo::TRANSLATE:
                    node->ApplyTranslation(delta);
                    break;
                default:
                    break;
                }
            }
            */
        }

        void DoHovertest(std::vector<DrawableThing> const& drawables)
        {
            SharedData const& shared = GetSharedData();

            if (!shared.IsRenderHovered() || ImGuizmo::IsUsing()) {
                ClearHover();
                return;
            }

            m_MaybeHover = shared.Hovertest(drawables);

            bool lcReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            bool rcReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
            bool shiftDown = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
            bool isUsingGizmo = ImGuizmo::IsUsing();

            if (!HasHover() && lcReleased && !isUsingGizmo && !shiftDown) {
                DeSelectAll();
                return;
            }

            if (HasHover() && lcReleased && !isUsingGizmo) {
                if (!shiftDown) {
                    DeSelectAll();
                }
                SelectHover();
                return;
            }

            /* TODO
            if (m_MaybeHover && rcReleased && !isUsingGizmo) {
                OpenHoverContextMenu();
            }
                        */

            DrawHoverTooltip();
        }

        void Draw3DViewer()
        {
            SharedData& shared = UpdSharedData();

            shared.Set3DSceneRect(ContentRegionAvailScreenRect());

            std::vector<DrawableThing> sceneEls;

            if (shared.IsShowingMeshes()) {
                for (auto const& [meshID, meshEl] : shared.GetCurrentModelGraph().GetMeshes()) {
                    sceneEls.push_back(shared.GenerateMeshElDrawable(meshEl, shared.GetMeshColor()));
                }
            }

            if (shared.IsShowingBodies()) {
                for (auto const& [bodyID, bodyEl] : shared.GetCurrentModelGraph().GetBodies()) {
                    shared.AppendBodyElAsFrame(bodyEl, sceneEls);
                    //sceneEls.push_back(shared.GenerateBodyElSphere(bodyEl, shared.GetMeshColor()));
                }
            }

            if (shared.IsShowingFloor()) {
                sceneEls.push_back(shared.GenerateFloorDrawable());
            }

            // hovertest the generated geometry
            DoHovertest(sceneEls);

            // assign rim highlights based on hover
            for (DrawableThing& dt : sceneEls) {
                dt.rimColor = RimIntensity(dt.id);
            }

            // optimize draw order
            Sort(sceneEls, OptimalDrawOrder);

            // draw
            DrawScene(
                RectDims(shared.Get3DSceneRect()),
                shared.GetCamera(),
                shared.GetLightDir(),
                shared.GetLightColor(),
                shared.GetBgColor(),
                sceneEls,
                shared.UpdSceneTex());

            DrawTextureAsImGuiImage(shared.UpdSceneTex(), RectDims(shared.Get3DSceneRect()));
            shared.SetIsRenderHovered(ImGui::IsItemHovered());
            Draw3DManipulators(sceneEls);
            // TODO: draw connection lines
            // TODO: draw context menu
        }

        std::unique_ptr<ModelWizardState> onEvent(SDL_Event const&) override
        {
            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<ModelWizardState> tick(float) override
        {
            SharedData& shared = UpdSharedData();

            if (shared.IsRenderHovered() && !ImGuizmo::IsUsing()) {
                UpdatePolarCameraFromImGuiUserInput(shared.Get3DSceneDims(), shared.UpdCamera());
            }

            UpdateFromImGuiKeyboardState();

            return std::move(m_MaybeNextState);
        }

        std::unique_ptr<ModelWizardState> draw() override
        {
            ImGuizmo::BeginFrame();

            if (ImGui::Begin("wizardstep2sidebar")) {
                DrawSidebar();
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
        // scene elements selected by the user
        std::unordered_set<LogicalID> m_Selected;

        // (maybe) hover + worldspace location of the hover
        std::pair<LogicalID, glm::vec3> m_MaybeHover;

        // (maybe) the scene element the user opened a context menu for
        std::pair<LogicalID, glm::vec3> m_MaybeOpenedContextMenu;

        // (maybe) the next state the host screen should transition to
        std::unique_ptr<ModelWizardState> m_MaybeNextState;

        // ImGuizmo state
        struct {
            glm::mat4 mtx{};
            ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        } m_ImGuizmoState;
    };
}

/*
 *

            std::filesystem::path meshPath = App::resource("geometry/block.vtp");
            std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(SimTKLoadMesh(meshPath));

            ModelGraph mg;

            // first pendulum part
            LogicalIDT<BodyEl> firstBlockID = mg.AddBody("firstBlock", {0.5f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
            BodyEl firstBlockCopy = mg.GetBodyByIDOrThrow(firstBlockID);

            // join it to ground with a joint location +1.0Y in ground
            mg.AddJoint(
                *JointRegistry::indexOf(OpenSim::PinJoint{}),
                "",
                JointAttachment{LogicalIDT<BodyEl>::empty(), {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},
                JointAttachment{firstBlockID, {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}});

            // attach a block to the first pendulum bit
            LogicalIDT<MeshEl> meshID = mg.AddMesh(mesh, firstBlockID, meshPath);
            mg.SetMeshTranslationOrientationInGround(meshID, firstBlockCopy.GetTranslationOrientationInGround());

            // second pendulum part
            LogicalIDT<BodyEl> secondBlockID = mg.AddBody("secondBlock", {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f});

            // join it to the first part at the first part's location
            mg.AddJoint(
                *JointRegistry::indexOf(OpenSim::PinJoint{}),
                "",
                JointAttachment{firstBlockID, {{0.5f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},
                JointAttachment{secondBlockID, {{0.5f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}});



            LogicalIDT<MeshEl> meshID = mg.AddMesh(mesh, customJointBodyID, meshPath);
            TranslationOrientation bodyTO = mg.GetBodyByIDOrThrow(customJointBodyID).GetTranslationOrientationInGround();
            bodyTO.orientation.x += fpi4;
            mg.SetMeshTranslationOrientationInGround(meshID, bodyTO);

            std::vector<std::string> issuesOut;
            auto osimModel = CreateOpenSimModelFromModelGraph(mg, issuesOut);
            osimModel->updDisplayHints().set_show_frames(true);
            OSC_ASSERT_ALWAYS(issuesOut.empty());
            OSC_ASSERT_ALWAYS(osimModel != nullptr);
            auto mes = std::make_shared<MainEditorState>(std::move(osimModel));
            App::cur().requestTransition<ModelEditorScreen>(mes);

            return nullptr;
*/

/*

// top-level Impl struct
//
// this is what the `Screen` keeps between frames
struct osc::MeshesToModelWizardScreen::Impl final {

    void Select(std::shared_ptr<Node> const& node)
    {
        if (IsType<GroundNode>(*node)) {
            return;
        }

        m_SelectedNodes.insert(node);
    }

    void SelectAll()
    {
        m_SelectedNodes.clear();
        ForEachNode(m_ModelRoot, [this](auto const& nodePtr) { Select(nodePtr); });
    }

    void DeSelectAll()
    {
        m_SelectedNodes.clear();
    }

    bool IsSelected(Node const& node) const
    {
        for (auto const& selectedPtr : m_SelectedNodes) {
            if (selectedPtr.get() == &node) {
                return true;
            }
        }
        return false;
    }

    bool HasHover() const
    {
        return m_MaybeHover;
    }

    bool IsHovered(Node const& node) const
    {
        return m_MaybeHover == node;
    }

    void ClearHover()
    {
        m_MaybeHover.reset();
    }

    void SelectHover()
    {
        auto hover = m_MaybeHover.lock();
        if (hover) {
            m_SelectedNodes.insert(hover);
        }
    }

    bool IsInAssignmentMode() const
    {
        return m_MaybeCurrentAssignment.lock() != nullptr;
    }

    void StartAssigning(std::shared_ptr<Node> newAssignee)
    {
        // nullptr is allowed: effectively means "stop assigning"

        if (newAssignee && IsType<GroundNode>(*newAssignee)) {
            return;
        }

        m_MaybeCurrentAssignment = newAssignee;
    }

    void StartAssigningHover()
    {
        StartAssigning(m_MaybeHover.lock());
    }

    void LeaveAssignmentMode()
    {
        m_MaybeCurrentAssignment.reset();
    }

    void Assign(std::shared_ptr<Node> assignee, std::shared_ptr<Node> newParent)
    {
        auto& assigneeParent = assignee->UpdParent();
        assigneeParent.RemoveChild(assignee);
        assignee->SetParent(newParent);
        newParent->AddChild(assignee);
    }

    void AssignAssigneeTo(std::shared_ptr<Node> newParent)
    {
        if (!newParent) {
            return;
        }

        if (!newParent->CanHaveChildren()) {
            return;
        }

        auto assignee = m_MaybeCurrentAssignment.lock();

        if (!assignee) {
            return;
        }

        Assign(assignee, newParent);
        LeaveAssignmentMode();
    }

    float GetRimIntensity(Node const& node) const
    {
        if (IsSelected(node)) {
            return 1.0f;
        } else if (IsHovered(node)) {
            return 0.5f;
        } else {
            return 0.0f;
        }
    }

    void PushMeshLoadRequest(std::weak_ptr<Node> prefAttachmentPoint, std::filesystem::path const& meshPath)
    {
        m_MeshLoader.send(MeshLoadRequest{prefAttachmentPoint, meshPath});
    }

    template<typename Container>
    void PushMeshLoadRequests(std::weak_ptr<Node> attachmentPoint, Container const& meshPaths)
    {
        for (auto const& meshPath : meshPaths) {
            PushMeshLoadRequest(attachmentPoint, meshPath);
        }
    }

    void PromptUserForMeshFiles(std::weak_ptr<Node> attachmentPoint)
    {
        for (auto const& meshPath : ::PromptUserForMeshFiles()) {
            PushMeshLoadRequest(attachmentPoint, meshPath);
        }
    }

    std::shared_ptr<BodyNode> AddBody(glm::vec3 const& pos)
    {
        auto bodyNode = std::make_shared<BodyNode>(m_ModelRoot, pos);
        m_ModelRoot->AddChild(bodyNode);
        return bodyNode;
    }

    void DeleteNodeButReParentItsChildren(std::shared_ptr<Node> node)
    {
        if (!node->CanHaveParent()) {
            return;
        }

        auto parentPtr = node->UpdParentPtr();
        for (auto& childPtr : node->UpdChildren()) {
            parentPtr->AddChild(childPtr);
            childPtr->SetParent(parentPtr);
        }
        node->ClearChildren();

        node->UpdParent().RemoveChild(node);
    }

    void DeleteSubtree(std::shared_ptr<Node> node)
    {
        if (!node->CanHaveParent()) {
            return;
        }

        node->UpdParent().RemoveChild(node);
    }

    void DeleteSelected()
    {
        for (auto const& selectedNodePtr : m_SelectedNodes) {
            DeleteSubtree(selectedNodePtr);
        }
        m_SelectedNodes.clear();
    }

    void PopMeshLoader()
    {
        for (auto maybeResponse = m_MeshLoader.poll(); maybeResponse.has_value(); maybeResponse = m_MeshLoader.poll()) {
            MeshLoadResponse& meshLoaderResp = *maybeResponse;

            if (std::holds_alternative<MeshLoadOKResponse>(meshLoaderResp)) {
                // handle OK message from loader
                MeshLoadOKResponse& ok = std::get<MeshLoadOKResponse>(meshLoaderResp);
                auto maybeAttachmentPoint = ok.GetPreferredAttachmentPoint().lock();
                if (!maybeAttachmentPoint) {
                    maybeAttachmentPoint = m_ModelRoot;
                }
                auto meshNode = std::make_shared<MeshNode>(maybeAttachmentPoint, ok.GetMeshPtr(), ok.GetPath());
                meshNode->ApplyTranslation(glm::translate(glm::mat4{1.0f}, maybeAttachmentPoint->GetPos()));
                maybeAttachmentPoint->AddChild(meshNode);
            } else {
                MeshLoadErrorResponse& err = std::get<MeshLoadErrorResponse>(meshLoaderResp);
                log::error("%s: error loading mesh file: %s", err.GetPath().string().c_str(), err.what());
            }
        }
    }

    bool IsMouseOverRender() const
    {
        return m_IsRenderHovered;
    }

    void UpdateCameraFromImGuiUserInput()
    {
        if (!IsMouseOverRender()) {
            return;
        }

        if (ImGuizmo::IsUsing()) {
            return;
        }

        UpdatePolarCameraFromImGuiUserInput(RectDims(m_3DSceneRect), m_3DSceneCamera);
    }

    void UpdateFromImGuiKeyboardState()
    {
        // DELETE: delete any selected elements
        if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
            DeleteSelected();
        }

        // B: add body to hovered element
        if (ImGui::IsKeyPressed(SDL_SCANCODE_B)) {
            auto hover = m_MaybeHover.lock();
            if (hover) {
                DeSelectAll();
                glm::vec3 pos = hover->GetCenterPoint();
                auto body = AddBody(pos);
                Select(body);
            }
        }

        // A: assign a parent for the hovered element
        if (ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
            StartAssigningHover();
        }

        // ESC: leave assignment state
        if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
            LeaveAssignmentMode();
        }

        // CTRL+A: select all
        if ((ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) && ImGui::IsKeyPressed(SDL_SCANCODE_A)) {
            SelectAll();
        }

        // S: set manipulation mode to "scale"
        if (ImGui::IsKeyPressed(SDL_SCANCODE_S)) {
            m_ImGuizmoState.op = ImGuizmo::SCALE;
        }

        // R: set manipulation mode to "rotate"
        if (ImGui::IsKeyPressed(SDL_SCANCODE_R)) {
            m_ImGuizmoState.op = ImGuizmo::ROTATE;
        }

        // G: set manipulation mode to "grab" (translate)
        if (ImGui::IsKeyPressed(SDL_SCANCODE_G)) {
            m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
        }
    }

    void FocusCameraOn(Node const& node)
    {
        m_3DSceneCamera.focusPoint = -node.GetCenterPoint();
    }

    AABB GetSceneAABB() const
    {
        AABB sceneAABB = m_ModelRoot->GetBounds(m_SceneScaleFactor);
        ForEachNode(m_ModelRoot, [&](auto const& node) { sceneAABB = AABBUnion(sceneAABB, node->GetBounds(m_SceneScaleFactor)); });
        return sceneAABB;
    }

    void AutoScaleScene()
    {
        if (!m_ModelRoot->HasChildren()) {
            m_SceneScaleFactor = 1.0f;
            return;
        }

        float longest = AABBLongestDim(GetSceneAABB());

        m_SceneScaleFactor = 5.0f * longest;
        m_3DSceneCamera.focusPoint = {};
        m_3DSceneCamera.radius = 5.0f * longest;
    }

    glm::vec2 WorldPosToScreenPos(glm::vec3 const& worldPos)
    {
        return m_3DSceneCamera.projectOntoScreenRect(worldPos, m_3DSceneRect);
    }

    void DrawConnectionLine(ImU32 color, glm::vec2 parent, glm::vec2 child)
    {
        // the line
        ImGui::GetForegroundDrawList()->AddLine(parent, child, color, 2.0f);

        // moving dot between the two points to indicate directionality
        glm::vec2 child2parent = parent - child;
        glm::vec2 dotPos = child + m_AnimationPercentage*child2parent;
        ImGui::GetForegroundDrawList()->AddCircleFilled(dotPos, 4.0f, color);
    }

    void DrawConnectionLine(ImU32 color, Node const& parent, Node const& child)
    {
        DrawConnectionLine(color, WorldPosToScreenPos(parent.GetCenterPoint()), WorldPosToScreenPos(child.GetCenterPoint()));
    }

    void DrawLineToParent(ImU32 color, Node const& child)
    {
        auto parent = child.GetParentPtr();
        if (parent) {
            DrawConnectionLine(color, *parent, child);
        }
    }

    void DrawLinesToChildren(ImU32 color, Node const& node)
    {
        for (auto const& child : node.GetChildren()) {
            DrawConnectionLine(color, node, *child);
        }
    }

    void DrawAllConnectionLines(ImU32 color)
    {
        ForEachNode(m_ModelRoot, [this, color](auto const& node) { DrawLinesToChildren(color, *node); });
    }

    void DrawConnectionLinesFrom(ImU32 color, Node const& centralNode)
    {
        DrawLineToParent(color, centralNode);
        DrawLinesToChildren(color, centralNode);
    }

    void DrawConnectionLines(ImU32 color)
    {
        if (m_IsShowingAllConnectionLines) {
            DrawAllConnectionLines(color);
            return;
        }

        auto hover = m_MaybeHover.lock();

        // draw lines from hover to parent and children
        if (hover) {
            DrawConnectionLinesFrom(color, *hover);
        }

        // draw lines from selected els to their parents
        for (auto const& selectedNode : m_SelectedNodes) {
            if (selectedNode != hover) {
                DrawLineToParent(color, *selectedNode);
            }
        }
    }

    void DrawMeshHoverTooltip(MeshNode const& meshNode, glm::vec3 const& hoverPos)
    {
        ImGui::BeginTooltip();
        ImGui::Text("Imported Mesh");
        ImGui::Indent();
        ImGui::Text("Name = %s", meshNode.GetNameCStr());
        ImGui::Text("Filename = %s", meshNode.GetPath().filename().string().c_str());
        ImGui::Text("Conntected to = %s", meshNode.GetParent().GetNameCStr());
        auto pos = meshNode.GetCenterPoint();
        ImGui::Text("Center = (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        auto parentPos = meshNode.GetParent().GetCenterPoint();
        ImGui::Text("Center's Distance to Parent = %.2f", glm::length(pos - parentPos));
        ImGui::Text("Mouse Location = (%.2f, %.2f, %.2f)", hoverPos.x, hoverPos.y, hoverPos.z);
        ImGui::Text("Mouse Location's Distance to Parent = %.2f", glm::length(hoverPos - parentPos));
        ImGui::Unindent();
        ImGui::EndTooltip();
    }

    void DrawBodyHoverTooltip(BodyNode const& bodyNode)
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Body");
        ImGui::Indent();
        ImGui::Text("Name = %s", bodyNode.GetNameCStr());
        ImGui::Text("Connected to = %s", bodyNode.GetParent().GetNameCStr());
        ImGui::Text("Joint type = %s", bodyNode.GetJointTypeNameCStr());
        auto pos = bodyNode.GetPos();
        ImGui::Text("Pos = (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        auto parentPos = bodyNode.GetParent().GetPos();
        ImGui::Text("Distance to Parent = %.2f", glm::length(pos - parentPos));
        ImGui::Unindent();
        ImGui::EndTooltip();
    }

    void DrawHoverTooltip()
    {
        auto hover = m_MaybeHover.lock();

        if (!hover) {
            return;
        }

        if (auto meshNodePtr = dynamic_cast<MeshNode*>(hover.get())) {
            DrawMeshHoverTooltip(*meshNodePtr, m_MaybeHover.getPos());
        } else if (auto bodyPtr = dynamic_cast<BodyNode*>(hover.get())) {
            DrawBodyHoverTooltip(*bodyPtr);
        } else if (auto groundPtr = dynamic_cast<GroundNode*>(hover.get())) {
            ImGui::BeginTooltip();
            ImGui::Text("ground");
            ImGui::EndTooltip();
        }
    }

    void DrawMeshContextMenu(std::shared_ptr<MeshNode> meshNode, glm::vec3 const& pos)
    {
        if (ImGui::BeginPopup("contextmenu")) {
            if (ImGui::BeginMenu("add body")) {
                if (ImGui::MenuItem("at mouse location")) {
                    auto addedBody = AddBody(pos);
                    DeSelectAll();
                    Select(addedBody);
                }

                if (ImGui::MenuItem("at mouse location, and assign the mesh to it")) {
                    auto addedBody = AddBody(pos);
                    Assign(addedBody, meshNode->UpdParentPtr());
                    Assign(meshNode, addedBody);
                    DeSelectAll();
                    Select(addedBody);
                }

                if (ImGui::MenuItem("at center")) {
                    auto body = AddBody(meshNode->GetCenterPoint());
                    DeSelectAll();
                    Select(body);
                }

                if (ImGui::MenuItem("at center, and assign this mesh to it")) {
                    auto addedBody = AddBody(meshNode->GetCenterPoint());
                    Assign(addedBody, meshNode->UpdParentPtr());
                    Assign(meshNode, addedBody);
                    DeSelectAll();
                    Select(addedBody);
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("focus camera on this")) {
                FocusCameraOn(*meshNode);
            }

            if (ImGui::MenuItem("reassign parent")) {
                StartAssigning(meshNode);
            }

            if (ImGui::MenuItem("delete")) {
                DeleteSubtree(meshNode);
            }

            // draw name editor
            {
                char buf[256];
                std::strcpy(buf, meshNode->GetNameCStr());
                if (ImGui::InputText("set mesh name", buf, sizeof(buf))) {
                    meshNode->SetName(buf);
                }
            }
        }
        ImGui::EndPopup();
    }

    void DrawBodyContextMenu(std::shared_ptr<BodyNode> bodyNode)
    {
        if (ImGui::BeginPopup("contextmenu")) {
            if (ImGui::MenuItem("add body (assigned to this one)")) {
                auto addedBody = AddBody(bodyNode->GetCenterPoint());
                Assign(addedBody, bodyNode);
                DeSelectAll();
                Select(addedBody);
            }

            if (ImGui::MenuItem("assign mesh(es)")) {
                PromptUserForMeshFiles(bodyNode);
            }

            if (ImGui::MenuItem("focus camera on this")) {
                FocusCameraOn(*bodyNode);
            }

            if (ImGui::MenuItem("reassign parent")) {
                StartAssigning(bodyNode);
            }

            if (ImGui::BeginMenu("delete")) {
                if (ImGui::MenuItem("entire subtree")) {
                    DeleteSubtree(bodyNode);
                }

                if (ImGui::MenuItem("just this")) {
                    DeleteNodeButReParentItsChildren(bodyNode);
                }

                ImGui::EndMenu();
            }

            ImGui::Dummy({0.0f, 5.0f});
            ImGui::Separator();
            ImGui::Dummy({0.0f, 5.0f});

            // draw name editor
            {
                char buf[256];
                std::strcpy(buf, bodyNode->GetNameCStr());
                if (ImGui::InputText("name", buf, sizeof(buf))) {
                    bodyNode->SetName(buf);
                }
            }

            // draw mass editor
            {
                float mass = static_cast<float>(bodyNode->GetMass());
                if (ImGui::InputFloat("mass (kg)", &mass)) {
                    bodyNode->SetMass(static_cast<double>(mass));
                }
            }

            ImGui::Dummy({0.0f, 5.0f});
            ImGui::Separator();
            ImGui::Dummy({0.0f, 5.0f});

            int jointTypeIdx = static_cast<int>(bodyNode->GetJointTypeIndex());
            auto jointTypeStrings = JointRegistry::nameCStrings();
            if (ImGui::Combo("joint type", &jointTypeIdx, jointTypeStrings.data(), jointTypeStrings.size())) {
                bodyNode->SetJointTypeIndex(static_cast<size_t>(jointTypeIdx));
            }

            // draw joint name editor
            {
                char buf2[256];
                std::strcpy(buf2, bodyNode->GetJointNameCStr());
                if (ImGui::InputText("joint name", buf2, sizeof(buf2))) {
                    bodyNode->SetUserDefinedJointName(buf2);
                }
            }
        }
        ImGui::EndPopup();
    }

    void DrawContextMenu()
    {
        auto editedNode = m_MaybeNodeOpenedInContextMenu.lock();

        if (!editedNode) {
            return;
        }

        if (auto meshNodePtr = std::dynamic_pointer_cast<MeshNode>(editedNode)) {
            DrawMeshContextMenu(meshNodePtr, m_MaybeNodeOpenedInContextMenu.getPos());
        } else if (auto bodyPtr = std::dynamic_pointer_cast<BodyNode>(editedNode)) {
            DrawBodyContextMenu(bodyPtr);
        }
    }

    void OpenHoverContextMenu()
    {
        if (!m_MaybeHover) {
            return;
        }

        m_MaybeNodeOpenedInContextMenu = m_MaybeHover;
        ImGui::OpenPopup("contextmenu");
    }

    void Draw3DManipulators()
    {
        // if the user isn't manipulating anything, create an up-to-date
        // manipulation matrix
        if (!ImGuizmo::IsUsing()) {
            auto it = m_SelectedNodes.begin();
            auto end = m_SelectedNodes.end();

            if (it == end) {
                return;  // nothing's selected
            }

            AABB aabb = (*it)->GetBounds(m_SceneScaleFactor);
            while (++it != end) {
                aabb = AABBUnion(aabb, (*it)->GetBounds(m_SceneScaleFactor));
            }

            m_ImGuizmoState.mtx = glm::translate(glm::mat4{1.0f}, AABBCenter(aabb));
        }

        // else: is using OR nselected > 0 (so draw it)

        ImGuizmo::SetRect(
            m_3DSceneRect.p1.x,
            m_3DSceneRect.p1.y,
            RectDims(m_3DSceneRect).x,
            RectDims(m_3DSceneRect).y);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        glm::mat4 delta;
        bool manipulated = ImGuizmo::Manipulate(
            glm::value_ptr(m_3DSceneCamera.getViewMtx()),
            glm::value_ptr(m_3DSceneCamera.getProjMtx(RectAspectRatio(m_3DSceneRect))),
            m_ImGuizmoState.op,
            ImGuizmo::LOCAL,
            glm::value_ptr(m_ImGuizmoState.mtx),
            glm::value_ptr(delta),
            nullptr,
            nullptr,
            nullptr);

        if (!manipulated) {
            return;
        }

        for (auto& node : m_SelectedNodes) {
            switch (m_ImGuizmoState.op) {
            case ImGuizmo::SCALE:
                node->ApplyScale(delta);
                break;
            case ImGuizmo::ROTATE:
                node->ApplyRotation(delta);
                break;
            case ImGuizmo::TRANSLATE:
                node->ApplyTranslation(delta);
                break;
            default:
                break;
            }
        }
    }

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

    Hover DoHittest(glm::vec2 pos, bool isGroundInteractable, bool isBodiesInteractable, bool isMeshesInteractable) const
    {
        if (!PointIsInRect(m_3DSceneRect, pos)) {
            return {};
        }

        Line ray = m_3DSceneCamera.unprojectTopLeftPosToWorldRay(pos - m_3DSceneRect.p1, RectDims(m_3DSceneRect));

        std::shared_ptr<Node> closest = nullptr;
        float closestDist = std::numeric_limits<float>::max();

        ForEachNode(m_ModelRoot, [&](auto const& node) {
            if (!isBodiesInteractable && IsType<BodyNode>(*node)) {
                return;
            }

            if (!isMeshesInteractable && IsType<MeshNode>(*node)) {
                return;
            }

            if (!isGroundInteractable && IsType<GroundNode>(*node)) {
                return;
            }

            RayCollision rc = node->DoHittest(m_SceneScaleFactor, ray);
            if (rc.hit && rc.distance < closestDist) {
                closest = node;
                closestDist = rc.distance;
            }
        });

        glm::vec3 hitPos = ray.origin + closestDist*ray.dir;

        return Hover{closest, hitPos};
    }

    // draw sidebar containing basic documentation and some action buttons
    void DrawSidebar()
    {
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

        // draw editors (checkboxes, sliders, etc.)
        ImGui::Checkbox("show floor", &m_IsShowingFloor);
        ImGui::Checkbox("show meshes", &m_IsShowingMeshes);
        ImGui::Checkbox("show ground", &m_IsShowingGround);
        ImGui::Checkbox("show bodies", &m_IsShowingBodies);
        ImGui::Checkbox("can click meshes", &m_IsMeshesInteractable);
        ImGui::Checkbox("can click bodies", &m_IsBodiesInteractable);
        ImGui::Checkbox("show all connection lines", &m_IsShowingAllConnectionLines);
        ImGui::ColorEdit4("mesh color", glm::value_ptr(m_Colors.mesh));
        ImGui::ColorEdit4("ground color", glm::value_ptr(m_Colors.ground));
        ImGui::ColorEdit4("body color", glm::value_ptr(m_Colors.body));

        ImGui::InputFloat("scene_scale_factor", &m_SceneScaleFactor);
        if (ImGui::Button("autoscale scene_scale_factor")) {
            AutoScaleScene();
        }

        // draw actions (buttons, etc.)
        if (ImGui::Button("add body")) {
            auto newBody = AddBody({0.0f, 0.0f, 0.0f});
            DeSelectAll();
            Select(newBody);
        }
        if (ImGui::Button("select all")) {
            SelectAll();
        }
        if (ImGui::Button("clear selection")) {
            ClearHover();
        }
        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLUS "Import Meshes")) {
            PromptUserForMeshFiles(m_ModelRoot);
        }
        ImGui::PopStyleColor();

        m_ErrorMessageBuffer.clear();
        if (!GetDagIssuesRecursive(*m_ModelRoot, m_ErrorMessageBuffer)) {

            // show button for model creation if no issues
            if (ImGui::Button("next >>")) {
                m_MaybeOutputModel = CreateModelFromDag(*m_ModelRoot, m_ErrorMessageBuffer);
            }
        } else {

            // show issues
            ImGui::Text("issues (%zu):", m_ErrorMessageBuffer.size());
            ImGui::Separator();
            ImGui::Dummy(ImVec2{0.0f, 5.0f});
            for (std::string const& s : m_ErrorMessageBuffer) {
                ImGui::TextUnformatted(s.c_str());
            }
        }
    }

    DrawableThing GenerateMeshSceneEl(MeshNode const& meshNode, glm::vec4 const& color)
    {
        DrawableThing rv;
        rv.mesh = meshNode.GetMesh();
        rv.modelMatrix = meshNode.GetModelMatrix(m_SceneScaleFactor);
        rv.normalMatrix = NormalMatrix(rv.modelMatrix);
        rv.color = color;
        rv.rimColor = GetRimIntensity(meshNode);
        rv.maybeDiffuseTex = nullptr;
        return rv;
    }

    DrawableThing GenerateSphereSceneEl(BodyNode const& bodyNode, glm::vec4 const& color)
    {
        DrawableThing rv;
        rv.mesh = m_SphereMesh;
        rv.modelMatrix = bodyNode.GetModelMatrix(m_SceneScaleFactor);
        rv.normalMatrix = NormalMatrix(rv.modelMatrix);
        rv.color = color;
        rv.rimColor = GetRimIntensity(bodyNode);
        rv.maybeDiffuseTex = nullptr;
        return rv;
    }

    DrawableThing GenerateFloor()
    {
        DrawableThing dt;
        dt.mesh = m_Floor.mesh;
        dt.modelMatrix = GetFloorModelMtx();
        dt.normalMatrix = NormalMatrix(dt.modelMatrix);
        dt.color = {0.0f, 0.0f, 0.0f, 1.0f};  // doesn't matter: it's textured
        dt.rimColor = 0.0f;
        dt.maybeDiffuseTex = m_Floor.texture;
        return dt;
    }

    DrawableThing GenerateGroundSceneEl()
    {
        DrawableThing dt;
        dt.mesh = m_SphereMesh;
        dt.modelMatrix = glm::scale(glm::mat4{1.0f}, glm::vec3{g_SphereRadius, g_SphereRadius, g_SphereRadius});
        dt.normalMatrix = NormalMatrix(dt.modelMatrix);
        dt.color = {0.0f, 0.0f, 0.0f, 1.0f};
        dt.rimColor = 0.0f;
        dt.maybeDiffuseTex = nullptr;
        return dt;
    }

    void DrawTextureAsImguiImage(gl::Texture2D& t, glm::vec2 dims)
    {
        void* textureHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(t.get()));
        ImVec2 uv0{0.0f, 1.0f};
        ImVec2 uv1{1.0f, 0.0f};
        ImGui::Image(textureHandle, dims, uv0, uv1);
        m_IsRenderHovered = ImGui::IsItemHovered();
    }

    void HoverTest_AssignmentMode()
    {
        auto assignee = m_MaybeCurrentAssignment.lock();
        if (!assignee) {
            return;
        }

        m_MaybeHover.reset();  // reset old hover

        if (!IsMouseOverRender()) {
            return;
        }

        Hover newHover = DoHittest(ImGui::GetMousePos(), true, m_IsBodiesInteractable, false);

        auto hover = newHover.lock();

        if (!hover) {
            return;  // nothing hovered
        }

        if (hover == assignee) {
            return;  // can't self-assign
        }

        if (!hover->CanHaveChildren()) {
            return;  // can't assign to a node that can't have childrens
        }

        if (IsInclusiveDescendantOf(assignee.get(), hover.get())) {
            return;  // can't perform an assignment that would result in a cycle
        }

        // else: it's ok for the hovered node to be the new hover
        m_MaybeHover = newHover;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            AssignAssigneeTo(hover);
            return;
        }

        ImGui::BeginTooltip();
        ImGui::Text("assign to %s", hover->GetNameCStr());
        ImGui::EndTooltip();
    }

    void Draw3DViewer_AssignmentMode()
    {
        HoverTest_AssignmentMode();

        auto assignee = m_MaybeCurrentAssignment.lock();
        if (!assignee) {
            // nothing being assigned, this draw function might be an invalid call, just quit early
            return;
        }

        std::vector<DrawableThing> sceneEls;

        // draw assignee as a rim-highlighted el
        if (auto const* bodyPtr = dynamic_cast<BodyNode const*>(assignee.get())) {
            auto& el = sceneEls.emplace_back(GenerateSphereSceneEl(*bodyPtr, m_Colors.body));
            el.color.a = 0.5f;
            el.rimColor = 0.5f;
        } else if (auto const* meshPtr = dynamic_cast<MeshNode const*>(assignee.get())) {
            auto& el = sceneEls.emplace_back(GenerateMeshSceneEl(*meshPtr, m_Colors.mesh));
            el.color.a = 0.5f;
            el.rimColor = 0.5f;
        }

        // draw all frame nodes (they are possible attachment points)
        ForEachNode(m_ModelRoot, [&](auto const& node) {
            if (auto const* bodyPtr = dynamic_cast<BodyNode const*>(node.get())) {
                auto& el = sceneEls.emplace_back(GenerateSphereSceneEl(*bodyPtr, m_Colors.body));
                el.rimColor = (m_MaybeHover == *bodyPtr) ? 0.35f : 0.0f;
                if (IsInclusiveDescendantOf(assignee.get(), bodyPtr)) {
                    el.color.a = 0.1f;  // make circular assignments faint
                }
            } else if (auto const* meshPtr = dynamic_cast<MeshNode const*>(node.get())) {
                auto& el = sceneEls.emplace_back(GenerateMeshSceneEl(*meshPtr, m_Colors.mesh));
                el.rimColor = 0.0f;
                el.color.a = 0.1f;  // show (non-assignable) meshes faintly
            }
        });

        // always draw ground (it's a possible attachment point)
        sceneEls.push_back(GenerateGroundSceneEl());

        if (m_IsShowingFloor) {
            sceneEls.push_back(GenerateFloor());
        }

        std::sort(sceneEls.begin(), sceneEls.end(), OptimalDrawOrder);

        DrawScene(
            RectDims(m_3DSceneRect),
            m_3DSceneCamera.getProjMtx(RectAspectRatio(m_3DSceneRect)),
            m_3DSceneCamera.getViewMtx(),
            m_3DSceneCamera.getPos(),
            m_3DSceneLightDir,
            m_3DSceneLightColor,
            m_3DSceneBgColor,
            sceneEls,
            m_3DSceneTex);
        DrawTextureAsImguiImage(m_3DSceneTex, RectDims(m_3DSceneRect));

        // draw connection lines, as appropriate
        {
            auto hover = m_MaybeHover.lock();

            auto drawLine = [&](auto const& nodePtr) {
                for (auto const& child : nodePtr->GetChildren()) {
                    if (child == assignee) {
                        if (hover && hover != assignee) {
                            // do not draw: the max-strength line will be drawn later
                            continue;
                        } else {
                            // draw a medium-strength line for the existing connection
                            DrawConnectionLine(ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 0.33f}), *nodePtr, *child);
                        }
                    } else {
                        // draw a faint connection line (these lines are non-participating)
                        DrawConnectionLine(ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 0.05f}), *nodePtr, *child);
                    }
                }
            };

            if (hover && hover != assignee) {
                // draw max-strength line for user's current interest
                DrawConnectionLine(ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 1.0f}), *hover, *assignee);
            }

            ForEachNode(m_ModelRoot, drawLine);
        }
    }

    void HoverTest_NormalMode()
    {
        if (!IsMouseOverRender()) {
            m_MaybeHover.reset();
            return;
        }

        if (ImGuizmo::IsUsing()) {
            return;
        }

        m_MaybeHover = DoHittest(ImGui::GetMousePos(), false, m_IsBodiesInteractable, m_IsMeshesInteractable);

        bool lcReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        bool rcReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
        bool shiftDown = ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT);
        bool isUsingGizmo = ImGuizmo::IsUsing();

        if (!m_MaybeHover && lcReleased && !isUsingGizmo && !shiftDown) {
            DeSelectAll();
            return;
        }

        if (m_MaybeHover && lcReleased && !isUsingGizmo) {
            if (!shiftDown) {
                DeSelectAll();
            }
            SelectHover();
            return;
        }

        if (m_MaybeHover && rcReleased && !isUsingGizmo) {
            OpenHoverContextMenu();
        }

        DrawHoverTooltip();
    }

    void Draw3dViewer_NormalMode()
    {
        HoverTest_NormalMode();

        std::vector<DrawableThing> sceneEls;

        // add DAG decorations
        ForEachNode(m_ModelRoot, [&](auto const& node) {
            if (auto const* bodyPtr = dynamic_cast<BodyNode const*>(node.get())) {
                sceneEls.push_back(GenerateSphereSceneEl(*bodyPtr, m_Colors.body));
            } else if (auto const* meshPtr = dynamic_cast<MeshNode const*>(node.get())) {
                sceneEls.push_back(GenerateMeshSceneEl(*meshPtr, m_Colors.mesh));
            }
        });

        if (m_IsShowingFloor) {
            sceneEls.push_back(GenerateFloor());
        }

        if (m_IsShowingGround) {
            sceneEls.push_back(GenerateGroundSceneEl());
        }

        std::sort(sceneEls.begin(), sceneEls.end(), OptimalDrawOrder);

        DrawScene(
            RectDims(m_3DSceneRect),
            m_3DSceneCamera.getProjMtx(RectAspectRatio(m_3DSceneRect)),
            m_3DSceneCamera.getViewMtx(),
            m_3DSceneCamera.getPos(),
            m_3DSceneLightDir,
            m_3DSceneLightColor,
            m_3DSceneBgColor,
            sceneEls,
            m_3DSceneTex);
        DrawTextureAsImguiImage(m_3DSceneTex, RectDims(m_3DSceneRect));
        Draw3DManipulators();
        DrawConnectionLines(ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 0.3f}));
        DrawContextMenu();
    }

    Rect ContentRegionAvailRect()
    {
        glm::vec2 topLeft = ImGui::GetCursorScreenPos();
        glm::vec2 dims = ImGui::GetContentRegionAvail();
        glm::vec2 bottomRight = topLeft + dims;

        return Rect{topLeft, bottomRight};
    }

    void Draw3DViewer()
    {
        m_3DSceneRect = ContentRegionAvailRect();

        if (!IsInAssignmentMode()) {
            Draw3dViewer_NormalMode();
        } else {
            Draw3DViewer_AssignmentMode();
        }
    }

    void onMount()
    {
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void tick(float dt)
    {
        float dotMotionsPerSecond = 0.35f;
        float ignoreMe;
        m_AnimationPercentage = std::modf(m_AnimationPercentage + std::modf(dotMotionsPerSecond * dt, &ignoreMe), &ignoreMe);

        if (IsMouseOverRender()) {
            UpdateCameraFromImGuiUserInput();
        }
        UpdateFromImGuiKeyboardState();

        PopMeshLoader();

        if (m_MaybeOutputModel) {
            auto mes = std::make_shared<MainEditorState>(std::move(m_MaybeOutputModel));
            App::cur().requestTransition<ModelEditorScreen>(mes);
        }
    }

    void onEvent(SDL_Event const& e)
    {
        if (osc::ImGuiOnEvent(e)) {
            return;
        }

        if (e.type == SDL_DROPFILE && e.drop.file != nullptr) {
            PushMeshLoadRequest(m_ModelRoot, std::filesystem::path{e.drop.file});
        }
    }

    void draw()
    {
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        osc::ImGuiNewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

        // must be called at the start of each frame
        ImGuizmo::BeginFrame();

        // draw sidebar in a (moveable + resizeable) ImGui panel
        if (ImGui::Begin("wizardstep2sidebar")) {
            DrawSidebar();
        }
        ImGui::End();

        // draw 3D viewer in a (moveable + resizeable) ImGui panel
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
        if (ImGui::Begin("wizardsstep2viewer")) {
            ImGui::PopStyleVar();
            Draw3DViewer();
        } else {
            ImGui::PopStyleVar();
        }
        ImGui::End();

        osc::ImGuiRender();
    }
};
using Impl = osc::MeshesToModelWizardScreen::Impl;
*/


// top-level screen implementation
//
// this effectively just feeds the underlying state machine pattern established by
// the `ModelWizardState` class
struct osc::MeshesToModelWizardScreen::Impl final {
public:
    Impl() :
        m_SharedData{},
        m_CurrentState{std::make_unique<StandardModeState>(m_SharedData)}
    {
    }

    Impl(std::vector<std::filesystem::path> meshPaths) :
        m_SharedData{std::move(meshPaths)},
        m_CurrentState{std::make_unique<StandardModeState>(m_SharedData)}
    {
    }

    void onMount()
    {
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (osc::ImGuiOnEvent(e)) {
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
    }

private:
    SharedData m_SharedData;
    std::unique_ptr<ModelWizardState> m_CurrentState;
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
