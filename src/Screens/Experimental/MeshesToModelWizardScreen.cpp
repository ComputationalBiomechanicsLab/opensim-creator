#include "MeshesToModelWizardScreen.hpp"

#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/MainEditorState.hpp"
#include "src/Styling.hpp"
#include "src/3D/BVH.hpp"
#include "src/3D/Shaders/InstancedGouraudColorShader.hpp"
#include "src/3D/Shaders/EdgeDetectionShader.hpp"
#include "src/3D/Shaders/GouraudShader.hpp"
#include "src/3D/Shaders/SolidColorShader.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
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
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
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

// private impl details
namespace {

    // base radius of rendered spheres (e.g. for bodies)
    constexpr float g_SphereRadius = 0.025f;


    class Node;

    // a mesh loading request
    class MeshLoadRequest final {
    public:
        MeshLoadRequest(std::weak_ptr<Node> prefAttachmentPoint, std::filesystem::path path) :
            m_PreferredAttachmentPoint{prefAttachmentPoint},
            m_Path{path}
        {
        }

        std::filesystem::path const& GetPath() const { return m_Path; }
        std::weak_ptr<Node> GetPreferredAttachmentPoint() const { return m_PreferredAttachmentPoint; }

    private:
        std::weak_ptr<Node> m_PreferredAttachmentPoint;
        std::filesystem::path m_Path;
    };

    // an OK response to a mesh loading request
    class MeshLoadOKResponse final {
    public:
        MeshLoadOKResponse(MeshLoadRequest const& req, std::shared_ptr<Mesh> mesh) :
            m_Req{req}, m_Mesh{mesh}
        {
            OSC_ASSERT_ALWAYS(m_Mesh != nullptr);
        }

        std::filesystem::path const& GetPath() const { return m_Req.GetPath(); }
        std::weak_ptr<Node> GetPreferredAttachmentPoint() const { return m_Req.GetPreferredAttachmentPoint(); }
        std::shared_ptr<Mesh> GetMeshPtr() const { return m_Mesh; }

    private:
        MeshLoadRequest m_Req;
        std::shared_ptr<Mesh> m_Mesh;
    };

    // an ERROR response to a mesh loading request
    class MeshLoadErrorResponse final {
    public:
        MeshLoadErrorResponse(MeshLoadRequest const& req, std::string error) :
            m_Req{req}, m_Error{error}
        {
        }

        std::filesystem::path const& GetPath() const { return m_Req.GetPath(); }
        char const* what() const { return m_Error.c_str(); }

    private:
        MeshLoadRequest m_Req;
        std::string m_Error;
    };

    // a response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // responds to a mesh loading request
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest const& msg) noexcept
    {
        try {
            auto mesh = std::make_shared<Mesh>(SimTKLoadMesh(msg.GetPath()));  // can throw
            return MeshLoadOKResponse{msg, std::move(mesh)};
        } catch (std::exception const& ex) {
            return MeshLoadErrorResponse{msg, ex.what()};
        }
    }

    // a mesh-loading background worker
    using Mesh_loader = spsc::Worker<MeshLoadRequest, MeshLoadResponse, decltype(respondToMeshloadRequest)>;

    // returns a const-ized version of a non-const `span`
    nonstd::span<std::shared_ptr<Node const> const> ToConstSpan(std::vector<std::shared_ptr<Node>> const& v)
    {
        auto ptr = reinterpret_cast<std::shared_ptr<Node const> const*>(v.data());
        return {ptr, ptr + v.size()};
    }

    // returns true if exactly one instance of `el` was removed from `els`
    template<typename El>
    bool RemoveFirstEquivalentEl(std::vector<El>& els, El const& el)
    {
        for (auto it = els.begin(); it != els.end(); ++it) {
            if (*it == el) {
                els.erase(it);
                return true;
            }
        }
        return false;
    }

    // returns the path's filename with the extension removed
    static std::string FileNameWithoutExtension(std::filesystem::path const& p)
    {
        return p.filename().replace_extension("").string();
    }

    // the type of joint used in a frame-to-frame joint
    using BofJointType = int;
    enum BofJointType_
    {
        BofJointType_WeldJoint = 0,
        BofJointType_PinJoint,
        BofJointType_NumJointTypes,
    };

    // returns string representations of all joint types (enum)
    std::array<std::string const, BofJointType_NumJointTypes> const& GetJointTypeStrings()
    {
        static std::array<std::string const, BofJointType_NumJointTypes> const g_Names = {"WeldJoint", "PinJoint"};
        return g_Names;
    }

    // returns CString representations of all joint types (enum)
    std::array<char const*, BofJointType_NumJointTypes> GetJointTypeCStrings()
    {
        auto strings = GetJointTypeStrings();
        std::array<char const*, BofJointType_NumJointTypes> rv;
        for (size_t i = 0; i < BofJointType_NumJointTypes; ++i) {
            rv[i] = strings[i].c_str();
        }
        return rv;
    }

    // returns a string representation of a joint type
    std::string const& ToString(BofJointType jt)
    {
        OSC_ASSERT_ALWAYS(0 <= jt && jt < BofJointType_NumJointTypes);
        return GetJointTypeStrings()[jt];
    }

    // represents a joint between a body-or-frame (BoF) and its parent
    class BofJoint final {
    public:
        BofJoint(std::shared_ptr<Node> other) :
            m_Other{other},
            m_JointType{BofJointType_WeldJoint},
            m_MaybeUserDefinedPivotPoint{std::nullopt},
            m_MaybeUserDefinedJointName{}
        {
            OSC_ASSERT_ALWAYS(other != nullptr);
        }

        std::shared_ptr<Node> UpdOther() { auto p = m_Other.lock(); OSC_ASSERT_ALWAYS(p != nullptr && "this node probably leaked"); return p; }
        std::shared_ptr<Node> GetOther() const { auto p = m_Other.lock(); OSC_ASSERT_ALWAYS(p != nullptr && "this node probably leaked"); return p; }
        void SetOther(std::shared_ptr<Node> newOther) { OSC_ASSERT_ALWAYS(newOther != nullptr); m_Other = newOther; }
        BofJointType GetJointType() const { return m_JointType; }
        void SetJointType(BofJointType newJointType) { m_JointType = newJointType; }
        std::string const& GetJointTypeName() const { return ToString(m_JointType); }
        bool HasUserDefinedPivotPoint() const { return m_MaybeUserDefinedPivotPoint != std::nullopt; }
        std::optional<glm::vec3> GetUserDefinedPivotPoint() const { return m_MaybeUserDefinedPivotPoint; }
        void SetUserDefinedPivotPoint(glm::vec3 const& newPivotPoint) { m_MaybeUserDefinedPivotPoint = newPivotPoint; }
        bool HasUserDefinedJointName() const { return !m_MaybeUserDefinedJointName.empty(); }
        std::string const& GetUserDefinedJointName() const { return m_MaybeUserDefinedJointName; }
        void SetUserDefinedJointName(std::string newName) { m_MaybeUserDefinedJointName = std::move(newName); }
        std::string const& GetName() const { return HasUserDefinedJointName() ? GetUserDefinedJointName() : GetJointTypeName(); }

    private:
        std::weak_ptr<Node> m_Other;
        BofJointType m_JointType;
        std::optional<glm::vec3> m_MaybeUserDefinedPivotPoint;
        std::string m_MaybeUserDefinedJointName;
    };

    // the tree datastructure that the user is building
    class Node {
    public:
        // parent
        virtual bool CanHaveParent() const = 0;
        virtual std::shared_ptr<Node> UpdParentPtr() = 0;
        virtual std::shared_ptr<Node const> GetParentPtr() const = 0;
        virtual void SetParent(std::shared_ptr<Node>) = 0;

        // children
        virtual bool CanHaveChildren() const = 0;
        virtual nonstd::span<std::shared_ptr<Node const> const> GetChildren() const = 0;
        virtual nonstd::span<std::shared_ptr<Node>> UpdChildren() = 0;
        virtual void ClearChildren() = 0;
        virtual void AddChild(std::shared_ptr<Node>) = 0;
        virtual bool RemoveChild(std::shared_ptr<Node>) = 0;

        // position/bounds/xform
        virtual glm::vec3 GetPos() const = 0;
        virtual AABB GetBounds(float sceneScaleFactor) const = 0;
        virtual glm::mat4 GetModelMatrix(float sceneScaleFactor) const = 0;
        virtual void ApplyRotation(glm::mat4 const&) = 0;
        virtual void ApplyScale(glm::mat4 const&) = 0;
        virtual void ApplyTranslation(glm::mat4 const&) = 0;
        virtual glm::vec3 GetCenterPoint() const = 0;
        virtual RayCollision DoHittest(float sceneScaleFactor, Line const&) const = 0;

        // name
        virtual std::string const& GetName() const = 0;
        virtual void SetName(char const*) = 0;

        // helpers that use the virtual interface
        Node& UpdParent() { auto p = UpdParentPtr(); OSC_ASSERT_ALWAYS(p != nullptr); return *p; }
        Node const& GetParent() const { auto p = GetParentPtr(); OSC_ASSERT_ALWAYS(p != nullptr); return *p; }
        char const* GetNameCStr() const { return GetName().c_str(); }
        bool HasParent() const { return CanHaveParent() && GetParentPtr() != nullptr; }
        bool HasChildren() const { return CanHaveChildren() && !GetChildren().empty(); }
    };

    // node representing the model's ground (the root of the model tree)
    class GroundNode final : public Node {
    public:
        GroundNode() : m_Children{} {}

        Sphere GetSphere(float sceneScaleFactor) const
        {
            return Sphere{GetPos(), sceneScaleFactor * g_SphereRadius};
        }

        bool CanHaveParent() const override { return false; }
        std::shared_ptr<Node> UpdParentPtr() override { return nullptr; }
        std::shared_ptr<Node const> GetParentPtr() const override { return nullptr; }
        void SetParent(std::shared_ptr<Node>) override { throw std::runtime_error{"cannot call SetParent on GroundNode"}; }

        bool CanHaveChildren() const override { return true; }
        nonstd::span<std::shared_ptr<Node const> const> GetChildren() const override { return ToConstSpan(m_Children); }
        nonstd::span<std::shared_ptr<Node>> UpdChildren() override { return m_Children; }
        void ClearChildren() override { m_Children.clear(); }
        void AddChild(std::shared_ptr<Node> n) override { m_Children.push_back(n); }
        bool RemoveChild(std::shared_ptr<Node> n) override { return RemoveFirstEquivalentEl(m_Children, n); }

        glm::vec3 GetPos() const override { return {0.0f, 0.0f, 0.0f}; }
        AABB GetBounds(float sceneScaleFactor) const override { return SphereToAABB(GetSphere(sceneScaleFactor)); }
        glm::mat4 GetModelMatrix(float) const override { return glm::mat4{1.0f}; }
        void ApplyRotation(glm::mat4 const&) override {}
        void ApplyScale(glm::mat4 const&) override {}
        void ApplyTranslation(glm::mat4 const&) override {}
        glm::vec3 GetCenterPoint() const override { return {0.0f, 0.0f, 0.0f}; }
        RayCollision DoHittest(float sceneScaleFactor, Line const& ray) const override { return GetRayCollisionSphere(ray, GetSphere(sceneScaleFactor)); }

        std::string const& GetName() const override { static std::string const name{"ground"}; return name; }
        void SetName(char const*) override {}

    private:
        std::vector<std::shared_ptr<Node>> m_Children;
    };

    // node representing a body-or-frame (BOF)
    class BofNode final : public Node {
    public:
        BofNode(std::shared_ptr<Node> parent, bool isBody, glm::vec3 const& pos) :
            m_Joint{parent},
            m_Children{},
            m_IsBody{isBody},
            m_Pos{pos},
            m_Name{}
        {
            static std::atomic<int> g_LatestBodyIdx = 0;
            m_Name = (m_IsBody ? std::string{"body"} : std::string{"physicaloffsetframe"}) + std::to_string(g_LatestBodyIdx++);
        }

        bool IsBody() const { return m_IsBody; }
        void SetPos(glm::vec3 const& newPos) { m_Pos = newPos; }
        Sphere GetSphere(float sceneScaleFactor) const
        {
            return Sphere{m_Pos, sceneScaleFactor * g_SphereRadius};
        }
        BofJointType GetJointType() const { return m_Joint.GetJointType(); }
        void SetJointType(BofJointType newJointType) { m_Joint.SetJointType(newJointType); }
        char const* GetJointTypeNameCStr() const { return m_Joint.GetJointTypeName().c_str(); }
        char const* GetJointNameCStr() const { return m_Joint.GetName().c_str(); }
        bool HasUserDefinedJointName() const { return m_Joint.HasUserDefinedJointName(); }
        std::string const& GetUserDefinedJointName() const { return m_Joint.GetUserDefinedJointName(); }
        void SetUserDefinedJointName(std::string newJointName) { m_Joint.SetUserDefinedJointName(std::move(newJointName)); }

        bool CanHaveParent() const override { return true; }
        std::shared_ptr<Node> UpdParentPtr() override { return m_Joint.UpdOther(); }
        std::shared_ptr<Node const> GetParentPtr() const override { return m_Joint.GetOther(); }
        void SetParent(std::shared_ptr<Node> newParent) override { m_Joint.SetOther(newParent); }

        bool CanHaveChildren() const override { return true; }
        nonstd::span<std::shared_ptr<Node const> const> GetChildren() const override { return ToConstSpan(m_Children); }
        nonstd::span<std::shared_ptr<Node>> UpdChildren() override { return m_Children; }
        void ClearChildren() override { m_Children.clear(); }
        void AddChild(std::shared_ptr<Node> child) override { m_Children.push_back(child); }
        bool RemoveChild(std::shared_ptr<Node> child) override { return RemoveFirstEquivalentEl(m_Children, child); }

        glm::vec3 GetPos() const override { return m_Pos; }
        AABB GetBounds(float sceneScaleFactor) const override { Sphere s{m_Pos, sceneScaleFactor * g_SphereRadius}; return SphereToAABB(s); }
        glm::mat4 GetModelMatrix(float sceneScaleFactor) const override
        {
            float r = sceneScaleFactor * g_SphereRadius;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{r,r,r});
            glm::mat4 translator = glm::translate(glm::mat4{1.0f}, m_Pos);
            return translator * scaler;
        }
        void ApplyRotation(glm::mat4 const&) override {}
        void ApplyScale(glm::mat4 const&) override {}
        void ApplyTranslation(glm::mat4 const& translationMtx) override { m_Pos = glm::vec3{translationMtx * glm::vec4{m_Pos, 1.0f}}; }
        glm::vec3 GetCenterPoint() const override { return m_Pos; }
        RayCollision DoHittest(float sceneScaleFactor, Line const& ray) const override { return GetRayCollisionSphere(ray, GetSphere(sceneScaleFactor)); }

        std::string const& GetName() const override { return m_Name; }
        void SetName(char const* newName) override { m_Name = newName; }

    private:
        BofJoint m_Joint;
        std::vector<std::shared_ptr<Node>> m_Children;
        bool m_IsBody;
        glm::vec3 m_Pos;
        std::string m_Name;
    };

    // node representing a decorative mesh
    class MeshNode final : public Node {
    public:
        MeshNode(std::weak_ptr<Node> parent, std::shared_ptr<Mesh> mesh, std::filesystem::path const& path) :
            m_Parent{parent},
            m_ModelMtx{1.0f},
            m_Mesh{mesh},
            m_Path{path},
            m_Name{FileNameWithoutExtension(m_Path)}
        {
        }

        glm::mat4 GetModelMatrix() const { return m_ModelMtx; }
        std::shared_ptr<Mesh> GetMesh() const { return m_Mesh; }
        std::filesystem::path const& GetPath() const { return m_Path; }

        bool CanHaveParent() const override { return true; }
        std::shared_ptr<Node> UpdParentPtr() override { auto p = m_Parent.lock(); OSC_ASSERT_ALWAYS(p != nullptr && "this node probably leaked"); return p; }
        std::shared_ptr<Node const> GetParentPtr() const override { auto p = m_Parent.lock(); OSC_ASSERT_ALWAYS(p != nullptr && "this node probably leaked"); return p; }
        void SetParent(std::shared_ptr<Node> newParent) override { m_Parent = newParent; }
        bool CanHaveChildren() const override { return false; }
        nonstd::span<std::shared_ptr<Node const> const> GetChildren() const override { return {}; }
        nonstd::span<std::shared_ptr<Node>> UpdChildren() override { return {}; }
        void ClearChildren() override {}
        void AddChild(std::shared_ptr<Node>) override {}
        bool RemoveChild(std::shared_ptr<Node>) override { return false; }
        glm::vec3 GetPos() const override { return m_ModelMtx[3]; }
        AABB GetBounds(float) const override
        {
            // scale factor is ignored for meshes
            return AABBApplyXform(m_Mesh->getAABB(), m_ModelMtx);
        }
        glm::mat4 GetModelMatrix(float) const override { return m_ModelMtx; }
        void ApplyRotation(glm::mat4 const& mtx) override { m_ModelMtx = mtx * m_ModelMtx; }
        void ApplyScale(glm::mat4 const& mtx) override { m_ModelMtx = m_ModelMtx * mtx; }
        void ApplyTranslation(glm::mat4 const& mtx) override { m_ModelMtx = mtx * m_ModelMtx; }
        glm::vec3 GetCenterPoint() const override { return AABBCenter(AABBApplyXform(m_Mesh->getAABB(), m_ModelMtx)); }
        RayCollision DoHittest(float sceneScaleFactor, Line const& ray) const override
        {
            // do a fast ray-to-AABB collision test
            RayCollision rayAABBCollision = GetRayCollisionAABB(ray, GetBounds(sceneScaleFactor));
            if (!rayAABBCollision.hit) {
                return rayAABBCollision;
            }

            // it *may* have hit: refine by doing a slower ray-to-triangle test
            glm::mat4 world2model = glm::inverse(GetModelMatrix());
            Line rayModel = LineApplyXform(ray, world2model);
            RayCollision rayTriangleCollision = m_Mesh->getClosestRayTriangleCollision(rayModel);

            return rayTriangleCollision;
        }
        std::string const& GetName() const override { return m_Name; }
        void SetName(char const* newName) override { m_Name = newName; }

    private:
        std::weak_ptr<Node> m_Parent;
        glm::mat4 m_ModelMtx;
        std::shared_ptr<Mesh> m_Mesh;
        std::filesystem::path m_Path;
        std::string m_Name;
    };

    // returns true if the provided reference of type `T` was instantiated as type `U`
    template<typename U, typename T>
    bool IsType(T const& v)
    {
        return typeid(v) == typeid(U);
    }

    // returns true if `node` is, or is a child of, a `GroundNode`
    bool IsInclusiveChildOfGround(Node const& node)
    {
        if (IsType<GroundNode>(node)) {
            return true;
        }

        for (auto ptr = node.GetParentPtr(); ptr; ptr = ptr->GetParentPtr()) {
            if (IsType<GroundNode>(*ptr)) {
                return true;
            }
        }

        return false;
    }

    // returns true if `node` is a direct descendant (child) of `ancestor`
    bool IsDirectDescendantOf(Node const* ancestor, Node const* node)
    {
        return node->GetParentPtr().get() == ancestor;
    }

    // returns true if `node` is a descendant (child, grandchild, etc.) of `ancestor`
    bool IsInclusiveDescendantOf(Node const* ancestor, Node const* node)
    {
        OSC_ASSERT_ALWAYS(ancestor != nullptr);
        OSC_ASSERT_ALWAYS(node != nullptr);

        for (auto ptr = node->GetParentPtr(); ptr; ptr = ptr->GetParentPtr()) {
            if (ptr.get() == ancestor) {
                return true;
            }
        }

        return false;
    }

    // returns true if `node` is a direct ancestor (parent) of `descendant`
    bool IsDirectAncestorOf(Node const* descendant, Node const* node)
    {
        OSC_ASSERT_ALWAYS(descendant != nullptr);
        OSC_ASSERT_ALWAYS(node != nullptr);

        auto children = node->GetChildren();
        auto pointsToNode = [node](auto const& child) { return child.get() == node; };

        return AnyOf(children, pointsToNode);
    }

    // returns true if `node` is, or is an ancestor (parent, grandparent, etc.), of `descendant`
    bool IsAncestorOf(Node const* descendant, Node const* node)
    {
        if (IsDirectAncestorOf(descendant, node)) {
            return true;
        }

        for (auto const& child : node->GetChildren()) {
            if (IsAncestorOf(descendant, child.get())) {
                return true;
            }
        }

        return false;
    }

    // calls `f` on each node (inclusive) of `n`
    template<typename Callable>
    void ForEachNode(std::shared_ptr<Node> n, Callable f)
    {
        f(n);
        for (auto const& child : n->UpdChildren()) {
            ForEachNode(child, f);
        }
    }
}

// attaches a `MeshNode` to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
static void AttachMeshNodeToModel(OpenSim::Model& model,
                                  MeshNode const& node,
                                  OpenSim::PhysicalFrame& parentPhysFrame)
{

    // ensure the model is up-to-date (the model-generation step modifies the model
    // quite a bit)
    model.finalizeFromProperties();
    model.finalizeConnections();
    SimTK::State s = model.initSystem();
    model.realizePosition(s);

    // get relevant transform matrices
    SimTK::Transform parentToGroundMtx = parentPhysFrame.getTransformInGround(s);
    SimTK::Transform groundToParentMtx = parentToGroundMtx.invert();

    // create a POF that attaches to the body
    //
    // this is necessary to independently transform the mesh relative
    // to the parent's transform (the mesh is currently transformed relative
    // to ground)
    OpenSim::PhysicalOffsetFrame* meshPhysOffsetFrame = new OpenSim::PhysicalOffsetFrame{};
    meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
    meshPhysOffsetFrame->setName(node.GetName() + "_offset");

    // without setting `setObjectTransform`, the mesh will be subjected to
    // the POF's object-to-ground transform. so vertices in the matrix are
    // already in "object space" and we want to figure out how to transform
    // them as if they were in our current (world) space
    SimTK::Transform mmtx = SimTKTransformFromMat4x3(node.GetModelMatrix());

    meshPhysOffsetFrame->setOffsetTransform(groundToParentMtx * mmtx);
    parentPhysFrame.addComponent(meshPhysOffsetFrame);

    // attach mesh to the POF
    auto mesh = std::make_unique<OpenSim::Mesh>(node.GetPath());
    mesh->setName(node.GetName());
    meshPhysOffsetFrame->attachGeometry(mesh.release());
}

static void RecursivelyAddNodeToModel(OpenSim::Model& model,
                                      Node const& node,
                                      OpenSim::PhysicalFrame& parentPhysFrame);

// adds a `BofNode` to a parent `OpenSim::PhysicalFrame` that is part of an
// `OpenSim::Model` and then recursively adds the `BofNode`'s children
static void RecursivelyAddBodyNodeToModel(OpenSim::Model& model,
                                          BofNode const& node,
                                          OpenSim::PhysicalFrame& parentPhysFrame)
{
    if (!node.IsBody()) {
        throw std::runtime_error{"POFs: TODO"};
    }

    auto const& parent = node.GetParent();

    // joint that connects the POF to the body
    OpenSim::Joint* joint = nullptr;
    switch (node.GetJointType()) {
    case BofJointType_WeldJoint:
        joint = new OpenSim::WeldJoint{};
        break;
    case BofJointType_PinJoint:
        joint = new OpenSim::PinJoint{};
        break;
    default:
        throw std::runtime_error{"unknown joint type provided to model generator"};
    }

    joint->setName(node.HasUserDefinedJointName() ? node.GetUserDefinedJointName() : node.GetName() + "_to_" + parent.GetName());

    // the body
    OpenSim::Body* body = new OpenSim::Body{};
    body->setName(node.GetName());
    body->setMass(1.0);

    // the POF that is offset from the parent physical frame
    OpenSim::PhysicalOffsetFrame* pof = new OpenSim::PhysicalOffsetFrame{};
    pof->setName(parent.GetName() + "_offset");

    glm::vec3 worldParentPos = parent.GetPos();
    glm::vec3 worldBOFPos = node.GetPos();
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

    // done! - time to recurse
    for (auto const& child : node.GetChildren()) {
        RecursivelyAddNodeToModel(model, *child, *body);
    }
}

// recursively adds a node to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
static void RecursivelyAddNodeToModel(OpenSim::Model& model,
                                      Node const& node,
                                      OpenSim::PhysicalFrame& parentPhysFrame)
{
    if (auto const* bofnode = dynamic_cast<BofNode const*>(&node)) {
        RecursivelyAddBodyNodeToModel(model, *bofnode, parentPhysFrame);
    } else if (auto const* meshnode = dynamic_cast<MeshNode const*>(&node)) {
        AttachMeshNodeToModel(model, *meshnode, parentPhysFrame);
    }
}

// populates `issues` with any issues that are dectected in the `Node` (recursive)
static bool GetDagIssuesRecursive(Node const& node, std::vector<std::string>& issues)
{
    bool rv = false;

    if (!IsInclusiveChildOfGround(node)) {
        issues.push_back("detected body or frame that is not connected to ground");
        rv = true;
    }

    for (auto const& child : node.GetChildren()) {
        if (GetDagIssuesRecursive(*child, issues)) {
            rv = true;
        }
    }

    return rv;
}

// map a DAG onto an OpenSim model
static std::unique_ptr<OpenSim::Model> CreateModelFromDag(GroundNode const& root,
                                                          std::vector<std::string>& issues)
{
    issues.clear();
    if (GetDagIssuesRecursive(root, issues)) {
        log::error("cannot create an osim model: advancement issues detected");
        return nullptr;
    }

    // find nodes directly connect to ground and start recursing from there
    auto rv = std::make_unique<OpenSim::Model>();
    for (auto const& child : root.GetChildren()) {
        RecursivelyAddNodeToModel(*rv, *child, rv->updGround());
    }
    return rv;
}

// draw an ImGui color picker for an OSC Rgba32
static void Rgba32_ColorEdit4(char const* label, Rgba32* rgba)
{
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

// generate a quad used for rendering the chequered floor
static Mesh generateFloorMesh()
{
    Mesh m{GenTexturedQuad()};
    m.scaleTexCoords(200.0f);
    return m;
}

// synchronously prompts the user to select multiple mesh files through
// a native OS file dialog
static std::vector<std::filesystem::path> PromptUserForMeshFiles()
{
    nfdpathset_t s{};
    nfdresult_t result = NFD_OpenDialogMultiple("obj,vtp,stl", nullptr, &s);

    std::vector<std::filesystem::path> rv;
    if (result == NFD_OKAY) {
        OSC_SCOPE_GUARD({ NFD_PathSet_Free(&s); });

        size_t len = NFD_PathSet_GetCount(&s);
        rv.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            rv.push_back(NFD_PathSet_GetPath(&s, i));
        }
    } else if (result == NFD_CANCEL) {
    } else {
        log::error("NFD_OpenDialogMultiple error: %s", NFD_GetError());
    }

    return rv;
}

namespace {
    static gl::RenderBuffer MultisampledRenderBuffer(int samples, GLenum format, glm::ivec2 dims)
    {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, dims.x, dims.y);
        return rv;
    }

    static gl::RenderBuffer RenderBuffer(GLenum format, glm::ivec2 dims)
    {
        gl::RenderBuffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorage(GL_RENDERBUFFER, format, dims.x, dims.y);
        return rv;
    }

    static void SceneTex(gl::Texture2D& out, GLint level, GLint internalFormat, glm::ivec2 dims, GLenum format, GLenum type)
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

    static gl::Texture2D SceneTex(GLint level, GLint internalFormat, glm::ivec2 dims, GLenum format, GLenum type)
    {
        gl::Texture2D rv;
        SceneTex(rv, level, internalFormat, dims, format, type);
        return rv;
    }

    struct FboBinding {
        virtual void Bind() = 0;
    };

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

    template<typename... Binding>
    gl::FrameBuffer FrameBufferWithBindings(Binding... bindings)
    {
        gl::FrameBuffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        (bindings.Bind(), ...);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
        return rv;
    }

    // thing being drawn in the scene
    struct DrawableThing final {
        std::shared_ptr<Mesh> mesh;
        glm::mat4x3 modelMatrix;
        glm::mat3x3 normalMatrix;
        glm::vec4 color;
        float rimColor;
        std::shared_ptr<gl::Texture2D> maybeDiffuseTex;
    };

    static bool OptimalDrawOrder(DrawableThing const& a, DrawableThing const& b)
    {
        return std::tie(b.color.a, b.mesh) < std::tie(a.color.a, a.mesh);
    }

    // instance on the GPU
    struct SceneGPUInstanceData final {
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 rgba;
    };

    static void DrawScene(glm::ivec2 dims,
                          glm::mat4 const& projMat,
                          glm::mat4 const& viewMat,
                          glm::vec3 const& viewPos,
                          glm::vec3 const& lightDir,
                          glm::vec3 const& lightCol,
                          glm::vec4 const& bgCol,
                          nonstd::span<DrawableThing const> drawables,
                          gl::Texture2D& outSceneTex)
    {

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

        SceneTex(outSceneTex, 0, GL_RGBA, dims, GL_RGBA, GL_UNSIGNED_BYTE);
        gl::FrameBuffer outputFBO = FrameBufferWithBindings(
            TexBinding{GL_COLOR_ATTACHMENT0, outSceneTex, 0}
        );

        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFBO);
        gl::BlitFramebuffer(0, 0, dims.x, dims.y, 0, 0, dims.x, dims.y, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // draw rims directly over the output texture
        if (true) {
            gl::Texture2D rimsTex;
            SceneTex(rimsTex, 0, GL_RED, dims, GL_RED, GL_UNSIGNED_BYTE);
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

    // node that is currently hovered over by the user's mouse and it's 3D worldspace location
    class Hover {
    public:
        Hover() : m_Ptr{}, m_Pos{} {}
        Hover(std::weak_ptr<Node> ptr, glm::vec3 const& loc) : m_Ptr{std::move(ptr)}, m_Pos{loc} {}

        glm::vec3 const& getPos() const { return m_Pos; }

        std::shared_ptr<Node> lock() const { return m_Ptr.lock(); }

        template<typename T>
        std::shared_ptr<T> lockDowncasted() const { return std::dynamic_pointer_cast<T>(m_Ptr.lock()); }

        operator bool () const { return m_Ptr.lock() != nullptr; }

        bool operator==(Node const& other) const { return m_Ptr.lock().get() == &other; }

        bool operator==(std::shared_ptr<Node> other) const { return m_Ptr.lock() == other; }

        void reset()
        {
            m_Ptr.reset();
            m_Pos = {};
        }

    private:
        std::weak_ptr<Node> m_Ptr;
        glm::vec3 m_Pos;
    };
}

struct osc::MeshesToModelWizardScreen::Impl final {

    // loader that loads mesh data in a background thread
    Mesh_loader m_MeshLoader = Mesh_loader::create(respondToMeshloadRequest);

    // mesh used by bodies/frames (BOFs)
    std::shared_ptr<Mesh> m_SphereMesh = std::make_shared<Mesh>(GenUntexturedUVSphere(12, 12));

    // model tree the user is editing
    std::shared_ptr<GroundNode> m_ModelRoot = std::make_shared<GroundNode>();

    // currently-selected tree nodes
    std::unordered_set<std::shared_ptr<Node>> m_SelectedNodes = {};

    // node that is currently hovered over by the user's mouse and it's 3D worldspace location
    Hover m_MaybeHover;

    // node that is currently being assigned
    std::weak_ptr<Node> m_MaybeCurrentAssignment;

    // node that is currently being edited via a context menu
    Hover m_MaybeNodeOpenedInContextMenu;

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
        glm::vec4 frame = {0.0f, 1.0f, 0.0f, 1.0f};
    } m_Colors;

    // scale factor for all non-mesh, non-overlay scene elements (e.g.
    // the floor, bodies)
    //
    // this is necessary because some meshes can be extremely small/large and
    // scene elements need to be scaled accordingly (e.g. without this, a body
    // sphere end up being much larger than a mesh instance). Imagine if the
    // mesh was the leg of a fly
    float m_SceneScaleFactor = 1.0f;

    // ImGuizmo state
    struct {
        glm::mat4 mtx{};
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    } m_ImGuizmoState;

    // floor data
    struct {
        std::shared_ptr<gl::Texture2D> texture = std::make_shared<gl::Texture2D>(genChequeredFloorTexture());
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(generateFloorMesh());
    } m_Floor;

    PolarPerspectiveCamera m_3DSceneCamera = []() {
        PolarPerspectiveCamera rv;
        rv.phi = fpi4;
        rv.theta = fpi4;
        rv.radius = 5.0f;
        return rv;
    }();

    glm::vec3 m_3DSceneLightDir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 m_3DSceneLightColor = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 m_3DSceneBgColor = {0.89f, 0.89f, 0.89f, 1.0f};

    // model created by this wizard
    //
    // `nullptr` until the model is successfully created
    std::unique_ptr<OpenSim::Model> m_MaybeOutputModel = nullptr;

    // recycled by the renderer to upload instance data
    std::vector<SceneGPUInstanceData> m_3DSceneInstanceBuffer;

    // buffer of known issues
    std::vector<std::string> m_ErrorMessageBuffer;

    // clamped to [0, 1] - used for any active animations, like the dots on the connection lines
    float m_AnimationPercentage = 0.0f;

    // true if a chequered floor should be drawn
    bool m_IsShowingFloor = true;

    // true if meshes should be drawn
    bool m_IsShowingMeshes = true;

    // true if ground should be drawn
    bool m_IsShowingGround = true;

    // true if bofs should be drawn
    bool m_IsShowingBofs = true;

    // true if all connection lines between entities should be
    // drawn, rather than just *hovered* entities
    bool m_IsShowingAllConnectionLines = true;

    // true if meshes shouldn't be hoverable/clickable in the 3D scene
    bool m_IsMeshesInteractable = true;

    // true if BOFs shouldn't be hoverable/clickable in the 3D scene
    bool m_IsBofsInteractable = true;

    // set to true after drawing the ImGui::Image
    bool m_IsRenderHovered = false;


    // --------------- methods ---------------

    Impl() = default;

    Impl(nonstd::span<std::filesystem::path> meshPaths)
    {
        PushMeshLoadRequests(m_ModelRoot, meshPaths);
    }

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

    std::shared_ptr<BofNode> AddBody(glm::vec3 const& pos)
    {
        auto bodyNode = std::make_shared<BofNode>(m_ModelRoot, true, pos);
        m_ModelRoot->AddChild(bodyNode);
        return bodyNode;
    }

    std::shared_ptr<BofNode> AddOffsetFrame(glm::vec3 const& pos)
    {
        auto pofNode = std::make_shared<BofNode>(m_ModelRoot, false, pos);
        m_ModelRoot->AddChild(pofNode);
        return pofNode;
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

        AABB sceneAABB = GetSceneAABB();
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
        auto pos = meshNode.GetPos();
        ImGui::Text("Center = (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        auto parentPos = meshNode.GetParent().GetPos();
        ImGui::Text("Center's Distance to Parent = %.2f", glm::length(pos - parentPos));
        ImGui::Text("Mouse Location = (%.2f, %.2f, %.2f)", hoverPos.x, hoverPos.y, hoverPos.z);
        ImGui::Text("Mouse Location's Distance to Parent = %.2f", glm::length(hoverPos - parentPos));
        ImGui::Unindent();
        ImGui::EndTooltip();
    }

    void DrawPfHoverTooltip(BofNode const& bofNode)
    {
        ImGui::BeginTooltip();
        if (bofNode.IsBody()) {
            ImGui::TextUnformatted("Body");
        } else {
            ImGui::TextUnformatted("PhysicalOffsetFrame");
        }
        ImGui::Indent();
        ImGui::Text("Name = %s", bofNode.GetNameCStr());
        ImGui::Text("Connected to = %s", bofNode.GetParent().GetNameCStr());
        ImGui::Text("Joint type = %s", bofNode.GetJointTypeNameCStr());
        auto pos = bofNode.GetPos();
        ImGui::Text("Pos = (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        auto parentPos = bofNode.GetParent().GetPos();
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
        } else if (auto bofPtr = dynamic_cast<BofNode*>(hover.get())) {
            DrawPfHoverTooltip(*bofPtr);
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

            if (ImGui::BeginMenu("add frame")) {
                if (ImGui::MenuItem("at mouse location")) {
                    auto addedFrame = AddOffsetFrame(pos);
                    DeSelectAll();
                    Select(addedFrame);
                }

                if (ImGui::MenuItem("at mouse location, and assign the mesh to it")) {
                    auto addedFrame = AddOffsetFrame(pos);
                    Assign(addedFrame, meshNode->UpdParentPtr());
                    Assign(meshNode, addedFrame);
                    DeSelectAll();
                    Select(addedFrame);
                }

                if (ImGui::MenuItem("at center")) {
                    auto addedFrame = AddOffsetFrame(meshNode->GetCenterPoint());
                    DeSelectAll();
                    Select(addedFrame);
                }

                if (ImGui::MenuItem("at center, and assign this mesh to it")) {
                    auto addedFrame = AddOffsetFrame(meshNode->GetCenterPoint());
                    Assign(addedFrame, meshNode->UpdParentPtr());
                    Assign(meshNode, addedFrame);
                    DeSelectAll();
                    Select(addedFrame);
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

    void DrawPfContextMenu(std::shared_ptr<BofNode> bofNode)
    {
        if (ImGui::BeginPopup("contextmenu")) {
            if (ImGui::MenuItem("add body (assigned to this one)")) {
                auto addedBody = AddBody(bofNode->GetCenterPoint());
                Assign(addedBody, bofNode);
                DeSelectAll();
                Select(addedBody);
            }

            if (ImGui::MenuItem("assign mesh(es)")) {
                PromptUserForMeshFiles(bofNode);
            }

            if (ImGui::MenuItem("focus camera on this")) {
                FocusCameraOn(*bofNode);
            }

            if (ImGui::MenuItem("reassign parent")) {
                StartAssigning(bofNode);
            }

            if (ImGui::BeginMenu("delete")) {
                if (ImGui::MenuItem("entire subtree")) {
                    DeleteSubtree(bofNode);
                }

                if (ImGui::MenuItem("just this")) {
                    DeleteNodeButReParentItsChildren(bofNode);
                }

                ImGui::EndMenu();
            }

            BofJointType jointType = bofNode->GetJointType();
            auto jointTypeStrings = GetJointTypeCStrings();
            if (ImGui::Combo("joint type", &jointType, jointTypeStrings.data(), jointTypeStrings.size())) {
                bofNode->SetJointType(jointType);
            }

            // draw name editor
            {
                char buf[256];
                std::strcpy(buf, bofNode->GetNameCStr());
                if (ImGui::InputText("set name", buf, sizeof(buf))) {
                    bofNode->SetName(buf);
                }
            }

            // draw joint name editor
            {
                char buf2[256];
                std::strcpy(buf2, bofNode->GetJointNameCStr());
                if (ImGui::InputText("set joint name", buf2, sizeof(buf2))) {
                    bofNode->SetUserDefinedJointName(buf2);
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
        } else if (auto bofPtr = std::dynamic_pointer_cast<BofNode>(editedNode)) {
            DrawPfContextMenu(bofPtr);
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

    Hover DoHittest(glm::vec2 pos, bool isGroundInteractable, bool isBofsInteractable, bool isMeshesInteractable) const
    {
        if (!PointIsInRect(m_3DSceneRect, pos)) {
            return {};
        }

        Line ray = m_3DSceneCamera.unprojectTopLeftPosToWorldRay(pos - m_3DSceneRect.p1, RectDims(m_3DSceneRect));

        std::shared_ptr<Node> closest = nullptr;
        float closestDist = std::numeric_limits<float>::max();

        ForEachNode(m_ModelRoot, [&](auto const& node) {
            if (!isBofsInteractable && IsType<BofNode>(*node)) {
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
        ImGui::Checkbox("show bofs", &m_IsShowingBofs);
        ImGui::Checkbox("can click meshes", &m_IsMeshesInteractable);
        ImGui::Checkbox("can click bofs", &m_IsBofsInteractable);
        ImGui::Checkbox("show all connection lines", &m_IsShowingAllConnectionLines);
        ImGui::ColorEdit4("mesh color", glm::value_ptr(m_Colors.mesh));
        ImGui::ColorEdit4("ground color", glm::value_ptr(m_Colors.ground));
        ImGui::ColorEdit4("body color", glm::value_ptr(m_Colors.body));
        ImGui::ColorEdit4("frame color", glm::value_ptr(m_Colors.frame));

        ImGui::InputFloat("scene_scale_factor", &m_SceneScaleFactor);
        if (ImGui::Button("autoscale scene_scale_factor")) {
            AutoScaleScene();
        }

        // draw actions (buttons, etc.)
        if (ImGui::Button("add frame")) {
            auto frame = AddOffsetFrame({0.0f, 0.0f, 0.0f});
            DeSelectAll();
            Select(frame);
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

    DrawableThing GenerateMeshSceneEl(MeshNode const& mn, glm::vec4 const& color)
    {
        DrawableThing rv;
        rv.mesh = mn.GetMesh();
        rv.modelMatrix = mn.GetModelMatrix(m_SceneScaleFactor);
        rv.normalMatrix = NormalMatrix(rv.modelMatrix);
        rv.color = color;
        rv.rimColor = GetRimIntensity(mn);
        rv.maybeDiffuseTex = nullptr;
        return rv;
    }

    DrawableThing GenerateSphereSceneEl(BofNode const& pfn, glm::vec4 const& color)
    {
        DrawableThing rv;
        rv.mesh = m_SphereMesh;
        rv.modelMatrix = pfn.GetModelMatrix(m_SceneScaleFactor);
        rv.normalMatrix = NormalMatrix(rv.modelMatrix);
        rv.color = color;
        rv.rimColor = GetRimIntensity(pfn);
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

        Hover newHover = DoHittest(ImGui::GetMousePos(), true, m_IsBofsInteractable, false);

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
        if (auto const* bofPtr = dynamic_cast<BofNode const*>(assignee.get())) {
            auto& el = sceneEls.emplace_back(GenerateSphereSceneEl(*bofPtr, bofPtr->IsBody() ? m_Colors.body : m_Colors.frame));
            el.color.a = 0.5f;
            el.rimColor = 0.5f;
        } else if (auto const* meshPtr = dynamic_cast<MeshNode const*>(assignee.get())) {
            auto& el = sceneEls.emplace_back(GenerateMeshSceneEl(*meshPtr, m_Colors.mesh));
            el.color.a = 0.5f;
            el.rimColor = 0.5f;
        }

        // draw all frame nodes (they are possible attachment points)
        ForEachNode(m_ModelRoot, [&](auto const& node) {
            if (auto const* bofPtr = dynamic_cast<BofNode const*>(node.get())) {
                auto& el = sceneEls.emplace_back(GenerateSphereSceneEl(*bofPtr, bofPtr->IsBody() ? m_Colors.body : m_Colors.frame));
                el.rimColor = (m_MaybeHover == *bofPtr) ? 0.35f : 0.0f;
                if (IsInclusiveDescendantOf(assignee.get(), bofPtr)) {
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

        m_MaybeHover = DoHittest(ImGui::GetMousePos(), true, m_IsBofsInteractable, m_IsMeshesInteractable);

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
            if (auto const* bofPtr = dynamic_cast<BofNode const*>(node.get())) {
                sceneEls.push_back(GenerateSphereSceneEl(*bofPtr, bofPtr->IsBody() ? m_Colors.body : m_Colors.frame));
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
