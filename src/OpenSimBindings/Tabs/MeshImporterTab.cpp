#include "MeshImporterTab.hpp"

#include "osc_config.hpp"

#include "src/Bindings/GlmHelpers.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/ImGuizmoHelpers.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/CollisionTests.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Sphere.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/OpenSimBindings/Graphics/SimTKMeshLoader.hpp"
#include "src/OpenSimBindings/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/Tabs/ModelEditorTab.hpp"
#include "src/OpenSimBindings/Widgets/MainMenu.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Panels/PerfPanel.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Tabs/TabHost.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/ClonePtr.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/DefaultConstructOnCopy.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Spsc.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/SaveChangesPopup.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <ImGuizmo.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Simulation/Model/OffsetFrame.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <SDL_events.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Mechanics.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <exception>
#include <filesystem>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <variant>

using osc::ClonePtr;
using osc::UID;
using osc::UIDT;
using osc::fpi;
using osc::fpi2;
using osc::fpi4;
using osc::AABB;
using osc::Sphere;
using osc::Mesh;
using osc::Transform;
using osc::PolarPerspectiveCamera;
using osc::Segment;
using osc::Rect;
using osc::Line;

// user-facing string constants
namespace
{
    static osc::CStringView constexpr c_GroundLabel = "Ground";
    static osc::CStringView constexpr c_GroundLabelPluralized = "Ground";
    static osc::CStringView constexpr c_GroundLabelOptionallyPluralized = "Ground(s)";
    static osc::CStringView constexpr c_GroundDescription = "Ground is an inertial reference frame in which the motion of all frames and points may conveniently and efficiently be expressed. It is always defined to be at (0, 0, 0) in 'worldspace' and cannot move. All bodies in the model must eventually attach to ground via joints.";

    static osc::CStringView constexpr c_MeshLabel = "Mesh";
    static osc::CStringView constexpr c_MeshLabelPluralized = "Meshes";
    static osc::CStringView constexpr c_MeshLabelOptionallyPluralized = "Mesh(es)";
    static osc::CStringView constexpr c_MeshDescription = "Meshes are decorational components in the model. They can be translated, rotated, and scaled. Typically, meshes are 'attached' to other elements in the model, such as bodies. When meshes are 'attached' to something, they will 'follow' the thing they are attached to.";
    static osc::CStringView constexpr c_MeshAttachmentCrossrefName = "parent";

    static osc::CStringView constexpr c_BodyLabel = "Body";
    static osc::CStringView constexpr c_BodyLabelPluralized = "Bodies";
    static osc::CStringView constexpr c_BodyLabelOptionallyPluralized = "Body(s)";
    static osc::CStringView constexpr c_BodyDescription = "Bodies are active elements in the model. They define a 'frame' (effectively, a location + orientation) with a mass.\n\nOther body properties (e.g. inertia) can be edited in the main OpenSim Creator editor after you have converted the model into an OpenSim model.";

    static osc::CStringView constexpr c_JointLabel = "Joint";
    static osc::CStringView constexpr c_JointLabelPluralized = "Joints";
    static osc::CStringView constexpr c_JointLabelOptionallyPluralized = "Joint(s)";
    static osc::CStringView constexpr c_JointDescription = "Joints connect two physical frames (i.e. bodies and ground) together and specifies their relative permissible motion (e.g. PinJoints only allow rotation along one axis).\n\nIn OpenSim, joints are the 'edges' of a directed topology graph where bodies are the 'nodes'. All bodies in the model must ultimately connect to ground via joints.";
    static osc::CStringView constexpr c_JointParentCrossrefName = "parent";
    static osc::CStringView constexpr c_JointChildCrossrefName = "child";

    static osc::CStringView constexpr c_StationLabel = "Station";
    static osc::CStringView constexpr c_StationLabelPluralized = "Stations";
    static osc::CStringView constexpr c_StationLabelOptionallyPluralized = "Station(s)";
    static osc::CStringView constexpr c_StationDescription = "Stations are points of interest in the model. They can be used to compute a 3D location in the frame of the thing they are attached to.\n\nThe utility of stations is that you can use them to visually mark points of interest. Those points of interest will then be defined with respect to whatever they are attached to. This is useful because OpenSim typically requires relative coordinates for things in the model (e.g. muscle paths).";
    static osc::CStringView constexpr c_StationParentCrossrefName = "parent";

    static osc::CStringView constexpr c_TranslationDescription = "Translation of the component in ground. OpenSim defines this as 'unitless'; however, OpenSim models typically use meters.";
}

// senteniel UID constants
namespace
{
    class BodyEl;
    UIDT<BodyEl> const c_GroundID;
    UID const c_EmptyID;
    UID const c_RightClickedNothingID;
    UID const c_GroundGroupID;
    UID const c_MeshGroupID;
    UID const c_BodyGroupID;
    UID const c_JointGroupID;
    UID const c_StationGroupID;
}

// other constants
namespace
{
    static float constexpr c_ConnectionLineWidth = 1.0f;
}

// generic helper functions
namespace
{
    // returns a string representation of a spatial position (e.g. (0.0, 1.0, 3.0))
    std::string PosString(glm::vec3 const& pos)
    {
        std::stringstream ss;
        ss.precision(4);
        ss << '(' << pos.x << ", " << pos.y << ", " << pos.z << ')';
        return std::move(ss).str();
    }

    // returns easing function Y value for an X in the range [0, 1.0f]
    float EaseOutElastic(float x)
    {
        // adopted from: https://easings.net/#easeOutElastic

        constexpr float c4 = 2.0f*fpi / 3.0f;

        if (x <= 0.0f)
        {
            return 0.0f;
        }

        if (x >= 1.0f)
        {
            return 1.0f;
        }

        return std::pow(2.0f, -5.0f*x) * std::sin((x*10.0f - 0.75f) * c4) + 1.0f;
    }

    // returns the transform, but rotated such that the given axis points along the
    // given direction
    Transform PointAxisAlong(Transform const& t, int axis, glm::vec3 const& dir)
    {
        glm::vec3 beforeDir{};
        beforeDir[axis] = 1.0f;
        beforeDir = t.rotation * beforeDir;

        glm::quat rotBeforeToAfter = glm::rotation(beforeDir, dir);
        glm::quat newRotation = glm::normalize(rotBeforeToAfter * t.rotation);

        return t.withRotation(newRotation);
    }

    // performs the shortest (angular) rotation of a transform such that the
    // designated axis points towards a point in the same space
    Transform PointAxisTowards(Transform const& t, int axis, glm::vec3 const& p)
    {
        return PointAxisAlong(t, axis, glm::normalize(p - t.position));
    }

    // perform an intrinsic rotation about a transform's axis
    Transform RotateAlongAxis(Transform const& t, int axis, float angRadians)
    {
        glm::vec3 ax{};
        ax[axis] = 1.0f;
        ax = t.rotation * ax;

        glm::quat q = glm::angleAxis(angRadians, ax);

        return t.withRotation(glm::normalize(q * t.rotation));
    }

    Transform ToOsimTransform(SimTK::Transform const& t)
    {
        // extract the SimTK transform into a 4x3 matrix
        glm::mat4x3 m = osc::ToMat4x3(t);

        // take the 3x3 left-hand side (rotation) and decompose that into a quaternion
        glm::quat rotation = glm::quat_cast(glm::mat3{m});

        // take the right-hand column (translation) and assign it as the position
        glm::vec3 position = m[3];

        return Transform{position, rotation};
    }

    // returns a camera that is in the initial position the camera should be in for this screen
    PolarPerspectiveCamera CreateDefaultCamera()
    {
        PolarPerspectiveCamera rv;
        rv.phi = fpi4;
        rv.theta = fpi4;
        rv.radius = 2.5f;
        return rv;
    }

    void SpacerDummy()
    {
        ImGui::Dummy({0.0f, 5.0f});
    }

    glm::vec4 FaintifyColor(glm::vec4 const& srcColor)
    {
        glm::vec4 color = srcColor;
        color.a *= 0.2f;
        return color;
    }

    glm::vec4 RedifyColor(glm::vec4 const& srcColor)
    {
        constexpr float factor = 0.8f;
        return {srcColor[0], factor * srcColor[1], factor * srcColor[2], factor * srcColor[3]};
    }

    // returns true if `c` is a character that can appear within the name of
    // an OpenSim::Component
    bool IsValidOpenSimComponentNameCharacter(char c)
    {
        if (std::isalpha(static_cast<uint8_t>(c)) ||
            ('0' <= c && c <= '9') ||
            (c == '-' || c == '_'))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // returns a sanitized form of the `s` that OpenSim should accept
    std::string SanitizeToOpenSimComponentName(std::string_view sv)
    {
        std::string rv;
        for (char c : sv)
        {
            if (IsValidOpenSimComponentNameCharacter(c))
            {
                rv += c;
            }
        }
        return rv;
    }
}

// UI layering support
//
// the visualizer can push the 3D visualizer into different modes (here, "layers") that
// have different behavior. E.g.:
//
// - normal mode (editing stuff)
// - picking another body in the scene mode
namespace
{
    class Layer;

    // the "parent" thing that is hosting the layer
    class LayerHost {
    protected:
        LayerHost() = default;
        LayerHost(LayerHost const&) = default;
        LayerHost(LayerHost&&) noexcept = default;
        LayerHost& operator=(LayerHost const&) = default;
        LayerHost& operator=(LayerHost&&) noexcept = default;
    public:
        virtual ~LayerHost() noexcept = default;

        void requestPop(Layer& layer)
        {
            implRequestPop(layer);
        }

    private:
        virtual void implRequestPop(Layer&) = 0;
    };

    // a layer that is hosted by the parent
    class Layer {
    protected:
        Layer(LayerHost& parent) : m_Parent{&parent}
        {
        }
        Layer(Layer const&) = default;
        Layer(Layer&&) noexcept = default;
        Layer& operator=(Layer const&) = default;
        Layer& operator=(Layer&&) noexcept = default;
    public:
        virtual ~Layer() noexcept = default;

        bool onEvent(SDL_Event const& e)
        {
            return implOnEvent(e);
        }

        void tick(float dt)
        {
            implTick(dt);
        }

        void draw()
        {
            implDraw();
        }

    protected:
        void requestPop()
        {
            m_Parent->requestPop(*this);
        }

    private:
        virtual bool implOnEvent(SDL_Event const&) = 0;
        virtual void implTick(float) = 0;
        virtual void implDraw() = 0;

        LayerHost* m_Parent;
    };
}

// 3D rendering support
//
// this code exists to make the modelgraph, and any other decorations (lines, hovers, selections, etc.)
// renderable in the UI
namespace
{
    // returns a transform that maps a sphere mesh (defined to be @ 0,0,0 with radius 1)
    // to some sphere in the scene (e.g. a body/ground)
    Transform SphereMeshToSceneSphereTransform(Sphere const& sceneSphere)
    {
        Transform t;
        t.scale *= sceneSphere.radius;
        t.position = sceneSphere.origin;
        return t;
    }

    // something that is being drawn in the scene
    struct DrawableThing final {
        UID id = c_EmptyID;
        UID groupId = c_EmptyID;
        Mesh mesh;
        Transform transform;
        glm::vec4 color;
        osc::SceneDecorationFlags flags;
        std::optional<osc::Material> maybeMaterial;
        std::optional<osc::MaterialPropertyBlock> maybePropertyBlock;
    };

    AABB CalcBounds(DrawableThing const& dt)
    {
        return osc::TransformAABB(dt.mesh.getBounds(), dt.transform);
    }
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
namespace
{
    // a mesh loading request
    struct MeshLoadRequest final {
        UID preferredAttachmentPoint;
        std::vector<std::filesystem::path> paths;
    };

    // a successfully-loaded mesh
    struct LoadedMesh final {
        std::filesystem::path path;
        Mesh meshData;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        UID preferredAttachmentPoint;
        std::vector<LoadedMesh> meshes;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        UID preferredAttachmentPoint;
        std::filesystem::path path;
        std::string error;
    };

    // an OK or ERROR response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // returns an OK or ERROR response to a mesh load request
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest msg)
    {
        std::vector<LoadedMesh> loadedMeshes;
        loadedMeshes.reserve(msg.paths.size());

        for (std::filesystem::path const& path : msg.paths)
        {
            try
            {
                loadedMeshes.push_back(LoadedMesh{path, osc::LoadMeshViaSimTK(path)});
            }
            catch (std::exception const& ex)
            {
                // swallow the exception and emit a log error
                //
                // older implementations used to cancel loading the entire batch by returning
                // an MeshLoadErrorResponse, but that wasn't a good idea because there are
                // times when a user will drag in a bunch of files and expect all the valid
                // ones to load (#303)

                osc::log::error("%s: error loading mesh file: %s", path.string().c_str(), ex.what());
            }
        }

        // HACK: ensure the UI thread redraws after the mesh is loaded
        osc::App::upd().requestRedraw();

        return MeshLoadOKResponse{msg.preferredAttachmentPoint, std::move(loadedMeshes)};
    }

    // a class that loads meshes in a background thread
    //
    // the UI thread must `.poll()` this to check for responses
    class MeshLoader final {
        using Worker = osc::spsc::Worker<MeshLoadRequest, MeshLoadResponse, decltype(respondToMeshloadRequest)>;

    public:
        MeshLoader() : m_Worker{Worker::create(respondToMeshloadRequest)}
        {
        }

        void send(MeshLoadRequest req)
        {
            m_Worker.send(std::move(req));
        }

        std::optional<MeshLoadResponse> poll()
        {
            return m_Worker.poll();
        }

    private:
        Worker m_Worker;
    };
}

// scene element support
//
// the editor UI uses custom scene elements, rather than OpenSim types, because they have to
// support:
//
// - visitor patterns (custom UI elements tailored to each known type)
// - value semantics (undo/redo, rollbacks, etc.)
// - groundspace manipulation (3D gizmos, drag and drop)
// - easy UI integration (GLM datatypes, designed to be easy to dump into OpenGL, etc.)
namespace
{
    // a "class" for a scene element
    class SceneEl;
    class SceneElClass final {
    public:
        SceneElClass(
            osc::CStringView name,
            osc::CStringView namePluralized,
            osc::CStringView nameOptionallyPluralized,
            osc::CStringView icon,
            osc::CStringView description,
            std::unique_ptr<SceneEl> defaultObject) :

            m_ID{},
            m_Name{std::move(name)},
            m_NamePluralized{std::move(namePluralized)},
            m_NameOptionallyPluralized{std::move(nameOptionallyPluralized)},
            m_Icon{std::move(icon)},
            m_Description{std::move(description)},
            m_DefaultObject{std::move(defaultObject)},
            m_UniqueCounter{0}
        {
        }

        UID GetID() const
        {
            return m_ID;
        }

        char const* GetNameCStr() const
        {
            return m_Name.c_str();
        }

        std::string_view GetNameSV() const
        {
            return m_Name;
        }

        char const* GetNamePluralizedCStr() const
        {
            return m_NamePluralized.c_str();
        }

        char const* GetNameOptionallyPluralized() const
        {
            return m_NameOptionallyPluralized.c_str();
        }

        char const* GetIconCStr() const
        {
            return m_Icon.c_str();
        }

        char const* GetDescriptionCStr() const
        {
            return m_Description.c_str();
        }

        int32_t FetchAddUniqueCounter() const
        {
            return m_UniqueCounter++;
        }

        SceneEl const& GetDefaultObject() const
        {
            return *m_DefaultObject;
        }

    private:
        UID m_ID;
        std::string m_Name;
        std::string m_NamePluralized;
        std::string m_NameOptionallyPluralized;
        std::string m_Icon;
        std::string m_Description;
        std::unique_ptr<SceneEl> m_DefaultObject;
        mutable std::atomic<int32_t> m_UniqueCounter;
    };

    // logical comparison
    bool operator==(SceneElClass const& a, SceneElClass const& b)
    {
        return a.GetID() == b.GetID();
    }

    // logical comparison
    bool operator!=(SceneElClass const& a, SceneElClass const& b)
    {
        return !(a == b);
    }

    // returns a unique string that can be used to name an instance of the given class
    std::string GenerateName(SceneElClass const& c)
    {
        std::stringstream ss;
        ss << c.GetNameSV() << c.FetchAddUniqueCounter();
        return std::move(ss).str();
    }

    // forward decls for supported scene elements
    class GroundEl;
    class MeshEl;
    class BodyEl;
    class JointEl;
    class StationEl;

    // a visitor for `const` scene elements
    class ConstSceneElVisitor {
    public:
        virtual ~ConstSceneElVisitor() noexcept = default;
        virtual void operator()(GroundEl const&) = 0;
        virtual void operator()(MeshEl const&) = 0;
        virtual void operator()(BodyEl const&) = 0;
        virtual void operator()(JointEl const&) = 0;
        virtual void operator()(StationEl const&) = 0;
    };

    // a visitor for non-`const` scene elements
    class SceneElVisitor {
    public:
        virtual ~SceneElVisitor() noexcept = default;
        virtual void operator()(GroundEl&) = 0;
        virtual void operator()(MeshEl&) = 0;
        virtual void operator()(BodyEl&) = 0;
        virtual void operator()(JointEl&) = 0;
        virtual void operator()(StationEl&) = 0;
    };

    // runtime flags for a scene el type
    //
    // helps the UI figure out what it should/shouldn't show for a particular type
    // without having to resort to peppering visitors everywhere
    using SceneElFlags = int;
    enum SceneElFlags_ {
        SceneElFlags_None = 0,
        SceneElFlags_CanChangeLabel = 1<<0,
        SceneElFlags_CanChangePosition = 1<<1,
        SceneElFlags_CanChangeRotation = 1<<2,
        SceneElFlags_CanChangeScale = 1<<3,
        SceneElFlags_CanDelete = 1<<4,
        SceneElFlags_CanSelect = 1<<5,
        SceneElFlags_HasPhysicalSize = 1<<6,
    };

    // returns the "direction" of a cross reference
    //
    // most of the time, the direction is towards whatever's being connected to,
    // but sometimes it can be the opposite, depending on how the datastructure
    // is ultimately used
    using CrossrefDirection = int;
    enum CrossrefDirection_ {
        CrossrefDirection_None = 0,
        CrossrefDirection_ToParent = 1<<0,
        CrossrefDirection_ToChild = 1<<1,
        CrossrefDirection_Both = CrossrefDirection_ToChild | CrossrefDirection_ToParent
    };

    // base class for all scene elements
    class SceneEl {
    protected:
        SceneEl() = default;
        SceneEl(SceneEl const&) = default;
        SceneEl(SceneEl&&) noexcept = default;
        SceneEl& operator=(SceneEl const&) = default;
        SceneEl& operator=(SceneEl&&) noexcept = default;
    public:
        virtual ~SceneEl() noexcept = default;

        SceneElClass const& GetClass() const
        {
            return implGetClass();
        }

        // allow runtime cloning of a particular instance
        std::unique_ptr<SceneEl> clone() const
        {
            return implClone();
        }

        // accept visitors so that downstream code can use visitors when they need to
        // handle specific types
        void Accept(ConstSceneElVisitor& visitor) const
        {
            implAccept(visitor);
        }
        void Accept(SceneElVisitor& visitor)
        {
            implAccept(visitor);
        }

        // each scene element may be referencing `n` (>= 0) other scene elements by
        // ID. These methods allow implementations to ask what and how
        int GetNumCrossReferences() const
        {
            return implGetNumCrossReferences();
        }

        UID GetCrossReferenceConnecteeID(int i) const
        {
            return implGetCrossReferenceConnecteeID(i);
        }
        void SetCrossReferenceConnecteeID(int i, UID newID)
        {
            implSetCrossReferenceConnecteeID(i, newID);
        }
        osc::CStringView GetCrossReferenceLabel(int i) const
        {
            return implGetCrossReferenceLabel(i);
        }
        CrossrefDirection GetCrossReferenceDirection(int i) const
        {
            return implGetCrossReferenceDirection(i);
        }

        SceneElFlags GetFlags() const
        {
            return implGetFlags();
        }

        UID GetID() const
        {
            return implGetID();
        }

        std::ostream& operator<<(std::ostream& o) const
        {
            return implWriteToStream(o);
        }

        osc::CStringView GetLabel() const
        {
            return implGetLabel();
        }

        void SetLabel(std::string_view newLabel)
        {
            implSetLabel(std::move(newLabel));
        }

        Transform GetXform() const
        {
            return implGetXform();
        }
        void SetXform(Transform const& newTransform)
        {
            implSetXform(newTransform);
        }

        AABB CalcBounds() const
        {
            return implCalcBounds();
        }

        // helper methods (overrideable)
        //
        // these position/scale/rotation methods are here as member virtual functions
        // because downstream classes may only actually hold a subset of a full
        // transform (e.g. only position). There is a perf advantage to only returning
        // what was asked for.

        glm::vec3 GetPos() const
        {
            return implGetPos();
        }
        void SetPos(glm::vec3 const& newPos)
        {
            implSetPos(newPos);
        }

        glm::vec3 GetScale() const
        {
            return implGetScale();
        }

        void SetScale(glm::vec3 const& newScale)
        {
            implSetScale(newScale);
        }

        glm::quat GetRotation() const
        {
            return implGetRotation();
        }

        void SetRotation(glm::quat const& newRotation)
        {
            implSetRotation(newRotation);
        }

    private:
        virtual SceneElClass const& implGetClass() const = 0;
        virtual std::unique_ptr<SceneEl> implClone() const = 0;
        virtual void implAccept(ConstSceneElVisitor&) const = 0;
        virtual void implAccept(SceneElVisitor&) = 0;
        virtual int implGetNumCrossReferences() const
        {
            return 0;
        }
        virtual UID implGetCrossReferenceConnecteeID(int) const
        {
            throw std::runtime_error{"cannot get cross reference ID: no method implemented"};
        }
        virtual void implSetCrossReferenceConnecteeID(int, UID)
        {
            throw std::runtime_error{"cannot set cross reference ID: no method implemented"};
        }
        virtual osc::CStringView implGetCrossReferenceLabel(int) const
        {
            throw std::runtime_error{"cannot get cross reference label: no method implemented"};
        }
        virtual CrossrefDirection implGetCrossReferenceDirection(int) const
        {
            return CrossrefDirection_ToParent;
        }
        virtual SceneElFlags implGetFlags() const = 0;

        virtual UID implGetID() const = 0;
        virtual std::ostream& implWriteToStream(std::ostream&) const = 0;

        virtual osc::CStringView implGetLabel() const = 0;
        virtual void implSetLabel(std::string_view) = 0;

        virtual Transform implGetXform() const = 0;
        virtual void implSetXform(Transform const&) = 0;

        virtual AABB implCalcBounds() const = 0;

        virtual glm::vec3 implGetPos() const
        {
            return GetXform().position;
        }
        virtual void implSetPos(glm::vec3 const& newPos)
        {
            Transform t = GetXform();
            t.position = newPos;
            SetXform(t);
        }

        virtual glm::vec3 implGetScale() const
        {
            return GetXform().scale;
        }
        virtual void implSetScale(glm::vec3 const& newScale)
        {
            Transform t = GetXform();
            t.scale = newScale;
            SetXform(t);
        }

        virtual glm::quat implGetRotation() const
        {
            return GetXform().rotation;
        }
        virtual void implSetRotation(glm::quat const& newRotation)
        {
            Transform t = GetXform();
            t.rotation = newRotation;
            SetXform(t);
        }
    };

    // SceneEl helper methods

    void ApplyTranslation(SceneEl& el, glm::vec3 const& translation)
    {
        el.SetPos(el.GetPos() + translation);
    }

    void ApplyRotation(SceneEl& el, glm::vec3 const& eulerAngles, glm::vec3 const& rotationCenter)
    {
        Transform t = el.GetXform();
        ApplyWorldspaceRotation(t, eulerAngles, rotationCenter);
        el.SetXform(t);
    }

    void ApplyScale(SceneEl& el, glm::vec3 const& scaleFactors)
    {
        el.SetScale(el.GetScale() * scaleFactors);
    }

    bool CanChangeLabel(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_CanChangeLabel;
    }

    bool CanChangePosition(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_CanChangePosition;
    }

    bool CanChangeRotation(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_CanChangeRotation;
    }

    bool CanChangeScale(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_CanChangeScale;
    }

    bool CanDelete(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_CanDelete;
    }

    bool CanSelect(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_CanSelect;
    }

    bool HasPhysicalSize(SceneEl const& el)
    {
        return el.GetFlags() & SceneElFlags_HasPhysicalSize;
    }

    bool IsCrossReferencing(SceneEl const& el, UID id, CrossrefDirection direction = CrossrefDirection_Both)
    {
        for (int i = 0, len = el.GetNumCrossReferences(); i < len; ++i)
        {
            if (el.GetCrossReferenceConnecteeID(i) == id && el.GetCrossReferenceDirection(i) & direction)
            {
                return true;
            }
        }
        return false;
    }

    class GroundEl final : public SceneEl {
    public:

        static SceneElClass const& Class()
        {
            static SceneElClass const s_Class =
            {
                c_GroundLabel,
                c_GroundLabelPluralized,
                c_GroundLabelOptionallyPluralized,
                ICON_FA_DOT_CIRCLE,
                c_GroundDescription,
                std::unique_ptr<SceneEl>{new GroundEl{}}
            };

            return s_Class;
        }

        UIDT<BodyEl> GetID() const
        {
            return c_GroundID;
        }

    private:
        SceneElClass const& implGetClass() const final
        {
            return Class();
        }

        std::unique_ptr<SceneEl> implClone() const final
        {
            return std::make_unique<GroundEl>(*this);
        }

        void implAccept(ConstSceneElVisitor& visitor) const final
        {
            visitor(*this);
        }

        void implAccept(SceneElVisitor& visitor) final
        {
            visitor(*this);
        }

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags_None;
        }

        UID implGetID() const final
        {
            return c_GroundID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << c_GroundLabel << "()";
        }

        osc::CStringView implGetLabel() const final
        {
            return c_GroundLabel;
        }

        void implSetLabel(std::string_view) final
        {
            // ignore: cannot set ground's name
        }

        Transform implGetXform() const final
        {
            return Transform{};
        }

        void implSetXform(Transform const&) final
        {
            // ignore: cannot change ground's xform
        }

        AABB implCalcBounds() const final
        {
            return AABB{};
        }
    };

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
    class MeshEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass const s_Class =
            {
                c_MeshLabel,
                c_MeshLabelPluralized,
                c_MeshLabelOptionallyPluralized,
                ICON_FA_CUBE,
                c_MeshDescription,
                std::unique_ptr<SceneEl>{new MeshEl{}}
            };

            return s_Class;
        }

        MeshEl() :
            m_ID{},
            m_Attachment{},
            m_MeshData{osc::App::singleton<osc::MeshCache>()->getBrickMesh()},
            m_Path{"invalid"}
        {
            // default ctor for prototype storage
        }

        MeshEl(
            UIDT<MeshEl> id,
            UID attachment,  // can be c_GroundID
            Mesh const& meshData,
            std::filesystem::path const& path) :

            m_ID{std::move(id)},
            m_Attachment{std::move(attachment)},
            m_MeshData{meshData},
            m_Path{path}
        {
        }

        MeshEl(
            UID attachment,  // can be c_GroundID
            Mesh const& meshData,
            std::filesystem::path const& path) :

            MeshEl{UIDT<MeshEl>{}, std::move(attachment), meshData, path}
        {
        }

        UIDT<MeshEl> GetID() const
        {
            return m_ID;
        }

        Mesh const& getMeshData() const
        {
            return m_MeshData;
        }

        std::filesystem::path const& getPath() const
        {
            return m_Path;
        }

        UID getParentID() const
        {
            return m_Attachment;
        }

        void setParentID(UID newParent)
        {
            m_Attachment = newParent;
        }

    private:
        SceneElClass const& implGetClass() const final
        {
            return Class();
        }

        std::unique_ptr<SceneEl> implClone() const final
        {
            return std::make_unique<MeshEl>(*this);
        }

        void implAccept(ConstSceneElVisitor& visitor) const final
        {
            visitor(*this);
        }

        void implAccept(SceneElVisitor& visitor) final
        {
            visitor(*this);
        }

        int implGetNumCrossReferences() const final
        {
            return 1;
        }

        UID implGetCrossReferenceConnecteeID(int i) const final
        {
            switch (i) {
            case 0:
                return m_Attachment;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            switch (i) {
            case 0:
                m_Attachment = osc::DowncastID<BodyEl>(id);
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        osc::CStringView implGetCrossReferenceLabel(int i) const final
        {
            switch (i) {
            case 0:
                return c_MeshAttachmentCrossrefName;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags_CanChangeLabel |
                SceneElFlags_CanChangePosition |
                SceneElFlags_CanChangeRotation |
                SceneElFlags_CanChangeScale |
                SceneElFlags_CanDelete |
                SceneElFlags_CanSelect |
                SceneElFlags_HasPhysicalSize;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << "MeshEl("
                << "ID = " << m_ID
                << ", Attachment = " << m_Attachment
                << ", Xform = " << Xform
                << ", MeshData = " << &m_MeshData
                << ", Path = " << m_Path
                << ", Name = " << Name
                << ')';
        }

        osc::CStringView implGetLabel() const final
        {
            return Name;
        }

        void implSetLabel(std::string_view sv) final
        {
            Name = SanitizeToOpenSimComponentName(sv);
        }

        Transform implGetXform() const final
        {
            return Xform;
        }

        void implSetXform(Transform const& t) final
        {
            Xform = t;
        }

        AABB implCalcBounds() const final
        {
            return osc::TransformAABB(m_MeshData.getBounds(), Xform);
        }

        UIDT<MeshEl> m_ID;
        UID m_Attachment;  // can be c_GroundID
        Transform Xform;
        Mesh m_MeshData;
        std::filesystem::path m_Path;
        std::string Name{SanitizeToOpenSimComponentName(osc::FileNameWithoutExtension(m_Path))};
    };

    // a body scene element
    //
    // In this mesh importer, bodies are positioned + oriented in ground (see MeshEl for explanation of why).
    class BodyEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass const s_Class =
            {
                c_BodyLabel,
                c_BodyLabelPluralized,
                c_BodyLabelOptionallyPluralized,
                ICON_FA_CIRCLE,
                c_BodyDescription,
                std::unique_ptr<SceneEl>{new BodyEl{}}
            };

            return s_Class;
        }

        BodyEl() :
            m_ID{},
            m_Name{"prototype"},
            m_Xform{}
        {
            // default ctor for prototype storage
        }

        BodyEl(
            UIDT<BodyEl> id,
            std::string const& name,
            Transform const& xform) :

            m_ID{id},
            m_Name{SanitizeToOpenSimComponentName(name)},
            m_Xform{xform}
        {
        }

        BodyEl(std::string const& name, Transform const& xform) :
            BodyEl{UIDT<BodyEl>{}, name, xform}
        {
        }

        explicit BodyEl(Transform const& xform) :
            BodyEl{UIDT<BodyEl>{}, GenerateName(Class()), xform}
        {
        }

        UIDT<BodyEl> GetID() const
        {
            return m_ID;
        }

        double getMass() const
        {
            return Mass;
        }

        void setMass(double newMass)
        {
            Mass = newMass;
        }

    private:
        SceneElClass const& implGetClass() const final
        {
            return Class();
        }

        std::unique_ptr<SceneEl> implClone() const final
        {
            return std::make_unique<BodyEl>(*this);
        }

        void implAccept(ConstSceneElVisitor& visitor) const final
        {
            visitor(*this);
        }

        void implAccept(SceneElVisitor& visitor) final
        {
            visitor(*this);
        }

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags_CanChangeLabel |
                SceneElFlags_CanChangePosition |
                SceneElFlags_CanChangeRotation |
                SceneElFlags_CanDelete |
                SceneElFlags_CanSelect;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << "BodyEl(ID = " << m_ID
                << ", Name = " << m_Name
                << ", Xform = " << m_Xform
                << ", Mass = " << Mass
                << ')';
        }

        osc::CStringView implGetLabel() const final
        {
            return m_Name;
        }

        void implSetLabel(std::string_view sv) final
        {
            m_Name = SanitizeToOpenSimComponentName(sv);
        }

        Transform implGetXform() const final
        {
            return m_Xform;
        }

        void implSetXform(Transform const& newXform) final
        {
            m_Xform = newXform;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        void implSetScale(glm::vec3 const&) final
        {
            // ignore: scaling a body, which is a point, does nothing
        }

        AABB implCalcBounds() const final
        {
            return AABB{m_Xform.position, m_Xform.position};
        }

        UIDT<BodyEl> m_ID;
        std::string m_Name;
        Transform m_Xform;
        double Mass = 1.0f;  // OpenSim goes bananas if a body has a mass <= 0
    };

    // a joint scene element
    //
    // see `JointAttachment` comment for an explanation of why it's designed this way.
    class JointEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass const s_Class =
            {
                c_JointLabel,
                c_JointLabelPluralized,
                c_JointLabelOptionallyPluralized,
                ICON_FA_LINK,
                c_JointDescription,
                std::unique_ptr<SceneEl>{new JointEl{}}
            };

            return s_Class;
        }

        JointEl() :
            m_ID{},
            m_JointTypeIndex{0},
            m_UserAssignedName{"prototype"},
            m_Parent{},
            m_Child{},
            m_Xform{}
        {
            // default ctor for prototype allocation
        }

        JointEl(
            UIDT<JointEl> id,
            size_t jointTypeIdx,
            std::string const& userAssignedName,  // can be empty
            UID parent,
            UIDT<BodyEl> child,
            Transform const& xform) :

            m_ID{std::move(id)},
            m_JointTypeIndex{std::move(jointTypeIdx)},
            m_UserAssignedName{SanitizeToOpenSimComponentName(userAssignedName)},
            m_Parent{std::move(parent)},
            m_Child{std::move(child)},
            m_Xform{xform}
        {
        }

        JointEl(
            size_t jointTypeIdx,
            std::string const& userAssignedName,  // can be empty
            UID parent,
            UIDT<BodyEl> child,
            Transform const& xform) :

            JointEl
            {
                UIDT<JointEl>{},
                std::move(jointTypeIdx),
                userAssignedName,
                std::move(parent),
                std::move(child),
                xform
            }
        {
        }

        UIDT<JointEl> GetID() const
        {
            return m_ID;
        }

        osc::CStringView GetSpecificTypeName() const
        {
            return osc::JointRegistry::nameStrings()[m_JointTypeIndex];
        }

        UID getParentID() const
        {
            return m_Parent;
        }

        UIDT<BodyEl> getChildID() const
        {
            return m_Child;
        }

        osc::CStringView getUserAssignedName() const
        {
            return m_UserAssignedName;
        }

        size_t getJointTypeIndex() const
        {
            return m_JointTypeIndex;
        }

        void setJointTypeIndex(size_t i)
        {
            m_JointTypeIndex = i;
        }

    private:
        SceneElClass const& implGetClass() const final
        {
            return Class();
        }

        std::unique_ptr<SceneEl> implClone() const final
        {
            return std::make_unique<JointEl>(*this);
        }

        void implAccept(ConstSceneElVisitor& visitor) const final
        {
            visitor(*this);
        }

        void implAccept(SceneElVisitor& visitor) final
        {
            visitor(*this);
        }

        int implGetNumCrossReferences() const final
        {
            return 2;
        }

        UID implGetCrossReferenceConnecteeID(int i) const final
        {
            switch (i) {
            case 0:
                return m_Parent;
            case 1:
                return m_Child;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            switch (i) {
            case 0:
                m_Parent = id;
                break;
            case 1:
                m_Child = osc::DowncastID<BodyEl>(id);
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        osc::CStringView implGetCrossReferenceLabel(int i) const final
        {
            switch (i) {
            case 0:
                return c_JointParentCrossrefName;
            case 1:
                return c_JointChildCrossrefName;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        CrossrefDirection implGetCrossReferenceDirection(int i) const final
        {
            switch (i) {
            case 0:
                return CrossrefDirection_ToParent;
            case 1:
                return CrossrefDirection_ToChild;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags_CanChangeLabel |
                SceneElFlags_CanChangePosition |
                SceneElFlags_CanChangeRotation |
                SceneElFlags_CanDelete |
                SceneElFlags_CanSelect;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << "JointEl(ID = " << m_ID
                << ", JointTypeIndex = " << m_JointTypeIndex
                << ", UserAssignedName = " << m_UserAssignedName
                << ", Parent = " << m_Parent
                << ", Child = " << m_Child
                << ", Xform = " << m_Xform
                << ')';
        }

        osc::CStringView implGetLabel() const final
        {
            return m_UserAssignedName.empty() ? GetSpecificTypeName() : m_UserAssignedName;
        }

        void implSetLabel(std::string_view sv) final
        {
            m_UserAssignedName = SanitizeToOpenSimComponentName(sv);
        }

        Transform implGetXform() const final
        {
            return m_Xform;
        }

        void implSetXform(Transform const& t) final
        {
            m_Xform = t;
            m_Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        void implSetScale(glm::vec3 const&)
        {
            // ignore
        }

        AABB implCalcBounds() const final
        {
            return AABB{m_Xform.position, m_Xform.position};
        }

        UIDT<JointEl> m_ID;
        size_t m_JointTypeIndex;
        std::string m_UserAssignedName;
        UID m_Parent;  // can be ground
        UIDT<BodyEl> m_Child;
        Transform m_Xform;  // joint center
    };

    bool IsAttachedTo(JointEl const& joint, BodyEl const& b)
    {
        return joint.getParentID() == b.GetID() || joint.getChildID() == b.GetID();
    }

    // a station (point of interest)
    class StationEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass const s_Class =
            {
                c_StationLabel,
                c_StationLabelPluralized,
                c_StationLabelOptionallyPluralized,
                ICON_FA_MAP_PIN,
                c_StationDescription,
                std::unique_ptr<SceneEl>{new StationEl{}}
            };

            return s_Class;
        }

        StationEl() :
            m_ID{},
            m_Attachment{},
            m_Position{},
            m_Name{"prototype"}
        {
            // default ctor for prototype allocation
        }

        StationEl(
            UIDT<StationEl> id,
            UIDT<BodyEl> attachment,  // can be c_GroundID
            glm::vec3 const& position,
            std::string const& name) :

            m_ID{std::move(id)},
            m_Attachment{std::move(attachment)},
            m_Position{position},
            m_Name{SanitizeToOpenSimComponentName(name)}
        {
        }

        StationEl(
            UIDT<BodyEl> attachment,  // can be c_GroundID
            glm::vec3 const& position,
            std::string const& name) :

            m_ID{},
            m_Attachment{std::move(attachment)},
            m_Position{position},
            m_Name{SanitizeToOpenSimComponentName(name)}
        {
        }

        UIDT<StationEl> GetID() const
        {
            return m_ID;
        }

        UID getParentID() const
        {
            return m_Attachment;
        }

    private:
        SceneElClass const& implGetClass() const final
        {
            return Class();
        }

        std::unique_ptr<SceneEl> implClone() const final
        {
            return std::make_unique<StationEl>(*this);
        }

        void implAccept(ConstSceneElVisitor& visitor) const final
        {
            visitor(*this);
        }

        void implAccept(SceneElVisitor& visitor) final
        {
            visitor(*this);
        }

        int implGetNumCrossReferences() const final
        {
            return 1;
        }

        UID implGetCrossReferenceConnecteeID(int i) const final
        {
            switch (i) {
            case 0:
                return m_Attachment;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        void implSetCrossReferenceConnecteeID(int i, UID id) final
        {
            switch (i) {
            case 0:
                m_Attachment = osc::DowncastID<BodyEl>(id);
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        osc::CStringView implGetCrossReferenceLabel(int i) const final
        {
            switch (i) {
            case 0:
                return c_StationParentCrossrefName;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags_CanChangeLabel |
                SceneElFlags_CanChangePosition |
                SceneElFlags_CanDelete |
                SceneElFlags_CanSelect;
        }

        UID implGetID() const final
        {
            return m_ID;
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            using osc::operator<<;

            return o << "StationEl("
                << "ID = " << m_ID
                << ", Attachment = " << m_Attachment
                << ", Position = " << m_Position
                << ", Name = " << m_Name
                << ')';
        }

        osc::CStringView implGetLabel() const final
        {
            return m_Name;
        }

        void implSetLabel(std::string_view sv) final
        {
            m_Name = SanitizeToOpenSimComponentName(sv);
        }

        Transform implGetXform() const final
        {
            return Transform{m_Position};
        }

        void implSetXform(Transform const& t) final
        {
            m_Position = t.position;
        }

        AABB implCalcBounds() const final
        {
            return AABB{m_Position, m_Position};
        }

        UIDT<StationEl> m_ID;
        UIDT<BodyEl> m_Attachment;  // can be c_GroundID
        glm::vec3 m_Position;
        std::string m_Name;
    };


    // returns true if a mesh can be attached to the given element
    bool CanAttachMeshTo(SceneEl const& e)
    {
        struct Visitor final : public ConstSceneElVisitor {
            void operator()(GroundEl const&) final { result = true; }
            void operator()(MeshEl const&) final { result = false; }
            void operator()(BodyEl const&) final { result = true; }
            void operator()(JointEl const&) final { result = true; }
            void operator()(StationEl const&) final { result = false; }

            bool result = false;
        };

        Visitor v;
        e.Accept(v);
        return v.result;
    }

    // returns `true` if a `StationEl` can be attached to the element
    bool CanAttachStationTo(SceneEl const& e)
    {
        struct Visitor final : public ConstSceneElVisitor {
            void operator()(GroundEl const&) final { result = true; }
            void operator()(MeshEl const&) final { result = true; }
            void operator()(BodyEl const&) final { result = true; }
            void operator()(JointEl const&) final { result = false; }
            void operator()(StationEl const&) final { result = false; }

            bool result = false;
        };

        Visitor v;
        e.Accept(v);
        return v.result;
    }

    // returns true if the given SceneEl is of a particular scene el type
    template<typename T>
    bool Is(SceneEl const& el)
    {
        static_assert(std::is_base_of_v<SceneEl, T>);
        return dynamic_cast<T const*>(&el);
    }

    std::vector<SceneElClass const*> GenerateSceneElClassList()
    {
        return {
            &GroundEl::Class(),
            &MeshEl::Class(),
            &BodyEl::Class(),
            &JointEl::Class(),
            &StationEl::Class()
        };
    }

    std::vector<SceneElClass const*> const& GetSceneElClasses()
    {
        static std::vector<SceneElClass const*> const s_Classes = GenerateSceneElClassList();
        return s_Classes;
    }

    glm::vec3 AverageCenter(MeshEl const& el)
    {
        glm::vec3 const centerpointInModelSpace = AverageCenterpoint(el.getMeshData());
        return el.GetXform() * centerpointInModelSpace;
    }

    glm::vec3 MassCenter(MeshEl const& el)
    {
        glm::vec3 const massCenterInModelSpace = MassCenter(el.getMeshData());
        return el.GetXform() * massCenterInModelSpace;
    }
}

// modelgraph support
//
// scene elements are collected into a single, potentially interconnected, model graph
// datastructure. This datastructure is what ultimately maps into an "OpenSim::Model".
//
// Main design considerations:
//
// - Must have somewhat fast associative lookup semantics, because the UI needs to
//   traverse the graph in a value-based (rather than pointer-based) way
//
// - Must have value semantics, so that other code such as the undo/redo buffer can
//   copy an entire ModelGraph somewhere else in memory without having to worry about
//   aliased mutations
namespace
{
    class ModelGraph final {

        // helper class for iterating over model graph elements
        template<bool IsConst, typename T = SceneEl>
        class Iterator final {
        public:
            Iterator(
                std::map<UID, ClonePtr<SceneEl>>::iterator pos,
                std::map<UID, ClonePtr<SceneEl>>::iterator end) :
                m_Pos{pos},
                m_End{end}
            {
                // ensure iterator initially points at an element with the correct type
                while (m_Pos != m_End) {
                    if (dynamic_cast<T const*>(m_Pos->second.get())) {
                        break;
                    }
                    ++m_Pos;
                }
            }

            // implict conversion from non-const- to const-iterator

            template<bool IsConst_ = IsConst>
            operator typename std::enable_if_t<!IsConst_, Iterator<true, T>>() const noexcept
            {
                return Iterator<true, T>{m_Pos, m_End};
            }

            // LegacyIterator

            Iterator& operator++() noexcept
            {
                while (++m_Pos != m_End) {
                    if (dynamic_cast<T const*>(m_Pos->second.get())) {
                        break;
                    }
                }
                return *this;
            }

            template<bool IsConst_ = IsConst>
            typename std::enable_if_t<IsConst_, T const&> operator*() const noexcept
            {
                return dynamic_cast<T const&>(*m_Pos->second);
            }

            template<bool IsConst_ = IsConst>
            typename std::enable_if_t<!IsConst_, T&> operator*() const noexcept
            {
                return dynamic_cast<T&>(*m_Pos->second);
            }

            // EqualityComparable

            template<bool OtherConst>
            bool operator!=(Iterator<OtherConst, T> const& other) const noexcept
            {
                return m_Pos != other.m_Pos;
            }

            template<bool OtherConst>
            bool operator==(Iterator<OtherConst> const& other) const noexcept
            {
                return !(*this != other);
            }

            // LegacyInputIterator

            template<bool IsConst_ = IsConst>
            typename std::enable_if_t<IsConst_, T const*> operator->() const noexcept
            {

                return &dynamic_cast<T const&>(*m_Pos->second);
            }

            template<bool IsConst_ = IsConst>
            typename std::enable_if_t<!IsConst_, T*> operator->() const noexcept
            {
                return &dynamic_cast<T&>(*m_Pos->second);
            }

        private:
            std::map<UID, ClonePtr<SceneEl>>::iterator m_Pos;
            std::map<UID, ClonePtr<SceneEl>>::iterator m_End;  // needed because of multi-step advances
        };

        // helper class for an iterable object with a beginning + end
        template<bool IsConst, typename T = SceneEl>
        class Iterable final {
        public:
            Iterable(std::map<UID, ClonePtr<SceneEl>>& els) :
                m_Begin{els.begin(), els.end()},
                m_End{els.end(), els.end()}
            {
            }

            Iterator<IsConst, T> begin() { return m_Begin; }
            Iterator<IsConst, T> end() { return m_End; }

        private:
            Iterator<IsConst, T> m_Begin;
            Iterator<IsConst, T> m_End;
        };

    public:

        ModelGraph() :
            // insert a senteniel ground element into the model graph (it should always
            // be there)
            m_Els{{c_GroundID, ClonePtr<SceneEl>{GroundEl{}}}}
        {
        }

        std::unique_ptr<ModelGraph> clone() const
        {
            return std::make_unique<ModelGraph>(*this);
        }


        SceneEl* TryUpdElByID(UID id)
        {
            auto it = m_Els.find(id);

            if (it == m_Els.end())
            {
                return nullptr;  // ID does not exist in the element collection
            }

            return it->second.get();
        }

        template<typename T = SceneEl>
        T* TryUpdElByID(UID id)
        {
            static_assert(std::is_base_of_v<SceneEl, T>);

            SceneEl* p = TryUpdElByID(id);

            return p ? dynamic_cast<T*>(p) : nullptr;
        }

        template<typename T = SceneEl>
        T const* TryGetElByID(UID id) const
        {
            return const_cast<ModelGraph&>(*this).TryUpdElByID<T>(id);
        }

        template<typename T = SceneEl>
        T& UpdElByID(UID id)
        {
            T* ptr = TryUpdElByID<T>(id);

            if (!ptr)
            {
                std::stringstream msg;
                msg << "could not find a scene element of type " << typeid(T).name() << " with ID = " << id;
                throw std::runtime_error{std::move(msg).str()};
            }

            return *ptr;
        }

        template<typename T = SceneEl>
        T const& GetElByID(UID id) const
        {
            return const_cast<ModelGraph&>(*this).UpdElByID<T>(id);
        }

        template<typename T = SceneEl>
        bool ContainsEl(UID id) const
        {
            return TryGetElByID<T>(id);
        }

        template<typename T = SceneEl>
        bool ContainsEl(SceneEl const& e) const
        {
            return ContainsEl<T>(e.GetID());
        }

        template<typename T = SceneEl>
        Iterable<false, T> iter()
        {
            return Iterable<false, T>{m_Els};
        }

        template<typename T = SceneEl>
        Iterable<true, T> iter() const
        {
            return Iterable<true, T>{const_cast<ModelGraph&>(*this).m_Els};
        }

        SceneEl& AddEl(std::unique_ptr<SceneEl> el)
        {
            // ensure element connects to things that already exist in the model
            // graph

            for (int i = 0, len = el->GetNumCrossReferences(); i < len; ++i)
            {
                if (!ContainsEl(el->GetCrossReferenceConnecteeID(i)))
                {
                    std::stringstream ss;
                    ss << "cannot add '" << el->GetLabel() << "' (ID = " << el->GetID() << ") to model graph because it contains a cross reference (label = " << el->GetCrossReferenceLabel(i) << ") to a scene element that does not exist in the model graph";
                    throw std::runtime_error{std::move(ss).str()};
                }
            }

            return *m_Els.emplace(el->GetID(), std::move(el)).first->second;
        }

        template<typename T, typename... Args>
        T& AddEl(Args&&... args)
        {
            return static_cast<T&>(AddEl(std::unique_ptr<SceneEl>{std::make_unique<T>(std::forward<Args>(args)...)}));
        }

        bool DeleteElByID(UID id)
        {
            SceneEl const* el = TryGetElByID(id);

            if (!el)
            {
                return false;  // ID doesn't exist in the model graph
            }

            // collect all to-be-deleted elements into one deletion set so that the deletion
            // happens in separate phase from the "search for things to delete" phase
            std::unordered_set<UID> deletionSet;
            PopulateDeletionSet(*el, deletionSet);

            for (UID deletedID : deletionSet)
            {
                DeSelect(deletedID);

                // move element into deletion set, rather than deleting it immediately,
                // so that code that relies on references to the to-be-deleted element
                // still works until an explicit `.GarbageCollect()` call

                auto it = m_Els.find(deletedID);
                if (it != m_Els.end())
                {
                    m_DeletedEls->push_back(std::move(it->second));
                    m_Els.erase(it);
                }
            }

            return !deletionSet.empty();
        }

        bool DeleteEl(SceneEl const& el)
        {
            return DeleteElByID(el.GetID());
        }

        void GarbageCollect()
        {
            m_DeletedEls->clear();
        }


        // selection logic

        std::unordered_set<UID> const& GetSelected() const
        {
            return m_SelectedEls;
        }

        bool IsSelected(UID id) const
        {
            return Contains(m_SelectedEls, id);
        }

        bool IsSelected(SceneEl const& el) const
        {
            return IsSelected(el.GetID());
        }

        void Select(UID id)
        {
            SceneEl const* e = TryGetElByID(id);

            if (e && CanSelect(*e))
            {
                m_SelectedEls.insert(id);
            }
        }

        void Select(SceneEl const& el)
        {
            Select(el.GetID());
        }

        void DeSelect(UID id)
        {
            m_SelectedEls.erase(id);
        }

        void DeSelect(SceneEl const& el)
        {
            DeSelect(el.GetID());
        }

        void SelectAll()
        {
            for (SceneEl const& e : iter())
            {
                if (CanSelect(e))
                {
                    m_SelectedEls.insert(e.GetID());
                }
            }
        }

        void DeSelectAll()
        {
            m_SelectedEls.clear();
        }

    private:

        void PopulateDeletionSet(SceneEl const& deletionTarget, std::unordered_set<UID>& out)
        {
            UID deletedID = deletionTarget.GetID();

            // add the deletion target to the deletion set (if applicable)
            if (CanDelete(deletionTarget))
            {
                if (!out.emplace(deletedID).second)
                {
                    throw std::runtime_error{"cannot populate deletion set - cycle detected"};
                }
            }

            // iterate over everything else in the model graph and look for things
            // that cross-reference the to-be-deleted element - those things should
            // also be deleted

            for (SceneEl const& el : iter())
            {
                if (IsCrossReferencing(el, deletedID))
                {
                    PopulateDeletionSet(el, out);
                }
            }
        }

        std::map<UID, ClonePtr<SceneEl>> m_Els;
        std::unordered_set<UID> m_SelectedEls;
        osc::DefaultConstructOnCopy<std::vector<ClonePtr<SceneEl>>> m_DeletedEls;
    };

    void SelectOnly(ModelGraph& mg, SceneEl const& e)
    {
        mg.DeSelectAll();
        mg.Select(e);
    }

    bool HasSelection(ModelGraph const& mg)
    {
        return !mg.GetSelected().empty();
    }

    void DeleteSelected(ModelGraph& mg)
    {
        // copy deletion set to ensure iterator can't be invalidated by deletion
        auto selected = mg.GetSelected();

        for (UID id : selected)
        {
            mg.DeleteElByID(id);
        }

        mg.DeSelectAll();
    }

    osc::CStringView GetLabel(ModelGraph const& mg, UID id)
    {
        return mg.GetElByID(id).GetLabel();
    }

    Transform GetTransform(ModelGraph const& mg, UID id)
    {
        return mg.GetElByID(id).GetXform();
    }

    glm::vec3 GetPosition(ModelGraph const& mg, UID id)
    {
        return mg.GetElByID(id).GetPos();
    }

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(ModelGraph const& mg, SceneEl const& el)
    {
        UID id = el.GetID();
        for (JointEl const& j : mg.iter<JointEl>())
        {
            if (j.getChildID() == id)
            {
                return true;
            }
        }
        return false;
    }

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(ModelGraph const& modelGraph, JointEl const& jointEl)
    {
        if (jointEl.getChildID() == c_GroundID)
        {
            return true;  // ground cannot be a child in a joint
        }

        if (jointEl.getParentID() == jointEl.getChildID())
        {
            return true;  // is directly attached to itself
        }

        if (jointEl.getParentID() != c_GroundID && !modelGraph.ContainsEl<BodyEl>(jointEl.getParentID()))
        {
            return true;  // has a parent ID that's invalid for this model graph
        }

        if (!modelGraph.ContainsEl<BodyEl>(jointEl.getChildID()))
        {
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

        if (joint.getParentID() == c_GroundID)
        {
            return true;  // it's directly attached to ground
        }

        BodyEl const* parent = modelGraph.TryGetElByID<BodyEl>(joint.getParentID());

        if (!parent)
        {
            return false;  // joint's parent is garbage
        }

        // else: recurse to parent
        return IsBodyAttachedToGround(modelGraph, *parent, previousVisits);
    }

    // returns `true` if `body` is attached to ground
    bool IsBodyAttachedToGround(ModelGraph const& modelGraph,
        BodyEl const& body,
        std::unordered_set<UID>& previouslyVisitedJoints)
    {
        bool childInAtLeastOneJoint = false;

        for (JointEl const& jointEl : modelGraph.iter<JointEl>())
        {
            OSC_ASSERT(!IsGarbageJoint(modelGraph, jointEl));

            if (jointEl.getChildID() == body.GetID())
            {
                childInAtLeastOneJoint = true;

                bool alreadyVisited = !previouslyVisitedJoints.emplace(jointEl.GetID()).second;
                if (alreadyVisited)
                {
                    continue;  // skip this joint: was previously visited
                }

                if (IsJointAttachedToGround(modelGraph, jointEl, previouslyVisitedJoints))
                {
                    return true;  // recurse
                }
            }
        }

        return !childInAtLeastOneJoint;
    }

    // returns `true` if `modelGraph` contains issues
    bool GetModelGraphIssues(ModelGraph const& modelGraph, std::vector<std::string>& issuesOut)
    {
        issuesOut.clear();

        for (JointEl const& joint : modelGraph.iter<JointEl>())
        {
            if (IsGarbageJoint(modelGraph, joint))
            {
                std::stringstream ss;
                ss << joint.GetLabel() << ": joint is garbage (this is an implementation error)";
                throw std::runtime_error{std::move(ss).str()};
            }
        }

        for (BodyEl const& body : modelGraph.iter<BodyEl>())
        {
            std::unordered_set<UID> previouslyVisitedJoints;
            if (!IsBodyAttachedToGround(modelGraph, body, previouslyVisitedJoints))
            {
                std::stringstream ss;
                ss << body.GetLabel() << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
                issuesOut.push_back(std::move(ss).str());
            }
        }

        return !issuesOut.empty();
    }

    // returns a string representing the subheader of a scene element
    std::string GetContextMenuSubHeaderText(ModelGraph const& mg, SceneEl const& e)
    {
        class Visitor final : public ConstSceneElVisitor {
        public:
            Visitor(std::stringstream& ss, ModelGraph const& mg) :
                m_SS{ss},
                m_Mg{mg}
            {
            }

            void operator()(GroundEl const&) final
            {
                m_SS << "(scene origin)";
            }
            void operator()(MeshEl const& m) final
            {
                m_SS << '(' << m.GetClass().GetNameSV() << ", " << m.getPath().filename().string() << ", attached to " << GetLabel(m_Mg, m.getParentID()) << ')';
            }
            void operator()(BodyEl const& b) final
            {
                m_SS << '(' << b.GetClass().GetNameSV() << ')';
            }
            void operator()(JointEl const& j) final
            {
                m_SS << '(' << j.GetSpecificTypeName() << ", " << GetLabel(m_Mg, j.getChildID()) << " --> " << GetLabel(m_Mg, j.getParentID()) << ')';
            }
            void operator()(StationEl const& s) final
            {
                m_SS << '(' << s.GetClass().GetNameSV() << ", attached to " << GetLabel(m_Mg, s.getParentID()) << ')';
            }

        private:
            std::stringstream& m_SS;
            ModelGraph const& m_Mg;
        };

        std::stringstream ss;
        Visitor v{ss, mg};
        e.Accept(v);
        return std::move(ss).str();
    }

    // returns true if the given element (ID) is in the "selection group" of
    bool IsInSelectionGroupOf(ModelGraph const& mg, UID parent, UID id)
    {
        if (id == c_EmptyID || parent == c_EmptyID)
        {
            return false;
        }

        if (id == parent)
        {
            return true;
        }

        BodyEl const* bodyEl = nullptr;

        if (BodyEl const* be = mg.TryGetElByID<BodyEl>(parent))
        {
            bodyEl = be;
        }
        else if (MeshEl const* me = mg.TryGetElByID<MeshEl>(parent))
        {
            bodyEl = mg.TryGetElByID<BodyEl>(me->getParentID());
        }

        if (!bodyEl)
        {
            return false;  // parent isn't attached to any body (or isn't a body)
        }

        if (BodyEl const* be = mg.TryGetElByID<BodyEl>(id))
        {
            return be->GetID() == bodyEl->GetID();
        }
        else if (MeshEl const* me = mg.TryGetElByID<MeshEl>(id))
        {
            return me->getParentID() == bodyEl->GetID();
        }
        else
        {
            return false;
        }
    }

    template<typename Consumer>
    void ForEachIDInSelectionGroup(ModelGraph const& mg, UID parent, Consumer f)
    {
        for (SceneEl const& e : mg.iter())
        {
            UID id = e.GetID();

            if (IsInSelectionGroupOf(mg, parent, id))
            {
                f(id);
            }
        }
    }

    void SelectAnythingGroupedWith(ModelGraph& mg, UID el)
    {
        ForEachIDInSelectionGroup(mg, el, [&mg](UID other)
            {
                mg.Select(other);
            });
    }

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the scene
    UIDT<BodyEl> GetStationAttachmentParent(ModelGraph const& mg, SceneEl const& el)
    {
        class Visitor final : public ConstSceneElVisitor {
        public:
            explicit Visitor(ModelGraph const& mg) : m_Mg{mg} {}

            void operator()(GroundEl const&) { m_Result = c_GroundID; }
            void operator()(MeshEl const& meshEl) { m_Mg.ContainsEl<BodyEl>(meshEl.getParentID()) ? m_Result = osc::DowncastID<BodyEl>(meshEl.getParentID()) : c_GroundID; }
            void operator()(BodyEl const& bodyEl) { m_Result = bodyEl.GetID(); }
            void operator()(JointEl const&) { m_Result = c_GroundID; }  // can't be attached
            void operator()(StationEl const&) { m_Result = c_GroundID; }  // can't be attached

            UIDT<BodyEl> result() const { return m_Result; }

        private:
            UIDT<BodyEl> m_Result = c_GroundID;
            ModelGraph const& m_Mg;
        };

        Visitor v{mg};
        el.Accept(v);
        return v.result();
    }

    // points an axis of a given element towards some other element in the model graph
    void PointAxisTowards(ModelGraph& mg, UID id, int axis, UID other)
    {
        glm::vec3 choicePos = GetPosition(mg, other);
        Transform sourceXform = Transform{GetPosition(mg, id)};

        mg.UpdElByID(id).SetXform(PointAxisTowards(sourceXform, axis, choicePos));
    }

    // returns recommended rim intensity for an element in the model graph
    osc::SceneDecorationFlags ComputeFlags(ModelGraph const& mg, UID id, UID hoverID = c_EmptyID)
    {
        if (id == c_EmptyID)
        {
            return osc::SceneDecorationFlags_None;
        }
        else if (mg.IsSelected(id))
        {
            return osc::SceneDecorationFlags_IsSelected;
        }
        else if (id == hoverID)
        {
            return osc::SceneDecorationFlags_IsHovered | osc::SceneDecorationFlags_IsChildOfHovered;
        }
        else if (IsInSelectionGroupOf(mg, hoverID, id))
        {
            return osc::SceneDecorationFlags_IsChildOfHovered;
        }
        else
        {
            return osc::SceneDecorationFlags_None;
        }
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
namespace
{
    // a single immutable and independent snapshot of the model, with a commit message + time
    // explaining what the snapshot "is" (e.g. "loaded file", "rotated body") and when it was
    // created
    class ModelGraphCommit final {
    public:
        ModelGraphCommit(
            UID parentID,  // can be c_EmptyID
            ClonePtr<ModelGraph> modelGraph,
            std::string_view commitMessage) :

            m_ID{},
            m_ParentID{parentID},
            m_ModelGraph{std::move(modelGraph)},
            m_CommitMessage{std::move(commitMessage)},
            m_CommitTime{std::chrono::system_clock::now()}
        {
        }

        UID GetID() const { return m_ID; }
        UID GetParentID() const { return m_ParentID; }
        ModelGraph const& GetModelGraph() const { return *m_ModelGraph; }
        std::string const& GetCommitMessage() const { return m_CommitMessage; }
        std::chrono::system_clock::time_point const& GetCommitTime() const { return m_CommitTime; }
        std::unique_ptr<ModelGraphCommit> clone() { return std::make_unique<ModelGraphCommit>(*this); }

    private:
        UID m_ID;
        UID m_ParentID;
        ClonePtr<ModelGraph> m_ModelGraph;
        std::string m_CommitMessage;
        std::chrono::system_clock::time_point m_CommitTime;
    };

    // undoable model graph storage
    class CommittableModelGraph final {
    public:
        CommittableModelGraph(std::unique_ptr<ModelGraph> mg) :
            m_Scratch{std::move(mg)},
            m_Current{c_EmptyID},
            m_BranchHead{c_EmptyID},
            m_Commits{}
        {
            Commit("created model graph");
        }

        CommittableModelGraph(ModelGraph const& mg) :
            CommittableModelGraph{std::make_unique<ModelGraph>(mg)}
        {
        }

        CommittableModelGraph() :
            CommittableModelGraph{std::make_unique<ModelGraph>()}
        {
        }

        UID Commit(std::string_view commitMsg)
        {
            auto snapshot = std::make_unique<ModelGraphCommit>(m_Current, ClonePtr<ModelGraph>{*m_Scratch}, commitMsg);
            UID id = snapshot->GetID();

            m_Commits.try_emplace(id, std::move(snapshot));
            m_Current = id;
            m_BranchHead = id;

            return id;
        }

        ModelGraphCommit const* TryGetCommitByID(UID id) const
        {
            auto it = m_Commits.find(id);
            return it != m_Commits.end() ? it->second.get() : nullptr;
        }

        ModelGraphCommit const& GetCommitByID(UID id) const
        {
            ModelGraphCommit const* ptr = TryGetCommitByID(id);
            if (!ptr)
            {
                std::stringstream ss;
                ss << "failed to find commit with ID = " << id;
                throw std::runtime_error{std::move(ss).str()};
            }
            return *ptr;
        }

        bool HasCommit(UID id) const
        {
            return TryGetCommitByID(id);
        }

        template<typename Consumer>
        void ForEachCommitUnordered(Consumer f) const
        {
            for (auto const& [id, commit] : m_Commits)
            {
                f(*commit);
            }
        }

        UID GetCheckoutID() const
        {
            return m_Current;
        }

        void Checkout(UID id)
        {
            ModelGraphCommit const* c = TryGetCommitByID(id);

            if (c)
            {
                m_Scratch = c->GetModelGraph();
                m_Current = c->GetID();
                m_BranchHead = c->GetID();
            }
        }

        bool CanUndo() const
        {
            ModelGraphCommit const* c = TryGetCommitByID(m_Current);
            return c ? c->GetParentID() != c_EmptyID : false;
        }

        void Undo()
        {
            ModelGraphCommit const* cur = TryGetCommitByID(m_Current);

            if (!cur)
            {
                return;
            }

            ModelGraphCommit const* parent = TryGetCommitByID(cur->GetParentID());

            if (parent)
            {
                m_Scratch = parent->GetModelGraph();
                m_Current = parent->GetID();
                // don't update m_BranchHead
            }
        }

        bool CanRedo() const
        {
            return m_BranchHead != m_Current && HasCommit(m_BranchHead);
        }

        void Redo()
        {
            if (m_BranchHead == m_Current)
            {
                return;
            }

            ModelGraphCommit const* c = TryGetCommitByID(m_BranchHead);
            while (c && c->GetParentID() != m_Current)
            {
                c = TryGetCommitByID(c->GetParentID());
            }

            if (c)
            {
                m_Scratch = c->GetModelGraph();
                m_Current = c->GetID();
                // don't update m_BranchHead
            }
        }

        ModelGraph& UpdScratch()
        {
            return *m_Scratch;
        }

        ModelGraph const& GetScratch() const
        {
            return *m_Scratch;
        }

        void GarbageCollect()
        {
            m_Scratch->GarbageCollect();
        }

    private:
        ClonePtr<ModelGraph> m_Scratch;  // mutable staging area
        UID m_Current;  // where scratch will commit to
        UID m_BranchHead;  // head of current branch (for redo)
        std::unordered_map<UID, ClonePtr<ModelGraphCommit>> m_Commits;
    };

    bool PointAxisTowards(CommittableModelGraph& cmg, UID id, int axis, UID other)
    {
        PointAxisTowards(cmg.UpdScratch(), id, axis, other);
        cmg.Commit("reoriented " + GetLabel(cmg.GetScratch(), id));
        return true;
    }

    bool TryAssignMeshAttachments(CommittableModelGraph& cmg, std::unordered_set<UID> const& meshIDs, UID newAttachment)
    {
        ModelGraph& mg = cmg.UpdScratch();

        if (newAttachment != c_GroundID && !mg.ContainsEl<BodyEl>(newAttachment))
        {
            return false;  // bogus ID passed
        }

        for (UID id : meshIDs)
        {
            MeshEl* ptr = mg.TryUpdElByID<MeshEl>(id);

            if (!ptr)
            {
                continue;  // hardening: ignore invalid assignments
            }

            ptr->setParentID(osc::DowncastID<BodyEl>(newAttachment));
        }

        std::stringstream commitMsg;
        commitMsg << "assigned mesh";
        if (meshIDs.size() > 1)
        {
            commitMsg << "es";
        }
        commitMsg << " to " << mg.GetElByID(newAttachment).GetLabel();


        cmg.Commit(std::move(commitMsg).str());

        return true;
    }

    bool TryCreateJoint(CommittableModelGraph& cmg, UID childID, UID parentID)
    {
        ModelGraph& mg = cmg.UpdScratch();

        size_t jointTypeIdx = *osc::JointRegistry::indexOf<OpenSim::WeldJoint>();
        glm::vec3 parentPos = GetPosition(mg, parentID);
        glm::vec3 childPos = GetPosition(mg, childID);
        glm::vec3 midPoint = osc::Midpoint(parentPos, childPos);

        JointEl& jointEl = mg.AddEl<JointEl>(jointTypeIdx, "", parentID, osc::DowncastID<BodyEl>(childID), Transform{midPoint});
        SelectOnly(mg, jointEl);

        cmg.Commit("added " + jointEl.GetLabel());

        return true;
    }

    bool TryOrientElementAxisAlongTwoPoints(CommittableModelGraph& cmg, UID id, int axis, glm::vec3 p1, glm::vec3 p2)
    {
        ModelGraph& mg = cmg.UpdScratch();
        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        glm::vec3 dir = glm::normalize(p2 - p1);
        Transform t = el->GetXform();

        el->SetXform(PointAxisAlong(t, axis, dir));
        cmg.Commit("reoriented " + el->GetLabel());

        return true;
    }

    bool TryTranslateElementBetweenTwoPoints(CommittableModelGraph& cmg, UID id, glm::vec3 const& a, glm::vec3 const& b)
    {
        ModelGraph& mg = cmg.UpdScratch();
        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        el->SetPos(osc::Midpoint(a, b));
        cmg.Commit("translated " + el->GetLabel());

        return true;
    }

    bool TryTranslateBetweenTwoElements(CommittableModelGraph& cmg, UID id, UID a, UID b)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        SceneEl const* aEl = mg.TryGetElByID(a);

        if (!aEl)
        {
            return false;
        }

        SceneEl const* bEl = mg.TryGetElByID(b);

        if (!bEl)
        {
            return false;
        }

        el->SetPos(osc::Midpoint(aEl->GetPos(), bEl->GetPos()));
        cmg.Commit("translated " + el->GetLabel());

        return true;
    }

    bool TryTranslateElementToAnotherElement(CommittableModelGraph& cmg, UID id, UID other)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        SceneEl* otherEl = mg.TryUpdElByID(other);

        if (!otherEl)
        {
            return false;
        }

        el->SetPos(otherEl->GetPos());
        cmg.Commit("moved " + el->GetLabel());

        return true;
    }

    bool TryTranslateToMeshAverageCenter(CommittableModelGraph& cmg, UID id, UID meshID)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        MeshEl const* mesh = mg.TryGetElByID<MeshEl>(meshID);

        if (!mesh)
        {
            return false;
        }

        el->SetPos(AverageCenter(*mesh));
        cmg.Commit("moved " + el->GetLabel());

        return true;
    }

    bool TryTranslateToMeshBoundsCenter(CommittableModelGraph& cmg, UID id, UID meshID)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl* const el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        MeshEl const* mesh = mg.TryGetElByID<MeshEl>(meshID);

        if (!mesh)
        {
            return false;
        }

        glm::vec3 const boundsMidpoint = Midpoint(mesh->CalcBounds());

        el->SetPos(boundsMidpoint);
        cmg.Commit("moved " + el->GetLabel());

        return true;
    }

    bool TryTranslateToMeshMassCenter(CommittableModelGraph& cmg, UID id, UID meshID)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl* const el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        MeshEl const* mesh = mg.TryGetElByID<MeshEl>(meshID);

        if (!mesh)
        {
            return false;
        }

        el->SetPos(MassCenter(*mesh));
        cmg.Commit("moved " + el->GetLabel());

        return true;
    }

    bool TryReassignCrossref(CommittableModelGraph& cmg, UID id, int crossref, UID other)
    {
        if (other == id)
        {
            return false;
        }

        ModelGraph& mg = cmg.UpdScratch();
        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        if (!mg.ContainsEl(other))
        {
            return false;
        }

        el->SetCrossReferenceConnecteeID(crossref, other);
        cmg.Commit("reassigned " + el->GetLabel() + " " + el->GetCrossReferenceLabel(crossref));

        return true;
    }

    bool DeleteSelected(CommittableModelGraph& cmg)
    {
        ModelGraph& mg = cmg.UpdScratch();

        if (!HasSelection(mg))
        {
            return false;
        }

        DeleteSelected(cmg.UpdScratch());
        cmg.Commit("deleted selection");

        return true;
    }

    bool DeleteEl(CommittableModelGraph& cmg, UID id)
    {
        ModelGraph& mg = cmg.UpdScratch();
        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        std::string label = to_string(el->GetLabel());

        if (!mg.DeleteEl(*el))
        {
            return false;
        }

        cmg.Commit("deleted " + label);
        return true;
    }

    void RotateAxisXRadians(CommittableModelGraph& cmg, SceneEl& el, int axis, float radians)
    {
        el.SetXform(RotateAlongAxis(el.GetXform(), axis, radians));
        cmg.Commit("reoriented " + el.GetLabel());
    }

    bool TryCopyOrientation(CommittableModelGraph& cmg, UID id, UID other)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl* el = mg.TryUpdElByID(id);

        if (!el)
        {
            return false;
        }

        SceneEl* otherEl = mg.TryUpdElByID(other);

        if (!otherEl)
        {
            return false;
        }

        el->SetRotation(otherEl->GetRotation());
        cmg.Commit("reoriented " + el->GetLabel());

        return true;
    }


    UIDT<BodyEl> AddBody(CommittableModelGraph& cmg, glm::vec3 const& pos, UID andTryAttach)
    {
        ModelGraph& mg = cmg.UpdScratch();

        BodyEl& b = mg.AddEl<BodyEl>(GenerateName(BodyEl::Class()), Transform{pos});
        mg.DeSelectAll();
        mg.Select(b.GetID());

        MeshEl* el = mg.TryUpdElByID<MeshEl>(andTryAttach);
        if (el)
        {
            if (el->getParentID() == c_GroundID || el->getParentID() == c_EmptyID)
            {
                el->setParentID(b.GetID());
                mg.Select(*el);
            }
        }

        cmg.Commit(std::string{"added "} + b.GetLabel());

        return b.GetID();
    }

    UIDT<BodyEl> AddBody(CommittableModelGraph& cmg)
    {
        return AddBody(cmg, {}, c_EmptyID);
    }

    bool AddStationAtLocation(CommittableModelGraph& cmg, SceneEl const& el, glm::vec3 const& loc)
    {
        ModelGraph& mg = cmg.UpdScratch();

        if (!CanAttachStationTo(el))
        {
            return false;
        }

        StationEl& station = mg.AddEl<StationEl>(UIDT<StationEl>{}, GetStationAttachmentParent(mg, el), loc, GenerateName(StationEl::Class()));
        SelectOnly(mg, station);
        cmg.Commit("added station " + station.GetLabel());
        return true;
    }

    bool AddStationAtLocation(CommittableModelGraph& cmg, UID elID, glm::vec3 const& loc)
    {
        ModelGraph& mg = cmg.UpdScratch();

        SceneEl const* el = mg.TryGetElByID(elID);

        if (!el)
        {
            return false;
        }

        return AddStationAtLocation(cmg, *el, loc);
    }
}

// OpenSim::Model generation support
//
// the ModelGraph that this UI manipulates ultimately needs to be transformed
// into a standard OpenSim model. This code does that.
namespace
{
    // stand-in method that should be replaced by actual support for scale-less transforms
    // (dare i call them.... frames ;))
    Transform IgnoreScale(Transform const& t)
    {
        Transform copy{t};
        copy.scale = {1.0f, 1.0f, 1.0f};
        return copy;
    }

    // attaches a mesh to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
    void AttachMeshElToFrame(MeshEl const& meshEl,
        Transform const& parentXform,
        OpenSim::PhysicalFrame& parentPhysFrame)
    {
        // create a POF that attaches to the body
        auto meshPhysOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
        meshPhysOffsetFrame->setName(std::string{meshEl.GetLabel()} + "_offset");

        // set the POFs transform to be equivalent to the mesh's (in-ground) transform,
        // but in the parent frame
        SimTK::Transform mesh2ground = ToSimTKTransform(meshEl.GetXform());
        SimTK::Transform parent2ground = ToSimTKTransform(parentXform);
        meshPhysOffsetFrame->setOffsetTransform(parent2ground.invert() * mesh2ground);

        // attach the mesh data to the transformed POF
        auto mesh = std::make_unique<OpenSim::Mesh>(meshEl.getPath().string());
        mesh->setName(std::string{meshEl.GetLabel()});
        mesh->set_scale_factors(osc::ToSimTKVec3(meshEl.GetXform().scale));
        meshPhysOffsetFrame->attachGeometry(mesh.release());

        // make it a child of the parent's physical frame
        parentPhysFrame.addComponent(meshPhysOffsetFrame.release());
    }

    // create a body for the `model`, but don't add it to the model yet
    //
    // *may* add any attached meshes to the model, though
    std::unique_ptr<OpenSim::Body> CreateDetatchedBody(ModelGraph const& mg, BodyEl const& bodyEl)
    {
        auto addedBody = std::make_unique<OpenSim::Body>();

        addedBody->setName(std::string{bodyEl.GetLabel()});
        addedBody->setMass(bodyEl.getMass());

        // HACK: set the inertia of the emitted body to be nonzero
        //
        // the reason we do this is because having a zero inertia on a body can cause
        // the simulator to freak out in some scenarios.
        {
            double moment = 0.01 * bodyEl.getMass();
            SimTK::Vec3 moments{moment, moment, moment};
            SimTK::Vec3 products{0.0, 0.0, 0.0};
            addedBody->setInertia(SimTK::Inertia{moments, products});
        }

        // connect meshes to the body, if necessary
        //
        // the body's orientation is going to be handled when the joints are added (by adding
        // relevant offset frames etc.)
        for (MeshEl const& mesh : mg.iter<MeshEl>())
        {
            if (mesh.getParentID() == bodyEl.GetID())
            {
                AttachMeshElToFrame(mesh, bodyEl.GetXform(), *addedBody);
            }
        }

        return addedBody;
    }

    // result of a lookup for (effectively) a physicalframe
    struct JointAttachmentCachedLookupResult {
        // can be nullptr (indicating Ground)
        BodyEl const* bodyEl;

        // can be nullptr (indicating ground/cache hit)
        std::unique_ptr<OpenSim::Body> createdBody;

        // always != nullptr, can point to `createdBody`, or an existing body from the cache, or Ground
        OpenSim::PhysicalFrame* physicalFrame;
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
        rv.bodyEl = mg.TryGetElByID<BodyEl>(elID);
        rv.createdBody = nullptr;
        rv.physicalFrame = nullptr;

        if (rv.bodyEl)
        {
            auto it = visitedBodies.find(elID);
            if (it == visitedBodies.end())
            {
                // haven't visited the body before
                rv.createdBody = CreateDetatchedBody(mg, *rv.bodyEl);
                rv.physicalFrame = rv.createdBody.get();

                // add it to the cache
                visitedBodies.emplace(elID, rv.createdBody.get());
            }
            else
            {
                // visited the body before, use cached result
                rv.createdBody = nullptr;  // it's not this function's responsibility to add it
                rv.physicalFrame = it->second;
            }
        }
        else
        {
            // the element is connected to ground
            rv.createdBody = nullptr;
            rv.physicalFrame = &model.updGround();
        }

        return rv;
    }

    // compute the name of a joint from its attached frames
    std::string CalcJointName(
        JointEl const& jointEl,
        OpenSim::PhysicalFrame const& parentFrame,
        OpenSim::PhysicalFrame const& childFrame)
    {
        if (!jointEl.getUserAssignedName().empty())
        {
            return std::string{jointEl.getUserAssignedName()};
        }
        else
        {
            return childFrame.getName() + "_to_" + parentFrame.getName();
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
        OpenSim::Joint const* proto = osc::JointRegistry::prototypes()[jointTypeIdx].get();
        size_t typeHash = typeid(*proto).hash_code();

        if (typeHash == typeid(OpenSim::FreeJoint).hash_code())
        {
            return JointDegreesOfFreedom{{0, 1, 2}, {3, 4, 5}};
        }
        else if (typeHash == typeid(OpenSim::PinJoint).hash_code())
        {
            return JointDegreesOfFreedom{{-1, -1, 0}, {-1, -1, -1}};
        }
        else
        {
            return JointDegreesOfFreedom{};  // unknown joint type
        }
    }

    glm::vec3 GetJointAxisLengths(JointEl const& joint)
    {
        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(joint.getJointTypeIndex());

        glm::vec3 rv;
        for (int i = 0; i < 3; ++i)
        {
            rv[i] = dofs.orientation[static_cast<size_t>(i)] == -1 ? 0.6f : 1.0f;
        }
        return rv;
    }

    // sets the names of a joint's coordinates
    void SetJointCoordinateNames(
        OpenSim::Joint& joint,
        std::string const& prefix)
    {
        constexpr std::array<char const*, 3> const translationNames = {"_tx", "_ty", "_tz"};
        constexpr std::array<char const*, 3> const rotationNames = {"_rx", "_ry", "_rz"};

        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(*osc::JointRegistry::indexOf(joint));

        // translations
        for (int i = 0; i < 3; ++i)
        {
            if (dofs.translation[i] != -1)
            {
                joint.upd_coordinates(dofs.translation[i]).setName(prefix + translationNames[i]);
            }
        }

        // rotations
        for (int i = 0; i < 3; ++i)
        {
            if (dofs.orientation[i] != -1)
            {
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
        {
            bool wasInserted = visitedJoints.emplace(joint.GetID()).second;
            if (!wasInserted)
            {
                // graph cycle detected: joint was already previously visited and shouldn't be traversed again
                return;
            }
        }

        // lookup each side of the joint, creating the bodies if necessary
        JointAttachmentCachedLookupResult parent = LookupPhysFrame(mg, model, visitedBodies, joint.getParentID());
        JointAttachmentCachedLookupResult child = LookupPhysFrame(mg, model, visitedBodies, joint.getChildID());

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parentPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parentPOF->setName(parent.physicalFrame->getName() + "_offset");
        parentPOF->setParentFrame(*parent.physicalFrame);
        glm::mat4 toParentPofInParent =  ToInverseMat4(IgnoreScale(GetTransform(mg, joint.getParentID()))) * ToMat4(IgnoreScale(joint.GetXform()));
        parentPOF->set_translation(osc::ToSimTKVec3(toParentPofInParent[3]));
        parentPOF->set_orientation(osc::ToSimTKVec3(osc::ExtractEulerAngleXYZ(toParentPofInParent)));

        // create the child OpenSim::PhysicalOffsetFrame
        auto childPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        childPOF->setName(child.physicalFrame->getName() + "_offset");
        childPOF->setParentFrame(*child.physicalFrame);
        glm::mat4 toChildPofInChild = ToInverseMat4(IgnoreScale(GetTransform(mg, joint.getChildID()))) * ToMat4(IgnoreScale(joint.GetXform()));
        childPOF->set_translation(osc::ToSimTKVec3(toChildPofInChild[3]));
        childPOF->set_orientation(osc::ToSimTKVec3(osc::ExtractEulerAngleXYZ(toChildPofInChild)));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto jointUniqPtr = std::unique_ptr<OpenSim::Joint>(osc::JointRegistry::prototypes()[joint.getJointTypeIndex()]->clone());

        // set its name
        std::string jointName = CalcJointName(joint, *parent.physicalFrame, *child.physicalFrame);
        jointUniqPtr->setName(jointName);

        // set joint coordinate names
        SetJointCoordinateNames(*jointUniqPtr, jointName);

        // add + connect the joint to the POFs
        OpenSim::PhysicalOffsetFrame* parentPtr = parentPOF.get();
        OpenSim::PhysicalOffsetFrame* childPtr = childPOF.get();
        jointUniqPtr->addFrame(parentPOF.release());  // care: ownership change happens here (#642)
        jointUniqPtr->addFrame(childPOF.release());  // care: ownership change happens here (#642)
        jointUniqPtr->connectSocket_parent_frame(*parentPtr);
        jointUniqPtr->connectSocket_child_frame(*childPtr);

        // if a child body was created during this step (e.g. because it's not a cyclic connection)
        // then add it to the model
        OSC_ASSERT_ALWAYS(parent.createdBody == nullptr && "at this point in the algorithm, all parents should have already been created");
        if (child.createdBody)
        {
            model.addBody(child.createdBody.release());  // add created body to model
        }

        // add the joint to the model
        model.addJoint(jointUniqPtr.release());

        // if there are any meshes attached to the joint, attach them to the parent
        for (MeshEl const& mesh : mg.iter<MeshEl>())
        {
            if (mesh.getParentID() == joint.GetID())
            {
                AttachMeshElToFrame(mesh, joint.GetXform(), *parentPtr);
            }
        }

        // recurse by finding where the child of this joint is the parent of some other joint
        OSC_ASSERT_ALWAYS(child.bodyEl != nullptr && "child should always be an identifiable body element");
        for (JointEl const& otherJoint : mg.iter<JointEl>())
        {
            if (otherJoint.getParentID() == child.bodyEl->GetID())
            {
                AttachJointRecursive(mg, model, otherJoint, visitedBodies, visitedJoints);
            }
        }
    }

    // attaches `BodyEl` into `model` by directly attaching it to ground with a WeldJoint
    void AttachBodyDirectlyToGround(ModelGraph const& mg,
        OpenSim::Model& model,
        BodyEl const& bodyEl,
        std::unordered_map<UID, OpenSim::Body*>& visitedBodies)
    {
        std::unique_ptr<OpenSim::Body> addedBody = CreateDetatchedBody(mg, bodyEl);
        auto weldJoint = std::make_unique<OpenSim::WeldJoint>();
        auto parentFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        auto childFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();

        // set names
        weldJoint->setName(std::string{bodyEl.GetLabel()} + "_to_ground");
        parentFrame->setName("ground_offset");
        childFrame->setName(std::string{bodyEl.GetLabel()} + "_offset");

        // make the parent have the same position + rotation as the placed body
        parentFrame->setOffsetTransform(ToSimTKTransform(bodyEl.GetXform()));

        // attach the parent directly to ground and the child directly to the body
        // and make them the two attachments of the joint
        parentFrame->setParentFrame(model.getGround());
        childFrame->setParentFrame(*addedBody);
        weldJoint->connectSocket_parent_frame(*parentFrame);
        weldJoint->connectSocket_child_frame(*childFrame);

        // populate the "already visited bodies" cache
        visitedBodies[bodyEl.GetID()] = addedBody.get();

        // add the components into the OpenSim::Model
        weldJoint->addFrame(parentFrame.release());
        weldJoint->addFrame(childFrame.release());
        model.addBody(addedBody.release());
        model.addJoint(weldJoint.release());
    }

    void AddStationToModel(ModelGraph const& mg,
        OpenSim::Model& model,
        StationEl const& stationEl,
        std::unordered_map<UID, OpenSim::Body*>& visitedBodies)
    {

        JointAttachmentCachedLookupResult res = LookupPhysFrame(mg, model, visitedBodies, stationEl.getParentID());
        OSC_ASSERT_ALWAYS(res.physicalFrame != nullptr && "all physical frames should have been added by this point in the model-building process");

        SimTK::Transform parentXform = ToSimTKTransform(mg.GetElByID(stationEl.getParentID()).GetXform());
        SimTK::Transform stationXform = ToSimTKTransform(stationEl.GetXform());
        SimTK::Vec3 locationInParent = (parentXform.invert() * stationXform).p();

        auto station = std::make_unique<OpenSim::Station>(*res.physicalFrame, locationInParent);
        station->setName(to_string(stationEl.GetLabel()));
        res.physicalFrame->addComponent(station.release());
    }

    // if there are no issues, returns a new OpenSim::Model created from the Modelgraph
    //
    // otherwise, returns nullptr and issuesOut will be populated with issue messages
    std::unique_ptr<OpenSim::Model> CreateOpenSimModelFromModelGraph(ModelGraph const& mg,
        std::vector<std::string>& issuesOut)
    {
        if (GetModelGraphIssues(mg, issuesOut))
        {
            osc::log::error("cannot create an osim model: issues detected");
            for (std::string const& issue : issuesOut)
            {
                osc::log::error("issue: %s", issue.c_str());
            }
            return nullptr;
        }

        // create the output model
        auto model = std::make_unique<OpenSim::Model>();
        model->updDisplayHints().upd_show_frames() = true;

        // add any meshes that are directly connected to ground (i.e. meshes that are not attached to a body)
        for (MeshEl const& meshEl : mg.iter<MeshEl>())
        {
            if (meshEl.getParentID() == c_GroundID)
            {
                AttachMeshElToFrame(meshEl, Transform{}, model->updGround());
            }
        }

        // keep track of any bodies/joints already visited (there might be cycles)
        std::unordered_map<UID, OpenSim::Body*> visitedBodies;
        std::unordered_set<UID> visitedJoints;

        // directly connect any bodies that participate in no joints into the model with a default joint
        for (BodyEl const& bodyEl : mg.iter<BodyEl>())
        {
            if (!IsAChildAttachmentInAnyJoint(mg, bodyEl))
            {
                AttachBodyDirectlyToGround(mg, *model, bodyEl, visitedBodies);
            }
        }

        // add bodies that do participate in joints into the model
        //
        // note: these bodies may use the non-participating bodies (above) as parents
        for (JointEl const& jointEl : mg.iter<JointEl>())
        {
            if (jointEl.getParentID() == c_GroundID || ContainsKey(visitedBodies, jointEl.getParentID()))
            {
                AttachJointRecursive(mg, *model, jointEl, visitedBodies, visitedJoints);
            }
        }

        // add stations into the model
        for (StationEl const& el : mg.iter<StationEl>())
        {
            AddStationToModel(mg, *model, el, visitedBodies);
        }

        // invalidate all properties, so that model.finalizeFromProperties() *must*
        // reload everything with no caching
        //
        // otherwise, parts of the model (cough cough, OpenSim::Geometry::finalizeFromProperties)
        // will fail to load data because it will internally set itself as up to date, even though
        // it failed to load a mesh file because a parent was missing. See #330
        for (OpenSim::Component& c : model->updComponentList())
        {
            for (int i = 0; i < c.getNumProperties(); ++i)
            {
                c.updPropertyByIndex(i);
            }
        }

        // ensure returned model is initialized from latest graph
        model->finalizeConnections();  // ensure all sockets are finalized to paths (#263)
        osc::InitializeModel(*model);
        osc::InitializeState(*model);

        return model;
    }

    // tries to find the first body connected to the given PhysicalFrame by assuming
    // that the frame is either already a body or is an offset to a body
    OpenSim::PhysicalFrame const* TryInclusiveRecurseToBodyOrGround(OpenSim::Frame const& f, std::unordered_set<OpenSim::Frame const*> visitedFrames)
    {
        if (!visitedFrames.emplace(&f).second)
        {
            return nullptr;
        }

        if (auto const* body = dynamic_cast<OpenSim::Body const*>(&f))
        {
            return body;
        }
        else if (auto const* ground = dynamic_cast<OpenSim::Ground const*>(&f))
        {
            return ground;
        }
        else if (auto const* pof = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&f))
        {
            return TryInclusiveRecurseToBodyOrGround(pof->getParentFrame(), visitedFrames);
        }
        else if (auto const* station = dynamic_cast<OpenSim::Station const*>(&f))
        {
            return TryInclusiveRecurseToBodyOrGround(station->getParentFrame(), visitedFrames);
        }
        else
        {
            return nullptr;
        }
    }

    // tries to find the first body connected to the given PhysicalFrame by assuming
    // that the frame is either already a body or is an offset to a body
    OpenSim::PhysicalFrame const* TryInclusiveRecurseToBodyOrGround(OpenSim::Frame const& f)
    {
        return TryInclusiveRecurseToBodyOrGround(f, {});
    }

    ModelGraph CreateModelGraphFromInMemoryModel(OpenSim::Model m)
    {
        // init model+state
        osc::InitializeModel(m);
        SimTK::State const& st = osc::InitializeState(m);

        // this is what this method populates
        ModelGraph rv;

        // used to figure out how a body in the OpenSim::Model maps into the ModelGraph
        std::unordered_map<OpenSim::Body const*, UIDT<BodyEl>> bodyLookup;

        // used to figure out how a joint in the OpenSim::Model maps into the ModelGraph
        std::unordered_map<OpenSim::Joint const*, UIDT<JointEl>> jointLookup;

        // import all the bodies from the model file
        for (OpenSim::Body const& b : m.getComponentList<OpenSim::Body>())
        {
            std::string name = b.getName();
            Transform xform = ToOsimTransform(b.getTransformInGround(st));

            BodyEl& el = rv.AddEl<BodyEl>(name, xform);
            el.setMass(b.getMass());

            bodyLookup.emplace(&b, el.GetID());
        }

        // then try and import all the joints (by looking at their connectivity)
        for (OpenSim::Joint const& j : m.getComponentList<OpenSim::Joint>())
        {
            OpenSim::PhysicalFrame const& parentFrame = j.getParentFrame();
            OpenSim::PhysicalFrame const& childFrame = j.getChildFrame();

            OpenSim::PhysicalFrame const* parentBodyOrGround = TryInclusiveRecurseToBodyOrGround(parentFrame);
            OpenSim::PhysicalFrame const* childBodyOrGround = TryInclusiveRecurseToBodyOrGround(childFrame);

            if (!parentBodyOrGround || !childBodyOrGround)
            {
                // can't find what they're connected to
                continue;
            }

            auto maybeType = osc::JointRegistry::indexOf(j);

            if (!maybeType)
            {
                // joint has a type the mesh importer doesn't support
                continue;
            }

            size_t type = maybeType.value();
            std::string name = j.getName();

            UID parent = c_EmptyID;

            if (dynamic_cast<OpenSim::Ground const*>(parentBodyOrGround))
            {
                parent = c_GroundID;
            }
            else
            {
                auto it = bodyLookup.find(static_cast<OpenSim::Body const*>(parentBodyOrGround));
                if (it == bodyLookup.end())
                {
                    // joint is attached to a body that isn't ground or cached?
                    continue;
                }
                else
                {
                    parent = it->second;
                }
            }

            UIDT<BodyEl> child = osc::DowncastID<BodyEl>(c_EmptyID);

            if (dynamic_cast<OpenSim::Ground const*>(childBodyOrGround))
            {
                // ground can't be a child in a joint
                continue;
            }
            else
            {
                auto it = bodyLookup.find(static_cast<OpenSim::Body const*>(childBodyOrGround));
                if (it == bodyLookup.end())
                {
                    // joint is attached to a body that isn't ground or cached?
                    continue;
                }
                else
                {
                    child = it->second;
                }
            }

            if (parent == c_EmptyID || child == c_EmptyID)
            {
                // something horrible happened above
                continue;
            }

            Transform xform = ToOsimTransform(parentFrame.getTransformInGround(st));

            JointEl& jointEl = rv.AddEl<JointEl>(type, name, parent, child, xform);
            jointLookup.emplace(&j, jointEl.GetID());
        }


        // then try to import all the meshes
        for (OpenSim::Mesh const& mesh : m.getComponentList<OpenSim::Mesh>())
        {
            std::optional<std::filesystem::path> maybeMeshPath = osc::FindGeometryFileAbsPath(m, mesh);

            if (!maybeMeshPath)
            {
                continue;
            }

            std::filesystem::path const& realLocation = *maybeMeshPath;

            Mesh meshData;
            try
            {
                meshData = osc::LoadMeshViaSimTK(realLocation.string());
            }
            catch (std::exception const& ex)
            {
                osc::log::error("error loading mesh: %s", ex.what());
                continue;
            }

            OpenSim::Frame const& frame = mesh.getFrame();
            OpenSim::PhysicalFrame const* frameBodyOrGround = TryInclusiveRecurseToBodyOrGround(frame);

            if (!frameBodyOrGround)
            {
                // can't find what it's connected to?
                continue;
            }

            UID attachment = c_EmptyID;
            if (dynamic_cast<OpenSim::Ground const*>(frameBodyOrGround))
            {
                attachment = c_GroundID;
            }
            else
            {
                if (auto bodyIt = bodyLookup.find(static_cast<OpenSim::Body const*>(frameBodyOrGround)); bodyIt != bodyLookup.end())
                {
                    attachment = bodyIt->second;
                }
                else
                {
                    // mesh is attached to something that isn't a ground or a body?
                    continue;
                }
            }

            if (attachment == c_EmptyID)
            {
                // couldn't figure out what to attach to
                continue;
            }

            MeshEl& el = rv.AddEl<MeshEl>(attachment, meshData, realLocation);
            osc::Transform newTransform = ToOsimTransform(frame.getTransformInGround(st));
            newTransform.scale = osc::ToVec3(mesh.get_scale_factors());

            el.SetXform(newTransform);
            el.SetLabel(mesh.getName());
        }

        // then try to import all the stations
        for (OpenSim::Station const& station : m.getComponentList<OpenSim::Station>())
        {
            // edge-case: it's a path point: ignore it because it will spam the converter
            if (dynamic_cast<OpenSim::AbstractPathPoint const*>(&station))
            {
                continue;
            }

            if (dynamic_cast<OpenSim::AbstractPathPoint const*>(&station.getOwner()))
            {
                continue;
            }

            OpenSim::PhysicalFrame const& frame = station.getParentFrame();
            OpenSim::PhysicalFrame const* frameBodyOrGround = TryInclusiveRecurseToBodyOrGround(frame);

            UID attachment = c_EmptyID;
            if (dynamic_cast<OpenSim::Ground const*>(frameBodyOrGround))
            {
                attachment = c_GroundID;
            }
            else
            {
                if (auto it = bodyLookup.find(static_cast<OpenSim::Body const*>(frameBodyOrGround)); it != bodyLookup.end())
                {
                    attachment = it->second;
                }
                else
                {
                    // station is attached to something that isn't ground or a cached body
                    continue;
                }
            }

            if (attachment == c_EmptyID)
            {
                // can't figure out what to attach to
                continue;
            }

            glm::vec3 pos = osc::ToVec3(station.findLocationInFrame(st, m.getGround()));
            std::string name = station.getName();

            rv.AddEl<StationEl>(osc::DowncastID<BodyEl>(attachment), pos, name);
        }

        return rv;
    }

    ModelGraph CreateModelFromOsimFile(std::filesystem::path const& p)
    {
        return CreateModelGraphFromInMemoryModel(OpenSim::Model{p.string()});
    }
}

// shared data support
//
// data that's shared between multiple UI states.
namespace
{
    // a class that holds hover user mousehover information
    class Hover final {
    public:
        Hover() :
            ID{c_EmptyID},
            Pos{}
        {
        }
        Hover(UID id_, glm::vec3 pos_) :
            ID{id_},
            Pos{pos_}
        {
        }
        explicit operator bool () const noexcept
        {
            return ID != c_EmptyID;
        }
        void reset()
        {
            *this = Hover{};
        }

        UID ID;
        glm::vec3 Pos;
    };

    class SharedData final {
    public:
        SharedData() = default;

        SharedData(std::vector<std::filesystem::path> meshFiles)
        {
            PushMeshLoadRequests(std::move(meshFiles));
        }


        //
        // OpenSim OUTPUT MODEL STUFF
        //

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
            try
            {
                m_MaybeOutputModel = CreateOpenSimModelFromModelGraph(GetModelGraph(), m_IssuesBuffer);
            }
            catch (std::exception const& ex)
            {
                osc::log::error("error occurred while trying to create an OpenSim model from the mesh editor scene: %s", ex.what());
            }
        }


        //
        // MODEL GRAPH STUFF
        //

        bool OpenOsimFileAsModelGraph()
        {
            std::optional<std::filesystem::path> const maybeOsimPath = osc::PromptUserForFile("osim");

            if (maybeOsimPath)
            {
                m_ModelGraphSnapshots = CommittableModelGraph{CreateModelFromOsimFile(*maybeOsimPath)};
                m_MaybeModelGraphExportLocation = *maybeOsimPath;
                m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.GetCheckoutID();
                return true;
            }
            else
            {
                return false;
            }
        }

        bool ExportModelGraphTo(std::filesystem::path const& exportPath)
        {
            std::vector<std::string> issues;
            std::unique_ptr<OpenSim::Model> m;

            try
            {
                m = CreateOpenSimModelFromModelGraph(GetModelGraph(), issues);
            }
            catch (std::exception const& ex)
            {
                osc::log::error("error occurred while trying to create an OpenSim model from the mesh editor scene: %s", ex.what());
            }

            if (m)
            {
                m->print(exportPath.string());
                m_MaybeModelGraphExportLocation = exportPath;
                m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.GetCheckoutID();
                return true;
            }
            else
            {
                for (std::string const& issue : issues)
                {
                    osc::log::error("%s", issue.c_str());
                }
                return false;
            }
        }

        bool ExportAsModelGraphAsOsimFile()
        {
            std::optional<std::filesystem::path> const maybeExportPath =
                osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("osim");

            if (!maybeExportPath)
            {
                return false;  // user probably cancelled out
            }

            return ExportModelGraphTo(*maybeExportPath);
        }

        bool ExportModelGraphAsOsimFile()
        {
            if (m_MaybeModelGraphExportLocation.empty())
            {
                return ExportAsModelGraphAsOsimFile();
            }

            return ExportModelGraphTo(m_MaybeModelGraphExportLocation);
        }

        bool IsModelGraphUpToDateWithDisk() const
        {
            return m_MaybeModelGraphExportedUID == m_ModelGraphSnapshots.GetCheckoutID();
        }

        bool IsCloseRequested() const
        {
            return m_CloseRequested;
        }

        void RequestClose()
        {
            m_CloseRequested = true;
        }

        void ResetRequestClose()
        {
            m_CloseRequested = false;
        }

        bool IsNewMeshImpoterTabRequested()
        {
            return m_NewTabRequested == true;
        }

        void RequestNewMeshImporterTab()
        {
            m_NewTabRequested = true;
        }

        void ResetRequestNewMeshImporter()
        {
            m_NewTabRequested = false;
        }

        std::string GetDocumentName() const
        {
            if (m_MaybeModelGraphExportLocation.empty())
            {
                return "untitled.osim";
            }
            else
            {
                return m_MaybeModelGraphExportLocation.filename().string();
            }
        }

        std::string GetRecommendedTitle() const
        {
            std::stringstream ss;
            ss << ICON_FA_CUBE << ' ' << GetDocumentName();
            return std::move(ss).str();
        }

        ModelGraph const& GetModelGraph() const
        {
            return m_ModelGraphSnapshots.GetScratch();
        }

        ModelGraph& UpdModelGraph()
        {
            return m_ModelGraphSnapshots.UpdScratch();
        }

        CommittableModelGraph& UpdCommittableModelGraph()
        {
            return m_ModelGraphSnapshots;
        }

        void CommitCurrentModelGraph(std::string_view commitMsg)
        {
            m_ModelGraphSnapshots.Commit(commitMsg);
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

        bool HasSelection() const
        {
            return ::HasSelection(GetModelGraph());
        }

        bool IsSelected(UID id) const
        {
            return GetModelGraph().IsSelected(id);
        }


        //
        // MESH LOADING STUFF
        //

        void PushMeshLoadRequests(UID attachmentPoint, std::vector<std::filesystem::path> paths)
        {
            m_MeshLoader.send(MeshLoadRequest{attachmentPoint, std::move(paths)});
        }

        void PushMeshLoadRequests(std::vector<std::filesystem::path> paths)
        {
            PushMeshLoadRequests(c_GroundID, std::move(paths));
        }

        void PushMeshLoadRequest(UID attachmentPoint, std::filesystem::path const& path)
        {
            PushMeshLoadRequests(attachmentPoint, std::vector<std::filesystem::path>{path});
        }

        void PushMeshLoadRequest(std::filesystem::path const& meshFilePath)
        {
            PushMeshLoadRequest(c_GroundID, meshFilePath);
        }

        // called when the mesh loader responds with a fully-loaded mesh
        void PopMeshLoader_OnOKResponse(MeshLoadOKResponse& ok)
        {
            if (ok.meshes.empty())
            {
                return;
            }

            // add each loaded mesh into the model graph
            ModelGraph& mg = UpdModelGraph();
            mg.DeSelectAll();

            for (LoadedMesh const& lm : ok.meshes)
            {
                SceneEl* el = mg.TryUpdElByID(ok.preferredAttachmentPoint);

                if (el)
                {
                    MeshEl& mesh = mg.AddEl<MeshEl>(ok.preferredAttachmentPoint, lm.meshData, lm.path);
                    mesh.SetXform(el->GetXform());
                    mg.Select(mesh);
                    mg.Select(*el);
                }
            }

            // commit
            {
                std::stringstream commitMsgSS;
                if (ok.meshes.empty())
                {
                    commitMsgSS << "loaded 0 meshes";
                }
                else if (ok.meshes.size() == 1)
                {
                    commitMsgSS << "loaded " << ok.meshes[0].path.filename();
                }
                else
                {
                    commitMsgSS << "loaded " << ok.meshes.size() << " meshes";
                }

                CommitCurrentModelGraph(std::move(commitMsgSS).str());
            }
        }

        // called when the mesh loader responds with a mesh loading error
        void PopMeshLoader_OnErrorResponse(MeshLoadErrorResponse& err)
        {
            osc::log::error("%s: error loading mesh file: %s", err.path.string().c_str(), err.error.c_str());
        }

        void PopMeshLoader()
        {
            for (auto maybeResponse = m_MeshLoader.poll(); maybeResponse.has_value(); maybeResponse = m_MeshLoader.poll())
            {
                MeshLoadResponse& meshLoaderResp = *maybeResponse;

                if (std::holds_alternative<MeshLoadOKResponse>(meshLoaderResp))
                {
                    PopMeshLoader_OnOKResponse(std::get<MeshLoadOKResponse>(meshLoaderResp));
                }
                else
                {
                    PopMeshLoader_OnErrorResponse(std::get<MeshLoadErrorResponse>(meshLoaderResp));
                }
            }
        }

        std::vector<std::filesystem::path> PromptUserForMeshFiles() const
        {
            return osc::PromptUserForFiles("obj,vtp,stl");
        }

        void PromptUserForMeshFilesAndPushThemOntoMeshLoader()
        {
            PushMeshLoadRequests(PromptUserForMeshFiles());
        }


        //
        // UI OVERLAY STUFF
        //

        glm::vec2 WorldPosToScreenPos(glm::vec3 const& worldPos) const
        {
            return GetCamera().projectOntoScreenRect(worldPos, Get3DSceneRect());
        }

        void DrawConnectionLineTriangleAtMidpoint(ImU32 color, glm::vec3 parent, glm::vec3 child) const
        {
            constexpr float triangleWidth = 6.0f * c_ConnectionLineWidth;
            constexpr float triangleWidthSquared = triangleWidth*triangleWidth;

            glm::vec2 parentScr = WorldPosToScreenPos(parent);
            glm::vec2 childScr = WorldPosToScreenPos(child);
            glm::vec2 child2ParentScr = parentScr - childScr;

            if (glm::dot(child2ParentScr, child2ParentScr) < triangleWidthSquared)
            {
                return;
            }

            glm::vec3 midpoint = osc::Midpoint(parent, child);
            glm::vec2 midpointScr = WorldPosToScreenPos(midpoint);
            glm::vec2 directionScr = glm::normalize(child2ParentScr);
            glm::vec2 directionNormalScr = {-directionScr.y, directionScr.x};

            glm::vec2 p1 = midpointScr + (triangleWidth/2.0f)*directionNormalScr;
            glm::vec2 p2 = midpointScr - (triangleWidth/2.0f)*directionNormalScr;
            glm::vec2 p3 = midpointScr + triangleWidth*directionScr;

            ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, color);
        }

        void DrawConnectionLine(ImU32 color, glm::vec3 const& parent, glm::vec3 const& child) const
        {
            // the line
            ImGui::GetWindowDrawList()->AddLine(WorldPosToScreenPos(parent), WorldPosToScreenPos(child), color, c_ConnectionLineWidth);

            // the triangle
            DrawConnectionLineTriangleAtMidpoint(color, parent, child);
        }

        void DrawConnectionLines(SceneEl const& el, ImU32 color, std::unordered_set<UID> const& excludedIDs) const
        {
            for (int i = 0, len = el.GetNumCrossReferences(); i < len; ++i)
            {
                UID refID = el.GetCrossReferenceConnecteeID(i);

                if (Contains(excludedIDs, refID))
                {
                    continue;
                }

                SceneEl const* other = GetModelGraph().TryGetElByID(refID);

                if (!other)
                {
                    continue;
                }

                glm::vec3 child = el.GetPos();
                glm::vec3 parent = other->GetPos();

                if (el.GetCrossReferenceDirection(i) == CrossrefDirection_ToChild) {
                    std::swap(parent, child);
                }

                DrawConnectionLine(color, parent, child);
            }
        }

        void DrawConnectionLines(SceneEl const& el, ImU32 color) const
        {
            DrawConnectionLines(el, color, std::unordered_set<UID>{});
        }

        void DrawConnectionLineToGround(SceneEl const& el, ImU32 color) const
        {
            if (el.GetID() == c_GroundID)
            {
                return;
            }

            DrawConnectionLine(color, glm::vec3{}, el.GetPos());
        }

        bool ShouldShowConnectionLines(SceneEl const& el) const
        {
            class Visitor final : public ConstSceneElVisitor {
            public:
                Visitor(SharedData const& shared) : m_Shared{shared} {}

                void operator()(GroundEl const&) final
                {
                    m_Result = false;
                }

                void operator()(MeshEl const&) final
                {
                    m_Result = m_Shared.IsShowingMeshConnectionLines();
                }

                void operator()(BodyEl const&) final
                {
                    m_Result = m_Shared.IsShowingBodyConnectionLines();
                }

                void operator()(JointEl const&) final
                {
                    m_Result = m_Shared.IsShowingJointConnectionLines();
                }

                void operator()(StationEl const&) final
                {
                    m_Result = m_Shared.IsShowingMeshConnectionLines();
                }

                bool result() const
                {
                    return m_Result;
                }

            private:
                SharedData const& m_Shared;
                bool m_Result = false;
            };

            Visitor v{*this};
            el.Accept(v);
            return v.result();
        }

        void DrawConnectionLines(ImVec4 colorVec, std::unordered_set<UID> const& excludedIDs) const
        {
            ModelGraph const& mg = GetModelGraph();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);

            for (SceneEl const& el : mg.iter())
            {
                UID id = el.GetID();

                if (Contains(excludedIDs, id))
                {
                    continue;
                }

                if (!ShouldShowConnectionLines(el))
                {
                    continue;
                }

                if (el.GetNumCrossReferences() > 0)
                {
                    DrawConnectionLines(el, color, excludedIDs);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    DrawConnectionLineToGround(el, color);
                }
            }
        }

        void DrawConnectionLines(ImVec4 colorVec) const
        {
            DrawConnectionLines(colorVec, {});
        }

        void DrawConnectionLines(Hover const& currentHover) const
        {
            ModelGraph const& mg = GetModelGraph();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(m_Colors.connectionLines);

            for (SceneEl const& el : mg.iter())
            {
                UID id = el.GetID();

                if (id != currentHover.ID && !IsCrossReferencing(el, currentHover.ID))
                {
                    continue;
                }

                if (!ShouldShowConnectionLines(el))
                {
                    continue;
                }

                if (el.GetNumCrossReferences() > 0)
                {
                    DrawConnectionLines(el, color);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    DrawConnectionLineToGround(el, color);
                }
            }
            //DrawConnectionLines(m_Colors.connectionLines);
        }


        //
        // RENDERING STUFF
        //

        void SetContentRegionAvailAsSceneRect()
        {
            Set3DSceneRect(osc::ContentRegionAvailScreenRect());
        }

        void DrawScene(nonstd::span<DrawableThing> drawables)
        {
            // setup rendering params
            osc::SceneRendererParams p;
            p.dimensions = osc::Dimensions(Get3DSceneRect());
            p.samples = osc::App::get().getMSXAASamplesRecommended();
            p.drawRims = true;
            p.drawFloor = false;
            p.nearClippingPlane = m_3DSceneCamera.znear;
            p.farClippingPlane = m_3DSceneCamera.zfar;
            p.viewMatrix = m_3DSceneCamera.getViewMtx();
            p.projectionMatrix = m_3DSceneCamera.getProjMtx(osc::AspectRatio(p.dimensions));
            p.viewPos = m_3DSceneCamera.getPos();
            p.lightDirection = osc::RecommendedLightDirection(m_3DSceneCamera);
            p.lightColor = {1.0f, 1.0f, 1.0f};
            p.ambientStrength = 0.35f;
            p.diffuseStrength = 0.65f;
            p.specularStrength = 0.4f;
            p.shininess = 32;
            p.backgroundColor = GetColorSceneBackground();

            std::vector<osc::SceneDecoration> decs;
            decs.reserve(drawables.size());
            for (DrawableThing const& dt : drawables)
            {
                decs.emplace_back(dt.mesh, dt.transform, dt.color, std::string{}, dt.flags, dt.maybeMaterial, dt.maybePropertyBlock);
            }

            // render
            m_SceneRenderer.draw(decs, p);

            // send texture to ImGui
            osc::DrawTextureAsImGuiImage(m_SceneRenderer.updRenderTexture(), m_SceneRenderer.getDimensions());

            // handle hittesting, etc.
            SetIsRenderHovered(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
        }

        bool IsRenderHovered() const
        {
            return m_IsRenderHovered;
        }

        void SetIsRenderHovered(bool newIsHovered)
        {
            m_IsRenderHovered = newIsHovered;
        }

        Rect const& Get3DSceneRect() const
        {
            return m_3DSceneRect;
        }

        void Set3DSceneRect(Rect const& newRect)
        {
            m_3DSceneRect = newRect;
        }

        glm::vec2 Get3DSceneDims() const
        {
            return Dimensions(m_3DSceneRect);
        }

        PolarPerspectiveCamera const& GetCamera() const
        {
            return m_3DSceneCamera;
        }

        PolarPerspectiveCamera& UpdCamera()
        {
            return m_3DSceneCamera;
        }

        void FocusCameraOn(glm::vec3 const& focusPoint)
        {
            m_3DSceneCamera.focusPoint = -focusPoint;
        }

        osc::RenderTexture& UpdSceneTex()
        {
            return m_SceneRenderer.updRenderTexture();
        }

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

        nonstd::span<char const* const> GetColorLabels() const
        {
            return c_ColorNames;
        }

        glm::vec4 const& GetColorSceneBackground() const
        {
            return m_Colors.sceneBackground;
        }

        glm::vec4 const& GetColorMesh() const
        {
            return m_Colors.meshes;
        }

        void SetColorMesh(glm::vec4 const& newColor)
        {
            m_Colors.meshes = newColor;
        }

        glm::vec4 const& GetColorGround() const
        {
            return m_Colors.ground;
        }

        glm::vec4 const& GetColorStation() const
        {
            return m_Colors.stations;
        }

        glm::vec4 const& GetColorConnectionLine() const
        {
            return m_Colors.connectionLines;
        }

        void SetColorConnectionLine(glm::vec4 const& newColor)
        {
            m_Colors.connectionLines = newColor;
        }

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

        nonstd::span<char const* const> GetVisibilityFlagLabels() const
        {
            return c_VisibilityFlagNames;
        }

        bool IsShowingMeshes() const
        {
            return m_VisibilityFlags.meshes;
        }

        void SetIsShowingMeshes(bool newIsShowing)
        {
            m_VisibilityFlags.meshes = newIsShowing;
        }

        bool IsShowingBodies() const
        {
            return m_VisibilityFlags.bodies;
        }

        void SetIsShowingBodies(bool newIsShowing)
        {
            m_VisibilityFlags.bodies = newIsShowing;
        }

        bool IsShowingJointCenters() const
        {
            return m_VisibilityFlags.joints;
        }

        void SetIsShowingJointCenters(bool newIsShowing)
        {
            m_VisibilityFlags.joints = newIsShowing;
        }

        bool IsShowingGround() const
        {
            return m_VisibilityFlags.ground;
        }

        void SetIsShowingGround(bool newIsShowing)
        {
            m_VisibilityFlags.ground = newIsShowing;
        }

        bool IsShowingFloor() const
        {
            return m_VisibilityFlags.floor;
        }

        void SetIsShowingFloor(bool newIsShowing)
        {
            m_VisibilityFlags.floor = newIsShowing;
        }

        bool IsShowingStations() const
        {
            return m_VisibilityFlags.stations;
        }

        void SetIsShowingStations(bool v)
        {
            m_VisibilityFlags.stations = v;
        }

        bool IsShowingJointConnectionLines() const
        {
            return m_VisibilityFlags.jointConnectionLines;
        }

        void SetIsShowingJointConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.jointConnectionLines = newIsShowing;
        }

        bool IsShowingMeshConnectionLines() const
        {
            return m_VisibilityFlags.meshConnectionLines;
        }

        void SetIsShowingMeshConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.meshConnectionLines = newIsShowing;
        }

        bool IsShowingBodyConnectionLines() const
        {
            return m_VisibilityFlags.bodyToGroundConnectionLines;
        }

        void SetIsShowingBodyConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.bodyToGroundConnectionLines = newIsShowing;
        }

        bool IsShowingStationConnectionLines() const
        {
            return m_VisibilityFlags.stationConnectionLines;
        }

        void SetIsShowingStationConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.stationConnectionLines = newIsShowing;
        }

        Transform GetFloorTransform() const
        {
            Transform t;
            t.rotation = glm::angleAxis(fpi2, glm::vec3{-1.0f, 0.0f, 0.0f});
            t.scale = {m_SceneScaleFactor * 100.0f, m_SceneScaleFactor * 100.0f, 1.0f};
            return t;
        }

        DrawableThing GenerateFloorDrawable() const
        {
            Transform t = GetFloorTransform();
            t.scale *= 0.5f;

            osc::Material material{osc::App::singleton<osc::ShaderCache>()->load(osc::App::resource("shaders/SolidColor.vert"), osc::App::resource("shaders/SolidColor.frag"))};
            material.setVec4("uColor", m_Colors.gridLines);

            DrawableThing dt;
            dt.id = c_EmptyID;
            dt.groupId = c_EmptyID;
            dt.mesh = osc::App::singleton<osc::MeshCache>()->get100x100GridMesh();
            dt.transform = t;
            dt.color = m_Colors.gridLines;
            dt.flags = osc::SceneDecorationFlags_None;
            dt.maybeMaterial = std::move(material);
            return dt;
        }

        float GetSphereRadius() const
        {
            return 0.02f * m_SceneScaleFactor;
        }

        Sphere SphereAtTranslation(glm::vec3 const& translation) const
        {
            return Sphere{translation, GetSphereRadius()};
        }

        void AppendAsFrame(
            UID logicalID,
            UID groupID,
            Transform const& xform,
            std::vector<DrawableThing>& appendOut,
            float alpha = 1.0f,
            osc::SceneDecorationFlags flags = osc::SceneDecorationFlags_None,
            glm::vec3 legLen = {1.0f, 1.0f, 1.0f},
            glm::vec3 coreColor = {1.0f, 1.0f, 1.0f}) const
        {
            float const coreRadius = GetSphereRadius();
            float const legThickness = 0.5f * coreRadius;

            // this is how much the cylinder has to be "pulled in" to the core to hide the edges
            float const cylinderPullback = coreRadius * std::sin((osc::fpi * legThickness) / coreRadius);

            // emit origin sphere
            {
                Transform t;
                t.scale *= coreRadius;
                t.rotation = xform.rotation;
                t.position = xform.position;

                DrawableThing& sphere = appendOut.emplace_back();
                sphere.id = logicalID;
                sphere.groupId = groupID;
                sphere.mesh = m_SphereMesh;
                sphere.transform = t;
                sphere.color = {coreColor, alpha};
                sphere.flags = flags;
            }

            // emit "legs"
            for (int i = 0; i < 3; ++i)
            {
                // cylinder meshes are -1.0f to 1.0f in Y, so create a transform that maps the
                // mesh onto the legs, which are:
                //
                // - 4.0f * leglen[leg] * radius long
                // - 0.5f * radius thick

                glm::vec3 const meshDirection = {0.0f, 1.0f, 0.0f};
                glm::vec3 cylinderDirection = {};
                cylinderDirection[i] = 1.0f;

                float const actualLegLen = 4.0f * legLen[i] * coreRadius;

                Transform t;
                t.scale.x = legThickness;
                t.scale.y = 0.5f * actualLegLen;  // cylinder is 2 units high
                t.scale.z = legThickness;
                t.rotation = glm::normalize(xform.rotation * glm::rotation(meshDirection, cylinderDirection));
                t.position = xform.position + (t.rotation * (((GetSphereRadius() + (0.5f * actualLegLen)) - cylinderPullback) * meshDirection));

                glm::vec4 color = {0.0f, 0.0f, 0.0f, alpha};
                color[i] = 1.0f;

                DrawableThing& se = appendOut.emplace_back();
                se.id = logicalID;
                se.groupId = groupID;
                se.mesh = m_CylinderMesh;
                se.transform = t;
                se.color = color;
                se.flags = flags;
            }
        }

        void AppendAsCubeThing(UID logicalID,
            UID groupID,
            Transform const& xform,
            std::vector<DrawableThing>& appendOut) const
        {
            float const halfWidth = 1.5f * GetSphereRadius();

            // core
            {
                Transform scaled{xform};
                scaled.scale *= halfWidth;

                DrawableThing& originCube = appendOut.emplace_back();
                originCube.id = logicalID;
                originCube.groupId = groupID;
                originCube.mesh = osc::App::singleton<osc::MeshCache>()->getBrickMesh();
                originCube.transform = scaled;
                originCube.color = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
                originCube.flags = osc::SceneDecorationFlags_None;
            }

            // legs
            for (int i = 0; i < 3; ++i)
            {
                // cone mesh has a source height of 2, stretches from -1 to +1 in Y
                float const coneHeight = 0.75f * halfWidth;

                glm::vec3 const meshDirection = {0.0f, 1.0f, 0.0f};
                glm::vec3 coneDirection = {};
                coneDirection[i] = 1.0f;

                Transform t;
                t.scale.x = 0.5f * halfWidth;
                t.scale.y = 0.5f * coneHeight;
                t.scale.z = 0.5f * halfWidth;
                t.rotation = xform.rotation * glm::rotation(meshDirection, coneDirection);
                t.position = xform.position + (t.rotation * ((halfWidth + (0.5f * coneHeight)) * meshDirection));

                glm::vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[i] = 1.0f;

                DrawableThing& legCube = appendOut.emplace_back();
                legCube.id = logicalID;
                legCube.groupId = groupID;
                legCube.mesh = osc::App::singleton<osc::MeshCache>()->getConeMesh();
                legCube.transform = t;
                legCube.color = color;
                legCube.flags = osc::SceneDecorationFlags_None;
            }
        }


        //
        // HOVERTEST/INTERACTIVITY
        //

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
            return c_InteractivityFlagNames;
        }

        bool IsMeshesInteractable() const
        {
            return m_InteractivityFlags.meshes;
        }

        void SetIsMeshesInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.meshes = newIsInteractable;
        }

        bool IsBodiesInteractable() const
        {
            return m_InteractivityFlags.bodies;
        }

        void SetIsBodiesInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.bodies = newIsInteractable;
        }

        bool IsJointCentersInteractable() const
        {
            return m_InteractivityFlags.joints;
        }

        void SetIsJointCentersInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.joints = newIsInteractable;
        }

        bool IsGroundInteractable() const
        {
            return m_InteractivityFlags.ground;
        }

        void SetIsGroundInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.ground = newIsInteractable;
        }

        bool IsStationsInteractable() const
        {
            return m_InteractivityFlags.stations;
        }

        void SetIsStationsInteractable(bool v)
        {
            m_InteractivityFlags.stations = v;
        }

        float GetSceneScaleFactor() const
        {
            return m_SceneScaleFactor;
        }

        void SetSceneScaleFactor(float newScaleFactor)
        {
            m_SceneScaleFactor = newScaleFactor;
        }

        Hover Hovertest(std::vector<DrawableThing> const& drawables) const
        {
            Rect const sceneRect = Get3DSceneRect();
            glm::vec2 const mousePos = ImGui::GetMousePos();

            if (!IsPointInRect(sceneRect, mousePos))
            {
                // mouse isn't over the scene render
                return Hover{};
            }

            glm::vec2 const sceneDims = Dimensions(sceneRect);
            glm::vec2 const relMousePos = mousePos - sceneRect.p1;

            Line const ray = GetCamera().unprojectTopLeftPosToWorldRay(relMousePos, sceneDims);
            bool const hittestMeshes = IsMeshesInteractable();
            bool const hittestBodies = IsBodiesInteractable();
            bool const hittestJointCenters = IsJointCentersInteractable();
            bool const hittestGround = IsGroundInteractable();
            bool const hittestStations = IsStationsInteractable();

            UID closestID = c_EmptyID;
            float closestDist = std::numeric_limits<float>::max();
            for (DrawableThing const& drawable : drawables)
            {
                if (drawable.id == c_EmptyID)
                {
                    continue;  // no hittest data
                }

                if (drawable.groupId == c_BodyGroupID && !hittestBodies)
                {
                    continue;
                }

                if (drawable.groupId == c_MeshGroupID && !hittestMeshes)
                {
                    continue;
                }

                if (drawable.groupId == c_JointGroupID && !hittestJointCenters)
                {
                    continue;
                }

                if (drawable.groupId == c_GroundGroupID && !hittestGround)
                {
                    continue;
                }

                if (drawable.groupId == c_StationGroupID && !hittestStations)
                {
                    continue;
                }

                std::optional<osc::RayCollision> const rc = osc::GetClosestWorldspaceRayCollision(
                    drawable.mesh,
                    drawable.transform,
                    ray
                );

                if (rc && rc->distance < closestDist)
                {
                    closestID = drawable.id;
                    closestDist = rc->distance;
                }
            }

            glm::vec3 const hitPos = closestID != c_EmptyID ? ray.origin + closestDist*ray.dir : glm::vec3{};

            return Hover{closestID, hitPos};
        }

        //
        // SCENE ELEMENT STUFF (specific methods for specific scene element types)
        //

        void UnassignMesh(MeshEl const& me)
        {
            UpdModelGraph().UpdElByID<MeshEl>(me.GetID()).getParentID() = c_GroundID;

            std::stringstream ss;
            ss << "unassigned '" << me.GetLabel() << "' back to ground";
            CommitCurrentModelGraph(std::move(ss).str());
        }

        DrawableThing GenerateMeshElDrawable(MeshEl const& meshEl) const
        {
            DrawableThing rv;
            rv.id = meshEl.GetID();
            rv.groupId = c_MeshGroupID;
            rv.mesh = meshEl.getMeshData();
            rv.transform = meshEl.GetXform();
            rv.color = meshEl.getParentID() == c_GroundID || meshEl.getParentID() == c_EmptyID ? RedifyColor(GetColorMesh()) : GetColorMesh();
            rv.flags = osc::SceneDecorationFlags_None;
            return rv;
        }

        DrawableThing GenerateBodyElSphere(BodyEl const& bodyEl, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = bodyEl.GetID();
            rv.groupId = c_BodyGroupID;
            rv.mesh = m_SphereMesh;
            rv.transform = SphereMeshToSceneSphereTransform(SphereAtTranslation(bodyEl.GetXform().position));
            rv.color = color;
            rv.flags = osc::SceneDecorationFlags_None;
            return rv;
        }

        DrawableThing GenerateGroundSphere(glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = c_GroundID;
            rv.groupId = c_GroundGroupID;
            rv.mesh = m_SphereMesh;
            rv.transform = SphereMeshToSceneSphereTransform(SphereAtTranslation({0.0f, 0.0f, 0.0f}));
            rv.color = color;
            rv.flags = osc::SceneDecorationFlags_None;
            return rv;
        }

        DrawableThing GenerateStationSphere(StationEl const& el, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = el.GetID();
            rv.groupId = c_StationGroupID;
            rv.mesh = m_SphereMesh;
            rv.transform = SphereMeshToSceneSphereTransform(SphereAtTranslation(el.GetPos()));
            rv.color = color;
            rv.flags = osc::SceneDecorationFlags_None;
            return rv;
        }

        void AppendBodyElAsCubeThing(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            AppendAsCubeThing(bodyEl.GetID(), c_BodyGroupID, bodyEl.GetXform(), appendOut);
        }

        void AppendBodyElAsFrame(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            AppendAsFrame(bodyEl.GetID(), c_BodyGroupID, bodyEl.GetXform(), appendOut);
        }

        void AppendDrawables(SceneEl const& e, std::vector<DrawableThing>& appendOut) const
        {
            class Visitor final : public ConstSceneElVisitor {
            public:
                Visitor(
                    SharedData const& data,
                    std::vector<DrawableThing>& out) :

                    m_Data{data},
                    m_Out{out}
                {
                }

                void operator()(GroundEl const&) final
                {
                    if (!m_Data.IsShowingGround())
                    {
                        return;
                    }

                    m_Out.push_back(m_Data.GenerateGroundSphere(m_Data.GetColorGround()));
                }
                void operator()(MeshEl const& el) final
                {
                    if (!m_Data.IsShowingMeshes())
                    {
                        return;
                    }

                    m_Out.push_back(m_Data.GenerateMeshElDrawable(el));
                }
                void operator()(BodyEl const& el) final
                {
                    if (!m_Data.IsShowingBodies())
                    {
                        return;
                    }

                    m_Data.AppendBodyElAsCubeThing(el, m_Out);
                }
                void operator()(JointEl const& el) final
                {
                    if (!m_Data.IsShowingJointCenters()) {
                        return;
                    }

                    m_Data.AppendAsFrame(el.GetID(),
                        c_JointGroupID,
                        el.GetXform(),
                        m_Out,
                        1.0f,
                        osc::SceneDecorationFlags_None,
                        GetJointAxisLengths(el));
                }
                void operator()(StationEl const& el) final
                {
                    if (!m_Data.IsShowingStations())
                    {
                        return;
                    }

                    m_Out.push_back(m_Data.GenerateStationSphere(el, m_Data.GetColorStation()));
                }
            private:
                SharedData const& m_Data;
                std::vector<DrawableThing>& m_Out;
            };

            Visitor visitor{*this, appendOut};
            e.Accept(visitor);
        }


        //
        // TOP-LEVEL STUFF
        //

        bool onEvent(SDL_Event const& e)
        {
            // if the user drags + drops a file into the window, assume it's a meshfile
            // and start loading it
            if (e.type == SDL_DROPFILE && e.drop.file != nullptr)
            {
                m_DroppedFiles.emplace_back(e.drop.file);
                return true;
            }

            return false;
        }

        void tick(float)
        {
            // push any user-drag-dropped files as one batch
            if (!m_DroppedFiles.empty())
            {
                std::vector<std::filesystem::path> buf;
                std::swap(buf, m_DroppedFiles);
                PushMeshLoadRequests(std::move(buf));
            }

            // pop any background-loaded meshes
            PopMeshLoader();

            m_ModelGraphSnapshots.GarbageCollect();
        }

    private:
        // in-memory model graph (snapshots) that the user is manipulating
        CommittableModelGraph m_ModelGraphSnapshots;

        // (maybe) the filesystem location where the model graph should be saved
        std::filesystem::path m_MaybeModelGraphExportLocation;

        // (maybe) the UID of the model graph when it was last successfully saved to disk (used for dirty checking)
        UID m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.GetCheckoutID();

        // a batch of files that the user drag-dropped into the UI in the last frame
        std::vector<std::filesystem::path> m_DroppedFiles;

        // loads meshes in a background thread
        MeshLoader m_MeshLoader;

        // sphere mesh used by various scene elements
        Mesh m_SphereMesh = osc::GenUntexturedUVSphere(12, 12);

        // cylinder mesh used by various scene elements
        Mesh m_CylinderMesh = osc::GenUntexturedSimbodyCylinder(16);

        // main 3D scene camera
        PolarPerspectiveCamera m_3DSceneCamera = CreateDefaultCamera();

        // screenspace rect where the 3D scene is currently being drawn to
        osc::Rect m_3DSceneRect = {};

        // renderer that draws the scene
        osc::SceneRenderer m_SceneRenderer{osc::App::config(), *osc::App::singleton<osc::MeshCache>(), *osc::App::singleton<osc::ShaderCache>()};

        // COLORS
        //
        // these are runtime-editable color values for things in the scene
        struct {
            glm::vec4 ground{196.0f/255.0f, 196.0f/255.0f, 196.0/255.0f, 1.0f};
            glm::vec4 meshes{1.0f, 1.0f, 1.0f, 1.0f};
            glm::vec4 stations{196.0f/255.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 connectionLines{0.6f, 0.6f, 0.6f, 1.0f};
            glm::vec4 sceneBackground{96.0f/255.0f, 96.0f/255.0f, 96.0f/255.0f, 1.0f};
            glm::vec4 gridLines{112.0f/255.0f, 112.0f/255.0f, 112.0f/255.0f, 1.0f};
        } m_Colors;
        static auto constexpr c_ColorNames = osc::MakeSizedArray<char const*, sizeof(decltype(m_Colors))/sizeof(glm::vec4)>(
            "ground",
            "meshes",
            "stations",
            "connection lines",
            "scene background",
            "grid lines"
        );

        // VISIBILITY
        //
        // these are runtime-editable visibility flags for things in the scene
        struct {
            bool ground = true;
            bool meshes = true;
            bool bodies = true;
            bool joints = true;
            bool stations = true;
            bool jointConnectionLines = true;
            bool meshConnectionLines = true;
            bool bodyToGroundConnectionLines = true;
            bool stationConnectionLines = true;
            bool floor = true;
        } m_VisibilityFlags;
        static auto constexpr c_VisibilityFlagNames = osc::MakeSizedArray<char const*, sizeof(decltype(m_VisibilityFlags))/sizeof(bool)>(
            "ground",
            "meshes",
            "bodies",
            "joints",
            "stations",
            "joint connection lines",
            "mesh connection lines",
            "body-to-ground connection lines",
            "station connection lines",
            "grid lines"
        );

        // LOCKING
        //
        // these are runtime-editable flags that dictate what gets hit-tested
        struct {
            bool ground = true;
            bool meshes = true;
            bool bodies = true;
            bool joints = true;
            bool stations = true;
        } m_InteractivityFlags;
        static auto constexpr c_InteractivityFlagNames = osc::MakeSizedArray<char const*, sizeof(decltype(m_InteractivityFlags))/sizeof(bool)>(
            "ground",
            "meshes",
            "bodies",
            "joints",
            "stations"
        );

    public:
        // WINDOWS
        //
        // these are runtime-editable flags that dictate which panels are open
        static inline size_t constexpr c_NumPanelStates = 4;
        std::array<bool, c_NumPanelStates> m_PanelStates{false, true, false, false};
        static auto constexpr c_OpenedPanelNames = osc::MakeSizedArray<char const*, c_NumPanelStates>(
            "History",
            "Navigator",
            "Log",
            "Performance"
        );
        enum PanelIndex_ {
            PanelIndex_History = 0,
            PanelIndex_Navigator,
            PanelIndex_Log,
            PanelIndex_Performance,
            PanelIndex_COUNT,
        };
        osc::LogViewer m_Logviewer;
        osc::PerfPanel m_PerfPanel{"Performance"};

        std::optional<osc::SaveChangesPopup> m_MaybeSaveChangesPopup;
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

        // true if the implementation wants the host to close the mesh importer UI
        bool m_CloseRequested = false;

        // true if the implementation wants the host to open a new mesh importer
        bool m_NewTabRequested = false;
    };
}

// select 2 mesh points layer
namespace
{

    // runtime options for "Select two mesh points" UI layer
    struct Select2MeshPointsOptions final {

        // a function that is called when the implementation detects two points have
        // been clicked
        //
        // the function should return `true` if the points are accepted
        std::function<bool(glm::vec3, glm::vec3)> onTwoPointsChosen = [](glm::vec3, glm::vec3)
        {
            return true;
        };

        std::string header = "choose first (left-click) and second (right click) mesh positions (ESC to cancel)";
    };

    // UI layer that lets the user select two points on a mesh with left-click and
    // right-click
    class Select2MeshPointsLayer final : public Layer {
    public:
        Select2MeshPointsLayer(
            LayerHost& parent,
            std::shared_ptr<SharedData> shared,
            Select2MeshPointsOptions options) :

            Layer{parent},
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:

        bool IsBothPointsSelected() const
        {
            return m_MaybeFirstLocation && m_MaybeSecondLocation;
        }

        bool IsAnyPointSelected() const
        {
            return m_MaybeFirstLocation || m_MaybeSecondLocation;
        }

        // handle the transition that may occur after the user clicks two points
        void HandlePossibleTransitionToNextStep()
        {
            if (!IsBothPointsSelected())
            {
                return;  // user hasn't selected two points yet
            }

            bool pointsAccepted = m_Options.onTwoPointsChosen(*m_MaybeFirstLocation, *m_MaybeSecondLocation);

            if (pointsAccepted)
            {
                requestPop();
            }
            else
            {
                // points were rejected, so reset them
                m_MaybeFirstLocation.reset();
                m_MaybeSecondLocation.reset();
            }
        }

        // handle any side-effects of the user interacting with whatever they are
        // hovered over
        void HandleHovertestSideEffects()
        {
            if (!m_MaybeCurrentHover)
            {
                return;  // nothing hovered
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                // LEFT CLICK: set first mouse location
                m_MaybeFirstLocation = m_MaybeCurrentHover.Pos;
                HandlePossibleTransitionToNextStep();
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                // RIGHT CLICK: set second mouse location
                m_MaybeSecondLocation = m_MaybeCurrentHover.Pos;
                HandlePossibleTransitionToNextStep();
            }
        }

        // generate 3D drawable geometry for this particular layer
        std::vector<DrawableThing>& GenerateDrawables()
        {
            m_DrawablesBuffer.clear();

            ModelGraph const& mg = m_Shared->GetModelGraph();

            for (MeshEl const& meshEl : mg.iter<MeshEl>())
            {
                m_DrawablesBuffer.emplace_back(m_Shared->GenerateMeshElDrawable(meshEl));
            }

            m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());

            return m_DrawablesBuffer;
        }

        // draw tooltip that pops up when user is moused over a mesh
        void DrawHoverTooltip()
        {
            if (!m_MaybeCurrentHover)
            {
                return;
            }

            ImGui::BeginTooltip();
            ImGui::Text("%s", PosString(m_MaybeCurrentHover.Pos).c_str());
            ImGui::TextDisabled("(left-click to assign as first point, right-click to assign as second point)");
            ImGui::EndTooltip();
        }

        // draw 2D overlay over the render, things like connection lines, dots, etc.
        void DrawOverlay()
        {
            if (!IsAnyPointSelected())
            {
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

        // draw 2D "choose something" text at the top of the render
        void DrawHeaderText() const
        {
            if (m_Options.header.empty())
            {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
            glm::vec2 padding{10.0f, 10.0f};
            glm::vec2 pos = m_Shared->Get3DSceneRect().p1 + padding;
            ImGui::GetWindowDrawList()->AddText(pos, color, m_Options.header.c_str());
        }

        // draw a user-clickable button for cancelling out of this choosing state
        void DrawCancelButton()
        {
            char const* const text = ICON_FA_ARROW_LEFT " Cancel (ESC)";

            glm::vec2 framePad = {10.0f, 10.0f};
            glm::vec2 margin = {25.0f, 35.0f};
            Rect sceneRect = m_Shared->Get3DSceneRect();
            glm::vec2 textDims = ImGui::CalcTextSize(text);

            ImGui::SetCursorScreenPos(sceneRect.p2 - textDims - framePad - margin);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePad);
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_GREYED_RGBA);
            if (ImGui::Button(text))
            {
                requestPop();
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        bool implOnEvent(SDL_Event const& e) final
        {
            return m_Shared->onEvent(e);
        }

        void implTick(float dt) final
        {
            m_Shared->tick(dt);

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                // ESC: user cancelled out
                requestPop();
            }

            bool isRenderHovered = m_Shared->IsRenderHovered();

            if (isRenderHovered)
            {
                UpdatePolarCameraFromImGuiMouseInputs(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }
        }

        void implDraw() final
        {
            m_Shared->SetContentRegionAvailAsSceneRect();
            std::vector<DrawableThing>& drawables = GenerateDrawables();
            m_MaybeCurrentHover = m_Shared->Hovertest(drawables);
            HandleHovertestSideEffects();

            m_Shared->DrawScene(drawables);
            DrawOverlay();
            DrawHoverTooltip();
            DrawHeaderText();
            DrawCancelButton();
        }

        // data that's shared between other UI states
        std::shared_ptr<SharedData> m_Shared;

        // options for this state
        Select2MeshPointsOptions m_Options;

        // (maybe) user mouse hover
        Hover m_MaybeCurrentHover;

        // (maybe) first mesh location
        std::optional<glm::vec3> m_MaybeFirstLocation;

        // (maybe) second mesh location
        std::optional<glm::vec3> m_MaybeSecondLocation;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;
    };
}

// choose specific element layer
namespace
{
    // options for when the UI transitions into "choose something" mode
    struct ChooseElLayerOptions final {

        // types of elements the user can choose in this screen
        bool canChooseBodies = true;
        bool canChooseGround = true;
        bool canChooseMeshes = true;
        bool canChooseJoints = true;
        bool canChooseStations = false;

        // (maybe) elements the assignment is ultimately assigning
        std::unordered_set<UID> maybeElsAttachingTo;

        // false implies the user is attaching "away from" what they select (used for drawing arrows)
        bool isAttachingTowardEl = true;

        // (maybe) elements that are being replaced by the user's choice
        std::unordered_set<UID> maybeElsBeingReplacedByChoice;

        // the number of elements the user must click before OnUserChoice is called
        int numElementsUserMustChoose = 1;

        // function that returns true if the "caller" is happy with the user's choice
        std::function<bool(nonstd::span<UID>)> onUserChoice = [](nonstd::span<UID>)
        {
            return true;
        };

        // user-facing header text
        std::string header = "choose something";
    };

    // "choose `n` things" UI layer
    //
    // this is what's drawn when the user's being prompted to choose scene elements
    class ChooseElLayer final : public Layer {
    public:
        ChooseElLayer(
            LayerHost& parent,
            std::shared_ptr<SharedData> shared,
            ChooseElLayerOptions options) :

            Layer{parent},
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:
        // returns true if the user's mouse is hovering over the given scene element
        bool IsHovered(SceneEl const& el) const
        {
            return el.GetID() == m_MaybeHover.ID;
        }

        // returns true if the user has already selected the given scene element
        bool IsSelected(SceneEl const& el) const
        {
            return Contains(m_SelectedEls, el.GetID());
        }

        // returns true if the user can (de)select the given element
        bool IsSelectable(SceneEl const& el) const
        {
            if (Contains(m_Options.maybeElsAttachingTo, el.GetID()))
            {
                return false;
            }

            struct Visitor final : public ConstSceneElVisitor {
            public:
                Visitor(ChooseElLayerOptions const& opts) : m_Opts{opts}
                {
                }

                bool result() const
                {
                    return m_Result;
                }

                void operator()(GroundEl const&) final
                {
                    m_Result = m_Opts.canChooseGround;
                }

                void operator()(MeshEl const&) final
                {
                    m_Result = m_Opts.canChooseMeshes;
                }

                void operator()(BodyEl const&) final
                {
                    m_Result = m_Opts.canChooseBodies;
                }

                void operator()(JointEl const&) final
                {
                    m_Result = m_Opts.canChooseJoints;
                }

                void operator()(StationEl const&) final
                {
                    m_Result = m_Opts.canChooseStations;
                }

            private:
                bool m_Result = false;
                ChooseElLayerOptions const& m_Opts;
            };

            Visitor v{m_Options};
            el.Accept(v);
            return v.result();
        }

        void Select(SceneEl const& el)
        {
            if (!IsSelectable(el))
            {
                return;
            }

            if (IsSelected(el))
            {
                return;
            }

            m_SelectedEls.push_back(el.GetID());
        }

        void DeSelect(SceneEl const& el)
        {
            if (!IsSelectable(el))
            {
                return;
            }

            RemoveErase(m_SelectedEls, [elID = el.GetID()](UID id) { return id == elID; } );
        }

        void TryToggleSelectionStateOf(SceneEl const& el)
        {
            IsSelected(el) ? DeSelect(el) : Select(el);
        }

        void TryToggleSelectionStateOf(UID id)
        {
            SceneEl const* el = m_Shared->GetModelGraph().TryGetElByID(id);

            if (el)
            {
                TryToggleSelectionStateOf(*el);
            }
        }

        osc::SceneDecorationFlags ComputeFlags(SceneEl const& el) const
        {
            if (IsSelected(el))
            {
                return osc::SceneDecorationFlags_IsSelected;
            }
            else if (IsHovered(el))
            {
                return osc::SceneDecorationFlags_IsHovered;
            }
            else
            {
                return osc::SceneDecorationFlags_None;
            }
        }

        // returns a list of 3D drawable scene objects for this layer
        std::vector<DrawableThing>& GenerateDrawables()
        {
            m_DrawablesBuffer.clear();

            ModelGraph const& mg = m_Shared->GetModelGraph();

            float fadedAlpha = 0.2f;
            float animScale = EaseOutElastic(m_AnimationFraction);

            for (SceneEl const& el : mg.iter())
            {
                size_t start = m_DrawablesBuffer.size();
                m_Shared->AppendDrawables(el, m_DrawablesBuffer);
                size_t end = m_DrawablesBuffer.size();

                bool isSelectable = IsSelectable(el);
                osc::SceneDecorationFlags flags = ComputeFlags(el);

                for (size_t i = start; i < end; ++i)
                {
                    DrawableThing& d = m_DrawablesBuffer[i];
                    d.flags = flags;

                    if (!isSelectable)
                    {
                        d.color.a = fadedAlpha;
                        d.id = c_EmptyID;
                        d.groupId = c_EmptyID;
                    }
                    else
                    {
                        d.transform.scale *= animScale;
                    }
                }
            }

            // floor
            m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());

            return m_DrawablesBuffer;
        }

        void HandlePossibleCompletion()
        {
            if (static_cast<int>(m_SelectedEls.size()) < m_Options.numElementsUserMustChoose)
            {
                return;  // user hasn't selected enough stuff yet
            }

            if (m_Options.onUserChoice(m_SelectedEls))
            {
                requestPop();
            }
            else
            {
                // choice was rejected?
            }
        }

        // handle any side-effects from the user's mouse hover
        void HandleHovertestSideEffects()
        {
            if (!m_MaybeHover)
            {
                return;
            }

            DrawHoverTooltip();

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                TryToggleSelectionStateOf(m_MaybeHover.ID);
                HandlePossibleCompletion();
            }
        }

        // draw 2D tooltip that pops up when user is hovered over something in the scene
        void DrawHoverTooltip() const
        {
            if (!m_MaybeHover)
            {
                return;
            }

            SceneEl const* se = m_Shared->GetModelGraph().TryGetElByID(m_MaybeHover.ID);

            if (se)
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(se->GetLabel().c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("(%s, click to choose)", se->GetClass().GetNameCStr());
                ImGui::EndTooltip();
            }
        }

        // draw 2D connection overlay lines that show what's connected to what in the graph
        //
        // depends on layer options
        void DrawConnectionLines() const
        {
            if (!m_MaybeHover)
            {
                // user isn't hovering anything, so just draw all existing connection
                // lines, but faintly
                m_Shared->DrawConnectionLines(FaintifyColor(m_Shared->GetColorConnectionLine()));
                return;
            }

            // else: user is hovering *something*

            // draw all other connection lines but exclude the thing being assigned (if any)
            m_Shared->DrawConnectionLines(FaintifyColor(m_Shared->GetColorConnectionLine()), m_Options.maybeElsBeingReplacedByChoice);

            // draw strong connection line between the things being attached to and the hover
            for (UID elAttachingTo : m_Options.maybeElsAttachingTo)
            {
                glm::vec3 parentPos = GetPosition(m_Shared->GetModelGraph(), elAttachingTo);
                glm::vec3 childPos = GetPosition(m_Shared->GetModelGraph(), m_MaybeHover.ID);

                if (!m_Options.isAttachingTowardEl)
                {
                    std::swap(parentPos, childPos);
                }

                ImU32 strongColorU2 = ImGui::ColorConvertFloat4ToU32(m_Shared->GetColorConnectionLine());

                m_Shared->DrawConnectionLine(strongColorU2, parentPos, childPos);
            }
        }

        // draw 2D header text in top-left corner of the screen
        void DrawHeaderText() const
        {
            if (m_Options.header.empty())
            {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
            glm::vec2 padding = glm::vec2{10.0f, 10.0f};
            glm::vec2 pos = m_Shared->Get3DSceneRect().p1 + padding;
            ImGui::GetWindowDrawList()->AddText(pos, color, m_Options.header.c_str());
        }

        // draw a user-clickable button for cancelling out of this choosing state
        void DrawCancelButton()
        {
            char const* const text = ICON_FA_ARROW_LEFT " Cancel (ESC)";

            glm::vec2 framePad = {10.0f, 10.0f};
            glm::vec2 margin = {25.0f, 35.0f};
            Rect sceneRect = m_Shared->Get3DSceneRect();
            glm::vec2 textDims = ImGui::CalcTextSize(text);

            ImGui::SetCursorScreenPos(sceneRect.p2 - textDims - framePad - margin);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePad);
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_GREYED_RGBA);
            if (ImGui::Button(text))
            {
                requestPop();
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        bool implOnEvent(SDL_Event const& e) final
        {
            return m_Shared->onEvent(e);
        }

        void implTick(float dt) final
        {
            m_Shared->tick(dt);

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                // ESC: user cancelled out
                requestPop();
            }

            bool isRenderHovered = m_Shared->IsRenderHovered();

            if (isRenderHovered)
            {
                UpdatePolarCameraFromImGuiMouseInputs(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }

            if (m_AnimationFraction < 1.0f)
            {
                m_AnimationFraction = std::clamp(m_AnimationFraction + 0.5f*dt, 0.0f, 1.0f);
                osc::App::upd().requestRedraw();
            }
        }

        void implDraw() final
        {
            m_Shared->SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing>& drawables = GenerateDrawables();

            m_MaybeHover = m_Shared->Hovertest(drawables);
            HandleHovertestSideEffects();

            m_Shared->DrawScene(drawables);
            DrawConnectionLines();
            DrawHeaderText();
            DrawCancelButton();
        }

    private:
        // data that's shared between other UI states
        std::shared_ptr<SharedData> m_Shared;

        // options for this state
        ChooseElLayerOptions m_Options;

        // (maybe) user mouse hover
        Hover m_MaybeHover;

        // elements selected by user
        std::vector<UID> m_SelectedEls;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;

        // fraction that the system is through its animation cycle: ranges from 0.0 to 1.0 inclusive
        float m_AnimationFraction = 0.0f;
    };
}

// mesh importer tab implementation
class osc::MeshImporterTab::Impl final : public LayerHost {
public:
    Impl(std::weak_ptr<MainUIStateAPI> parent_) :
        m_Parent{std::move(parent_)},
        m_Shared{std::make_shared<SharedData>()}
    {
    }

    Impl(
        std::weak_ptr<MainUIStateAPI> parent_,
        std::vector<std::filesystem::path> meshPaths_) :

        m_Parent{std::move(parent_)},
        m_Shared{std::make_shared<SharedData>(std::move(meshPaths_))}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    bool isUnsaved() const
    {
        return !m_Shared->IsModelGraphUpToDateWithDisk();
    }

    bool trySave()
    {
        if (m_Shared->IsModelGraphUpToDateWithDisk())
        {
            // nothing to save
            return true;
        }
        else
        {
            // try to save the changes
            return m_Shared->ExportAsModelGraphAsOsimFile();
        }
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
    }

    void onUnmount()
    {
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (m_Shared->onEvent(e))
        {
            return true;
        }

        if (m_Maybe3DViewerModal)
        {
            std::shared_ptr<Layer> ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
            if (ptr->onEvent(e))
            {
                return true;
            }
        }

        return false;
    }

    void onTick()
    {
        float dt = osc::App::get().getDeltaSinceLastFrame().count();

        m_Shared->tick(dt);

        if (m_Maybe3DViewerModal)
        {
            std::shared_ptr<Layer> ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
            ptr->tick(dt);
        }

        // if some screen generated an OpenSim::Model, transition to the main editor
        if (m_Shared->HasOutputModel())
        {
            auto ptr = std::make_unique<UndoableModelStatePair>(std::move(m_Shared->UpdOutputModel()));
            ptr->setFixupScaleFactor(m_Shared->GetSceneScaleFactor());
            m_Parent.lock()->addAndSelectTab<ModelEditorTab>(m_Parent, std::move(ptr));
        }

        m_Name = m_Shared->GetRecommendedTitle();

        if (m_Shared->IsCloseRequested())
        {
            m_Parent.lock()->closeTab(m_TabID);
            m_Shared->ResetRequestClose();
        }

        if (m_Shared->IsNewMeshImpoterTabRequested())
        {
            m_Parent.lock()->addAndSelectTab<MeshImporterTab>(m_Parent);
            m_Shared->ResetRequestNewMeshImporter();
        }
    }

    void drawMainMenu()
    {
        DrawMainMenuFileMenu();
        DrawMainMenuEditMenu();
        DrawMainMenuWindowMenu();
        DrawMainMenuAboutMenu();
    }

    void onDraw()
    {
        // enable panel docking
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // handle keyboards using ImGui's input poller
        if (!m_Maybe3DViewerModal)
        {
            UpdateFromImGuiKeyboardState();
        }

        if (!m_Maybe3DViewerModal && m_Shared->IsRenderHovered() && !ImGuizmo::IsUsing())
        {
            UpdatePolarCameraFromImGuiMouseInputs(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
        }

        // draw history panel (if enabled)
        if (m_Shared->m_PanelStates[SharedData::PanelIndex_History])
        {
            if (ImGui::Begin("history", &m_Shared->m_PanelStates[SharedData::PanelIndex_History]))
            {
                DrawHistoryPanelContent();
            }
            ImGui::End();
        }

        // draw navigator panel (if enabled)
        if (m_Shared->m_PanelStates[SharedData::PanelIndex_Navigator])
        {
            if (ImGui::Begin("navigator", &m_Shared->m_PanelStates[SharedData::PanelIndex_Navigator]))
            {
                DrawNavigatorPanelContent();
            }
            ImGui::End();
        }

        // draw log panel (if enabled)
        if (m_Shared->m_PanelStates[SharedData::PanelIndex_Log])
        {
            if (ImGui::Begin("Log", &m_Shared->m_PanelStates[SharedData::PanelIndex_Log], ImGuiWindowFlags_MenuBar))
            {
                m_Shared->m_Logviewer.draw();
            }
            ImGui::End();
        }

        // draw performance panel (if enabled)
        if (m_Shared->m_PanelStates[SharedData::PanelIndex_Performance])
        {
            m_Shared->m_PerfPanel.open();
            m_Shared->m_PerfPanel.draw();
            if (!m_Shared->m_PerfPanel.isOpen())
            {
                m_Shared->m_PanelStates[SharedData::PanelIndex_Performance] = false;
            }
        }

        // draw contextual 3D modal (if there is one), else: draw standard 3D viewer
        DrawMainViewerPanelOrModal();

        // (maybe) draw popup modal
        if (m_Shared->m_MaybeSaveChangesPopup)
        {
            m_Shared->m_MaybeSaveChangesPopup->draw();
        }
    }

private:

    //
    // ACTIONS
    //

    // pop the current UI layer
    void implRequestPop(Layer&) final
    {
        m_Maybe3DViewerModal.reset();
        osc::App::upd().requestRedraw();
    }

    // try to select *only* what is currently hovered
    void SelectJustHover()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        m_Shared->UpdModelGraph().Select(m_MaybeHover.ID);
    }

    // try to select what is currently hovered *and* anything that is "grouped"
    // with the hovered item
    //
    // "grouped" here specifically means other meshes connected to the same body
    void SelectAnythingGroupedWithHover()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        SelectAnythingGroupedWith(m_Shared->UpdModelGraph(), m_MaybeHover.ID);
    }

    // add a body element to whatever's currently hovered at the hover (raycast) position
    void TryAddBodyToHoveredElement()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        AddBody(m_Shared->UpdCommittableModelGraph(), m_MaybeHover.Pos, {m_MaybeHover.ID});
    }

    void TryCreatingJointFromHoveredElement()
    {
        if (!m_MaybeHover)
        {
            return;  // nothing hovered
        }

        ModelGraph const& mg = m_Shared->GetModelGraph();

        SceneEl const* hoveredSceneEl = mg.TryGetElByID(m_MaybeHover.ID);

        if (!hoveredSceneEl)
        {
            return;  // current hover isn't in the current model graph
        }

        UIDT<BodyEl> maybeID = GetStationAttachmentParent(mg, *hoveredSceneEl);

        if (maybeID == c_GroundID || maybeID == c_EmptyID)
        {
            return;  // can't attach to it as-if it were a body
        }

        BodyEl const* bodyEl = mg.TryGetElByID<BodyEl>(maybeID);

        if (!bodyEl)
        {
            return;  // suggested attachment parent isn't in the current model graph?
        }

        TransitionToChoosingJointParent(*bodyEl);
    }

    // try transitioning the shown UI layer to one where the user is assigning a mesh
    void TryTransitionToAssigningHoverAndSelectionNextFrame()
    {
        ModelGraph const& mg = m_Shared->GetModelGraph();

        std::unordered_set<UID> meshes;
        meshes.insert(mg.GetSelected().begin(), mg.GetSelected().end());
        if (m_MaybeHover)
        {
            meshes.insert(m_MaybeHover.ID);
        }

        RemoveErase(meshes, [&mg](UID meshID) { return !mg.ContainsEl<MeshEl>(meshID); });

        if (meshes.empty())
        {
            return;  // nothing to assign
        }

        std::unordered_set<UID> attachments;
        for (UID meshID : meshes)
        {
            attachments.insert(mg.GetElByID<MeshEl>(meshID).getParentID());
        }

        TransitionToAssigningMeshesNextFrame(meshes, attachments);
    }

    void TryAddingStationAtMousePosToHoveredElement()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        AddStationAtLocation(m_Shared->UpdCommittableModelGraph(), m_MaybeHover.ID, m_MaybeHover.Pos);
    }

    //
    // TRANSITIONS
    //
    // methods for transitioning the main 3D UI to some other state
    //

    // transition the shown UI layer to one where the user is assigning a mesh
    void TransitionToAssigningMeshesNextFrame(std::unordered_set<UID> const& meshes, std::unordered_set<UID> const& existingAttachments)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = false;
        opts.maybeElsAttachingTo = meshes;
        opts.isAttachingTowardEl = false;
        opts.maybeElsBeingReplacedByChoice = existingAttachments;
        opts.header = "choose mesh attachment (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, meshes](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryAssignMeshAttachments(shared->UpdCommittableModelGraph(), meshes, choices.front());
        };

        // request a state transition
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing a joint parent
    void TransitionToChoosingJointParent(BodyEl const& child)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = false;
        opts.header = "choose joint parent (ESC to cancel)";
        opts.maybeElsAttachingTo = {child.GetID()};
        opts.isAttachingTowardEl = false;  // away from the body
        opts.onUserChoice = [shared = m_Shared, childID = child.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryCreateJoint(shared->UpdCommittableModelGraph(), childID, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing which element in the scene to point
    // an element's axis towards
    void TransitionToChoosingWhichElementToPointAxisTowards(SceneEl& el, int axis)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.maybeElsAttachingTo = {el.GetID()};
        opts.header = "choose what to point towards (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID(), axis](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return PointAxisTowards(shared->UpdCommittableModelGraph(), id, axis, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void TransitionToChoosingWhichElementToTranslateTo(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.maybeElsAttachingTo = {el.GetID()};
        opts.header = "choose what to translate to (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateElementToAnotherElement(shared->UpdCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void TransitionToChoosingElementsToTranslateBetween(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.maybeElsAttachingTo = {el.GetID()};
        opts.header = "choose two elements to translate between (ESC to cancel)";
        opts.numElementsUserMustChoose = 2;
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.size() < 2)
            {
                return false;
            }

            return TryTranslateBetweenTwoElements(
                shared->UpdCommittableModelGraph(),
                id,
                choices[0],
                choices[1]);
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void TransitionToCopyingSomethingElsesOrientation(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = true;
        opts.maybeElsAttachingTo = {el.GetID()};
        opts.header = "choose which orientation to copy (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryCopyOrientation(shared->UpdCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element should be oriented along
    void TransitionToOrientingElementAlongTwoMeshPoints(SceneEl& el, int axis)
    {
        Select2MeshPointsOptions opts;
        opts.onTwoPointsChosen = [shared = m_Shared, id = el.GetID(), axis](glm::vec3 a, glm::vec3 b)
        {
            return TryOrientElementAxisAlongTwoPoints(shared->UpdCommittableModelGraph(), id, axis, a, b);
        };
        m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element sould be translated to the midpoint of
    void TransitionToTranslatingElementAlongTwoMeshPoints(SceneEl& el)
    {
        Select2MeshPointsOptions opts;
        opts.onTwoPointsChosen = [shared = m_Shared, id = el.GetID()](glm::vec3 a, glm::vec3 b)
        {
            return TryTranslateElementBetweenTwoPoints(shared->UpdCommittableModelGraph(), id, a, b);
        };
        m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
    }

    void TransitionToTranslatingElementToMeshAverageCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = false;
        opts.canChooseGround = false;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateToMeshAverageCenter(shared->UpdCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void TransitionToTranslatingElementToMeshBoundsCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = false;
        opts.canChooseGround = false;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateToMeshBoundsCenter(shared->UpdCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void TransitionToTranslatingElementToMeshMassCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = false;
        opts.canChooseGround = false;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateToMeshMassCenter(shared->UpdCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing another element that
    // the element should be translated to the midpoint of
    void TransitionToTranslatingElementToAnotherElementsCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = true;
        opts.maybeElsAttachingTo = {el.GetID()};
        opts.header = "choose where to place it (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID()](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateElementToAnotherElement(shared->UpdCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void TransitionToReassigningCrossRef(SceneEl& el, int crossrefIdx)
    {
        int nRefs = el.GetNumCrossReferences();

        if (crossrefIdx < 0 || crossrefIdx >= nRefs)
        {
            return;  // invalid index?
        }

        SceneEl const* old = m_Shared->GetModelGraph().TryGetElByID(el.GetCrossReferenceConnecteeID(crossrefIdx));

        if (!old)
        {
            return;  // old el doesn't exist?
        }

        ChooseElLayerOptions opts;
        opts.canChooseBodies = Is<BodyEl>(*old) || Is<GroundEl>(*old);
        opts.canChooseGround = Is<BodyEl>(*old) || Is<GroundEl>(*old);
        opts.canChooseJoints = Is<JointEl>(*old);
        opts.canChooseMeshes = Is<MeshEl>(*old);
        opts.maybeElsAttachingTo = {el.GetID()};
        opts.header = "choose what to attach to";
        opts.onUserChoice = [shared = m_Shared, id = el.GetID(), crossrefIdx](nonstd::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }
            return TryReassignCrossref(shared->UpdCommittableModelGraph(), id, crossrefIdx, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // ensure any stale references into the modelgrah are cleaned up
    void GarbageCollectStaleRefs()
    {
        ModelGraph const& mg = m_Shared->GetModelGraph();

        if (m_MaybeHover && !mg.ContainsEl(m_MaybeHover.ID))
        {
            m_MaybeHover.reset();
        }

        if (m_MaybeOpenedContextMenu && !mg.ContainsEl(m_MaybeOpenedContextMenu.ID))
        {
            m_MaybeOpenedContextMenu.reset();
        }
    }

    // delete currently-selected scene elements
    void DeleteSelected()
    {
        ::DeleteSelected(m_Shared->UpdCommittableModelGraph());
        GarbageCollectStaleRefs();
    }

    // delete a particular scene element
    void DeleteEl(UID elID)
    {
        ::DeleteEl(m_Shared->UpdCommittableModelGraph(), elID);
        GarbageCollectStaleRefs();
    }

    // update this scene from the current keyboard state, as saved by ImGui
    bool UpdateFromImGuiKeyboardState()
    {
        if (ImGui::GetIO().WantCaptureKeyboard)
        {
            return false;
        }

        bool shiftDown = osc::IsShiftDown();
        bool ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_N))
        {
            // Ctrl+N: new scene
            m_Shared->RequestNewMeshImporterTab();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_O))
        {
            // Ctrl+O: open osim
            m_Shared->OpenOsimFileAsModelGraph();
            return true;
        }
        else if (ctrlOrSuperDown && shiftDown && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            // Ctrl+Shift+S: export as: export scene as osim to user-specified location
            m_Shared->ExportAsModelGraphAsOsimFile();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            // Ctrl+S: export: export scene as osim according to typical export heuristic
            m_Shared->ExportModelGraphAsOsimFile();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_W))
        {
            // Ctrl+W: close
            m_Shared->RequestClose();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_Q))
        {
            // Ctrl+Q: quit application
            osc::App::upd().requestQuit();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_A))
        {
            // Ctrl+A: select all
            m_Shared->SelectAll();
            return true;
        }
        else if (ctrlOrSuperDown && shiftDown && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            // Ctrl+Shift+Z: redo
            m_Shared->RedoCurrentModelGraph();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            // Ctrl+Z: undo
            m_Shared->UndoCurrentModelGraph();
            return true;
        }
        else if (osc::IsAnyKeyDown({ImGuiKey_Delete, ImGuiKey_Backspace}))
        {
            // Delete/Backspace: delete any selected elements
            DeleteSelected();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_B))
        {
            // B: add body to hovered element
            TryAddBodyToHoveredElement();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_A))
        {
            // A: assign a parent for the hovered element
            TryTransitionToAssigningHoverAndSelectionNextFrame();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_J))
        {
            // J: try to create a joint
            TryCreatingJointFromHoveredElement();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_T))
        {
            // T: try to add a station to the current hover
            TryAddingStationAtMousePosToHoveredElement();
            return true;
        }
        else if (UpdateImguizmoStateFromKeyboard(m_ImGuizmoState.op, m_ImGuizmoState.mode))
        {
            return true;
        }
        else if (UpdatePolarCameraFromImGuiKeyboardInputs(m_Shared->UpdCamera(), m_Shared->Get3DSceneRect(), CalcSceneAABB()))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void DrawNothingContextMenuContentHeader()
    {
        ImGui::Text(ICON_FA_BOLT " Actions");
        ImGui::SameLine();
        ImGui::TextDisabled("(nothing clicked)");
        ImGui::Separator();
    }

    void DrawSceneElContextMenuContentHeader(SceneEl const& e)
    {
        ImGui::Text("%s %s", e.GetClass().GetIconCStr(), e.GetLabel().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", GetContextMenuSubHeaderText(m_Shared->GetModelGraph(), e).c_str());
        ImGui::SameLine();
        osc::DrawHelpMarker(e.GetClass().GetNameCStr(), e.GetClass().GetDescriptionCStr());
        ImGui::Separator();
    }

    void DrawSceneElPropEditors(SceneEl const& e)
    {
        ModelGraph& mg = m_Shared->UpdModelGraph();

        // label/name editor
        if (CanChangeLabel(e))
        {
            std::string buf{static_cast<std::string_view>(e.GetLabel())};
            if (osc::InputString("Name", buf, 64))
            {
                mg.UpdElByID(e.GetID()).SetLabel(buf);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.GetClass().GetNameSV() << " name";
                m_Shared->CommitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Component Name", "This is the name that the component will have in the exported OpenSim model.");
        }

        // position editor
        if (CanChangePosition(e))
        {
            glm::vec3 translation = e.GetPos();
            if (ImGui::InputFloat3("Translation", glm::value_ptr(translation), OSC_DEFAULT_FLOAT_INPUT_FORMAT))
            {
                mg.UpdElByID(e.GetID()).SetPos(translation);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.GetLabel() << "'s translation";
                m_Shared->CommitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Translation", c_TranslationDescription);
        }

        // rotation editor
        if (CanChangeRotation(e))
        {
            glm::vec3 eulerDegs = glm::degrees(glm::eulerAngles(e.GetRotation()));

            if (ImGui::InputFloat3("Rotation (deg)", glm::value_ptr(eulerDegs), OSC_DEFAULT_FLOAT_INPUT_FORMAT))
            {
                glm::quat quatRads = glm::quat{glm::radians(eulerDegs)};
                mg.UpdElByID(e.GetID()).SetRotation(quatRads);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.GetLabel() << "'s rotation";
                m_Shared->CommitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Rotation", "These are the rotation Euler angles for the component in ground. Positive rotations are anti-clockwise along that axis.\n\nNote: the numbers may contain slight rounding error, due to backend constraints. Your values *should* be accurate to a few decimal places.");
        }

        // scale factor editor
        if (CanChangeScale(e))
        {
            glm::vec3 scaleFactors = e.GetScale();
            if (ImGui::InputFloat3("Scale", glm::value_ptr(scaleFactors), OSC_DEFAULT_FLOAT_INPUT_FORMAT))
            {
                mg.UpdElByID(e.GetID()).SetScale(scaleFactors);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.GetLabel() << "'s scale";
                m_Shared->CommitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Scale", "These are the scale factors of the component in ground. These scale-factors are applied to the element before any other transform (it scales first, then rotates, then translates).");
        }
    }

    // draw content of "Add" menu for some scene element
    void DrawAddOtherToSceneElActions(SceneEl& el, glm::vec3 const& clickPos)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});
        OSC_SCOPE_GUARD({ ImGui::PopStyleVar(); });

        int imguiID = 0;
        ImGui::PushID(imguiID++);
        OSC_SCOPE_GUARD({ ImGui::PopID(); });

        if (CanAttachMeshTo(el))
        {
            if (ImGui::MenuItem(ICON_FA_CUBE " Meshes"))
            {
                m_Shared->PushMeshLoadRequests(el.GetID(), m_Shared->PromptUserForMeshFiles());
            }
            osc::DrawTooltipIfItemHovered("Add Meshes", c_MeshDescription);
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (HasPhysicalSize(el))
        {
            if (ImGui::BeginMenu(ICON_FA_CIRCLE " Body"))
            {
                if (ImGui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                {
                    AddBody(m_Shared->UpdCommittableModelGraph(), el.GetPos(), el.GetID());
                }
                osc::DrawTooltipIfItemHovered("Add Body", c_BodyDescription.c_str());

                if (ImGui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                {
                    AddBody(m_Shared->UpdCommittableModelGraph(), clickPos, el.GetID());
                }
                osc::DrawTooltipIfItemHovered("Add Body", c_BodyDescription.c_str());

                if (ImGui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                {
                    AddBody(m_Shared->UpdCommittableModelGraph());
                }
                osc::DrawTooltipIfItemHovered("Add body", c_StationDescription);

                if (MeshEl const* meshEl = dynamic_cast<MeshEl const*>(&el))
                {
                    if (ImGui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                    {
                        glm::vec3 const location = Midpoint(meshEl->CalcBounds());
                        AddBody(m_Shared->UpdCommittableModelGraph(), location, meshEl->GetID());
                    }
                    osc::DrawTooltipIfItemHovered("Add Body", c_BodyDescription.c_str());

                    if (ImGui::MenuItem(ICON_FA_DIVIDE " at mesh average center"))
                    {
                        glm::vec3 const location = AverageCenter(*meshEl);
                        AddBody(m_Shared->UpdCommittableModelGraph(), location, meshEl->GetID());
                    }
                    osc::DrawTooltipIfItemHovered("Add Body", c_BodyDescription.c_str());

                    if (ImGui::MenuItem(ICON_FA_WEIGHT " at mesh mass center"))
                    {
                        glm::vec3 const location = MassCenter(*meshEl);
                        AddBody(m_Shared->UpdCommittableModelGraph(), location, meshEl->GetID());
                    }
                    osc::DrawTooltipIfItemHovered("Add body", c_StationDescription);
                }

                ImGui::EndMenu();
            }
        }
        else
        {
            if (ImGui::MenuItem(ICON_FA_CIRCLE " Body"))
            {
                AddBody(m_Shared->UpdCommittableModelGraph(), el.GetPos(), el.GetID());
            }
            osc::DrawTooltipIfItemHovered("Add Body", c_BodyDescription.c_str());
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (Is<BodyEl>(el))
        {
            if (ImGui::MenuItem(ICON_FA_LINK " Joint"))
            {
                TransitionToChoosingJointParent(dynamic_cast<BodyEl const&>(el));
            }
            osc::DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (CanAttachStationTo(el))
        {
            if (HasPhysicalSize(el))
            {
                if (ImGui::BeginMenu(ICON_FA_MAP_PIN " Station"))
                {
                    if (ImGui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                    {
                        AddStationAtLocation(m_Shared->UpdCommittableModelGraph(), el, el.GetPos());
                    }
                    osc::DrawTooltipIfItemHovered("Add Station", c_StationDescription);

                    if (ImGui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                    {
                        AddStationAtLocation(m_Shared->UpdCommittableModelGraph(), el, clickPos);
                    }
                    osc::DrawTooltipIfItemHovered("Add Station", c_StationDescription);

                    if (ImGui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                    {
                        AddStationAtLocation(m_Shared->UpdCommittableModelGraph(), el, glm::vec3{});
                    }
                    osc::DrawTooltipIfItemHovered("Add Station", c_StationDescription);

                    if (Is<MeshEl>(el))
                    {
                        if (ImGui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                        {
                            AddStationAtLocation(m_Shared->UpdCommittableModelGraph(), el, Midpoint(el.CalcBounds()));
                        }
                        osc::DrawTooltipIfItemHovered("Add Station", c_StationDescription);
                    }

                    ImGui::EndMenu();
                }
            }
            else
            {
                if (ImGui::MenuItem(ICON_FA_MAP_PIN " Station"))
                {
                    AddStationAtLocation(m_Shared->UpdCommittableModelGraph(), el, el.GetPos());
                }
                osc::DrawTooltipIfItemHovered("Add Station", c_StationDescription);
            }

        }
    }

    void DrawNothingActions()
    {
        if (ImGui::MenuItem(ICON_FA_CUBE " Add Meshes"))
        {
            m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        osc::DrawTooltipIfItemHovered("Add Meshes to the model", c_MeshDescription);

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add Other"))
        {
            DrawAddOtherMenuItems();

            ImGui::EndMenu();
        }
    }

    void DrawSceneElActions(SceneEl& el, glm::vec3 const& clickPos)
    {
        if (ImGui::MenuItem(ICON_FA_CAMERA " Focus camera on this"))
        {
            m_Shared->FocusCameraOn(Midpoint(el.CalcBounds()));
        }
        osc::DrawTooltipIfItemHovered("Focus camera on this scene element", "Focuses the scene camera on this element. This is useful for tracking the camera around that particular object in the scene");

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            DrawAddOtherToSceneElActions(el, clickPos);
            ImGui::EndMenu();
        }

        if (Is<BodyEl>(el))
        {
            if (ImGui::MenuItem(ICON_FA_LINK " Join to"))
            {
                TransitionToChoosingJointParent(dynamic_cast<BodyEl const&>(el));
            }
            osc::DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }

        if (CanDelete(el))
        {
            if (ImGui::MenuItem(ICON_FA_TRASH " Delete"))
            {
                ::DeleteEl(m_Shared->UpdCommittableModelGraph(), el.GetID());
                GarbageCollectStaleRefs();
                ImGui::CloseCurrentPopup();
            }
            osc::DrawTooltipIfItemHovered("Delete", "Deletes the component from the model. Deletion is undo-able (use the undo/redo feature). Anything attached to this element (e.g. joints, meshes) will also be deleted.");
        }
    }

    // draw the "Translate" menu for any generic `SceneEl`
    void DrawTranslateMenu(SceneEl& el)
    {
        if (!CanChangePosition(el))
        {
            return;  // can't change its position
        }

        if (!ImGui::BeginMenu(ICON_FA_ARROWS_ALT " Translate"))
        {
            return;  // top-level menu isn't open
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        for (int i = 0, len = el.GetNumCrossReferences(); i < len; ++i)
        {
            std::string label = "To " + el.GetCrossReferenceLabel(i);
            if (ImGui::MenuItem(label.c_str()))
            {
                TryTranslateElementToAnotherElement(m_Shared->UpdCommittableModelGraph(), el.GetID(), el.GetCrossReferenceConnecteeID(i));
            }
        }

        if (ImGui::MenuItem("To (select something)"))
        {
            TransitionToChoosingWhichElementToTranslateTo(el);
        }

        if (el.GetNumCrossReferences() == 2)
        {
            std::string label = "Between " + el.GetCrossReferenceLabel(0) + " and " + el.GetCrossReferenceLabel(1);
            if (ImGui::MenuItem(label.c_str()))
            {
                UID a = el.GetCrossReferenceConnecteeID(0);
                UID b = el.GetCrossReferenceConnecteeID(1);
                TryTranslateBetweenTwoElements(m_Shared->UpdCommittableModelGraph(), el.GetID(), a, b);
            }
        }

        if (ImGui::MenuItem("Between two scene elements"))
        {
            TransitionToChoosingElementsToTranslateBetween(el);
        }

        if (ImGui::MenuItem("Between two mesh points"))
        {
            TransitionToTranslatingElementAlongTwoMeshPoints(el);
        }

        if (ImGui::MenuItem("To mesh bounds center"))
        {
            TransitionToTranslatingElementToMeshBoundsCenter(el);
        }
        osc::DrawTooltipIfItemHovered("Translate to mesh bounds center", "Translates the given element to the center of the selected mesh's bounding box. The bounding box is the smallest box that contains all mesh vertices");

        if (ImGui::MenuItem("To mesh average center"))
        {
            TransitionToTranslatingElementToMeshAverageCenter(el);
        }
        osc::DrawTooltipIfItemHovered("Translate to mesh average center", "Translates the given element to the average center point of vertices in the selected mesh.\n\nEffectively, this adds each vertex location in the mesh, divides the sum by the number of vertices in the mesh, and sets the translation of the given object to that location.");

        if (ImGui::MenuItem("To mesh mass center"))
        {
            TransitionToTranslatingElementToMeshMassCenter(el);
        }
        osc::DrawTooltipIfItemHovered("Translate to mesh mess center", "Translates the given element to the mass center of the selected mesh.\n\nCAREFUL: the algorithm used to do this heavily relies on your triangle winding (i.e. normals) being correct and your mesh being a closed surface. If your mesh doesn't meet these requirements, you might get strange results (apologies: the only way to get around that problems involves complicated voxelization and leak-detection algorithms :( )");

        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    // draw the "Reorient" menu for any generic `SceneEl`
    void DrawReorientMenu(SceneEl& el)
    {
        if (!CanChangeRotation(el))
        {
            return;  // can't change its rotation
        }

        if (!ImGui::BeginMenu(ICON_FA_REDO " Reorient"))
        {
            return;  // top-level menu isn't open
        }
        osc::DrawTooltipIfItemHovered("Reorient the scene element", "Rotates the scene element in without changing its position");

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        {
            auto DrawMenuContent = [&](int axis)
            {
                for (int i = 0, len = el.GetNumCrossReferences(); i < len; ++i)
                {
                    std::string label = "Towards " + el.GetCrossReferenceLabel(i);

                    if (ImGui::MenuItem(label.c_str()))
                    {
                        PointAxisTowards(m_Shared->UpdCommittableModelGraph(), el.GetID(), axis, el.GetCrossReferenceConnecteeID(i));
                    }
                }

                if (ImGui::MenuItem("Towards (select something)"))
                {
                    TransitionToChoosingWhichElementToPointAxisTowards(el, axis);
                }

                if (ImGui::MenuItem("90 degress"))
                {
                    RotateAxisXRadians(m_Shared->UpdCommittableModelGraph(), el, axis, fpi/2.0f);
                }

                if (ImGui::MenuItem("180 degrees"))
                {
                    RotateAxisXRadians(m_Shared->UpdCommittableModelGraph(), el, axis, fpi);
                }

                if (ImGui::MenuItem("Along two mesh points"))
                {
                    TransitionToOrientingElementAlongTwoMeshPoints(el, axis);
                }
            };

            if (ImGui::BeginMenu("x"))
            {
                DrawMenuContent(0);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("y"))
            {
                DrawMenuContent(1);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("z"))
            {
                DrawMenuContent(2);
                ImGui::EndMenu();
            }
        }

        if (ImGui::MenuItem("copy"))
        {
            TransitionToCopyingSomethingElsesOrientation(el);
        }

        if (ImGui::MenuItem("reset"))
        {
            el.SetXform(Transform{el.GetPos()});
            m_Shared->CommitCurrentModelGraph("reset " + el.GetLabel() + " orientation");
        }

        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    // draw the "Mass" editor for a `BodyEl`
    void DrawMassEditor(BodyEl const& bodyEl)
    {
        float curMass = static_cast<float>(bodyEl.getMass());
        if (ImGui::InputFloat("Mass", &curMass, 0.0f, 0.0f, OSC_DEFAULT_FLOAT_INPUT_FORMAT))
        {
            m_Shared->UpdModelGraph().UpdElByID<BodyEl>(bodyEl.GetID()).setMass(static_cast<double>(curMass));
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            m_Shared->CommitCurrentModelGraph("changed body mass");
        }
        ImGui::SameLine();
        osc::DrawHelpMarker("Mass", "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
    }

    // draw the "Joint Type" editor for a `JointEl`
    void DrawJointTypeEditor(JointEl const& jointEl)
    {
        int currentIdx = static_cast<int>(jointEl.getJointTypeIndex());
        nonstd::span<char const* const> labels = osc::JointRegistry::nameCStrings();
        if (ImGui::Combo("Joint Type", &currentIdx, labels.data(), static_cast<int>(labels.size())))
        {
            m_Shared->UpdModelGraph().UpdElByID<JointEl>(jointEl.GetID()).setJointTypeIndex(static_cast<size_t>(currentIdx));
            m_Shared->CommitCurrentModelGraph("changed joint type");
        }
        ImGui::SameLine();
        osc::DrawHelpMarker("Joint Type", "This is the type of joint that should be added into the OpenSim model. The joint's type dictates what types of motion are permitted around the joint center. See the official OpenSim documentation for an explanation of each joint type.");
    }

    // draw the "Reassign Connection" menu, which lets users change an element's cross reference
    void DrawReassignCrossrefMenu(SceneEl& el)
    {
        int nRefs = el.GetNumCrossReferences();

        if (nRefs == 0)
        {
            return;
        }

        if (ImGui::BeginMenu(ICON_FA_EXTERNAL_LINK_ALT " Reassign Connection"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

            for (int i = 0; i < nRefs; ++i)
            {
                osc::CStringView label = el.GetCrossReferenceLabel(i);
                if (ImGui::MenuItem(label.c_str()))
                {
                    TransitionToReassigningCrossRef(el, i);
                }
            }

            ImGui::PopStyleVar();
            ImGui::EndMenu();
        }
    }

    // draw context menu content for when user right-clicks nothing
    void DrawNothingContextMenuContent()
    {
        DrawNothingContextMenuContentHeader();

        SpacerDummy();

        DrawNothingActions();
    }

    // draw context menu content for a `GroundEl`
    void DrawContextMenuContent(GroundEl& el, glm::vec3 const& clickPos)
    {
        DrawSceneElContextMenuContentHeader(el);

        SpacerDummy();

        DrawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `BodyEl`
    void DrawContextMenuContent(BodyEl& el, glm::vec3 const& clickPos)
    {
        DrawSceneElContextMenuContentHeader(el);

        SpacerDummy();

        DrawSceneElPropEditors(el);
        DrawMassEditor(el);

        SpacerDummy();

        DrawTranslateMenu(el);
        DrawReorientMenu(el);
        DrawReassignCrossrefMenu(el);
        DrawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `MeshEl`
    void DrawContextMenuContent(MeshEl& el, glm::vec3 const& clickPos)
    {
        DrawSceneElContextMenuContentHeader(el);

        SpacerDummy();

        DrawSceneElPropEditors(el);

        SpacerDummy();

        DrawTranslateMenu(el);
        DrawReorientMenu(el);
        DrawReassignCrossrefMenu(el);
        DrawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `JointEl`
    void DrawContextMenuContent(JointEl& el, glm::vec3 const& clickPos)
    {
        DrawSceneElContextMenuContentHeader(el);

        SpacerDummy();

        DrawSceneElPropEditors(el);
        DrawJointTypeEditor(el);

        SpacerDummy();

        DrawTranslateMenu(el);
        DrawReorientMenu(el);
        DrawReassignCrossrefMenu(el);
        DrawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `StationEl`
    void DrawContextMenuContent(StationEl& el, glm::vec3 const& clickPos)
    {
        DrawSceneElContextMenuContentHeader(el);

        SpacerDummy();

        DrawSceneElPropEditors(el);

        SpacerDummy();

        DrawTranslateMenu(el);
        DrawReorientMenu(el);
        DrawReassignCrossrefMenu(el);
        DrawSceneElActions(el, clickPos);
    }

    // draw context menu content for some scene element
    void DrawContextMenuContent(SceneEl& el, glm::vec3 const& clickPos)
    {
        // helper class for visiting each type of scene element
        class Visitor final : public SceneElVisitor {
        public:
            Visitor(
                osc::MeshImporterTab::Impl& state,
                glm::vec3 const& clickPos) :

                m_State{state},
                m_ClickPos{clickPos}
            {
            }

            void operator()(GroundEl& el) final
            {
                m_State.DrawContextMenuContent(el, m_ClickPos);
            }

            void operator()(MeshEl& el) final
            {
                m_State.DrawContextMenuContent(el, m_ClickPos);
            }

            void operator()(BodyEl& el) final
            {
                m_State.DrawContextMenuContent(el, m_ClickPos);
            }

            void operator()(JointEl& el) final
            {
                m_State.DrawContextMenuContent(el, m_ClickPos);
            }

            void operator()(StationEl& el) final
            {
                m_State.DrawContextMenuContent(el, m_ClickPos);
            }
        private:
            osc::MeshImporterTab::Impl& m_State;
            glm::vec3 const& m_ClickPos;
        };

        // context menu was opened on a scene element that exists in the modelgraph
        Visitor visitor{*this, clickPos};
        el.Accept(visitor);
    }

    // draw a context menu for the current state (if applicable)
    void DrawContextMenuContent()
    {
        if (!m_MaybeOpenedContextMenu)
        {
            // context menu not open, but just draw the "nothing" menu
            PushID(UID::empty());
            OSC_SCOPE_GUARD({ ImGui::PopID(); });
            DrawNothingContextMenuContent();
        }
        else if (m_MaybeOpenedContextMenu.ID == c_RightClickedNothingID)
        {
            // context menu was opened on "nothing" specifically
            PushID(UID::empty());
            OSC_SCOPE_GUARD({ ImGui::PopID(); });
            DrawNothingContextMenuContent();
        }
        else if (SceneEl* el = m_Shared->UpdModelGraph().TryUpdElByID(m_MaybeOpenedContextMenu.ID))
        {
            // context menu was opened on a scene element that exists in the modelgraph
            PushID(el->GetID());
            OSC_SCOPE_GUARD({ ImGui::PopID(); });
            DrawContextMenuContent(*el, m_MaybeOpenedContextMenu.Pos);
        }


        // context menu should be closed under these conditions
        if (osc::IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Escape}))
        {
            m_MaybeOpenedContextMenu.reset();
            ImGui::CloseCurrentPopup();
        }
    }

    // draw the content of the (undo/redo) "History" panel
    void DrawHistoryPanelContent()
    {
        CommittableModelGraph& storage = m_Shared->UpdCommittableModelGraph();

        std::vector<ModelGraphCommit const*> commits;
        storage.ForEachCommitUnordered([&commits](ModelGraphCommit const& c)
            {
                commits.push_back(&c);
            });

        auto orderedByTime = [](ModelGraphCommit const* a, ModelGraphCommit const* b)
        {
            return a->GetCommitTime() < b->GetCommitTime();
        };

        osc::Sort(commits, orderedByTime);

        int i = 0;
        for (ModelGraphCommit const* c : commits)
        {
            ImGui::PushID(static_cast<int>(i++));

            if (ImGui::Selectable(c->GetCommitMessage().c_str(), c->GetID() == storage.GetCheckoutID()))
            {
                storage.Checkout(c->GetID());
            }

            ImGui::PopID();
        }
    }

    void DrawNavigatorElement(SceneElClass const& c)
    {
        ModelGraph& mg = m_Shared->UpdModelGraph();

        ImGui::Text("%s %s", c.GetIconCStr(), c.GetNamePluralizedCStr());
        ImGui::SameLine();
        osc::DrawHelpMarker(c.GetNamePluralizedCStr(), c.GetDescriptionCStr());
        SpacerDummy();
        ImGui::Indent();

        bool empty = true;
        for (SceneEl const& el : mg.iter())
        {
            if (el.GetClass() != c)
            {
                continue;
            }

            empty = false;

            UID id = el.GetID();
            int styles = 0;

            if (id == m_MaybeHover.ID)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_HOVERED_COMPONENT_RGBA);
                ++styles;
            }
            else if (m_Shared->IsSelected(id))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_SELECTED_COMPONENT_RGBA);
                ++styles;
            }

            ImGui::Text("%s", el.GetLabel().c_str());

            ImGui::PopStyleColor(styles);

            if (ImGui::IsItemHovered())
            {
                m_MaybeHover = {id, {}};
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                if (!osc::IsShiftDown())
                {
                    m_Shared->UpdModelGraph().DeSelectAll();
                }
                m_Shared->UpdModelGraph().Select(id);
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                m_MaybeOpenedContextMenu = Hover{id, {}};
                ImGui::OpenPopup("##maincontextmenu");
                osc::App::upd().requestRedraw();
            }
        }

        if (empty)
        {
            ImGui::TextDisabled("(no %s)", c.GetNamePluralizedCStr());
        }
        ImGui::Unindent();
    }

    void DrawNavigatorPanelContent()
    {
        for (SceneElClass const* c : GetSceneElClasses())
        {
            DrawNavigatorElement(*c);
            SpacerDummy();
        }

        // a navigator element might have opened the context menu in the navigator panel
        //
        // this can happen when the user right-clicks something in the navigator
        if (ImGui::BeginPopup("##maincontextmenu"))
        {
            DrawContextMenuContent();
            ImGui::EndPopup();
        }
    }

    void DrawAddOtherMenuItems()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        if (ImGui::MenuItem(ICON_FA_CUBE " Meshes"))
        {
            m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        osc::DrawTooltipIfItemHovered("Add Meshes", c_MeshDescription);

        if (ImGui::MenuItem(ICON_FA_CIRCLE " Body"))
        {
            AddBody(m_Shared->UpdCommittableModelGraph());
        }
        osc::DrawTooltipIfItemHovered("Add Body", c_BodyDescription);

        if (ImGui::MenuItem(ICON_FA_MAP_PIN " Station"))
        {
            ModelGraph& mg = m_Shared->UpdModelGraph();
            StationEl& e = mg.AddEl<StationEl>(UIDT<StationEl>{}, c_GroundID, glm::vec3{}, GenerateName(StationEl::Class()));
            SelectOnly(mg, e);
        }
        osc::DrawTooltipIfItemHovered("Add Station", StationEl::Class().GetDescriptionCStr());

        ImGui::PopStyleVar();
    }

    void Draw3DViewerOverlayTopBar()
    {
        int imguiID = 0;

        if (ImGui::Button(ICON_FA_CUBE " Add Meshes"))
        {
            m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        osc::DrawTooltipIfItemHovered("Add Meshes to the model", c_MeshDescription);

        ImGui::SameLine();

        ImGui::Button(ICON_FA_PLUS " Add Other");
        osc::DrawTooltipIfItemHovered("Add components to the model");

        if (ImGui::BeginPopupContextItem("##additemtoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawAddOtherMenuItems();
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_PAINT_ROLLER " Colors");
        osc::DrawTooltipIfItemHovered("Change scene display colors", "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

        if (ImGui::BeginPopupContextItem("##addpainttoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            nonstd::span<glm::vec4 const> colors = m_Shared->GetColors();
            nonstd::span<char const* const> labels = m_Shared->GetColorLabels();
            OSC_ASSERT(colors.size() == labels.size() && "every color should have a label");

            for (size_t i = 0; i < colors.size(); ++i)
            {
                glm::vec4 colorVal = colors[i];
                ImGui::PushID(imguiID++);
                if (ImGui::ColorEdit4(labels[i], glm::value_ptr(colorVal)))
                {
                    m_Shared->SetColor(i, colorVal);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_EYE " Visibility");
        osc::DrawTooltipIfItemHovered("Change what's visible in the 3D scene", "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

        if (ImGui::BeginPopupContextItem("##changevisibilitypopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            nonstd::span<bool const> visibilities = m_Shared->GetVisibilityFlags();
            nonstd::span<char const* const> labels = m_Shared->GetVisibilityFlagLabels();
            OSC_ASSERT(visibilities.size() == labels.size() && "every visibility flag should have a label");

            for (size_t i = 0; i < visibilities.size(); ++i)
            {
                bool v = visibilities[i];
                ImGui::PushID(imguiID++);
                if (ImGui::Checkbox(labels[i], &v))
                {
                    m_Shared->SetVisibilityFlag(i, v);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_LOCK " Interactivity");
        osc::DrawTooltipIfItemHovered("Change what your mouse can interact with in the 3D scene", "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

        if (ImGui::BeginPopupContextItem("##changeinteractionlockspopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            nonstd::span<bool const> interactables = m_Shared->GetIneractivityFlags();
            nonstd::span<char const* const> labels =  m_Shared->GetInteractivityFlagLabels();
            OSC_ASSERT(interactables.size() == labels.size());

            for (size_t i = 0; i < interactables.size(); ++i)
            {
                bool v = interactables[i];
                ImGui::PushID(imguiID++);
                if (ImGui::Checkbox(labels[i], &v))
                {
                    m_Shared->SetInteractivityFlag(i, v);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        DrawGizmoOpSelector(m_ImGuizmoState.op);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
        ImGui::SameLine();
        ImGui::PopStyleVar();

        // local/global dropdown
        DrawGizmoModeSelector(m_ImGuizmoState.mode);
        ImGui::SameLine();

        // scale factor
        {
            char const* const tooltipTitle = "Change scene scale factor";
            char const* const tooltipDesc = "This rescales *some* elements in the scene. Specifically, the ones that have no 'size', such as body frames, joint frames, and the chequered floor texture.\n\nChanging this is handy if you are working on smaller or larger models, where the size of the (decorative) frames and floor are too large/small compared to the model you are working on.\n\nThis is purely decorative and does not affect the exported OpenSim model in any way.";

            float sf = m_Shared->GetSceneScaleFactor();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("1000.00").x);
            if (ImGui::InputFloat("scene scale factor", &sf))
            {
                m_Shared->SetSceneScaleFactor(sf);
            }
            osc::DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
        }
    }

    std::optional<AABB> CalcSceneAABB() const
    {
        auto const it = m_DrawablesBuffer.begin();
        std::optional<AABB> rv;
        for (DrawableThing const& drawable : m_DrawablesBuffer)
        {
            if (drawable.id != c_EmptyID)
            {
                AABB const bounds = CalcBounds(drawable);
                rv = rv ? Union(*rv, bounds) : bounds;
            }
        }
        return rv;
    }

    void Draw3DViewerOverlayBottomBar()
    {
        ImGui::PushID("##3DViewerOverlay");

        // bottom-left axes overlay
        {
            ImGuiStyle const& style = ImGui::GetStyle();
            Rect const& r = m_Shared->Get3DSceneRect();
            glm::vec2 const topLeft =
            {
                r.p1.x + style.WindowPadding.x,
                r.p2.y - style.WindowPadding.y - CalcAlignmentAxesDimensions().y,
            };
            ImGui::SetCursorScreenPos(topLeft);
            DrawAlignmentAxes(m_Shared->GetCamera().getViewMtx());
        }

        Rect sceneRect = m_Shared->Get3DSceneRect();
        glm::vec2 trPos = {sceneRect.p1.x + 100.0f, sceneRect.p2.y - 55.0f};
        ImGui::SetCursorScreenPos(trPos);

        if (ImGui::Button(ICON_FA_SEARCH_MINUS))
        {
            m_Shared->UpdCamera().radius *= 1.2f;
        }
        osc::DrawTooltipIfItemHovered("Zoom Out");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_SEARCH_PLUS))
        {
            m_Shared->UpdCamera().radius *= 0.8f;
        }
        osc::DrawTooltipIfItemHovered("Zoom In");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
        {
            if (std::optional<AABB> const sceneAABB = CalcSceneAABB())
            {
                osc::AutoFocus(m_Shared->UpdCamera(), *sceneAABB, osc::AspectRatio(m_Shared->Get3DSceneDims()));
            }
        }
        osc::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");

        ImGui::SameLine();

        if (ImGui::Button("X"))
        {
            m_Shared->UpdCamera().theta = fpi2;
            m_Shared->UpdCamera().phi = 0.0f;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->UpdCamera().theta = -fpi2;
            m_Shared->UpdCamera().phi = 0.0f;
        }
        osc::DrawTooltipIfItemHovered("Face camera facing along X", "Right-clicking faces it along X, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button("Y"))
        {
            m_Shared->UpdCamera().theta = 0.0f;
            m_Shared->UpdCamera().phi = fpi2;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->UpdCamera().theta = 0.0f;
            m_Shared->UpdCamera().phi = -fpi2;
        }
        osc::DrawTooltipIfItemHovered("Face camera facing along Y", "Right-clicking faces it along Y, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button("Z"))
        {
            m_Shared->UpdCamera().theta = 0.0f;
            m_Shared->UpdCamera().phi = 0.0f;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->UpdCamera().theta = fpi;
            m_Shared->UpdCamera().phi = 0.0f;
        }
        osc::DrawTooltipIfItemHovered("Face camera facing along Z", "Right-clicking faces it along Z, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_CAMERA))
        {
            m_Shared->UpdCamera() = CreateDefaultCamera();
        }
        osc::DrawTooltipIfItemHovered("Reset camera", "Resets the camera to its default position (the position it's in when the wizard is first loaded)");

        ImGui::PopID();
    }

    void Draw3DViewerOverlayConvertToOpenSimModelButton()
    {
        char const* const text = "Convert to OpenSim Model " ICON_FA_ARROW_RIGHT;

        glm::vec2 framePad = {10.0f, 10.0f};
        glm::vec2 margin = {25.0f, 35.0f};
        Rect sceneRect = m_Shared->Get3DSceneRect();
        glm::vec2 textDims = ImGui::CalcTextSize(text);

        ImGui::SetCursorScreenPos(sceneRect.p2 - textDims - framePad - margin);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePad);
        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(text))
        {
            m_Shared->TryCreateOutputModel();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        osc::DrawTooltipIfItemHovered("Convert current scene to an OpenSim Model", "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nYour progress in this tab will remain untouched.");
    }

    void Draw3DViewerOverlay()
    {
        Draw3DViewerOverlayTopBar();
        Draw3DViewerOverlayBottomBar();
        Draw3DViewerOverlayConvertToOpenSimModelButton();
    }

    void DrawSceneElTooltip(SceneEl const& e) const
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s %s", e.GetClass().GetIconCStr(), e.GetLabel().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", GetContextMenuSubHeaderText(m_Shared->GetModelGraph(), e).c_str());
        ImGui::EndTooltip();
    }

    void DrawHoverTooltip()
    {
        if (!m_MaybeHover)
        {
            return;  // nothing is hovered
        }

        if (SceneEl const* e = m_Shared->GetModelGraph().TryGetElByID(m_MaybeHover.ID))
        {
            DrawSceneElTooltip(*e);
        }
    }

    // draws 3D manipulator overlays (drag handles, etc.)
    void DrawSelection3DManipulatorGizmos()
    {
        if (!m_Shared->HasSelection())
        {
            return;  // can only manipulate if selecting something
        }

        // if the user isn't *currently* manipulating anything, create an
        // up-to-date manipulation matrix
        //
        // this is so that ImGuizmo can *show* the manipulation axes, and
        // because the user might start manipulating during this frame
        if (!ImGuizmo::IsUsing())
        {
            auto it = m_Shared->GetCurrentSelection().begin();
            auto end = m_Shared->GetCurrentSelection().end();

            if (it == end)
            {
                return;  // sanity exit
            }

            ModelGraph const& mg = m_Shared->GetModelGraph();

            int n = 0;

            Transform ras = GetTransform(mg, *it);
            ++it;
            ++n;

            while (it != end)
            {
                ras += GetTransform(mg, *it);
                ++it;
                ++n;
            }

            ras /= static_cast<float>(n);
            ras.rotation = glm::normalize(ras.rotation);

            m_ImGuizmoState.mtx = ToMat4(ras);
        }

        // else: is using OR nselected > 0 (so draw it)

        Rect sceneRect = m_Shared->Get3DSceneRect();

        ImGuizmo::SetRect(
            sceneRect.p1.x,
            sceneRect.p1.y,
            Dimensions(sceneRect).x,
            Dimensions(sceneRect).y
        );
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::AllowAxisFlip(false);  // user's didn't like this feature in UX sessions

        glm::mat4 delta;
        SetImguizmoStyleToOSCStandard();
        bool manipulated = ImGuizmo::Manipulate(
            glm::value_ptr(m_Shared->GetCamera().getViewMtx()),
            glm::value_ptr(m_Shared->GetCamera().getProjMtx(AspectRatio(sceneRect))),
            m_ImGuizmoState.op,
            m_ImGuizmoState.mode,
            glm::value_ptr(m_ImGuizmoState.mtx),
            glm::value_ptr(delta),
            nullptr,
            nullptr,
            nullptr
        );

        bool isUsingThisFrame = ImGuizmo::IsUsing();
        bool wasUsingLastFrame = m_ImGuizmoState.wasUsingLastFrame;
        m_ImGuizmoState.wasUsingLastFrame = isUsingThisFrame;  // so next frame can know

                                                               // if the user was using the gizmo last frame, and isn't using it this frame,
                                                               // then they probably just finished a manipulation, which should be snapshotted
                                                               // for undo/redo support
        if (wasUsingLastFrame && !isUsingThisFrame)
        {
            m_Shared->CommitCurrentModelGraph("manipulated selection");
            osc::App::upd().requestRedraw();
        }

        // if no manipulation happened this frame, exit early
        if (!manipulated)
        {
            return;
        }

        glm::vec3 translation;
        glm::vec3 rotation;
        glm::vec3 scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(delta),
            glm::value_ptr(translation),
            glm::value_ptr(rotation),
            glm::value_ptr(scale)
        );
        rotation = glm::radians(rotation);

        for (UID id : m_Shared->GetCurrentSelection())
        {
            SceneEl& el = m_Shared->UpdModelGraph().UpdElByID(id);
            switch (m_ImGuizmoState.op) {
            case ImGuizmo::ROTATE:
                ApplyRotation(el, rotation, m_ImGuizmoState.mtx[3]);
                break;
            case ImGuizmo::TRANSLATE:
                ApplyTranslation(el, translation);
                break;
            case ImGuizmo::SCALE:
                ApplyScale(el, scale);
                break;
            default:
                break;
            }
        }
    }

    // perform a hovertest on the current 3D scene to determine what the user's mouse is over
    Hover HovertestScene(std::vector<DrawableThing> const& drawables)
    {
        if (!m_Shared->IsRenderHovered())
        {
            return m_MaybeHover;
        }

        if (ImGuizmo::IsUsing())
        {
            return Hover{};
        }

        return m_Shared->Hovertest(drawables);
    }

    // handle any side effects for current user mouse hover
    void HandleCurrentHover()
    {
        if (!m_Shared->IsRenderHovered())
        {
            return;  // nothing hovered
        }

        bool const lcClicked = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
        bool const shiftDown = osc::IsShiftDown();
        bool const altDown = osc::IsAltDown();
        bool const isUsingGizmo = ImGuizmo::IsUsing();

        if (!m_MaybeHover && lcClicked && !isUsingGizmo && !shiftDown)
        {
            // user clicked in some empty part of the screen: clear selection
            m_Shared->DeSelectAll();
        }
        else if (m_MaybeHover && lcClicked && !isUsingGizmo)
        {
            // user clicked hovered thing: select hovered thing
            if (!shiftDown)
            {
                // user wasn't holding SHIFT, so clear selection
                m_Shared->DeSelectAll();
            }

            if (altDown)
            {
                // ALT: only select the thing the mouse is over
                SelectJustHover();
            }
            else
            {
                // NO ALT: select the "grouped items"
                SelectAnythingGroupedWithHover();
            }
        }
    }

    // generate 3D scene drawables for current state
    std::vector<DrawableThing>& GenerateDrawables()
    {
        m_DrawablesBuffer.clear();

        for (SceneEl const& e : m_Shared->GetModelGraph().iter())
        {
            m_Shared->AppendDrawables(e, m_DrawablesBuffer);
        }

        if (m_Shared->IsShowingFloor())
        {
            m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());
        }

        return m_DrawablesBuffer;
    }

    // draws main 3D viewer panel
    void Draw3DViewer()
    {
        m_Shared->SetContentRegionAvailAsSceneRect();

        std::vector<DrawableThing>& sceneEls = GenerateDrawables();

        // hovertest the generated geometry
        m_MaybeHover = HovertestScene(sceneEls);
        HandleCurrentHover();

        // assign rim highlights based on hover
        for (DrawableThing& dt : sceneEls)
        {
            dt.flags = ComputeFlags(m_Shared->GetModelGraph(), dt.id, m_MaybeHover.ID);
        }

        // draw 3D scene (effectively, as an ImGui::Image)
        m_Shared->DrawScene(sceneEls);
        if (m_Shared->IsRenderHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
        {
            m_MaybeOpenedContextMenu = m_MaybeHover;
            ImGui::OpenPopup("##maincontextmenu");
        }

        bool ctxMenuShowing = false;
        if (ImGui::BeginPopup("##maincontextmenu"))
        {
            ctxMenuShowing = true;
            DrawContextMenuContent();
            ImGui::EndPopup();
        }

        if (m_Shared->IsRenderHovered() && m_MaybeHover && (ctxMenuShowing ? m_MaybeHover.ID != m_MaybeOpenedContextMenu.ID : true))
        {
            DrawHoverTooltip();
        }

        // draw overlays/gizmos
        DrawSelection3DManipulatorGizmos();
        m_Shared->DrawConnectionLines(m_MaybeHover);
    }

    void DrawMainMenuFileMenu()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N"))
            {
                m_Shared->RequestNewMeshImporterTab();
            }

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Import", "Ctrl+O"))
            {
                m_Shared->OpenOsimFileAsModelGraph();
            }
            osc::DrawTooltipIfItemHovered("Import osim into mesh importer", "Try to import an existing osim file into the mesh importer.\n\nBEWARE: the mesh importer is *not* an OpenSim model editor. The import process will delete information from your osim in order to 'jam' it into this screen. The main purpose of this button is to export/import mesh editor scenes, not to edit existing OpenSim models.");

            if (ImGui::MenuItem(ICON_FA_SAVE " Export", "Ctrl+S"))
            {
                m_Shared->ExportModelGraphAsOsimFile();
            }
            osc::DrawTooltipIfItemHovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            if (ImGui::MenuItem(ICON_FA_SAVE " Export As", "Shift+Ctrl+S"))
            {
                m_Shared->ExportAsModelGraphAsOsimFile();
            }
            osc::DrawTooltipIfItemHovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W"))
            {
                m_Shared->RequestClose();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
            {
                osc::App::upd().requestQuit();
            }

            ImGui::EndMenu();
        }
    }

    void DrawMainMenuEditMenu()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Shared->CanUndoCurrentModelGraph()))
            {
                m_Shared->UndoCurrentModelGraph();
            }
            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Shared->CanRedoCurrentModelGraph()))
            {
                m_Shared->RedoCurrentModelGraph();
            }
            ImGui::EndMenu();
        }
    }

    void DrawMainMenuWindowMenu()
    {

        if (ImGui::BeginMenu("Window"))
        {
            for (int i = 0; i < SharedData::PanelIndex_COUNT; ++i)
            {
                if (ImGui::MenuItem(SharedData::c_OpenedPanelNames[i], nullptr, m_Shared->m_PanelStates[i]))
                {
                    m_Shared->m_PanelStates[i] = !m_Shared->m_PanelStates[i];
                }
            }
            ImGui::EndMenu();
        }
    }

    void DrawMainMenuAboutMenu()
    {
        osc::MainMenuAboutTab{}.draw();
    }

    // draws main 3D viewer, or a modal (if one is active)
    void DrawMainViewerPanelOrModal()
    {
        if (m_Maybe3DViewerModal)
        {
            // ensure it stays alive - even if it pops itself during the drawcall
            std::shared_ptr<Layer> const ptr = m_Maybe3DViewerModal;

            // open it "over" the whole UI as a "modal" - so that the user can't click things
            // outside of the panel
            ImGui::OpenPopup("##visualizermodalpopup");
            ImGui::SetNextWindowSize(m_Shared->Get3DSceneDims());
            ImGui::SetNextWindowPos(m_Shared->Get3DSceneRect().p1);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});

            ImGuiWindowFlags const modalFlags =
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize;

            if (ImGui::BeginPopupModal("##visualizermodalpopup", nullptr, modalFlags))
            {
                ImGui::PopStyleVar();
                ptr->draw();
                ImGui::EndPopup();
            }
            else
            {
                ImGui::PopStyleVar();
            }
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("wizard_3dViewer"))
            {
                ImGui::PopStyleVar();
                Draw3DViewer();
                ImGui::SetCursorPos(glm::vec2{ImGui::GetCursorStartPos()} + glm::vec2{10.0f, 10.0f});
                Draw3DViewerOverlay();
            }
            else
            {
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
    }

    // tab data
    UID m_TabID;
    std::weak_ptr<MainUIStateAPI> m_Parent;
    std::string m_Name = "MeshImporterTab";

    // data shared between states
    std::shared_ptr<SharedData> m_Shared;

    // buffer that's filled with drawable geometry during a drawcall
    std::vector<DrawableThing> m_DrawablesBuffer;

    // (maybe) hover + worldspace location of the hover
    Hover m_MaybeHover;

    // (maybe) the scene element that the user opened a context menu for
    Hover m_MaybeOpenedContextMenu;

    // (maybe) the next state the host screen should transition to
    std::shared_ptr<Layer> m_Maybe3DViewerModal;

    // ImGuizmo state
    struct {
        bool wasUsingLastFrame = false;
        glm::mat4 mtx{1.0f};
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mode = ImGuizmo::WORLD;
    } m_ImGuizmoState;
};


// public API (PIMPL)

osc::MeshImporterTab::MeshImporterTab(
    std::weak_ptr<MainUIStateAPI> parent_) :

    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::MeshImporterTab::MeshImporterTab(
    std::weak_ptr<MainUIStateAPI> parent_,
    std::vector<std::filesystem::path> files_) :

    m_Impl{std::make_unique<Impl>(std::move(parent_), std::move(files_))}
{
}

osc::MeshImporterTab::MeshImporterTab(MeshImporterTab&&) noexcept = default;
osc::MeshImporterTab& osc::MeshImporterTab::operator=(MeshImporterTab&&) noexcept = default;
osc::MeshImporterTab::~MeshImporterTab() noexcept = default;

osc::UID osc::MeshImporterTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MeshImporterTab::implGetName() const
{
    return m_Impl->getName();
}

bool osc::MeshImporterTab::implIsUnsaved() const
{
    return m_Impl->isUnsaved();
}

bool osc::MeshImporterTab::implTrySave()
{
    return m_Impl->trySave();
}

void osc::MeshImporterTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::MeshImporterTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::MeshImporterTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::MeshImporterTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::MeshImporterTab::implOnDrawMainMenu()
{
    m_Impl->drawMainMenu();
}

void osc::MeshImporterTab::implOnDraw()
{
    m_Impl->onDraw();
}
