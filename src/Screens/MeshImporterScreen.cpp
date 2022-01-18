#include "MeshImporterScreen.hpp"

#include "src/3D/Shaders/EdgeDetectionShader.hpp"
#include "src/3D/Shaders/GouraudShader.hpp"
#include "src/3D/Shaders/SolidColorShader.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/Texturing.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Screens/ModelEditorScreen.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/UI/MainMenu.hpp"
#include "src/UI/LogViewer.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ClonePtr.hpp"
#include "src/Utils/DefaultConstructOnCopy.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/Spsc.hpp"
#include "src/Utils/UID.hpp"
#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/MainEditorState.hpp"
#include "src/Styling.hpp"
#include "src/os.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
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
#include <stdexcept>
#include <string.h>
#include <string>
#include <unordered_set>
#include <vector>
#include <variant>

using namespace osc;

// user-facing string constants
namespace
{
    #define OSC_GROUND_DESC "Ground is an inertial reference frame in which the motion of all frames and points may conveniently and efficiently be expressed. It is always defined to be at (0, 0, 0) in 'worldspace' and cannot move. All bodies in the model must eventually attach to ground via joints."
    #define OSC_BODY_DESC "Bodies are active elements in the model. They define a 'frame' (effectively, a location + orientation) with a mass.\n\nOther body properties (e.g. inertia) can be edited in the main OpenSim Creator editor after you have converted the model into an OpenSim model."
    #define OSC_MESH_DESC "Meshes are decorational components in the model. They can be translated, rotated, and scaled. Typically, meshes are 'attached' to other elements in the model, such as bodies. When meshes are 'attached' to something, they will 'follow' the thing they are attached to."
    #define OSC_JOINT_DESC "Joints connect two physical frames (i.e. bodies and ground) together and specifies their relative permissible motion (e.g. PinJoints only allow rotation along one axis).\n\nIn OpenSim, joints are the 'edges' of a directed topology graph where bodies are the 'nodes'. All bodies in the model must ultimately connect to ground via joints."
    #define OSC_STATION_DESC "Stations are points of interest in the model. They can be used to compute a 3D location in the frame of the thing they are attached to.\n\nThe utility of stations is that you can use them to visually mark points of interest. Those points of interest will then be defined with respect to whatever they are attached to. This is useful because OpenSim typically requires relative coordinates for things in the model (e.g. muscle paths)."

    #define OSC_TRANSLATION_DESC  "Translation of the component in ground. OpenSim defines this as 'unitless'; however, OpenSim models typically use meters."
    #define OSC_FLOAT_INPUT_FORMAT "%.4f"

    std::string const g_GroundLabel = "Ground";
    std::string const g_GroundLabelPluralized = "Grounds";
    std::string const g_GroundLabelOptionallyPluralized = "Ground(s)";
    std::string const g_GroundDescription = OSC_GROUND_DESC;

    std::string const g_MeshLabel = "Mesh";
    std::string const g_MeshLabelPluralized = "Meshes";
    std::string const g_MeshLabelOptionallyPluralized = "Mesh(es)";
    std::string const g_MeshDescription = OSC_MESH_DESC;
    std::string const g_MeshAttachmentCrossrefName = "parent";

    std::string const g_BodyLabel = "Body";
    std::string const g_BodyLabelPluralized = "Bodies";
    std::string const g_BodyLabelOptionallyPluralized = "Body(s)";
    std::string const g_BodyDescription = OSC_BODY_DESC;

    std::string const g_JointLabel = "Joint";
    std::string const g_JointLabelPluralized = "Joints";
    std::string const g_JointLabelOptionallyPluralized = "Joint(s)";
    std::string const g_JointDescription = OSC_JOINT_DESC;
    std::string const g_JointParentCrossrefName = "parent";
    std::string const g_JointChildCrossrefName = "child";

    std::string const g_StationLabel = "Station";
    std::string const g_StationLabelPluralized = "Stations";
    std::string const g_StationLabelOptionallyPluralized = "Station(s)";
    std::string const g_StationDescription = OSC_STATION_DESC;
    std::string const g_StationParentCrossrefName = "parent";
}

// senteniel UID constants
namespace
{
    class BodyEl;
    UIDT<BodyEl> const g_GroundID = GenerateIDT<BodyEl>();
    UID const g_EmptyID = GenerateID();
    UID const g_RightClickedNothingID = GenerateID();
    UID const g_GroundGroupID = GenerateID();
    UID const g_MeshGroupID = GenerateID();
    UID const g_BodyGroupID = GenerateID();
    UID const g_JointGroupID = GenerateID();
    UID const g_StationGroupID = GenerateID();
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

        return powf(2.0f, -5.0f*x) * sinf((x*10.0f - 0.75f) * c4) + 1.0f;
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
    Transform RotateAxis(Transform const& t, int axis, float angRadians)
    {
        glm::vec3 ax{};
        ax[axis] = 1.0f;
        ax = t.rotation * ax;

        return t.withRotation(glm::angleAxis(angRadians, ax) * t.rotation);
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

    // see: https://stackoverflow.com/questions/56466282/stop-compilation-if-if-constexpr-does-not-match
    template <auto A, typename...> auto dependent_value = A;
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
    // the "parent" thing that is hosting the layer
    class LayerHost {
    public:
        virtual ~LayerHost() noexcept = default;
        virtual void pop() = 0;
    };

    // a layer that is hosted by the parent
    class Layer {
    public:
        Layer(LayerHost& parent) : m_Parent{parent} {}
        virtual ~Layer() noexcept = default;

        virtual bool onEvent(SDL_Event const&) = 0;
        virtual void tick(float) = 0;
        virtual void draw() = 0;

    protected:
        LayerHost& m_Parent;
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
        return dt.mesh->getWorldspaceAABB(dt.modelMatrix);
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
            RboBinding{GL_DEPTH_STENCIL_ATTACHMENT, sceneDepth24Stencil8RBO}
        );

        gl::Viewport(0, 0, dims.x, dims.y);

        gl::BindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        gl::ClearColor(bgCol);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the scene to the scene FBO
        if (true)
        {
            GouraudShader& shader = App::cur().getShaderCache().getShader<GouraudShader>();

            gl::UseProgram(shader.program);
            gl::Uniform(shader.uProjMat, projMat);
            gl::Uniform(shader.uViewMat, viewMat);
            gl::Uniform(shader.uLightDir, lightDir);
            gl::Uniform(shader.uLightColor, lightCol);
            gl::Uniform(shader.uViewPos, viewPos);
            for (DrawableThing const& d : drawables)
            {
                gl::Uniform(shader.uModelMat, d.modelMatrix);
                gl::Uniform(shader.uNormalMat, d.normalMatrix);
                gl::Uniform(shader.uDiffuseColor, d.color);
                if (d.maybeDiffuseTex)
                {
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(*d.maybeDiffuseTex);
                    gl::Uniform(shader.uIsTextured, true);
                    gl::Uniform(shader.uSampler0, GL_TEXTURE0-GL_TEXTURE0);
                }
                else
                {
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
        {
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
            for (DrawableThing const& d : drawables)
            {
                if (d.rimColor <= 0.05f)
                {
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
            gl::Uniform(eds.uRimRgba,  glm::vec4{0.8f, 0.5f, 0.3f, 0.8f});
            gl::Uniform(eds.uRimThickness, 1.75f / VecLongestDimVal(dims));
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
        UID PreferredAttachmentPoint;
        std::vector<std::filesystem::path> Paths;
    };

    // a successfully-loaded mesh
    struct LoadedMesh final {
        std::filesystem::path Path;
        std::shared_ptr<Mesh> MeshData;
    };

    // an OK response to a mesh loading request
    struct MeshLoadOKResponse final {
        UID PreferredAttachmentPoint;
        std::vector<LoadedMesh> Meshes;
    };

    // an ERROR response to a mesh loading request
    struct MeshLoadErrorResponse final {
        UID PreferredAttachmentPoint;
        std::filesystem::path Path;
        std::string Error;
    };

    // an OK or ERROR response to a mesh loading request
    using MeshLoadResponse = std::variant<MeshLoadOKResponse, MeshLoadErrorResponse>;

    // returns an OK or ERROR response to a mesh load request
    MeshLoadResponse respondToMeshloadRequest(MeshLoadRequest msg)
    {
        std::vector<LoadedMesh> loadedMeshes;
        loadedMeshes.reserve(msg.Paths.size());

        for (std::filesystem::path const& path : msg.Paths)
        {
            try
            {
                std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(SimTKLoadMesh(path));
                loadedMeshes.push_back(LoadedMesh{path, std::move(mesh)});
            }
            catch (std::exception const& ex)
            {
                return MeshLoadErrorResponse{msg.PreferredAttachmentPoint, path, ex.what()};
            }
        }
        App::cur().requestRedraw();  // TODO: HACK: try to make the UI thread redraw around the time this is sent
        return MeshLoadOKResponse{msg.PreferredAttachmentPoint, std::move(loadedMeshes)};
    }

    // a class that loads meshes in a background thread
    //
    // the UI thread must `.poll()` this to check for responses
    class MeshLoader final {
        using Worker = spsc::Worker<MeshLoadRequest, MeshLoadResponse, decltype(respondToMeshloadRequest)>;

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
        SceneElClass(std::string name,
                     std::string namePluralized,
                     std::string nameOptionallyPluralized,
                     std::string icon,
                     std::string description,
                     std::unique_ptr<SceneEl> defaultObject) :
            m_ID{GenerateID()},
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

        int FetchAddUniqueCounter() const
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
        mutable std::atomic<uint32_t> m_UniqueCounter;
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
    public:
        virtual ~SceneEl() noexcept = default;

        virtual SceneElClass const& GetClass() const = 0;

        // allow runtime cloning of a particular instance
        virtual std::unique_ptr<SceneEl> clone() const = 0;

        // accept visitors so that downstream code can use visitors when they need to
        // handle specific types
        virtual void Accept(ConstSceneElVisitor&) const = 0;
        virtual void Accept(SceneElVisitor&) = 0;

        // each scene element may be referencing `n` (>= 0) other scene elements by
        // ID. These methods allow implementations to ask what and how
        virtual int GetNumCrossReferences() const
        {
            return 0;
        }
        virtual UID GetCrossReferenceConnecteeID(int) const
        {
            throw std::runtime_error{"cannot get cross reference ID: no method implemented"};
        }
        virtual void SetCrossReferenceConnecteeID(int, UID)
        {
            throw std::runtime_error{"cannot set cross reference ID: no method implemented"};
        }
        virtual std::string const& GetCrossReferenceLabel(int) const
        {
            throw std::runtime_error{"cannot get cross reference label: no method implemented"};
        }
        virtual CrossrefDirection GetCrossReferenceDirection(int) const
        {
            return CrossrefDirection_ToParent;
        }

        virtual SceneElFlags GetFlags() const = 0;

        virtual UID GetID() const = 0;
        virtual std::ostream& operator<<(std::ostream&) const = 0;

        virtual std::string const& GetLabel() const = 0;
        virtual void SetLabel(std::string_view) = 0;

        virtual Transform GetXform() const = 0;
        virtual void SetXform(Transform const&) = 0;

        virtual AABB CalcBounds() const = 0;

        // helper methods (virtual member funcs)
        //
        // these position/scale/rotation methods are here as member virtual functions
        // because downstream classes may only actually hold a subset of a full
        // transform (e.g. only position). There is a perf advantage to only returning
        // what was asked for.

        virtual glm::vec3 GetPos() const
        {
            return GetXform().position;
        }
        virtual void SetPos(glm::vec3 const& newPos)
        {
            Transform t = GetXform();
            t.position = newPos;
            SetXform(t);
        }

        virtual glm::vec3 GetScale() const
        {
            return GetXform().scale;
        }
        virtual void SetScale(glm::vec3 const& newScale)
        {
            Transform t = GetXform();
            t.scale = newScale;
            SetXform(t);
        }

        virtual glm::quat GetRotation() const
        {
            return GetXform().rotation;
        }
        virtual void SetRotation(glm::quat const& newRotation)
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
        applyWorldspaceRotation(t, eulerAngles, rotationCenter);
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
            static SceneElClass g_Class =
            {
                g_GroundLabel,
                g_GroundLabelPluralized,
                g_GroundLabelOptionallyPluralized,
                ICON_FA_DOT_CIRCLE,
                g_GroundDescription,
                std::unique_ptr<SceneEl>{new GroundEl{}}
            };

            return g_Class;
        }

        SceneElClass const& GetClass() const override
        {
            return Class();
        }

        std::unique_ptr<SceneEl> clone() const override
        {
            return std::make_unique<GroundEl>(*this);
        }

        void Accept(ConstSceneElVisitor& visitor) const override
        {
            visitor(*this);
        }

        void Accept(SceneElVisitor& visitor) override
        {
            visitor(*this);
        }

        SceneElFlags GetFlags() const override
        {
            return SceneElFlags_None;
        }

        UID GetID() const override
        {
            return g_GroundID;
        }

        std::ostream& operator<<(std::ostream& o) const override
        {
            return o << g_GroundLabel << "()";
        }

        std::string const& GetLabel() const override
        {
            return g_GroundLabel;
        }

        void SetLabel(std::string_view) override
        {
            // ignore: cannot set ground's name
        }

        Transform GetXform() const override
        {
            return Transform{};
        }

        void SetXform(Transform const&) override
        {
            // ignore: cannot change ground's xform
        }

        AABB CalcBounds() const override
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
    class BodyEl;
    class MeshEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass g_Class =
            {
                g_MeshLabel,
                g_MeshLabelPluralized,
                g_MeshLabelOptionallyPluralized,
                ICON_FA_CUBE,
                g_MeshDescription,
                std::unique_ptr<SceneEl>{new MeshEl{}}
            };

            return g_Class;
        }

        MeshEl() :
            ID{GenerateIDT<MeshEl>()},
            Attachment{GenerateIDT<BodyEl>()},
            MeshData{nullptr},
            Path{"invalid"}
        {
            // default ctor for prototype storage
        }

        MeshEl(UIDT<MeshEl> id,
               UIDT<BodyEl> attachment,  // can be g_GroundID
               std::shared_ptr<Mesh> meshData,
               std::filesystem::path const& path) :

            ID{std::move(id)},
            Attachment{std::move(attachment)},
            MeshData{std::move(meshData)},
            Path{path}
        {
        }

        MeshEl(UIDT<BodyEl> attachment,  // can be g_GroundID
               std::shared_ptr<Mesh> meshData,
               std::filesystem::path const& path) :

            MeshEl{GenerateIDT<MeshEl>(),
                   std::move(attachment),
                   std::move(meshData),
                   path}
        {
        }

        SceneElClass const& GetClass() const override
        {
            return Class();
        }

        std::unique_ptr<SceneEl> clone() const override
        {
            return std::make_unique<MeshEl>(*this);
        }

        void Accept(ConstSceneElVisitor& visitor) const override
        {
            visitor(*this);
        }

        void Accept(SceneElVisitor& visitor) override
        {
            visitor(*this);
        }

        int GetNumCrossReferences() const override
        {
            return 1;
        }

        UID GetCrossReferenceConnecteeID(int i) const override
        {
            switch (i) {
            case 0:
                return Attachment;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        void SetCrossReferenceConnecteeID(int i, UID id) override
        {
            switch (i) {
            case 0:
                Attachment = DowncastID<BodyEl>(id);
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        std::string const& GetCrossReferenceLabel(int i) const override
        {
            switch (i) {
            case 0:
                return g_MeshAttachmentCrossrefName;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        SceneElFlags GetFlags() const override
        {
            return SceneElFlags_CanChangeLabel |
                    SceneElFlags_CanChangePosition |
                    SceneElFlags_CanChangeRotation |
                    SceneElFlags_CanChangeScale |
                    SceneElFlags_CanDelete |
                    SceneElFlags_CanSelect |
                    SceneElFlags_HasPhysicalSize;
        }

        UID GetID() const override
        {
            return ID;
        }

        std::ostream& operator<<(std::ostream& o) const override
        {
            return o << "MeshEl("
                     << "ID = " << ID
                     << ", Attachment = " << Attachment
                     << ", Xform = " << Xform
                     << ", MeshData = " << MeshData.get()
                     << ", Path = " << Path
                     << ", Name = " << Name
                     << ')';
        }

        std::string const& GetLabel() const override
        {
            return Name;
        }

        void SetLabel(std::string_view sv) override
        {
            Name = std::move(sv);
        }

        Transform GetXform() const override
        {
            return Xform;
        }

        void SetXform(Transform const& t) override
        {
            Xform = std::move(t);
        }

        AABB CalcBounds() const override
        {
            return MeshData->getWorldspaceAABB(Xform);
        }

        UIDT<MeshEl> ID;
        UIDT<BodyEl> Attachment;  // can be g_GroundID
        Transform Xform;
        std::shared_ptr<Mesh> MeshData;
        std::filesystem::path Path;
        std::string Name{FileNameWithoutExtension(Path)};
    };

    // a body scene element
    //
    // In this mesh importer, bodies are positioned + oriented in ground (see MeshEl for explanation of why).
    class BodyEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass g_Class =
            {
                g_BodyLabel,
                g_BodyLabelPluralized,
                g_BodyLabelOptionallyPluralized,
                ICON_FA_CIRCLE,
                g_BodyDescription,
                std::unique_ptr<SceneEl>{new BodyEl{}}
            };

            return g_Class;
        }

        BodyEl() :
            ID{GenerateIDT<BodyEl>()},
            Name{"prototype"},
            Xform{}
        {
            // default ctor for prototype storage
        }

        BodyEl(UIDT<BodyEl> id, std::string const& name, Transform const& xform) :
            ID{id},
            Name{name},
            Xform{xform}
        {
        }

        BodyEl(std::string const& name, Transform const& xform) :
            BodyEl{GenerateIDT<BodyEl>(), name, xform}
        {
        }

        explicit BodyEl(Transform const& xform) :
            BodyEl{GenerateIDT<BodyEl>(), GenerateName(Class()), xform}
        {
        }

        SceneElClass const& GetClass() const override
        {
            return Class();
        }

        std::unique_ptr<SceneEl> clone() const override
        {
            return std::make_unique<BodyEl>(*this);
        }

        void Accept(ConstSceneElVisitor& visitor) const override
        {
            visitor(*this);
        }

        void Accept(SceneElVisitor& visitor) override
        {
            visitor(*this);
        }

        SceneElFlags GetFlags() const override
        {
            return SceneElFlags_CanChangeLabel |
                    SceneElFlags_CanChangePosition |
                    SceneElFlags_CanChangeRotation |
                    SceneElFlags_CanDelete |
                    SceneElFlags_CanSelect;
        }

        UID GetID() const override
        {
            return ID;
        }

        std::ostream& operator<<(std::ostream& o) const override
        {
            return o << "BodyEl(ID = " << ID
                     << ", Name = " << Name
                     << ", Xform = " << Xform
                     << ", Mass = " << Mass
                     << ')';
        }

        std::string const& GetLabel() const override
        {
            return Name;
        }

        void SetLabel(std::string_view sv) override
        {
            Name = std::move(sv);
        }

        Transform GetXform() const override
        {
            return Xform;
        }

        void SetXform(Transform const& newXform) override
        {
            Xform = newXform;
            Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        void SetScale(glm::vec3 const&) override
        {
            // ignore: scaling a body, which is a point, does nothing
        }

        AABB CalcBounds() const override
        {
            return AABB{Xform.position, Xform.position};
        }

        UIDT<BodyEl> ID;
        std::string Name;
        Transform Xform;
        double Mass{1.0f};  // OpenSim goes bananas if a body has a mass <= 0
    };

    // a joint scene element
    //
    // see `JointAttachment` comment for an explanation of why it's designed this way.
    class JointEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass g_Class =
            {
                g_JointLabel,
                g_JointLabelPluralized,
                g_JointLabelOptionallyPluralized,
                ICON_FA_LINK,
                g_JointDescription,
                std::unique_ptr<SceneEl>{new JointEl{}}
            };

            return g_Class;
        }

        JointEl() :
            ID{GenerateIDT<JointEl>()},
            JointTypeIndex{0},
            UserAssignedName{"prototype"},
            Parent{GenerateID()},
            Child{GenerateIDT<BodyEl>()},
            Xform{}
        {
            // default ctor for prototype allocation
        }

        JointEl(UIDT<JointEl> id,
                size_t jointTypeIdx,
                std::string userAssignedName,  // can be empty
                UID parent,
                UIDT<BodyEl> child,
                Transform const& xform) :

            ID{std::move(id)},
            JointTypeIndex{std::move(jointTypeIdx)},
            UserAssignedName{std::move(userAssignedName)},
            Parent{std::move(parent)},
            Child{std::move(child)},
            Xform{std::move(xform)}
        {
        }

        JointEl(size_t jointTypeIdx,
                std::string userAssignedName,  // can be empty
                UID parent,
                UIDT<BodyEl> child,
                Transform const& xform) :
            JointEl{
                GenerateIDT<JointEl>(),
                std::move(jointTypeIdx),
                std::move(userAssignedName),
                std::move(parent),
                std::move(child),
                xform}
        {
        }

        SceneElClass const& GetClass() const override
        {
            return Class();
        }

        std::unique_ptr<SceneEl> clone() const override
        {
            return std::make_unique<JointEl>(*this);
        }

        void Accept(ConstSceneElVisitor& visitor) const override
        {
            visitor(*this);
        }

        void Accept(SceneElVisitor& visitor) override
        {
            visitor(*this);
        }

        int GetNumCrossReferences() const override
        {
            return 2;
        }

        UID GetCrossReferenceConnecteeID(int i) const override
        {
            switch (i) {
            case 0:
                return Parent;
            case 1:
                return Child;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        void SetCrossReferenceConnecteeID(int i, UID id) override
        {
            switch (i) {
            case 0:
                Parent = id;
                break;
            case 1:
                Child = DowncastID<BodyEl>(id);
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        std::string const& GetCrossReferenceLabel(int i) const override
        {
            switch (i) {
            case 0:
                return g_JointParentCrossrefName;
            case 1:
                return g_JointChildCrossrefName;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        CrossrefDirection GetCrossReferenceDirection(int i) const override
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

        SceneElFlags GetFlags() const override
        {
            return SceneElFlags_CanChangeLabel |
                    SceneElFlags_CanChangePosition |
                    SceneElFlags_CanChangeRotation |
                    SceneElFlags_CanDelete |
                    SceneElFlags_CanSelect;
        }

        UID GetID() const override
        {
            return ID;
        }

        std::ostream& operator<<(std::ostream& o) const override
        {
            return o << "JointEl(ID = " << ID
                     << ", JointTypeIndex = " << JointTypeIndex
                     << ", UserAssignedName = " << UserAssignedName
                     << ", Parent = " << Parent
                     << ", Child = " << Child
                     << ", Xform = " << Xform
                     << ')';
        }

        std::string const& GetSpecificTypeName() const
        {
             return JointRegistry::nameStrings()[JointTypeIndex];
        }

        std::string const& GetLabel() const override
        {
            return UserAssignedName.empty() ? GetSpecificTypeName() : UserAssignedName;
        }

        void SetLabel(std::string_view sv) override
        {
            UserAssignedName = std::move(sv);
        }

        Transform GetXform() const override
        {
            return Xform;
        }

        void SetXform(Transform const& t) override
        {
            Xform = std::move(t);
            Xform.scale = {1.0f, 1.0f, 1.0f};
        }

        void SetScale(glm::vec3 const&) override {}  // ignore

        AABB CalcBounds() const override
        {
            return AABB{Xform.position, Xform.position};
        }

        bool IsAttachedTo(BodyEl const& b) const
        {
            return Parent == b.ID || Child == b.ID;
        }

        UIDT<JointEl> ID;
        size_t JointTypeIndex;
        std::string UserAssignedName;
        UID Parent;  // can be ground
        UIDT<BodyEl> Child;
        Transform Xform;  // joint center
    };


    // a station (point of interest)
    class BodyEl;
    class StationEl final : public SceneEl {
    public:
        static SceneElClass const& Class()
        {
            static SceneElClass g_Class =
            {
                g_StationLabel,
                g_StationLabelPluralized,
                g_StationLabelOptionallyPluralized,
                ICON_FA_MAP_PIN,
                g_StationDescription,
                std::unique_ptr<SceneEl>{new StationEl{}}
            };

            return g_Class;
        }

        StationEl() :
            ID{GenerateIDT<StationEl>()},
            Attachment{GenerateIDT<BodyEl>()},
            Position{},
            Name{"prototype"}
        {
            // default ctor for prototype allocation
        }

        StationEl(UIDT<StationEl> id,
                  UIDT<BodyEl> attachment,  // can be g_GroundID
                  glm::vec3 const& position,
                  std::string name) :
            ID{std::move(id)},
            Attachment{std::move(attachment)},
            Position{std::move(position)},
            Name{std::move(name)}
        {
        }

        SceneElClass const& GetClass() const override
        {
            return Class();
        }

        std::unique_ptr<SceneEl> clone() const override
        {
            return std::make_unique<StationEl>(*this);
        }

        void Accept(ConstSceneElVisitor& visitor) const override
        {
            visitor(*this);
        }

        void Accept(SceneElVisitor& visitor) override
        {
            visitor(*this);
        }

        int GetNumCrossReferences() const override
        {
            return 1;
        }

        UID GetCrossReferenceConnecteeID(int i) const override
        {
            switch (i) {
            case 0:
                return Attachment;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        void SetCrossReferenceConnecteeID(int i, UID id) override
        {
            switch (i) {
            case 0:
                Attachment = DowncastID<BodyEl>(id);
                break;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        std::string const& GetCrossReferenceLabel(int i) const override
        {
            switch (i) {
            case 0:
                return g_StationParentCrossrefName;
            default:
                throw std::runtime_error{"invalid index accessed for cross reference"};
            }
        }

        SceneElFlags GetFlags() const override
        {
            return SceneElFlags_CanChangeLabel |
                    SceneElFlags_CanChangePosition |
                    SceneElFlags_CanDelete |
                    SceneElFlags_CanSelect;
        }

        UID GetID() const override
        {
            return ID;
        }

        std::ostream& operator<<(std::ostream& o) const override
        {
            using osc::operator<<;

            return o << "StationEl("
                     << "ID = " << ID
                     << ", Attachment = " << Attachment
                     << ", Position = " << Position
                     << ", Name = " << Name
                     << ')';
        }

        std::string const& GetLabel() const override
        {
            return Name;
        }

        void SetLabel(std::string_view sv) override
        {
            Name = std::move(sv);
        }

        Transform GetXform() const override
        {
            return Transform::atPosition(Position);
        }

        void SetXform(Transform const& t) override
        {
            Position = t.position;
        }

        AABB CalcBounds() const override
        {
            return AABB{Position, Position};
        }

        UIDT<StationEl> ID;
        UIDT<BodyEl> Attachment;  // can be g_GroundID
        glm::vec3 Position;
        std::string Name;
    };


    // returns true if a mesh can be attached to the given element
    bool CanAttachMeshTo(SceneEl const& e)
    {
        struct Visitor final : public ConstSceneElVisitor
        {
            bool m_Result = false;

            void operator()(GroundEl const&) override { m_Result = true; }
            void operator()(MeshEl const&) override { m_Result = false; }
            void operator()(BodyEl const&) override { m_Result = true; }
            void operator()(JointEl const&) override { m_Result = false; }
            void operator()(StationEl const&) override { m_Result = false; }
        };

        Visitor v;
        e.Accept(v);
        return v.m_Result;
    }

    // returns `true` if a `StationEl` can be attached to the element
    bool CanAttachStationTo(SceneEl const& e)
    {
        struct Visitor final : public ConstSceneElVisitor
        {
            bool m_Result = false;

            void operator()(GroundEl const&) override { m_Result = true; }
            void operator()(MeshEl const&) override { m_Result = true; }
            void operator()(BodyEl const&) override { m_Result = true; }
            void operator()(JointEl const&) override { m_Result = false; }
            void operator()(StationEl const&) override { m_Result = false; }
        };

        Visitor v;
        e.Accept(v);
        return v.m_Result;
    }

    // returns the ID of the thing the station should attach to when trying to
    // attach to something in the scene
    UIDT<BodyEl> StationAttachmentParent(SceneEl const& el)
    {
        struct Visitor final : public ConstSceneElVisitor
        {
            UIDT<BodyEl> m_Result = g_GroundID;

            void operator()(GroundEl const&) { m_Result = g_GroundID; }
            void operator()(MeshEl const& el) { m_Result = el.Attachment; }
            void operator()(BodyEl const& el) { m_Result = el.ID; }
            void operator()(JointEl const&) { m_Result = g_GroundID; }  // can't be attached
            void operator()(StationEl const&) { m_Result = g_GroundID; }  // can't be attached
        };

        Visitor v;
        el.Accept(v);
        return v.m_Result;
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

    std::vector<SceneElClass const*> GetSceneElClasses()
    {
        static std::vector<SceneElClass const*> g_Classes = GenerateSceneElClassList();

        return g_Classes;
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
        class Iterator {
        public:
            Iterator(std::map<UID, ClonePtr<SceneEl>>::iterator pos,
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

            template<bool _IsConst = IsConst>
            operator typename std::enable_if_t<!_IsConst, Iterator<true, T>>() const noexcept
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

            template<bool _IsConst = IsConst>
            typename std::enable_if_t<_IsConst, T const&> operator*() const noexcept
            {
                return dynamic_cast<T const&>(*m_Pos->second);
            }

            template<bool _IsConst = IsConst>
            typename std::enable_if_t<!_IsConst, T&> operator*() const noexcept
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

            template<bool _IsConst = IsConst>
            typename std::enable_if_t<_IsConst, T const*> operator->() const noexcept
            {

                return &dynamic_cast<T const&>(*m_Pos->second);
            }

            template<bool _IsConst = IsConst>
            typename std::enable_if_t<!_IsConst, T*> operator->() const noexcept
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
            m_Els{{g_GroundID, ClonePtr<SceneEl>{GroundEl{}}}}
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
        DefaultConstructOnCopy<std::vector<ClonePtr<SceneEl>>> m_DeletedEls;
    };

    // returns true if the mesh el has been assigned to a body that exists in the model graph
    bool IsAssignedToBody(ModelGraph const& mg, MeshEl const& mesh)
    {
        return mg.ContainsEl<BodyEl>(mesh.Attachment);
    }

    void SelectOnly(ModelGraph& mg, UID id)
    {
        mg.DeSelectAll();
        mg.Select(id);
    }

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

    std::string const& GetLabel(ModelGraph const& mg, UID id)
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

    glm::quat GetRotation(ModelGraph const& mg, UID id)
    {
        return mg.GetElByID(id).GetRotation();
    }

    // returns `true` if `body` participates in any joint in the model graph
    bool IsAChildAttachmentInAnyJoint(ModelGraph const& mg, SceneEl const& el)
    {
        UID id = el.GetID();
        for (JointEl const& j : mg.iter<JointEl>())
        {
            if (j.Child == id)
            {
                return true;
            }
        }
        return false;
    }

    // returns `true` if a Joint is complete b.s.
    bool IsGarbageJoint(ModelGraph const& modelGraph, JointEl const& jointEl)
    {
        if (jointEl.Child == g_GroundID)
        {
            return true;  // ground cannot be a child in a joint
        }

        if (jointEl.Parent == jointEl.Child)
        {
            return true;  // is directly attached to itself
        }

        if (jointEl.Parent != g_GroundID && !modelGraph.ContainsEl<BodyEl>(jointEl.Parent))
        {
             return true;  // has a parent ID that's invalid for this model graph
        }

        if (!modelGraph.ContainsEl<BodyEl>(jointEl.Child))
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

        if (joint.Parent == g_GroundID)
        {
            return true;  // it's directly attached to ground
        }

        BodyEl const* parent = modelGraph.TryGetElByID<BodyEl>(joint.Parent);

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

            if (jointEl.Child == body.ID)
            {
                childInAtLeastOneJoint = true;

                bool alreadyVisited = !previouslyVisitedJoints.emplace(jointEl.ID).second;
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
                ss << body.Name << ": body is not attached to ground: it is connected by a joint that, itself, does not connect to ground";
                issuesOut.push_back(std::move(ss).str());
            }
        }

        return !issuesOut.empty();
    }

    // returns a string representing the subheader of a scene element
    std::string GetContextMenuSubHeaderText(ModelGraph const& mg, SceneEl const& e)
    {
        struct Visitor final : public ConstSceneElVisitor
        {
            std::stringstream& m_SS;
            ModelGraph const& m_Mg;

            Visitor(std::stringstream& ss, ModelGraph const& mg) : m_SS{ss}, m_Mg{mg} {}

            void operator()(GroundEl const&) override
            {
                m_SS << "(scene origin)";
            }
            void operator()(MeshEl const& m) override
            {
                m_SS << '(' << m.GetClass().GetNameSV() << ", " << m.Path.filename().string() << ", attached to " << GetLabel(m_Mg, m.Attachment) << ')';
            }
            void operator()(BodyEl const& b) override
            {
                m_SS << '(' << b.GetClass().GetNameSV() << ')';
            }
            void operator()(JointEl const& j) override
            {
                m_SS << '(' << j.GetSpecificTypeName() << ", " << GetLabel(m_Mg, j.Child) << " --> " << GetLabel(m_Mg, j.Parent) << ')';
            }
            void operator()(StationEl const& s) override
            {
                m_SS << '(' << s.GetClass().GetNameSV() << ", attached to " << GetLabel(m_Mg, s.Attachment) << ')';
            }
        };

        std::stringstream ss;
        Visitor v{ss, mg};
        e.Accept(v);
        return std::move(ss).str();
    }

    // returns true if the given element (ID) is in the "selection group" of
    bool IsInSelectionGroupOf(ModelGraph const& mg, UID parent, UID id)
    {
        if (id == g_EmptyID || parent == g_EmptyID)
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
            bodyEl = mg.TryGetElByID<BodyEl>(me->Attachment);
        }

        if (!bodyEl)
        {
            return false;  // parent isn't attached to any body (or isn't a body)
        }

        if (BodyEl const* be = mg.TryGetElByID<BodyEl>(id))
        {
            return be->ID == bodyEl->ID;
        }
        else if (MeshEl const* me = mg.TryGetElByID<MeshEl>(id))
        {
            return me->Attachment == bodyEl->ID;
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

    // adds a body to a mesh at a given position
    BodyEl& AddBodyToMeshAtPosition(ModelGraph& mg, MeshEl const& mesh, glm::vec3 const& pos)
    {
        BodyEl& b = mg.AddEl<BodyEl>(Transform::atPosition(pos));
        SelectOnly(mg, b.ID);

        if (!IsAssignedToBody(mg, mesh))
        {
            mg.UpdElByID<MeshEl>(mesh.ID).Attachment = b.ID;
        }

        return b;
    }

    // points an axis of a given element towards some other element in the model graph
    void PointAxisTowards(ModelGraph& mg, UID id, int axis, UID other)
    {
        glm::vec3 choicePos = GetPosition(mg, other);
        Transform sourceXform = GetPosition(mg, id);

        mg.UpdElByID(id).SetXform(PointAxisTowards(sourceXform, axis, choicePos));
    }

    // returns recommended rim intensity for an element in the model graph
    float CalcRimIntensity(ModelGraph const& mg, UID id, UID hoverID = g_EmptyID)
    {
        if (id == g_EmptyID)
        {
            return 0.0f;
        }
        else if (mg.IsSelected(id))
        {
            return 1.0f;
        }
        else if (IsInSelectionGroupOf(mg, hoverID, id))
        {
            return 0.6f;
        }
        else
        {
            return 0.0f;
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
    class ModelGraphCommit {
    public:
        ModelGraphCommit(UID parentID,  // can be g_EmptyID
                         ClonePtr<ModelGraph> modelGraph,
                         std::string_view commitMessage) :

            m_ID{GenerateID()},
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
        CommittableModelGraph() :
            m_Scratch{new ModelGraph{}},
            m_Current{g_EmptyID},
            m_BranchHead{g_EmptyID},
            m_Commits{}
        {
            Commit("created model graph");
        }

        void Commit(std::string_view commitMsg)
        {
            auto snapshot = std::make_unique<ModelGraphCommit>(m_Current, ClonePtr<ModelGraph>{*m_Scratch}, commitMsg);
            UID id = snapshot->GetID();
            m_Commits.try_emplace(id, std::move(snapshot));
            m_Current = id;
            m_BranchHead = id;
        }

        ModelGraphCommit const* TryGetCommitByID(UID id) const
        {
            auto hasSameID = [id](auto const& p)
            {
                return p.second->GetID() == id;
            };

            auto it = FindIf(m_Commits, hasSameID);

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
            return c ? c->GetParentID() != g_EmptyID : false;
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

    bool AddBodyToMeshAtPosition(CommittableModelGraph& cmg, MeshEl const& mesh, glm::vec3 const& pos)
    {
        BodyEl& b = AddBodyToMeshAtPosition(cmg.UpdScratch(), mesh, pos);
        cmg.Commit("added " + b.GetLabel());
        return true;
    }

    bool TryAssignMeshAttachment(CommittableModelGraph& cmg, UID meshID, UID newAttachment)
    {
        ModelGraph& mg = cmg.UpdScratch();

        MeshEl* el = mg.TryUpdElByID<MeshEl>(meshID);

        if (!el)
        {
            return false;
        }

        // can only attach to itself (ground), ground, or a body that exists in the model graph
        if (!(newAttachment == meshID || newAttachment == g_GroundID || mg.TryUpdElByID<BodyEl>(newAttachment)))
        {
            return false;
        }

        if (newAttachment == meshID || newAttachment == g_GroundID)
        {
            el->Attachment = g_GroundID;
            cmg.Commit("assigned mesh to ground");
        }
        else
        {
            el->Attachment = DowncastID<BodyEl>(newAttachment);
            cmg.Commit("assigned mesh to body");
        }

        return true;
    }

    bool TryCreateJoint(CommittableModelGraph& cmg, UID childID, UID parentID)
    {
        ModelGraph& mg = cmg.UpdScratch();

        size_t freejointIdx = *JointRegistry::indexOf(OpenSim::FreeJoint{});
        glm::vec3 parentPos = GetPosition(mg, parentID);
        glm::vec3 childPos = GetPosition(mg, childID);
        glm::vec3 midPoint = VecMidpoint(parentPos, childPos);

        JointEl& jointEl = mg.AddEl<JointEl>(freejointIdx, "", parentID, DowncastID<BodyEl>(childID), Transform::atPosition(midPoint));
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

        el->SetPos(VecMidpoint(a, b));
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

        el->SetPos(VecMidpoint(aEl->GetPos(), bEl->GetPos()));
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

        std::string label = el->GetLabel();

        if (!mg.DeleteEl(*el))
        {
            return false;
        }

        cmg.Commit("deleted " + label);
        return true;
    }

    void ResetModelGraph(CommittableModelGraph& cmg)
    {
        cmg.UpdScratch() = ModelGraph{};
        cmg.Commit("created new scene");
    }

    void RotateAxis180Degrees(CommittableModelGraph& cmg, SceneEl& el, int axis)
    {
        el.SetXform(RotateAxis(el.GetXform(), axis, fpi));
        cmg.Commit("reoriented " + el.GetLabel());
    }
}

// OpenSim::Model generation support
//
// the ModelGraph that this UI manipulates ultimately needs to be transformed
// into a standard OpenSim model. This code does that.
namespace
{
    // attaches a mesh to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
    void AttachMeshElToFrame(MeshEl const& meshEl,
                             Transform const& parentXform,
                             OpenSim::PhysicalFrame& parentPhysFrame)
    {
        // create a POF that attaches to the body
        auto meshPhysOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysOffsetFrame->setParentFrame(parentPhysFrame);
        meshPhysOffsetFrame->setName(meshEl.Name + "_offset");

        // re-express the transform matrix in the parent's frame
        glm::mat4 mesh2parent = toInverseMat4(parentXform) * toMat4(meshEl.Xform);

        // set it as the transform
        meshPhysOffsetFrame->setOffsetTransform(SimTKTransformFromMat4x3(mesh2parent));

        // attach mesh to the POF
        auto mesh = std::make_unique<OpenSim::Mesh>(meshEl.Path.string());
        mesh->setName(meshEl.Name);
        mesh->set_scale_factors(SimTKVec3FromV3(meshEl.Xform.scale));
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

        for (MeshEl const& mesh : mg.iter<MeshEl>())
        {
            if (mesh.Attachment == bodyEl.ID)
            {
                AttachMeshElToFrame(mesh, bodyEl.Xform, *addedBody);
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
    std::string CalcJointName(JointEl const& jointEl,
                              OpenSim::PhysicalFrame const& parentFrame,
                              OpenSim::PhysicalFrame const& childFrame)
    {
        if (!jointEl.UserAssignedName.empty())
        {
            return jointEl.UserAssignedName;
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
        OpenSim::Joint const* proto = JointRegistry::prototypes()[jointTypeIdx].get();
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
        JointDegreesOfFreedom dofs = GetDegreesOfFreedom(joint.JointTypeIndex);

        glm::vec3 rv;
        for (int i = 0; i < 3; ++i)
        {
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
            bool wasInserted = visitedJoints.emplace(joint.ID).second;
            if (!wasInserted)
            {
                // graph cycle detected: joint was already previously visited and shouldn't be traversed again
                return;
            }
        }

        // lookup each side of the joint, creating the bodies if necessary
        JointAttachmentCachedLookupResult parent = LookupPhysFrame(mg, model, visitedBodies, joint.Parent);
        JointAttachmentCachedLookupResult child = LookupPhysFrame(mg, model, visitedBodies, joint.Child);

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parentPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parentPOF->setName(parent.physicalFrame->getName() + "_offset");
        parentPOF->setParentFrame(*parent.physicalFrame);
        glm::mat4 toParentPofInParent = toInverseMat4(GetTransform(mg, joint.Parent)) * toMat4(joint.Xform);
        parentPOF->set_translation(SimTKVec3FromV3(toParentPofInParent[3]));
        parentPOF->set_orientation(SimTKVec3FromV3(extractEulerAngleXYZ(toParentPofInParent)));

        // create the child OpenSim::PhysicalOffsetFrame
        auto childPOF = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        childPOF->setName(child.physicalFrame->getName() + "_offset");
        childPOF->setParentFrame(*child.physicalFrame);
        glm::mat4 toChildPofInChild = toInverseMat4(GetTransform(mg, joint.Child)) * toMat4(joint.Xform);
        childPOF->set_translation(SimTKVec3FromV3(toChildPofInChild[3]));
        childPOF->set_orientation(SimTKVec3FromV3(extractEulerAngleXYZ(toChildPofInChild)));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto jointUniqPtr = std::unique_ptr<OpenSim::Joint>(JointRegistry::prototypes()[joint.JointTypeIndex]->clone());

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
        if (child.createdBody)
        {
            model.addBody(child.createdBody.release());  // add created body to model
        }

        // add the joint to the model
        model.addJoint(jointUniqPtr.release());

        // recurse by finding where the child of this joint is the parent of some other joint
        OSC_ASSERT_ALWAYS(child.bodyEl != nullptr && "child should always be an identifiable body element");
        for (JointEl const& otherJoint : mg.iter<JointEl>())
        {
            if (otherJoint.Parent == child.bodyEl->ID)
            {
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
        glm::vec3 eulers = eulerAnglesXYZ(bodyEl.Xform);
        freeJoint->upd_coordinates(0).setDefaultValue(static_cast<double>(eulers[0]));
        freeJoint->upd_coordinates(1).setDefaultValue(static_cast<double>(eulers[1]));
        freeJoint->upd_coordinates(2).setDefaultValue(static_cast<double>(eulers[2]));
        freeJoint->upd_coordinates(3).setDefaultValue(static_cast<double>(bodyEl.Xform.position[0]));
        freeJoint->upd_coordinates(4).setDefaultValue(static_cast<double>(bodyEl.Xform.position[1]));
        freeJoint->upd_coordinates(5).setDefaultValue(static_cast<double>(bodyEl.Xform.position[2]));

        // connect joint from ground to the body
        freeJoint->connectSocket_parent_frame(model.getGround());
        freeJoint->connectSocket_child_frame(*addedBody);

        // populate it in the "already visited bodies" cache
        visitedBodies[bodyEl.ID] = addedBody.get();

        // add the body + joint to the output model
        model.addBody(addedBody.release());
        model.addJoint(freeJoint.release());
    }

    void AddStationToModel(ModelGraph const& mg,
                           OpenSim::Model& model,
                           StationEl const& stationEl,
                           std::unordered_map<UID, OpenSim::Body*>& visitedBodies)
    {

        JointAttachmentCachedLookupResult res = LookupPhysFrame(mg, model, visitedBodies, stationEl.Attachment);
        OSC_ASSERT_ALWAYS(res.physicalFrame != nullptr);

        Transform parentXform = mg.GetElByID(stationEl.Attachment).GetXform();
        Transform stationXform = stationEl.GetXform();
        glm::vec3 pos = (toInverseMat4(parentXform) * toMat4(stationXform))[3];

        SimTK::Vec3 locationInFrame = SimTKVec3FromV3(pos);

        auto station = std::make_unique<OpenSim::Station>(*res.physicalFrame, locationInFrame);
        station->setName(stationEl.GetLabel());
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
            log::error("cannot create an osim model: issues detected");
            for (std::string const& issue : issuesOut)
            {
                log::error("issue: %s", issue.c_str());
            }
            return nullptr;
        }

        // create the output model
        auto model = std::make_unique<OpenSim::Model>();
        model->updDisplayHints().upd_show_frames() = true;

        // add any meshes that are directly connected to ground (i.e. meshes that are not attached to a body)
        for (MeshEl const& meshEl : mg.iter<MeshEl>())
        {
            if (meshEl.Attachment == g_GroundID)
            {
                AttachMeshElToFrame(meshEl, Transform{}, model->updGround());
            }
        }

        // keep track of any bodies/joints already visited (there might be cycles)
        std::unordered_map<UID, OpenSim::Body*> visitedBodies;
        std::unordered_set<UID> visitedJoints;

        // directly connect any bodies that participate in no joints into the model with a freejoint
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
            if (jointEl.Parent == g_GroundID || ContainsKey(visitedBodies, jointEl.Parent))
            {
                AttachJointRecursive(mg, *model, jointEl, visitedBodies, visitedJoints);
            }
        }

        // add stations into the model
        for (StationEl const& el : mg.iter<StationEl>())
        {
            AddStationToModel(mg, *model, el, visitedBodies);
        }

        return model;
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
        Hover() : ID{g_EmptyID}, Pos{}
        {
        }
        Hover(UID id_, glm::vec3 pos_) : ID{id_}, Pos{pos_}
        {
        }
        operator bool () const noexcept
        {
            return ID != g_EmptyID;
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
            PushMeshLoadRequests(meshFiles);
        }


        //
        // MODEL EXPORT STUFF
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
            m_MaybeOutputModel = CreateOpenSimModelFromModelGraph(GetModelGraph(), m_IssuesBuffer);
        }


        //
        // MODEL GRAPH STUFF
        //

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
            return ::HasSelection(GetModelGraph());
        }

        bool IsSelected(UID id) const
        {
            return GetModelGraph().IsSelected(id);
        }


        //
        // MESH LOADING STUFF
        //

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
            if (ok.Meshes.empty())
            {
                return;
            }


            // add each loaded mesh into the model graph
            ModelGraph& mg = UpdModelGraph();
            mg.DeSelectAll();
            for (LoadedMesh const& lm : ok.Meshes)
            {
                UIDT<BodyEl> attachmentID = DowncastID<BodyEl>(ok.PreferredAttachmentPoint);
                MeshEl& mesh = mg.AddEl<MeshEl>(attachmentID, lm.MeshData, lm.Path);

                BodyEl* maybeBody = mg.TryUpdElByID<BodyEl>(ok.PreferredAttachmentPoint);
                if (maybeBody)
                {
                    mg.Select(maybeBody->ID);
                    mesh.Xform = maybeBody->Xform;
                }

                mg.Select(mesh.ID);
            }

            // commit
            {
                std::stringstream commitMsgSS;
                if (ok.Meshes.empty())
                {
                    commitMsgSS << "loaded 0 meshes";
                }
                else if (ok.Meshes.size() == 1)
                {
                    commitMsgSS << "loaded " << ok.Meshes[0].Path.filename();
                }
                else
                {
                    commitMsgSS << "loaded " << ok.Meshes.size() << " meshes";
                }

                CommitCurrentModelGraph(std::move(commitMsgSS).str());
            }
        }

        // called when the mesh loader responds with a mesh loading error
        void PopMeshLoader_OnErrorResponse(MeshLoadErrorResponse& err)
        {
            log::error("%s: error loading mesh file: %s", err.Path.string().c_str(), err.Error.c_str());
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
            return PromptUserForFiles("obj,vtp,stl");
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

        static constexpr float connectionLineWidth = 1.0f;

        void DrawConnectionLineTriangle(ImU32 color, glm::vec3 parent, glm::vec3 child) const
        {
            constexpr float triangleWidth = 6.0f * connectionLineWidth;
            constexpr float triangleWidthSquared = triangleWidth*triangleWidth;

            glm::vec2 parentScr = WorldPosToScreenPos(parent);
            glm::vec2 childScr = WorldPosToScreenPos(child);
            glm::vec2 child2ParentScr = parentScr - childScr;

            if (glm::dot(child2ParentScr, child2ParentScr) < triangleWidthSquared)
            {
                return;
            }

            glm::vec3 midpoint = VecMidpoint(parent, child);
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
            ImGui::GetWindowDrawList()->AddLine(WorldPosToScreenPos(parent), WorldPosToScreenPos(child), color, connectionLineWidth);

            // the triangle
            DrawConnectionLineTriangle(color, parent, child);
        }

        void DrawConnectionLines(SceneEl const& el, ImU32 color, UID excludeID = g_EmptyID) const
        {
            for (int i = 0, len = el.GetNumCrossReferences(); i < len; ++i)
            {
                UID refID = el.GetCrossReferenceConnecteeID(i);

                if (refID == excludeID)
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

        void DrawConnectionLineToGround(SceneEl const& el, ImU32 color) const
        {
            if (el.GetID() == g_GroundID)
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

                void operator()(GroundEl const&) override
                {
                    m_Result = false;
                }

                void operator()(MeshEl const&) override
                {
                    m_Result = m_Shared.IsShowingMeshConnectionLines();
                }

                void operator()(BodyEl const&) override
                {
                    m_Result = m_Shared.IsShowingBodyConnectionLines();
                }

                void operator()(JointEl const&) override
                {
                    m_Result = m_Shared.IsShowingJointConnectionLines();
                }

                void operator()(StationEl const&) override
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

        void DrawConnectionLines(ImVec4 colorVec, UID excludeID = g_EmptyID) const
        {
            ModelGraph const& mg = GetModelGraph();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);

            for (SceneEl const& el : mg.iter())
            {
                UID id = el.GetID();

                if (id == excludeID)
                {
                    continue;
                }

                if (el.GetNumCrossReferences() > 0)
                {
                    DrawConnectionLines(el, color, excludeID);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    DrawConnectionLineToGround(el, color);
                }
            }
        }

        void DrawConnectionLines() const
        {
            DrawConnectionLines(m_Colors.FaintConnection);
        }


        //
        // RENDERING STUFF
        //

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
            return RectDims(m_3DSceneRect);
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

        gl::Texture2D& UpdSceneTex()
        {
            return m_3DSceneTex;
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
            return g_ColorNames;
        }

        glm::vec4 const& GetColorSceneBackground() const
        {
            return m_Colors.SceneBackground;
        }

        glm::vec4 const& GetColorMesh() const
        {
            return m_Colors.Mesh;
        }

        void SetColorMesh(glm::vec4 const& newColor)
        {
            m_Colors.Mesh = newColor;
        }

        glm::vec4 const& SetColorUnassignedMesh() const
        {
            return m_Colors.UnassignedMesh;
        }

        void SetColorUnassignedMesh(glm::vec4 const& newColor)
        {
            m_Colors.UnassignedMesh = newColor;
        }

        glm::vec4 const& GetColorGround() const
        {
            return m_Colors.Ground;
        }

        glm::vec4 const& GetColorStation() const
        {
            return m_Colors.Station;
        }

        glm::vec4 const& GetColorSolidConnectionLine() const
        {
            return m_Colors.SolidConnection;
        }

        void SetColorSolidConnectionLine(glm::vec4 const& newColor)
        {
            m_Colors.SolidConnection = newColor;
        }

        glm::vec4 const& GetColorTransparentFaintConnectionLine() const
        {
            return m_Colors.TransparentFaintConnection;
        }

        void SetColorTransparentFaintConnectionLine(glm::vec4 const& newColor)
        {
            m_Colors.TransparentFaintConnection = newColor;
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
            return g_VisibilityFlagNames;
        }

        bool IsShowingMeshes() const
        {
            return m_VisibilityFlags.Meshes;
        }

        void SetIsShowingMeshes(bool newIsShowing)
        {
            m_VisibilityFlags.Meshes = newIsShowing;
        }

        bool IsShowingBodies() const
        {
            return m_VisibilityFlags.Bodies;
        }

        void SetIsShowingBodies(bool newIsShowing)
        {
            m_VisibilityFlags.Bodies = newIsShowing;
        }

        bool IsShowingJointCenters() const
        {
            return m_VisibilityFlags.JointCenters;
        }

        void SetIsShowingJointCenters(bool newIsShowing)
        {
            m_VisibilityFlags.JointCenters = newIsShowing;
        }

        bool IsShowingGround() const
        {
            return m_VisibilityFlags.Ground;
        }

        void SetIsShowingGround(bool newIsShowing)
        {
            m_VisibilityFlags.Ground = newIsShowing;
        }

        bool IsShowingFloor() const
        {
            return m_VisibilityFlags.Floor;
        }

        void SetIsShowingFloor(bool newIsShowing)
        {
            m_VisibilityFlags.Floor = newIsShowing;
        }

        bool IsShowingStations() const
        {
            return m_VisibilityFlags.Stations;
        }

        void SetIsShowingStations(bool v)
        {
            m_VisibilityFlags.Stations = v;
        }

        bool IsShowingJointConnectionLines() const
        {
            return m_VisibilityFlags.JointConnectionLines;
        }

        void SetIsShowingJointConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.JointConnectionLines = newIsShowing;
        }

        bool IsShowingMeshConnectionLines() const
        {
            return m_VisibilityFlags.MeshConnectionLines;
        }

        void SetIsShowingMeshConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.MeshConnectionLines = newIsShowing;
        }

        bool IsShowingBodyConnectionLines() const
        {
            return m_VisibilityFlags.BodyToGroundConnectionLines;
        }

        void SetIsShowingBodyConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.BodyToGroundConnectionLines = newIsShowing;
        }

        bool IsShowingStationConnectionLines() const
        {
            return m_VisibilityFlags.StationConnectionLines;
        }

        void SetIsShowingStationConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.StationConnectionLines = newIsShowing;
        }

        glm::mat4 GetFloorModelMtx() const
        {
            glm::mat4 rv{1.0f};

            rv = glm::rotate(rv, fpi2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {m_SceneScaleFactor * 100.0f, m_SceneScaleFactor * 100.0f, 1.0f});

            return rv;
        }

        DrawableThing GenerateFloorDrawable() const
        {
            DrawableThing dt;
            dt.id = g_EmptyID;
            dt.groupId = g_EmptyID;
            dt.mesh = App::meshes().get100x100GridMesh();
            dt.modelMatrix = GetFloorModelMtx() * glm::scale(glm::mat4{1.0f}, glm::vec3{0.5f, 0.5f, 0.5f});
            dt.normalMatrix = NormalMatrix(dt.modelMatrix);
            dt.color = m_Colors.FloorTint;
            dt.rimColor = 0.0f;
            dt.maybeDiffuseTex = nullptr;
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

        void AppendAsFrame(UID logicalID,
                           UID groupID,
                           Transform const& xform,
                           std::vector<DrawableThing>& appendOut,
                           float alpha = 1.0f,
                           float rimAlpha = 0.0f,
                           glm::vec3 legLen = {1.0f, 1.0f, 1.0f},
                           glm::vec3 coreColor = {1.0f, 1.0f, 1.0f}) const
        {
            // stolen from SceneGeneratorNew.cpp

            glm::vec3 origin = xform.position;
            glm::mat3 rotation = glm::toMat3(xform.rotation);

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
            for (int i = 0; i < 3; ++i)
            {
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
                               Transform const& xform,
                               std::vector<DrawableThing>& appendOut,
                               float alpha = 1.0f,
                               float rimAlpha = 0.0f,
                               glm::vec3 legLen = {1.0f, 1.0f, 1.0f},
                               glm::vec3 coreColor = {1.0f, 1.0f, 1.0f},
                               glm::vec3 sfs = {1.0f, 1.0f, 1.0f}) const
        {
            glm::mat4 baseMmtx = toMat4(xform);

            float halfWidths = 1.5f * GetSphereRadius();
            glm::vec3 scaleFactors = halfWidths * sfs;

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
            for (int i = 0; i < 3; ++i)
            {
                Segment coneLine{glm::vec3{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
                Segment outputLine{};
                outputLine.p1[i] = halfWidths;
                outputLine.p2[i] = 1.75f * halfWidths * legLen[i];

                glm::mat4 segXform = SegmentToSegmentXform(coneLine, outputLine);
                segXform = baseMmtx * segXform * glm::scale(glm::mat4{1.0f}, glm::vec3{halfWidths/2.0f, 1.0f, halfWidths/2.0f});

                glm::vec4 color = {0.0f, 0.0f, 0.0f, alpha};
                color[i] = 1.0f;

                DrawableThing& legCube = appendOut.emplace_back();
                legCube.id = logicalID;
                legCube.groupId = groupID;
                legCube.mesh = App::cur().meshes().getConeMesh();
                legCube.modelMatrix = segXform;
                legCube.normalMatrix = NormalMatrix(segXform);
                legCube.color = color;
                legCube.rimColor = rimAlpha;
                legCube.maybeDiffuseTex = nullptr;
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
            return g_InteractivityFlagNames;
        }

        bool IsMeshesInteractable() const
        {
            return m_InteractivityFlags.Meshes;
        }

        void SetIsMeshesInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.Meshes = newIsInteractable;
        }

        bool IsBodiesInteractable() const
        {
            return m_InteractivityFlags.Bodies;
        }

        void SetIsBodiesInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.Bodies = newIsInteractable;
        }

        bool IsJointCentersInteractable() const
        {
            return m_InteractivityFlags.JointCenters;
        }

        void SetIsJointCentersInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.JointCenters = newIsInteractable;
        }

        bool IsGroundInteractable() const
        {
            return m_InteractivityFlags.Ground;
        }

        void SetIsGroundInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.Ground = newIsInteractable;
        }

        bool IsStationsInteractable() const
        {
            return m_InteractivityFlags.Stations;
        }

        void SetIsStationsInteractable(bool v)
        {
            m_InteractivityFlags.Stations = v;
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
            Rect sceneRect = Get3DSceneRect();
            glm::vec2 mousePos = ImGui::GetMousePos();

            if (!PointIsInRect(sceneRect, mousePos))
            {
                return Hover{};
            }

            glm::vec2 sceneDims = RectDims(sceneRect);
            glm::vec2 relMousePos = mousePos - sceneRect.p1;

            Line ray = GetCamera().unprojectTopLeftPosToWorldRay(relMousePos, sceneDims);
            bool hittestMeshes = IsMeshesInteractable();
            bool hittestBodies = IsBodiesInteractable();
            bool hittestJointCenters = IsJointCentersInteractable();
            bool hittestGround = IsGroundInteractable();
            bool hittestStations = IsStationsInteractable();

            UID closestID = g_EmptyID;
            float closestDist = std::numeric_limits<float>::max();

            for (DrawableThing const& drawable : drawables)
            {
                if (drawable.id == g_EmptyID)
                {
                    continue;  // no hittest data
                }

                if (drawable.groupId == g_BodyGroupID && !hittestBodies)
                {
                    continue;
                }

                if (drawable.groupId == g_MeshGroupID && !hittestMeshes)
                {
                    continue;
                }

                if (drawable.groupId == g_JointGroupID && !hittestJointCenters)
                {
                    continue;
                }

                if (drawable.groupId == g_GroundGroupID && !hittestGround)
                {
                    continue;
                }

                if (drawable.groupId == g_StationGroupID && !hittestStations)
                {
                    continue;
                }

                RayCollision rc = drawable.mesh->getRayMeshCollisionInWorldspace(drawable.modelMatrix, ray);
                if (rc.hit && rc.distance < closestDist)
                {
                    closestID = drawable.id;
                    closestDist = rc.distance;
                }
            }

            glm::vec3 hitPos = closestID != g_EmptyID ? ray.origin + closestDist*ray.dir : glm::vec3{};

            return Hover{closestID, hitPos};
        }

        //
        // SCENE ELEMENT STUFF (specific methods for specific scene element types)
        //

        UIDT<BodyEl> AddBody(std::string const& name, glm::vec3 const& shift, glm::vec3 const& rot)
        {
            ModelGraph& mg = UpdModelGraph();

            BodyEl& b = mg.AddEl<BodyEl>(name, Transform{shift, rot});
            mg.DeSelectAll();
            mg.Select(b.ID);

            CommitCurrentModelGraph(std::string{"added "} + b.GetLabel());

            return b.ID;
        }

        UIDT<BodyEl> AddBody(glm::vec3 const& pos)
        {
            return AddBody(GenerateName(BodyEl::Class()), pos, {});
        }

        void UnassignMesh(MeshEl const& me)
        {
            UpdModelGraph().UpdElByID<MeshEl>(me.ID).Attachment = g_GroundID;

            std::stringstream ss;
            ss << "unassigned '" << me.Name << "' back to ground";
            CommitCurrentModelGraph(std::move(ss).str());
        }

        DrawableThing GenerateMeshElDrawable(MeshEl const& meshEl) const
        {
            DrawableThing rv;
            rv.id = meshEl.ID;
            rv.groupId = g_MeshGroupID;
            rv.mesh = meshEl.MeshData;
            rv.modelMatrix = toMat4(meshEl.Xform);
            rv.normalMatrix = toNormalMatrix(meshEl.Xform);
            rv.color = meshEl.Attachment == g_GroundID || meshEl.Attachment == g_EmptyID ? SetColorUnassignedMesh() : GetColorMesh();
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        DrawableThing GenerateBodyElSphere(BodyEl const& bodyEl, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = bodyEl.ID;
            rv.groupId = g_BodyGroupID;
            rv.mesh = m_SphereMesh;
            rv.modelMatrix = SphereMeshToSceneSphereXform(SphereAtTranslation(bodyEl.Xform.position));
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

        DrawableThing GenerateStationSphere(StationEl const& el, glm::vec4 const& color) const
        {
            DrawableThing rv;
            rv.id = el.GetID();
            rv.groupId = g_StationGroupID;
            rv.mesh = m_SphereMesh;
            rv.modelMatrix = SphereMeshToSceneSphereXform(SphereAtTranslation(el.GetPos()));
            rv.normalMatrix = NormalMatrix(rv.modelMatrix);
            rv.color = color;
            rv.rimColor = 0.0f;
            rv.maybeDiffuseTex = nullptr;
            return rv;
        }

        void AppendBodyElAsCubeThing(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            AppendAsCubeThing(bodyEl.ID, g_BodyGroupID, bodyEl.Xform, appendOut);
        }

        void AppendBodyElAsFrame(BodyEl const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            AppendAsFrame(bodyEl.ID, g_BodyGroupID, bodyEl.Xform, appendOut);
        }

        void AppendDrawables(SceneEl const& e, std::vector<DrawableThing>& appendOut) const
        {
            class Visitor : public ConstSceneElVisitor {
            public:
                Visitor(SharedData const& data,
                        std::vector<DrawableThing>& out) :
                    m_Data{data},
                    m_Out{out}
                {
                }

                void operator()(GroundEl const&) override
                {
                    if (!m_Data.IsShowingGround())
                    {
                        return;
                    }

                    m_Out.push_back(m_Data.GenerateGroundSphere(m_Data.GetColorGround()));
                }
                void operator()(MeshEl const& el) override
                {
                    if (!m_Data.IsShowingMeshes())
                    {
                        return;
                    }

                    m_Out.push_back(m_Data.GenerateMeshElDrawable(el));
                }
                void operator()(BodyEl const& el) override
                {
                    if (!m_Data.IsShowingBodies())
                    {
                        return;
                    }

                    m_Data.AppendBodyElAsCubeThing(el, m_Out);
                }
                void operator()(JointEl const& el) override
                {
                    if (!m_Data.IsShowingJointCenters()) {
                        return;
                    }

                    m_Data.AppendAsFrame(el.ID,
                                         g_JointGroupID,
                                         el.Xform,
                                         m_Out,
                                         1.0f,
                                         0.0f,
                                         GetJointAxisLengths(el));
                }
                void operator()(StationEl const& el) override
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
            if (HasOutputModel())
            {
                auto mainEditorState = std::make_shared<MainEditorState>(std::move(UpdOutputModel()));
                mainEditorState->editedModel.setFixupScaleFactor(m_SceneScaleFactor);
                for (auto& viewerPtr : mainEditorState->viewers)
                {
                    if (viewerPtr)
                    {
                        viewerPtr->requestAutoFocus();
                    }
                }

                App::cur().requestTransition<ModelEditorScreen>(mainEditorState);
            }

            m_ModelGraphSnapshots.GarbageCollect();
        }

    private:
        // model graph (snapshots) the user is working on
        CommittableModelGraph m_ModelGraphSnapshots;

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
        PolarPerspectiveCamera m_3DSceneCamera = CreateDefaultCamera();

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
        struct{
            glm::vec4 Mesh{1.0f, 1.0f, 1.0f, 1.0f};
            glm::vec4 UnassignedMesh{1.0f, 0.95f, 0.95, 1.0f};
            glm::vec4 Ground{196.0f/255.0f, 196.0f/255.0f, 196.0/255.0f, 1.0f};
            glm::vec4 Station{196.0f/255.0f, 0.0f, 0.0f, 1.0f};
            glm::vec4 FaintConnection{0.6f, 0.6f, 0.6f, 1.0f};
            glm::vec4 SolidConnection{0.9f, 0.9f, 0.9f, 1.0f};
            glm::vec4 TransparentFaintConnection{0.6f, 0.6f, 0.6f, 0.2f};
            glm::vec4 SceneBackground{96.0f/255.0f, 96.0f/255.0f, 96.0f/255.0f, 1.0f};
            glm::vec4 FloorTint{156.0f/255.0f, 156.0f/255.0f, 156.0f/255.0f, 1.0f};
        } m_Colors;
        static constexpr std::array<char const*, 9> g_ColorNames = {
            "mesh",
            "unassigned mesh",
            "ground",
            "station",
            "faint connection line",
            "solid connection line",
            "transparent faint connection line",
            "scene background",
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
            bool Stations = true;
            bool JointConnectionLines = true;
            bool MeshConnectionLines = true;
            bool BodyToGroundConnectionLines = true;
            bool StationConnectionLines = true;
        } m_VisibilityFlags;
        static constexpr std::array<char const*, 10> g_VisibilityFlagNames = {
            "floor",
            "meshes",
            "ground",
            "bodies",
            "joint centers",
            "stations",
            "joint connection lines",
            "mesh connection lines",
            "body-to-ground connection lines",
            "station connection lines"
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
            bool Stations = true;
        } m_InteractivityFlags;
        static constexpr std::array<char const*, 5> g_InteractivityFlagNames = {
            "meshes",
            "bodies",
            "joint centers",
            "ground",
            "stations",
        };
        static_assert(sizeof(decltype(m_InteractivityFlags))/sizeof(bool) == g_InteractivityFlagNames.size());

    public:
        // WINDOWS
        //
        // these are runtime-editable flags that dictate which panels are open
        std::array<bool, 3> m_PanelStates{false, true, false};
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

// select 2 mesh points layer
namespace
{

    // runtime options for "Select two mesh points" UI layer
    struct Select2MeshPointsOptions final {

        // a function that is called when the implementation detects two points have
        // been clicked
        //
        // the function should return `true` if the points are accepted
        std::function<bool(glm::vec3, glm::vec3)> OnTwoPointsChosen = [](glm::vec3, glm::vec3)
        {
            return true;
        };

        std::string Header = "choose first (left-click) and second (right click) mesh positions (ESC to cancel)";
    };

    // UI layer that lets the user select two points on a mesh with left-click and
    // right-click
    class Select2MeshPointsLayer final : public Layer {
    public:
        Select2MeshPointsLayer(LayerHost& parent,
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

            bool pointsAccepted = m_Options.OnTwoPointsChosen(*m_MaybeFirstLocation, *m_MaybeSecondLocation);

            if (pointsAccepted)
            {
                m_Parent.pop();
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
            if (m_Options.Header.empty())
            {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
            glm::vec2 padding{10.0f, 10.0f};
            glm::vec2 pos = m_Shared->Get3DSceneRect().p1 + padding;
            ImGui::GetWindowDrawList()->AddText(pos, color, m_Options.Header.c_str());
        }

    public:
        bool onEvent(SDL_Event const& e) override
        {
            return m_Shared->onEvent(e);
        }

        void tick(float dt) override
        {
            m_Shared->tick(dt);

            if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE))
            {
                // ESC: user cancelled out
                m_Parent.pop();
            }

            bool isRenderHovered = m_Shared->IsRenderHovered();

            if (isRenderHovered)
            {
                UpdatePolarCameraFromImGuiUserInput(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }
        }

        void draw() override
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

    private:
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
        bool CanChooseBodies = true;
        bool CanChooseGround = true;
        bool CanChooseMeshes = true;
        bool CanChooseJoints = true;
        UID MaybeElAttachingTo = g_EmptyID;
        bool IsAttachingTowardEl = true;  // false implies "away from"
        UID MaybeElBeingReplacedByChoice = g_EmptyID;
        std::function<bool(UID)> OnUserChoice = [](UID)
        {
            return true;
        };
        std::string Header = "choose something";
    };

    // "choose something" UI layer
    //
    // this is what's drawn when the user's being prompted to choose something
    // else in the scene
    class ChooseElLayer final : public Layer {
    public:
        ChooseElLayer(LayerHost& parent,
                      std::shared_ptr<SharedData> shared,
                      ChooseElLayerOptions options) :
            Layer{parent},
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:
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
                m_Shared->DrawConnectionLines(m_Shared->GetColorTransparentFaintConnectionLine());
                return;
            }

            // else: user is hovering *something*

            // draw all other connection lines but exclude the thing being assigned (if any)
            m_Shared->DrawConnectionLines(m_Shared->GetColorTransparentFaintConnectionLine(), m_Options.MaybeElBeingReplacedByChoice);

            if (m_Options.MaybeElAttachingTo == g_EmptyID)
            {
                return;  // we don't know what the user's choice is ultimately attaching to
            }

            // draw strong connection line between the thing being attached to and the hover
            glm::vec3 parentPos = GetPosition(m_Shared->GetModelGraph(), m_Options.MaybeElAttachingTo);
            glm::vec3 childPos = GetPosition(m_Shared->GetModelGraph(), m_MaybeHover.ID);

            if (!m_Options.IsAttachingTowardEl)
            {
                std::swap(parentPos, childPos);
            }

            ImU32 strongColorU2 = ImGui::ColorConvertFloat4ToU32(m_Shared->GetColorSolidConnectionLine());

            m_Shared->DrawConnectionLine(strongColorU2, parentPos, childPos);
        }

        // draw 2D header text in top-left corner of the screen
        void DrawHeaderText() const
        {
            if (m_Options.Header.empty())
            {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
            glm::vec2 padding = glm::vec2{10.0f, 10.0f};
            glm::vec2 pos = m_Shared->Get3DSceneRect().p1 + padding;
            ImGui::GetWindowDrawList()->AddText(pos, color, m_Options.Header.c_str());
        }

        bool IsSelectable(SceneEl const& el)
        {
            if (el.GetID() == m_Options.MaybeElAttachingTo)
            {
                return false;
            }

            struct Visitor final : public ConstSceneElVisitor {
            public:
                Visitor(ChooseElLayerOptions& opts) : m_Opts{opts}
                {
                }

                bool result() const
                {
                    return m_Result;
                }

                void operator()(GroundEl const&) override
                {
                    m_Result = m_Opts.CanChooseGround;
                }

                void operator()(MeshEl const&) override
                {
                    m_Result = m_Opts.CanChooseMeshes;
                }

                void operator()(BodyEl const&) override
                {
                    m_Result = m_Opts.CanChooseBodies;
                }

                void operator()(JointEl const&) override
                {
                    m_Result = m_Opts.CanChooseJoints;
                }

                void operator()(StationEl const&) override
                {
                    // TODO
                }

            private:
                bool m_Result = false;
                ChooseElLayerOptions const& m_Opts;
            };

            Visitor v{m_Options};
            el.Accept(v);
            return v.result();
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
                float rimColor = el.GetID() == m_MaybeHover.ID ? 0.8f : 0.0f;

                for (size_t i = start; i < end; ++i)
                {
                    DrawableThing& d = m_DrawablesBuffer[i];
                    d.rimColor = rimColor;

                    if (!isSelectable)
                    {
                        d.color.a = fadedAlpha;
                        d.id = g_EmptyID;
                        d.groupId = g_EmptyID;
                    }
                    else
                    {
                        d.modelMatrix = d.modelMatrix * glm::scale(glm::mat4{1.0f}, glm::vec3{animScale, animScale, animScale});
                    }
                }
            }

            // floor
            m_DrawablesBuffer.push_back(m_Shared->GenerateFloorDrawable());

            return m_DrawablesBuffer;
        }

        // handle any side-effects from the user's mouse hover
        void HandleHovertestSideEffects()
        {
            if (!m_MaybeHover)
            {
                return;
            }

            DrawHoverTooltip();

            // if user clicks on hovered element, then they are trying to select it
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (m_Options.OnUserChoice(m_MaybeHover.ID))
                {
                    m_Parent.pop();
                }
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
        bool onEvent(SDL_Event const& e) override
        {
            return m_Shared->onEvent(e);
        }

        void tick(float dt) override
        {
            m_Shared->tick(dt);

            if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE))
            {
                // ESC: user cancelled out
                m_Parent.pop();
            }

            bool isRenderHovered = m_Shared->IsRenderHovered();

            if (isRenderHovered)
            {
                UpdatePolarCameraFromImGuiUserInput(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }

            if (m_AnimationFraction < 1.0f)
            {
                m_AnimationFraction = std::clamp(m_AnimationFraction + 0.5f*dt, 0.0f, 1.0f);
                App::cur().requestRedraw();
            }
        }

        void draw() override
        {
            m_Shared->SetContentRegionAvailAsSceneRect();

            std::vector<DrawableThing>& drawables = GenerateDrawables();

            m_MaybeHover = m_Shared->Hovertest(drawables);
            HandleHovertestSideEffects();

            m_Shared->DrawScene(drawables);
            DrawConnectionLines();
            DrawHeaderText();
        }


    private:
        // data that's shared between other UI states
        std::shared_ptr<SharedData> m_Shared;

        // options for this state
        ChooseElLayerOptions m_Options;

        // (maybe) user mouse hover
        Hover m_MaybeHover;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;

        // fraction that the system is through its animation cycle: ranges from 0.0 to 1.0 inclusive
        float m_AnimationFraction = 0.0f;
    };
}

// main state
namespace
{
    // "standard" UI state
    //
    // this is what the user is typically interacting with when the UI loads
    class MainUIState final : public LayerHost {
    public:

        MainUIState(std::shared_ptr<SharedData> shared) : m_Shared{std::move(shared)}
        {
        }

        //
        // ACTIONS
        //

        // pop the current UI layer
        void pop() override
        {
            m_Maybe3DViewerModal.reset();
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
        void AddBodyToHoveredElement()
        {
            if (!m_MaybeHover)
            {
                return;
            }

            m_Shared->AddBody(m_MaybeHover.Pos);
        }

        // try transitioning the shown UI layer to one where the user is assigning a mesh
        void TryTransitionToAssigningHoveredMeshNextFrame()
        {
            if (!m_MaybeHover)
            {
                return;
            }

            MeshEl* maybeMesh = m_Shared->UpdModelGraph().TryUpdElByID<MeshEl>(m_MaybeHover.ID);

            if (!maybeMesh)
            {
                return;  // not hovering a mesh
            }

            TransitionToAssigningMeshNextFrame(*maybeMesh);
        }

        //
        // TRANSITIONS
        //
        // methods for transitioning the main 3D UI to some other state
        //

        // transition the shown UI layer to one where the user is assigning a mesh
        void TransitionToAssigningMeshNextFrame(MeshEl& meshEl)
        {
            ChooseElLayerOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = false;
            opts.CanChooseMeshes = false;
            opts.MaybeElAttachingTo = meshEl.ID;
            opts.IsAttachingTowardEl = false;
            opts.MaybeElBeingReplacedByChoice = meshEl.Attachment;
            opts.Header = "choose mesh attachment point (ESC to cancel)";
            opts.OnUserChoice = [shared = m_Shared, meshID = meshEl.ID](UID choice)
            {
                return TryAssignMeshAttachment(shared->UpdCommittableModelGraph(), meshID, choice);
            };

            // request a state transition
            m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
        }

        // transition the shown UI layer to one where the user is choosing a joint parent
        void TransitionToChoosingJointParent(BodyEl const& child)
        {
            ChooseElLayerOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = false;
            opts.CanChooseMeshes = false;
            opts.Header = "choose joint parent (ESC to cancel)";
            opts.MaybeElAttachingTo = child.GetID();
            opts.IsAttachingTowardEl = false;  // away from the body
            opts.OnUserChoice = [shared = m_Shared, childID = child.ID](UID parentID)
            {
                return TryCreateJoint(shared->UpdCommittableModelGraph(), childID, parentID);
            };
            m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
        }

        // transition the shown UI layer to one where the user is choosing which element in the scene to point
        // an element's axis towards
        void TransitionToChoosingWhichElementToPointAxisTowards(SceneEl& el, int axis)
        {
            ChooseElLayerOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = true;
            opts.CanChooseMeshes = false;
            opts.MaybeElAttachingTo = el.GetID();
            opts.Header = "choose what to point towards (ESC to cancel)";
            opts.OnUserChoice = [shared = m_Shared, id = el.GetID(), axis](UID userChoice)
            {
                return PointAxisTowards(shared->UpdCommittableModelGraph(), id, axis, userChoice);
            };
            m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
        }

        // transition the shown UI layer to one where the user is choosing two mesh points that
        // the element should be oriented along
        void TransitionToOrientingElementAlongTwoMeshPoints(SceneEl& el, int axis)
        {
            Select2MeshPointsOptions opts;
            opts.OnTwoPointsChosen = [shared = m_Shared, id = el.GetID(), axis](glm::vec3 a, glm::vec3 b)
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
            opts.OnTwoPointsChosen = [shared = m_Shared, id = el.GetID()](glm::vec3 a, glm::vec3 b)
            {
                return TryTranslateElementBetweenTwoPoints(shared->UpdCommittableModelGraph(), id, a, b);
            };
            m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
        }

        // transition the shown UI layer to one where the user is choosing another element that
        // the element should be translated to the midpoint of
        void TransitionToTranslatingElementToAnotherElementsCenter(SceneEl& el)
        {
            ChooseElLayerOptions opts;
            opts.CanChooseBodies = true;
            opts.CanChooseGround = true;
            opts.CanChooseJoints = true;
            opts.CanChooseMeshes = true;
            opts.MaybeElAttachingTo = el.GetID();
            opts.Header = "choose where to place it (ESC to cancel)";
            opts.OnUserChoice = [shared = m_Shared, id = el.GetID()](UID userChoice)
            {
                return TryTranslateElementToAnotherElement(shared->UpdCommittableModelGraph(), id, userChoice);
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
            opts.CanChooseBodies = Is<BodyEl>(*old) || Is<GroundEl>(*old);
            opts.CanChooseGround = Is<BodyEl>(*old) || Is<GroundEl>(*old);
            opts.CanChooseJoints = Is<JointEl>(*old);
            opts.CanChooseMeshes = Is<MeshEl>(*old);
            opts.MaybeElAttachingTo = el.GetID();
            opts.Header = "choose what to attach to";
            opts.OnUserChoice = [shared = m_Shared, id = el.GetID(), crossrefIdx](UID userChoice)
            {
                return TryReassignCrossref(shared->UpdCommittableModelGraph(), id, crossrefIdx, userChoice);
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

            if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_N))
            {
                // Ctrl+N: new scene
                ResetModelGraph(m_Shared->UpdCommittableModelGraph());
                return true;
            }
            else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_W))
            {
                // Ctrl+W: close
                App::cur().requestTransition<SplashScreen>();
                return true;
            }
            else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_Q))
            {
                // Ctrl+Q: quit application
                App::cur().requestQuit();
                return true;
            }
            else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_A))
            {
                // Ctrl+A: select all
                m_Shared->SelectAll();
                return true;
            }
            else if (ctrlOrSuperDown && shiftDown && ImGui::IsKeyPressed(SDL_SCANCODE_Z))
            {
                // Ctrl+Shift+Z: redo
                m_Shared->RedoCurrentModelGraph();
                return true;
            }
            else if (ctrlOrSuperDown && ImGui::IsKeyPressed(SDL_SCANCODE_Z))
            {
                // Ctrl+Z: undo
                m_Shared->UndoCurrentModelGraph();
                return true;
            }
            else if (osc::IsAnyKeyDown({ SDL_SCANCODE_DELETE, SDL_SCANCODE_BACKSPACE}))
            {
                // Delete/Backspace: delete any selected elements
                DeleteSelected();
                return true;
            }
            else if (ImGui::IsKeyPressed(SDL_SCANCODE_B))
            {
                // B: add body to hovered element
                AddBodyToHoveredElement();
                return true;
            }
            else if (ImGui::IsKeyPressed(SDL_SCANCODE_A))
            {
                // A: assign a parent for the hovered element
                TryTransitionToAssigningHoveredMeshNextFrame();
                return true;
            }
            else if (ImGui::IsKeyPressed(SDL_SCANCODE_R))
            {
                // R: set manipulation mode to "rotate"
                if (m_ImGuizmoState.op == ImGuizmo::ROTATE)
                {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::ROTATE;
                return true;
            }
            else if (ImGui::IsKeyPressed(SDL_SCANCODE_G))
            {
                // G: set manipulation mode to "grab" (translate)
                if (m_ImGuizmoState.op == ImGuizmo::TRANSLATE)
                {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::TRANSLATE;
                return true;
            }
            else if (ImGui::IsKeyPressed(SDL_SCANCODE_S))
            {
                // S: set manipulation mode to "scale"
                if (m_ImGuizmoState.op == ImGuizmo::SCALE)
                {
                    m_ImGuizmoState.mode = m_ImGuizmoState.mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                }
                m_ImGuizmoState.op = ImGuizmo::SCALE;
                return true;
            }
            else if (ImGui::IsKeyDown(SDL_SCANCODE_UP))
            {
                if (ctrlOrSuperDown)
                {
                    // pan
                    m_Shared->UpdCamera().pan(VecAspectRatio(m_Shared->Get3DSceneDims()), {0.0f, 0.1f});
                }
                else if (shiftDown)
                {
                    // rotate in 90-deg increments
                    m_Shared->UpdCamera().phi -= glm::radians(90.0f);
                }
                else
                {
                    // rotate in 10-deg increments
                    m_Shared->UpdCamera().phi -= glm::radians(10.0f);
                }
                return true;
            }
            else if (ImGui::IsKeyDown(SDL_SCANCODE_DOWN))
            {
                if (ctrlOrSuperDown)
                {
                    // pan
                    m_Shared->UpdCamera().pan(VecAspectRatio(m_Shared->Get3DSceneDims()), {0.0f, -0.1f});
                }
                else if (shiftDown)
                {
                    // rotate in 90-deg increments
                    m_Shared->UpdCamera().phi += glm::radians(90.0f);
                }
                else
                {
                    // rotate in 10-deg increments
                    m_Shared->UpdCamera().phi += glm::radians(10.0f);
                }
                return true;
            }
            else if (ImGui::IsKeyDown(SDL_SCANCODE_LEFT))
            {
                if (ctrlOrSuperDown)
                {
                    // pan
                    m_Shared->UpdCamera().pan(VecAspectRatio(m_Shared->Get3DSceneDims()), {0.1f, 0.0f});
                }
                else if (shiftDown)
                {
                    // rotate in 90-deg increments
                    m_Shared->UpdCamera().theta += glm::radians(90.0f);
                }
                else
                {
                    // rotate in 10-deg increments
                    m_Shared->UpdCamera().theta += glm::radians(10.0f);
                }
                return true;
            }
            else if (ImGui::IsKeyDown(SDL_SCANCODE_RIGHT))
            {
                if (ctrlOrSuperDown)
                {
                    // pan
                    m_Shared->UpdCamera().pan(VecAspectRatio(m_Shared->Get3DSceneDims()), {-0.1f, 0.0f});
                }
                else if (shiftDown)
                {
                    // rotate in 90-deg increments
                    m_Shared->UpdCamera().theta -= glm::radians(90.0f);
                }
                else
                {
                    // rotate in 10-deg increments
                    m_Shared->UpdCamera().theta -= glm::radians(10.0f);
                }
                return true;
            }
            else if (ImGui::IsKeyDown(SDL_SCANCODE_MINUS))
            {
                m_Shared->UpdCamera().radius *= 1.1f;
                return true;
            }
            else if (ImGui::IsKeyDown(SDL_SCANCODE_EQUALS))
            {
                m_Shared->UpdCamera().radius *= 0.9f;
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
            DrawHelpMarker(e.GetClass().GetNameCStr(), e.GetClass().GetDescriptionCStr());
            ImGui::Separator();
        }

        void DrawSceneElPropEditors(SceneEl const& e)
        {
            ModelGraph& mg = m_Shared->UpdModelGraph();

            // label/name editor
            if (CanChangeLabel(e))
            {
                char buf[256];
                std::strcpy(buf, e.GetLabel().c_str());
                if (ImGui::InputText("Name", buf, sizeof(buf)))
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
                DrawHelpMarker("Component Name", "This is the name that the component will have in the exported OpenSim model.");
            }

            // position editor
            if (CanChangePosition(e))
            {
                glm::vec3 translation = e.GetPos();
                if (ImGui::InputFloat3("Translation", glm::value_ptr(translation), OSC_FLOAT_INPUT_FORMAT))
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
                DrawHelpMarker("Translation", OSC_TRANSLATION_DESC);
            }

            // rotation editor
            if (CanChangeRotation(e))
            {
                glm::vec3 eulerDegs = glm::degrees(glm::eulerAngles(e.GetRotation()));

                if (ImGui::InputFloat3("Rotation (deg)", glm::value_ptr(eulerDegs), OSC_FLOAT_INPUT_FORMAT))
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
                DrawHelpMarker("Rotation", "These are the rotation Euler angles for the component in ground. Positive rotations are anti-clockwise along that axis.\n\nNote: the numbers may contain slight rounding error, due to backend constraints. Your values *should* be accurate to a few decimal places.");
            }

            // scale factor editor
            if (CanChangeScale(e))
            {
                glm::vec3 scaleFactors = e.GetScale();
                if (ImGui::InputFloat3("scale", glm::value_ptr(scaleFactors), OSC_FLOAT_INPUT_FORMAT))
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
                DrawHelpMarker("Scale", "These are the scale factors of the component in ground. These scale-factors are applied to the element before any other transform (it scales first, then rotates, then translates).");
            }
        }

        // draw content of "Add" menu for some scene element
        void DrawAddOtherToSceneElActions(SceneEl& el, glm::vec3 const& clickPos)
        {
            int imguiID = 0;

            ImGui::PushID(imguiID++);
            if (CanAttachMeshTo(el))
            {
                if (ImGui::MenuItem(ICON_FA_CUBE " Meshes"))
                {
                    m_Shared->PushMeshLoadRequests(DowncastID<BodyEl>(el.GetID()), m_Shared->PromptUserForMeshFiles());
                }
                DrawTooltipIfItemHovered("Add Meshes", OSC_MESH_DESC);
            }
            ImGui::PopID();

            ImGui::PushID(imguiID++);
            if (HasPhysicalSize(el))
            {
                if (ImGui::BeginMenu(ICON_FA_CIRCLE " Body"))
                {
                    if (ImGui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                    {
                        m_Shared->AddBody(el.GetPos());
                    }
                    DrawTooltipIfItemHovered("Add Body", OSC_BODY_DESC);

                    if (ImGui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                    {
                        m_Shared->AddBody(clickPos);
                    }
                    DrawTooltipIfItemHovered("Add Body", OSC_BODY_DESC);

                    if (Is<MeshEl>(el))
                    {
                        if (ImGui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                        {
                            m_Shared->AddBody(AABBCenter(el.CalcBounds()));
                        }
                        DrawTooltipIfItemHovered("Add Body", OSC_BODY_DESC);
                    }

                    ImGui::EndMenu();
                }
            }
            else
            {
                if (ImGui::MenuItem(ICON_FA_CIRCLE " Body"))
                {
                    m_Shared->AddBody(el.GetPos());
                }
                DrawTooltipIfItemHovered("Add Body", OSC_BODY_DESC);
            }
            ImGui::PopID();

            ImGui::PushID(imguiID++);
            if (Is<BodyEl>(el))
            {
                if (ImGui::MenuItem(ICON_FA_LINK " Joint"))
                {
                    TransitionToChoosingJointParent(dynamic_cast<BodyEl const&>(el));
                }
                DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a freejoint between the body and ground.");
            }
            ImGui::PopID();

            ImGui::PushID(imguiID++);
            if (CanAttachStationTo(el))
            {
                auto addStationAtLocation = [&](glm::vec3 const& loc)
                {
                    ModelGraph& mg = m_Shared->UpdModelGraph();
                    StationEl& station = mg.AddEl<StationEl>(GenerateIDT<StationEl>(), StationAttachmentParent(el), loc, GenerateName(StationEl::Class()));
                    SelectOnly(mg, station);
                    m_Shared->CommitCurrentModelGraph("added station " + station.GetLabel());
                };

                if (HasPhysicalSize(el))
                {
                    if (ImGui::BeginMenu(ICON_FA_MAP_PIN " Station"))
                    {
                        if (ImGui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                        {
                            addStationAtLocation(el.GetPos());
                        }
                        DrawTooltipIfItemHovered("Add Station", OSC_STATION_DESC);

                        if (ImGui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                        {
                            addStationAtLocation(clickPos);
                        }
                        DrawTooltipIfItemHovered("Add Station", OSC_STATION_DESC);

                        if (ImGui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                        {
                            addStationAtLocation(glm::vec3{});
                        }
                        DrawTooltipIfItemHovered("Add Station", OSC_STATION_DESC);

                        if (Is<MeshEl>(el))
                        {
                            if (ImGui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                            {
                                addStationAtLocation(AABBCenter(el.CalcBounds()));
                            }
                            DrawTooltipIfItemHovered("Add Station", OSC_STATION_DESC);
                        }

                        ImGui::EndMenu();
                    }
                }
                else
                {
                    if (ImGui::MenuItem(ICON_FA_MAP_PIN " Station"))
                    {
                        addStationAtLocation(el.GetPos());
                    }
                    DrawTooltipIfItemHovered("Add Station", OSC_STATION_DESC);
                }

            }
            ImGui::PopID();
        }

        void DrawNothingActions()
        {
            if (ImGui::MenuItem(ICON_FA_CUBE " Add Meshes"))
            {
                m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
            }
            DrawTooltipIfItemHovered("Add Meshes to the model", OSC_MESH_DESC);

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
                m_Shared->FocusCameraOn(AABBCenter(el.CalcBounds()));
            }
            DrawTooltipIfItemHovered("Focus camera on this scene element", "Focuses the scene camera on this element. This is useful for tracking the camera around that particular object in the scene");

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
                DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a freejoint between the body and ground.");
            }

            if (CanDelete(el))
            {
                if (ImGui::MenuItem(ICON_FA_TRASH " Delete"))
                {
                    ::DeleteEl(m_Shared->UpdCommittableModelGraph(), el.GetID());
                    GarbageCollectStaleRefs();
                    ImGui::CloseCurrentPopup();
                }
                DrawTooltipIfItemHovered("Delete", "Deletes the component from the model. Deletion is undo-able (use the undo/redo feature). Anything attached to this element (e.g. joints, meshes) will also be deleted.");
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

            for (int i = 0, len = el.GetNumCrossReferences(); i < len; ++i)
            {
                std::string label = "To " + el.GetCrossReferenceLabel(i);
                if (ImGui::MenuItem(label.c_str()))
                {
                    TryTranslateElementToAnotherElement(m_Shared->UpdCommittableModelGraph(), el.GetID(), el.GetCrossReferenceConnecteeID(i));
                }
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

            if (ImGui::MenuItem("Between two mesh points"))
            {
                TransitionToTranslatingElementAlongTwoMeshPoints(el);
            }

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
            DrawTooltipIfItemHovered("Reorient the scene element", "Rotates the scene element in without changing its position");

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

                    if (ImGui::MenuItem("180 degrees"))
                    {
                        RotateAxis180Degrees(m_Shared->UpdCommittableModelGraph(), el, axis);
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

            if (ImGui::MenuItem("reset"))
            {
                el.SetXform(Transform::atPosition(el.GetPos()));
                m_Shared->CommitCurrentModelGraph("reset " + el.GetLabel() + " orientation");
            }

            ImGui::EndMenu();
        }

        // draw the "Mass" editor for a `BodyEl`
        void DrawMassEditor(BodyEl const& bodyEl)
        {
            float curMass = static_cast<float>(bodyEl.Mass);
            if (ImGui::InputFloat("Mass", &curMass, 0.0f, 0.0f, OSC_FLOAT_INPUT_FORMAT))
            {
                m_Shared->UpdModelGraph().UpdElByID<BodyEl>(bodyEl.ID).Mass = static_cast<double>(curMass);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                m_Shared->CommitCurrentModelGraph("changed body mass");
            }
            ImGui::SameLine();
            DrawHelpMarker("Mass", "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
        }

        // draw the "Joint Type" editor for a `JointEl`
        void DrawJointTypeEditor(JointEl const& jointEl)
        {
            int currentIdx = static_cast<int>(jointEl.JointTypeIndex);
            nonstd::span<char const* const> labels = JointRegistry::nameCStrings();
            if (ImGui::Combo("Joint Type", &currentIdx, labels.data(), static_cast<int>(labels.size())))
            {
                m_Shared->UpdModelGraph().UpdElByID<JointEl>(jointEl.ID).JointTypeIndex = static_cast<size_t>(currentIdx);
                m_Shared->CommitCurrentModelGraph("changed joint type");
            }
            ImGui::SameLine();
            DrawHelpMarker("Joint Type", "This is the type of joint that should be added into the OpenSim model. The joint's type dictates what types of motion are permitted around the joint center. See the official OpenSim documentation for an explanation of each joint type.");
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
                for (int i = 0; i < nRefs; ++i)
                {
                    std::string const& label = el.GetCrossReferenceLabel(i);
                    if (ImGui::MenuItem(label.c_str()))
                    {
                        TransitionToReassigningCrossRef(el, i);
                    }
                }

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
            class Visitor : public SceneElVisitor
            {
            public:
                Visitor(MainUIState& state,
                        glm::vec3 const& clickPos) :
                    m_State{state},
                    m_ClickPos{clickPos}
                {
                }

                void operator()(GroundEl& el) override
                {
                    m_State.DrawContextMenuContent(el, m_ClickPos);
                }

                void operator()(MeshEl& el) override
                {
                    m_State.DrawContextMenuContent(el, m_ClickPos);
                }

                void operator()(BodyEl& el) override
                {
                    m_State.DrawContextMenuContent(el, m_ClickPos);
                }

                void operator()(JointEl& el) override
                {
                    m_State.DrawContextMenuContent(el, m_ClickPos);
                }

                void operator()(StationEl& el) override
                {
                    m_State.DrawContextMenuContent(el, m_ClickPos);
                }
            private:
                MainUIState& m_State;
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
                DrawNothingContextMenuContent();
            }
            else if (m_MaybeOpenedContextMenu.ID == g_RightClickedNothingID)
            {
                // context menu was opened on "nothing" specifically
                DrawNothingContextMenuContent();
            }
            else if (SceneEl* el = m_Shared->UpdModelGraph().TryUpdElByID(m_MaybeOpenedContextMenu.ID))
            {
                // context menu was opened on a scene element that exists in the modelgraph
                DrawContextMenuContent(*el, m_MaybeOpenedContextMenu.Pos);
            }


            // context menu should be closed under these conditions
            if (IsAnyKeyPressed({SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE}))
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

            Sort(commits, orderedByTime);

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

        void DrawHierarchyElement(SceneElClass const& c)
        {
            ModelGraph& mg = m_Shared->UpdModelGraph();

            ImGui::Text("%s %s", c.GetIconCStr(), c.GetNameCStr());
            ImGui::SameLine();
            DrawHelpMarker(c.GetNameCStr(), c.GetDescriptionCStr());
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
                    if (!IsShiftDown())
                    {
                        m_Shared->UpdModelGraph().DeSelectAll();
                    }
                    m_Shared->UpdModelGraph().Select(id);
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    m_MaybeOpenedContextMenu = Hover{id, {}};
                    ImGui::OpenPopup("##maincontextmenu");
                    App::cur().requestRedraw();
                }
            }

            if (empty)
            {
                ImGui::TextDisabled("(no %s)", c.GetNamePluralizedCStr());
            }
            ImGui::Unindent();
        }

        void DrawHierarchyPanelContent()
        {
            for (SceneElClass const* c : GetSceneElClasses())
            {
                DrawHierarchyElement(*c);
                SpacerDummy();
            }

            // a hierarchy element might have opened the context menu in the hierarchy panel
            //
            // this can happen when the user right-clicks something in the hierarchy
            if (ImGui::BeginPopup("##maincontextmenu"))
            {
                DrawContextMenuContent();
                ImGui::EndPopup();
            }
        }

        void DrawAddOtherMenuItems()
        {
            if (ImGui::MenuItem(ICON_FA_CUBE " Meshes"))
            {
                m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
            }
            DrawTooltipIfItemHovered("Add Meshes to the model", OSC_MESH_DESC);

            if (ImGui::MenuItem(ICON_FA_CIRCLE " Body"))
            {
                m_Shared->AddBody({0.0f, 0.0f, 0.0f});
            }
            DrawTooltipIfItemHovered("Add Body at Ground Location", OSC_BODY_DESC);

            if (ImGui::MenuItem(ICON_FA_MAP_PIN " Station"))
            {
                ModelGraph& mg = m_Shared->UpdModelGraph();
                StationEl& e = mg.AddEl<StationEl>(GenerateIDT<StationEl>(), g_GroundID, glm::vec3{}, GenerateName(StationEl::Class()));
                SelectOnly(mg, e);
            }
            DrawTooltipIfItemHovered("Add Station", StationEl::Class().GetDescriptionCStr());
        }

        void Draw3DViewerOverlayTopBar()
        {
            int imguiID = 0;

            if (ImGui::Button(ICON_FA_CUBE " Add Meshes"))
            {
                m_Shared->PromptUserForMeshFilesAndPushThemOntoMeshLoader();
            }
            DrawTooltipIfItemHovered("Add Meshes to the model", OSC_MESH_DESC);

            ImGui::SameLine();

            ImGui::Button(ICON_FA_PLUS " Add Other");
            DrawTooltipIfItemHovered("Add components to the model");

            if (ImGui::BeginPopupContextItem("##additemtoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
            {
                DrawAddOtherMenuItems();
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::Button(ICON_FA_PAINT_ROLLER " Colors");
            DrawTooltipIfItemHovered("Change scene display colors", "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

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
            DrawTooltipIfItemHovered("Change what's visible in the 3D scene", "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

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
            DrawTooltipIfItemHovered("Change what your mouse can interact with in the 3D scene", "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

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

            // translate/rotate/scale dropdown
            {
                char const* modes[] = {"translate", "rotate", "scale"};
                ImGuizmo::OPERATION ops[] = {ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE};
                int currentOp = static_cast<int>(std::distance(std::begin(ops), std::find(std::begin(ops), std::end(ops), m_ImGuizmoState.op)));

                ImGui::SetNextItemWidth(ImGui::CalcTextSize(modes[0]).x + 40.0f);
                if (ImGui::Combo("##opselect", &currentOp, modes, IM_ARRAYSIZE(modes)))
                {
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
                if (ImGui::Combo("##modeselect", &currentMode, modeLabels, IM_ARRAYSIZE(modeLabels)))
                {
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
                if (ImGui::InputFloat("scene scale factor", &sf))
                {
                    m_Shared->SetSceneScaleFactor(sf);
                }
                DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
            }
        }

        void Draw3DViewerOverlayBottomBar()
        {
            // bottom-left axes overlay
            DrawAlignmentAxesOverlayInBottomRightOf(m_Shared->GetCamera().getViewMtx(), m_Shared->Get3DSceneRect());

            Rect sceneRect = m_Shared->Get3DSceneRect();
            glm::vec2 trPos = {sceneRect.p1.x + 100.0f, sceneRect.p2.y - 55.0f};
            ImGui::SetCursorScreenPos(trPos);

            if (ImGui::Button(ICON_FA_SEARCH_MINUS))
            {
                m_Shared->UpdCamera().radius *= 1.2f;
            }
            DrawTooltipIfItemHovered("Zoom Out");

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_SEARCH_PLUS))
            {
                m_Shared->UpdCamera().radius *= 0.8f;
            }
            DrawTooltipIfItemHovered("Zoom In");

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                auto it = m_DrawablesBuffer.begin();
                bool containsAtLeastOne = false;
                AABB aabb;
                while (it != m_DrawablesBuffer.end())
                {
                    if (it->id != g_EmptyID)
                    {
                        aabb = CalcBounds(*it);
                        it++;
                        containsAtLeastOne = true;
                        break;
                    }
                    it++;
                }
                if (containsAtLeastOne)
                {
                    while (it != m_DrawablesBuffer.end())
                    {
                        if (it->id != g_EmptyID)
                        {
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
            DrawTooltipIfItemHovered("Face camera facing along X", "Right-clicking faces it along X, but in the opposite direction");

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
            DrawTooltipIfItemHovered("Face camera facing along Y", "Right-clicking faces it along Y, but in the opposite direction");

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
            DrawTooltipIfItemHovered("Face camera facing along Z", "Right-clicking faces it along Z, but in the opposite direction");

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_CAMERA))
            {
                m_Shared->UpdCamera() = CreateDefaultCamera();
            }
            DrawTooltipIfItemHovered("Reset camera", "Resets the camera to its default position (the position it's in when the wizard is first loaded)");
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
            DrawTooltipIfItemHovered("Convert current scene to an OpenSim Model", "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nThe converter will take what you have laid out on this screen and (internally) convert it into an equivalent OpenSim::Model. The conversion process is one-way: you can't edit the OpenSim model and go back to this screen. However, your progress on this screen is saved. You can return to the mesh importer screen, which will 'remember' its last state, if you want to make any additional changes/edits.");
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

                m_ImGuizmoState.mtx = toMat4(ras);
            }

            // else: is using OR nselected > 0 (so draw it)

            Rect sceneRect = m_Shared->Get3DSceneRect();

            ImGuizmo::SetRect(
                sceneRect.p1.x,
                sceneRect.p1.y,
                RectDims(sceneRect).x,
                RectDims(sceneRect).y);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::AllowAxisFlip(false);  // user's didn't like this feature in UX sessions

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
            m_ImGuizmoState.wasUsingLastFrame = isUsingThisFrame;  // so next frame can know

            // if the user was using the gizmo last frame, and isn't using it this frame,
            // then they probably just finished a manipulation, which should be snapshotted
            // for undo/redo support
            if (wasUsingLastFrame && !isUsingThisFrame)
            {
                m_Shared->CommitCurrentModelGraph("manipulated selection");
                App::cur().requestRedraw();
            }

            // if no manipulation happened this frame, exit early
            if (!manipulated)
            {
                return;
            }

            glm::vec3 translation;
            glm::vec3 rotation;
            glm::vec3 scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(delta), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
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

            bool lcClicked = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
            bool shiftDown = osc::IsShiftDown();
            bool altDown = osc::IsAltDown();
            bool isUsingGizmo = ImGuizmo::IsUsing();

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
                dt.rimColor = CalcRimIntensity(m_Shared->GetModelGraph(), dt.id, m_MaybeHover.ID);
            }

            // draw 3D scene (effectively, as an ImGui::Image)
            m_Shared->DrawScene(sceneEls);
            if (m_Shared->IsRenderHovered() && IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
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
            m_Shared->DrawConnectionLines();
        }

        void DrawMainMenuFileMenu()
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N"))
                {
                    ResetModelGraph(m_Shared->UpdCommittableModelGraph());
                }

                if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W"))
                {
                    App::cur().requestTransition<SplashScreen>();
                }

                if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
                {
                    App::cur().requestQuit();
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
                    if (ImGui::MenuItem(SharedData::g_OpenedPanelNames[i], nullptr, m_Shared->m_PanelStates[i]))
                    {
                        m_Shared->m_PanelStates[i] = !m_Shared->m_PanelStates[i];
                    }
                }
                ImGui::EndMenu();
            }
        }

        void DrawMainMenuAboutMenu()
        {
            MainMenuAboutTab{}.draw();
        }

        // draws main menu at top of screen
        void DrawMainMenu()
        {
            if (ImGui::BeginMainMenuBar())
            {
                DrawMainMenuFileMenu();
                DrawMainMenuEditMenu();
                DrawMainMenuWindowMenu();
                DrawMainMenuAboutMenu();

                ImGui::EndMainMenuBar();
            }
        }

        // draws main 3D viewer, or a modal (if one is active)
        void DrawMainViewerPanelOrModal()
        {
            if (m_Maybe3DViewerModal)
            {
                auto ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall

                // open it "over" the whole UI as a "modal" - so that the user can't click things
                // outside of the panel
                ImGui::OpenPopup("##visualizermodalpopup");
                ImGui::SetNextWindowSize(m_Shared->Get3DSceneDims());
                ImGui::SetNextWindowPos(m_Shared->Get3DSceneRect().p1);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});

                ImGuiWindowFlags modalFlags =
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

        bool onEvent(SDL_Event const& e)
        {
            if (m_Shared->onEvent(e))
            {
                return true;
            }

            if (m_Maybe3DViewerModal)
            {
                auto ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
                if (ptr->onEvent(e))
                {
                    return true;
                }
            }

            if (UpdateFromImGuiKeyboardState())
            {
                return true;
            }

            return false;
        }

        void tick(float dt)
        {
            m_Shared->tick(dt);

            if (!m_Maybe3DViewerModal && m_Shared->IsRenderHovered() && !ImGuizmo::IsUsing())
            {
                UpdatePolarCameraFromImGuiUserInput(m_Shared->Get3DSceneDims(), m_Shared->UpdCamera());
            }

            if (m_Maybe3DViewerModal)
            {
                auto ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
                ptr->tick(dt);
            }
        }

        void draw()
        {
            ImGuizmo::BeginFrame();

            // draw main menu at top of screen
            DrawMainMenu();

            // draw history panel (if enabled)
            if (m_Shared->m_PanelStates[SharedData::PanelIndex_History])
            {
                if (ImGui::Begin("history", &m_Shared->m_PanelStates[SharedData::PanelIndex_History]))
                {
                    DrawHistoryPanelContent();
                }
                ImGui::End();
            }

            // draw hierarchy panel (if enabled)
            if (m_Shared->m_PanelStates[SharedData::PanelIndex_Hierarchy])
            {
                if (ImGui::Begin("hierarchy", &m_Shared->m_PanelStates[SharedData::PanelIndex_Hierarchy]))
                {
                    DrawHierarchyPanelContent();
                }
                ImGui::End();
            }

            // draw log panel (if enabled)
            if (m_Shared->m_PanelStates[SharedData::PanelIndex_Log])
            {
                if (ImGui::Begin("log", &m_Shared->m_PanelStates[SharedData::PanelIndex_Log]))
                {
                    m_Shared->m_Logviewer.draw();
                }
                ImGui::End();
            }

            // draw contextual 3D modal (if there is one), else: draw standard 3D viewer
            DrawMainViewerPanelOrModal();
        }

    private:
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
            glm::mat4 mtx{};
            ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
            ImGuizmo::MODE mode = ImGuizmo::WORLD;
        } m_ImGuizmoState;
    };
}

// top-level screen implementation
//
// this effectively just feeds the underlying state machine pattern established by
// the `ModelWizardState` class
struct osc::MeshImporterScreen::Impl final {
public:
    Impl() :
        m_MainState{std::make_shared<SharedData>()}
    {
    }

    Impl(std::vector<std::filesystem::path> meshPaths) :
        m_MainState{std::make_shared<SharedData>(meshPaths)}
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
        if (osc::ImGuiOnEvent(e))
        {
            m_ShouldRequestRedraw = true;
        }

        m_MainState.onEvent(e);
    }

    void tick(float dt)
    {
        m_MainState.tick(dt);
    }

    void draw()
    {
        // clear the whole screen (it's a full redraw)
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set up ImGui's internal datastructures
        osc::ImGuiNewFrame();

        // enable panel docking
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // draw current state
        m_MainState.draw();

        // draw ImGui
        osc::ImGuiRender();

        // request another draw (e.g. because the state changed during this draw)
        if (m_ShouldRequestRedraw)
        {
            App::cur().requestRedraw();
            m_ShouldRequestRedraw = false;
        }
    }

private:
    MainUIState m_MainState;
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
osc::MeshImporterScreen::Impl* GetModelWizardScreenGLOBAL(std::vector<std::filesystem::path> paths)
{
    static osc::MeshImporterScreen::Impl* g_ModelImpoterScreenState = new osc::MeshImporterScreen::Impl{paths};
    return g_ModelImpoterScreenState;
}

osc::MeshImporterScreen::MeshImporterScreen() :
    m_Impl{GetModelWizardScreenGLOBAL({})}
{
}

osc::MeshImporterScreen::MeshImporterScreen(std::vector<std::filesystem::path> paths) :
    m_Impl{GetModelWizardScreenGLOBAL(paths)}
{
}

osc::MeshImporterScreen::~MeshImporterScreen() noexcept
{
    // HACK: don't delete Impl, because we're sharing it globally
}

void osc::MeshImporterScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MeshImporterScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MeshImporterScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MeshImporterScreen::draw()
{
    m_Impl->draw();
}

void osc::MeshImporterScreen::tick(float dt)
{
    m_Impl->tick(dt);
}
