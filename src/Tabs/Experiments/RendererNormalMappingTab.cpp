#include "RendererNormalMappingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/MeshTopology.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Cpp20Shims.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::vector<glm::vec4> ComputeTangents(
        osc::MeshTopology topology,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<glm::vec3 const> normals,
        nonstd::span<glm::vec2 const> texCoords,
        nonstd::span<uint16_t const> indices)
    {
        // related:
        //
        // *initial source: https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        // https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
        // https://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors
        // https://stackoverflow.com/questions/25349350/calculating-per-vertex-tangents-for-glsl
        // http://www.terathon.com/code/tangent.html
        // http://image.diku.dk/projects/media/morten.mikkelsen.08.pdf
        // http://www.crytek.com/download/Triangle_mesh_tangent_space_calculation.pdf

        std::vector<glm::vec4> rv;

        // edge-case: there's insufficient topological/normal/coordinate data, so
        //            return fallback-filled ({1,0,0,1}) vector
        if (topology != osc::MeshTopology::Triangles ||
            normals.empty() ||
            texCoords.empty())
        {
            rv.assign(verts.size(), {1.0f, 0.0f, 0.0f, 1.0f});
            return rv;
        }

        // else: there must be enough data to compute the tangents
        //
        // (but, just to keep sane, assert that the mesh data is actually valid)
        OSC_ASSERT_ALWAYS(std::all_of(
            indices.begin(),
            indices.end(),
            [nVerts = verts.size(), nNormals = normals.size(), nCoords = texCoords.size()](auto index)
            {
                return index < nVerts && index < nNormals && index < nCoords;
            }) && "the provided mesh contains invalid indices");

        // for smooth shading, vertices, normals, texture coordinates, and tangents
        // may be shared by multiple triangles. In this case, the tangents must be
        // averaged, so:
        //
        // - initialize all tangent vectors to `{0,0,0,0}`s
        // - initialize a weights vector filled with `0`s
        // - every time a tangent vector is computed:
        //     - accumulate a new average: `tangents[i] = (weights[i]*tangents[i] + newTangent)/weights[i]+1;`
        //     - increment weight: `weights[i]++`
        rv.assign(verts.size(), {0.0f, 0.0f, 0.0f, 0.0f});
        std::vector<uint16_t> weights(verts.size(), 0);
        auto const accumulateTangent = [&rv, &weights](auto i, glm::vec4 const& newTangent)
        {
            rv[i] = (static_cast<float>(weights[i])*rv[i] + newTangent)/(static_cast<float>(weights[i]+1));
            weights[i]++;
        };

        // compute tangent vectors from triangle primitives
        for (ptrdiff_t triBegin = 0, end = osc::ssize(indices)-2; triBegin < end; triBegin += 3)
        {
            // compute edge vectors in object and tangent (UV) space
            glm::vec3 const e1 = verts[indices[triBegin+1]] - verts[indices[triBegin+0]];
            glm::vec3 const e2 = verts[indices[triBegin+2]] - verts[indices[triBegin+0]];
            glm::vec2 const dUV1 = texCoords[indices[triBegin+1]] - texCoords[indices[triBegin+0]];  // delta UV for edge 1
            glm::vec2 const dUV2 = texCoords[indices[triBegin+2]] - texCoords[indices[triBegin+0]];

            // this is effectively inline-ing a matrix inversion + multiplication, see:
            //
            // - https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/normal_mapping_tangent.pdf
            // - https://learnopengl.com/Advanced-Lighting/Normal-Mapping
            float const invDeterminant = 1.0f/(dUV1.x*dUV2.y - dUV2.x*dUV1.y);
            glm::vec3 const tangent = invDeterminant * glm::vec3
            {
                dUV2.y*e1.x - dUV1.y*e2.x,
                dUV2.y*e1.y - dUV1.y*e2.y,
                dUV2.y*e1.z - dUV1.y*e2.z,
            };
            glm::vec3 const bitangent = invDeterminant * glm::vec3
            {
                -dUV2.x*e1.x + dUV1.x*e2.x,
                -dUV2.x*e1.y + dUV1.x*e2.y,
                -dUV2.x*e1.z + dUV1.x*e2.z,
            };

            // care: due to smooth shading, each normal may not actually be orthogonal
            // to the triangle's surface
            for (ptrdiff_t iVert = 0; iVert < 3; ++iVert)
            {
                auto const triVertIndex = indices[triBegin + iVert];

                // Gram-Schmidt orthogonalization (w.r.t. the stored normal)
                glm::vec3 const normal = glm::normalize(normals[triVertIndex]);
                glm::vec3 const orthoTangent = glm::normalize(tangent - glm::dot(normal, tangent)*normal);
                glm::vec3 const orthoBitangent = glm::normalize(bitangent - (glm::dot(orthoTangent, bitangent)*orthoTangent) - (glm::dot(normal, bitangent)*normal));

                // this algorithm doesn't produce bitangents. Instead, it writes the
                // "direction" (flip) of the bitangent w.r.t. `cross(normal, tangent)`
                //
                // (the shader can recompute the bitangent from: `cross(normal, tangent) * w`)
                float const w = glm::dot(glm::cross(normal, orthoTangent), orthoBitangent);

                accumulateTangent(triVertIndex, glm::vec4{orthoTangent, w});
            }
        }
        return rv;
    }

    // matches the quad used in LearnOpenGL's normal mapping tutorial
    osc::Mesh GenerateQuad()
    {
        std::vector<glm::vec3> const verts =
        {
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
        };

        std::vector<glm::vec3> const normals =
        {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
        };

        std::vector<glm::vec2> const texCoords =
        {
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
        };

        std::vector<uint16_t> const indices =
        {
            0, 1, 2,
            0, 2, 3,
        };

        std::vector<glm::vec4> const tangents = ComputeTangents(
            osc::MeshTopology::Triangles,
            verts,
            normals,
            texCoords,
            indices
        );
        OSC_ASSERT_ALWAYS(tangents.size() == verts.size());

        osc::Mesh rv;
        rv.setVerts(verts);
        rv.setNormals(normals);
        rv.setTexCoords(texCoords);
        rv.setTangents(tangents);
        rv.setIndices(indices);
        return rv;
    }
}

class osc::RendererNormalMappingTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
        m_NormalMappingMaterial.setTexture("uDiffuseMap", m_DiffuseMap);
        m_NormalMappingMaterial.setTexture("uNormalMap", m_NormalMap);

        // these roughly match what LearnOpenGL default to
        m_Camera.setPosition({0.0f, 0.0f, 3.0f});
        m_Camera.setCameraFOV(glm::radians(45.0f));
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);

        m_LightTransform.position = {0.5f, 1.0f, 0.3f};
        m_LightTransform.scale *= 0.2f;
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {
        m_IsMouseCaptured = true;
    }

    void onUnmount()
    {
        m_IsMouseCaptured = false;
        App::upd().setShowCursor(true);
    }

    bool onEvent(SDL_Event const& e)
    {
        // handle mouse capturing
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            m_IsMouseCaptured = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
        {
            m_IsMouseCaptured = true;
            return true;
        }
        return false;
    }

    void onTick()
    {
        // rotate the quad over time
        AppClock::duration const dt = App::get().getDeltaSinceAppStartup();
        float const angle = glm::radians(-10.0f * dt.count());
        glm::vec3 const axis = glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f});
        m_QuadTransform.rotation = glm::normalize(glm::quat{angle, axis});
    }

    void onDrawMainMenu()
    {
    }

    void onDraw()
    {
        // handle mouse capturing and update camera
        if (m_IsMouseCaptured)
        {
            UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
            App::upd().setShowCursor(false);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            App::upd().setShowCursor(true);
        }

        // clear screen and ensure camera has correct pixel rect
        App::upd().clearScreen({0.1f, 0.1f, 0.1f, 1.0f});

        // draw normal-mapped quad
        {
            m_NormalMappingMaterial.setVec3("uLightWorldPos", m_LightTransform.position);
            m_NormalMappingMaterial.setVec3("uViewWorldPos", m_Camera.getPosition());
            m_NormalMappingMaterial.setBool("uEnableNormalMapping", m_IsNormalMappingEnabled);
            Graphics::DrawMesh(m_QuadMesh, m_QuadTransform, m_NormalMappingMaterial, m_Camera);
        }

        // draw light source
        {
            m_LightCubeMaterial.setVec3("uLightColor", {1.0f, 1.0f, 1.0f});
            Graphics::DrawMesh(m_CubeMesh, m_LightTransform, m_LightCubeMaterial, m_Camera);
        }

        m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();

        ImGui::Begin("controls");
        ImGui::Checkbox("normal mapping", &m_IsNormalMappingEnabled);
        ImGui::End();
    }

private:
    // tab state
    UID m_ID;
    std::string m_Name = ICON_FA_COOKIE " NormalMapping (LearnOpenGL)";
    TabHost* m_Parent;
    bool m_IsMouseCaptured = false;

    // rendering state
    Material m_NormalMappingMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentNormalMapping.vert"),
            App::slurp("shaders/ExperimentNormalMapping.frag"),
        },
    };
    Material m_LightCubeMaterial
    {
        Shader
        {
            App::slurp("shaders/ExperimentLightCube.vert"),
            App::slurp("shaders/ExperimentLightCube.frag"),
        },
    };
    Mesh m_CubeMesh = GenLearnOpenGLCube();
    Mesh m_QuadMesh = GenerateQuad();
    Texture2D m_DiffuseMap = LoadTexture2DFromImage(App::resource("textures/brickwall.jpg"));
    Texture2D m_NormalMap = LoadTexture2DFromImage(App::resource("textures/brickwall_normal.jpg"));

    // scene state
    Camera m_Camera;
    glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
    Transform m_QuadTransform;
    Transform m_LightTransform;
    bool m_IsNormalMappingEnabled = true;
};


// public API (PIMPL)

osc::CStringView osc::RendererNormalMappingTab::id() noexcept
{
    return "Renderer/NormalMapping";
}

osc::RendererNormalMappingTab::RendererNormalMappingTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::RendererNormalMappingTab::RendererNormalMappingTab(RendererNormalMappingTab&&) noexcept = default;
osc::RendererNormalMappingTab& osc::RendererNormalMappingTab::operator=(RendererNormalMappingTab&&) noexcept = default;
osc::RendererNormalMappingTab::~RendererNormalMappingTab() noexcept = default;

osc::UID osc::RendererNormalMappingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererNormalMappingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererNormalMappingTab::implParent() const
{
    return m_Impl->parent();
}

void osc::RendererNormalMappingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererNormalMappingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererNormalMappingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererNormalMappingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererNormalMappingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererNormalMappingTab::implOnDraw()
{
    m_Impl->onDraw();
}
