// these are the things that this file "implements"

#include "src/Graphics/Camera.hpp"
#include "src/Graphics/CameraClearFlags.hpp"
#include "src/Graphics/CameraProjection.hpp"
#include "src/Graphics/DepthStencilFormat.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsContext.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MaterialPropertyBlock.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshTopography.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Graphics/TextureWrapMode.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderType.hpp"

// other includes...

#include "src/Bindings/Gl.hpp"
#include "src/Bindings/GlGlm.hpp"
#include "src/Bindings/GlmHelpers.hpp"
#include "src/Bindings/SDL2Helpers.hpp"
#include "src/Graphics/Image.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Rgba32.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/DefaultConstructOnCopy.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"

#include <GL/glew.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <nonstd/span.hpp>
#include <robin-hood-hashing/robin_hood.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

// vertex shader source used for blitting a textured quad (common use-case)
//
// it's here, rather than in an external resource file, because it is eagerly
// loaded while the graphics backend is initialized (i.e. potentially before
// the application is fully loaded)
static char const g_QuadVertexShaderSrc[] = R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main()
    {
        TexCoord = aTexCoord;
        gl_Position = vec4(aPos, 1.0);
    }
)";

// fragment shader source used for blitting a textured quad
//
// it's here, rather than in an external resource file, because it is eagerly
// loaded while the graphics backend is initialized (i.e. potentially before
// the application is fully loaded)
static char const g_QuadFragmentShaderSrc[] = R"(
    #version 330 core

    uniform sampler2D uTexture;

    in vec2 TexCoord;
    out vec4 FragColor;

    void main()
    {
        FragColor = texture(uTexture, TexCoord);
    }
)";

// generic utility functions
namespace
{
    template<typename T>
    void DoCopyOnWrite(std::shared_ptr<T>& p)
    {
        if (p.use_count() == 1)
        {
            return;  // sole owner: no need to copy
        }

        p = std::make_shared<T>(*p);
    }

    void PushAsBytes(float v, std::vector<std::byte>& out)
    {
        out.reserve(out.size() + sizeof(float));
        for (size_t i = 0; i < sizeof(float); ++i)
        {
            out.push_back(reinterpret_cast<std::byte*>(&v)[i]);
        }
    }

    void PushAsBytes(glm::vec3 const& v, std::vector<std::byte>& out)
    {
        out.reserve(out.size() + sizeof(glm::vec3));
        PushAsBytes(v.x, out);
        PushAsBytes(v.y, out);
        PushAsBytes(v.z, out);
    }

    void PushAsBytes(glm::vec2 const& v, std::vector<std::byte>& out)
    {
        out.reserve(out.size() + sizeof(glm::vec2));
        PushAsBytes(v.x, out);
        PushAsBytes(v.y, out);
    }

    void PushAsBytes(osc::Rgba32 const& c, std::vector<std::byte>& out)
    {
        out.reserve(out.size() + sizeof(osc::Rgba32));
        out.push_back(static_cast<std::byte>(c.r));
        out.push_back(static_cast<std::byte>(c.g));
        out.push_back(static_cast<std::byte>(c.b));
        out.push_back(static_cast<std::byte>(c.a));
    }
}

// material value storage
//
// materials can store a variety of stuff (colors, positions, offsets, textures, etc.). This
// code defines how it's actually stored at runtime
namespace
{
    using MaterialValue = std::variant<
        float,
        std::vector<float>,
        glm::vec2,
        glm::vec3,
        std::vector<glm::vec3>,
        glm::vec4,
        glm::mat3,
        glm::mat4,
        int,
        bool,
        osc::Texture2D,
        osc::RenderTexture
    >;

    osc::ShaderType GetShaderType(MaterialValue const& v)
    {
        using osc::VariantIndex;

        switch (v.index())
        {
        case VariantIndex<MaterialValue, glm::vec2>():
            return osc::ShaderType::Vec2;
        case VariantIndex<MaterialValue, float>():
        case VariantIndex<MaterialValue, std::vector<float>>():
            return osc::ShaderType::Float;
        case VariantIndex<MaterialValue, glm::vec3>():
        case VariantIndex<MaterialValue, std::vector<glm::vec3>>():
            return osc::ShaderType::Vec3;
        case VariantIndex<MaterialValue, glm::vec4>():
            return osc::ShaderType::Vec4;
        case VariantIndex<MaterialValue, glm::mat3>():
            return osc::ShaderType::Mat3;
        case VariantIndex<MaterialValue, glm::mat4>():
            return osc::ShaderType::Mat4;
        case VariantIndex<MaterialValue, int>():
            return osc::ShaderType::Int;
        case VariantIndex<MaterialValue, bool>():
            return osc::ShaderType::Bool;
        case VariantIndex<MaterialValue, osc::Texture2D>():
        case VariantIndex<MaterialValue, osc::RenderTexture>():
            return osc::ShaderType::Sampler2D;
        default:
            return osc::ShaderType::Unknown;
        }
    }
}

// shader (backend stuff)
namespace
{
    // LUT for human-readable form of the above
    static constexpr auto const g_ShaderTypeInternalStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::ShaderType::TOTAL)>(
        "Float",
        "Vec2",
        "Vec3",
        "Vec4",
        "Mat3",
        "Mat4",
        "Int",
        "Bool",
        "Sampler2D",
        "Unknown"
    );

    // convert a GL shader type to an internal shader type
    osc::ShaderType GLShaderTypeToShaderTypeInternal(GLenum e)
    {
        switch (e)
        {
        case GL_FLOAT:
            return osc::ShaderType::Float;
        case GL_FLOAT_VEC2:
            return osc::ShaderType::Vec2;
        case GL_FLOAT_VEC3:
            return osc::ShaderType::Vec3;
        case GL_FLOAT_VEC4:
            return osc::ShaderType::Vec4;
        case GL_FLOAT_MAT3:
            return osc::ShaderType::Mat3;
        case GL_FLOAT_MAT4:
            return osc::ShaderType::Mat4;
        case GL_INT:
            return osc::ShaderType::Int;
        case GL_BOOL:
            return osc::ShaderType::Bool;
        case GL_SAMPLER_2D:
            return osc::ShaderType::Sampler2D;
        case GL_INT_VEC2:
        case GL_INT_VEC3:
        case GL_INT_VEC4:
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_VEC2:
        case GL_UNSIGNED_INT_VEC3:
        case GL_UNSIGNED_INT_VEC4:
        case GL_DOUBLE:
        case GL_DOUBLE_VEC2:
        case GL_DOUBLE_VEC3:
        case GL_DOUBLE_VEC4:
        case GL_DOUBLE_MAT2:
        case GL_DOUBLE_MAT3:
        case GL_DOUBLE_MAT4:
        case GL_DOUBLE_MAT2x3:
        case GL_DOUBLE_MAT2x4:
        case GL_FLOAT_MAT2x3:
        case GL_FLOAT_MAT2x4:
        case GL_FLOAT_MAT3x2:
        case GL_FLOAT_MAT3x4:
        case GL_FLOAT_MAT4x2:
        case GL_FLOAT_MAT4x3:
        case GL_FLOAT_MAT2:
        default:
            return osc::ShaderType::Unknown;
        }
    }

    std::string NormalizeShaderElementName(char const* name)
    {
        std::string s{name};
        auto loc = s.find('[');
        if (loc != std::string::npos)
        {
            s.erase(loc);
        }
        return s;
    }

    // parsed-out description of a shader "element" (uniform/attribute)
    struct ShaderElement final {
        ShaderElement(int location_, osc::ShaderType type_, int size_) :
            Location{std::move(location_)},
            Type{std::move(type_)},
            Size{std::move(size_)}
        {
        }

        int Location;
        osc::ShaderType Type;
        int Size;
    };

    void PrintShaderElement(std::ostream& o, std::string_view name, ShaderElement const& se)
    {
        o << "ShadeElement(name = " << name << ", location = " << se.Location << ", type = " << se.Type << ", size = " << se.Size << ')';
    }

    template<typename Key>
    ShaderElement const* TryGetValue(robin_hood::unordered_map<std::string, ShaderElement> const& m, Key const& k)
    {
        auto const it = m.find(k);
        return it != m.end() ? &it->second : nullptr;
    }
}

namespace
{
    // transform storage: either as a matrix or a transform
    //
    // calling code is allowed to submit transforms as either osc::Transform (preferred) or
    // glm::mat4 (can be handier)
    //
    // these need to be stored as-is, because that's the smallest possible representation and
    // the drawing algorithm needs to traverse + sort the render objects at runtime (so size
    // is important)
    using Mat4OrTransform = std::variant<glm::mat4, osc::Transform>;

    glm::mat4 ToMat4(Mat4OrTransform const& matrixOrTransform)
    {
        return std::visit(osc::Overload
        {
            [](glm::mat4 const& matrix) { return matrix; },
            [](osc::Transform const& transform) { return ToMat4(transform); }
        }, matrixOrTransform);
    }

    glm::mat4 ToNormalMat4(Mat4OrTransform const& matrixOrTransform)
    {
        return std::visit(osc::Overload
        {
            [](glm::mat4 const& matrix) { return osc::ToNormalMatrix4(matrix); },
            [](osc::Transform const& transform) { return osc::ToNormalMatrix4(transform); }
        }, matrixOrTransform);
    }

    glm::mat4 ToNormalMat3(Mat4OrTransform const& matrixOrTransform)
    {
        return std::visit(osc::Overload
        {
            [](glm::mat4 const& matrix) { return osc::ToNormalMatrix(matrix); },
            [](osc::Transform const& transform) { return osc::ToNormalMatrix(transform); }
        }, matrixOrTransform);
    }

    // this is what is stored in the renderer's render queue
    struct RenderObject final {

        RenderObject(
            osc::Mesh const& mesh_,
            osc::Transform const& transform_,
            osc::Material const& material_,
            std::optional<osc::MaterialPropertyBlock> maybePropBlock_) :

            mesh{mesh_},
            transform{transform_},
            worldMidpoint{osc::TransformPoint(transform_, mesh.getMidpoint())},
            material{material_},
            maybePropBlock{std::move(maybePropBlock_)}
        {
        }

        RenderObject(
            osc::Mesh const& mesh_,
            glm::mat4 const& transform_,
            osc::Material const& material_,
            std::optional<osc::MaterialPropertyBlock> maybePropBlock_) :

            mesh{mesh_},
            transform{transform_},
            worldMidpoint{transform_ * glm::vec4{mesh.getMidpoint(), 1.0f}},
            material{material_},
            maybePropBlock{std::move(maybePropBlock_)}
        {
        }

        osc::Mesh mesh;
        Mat4OrTransform transform;
        glm::vec3 worldMidpoint;
        osc::Material material;
        std::optional<osc::MaterialPropertyBlock> maybePropBlock;
    };

    // returns true if the render object is opaque
    bool IsOpaque(RenderObject const& ro)
    {
        return !ro.material.getTransparent();
    }

    bool IsDepthTested(RenderObject const& ro)
    {
        return ro.material.getDepthTested();
    }

    glm::mat4 ModelMatrix(RenderObject const& ro)
    {
        return ToMat4(ro.transform);
    }

    glm::mat3 NormalMatrix(RenderObject const& ro)
    {
        return ToNormalMat3(ro.transform);
    }

    glm::mat4 NormalMatrix4(RenderObject const& ro)
    {
        return ToNormalMat4(ro.transform);
    }

    glm::vec3 const& WorldMidpoint(RenderObject const& ro)
    {
        return ro.worldMidpoint;
    }

    // function object that returns true if the first argument is farther from the second
    //
    // (handy for scene sorting)
    struct RenderObjectIsFartherFrom final {
        RenderObjectIsFartherFrom(glm::vec3 const& pos) : m_Pos{pos} {}

        bool operator()(RenderObject const& a, RenderObject const& b) const
        {
            glm::vec3 const aMidpointWorldSpace = WorldMidpoint(a);
            glm::vec3 const bMidpointWorldSpace = WorldMidpoint(b);
            glm::vec3 const camera2a = aMidpointWorldSpace - m_Pos;
            glm::vec3 const camera2b = bMidpointWorldSpace - m_Pos;
            float const camera2aDistanceSquared = glm::dot(camera2a, camera2a);
            float const camera2bDistanceSquared = glm::dot(camera2b, camera2b);
            return camera2aDistanceSquared > camera2bDistanceSquared;
        }
    private:
        glm::vec3 m_Pos;
    };

    struct RenderObjectHasMaterial final {
        RenderObjectHasMaterial(osc::Material const& material) : m_Material{&material} {}

        bool operator()(RenderObject const& ro) const
        {
            return ro.material == *m_Material;
        }
    private:
        osc::Material const* m_Material;
    };

    struct RenderObjectHasMaterialPropertyBlock final {
        RenderObjectHasMaterialPropertyBlock(std::optional<osc::MaterialPropertyBlock> const& mpb) : m_Mpb{&mpb} {}

        bool operator()(RenderObject const& ro) const
        {
            return ro.maybePropBlock == *m_Mpb;
        }

    private:
        std::optional<osc::MaterialPropertyBlock> const* m_Mpb;
    };

    struct RenderObjectHasMesh final {
        RenderObjectHasMesh(osc::Mesh const& mesh) : m_Mesh{mesh} {}

        bool operator()(RenderObject const& ro) const
        {
            return ro.mesh == m_Mesh;
        }
    private:
        osc::Mesh const& m_Mesh;
    };

    // sort a sequence of `RenderObject`s for optimal drawing
    std::vector<RenderObject>::iterator SortRenderQueue(std::vector<RenderObject>::iterator begin, std::vector<RenderObject>::iterator end, glm::vec3 cameraPos)
    {
        // split queue into [opaque | transparent]
        auto const opaqueEnd = std::partition(begin, end, IsOpaque);

        // optimize the opaque partition (it can be reordered safely)
        {
            // first, sub-parititon by material (top-level batch)
            auto materialBatchStart = begin;
            while (materialBatchStart != opaqueEnd)
            {
                auto const materialBatchEnd = std::partition(materialBatchStart, opaqueEnd, RenderObjectHasMaterial{materialBatchStart->material});

                // then sub-sub-partition by material property block
                auto propBatchStart = materialBatchStart;
                while (propBatchStart != materialBatchEnd)
                {
                    auto const propBatchEnd = std::partition(propBatchStart, materialBatchEnd, RenderObjectHasMaterialPropertyBlock{propBatchStart->maybePropBlock});

                    // then sub-sub-sub-partition by mesh
                    auto meshBatchStart = propBatchStart;
                    while (meshBatchStart != propBatchEnd)
                    {
                        auto const meshBatchEnd = std::partition(meshBatchStart, propBatchEnd, RenderObjectHasMesh{meshBatchStart->mesh});
                        meshBatchStart = meshBatchEnd;
                    }
                    propBatchStart = propBatchEnd;
                }
                materialBatchStart = materialBatchEnd;
            }
        }

        // sort the transparent partition by distance from camera (back-to-front)
        std::sort(opaqueEnd, end, RenderObjectIsFartherFrom{cameraPos});

        return opaqueEnd;
    }

    // top-level state for a "scene" (i.e. a render)
    struct SceneState final {
        glm::vec3 CameraPos;
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

        SceneState(
            glm::vec3 const& cameraPos_,
            glm::mat4 const& viewMatrix_,
            glm::mat4 const& projectionMatrix_) :

            CameraPos{cameraPos_},
            ViewMatrix{viewMatrix_},
            ProjectionMatrix{projectionMatrix_}
        {
        }
    };

    // the OpenGL data associated with an osc::Texture2D
    struct Texture2DOpenGLData final {
        gl::Texture2D Texture;
        osc::UID TextureParamsVersion;
    };

    // the OpenGL data associated with an osc::RenderTexture
    struct RenderTextureOpenGLData final {
        gl::FrameBuffer MultisampledFBO;
        gl::RenderBuffer MultisampledColorBuffer;
        gl::RenderBuffer MultisampledDepthBuffer;
        gl::FrameBuffer SingleSampledFBO;
        gl::Texture2D SingleSampledColorBuffer;
        gl::Texture2D SingleSampledDepthBuffer;
    };

    struct BufferElementDescription final {
        GLuint AttributeIndex;
        GLint NumComponents;
        GLenum ComponentType;
        GLboolean IsNormalized;
        GLsizei Stride;
        size_t Offset;
    };

    // the OpenGL data associated with an osc::Mesh
    struct MeshOpenGLData final {
        osc::UID DataVersion;
        gl::TypedBufferHandle<GL_ARRAY_BUFFER> ArrayBuffer;
        gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER> IndicesBuffer;
        std::vector<BufferElementDescription> BufferDescription;
    };

    void BindAndEnable(MeshOpenGLData const& data)
    {
        gl::BindBuffer(GL_ARRAY_BUFFER, data.ArrayBuffer);
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.IndicesBuffer);

        for (BufferElementDescription const& desc : data.BufferDescription)
        {
            glVertexAttribPointer(desc.AttributeIndex, desc.NumComponents, desc.ComponentType, desc.IsNormalized, desc.Stride, reinterpret_cast<void*>(static_cast<uintptr_t>(desc.Offset)));
            glEnableVertexAttribArray(desc.AttributeIndex);
        }
    }

    void UnbindAndDisable(MeshOpenGLData const& data)
    {
        for (BufferElementDescription const& desc : data.BufferDescription)
        {
            glDisableVertexAttribArray(desc.AttributeIndex);
        }
    }

    struct InstancingState final {
        InstancingState(size_t stride) :
            Stride{std::move(stride)}
        {
        }

        gl::ArrayBuffer<float> Buf;
        size_t Stride = 0;
        size_t BaseOffset = 0;
    };
}


//////////////////////////////////
//
// backend declaration
//
//////////////////////////////////

namespace osc
{
    class GraphicsBackend final {
    public:
        static void BindToInstancedAttributes(
            Shader::Impl const& shaderImpl,
            InstancingState& ins
        );

        static void UnbindFromInstancedAttributes(
            Shader::Impl const& shaderImpl,
            InstancingState& ins
        );

        static void HandleBatchWithSameMesh(
            nonstd::span<RenderObject const>,
            std::optional<InstancingState>& ins
        );

        static void HandleBatchWithSameMaterialPropertyBlock(
            nonstd::span<RenderObject const>,
            int& textureSlot,
            std::optional<InstancingState>& ins
        );

        static std::optional<InstancingState> UploadInstanceData(
            nonstd::span<RenderObject const>,
            osc::Shader::Impl const& shaderImpl
        );

        static void TryBindMaterialValueToShaderElement(
            ShaderElement const& se,
            MaterialValue const& v,
            int* textureSlot
        );

        static void HandleBatchWithSameMaterial(
            SceneState const&,
            nonstd::span<RenderObject const>
        );

        static void DrawBatchedByMaterial(
            SceneState const&,
            nonstd::span<RenderObject const>
        );

        static void DrawBatchedByOpaqueness(
            SceneState const&,
            nonstd::span<RenderObject const>
        );

        static void FlushRenderQueue(
            Camera::Impl& camera
        );

        static void RenderScene(
            Camera::Impl& camera
        );

        static void DrawMesh(
            Mesh const&,
            Transform const&,
            Material const&,
            Camera&,
            std::optional<MaterialPropertyBlock>
        );

        static void DrawMesh(
            Mesh const&,
            glm::mat4 const&,
            Material const&,
            Camera&,
            std::optional<MaterialPropertyBlock>
        );

        static void BlitToScreen(
            RenderTexture const&,
            Rect const&,
            osc::BlitFlags
        );

        static void BlitToScreen(
            RenderTexture const&,
            Rect const&,
            Material const&,
            osc::BlitFlags
        );

        static void Blit(
            Texture2D const&,
            RenderTexture& dest
        );

        static void ReadPixels(
            RenderTexture const&,
            Image& dest
        );
    };
}


//////////////////////////////////
//
// texture stuff
//
//////////////////////////////////

namespace
{
    static constexpr auto const g_TextureWrapModeStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::TextureWrapMode::TOTAL)>
    (
        "Repeat",
        "Clamp",
        "Mirror"
    );

    static constexpr auto const g_TextureFilterModeStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::TextureFilterMode::TOTAL)>
    (
        "Nearest",
        "Linear",
        "Mipmap"
    );

    GLint ToGLTextureFilterParam(osc::TextureFilterMode m)
    {
        switch (m)
        {
        case osc::TextureFilterMode::Nearest:
            return GL_NEAREST;
        case osc::TextureFilterMode::Linear:
            return GL_LINEAR;
        case osc::TextureFilterMode::Mipmap:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            return GL_LINEAR;
        }
    }

    GLint ToGLTextureTextureWrapParam(osc::TextureWrapMode m)
    {
        switch (m)
        {
        case osc::TextureWrapMode::Repeat:
            return GL_REPEAT;
        case osc::TextureWrapMode::Clamp:
            return GL_CLAMP_TO_EDGE;
        case osc::TextureWrapMode::Mirror:
            return GL_MIRRORED_REPEAT;
        default:
            return GL_REPEAT;
        }
    }
}

class osc::Texture2D::Impl final {
public:
    Impl(glm::ivec2 dimensions, nonstd::span<Rgba32 const> pixelsRowByRow) :
        Impl{std::move(dimensions), {&pixelsRowByRow[0].r, 4 * pixelsRowByRow.size()}, 4}
    {
    }

    Impl(glm::ivec2 dimensions, nonstd::span<uint8_t const> pixelsRowByRow) :
        Impl{std::move(dimensions), std::move(pixelsRowByRow), 1}
    {
    }

    Impl(glm::ivec2 dimensions, nonstd::span<uint8_t const> channels, int numChannels) :
        m_Dimensions{std::move(dimensions)},
        m_Pixels(channels.data(), channels.data() + channels.size()),
        m_NumChannels{numChannels}
    {
        OSC_ASSERT_ALWAYS(m_Dimensions.x >= 0 && m_Dimensions.y >= 0);
        OSC_ASSERT_ALWAYS(m_Dimensions.x * m_Dimensions.y == m_Pixels.size()/m_NumChannels);
        OSC_ASSERT_ALWAYS(m_NumChannels == 1 || m_NumChannels == 3 || m_NumChannels == 4);
    }

    glm::ivec2 getDimensions() const
    {
        return m_Dimensions;
    }

    float getAspectRatio() const
    {
        return AspectRatio(m_Dimensions);
    }

    TextureWrapMode getWrapMode() const
    {
        return getWrapModeU();
    }

    void setWrapMode(TextureWrapMode twm)
    {
        setWrapModeU(twm);
        setWrapModeV(twm);
        setWrapModeW(twm);
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeU() const
    {
        return m_WrapModeU;
    }

    void setWrapModeU(TextureWrapMode twm)
    {
        m_WrapModeU = std::move(twm);
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeV() const
    {
        return m_WrapModeV;
    }

    void setWrapModeV(TextureWrapMode twm)
    {
        m_WrapModeV = std::move(twm);
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeW() const
    {
        return m_WrapModeW;
    }

    void setWrapModeW(TextureWrapMode twm)
    {
        m_WrapModeW = std::move(twm);
        m_TextureParamsVersion.reset();
    }

    TextureFilterMode getFilterMode() const
    {
        return m_FilterMode;
    }

    void setFilterMode(TextureFilterMode tfm)
    {
        m_FilterMode = std::move(tfm);
        m_TextureParamsVersion.reset();
    }

    void* updTextureHandleHACK()
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(updTexture().get()));
    }

    // non PIMPL method

    gl::Texture2D& updTexture()
    {
        if (!*m_MaybeGPUTexture)
        {
            uploadToGPU();
        }
        OSC_ASSERT(*m_MaybeGPUTexture);

        Texture2DOpenGLData& bufs = **m_MaybeGPUTexture;

        if (bufs.TextureParamsVersion != m_TextureParamsVersion)
        {
            setTextureParams(bufs);
        }

        return bufs.Texture;
    }

private:
    void uploadToGPU()
    {
        *m_MaybeGPUTexture = Texture2DOpenGLData{};

        GLenum format = GL_RGBA;
        switch (m_NumChannels)
        {
        case 1:
            format = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        }

        // one-time upload, because pixels cannot be altered
        gl::BindTexture((*m_MaybeGPUTexture)->Texture);
        gl::TexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            m_Dimensions.x,
            m_Dimensions.y,
            0,
            format,
            GL_UNSIGNED_BYTE,
            m_Pixels.data()
        );
        glGenerateMipmap((*m_MaybeGPUTexture)->Texture.type);
        gl::BindTexture();
    }

    void setTextureParams(Texture2DOpenGLData& bufs)
    {
        gl::BindTexture(bufs.Texture);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ToGLTextureTextureWrapParam(m_WrapModeU));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ToGLTextureTextureWrapParam(m_WrapModeV));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, ToGLTextureTextureWrapParam(m_WrapModeW));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ToGLTextureFilterParam(m_FilterMode));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ToGLTextureFilterParam(m_FilterMode));
        gl::BindTexture();
        bufs.TextureParamsVersion = m_TextureParamsVersion;
    }

    friend class GraphicsBackend;

    glm::ivec2 m_Dimensions;
    std::vector<uint8_t> m_Pixels;
    int m_NumChannels;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Nearest;
    UID m_TextureParamsVersion;

    DefaultConstructOnCopy<std::optional<Texture2DOpenGLData>> m_MaybeGPUTexture;
};

std::ostream& osc::operator<<(std::ostream& o, TextureWrapMode twm)
{
    return o << g_TextureWrapModeStrings.at(static_cast<size_t>(twm));
}

std::ostream& osc::operator<<(std::ostream& o, TextureFilterMode twm)
{
    return o << g_TextureFilterModeStrings.at(static_cast<size_t>(twm));
}


osc::Texture2D::Texture2D(glm::ivec2 dimensions, nonstd::span<Rgba32 const> pixels) :
    m_Impl{std::make_shared<Impl>(std::move(dimensions), std::move(pixels))}
{
}

osc::Texture2D::Texture2D(glm::ivec2 dimensions, nonstd::span<uint8_t const> pixels) :
    m_Impl{std::make_shared<Impl>(std::move(dimensions), std::move(pixels))}
{
}

osc::Texture2D::Texture2D(glm::ivec2 dimensions, nonstd::span<uint8_t const> channels, int numChannels) :
    m_Impl{std::make_shared<Impl>(std::move(dimensions), std::move(channels), std::move(numChannels))}
{
}

osc::Texture2D::Texture2D(Texture2D const&) = default;
osc::Texture2D::Texture2D(Texture2D&&) noexcept = default;
osc::Texture2D& osc::Texture2D::operator=(Texture2D const&) = default;
osc::Texture2D& osc::Texture2D::operator=(Texture2D&&) noexcept = default;
osc::Texture2D::~Texture2D() noexcept = default;

glm::ivec2 osc::Texture2D::getDimensions() const
{
    return m_Impl->getDimensions();
}

float osc::Texture2D::getAspectRatio() const
{
    return m_Impl->getAspectRatio();
}

osc::TextureWrapMode osc::Texture2D::getWrapMode() const
{
    return m_Impl->getWrapMode();
}

void osc::Texture2D::setWrapMode(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapMode(std::move(twm));
}

osc::TextureWrapMode osc::Texture2D::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::Texture2D::setWrapModeU(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeU(std::move(twm));
}

osc::TextureWrapMode osc::Texture2D::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::Texture2D::setWrapModeV(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeV(std::move(twm));
}

osc::TextureWrapMode osc::Texture2D::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::Texture2D::setWrapModeW(TextureWrapMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeW(std::move(twm));
}

osc::TextureFilterMode osc::Texture2D::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::Texture2D::setFilterMode(TextureFilterMode twm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFilterMode(std::move(twm));
}

void* osc::Texture2D::updTextureHandleHACK()
{
    DoCopyOnWrite(m_Impl);
    return m_Impl->updTextureHandleHACK();
}

bool osc::operator==(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, Texture2D const&)
{
    return o << "Texture2D()";
}

osc::Texture2D osc::LoadTexture2DFromImageResource(std::string_view resource, ImageFlags flags)
{
    Image const img = Image::Load(App::get().resource(resource), flags);
    return Texture2D{img.getDimensions(), img.getPixelData(), img.getNumChannels()};
}


//////////////////////////////////
//
// render texture
//
//////////////////////////////////

namespace
{
    static constexpr auto const  g_RenderTextureFormatStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::RenderTextureFormat::TOTAL)>
    (
        "ARGB32",
        "RED"
    );

    static constexpr auto const g_DepthStencilFormatStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::DepthStencilFormat::TOTAL)>
    (
        "D24_UNorm_S8_UInt"
    );

    GLenum ToOpenGLColorFormat(osc::RenderTextureFormat f)
    {
        switch (f)
        {
        case osc::RenderTextureFormat::ARGB32:
            return GL_RGBA;
        case osc::RenderTextureFormat::RED:
        default:
            static_assert(static_cast<int>(osc::RenderTextureFormat::RED) + 1 == static_cast<int>(osc::RenderTextureFormat::TOTAL));
            return GL_RED;
        }
    }

    int GetNumChannels(osc::RenderTextureFormat f)
    {
        switch (f)
        {
        case osc::RenderTextureFormat::ARGB32:
            return 4;
        case osc::RenderTextureFormat::RED:
            return 1;
        default:
            static_assert(static_cast<int>(osc::RenderTextureFormat::TOTAL) == 2);
            return 1;
        }
    }
}

std::ostream& osc::operator<<(std::ostream& o, RenderTextureFormat f)
{
    return o << g_RenderTextureFormatStrings.at(static_cast<size_t>(f));
}

std::ostream& osc::operator<<(std::ostream& o, DepthStencilFormat f)
{
    return o << g_DepthStencilFormatStrings.at(static_cast<size_t>(f));
}

osc::RenderTextureDescriptor::RenderTextureDescriptor(glm::ivec2 dimensions) :
    m_Dimensions{Max(dimensions, glm::ivec2{0, 0})},
    m_AnialiasingLevel{1},
    m_ColorFormat{RenderTextureFormat::ARGB32},
    m_DepthStencilFormat{DepthStencilFormat::D24_UNorm_S8_UInt}
{
}

osc::RenderTextureDescriptor::RenderTextureDescriptor(RenderTextureDescriptor const&) = default;
osc::RenderTextureDescriptor::RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept = default;
osc::RenderTextureDescriptor& osc::RenderTextureDescriptor::operator=(RenderTextureDescriptor const&) = default;
osc::RenderTextureDescriptor& osc::RenderTextureDescriptor::operator=(RenderTextureDescriptor&&) noexcept = default;
osc::RenderTextureDescriptor::~RenderTextureDescriptor() noexcept = default;

glm::ivec2 osc::RenderTextureDescriptor::getDimensions() const
{
    return m_Dimensions;
}

void osc::RenderTextureDescriptor::setDimensions(glm::ivec2 d)
{
    OSC_ASSERT_ALWAYS(d.x >= 0 && d.y >= 0);
    m_Dimensions = d;
}

int osc::RenderTextureDescriptor::getAntialiasingLevel() const
{
    return m_AnialiasingLevel;
}

void osc::RenderTextureDescriptor::setAntialiasingLevel(int level)
{
    OSC_ASSERT_ALWAYS(level <= 64 && osc::NumBitsSetIn(level) == 1);
    m_AnialiasingLevel = level;
}

osc::RenderTextureFormat osc::RenderTextureDescriptor::getColorFormat() const
{
    return m_ColorFormat;
}

void osc::RenderTextureDescriptor::setColorFormat(RenderTextureFormat f)
{
    m_ColorFormat = f;
}

osc::DepthStencilFormat osc::RenderTextureDescriptor::getDepthStencilFormat() const
{
    return m_DepthStencilFormat;
}

void osc::RenderTextureDescriptor::setDepthStencilFormat(DepthStencilFormat f)
{
    m_DepthStencilFormat = f;
}

bool osc::operator==(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return
        a.m_Dimensions == b.m_Dimensions &&
        a.m_AnialiasingLevel == b.m_AnialiasingLevel &&
        a.m_ColorFormat == b.m_ColorFormat &&
        a.m_DepthStencilFormat == b.m_DepthStencilFormat;
}

bool osc::operator!=(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return !(a == b);
}

bool osc::operator<(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return std::tie(a.m_Dimensions.x, a.m_Dimensions.y, a.m_AnialiasingLevel, a.m_ColorFormat, a.m_DepthStencilFormat) < std::tie(b.m_Dimensions.x, a.m_Dimensions.y, b.m_AnialiasingLevel, b.m_ColorFormat, b.m_DepthStencilFormat);
}

std::ostream& osc::operator<<(std::ostream& o, RenderTextureDescriptor const& rtd)
{
    return o << "RenderTextureDescriptor(width = " << rtd.m_Dimensions.x << ", height = " << rtd.m_Dimensions.y << ", aa = " << rtd.m_AnialiasingLevel << ", colorFormat = " << rtd.m_ColorFormat << ", depthFormat = " << rtd.m_DepthStencilFormat << ")";
}

class osc::RenderTexture::Impl final {
public:
    Impl(RenderTextureDescriptor const& desc) : m_Descriptor{desc}
    {
    }

    glm::ivec2 getDimensions() const
    {
        return m_Descriptor.getDimensions();
    }

    void setDimensions(glm::ivec2 d)
    {
        if (d != getDimensions())
        {
            m_Descriptor.setDimensions(d);
            m_MaybeGPUBuffers->reset();
        }
    }

    RenderTextureFormat getColorFormat() const
    {
        return m_Descriptor.getColorFormat();
    }

    void setColorFormat(RenderTextureFormat format)
    {
        if (format != m_Descriptor.getColorFormat())
        {
            m_Descriptor.setColorFormat(format);
            m_MaybeGPUBuffers->reset();
        }
    }

    int getAntialiasingLevel() const
    {
        return m_Descriptor.getAntialiasingLevel();
    }

    void setAntialiasingLevel(int level)
    {
        if (level != m_Descriptor.getAntialiasingLevel())
        {
            m_Descriptor.setAntialiasingLevel(level);
            m_MaybeGPUBuffers->reset();
        }
    }

    DepthStencilFormat getDepthStencilFormat() const
    {
        return m_Descriptor.getDepthStencilFormat();
    }

    void setDepthStencilFormat(DepthStencilFormat format)
    {
        if (format != m_Descriptor.getDepthStencilFormat())
        {
            m_Descriptor.setDepthStencilFormat(format);
            m_MaybeGPUBuffers->reset();
        }
    }

    void reformat(RenderTextureDescriptor const& d)
    {
        if (d != m_Descriptor)
        {
            m_Descriptor = d;
            m_MaybeGPUBuffers->reset();
        }
    }

    void* updTextureHandleHACK()
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(getOutputTexture().get()));
    }

private:
    gl::FrameBuffer& getFrameBuffer()
    {
        if (!*m_MaybeGPUBuffers)
        {
            uploadToGPU();
        }
        return (*m_MaybeGPUBuffers)->MultisampledFBO;
    }

    gl::FrameBuffer& getOutputFrameBuffer()
    {
        if (!*m_MaybeGPUBuffers)
        {
            uploadToGPU();
        }
        return (*m_MaybeGPUBuffers)->SingleSampledFBO;
    }

    gl::Texture2D& getOutputTexture()
    {
        if (!*m_MaybeGPUBuffers)
        {
            uploadToGPU();
        }
        return (*m_MaybeGPUBuffers)->SingleSampledColorBuffer;
    }

    void uploadToGPU()
    {
        RenderTextureOpenGLData& bufs = m_MaybeGPUBuffers->emplace();
        glm::ivec2 const dimensions = m_Descriptor.getDimensions();

        // setup MSXAAed color buffer
        gl::BindRenderBuffer(bufs.MultisampledColorBuffer);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            m_Descriptor.getAntialiasingLevel(),
            ToOpenGLColorFormat(getColorFormat()),
            dimensions.x,
            dimensions.y
        );
        gl::BindRenderBuffer();

        // setup MSXAAed depth buffer
        gl::BindRenderBuffer(bufs.MultisampledDepthBuffer);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            m_Descriptor.getAntialiasingLevel(),
            GL_DEPTH24_STENCIL8,
            dimensions.x,
            dimensions.y
        );
        gl::BindRenderBuffer();

        // setup MSXAAed framebuffer (color+depth)
        gl::BindFramebuffer(GL_FRAMEBUFFER, bufs.MultisampledFBO);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufs.MultisampledColorBuffer);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, bufs.MultisampledDepthBuffer);

        // setup single-sampled color buffer (texture, so it can be sampled as part of compositing a UI)
        gl::BindTexture(bufs.SingleSampledColorBuffer);
        gl::TexImage2D(
            bufs.SingleSampledColorBuffer.type,
            0,
            ToOpenGLColorFormat(getColorFormat()),
            dimensions.x,
            dimensions.y,
            0,
            ToOpenGLColorFormat(getColorFormat()),
            GL_UNSIGNED_BYTE,
            nullptr
        );
        gl::TexParameteri(bufs.SingleSampledColorBuffer.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(bufs.SingleSampledColorBuffer.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(bufs.SingleSampledColorBuffer.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(bufs.SingleSampledColorBuffer.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(bufs.SingleSampledColorBuffer.type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl::BindTexture();

        // setup single-sampled depth buffer (texture, so it can be sampled as part of compositing a UI)
        //
        // https://stackoverflow.com/questions/27535727/opengl-create-a-depth-stencil-texture-for-reading
        gl::BindTexture(bufs.SingleSampledDepthBuffer);
        gl::TexImage2D(
            bufs.SingleSampledDepthBuffer.type,
            0,
            GL_DEPTH24_STENCIL8,
            dimensions.x,
            dimensions.y,
            0,
            GL_DEPTH_STENCIL,
            GL_UNSIGNED_INT_24_8,
            nullptr
        );
        gl::TexParameteri(bufs.SingleSampledDepthBuffer.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(bufs.SingleSampledDepthBuffer.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(bufs.SingleSampledDepthBuffer.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(bufs.SingleSampledDepthBuffer.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(bufs.SingleSampledDepthBuffer.type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl::BindTexture();

        // setup single-sampled framebuffer (color+depth)
        gl::BindFramebuffer(GL_FRAMEBUFFER, bufs.SingleSampledFBO);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufs.SingleSampledColorBuffer, 0);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, bufs.SingleSampledDepthBuffer, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }

    friend class GraphicsBackend;

    RenderTextureDescriptor m_Descriptor;
    DefaultConstructOnCopy<std::optional<RenderTextureOpenGLData>> m_MaybeGPUBuffers;
};

osc::RenderTexture::RenderTexture(RenderTextureDescriptor const& desc) :
    m_Impl{std::make_shared<Impl>(desc)}
{
}

osc::RenderTexture::RenderTexture(RenderTexture const&) = default;
osc::RenderTexture::RenderTexture(RenderTexture&&) noexcept = default;
osc::RenderTexture& osc::RenderTexture::operator=(RenderTexture const&) = default;
osc::RenderTexture& osc::RenderTexture::operator=(RenderTexture&&) noexcept = default;
osc::RenderTexture::~RenderTexture() noexcept = default;

glm::ivec2 osc::RenderTexture::getDimensions() const
{
    return m_Impl->getDimensions();
}

void osc::RenderTexture::setDimensions(glm::ivec2 d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDimensions(std::move(d));
}

osc::RenderTextureFormat osc::RenderTexture::getColorFormat() const
{
    return m_Impl->getColorFormat();
}

void osc::RenderTexture::setColorFormat(RenderTextureFormat format)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setColorFormat(format);
}

int osc::RenderTexture::getAntialiasingLevel() const
{
    return m_Impl->getAntialiasingLevel();
}

void osc::RenderTexture::setAntialiasingLevel(int level)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setAntialiasingLevel(level);
}

osc::DepthStencilFormat osc::RenderTexture::getDepthStencilFormat() const
{
    return m_Impl->getDepthStencilFormat();
}

void osc::RenderTexture::setDepthStencilFormat(DepthStencilFormat format)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDepthStencilFormat(format);
}

void osc::RenderTexture::reformat(RenderTextureDescriptor const& d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->reformat(d);
}

void* osc::RenderTexture::updTextureHandleHACK()
{
    DoCopyOnWrite(m_Impl);
    return m_Impl->updTextureHandleHACK();
}

bool osc::operator==(RenderTexture const& a, RenderTexture const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(RenderTexture const& a, RenderTexture const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(RenderTexture const& a, RenderTexture const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, RenderTexture const&)
{
    return o << "RenderTexture()";
}

void osc::EmplaceOrReformat(std::optional<RenderTexture>& t, RenderTextureDescriptor const& desc)
{
    if (t)
    {
        t->reformat(desc);
    }
    else
    {
        t.emplace(desc);
    }
}


//////////////////////////////////
//
// shader stuff
//
//////////////////////////////////

class osc::Shader::Impl final {
public:
    Impl(CStringView vertexShader, CStringView fragmentShader) :
        m_Program{gl::CreateProgramFrom(gl::CompileFromSource<gl::VertexShader>(vertexShader.c_str()), gl::CompileFromSource<gl::FragmentShader>(fragmentShader.c_str()))}
    {
        parseUniformsAndAttributesFromProgram();
    }

    Impl(CStringView vertexShader, CStringView geometryShader, CStringView fragmentShader) :
        m_Program{gl::CreateProgramFrom(gl::CompileFromSource<gl::VertexShader>(vertexShader.c_str()), gl::CompileFromSource<gl::FragmentShader>(fragmentShader.c_str()), gl::CompileFromSource<gl::GeometryShader>(geometryShader.c_str()))}
    {
        parseUniformsAndAttributesFromProgram();
    }

    std::optional<int> findPropertyIndex(std::string const& propertyName) const
    {
        auto const it = m_Uniforms.find(propertyName);

        if (it != m_Uniforms.end())
        {
            return static_cast<int>(std::distance(m_Uniforms.begin(), it));
        }
        else
        {
            return std::nullopt;
        }
    }

    int getPropertyCount() const
    {
        return static_cast<int>(m_Uniforms.size());
    }

    std::string const& getPropertyName(int i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->first;
    }

    ShaderType getPropertyType(int i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->second.Type;
    }

    // non-PIMPL APIs

    gl::Program& updProgram()
    {
        return m_Program;
    }

    robin_hood::unordered_map<std::string, ShaderElement> const& getUniforms() const
    {
        return m_Uniforms;
    }

    robin_hood::unordered_map<std::string, ShaderElement> const& getAttributes() const
    {
        return m_Attributes;
    }

private:
    void parseUniformsAndAttributesFromProgram()
    {
        constexpr GLsizei maxNameLen = 128;

        GLint numAttrs;
        glGetProgramiv(m_Program.get(), GL_ACTIVE_ATTRIBUTES, &numAttrs);

        GLint numUniforms;
        glGetProgramiv(m_Program.get(), GL_ACTIVE_UNIFORMS, &numUniforms);

        m_Attributes.reserve(numAttrs);
        for (GLint i = 0; i < numAttrs; i++)
        {
            GLint size; // size of the variable
            GLenum type; // type of the variable (float, vec3 or mat4, etc)
            GLchar name[maxNameLen]; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveAttrib(
                m_Program.get() ,
                static_cast<GLuint>(i),
                maxNameLen,
                &length,
                &size,
                &type,
                name
            );

            m_Attributes.try_emplace(
                NormalizeShaderElementName(name),
                static_cast<int>(glGetAttribLocation(m_Program.get(), name)),
                GLShaderTypeToShaderTypeInternal(type),
                static_cast<int>(size)
            );
        }

        m_Uniforms.reserve(numUniforms);
        for (GLint i = 0; i < numUniforms; i++)
        {
            GLint size; // size of the variable
            GLenum type; // type of the variable (float, vec3 or mat4, etc)
            GLchar name[maxNameLen]; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveUniform(
                m_Program.get(),
                static_cast<GLuint>(i),
                maxNameLen,
                &length,
                &size,
                &type,
                name
            );

            m_Uniforms.try_emplace(
                NormalizeShaderElementName(name),
                static_cast<int>(glGetUniformLocation(m_Program.get(), name)),
                GLShaderTypeToShaderTypeInternal(type),
                size
            );
        }

        // cache commonly-used "automatic" shader elements
        //
        // it's a perf optimization: the renderer uses this to skip lookups
        if (ShaderElement const* e = TryGetValue(m_Uniforms, "uModelMat"))
        {
            m_MaybeModelMatUniform = *e;
        }
        if (ShaderElement const* e = TryGetValue(m_Uniforms, "uNormalMat"))
        {
            m_MaybeNormalMatUniform = *e;
        }
        if (ShaderElement const* e = TryGetValue(m_Uniforms, "uViewMat"))
        {
            m_MaybeViewMatUniform = *e;
        }
        if (ShaderElement const* e = TryGetValue(m_Uniforms, "uProjMat"))
        {
            m_MaybeProjMatUniform = *e;
        }
        if (ShaderElement const* e = TryGetValue(m_Uniforms, "uViewProjMat"))
        {
            m_MaybeViewProjMatUniform = *e;
        }
        if (ShaderElement const* e = TryGetValue(m_Attributes, "aModelMat"))
        {
            m_MaybeInstancedModelMatAttr = *e;
        }
        if (ShaderElement const* e = TryGetValue(m_Attributes, "aNormalMat"))
        {
            m_MaybeInstancedNormalMatAttr = *e;
        }
    }

    friend class GraphicsBackend;

    UID m_UID;
    gl::Program m_Program;
    robin_hood::unordered_map<std::string, ShaderElement> m_Uniforms;
    robin_hood::unordered_map<std::string, ShaderElement> m_Attributes;
    std::optional<ShaderElement> m_MaybeModelMatUniform;
    std::optional<ShaderElement> m_MaybeNormalMatUniform;
    std::optional<ShaderElement> m_MaybeViewMatUniform;
    std::optional<ShaderElement> m_MaybeProjMatUniform;
    std::optional<ShaderElement> m_MaybeViewProjMatUniform;
    std::optional<ShaderElement> m_MaybeInstancedModelMatAttr;
    std::optional<ShaderElement> m_MaybeInstancedNormalMatAttr;
};


std::ostream& osc::operator<<(std::ostream& o, ShaderType shaderType)
{
    return o << g_ShaderTypeInternalStrings.at(static_cast<size_t>(shaderType));
}

osc::Shader::Shader(CStringView vertexShader, CStringView fragmentShader) :
    m_Impl{std::make_shared<Impl>(std::move(vertexShader), std::move(fragmentShader))}
{
}

osc::Shader::Shader(CStringView vertexShader, CStringView geometryShader, CStringView fragmentShader) :
    m_Impl{std::make_shared<Impl>(std::move(vertexShader), std::move(geometryShader), std::move(fragmentShader))}
{
}

osc::Shader::Shader(Shader const&) = default;
osc::Shader::Shader(Shader&&) noexcept = default;
osc::Shader& osc::Shader::operator=(Shader const&) = default;
osc::Shader& osc::Shader::operator=(Shader&&) noexcept = default;
osc::Shader::~Shader() noexcept = default;

std::optional<int> osc::Shader::findPropertyIndex(std::string const& propertyName) const
{
    return m_Impl->findPropertyIndex(propertyName);
}

int osc::Shader::getPropertyCount() const
{
    return m_Impl->getPropertyCount();
}

std::string const& osc::Shader::getPropertyName(int propertyIndex) const
{
    return m_Impl->getPropertyName(std::move(propertyIndex));
}

osc::ShaderType osc::Shader::getPropertyType(int propertyIndex) const
{
    return m_Impl->getPropertyType(std::move(propertyIndex));
}

bool osc::operator==(Shader const& a, Shader const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(Shader const& a, Shader const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(Shader const& a, Shader const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, Shader const& shader)
{
    o << "Shader(\n";
    {
        o << "    uniforms = [";

        char const* delim = "\n        ";
        for (auto const& [name, data] : shader.m_Impl->getUniforms())
        {
            o << delim;
            PrintShaderElement(o, name, data);
        }

        o << "\n    ],\n";
    }

    {
        o << "    attributes = [";

        char const* delim = "\n        ";
        for (auto const& [name, data] : shader.m_Impl->getAttributes())
        {
            o << delim;
            PrintShaderElement(o, name, data);
        }

        o << "\n    ]\n";
    }

    o << ')';

    return o;
}


//////////////////////////////////
//
// material stuff
//
//////////////////////////////////

class osc::Material::Impl final {
public:
    Impl(Shader shader) : m_Shader{std::move(shader)}
    {
    }

    Shader const& getShader() const
    {
        return m_Shader;
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(std::move(propertyName));
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<nonstd::span<float const>> getFloatArray(std::string_view propertyName) const
    {
        return getValue<std::vector<float>>(std::move(propertyName));
    }

    void setFloatArray(std::string_view propertyName, nonstd::span<float const> v)
    {
        setValue(std::move(propertyName), std::vector<float>(v.begin(), v.end()));
    }

    std::optional<glm::vec2> getVec2(std::string_view propertyName) const
    {
        return getValue<glm::vec2>(std::move(propertyName));
    }

    void setVec2(std::string_view propertyName, glm::vec2 value)
    {
        setValue(std::move(propertyName), std::move(value));
    }

    std::optional<glm::vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<glm::vec3>(std::move(propertyName));
    }

    void setVec3(std::string_view propertyName, glm::vec3 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<nonstd::span<glm::vec3 const>> getVec3Array(std::string_view propertyName) const
    {
        return getValue<std::vector<glm::vec3>>(std::move(propertyName));
    }

    void setVec3Array(std::string_view propertyName, nonstd::span<glm::vec3 const> value)
    {
        setValue(std::move(propertyName), std::vector<glm::vec3>(value.begin(), value.end()));
    }

    std::optional<glm::vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<glm::vec4>(std::move(propertyName));
    }

    void setVec4(std::string_view propertyName, glm::vec4 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<glm::mat3>(std::move(propertyName));
    }

    void setMat3(std::string_view propertyName, glm::mat3 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<glm::mat4>(std::move(propertyName));
    }

    void setMat4(std::string_view propertyName, glm::mat4 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<int> getInt(std::string_view propertyName) const
    {
        return getValue<int>(std::move(propertyName));
    }

    void setInt(std::string_view propertyName, int value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<bool> getBool(std::string_view propertyName) const
    {
        return getValue<bool>(std::move(propertyName));
    }

    void setBool(std::string_view propertyName, bool value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<Texture2D> getTexture(std::string_view propertyName) const
    {
        return getValue<Texture2D>(std::move(propertyName));
    }

    void setTexture(std::string_view propertyName, Texture2D t)
    {
        setValue(std::move(propertyName), std::move(t));
    }

    void clearTexture(std::string_view propertyName)
    {
        m_Values.erase(std::string{std::move(propertyName)});
    }

    std::optional<RenderTexture> getRenderTexture(std::string_view propertyName) const
    {
        return getValue<RenderTexture>(std::move(propertyName));
    }

    void setRenderTexture(std::string_view propertyName, RenderTexture t)
    {
        setValue(std::move(propertyName), std::move(t));
    }

    void clearRenderTexture(std::string_view propertyName)
    {
        m_Values.erase(std::string{std::move(propertyName)});
    }

    bool getTransparent() const
    {
        return m_IsTransparent;
    }

    void setTransparent(bool v)
    {
        m_IsTransparent = std::move(v);
    }

    bool getDepthTested() const
    {
        return m_IsDepthTested;
    }

    void setDepthTested(bool v)
    {
        m_IsDepthTested = std::move(v);
    }

    bool getWireframeMode() const
    {
        return m_IsWireframeMode;
    }

    void setWireframeMode(bool v)
    {
        m_IsWireframeMode = std::move(v);
    }

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        auto const it = m_Values.find(std::string{std::move(propertyName)});

        if (it == m_Values.end())
        {
            return std::nullopt;
        }

        if (!std::holds_alternative<T>(it->second))
        {
            return std::nullopt;
        }

        return std::get<T>(it->second);
    }

    template<typename T>
    void setValue(std::string_view propertyName, T&& v)
    {
        m_Values.insert_or_assign(std::string{propertyName}, std::forward<T&&>(v));
    }

    friend class GraphicsBackend;

    Shader m_Shader;
    robin_hood::unordered_map<std::string, MaterialValue> m_Values;
    bool m_IsTransparent = false;
    bool m_IsDepthTested = true;
    bool m_IsWireframeMode = false;
};

osc::Material::Material(Shader shader) :
    m_Impl{std::make_shared<Impl>(std::move(shader))}
{
}

osc::Material::Material(Material const&) = default;
osc::Material::Material(Material&&) noexcept = default;
osc::Material& osc::Material::operator=(Material const&) = default;
osc::Material& osc::Material::operator=(Material&&) noexcept = default;
osc::Material::~Material() noexcept = default;

osc::Shader const& osc::Material::getShader() const
{
    return m_Impl->getShader();
}

std::optional<float> osc::Material::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

void osc::Material::setFloat(std::string_view propertyName, float value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(value));
}

std::optional<nonstd::span<float const>> osc::Material::getFloatArray(std::string_view propertyName) const
{
    return m_Impl->getFloatArray(std::move(propertyName));
}

void osc::Material::setFloatArray(std::string_view propertyName, nonstd::span<float const> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloatArray(std::move(propertyName), std::move(vs));
}

std::optional<glm::vec2> osc::Material::getVec2(std::string_view propertyName) const
{
    return m_Impl->getVec2(std::move(propertyName));
}

void osc::Material::setVec2(std::string_view propertyName, glm::vec2 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec2(std::move(propertyName), std::move(value));
}

std::optional<nonstd::span<glm::vec3 const>> osc::Material::getVec3Array(std::string_view propertyName) const
{
    return m_Impl->getVec3Array(std::move(propertyName));
}

void osc::Material::setVec3Array(std::string_view propertyName, nonstd::span<glm::vec3 const> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec3Array(std::move(propertyName), std::move(vs));
}

std::optional<glm::vec3> osc::Material::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(std::move(propertyName));
}

void osc::Material::setVec3(std::string_view propertyName, glm::vec3 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec3(std::move(propertyName), std::move(value));
}

std::optional<glm::vec4> osc::Material::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(std::move(propertyName));
}

void osc::Material::setVec4(std::string_view propertyName, glm::vec4 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec4(std::move(propertyName), std::move(value));
}

std::optional<glm::mat3> osc::Material::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(std::move(propertyName));
}

void osc::Material::setMat3(std::string_view propertyName, glm::mat3 const& mat)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat3(std::move(propertyName), mat);
}

std::optional<glm::mat4> osc::Material::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(std::move(propertyName));
}

void osc::Material::setMat4(std::string_view propertyName, glm::mat4 const& mat)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat4(std::move(propertyName), mat);
}

std::optional<int> osc::Material::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

void osc::Material::setInt(std::string_view propertyName, int value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(value));
}

std::optional<bool> osc::Material::getBool(std::string_view propertyName) const
{
    return m_Impl->getBool(std::move(propertyName));
}

void osc::Material::setBool(std::string_view propertyName, bool value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBool(std::move(propertyName), std::move(value));
}

std::optional<osc::Texture2D> osc::Material::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

void osc::Material::setTexture(std::string_view propertyName, Texture2D t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

void osc::Material::clearTexture(std::string_view propertyName)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clearTexture(std::move(propertyName));
}

std::optional<osc::RenderTexture> osc::Material::getRenderTexture(std::string_view propertyName) const
{
    return m_Impl->getRenderTexture(std::move(propertyName));
}

void osc::Material::setRenderTexture(std::string_view propertyName, RenderTexture t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setRenderTexture(std::move(propertyName), std::move(t));
}

void osc::Material::clearRenderTexture(std::string_view propertyName)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clearRenderTexture(std::move(propertyName));
}

bool osc::Material::getTransparent() const
{
    return m_Impl->getTransparent();
}

void osc::Material::setTransparent(bool v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTransparent(std::move(v));
}

bool osc::Material::getDepthTested() const
{
    return m_Impl->getDepthTested();
}

void osc::Material::setDepthTested(bool v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDepthTested(std::move(v));
}

bool osc::Material::getWireframeMode() const
{
    return m_Impl->getWireframeMode();
}

void osc::Material::setWireframeMode(bool v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWireframeMode(std::move(v));
}

bool osc::operator==(Material const& a, Material const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(Material const& a, Material const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(Material const& a, Material const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, Material const&)
{
    return o << "Material()";
}


//////////////////////////////////
//
// material property block stuff
//
//////////////////////////////////

class osc::MaterialPropertyBlock::Impl final {
public:
    void clear()
    {
        m_Values.clear();
    }

    bool isEmpty() const
    {
        return m_Values.empty();
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(std::move(propertyName));
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<glm::vec3>(std::move(propertyName));
    }

    void setVec3(std::string_view propertyName, glm::vec3 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<glm::vec4>(std::move(propertyName));
    }

    void setVec4(std::string_view propertyName, glm::vec4 value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<glm::mat3>(std::move(propertyName));
    }

    void setMat3(std::string_view propertyName, glm::mat3 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<glm::mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<glm::mat4>(std::move(propertyName));
    }

    void setMat4(std::string_view propertyName, glm::mat4 const& value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<int> getInt(std::string_view propertyName) const
    {
        return getValue<int>(std::move(propertyName));
    }

    void setInt(std::string_view propertyName, int value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<bool> getBool(std::string_view propertyName) const
    {
        return getValue<bool>(std::move(propertyName));
    }

    void setBool(std::string_view propertyName, bool value)
    {
        setValue(std::move(propertyName), value);
    }

    std::optional<Texture2D> getTexture(std::string_view propertyName) const
    {
        return getValue<Texture2D>(std::move(propertyName));
    }

    void setTexture(std::string_view propertyName, Texture2D t)
    {
        setValue(std::move(propertyName), std::move(t));
    }

    bool operator==(Impl const& other) const
    {
        return m_Values == other.m_Values;
    }

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        auto const it = m_Values.find(std::string{std::move(propertyName)});

        if (it == m_Values.end())
        {
            return std::nullopt;
        }

        if (!std::holds_alternative<T>(it->second))
        {
            return std::nullopt;
        }

        return std::get<T>(it->second);
    }

    template<typename T>
    void setValue(std::string_view propertyName, T&& v)
    {
        m_Values.insert_or_assign(std::string{propertyName}, std::forward<T&&>(v));
    }

    friend class GraphicsBackend;

    robin_hood::unordered_map<std::string, MaterialValue> m_Values;
};

osc::MaterialPropertyBlock::MaterialPropertyBlock() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock const&) = default;
osc::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept = default;
osc::MaterialPropertyBlock& osc::MaterialPropertyBlock::operator=(MaterialPropertyBlock const&) = default;
osc::MaterialPropertyBlock& osc::MaterialPropertyBlock::operator=(MaterialPropertyBlock&&) noexcept = default;
osc::MaterialPropertyBlock::~MaterialPropertyBlock() noexcept = default;

void osc::MaterialPropertyBlock::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::MaterialPropertyBlock::isEmpty() const
{
    return m_Impl->isEmpty();
}

std::optional<float> osc::MaterialPropertyBlock::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setFloat(std::string_view propertyName, float value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(value));
}

std::optional<glm::vec3> osc::MaterialPropertyBlock::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setVec3(std::string_view propertyName, glm::vec3 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec3(std::move(propertyName), std::move(value));
}

std::optional<glm::vec4> osc::MaterialPropertyBlock::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setVec4(std::string_view propertyName, glm::vec4 value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVec4(std::move(propertyName), std::move(value));
}

std::optional<glm::mat3> osc::MaterialPropertyBlock::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setMat3(std::string_view propertyName, glm::mat3 const& value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat3(std::move(propertyName), value);
}

std::optional<glm::mat4> osc::MaterialPropertyBlock::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setMat4(std::string_view propertyName, glm::mat4 const& value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMat4(std::move(propertyName), value);
}

std::optional<int> osc::MaterialPropertyBlock::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setInt(std::string_view propertyName, int value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(value));
}

std::optional<bool> osc::MaterialPropertyBlock::getBool(std::string_view propertyName) const
{
    return m_Impl->getBool(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setBool(std::string_view propertyName, bool value)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBool(std::move(propertyName), std::move(value));
}

std::optional<osc::Texture2D> osc::MaterialPropertyBlock::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

void osc::MaterialPropertyBlock::setTexture(std::string_view propertyName, Texture2D t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

bool osc::operator==(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl == b.m_Impl || *a.m_Impl == *b.m_Impl;
}

bool osc::operator!=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, MaterialPropertyBlock const&)
{
    return o << "MaterialPropertyBlock()";
}


//////////////////////////////////
//
// mesh stuff
//
//////////////////////////////////

namespace
{
    static constexpr auto g_MeshTopographyStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::MeshTopography::TOTAL)>
    (
        "Triangles",
        "Lines"
    );

    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
    static_assert(alignof(PackedIndex) == alignof(uint32_t));

    GLenum ToOpenGLTopography(osc::MeshTopography t)
    {
        switch (t)
        {
        case osc::MeshTopography::Triangles:
            return GL_TRIANGLES;
        case osc::MeshTopography::Lines:
            return GL_LINES;
        default:
            return GL_TRIANGLES;
        }
    }
}

class osc::Mesh::Impl final {
public:

    MeshTopography getTopography() const
    {
        return m_Topography;
    }

    void setTopography(MeshTopography t)
    {
        m_Topography = std::move(t);
        m_Version->reset();
    }

    nonstd::span<glm::vec3 const> getVerts() const
    {
        return m_Vertices;
    }

    void setVerts(nonstd::span<glm::vec3 const> verts)
    {
        m_Vertices.assign(verts.begin(), verts.end());

        recalculateBounds();
        m_Version->reset();
    }

    nonstd::span<glm::vec3 const> getNormals() const
    {
        return m_Normals;
    }

    void setNormals(nonstd::span<glm::vec3 const> normals)
    {
        m_Normals.assign(normals.begin(), normals.end());

        m_Version->reset();
    }

    nonstd::span<glm::vec2 const> getTexCoords() const
    {
        return m_TexCoords;
    }

    void setTexCoords(nonstd::span<glm::vec2 const> coords)
    {
        m_TexCoords.assign(coords.begin(), coords.end());

        m_Version->reset();
    }

    nonstd::span<Rgba32 const> getColors()
    {
        return m_Colors;
    }

    void setColors(nonstd::span<Rgba32 const> colors)
    {
        m_Colors.assign(colors.begin(), colors.end());

        m_Version.reset();
    }

    MeshIndicesView getIndices() const
    {
        if (m_NumIndices <= 0)
        {
            return {};
        }
        else if (m_IndicesAre32Bit)
        {
            return {&m_IndicesData.front().u32, m_NumIndices};
        }
        else
        {
            return {&m_IndicesData.front().u16.a, m_NumIndices};
        }
    }

    void setIndices(MeshIndicesView indices)
    {
        indices.isU16() ? setIndices(indices.toU16Span()) : setIndices(indices.toU32Span());
    }

    void setIndices(nonstd::span<uint16_t const> indices)
    {
        m_IndicesAre32Bit = false;
        m_NumIndices = indices.size();
        m_IndicesData.resize((indices.size()+1)/2);
        std::copy(indices.begin(), indices.end(), &m_IndicesData.front().u16.a);

        recalculateBounds();
        m_Version->reset();
    }

    void setIndices(nonstd::span<std::uint32_t const> vs)
    {
        auto const isGreaterThanU16Max = [](uint32_t v)
        {
            return v > std::numeric_limits<uint16_t>::max();
        };

        if (std::any_of(vs.begin(), vs.end(), isGreaterThanU16Max))
        {
            m_IndicesAre32Bit = true;
            m_NumIndices = vs.size();
            m_IndicesData.resize(vs.size());
            std::copy(vs.begin(), vs.end(), &m_IndicesData.front().u32);
        }
        else
        {
            m_IndicesAre32Bit = false;
            m_NumIndices = vs.size();
            m_IndicesData.resize((vs.size()+1)/2);
            std::copy(vs.begin(), vs.end(), &m_IndicesData.front().u16.a);
        }

        recalculateBounds();
        m_Version->reset();
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    glm::vec3 getMidpoint() const
    {
        return m_Midpoint;
    }

    BVH const& getBVH() const
    {
        return m_TriangleBVH;
    }

    void clear()
    {
        m_Version->reset();
        m_Topography = MeshTopography::Triangles;
        m_Vertices.clear();
        m_Normals.clear();
        m_TexCoords.clear();
        m_Colors.clear();
        m_IndicesAre32Bit = false;
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = {};
        m_Midpoint = {};
    }

    // non-PIMPL methods

    MeshOpenGLData const& getOpenGLData()
    {
        if (!*m_MaybeGPUBuffers || (*m_MaybeGPUBuffers)->DataVersion != *m_Version)
        {
            uploadToGPU();
        }
        return **m_MaybeGPUBuffers;
    }

    void draw()
    {
        gl::DrawElements(
            ToOpenGLTopography(m_Topography),
            static_cast<GLsizei>(m_NumIndices),
            m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            nullptr
        );
    }

    void drawInstanced(size_t n)
    {
        glDrawElementsInstanced(
            ToOpenGLTopography(m_Topography),
            static_cast<GLsizei>(m_NumIndices),
            m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            nullptr,
            static_cast<GLsizei>(n)
        );
    }

private:

    void recalculateBounds()
    {
        if (m_NumIndices == 0)
        {
            m_AABB = {};
        }
        else if (m_IndicesAre32Bit)
        {
            nonstd::span<uint32_t const> const indices(&m_IndicesData.front().u32, m_NumIndices);

            // bounds-check the indices, to ensure nothing bizzare happens at runtime (#460)
            OSC_ASSERT(std::all_of(indices.begin(), indices.end(), [nVerts = m_Vertices.size()](uint32_t i) { return i < nVerts; }));

            m_AABB = AABBFromIndexedVerts(m_Vertices, indices);
            if (m_Topography == MeshTopography::Triangles)
            {
                BVH_BuildFromIndexedTriangles(m_TriangleBVH, m_Vertices, indices);
            }
            else
            {
                m_TriangleBVH.clear();
            }
        }
        else
        {
            nonstd::span<uint16_t const> const indices(&m_IndicesData.front().u16.a, m_NumIndices);

            // bounds-check the indices, to ensure nothing bizzare happens at runtime (#460)
            OSC_ASSERT(std::all_of(indices.begin(), indices.end(), [nVerts = m_Vertices.size()](uint16_t i) { return i < nVerts; }));

            m_AABB = AABBFromIndexedVerts(m_Vertices, indices);
            if (m_Topography == MeshTopography::Triangles)
            {
                BVH_BuildFromIndexedTriangles(m_TriangleBVH, m_Vertices, indices);
            }
            else
            {
                m_TriangleBVH.clear();
            }
        }
        m_Midpoint = Midpoint(m_AABB);
    }

    void uploadToGPU()
    {
        bool const hasNormals = !m_Normals.empty();
        bool const hasTexCoords = !m_TexCoords.empty();
        bool const hasColors = !m_Colors.empty();

        // compute the byte stride between each entry in the VBO
        GLsizei byteStride = sizeof(decltype(m_Vertices)::value_type);
        if (hasNormals)
        {
            OSC_ASSERT_ALWAYS(m_Normals.size() == m_Vertices.size() && "number of normals != number of verts");
            byteStride += sizeof(decltype(m_Normals)::value_type);
        }
        if (hasTexCoords)
        {
            OSC_ASSERT_ALWAYS(m_TexCoords.size() == m_Vertices.size() && "number of uvs != number of verts");
            byteStride += sizeof(decltype(m_TexCoords)::value_type);
        }
        if (hasColors)
        {
            OSC_ASSERT_ALWAYS(m_Colors.size() == m_Vertices.size() && "number of colors != number of verts");
            byteStride += sizeof(decltype(m_Colors)::value_type);
        }

        // pack VBO data into CPU-side buffer
        std::vector<std::byte> data;
        data.reserve(byteStride * m_Vertices.size());
        for (size_t i = 0; i < m_Vertices.size(); ++i)
        {
            PushAsBytes(m_Vertices.at(i), data);
            if (hasNormals)
            {
                PushAsBytes(m_Normals.at(i), data);
            }
            if (hasTexCoords)
            {
                PushAsBytes(m_TexCoords.at(i), data);
            }
            if (hasColors)
            {
                PushAsBytes(m_Colors.at(i), data);
            }
        }
        OSC_ASSERT_ALWAYS(data.size() == byteStride*m_Vertices.size() && "error uploading data to the GPU");

        // create the GPU-side buffers
        if (!(*m_MaybeGPUBuffers))
        {
            *m_MaybeGPUBuffers = MeshOpenGLData{};
        }
        MeshOpenGLData& buffers = **m_MaybeGPUBuffers;

        // upload VBO data into GPU-side buffer
        gl::BindBuffer(GL_ARRAY_BUFFER, buffers.ArrayBuffer);
        gl::BufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

        // upload indices into EBO
        {
            // before uploading, bounds-check the indices, to ensure nothing bizzare
            // happens at runtime (e.g. GPU driver behaving bizzarely, #460)
            if (m_NumIndices > 0)
            {
                if (m_IndicesAre32Bit)
                {
                    nonstd::span<uint32_t const> const indices(&m_IndicesData.front().u32, m_NumIndices);
                    OSC_ASSERT_ALWAYS(std::all_of(indices.begin(), indices.end(), [nVerts = m_Vertices.size()](uint32_t i) { return i < nVerts; }));
                }
                else
                {
                    nonstd::span<uint16_t const> const indices(&m_IndicesData.front().u16.a, m_NumIndices);
                    OSC_ASSERT_ALWAYS(std::all_of(indices.begin(), indices.end(), [nVerts = m_Vertices.size()](uint16_t i) { return i < nVerts; }));
                }
            }
        }

        size_t const eboNumBytes = m_NumIndices * (m_IndicesAre32Bit ? sizeof(uint32_t) : sizeof(uint16_t));
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.IndicesBuffer);
        gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, eboNumBytes, m_IndicesData.data(), GL_STATIC_DRAW);

        // then define the buffer's GPU-side format (for later shader binding)
        buffers.BufferDescription.clear();
        size_t offset = 0;
        {
            // vertex description
            auto& desc = buffers.BufferDescription.emplace_back();
            desc.AttributeIndex = SHADER_LOC_VERTEX_POSITION;
            desc.NumComponents = 3;
            desc.ComponentType = GL_FLOAT;
            desc.IsNormalized = GL_FALSE;
            desc.Stride = byteStride;
            desc.Offset = offset;
            offset += 3 * sizeof(float);
        }

        if (hasNormals)
        {
            // add normals description
            auto& desc = buffers.BufferDescription.emplace_back();
            desc.AttributeIndex = SHADER_LOC_VERTEX_NORMAL;
            desc.NumComponents = 3;
            desc.ComponentType = GL_FLOAT;
            desc.IsNormalized = GL_FALSE;
            desc.Stride = byteStride;
            desc.Offset = offset;
            offset += 3 * sizeof(float);
        }

        if (hasTexCoords)
        {
            // add texcoords description
            auto& desc = buffers.BufferDescription.emplace_back();
            desc.AttributeIndex = SHADER_LOC_VERTEX_TEXCOORD01;
            desc.NumComponents = 2;
            desc.ComponentType = GL_FLOAT;
            desc.IsNormalized = GL_FALSE;
            desc.Stride = byteStride;
            desc.Offset = offset;
            offset += 2 * sizeof(float);
        }

        if (hasColors)
        {
            // add colors description
            auto& desc = buffers.BufferDescription.emplace_back();
            desc.AttributeIndex = SHADER_LOC_COLOR;
            desc.NumComponents = 4;
            desc.ComponentType = GL_UNSIGNED_BYTE;
            desc.IsNormalized = GL_TRUE;
            desc.Stride = byteStride;
            desc.Offset = offset;
            offset += 4 * sizeof(uint8_t);
        }

        buffers.DataVersion = *m_Version;
    }

    DefaultConstructOnCopy<UID> m_UID;
    DefaultConstructOnCopy<UID> m_Version;
    MeshTopography m_Topography = MeshTopography::Triangles;
    std::vector<glm::vec3> m_Vertices;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    std::vector<Rgba32> m_Colors;

    bool m_IndicesAre32Bit = false;
    size_t m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;

    AABB m_AABB = {};
    glm::vec3 m_Midpoint = {};
    BVH m_TriangleBVH;

    DefaultConstructOnCopy<std::optional<MeshOpenGLData>> m_MaybeGPUBuffers;
};

std::ostream& osc::operator<<(std::ostream& o, MeshTopography mt)
{
    return o << g_MeshTopographyStrings.at(static_cast<size_t>(mt));
}

osc::Mesh::Mesh() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::Mesh::Mesh(Mesh const&) = default;
osc::Mesh::Mesh(Mesh&&) noexcept = default;
osc::Mesh& osc::Mesh::operator=(Mesh const&) = default;
osc::Mesh& osc::Mesh::operator=(Mesh&&) noexcept = default;
osc::Mesh::~Mesh() noexcept = default;

osc::MeshTopography osc::Mesh::getTopography() const
{
    return m_Impl->getTopography();
}

void osc::Mesh::setTopography(MeshTopography topography)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTopography(std::move(topography));
}

nonstd::span<glm::vec3 const> osc::Mesh::getVerts() const
{
    return m_Impl->getVerts();
}

void osc::Mesh::setVerts(nonstd::span<glm::vec3 const> verts)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVerts(std::move(verts));
}

nonstd::span<glm::vec3 const> osc::Mesh::getNormals() const
{
    return m_Impl->getNormals();
}

void osc::Mesh::setNormals(nonstd::span<glm::vec3 const> verts)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNormals(std::move(verts));
}

nonstd::span<glm::vec2 const> osc::Mesh::getTexCoords() const
{
    return m_Impl->getTexCoords();
}

void osc::Mesh::setTexCoords(nonstd::span<glm::vec2 const> coords)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexCoords(coords);
}

nonstd::span<osc::Rgba32 const> osc::Mesh::getColors()
{
    return m_Impl->getColors();
}

void osc::Mesh::setColors(nonstd::span<osc::Rgba32 const> colors)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setColors(colors);
}

osc::MeshIndicesView osc::Mesh::getIndices() const
{
    return m_Impl->getIndices();
}

void osc::Mesh::setIndices(MeshIndicesView indices)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(indices));
}

void osc::Mesh::setIndices(nonstd::span<uint16_t const> indices)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(indices));
}

void osc::Mesh::setIndices(nonstd::span<uint32_t const> indices)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(indices));
}

osc::AABB const& osc::Mesh::getBounds() const
{
    return m_Impl->getBounds();
}

glm::vec3 osc::Mesh::getMidpoint() const
{
    return m_Impl->getMidpoint();
}

osc::BVH const& osc::Mesh::getBVH() const
{
    return m_Impl->getBVH();
}

void osc::Mesh::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::operator==(Mesh const& a, Mesh const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(Mesh const& a, Mesh const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, Mesh const&)
{
    return o << "Mesh()";
}


//////////////////////////////////
//
// camera stuff
//
//////////////////////////////////

namespace
{
    // LUT for human-readable form of the above
    static constexpr auto const g_CameraProjectionStrings = osc::MakeArray<osc::CStringView, static_cast<size_t>(osc::CameraProjection::TOTAL)>
    (
        "Perspective",
        "Orthographic"
    );
}

class osc::Camera::Impl final {
public:
    Impl() = default;

    explicit Impl(RenderTexture t) : m_MaybeTexture{std::move(t)}
    {
    }

    glm::vec4 getBackgroundColor() const
    {
        return m_BackgroundColor;
    }

    void setBackgroundColor(glm::vec4 const& color)
    {
        m_BackgroundColor = color;
    }

    CameraProjection getCameraProjection() const
    {
        return m_CameraProjection;
    }

    void setCameraProjection(CameraProjection projection)
    {
        m_CameraProjection = std::move(projection);
    }

    float getOrthographicSize() const
    {
        return m_OrthographicSize;
    }

    void setOrthographicSize(float size)
    {
        m_OrthographicSize = std::move(size);
    }

    float getCameraFOV() const
    {
        return m_PerspectiveFov;
    }

    void setCameraFOV(float size)
    {
        m_PerspectiveFov = std::move(size);
    }

    float getNearClippingPlane() const
    {
        return m_NearClippingPlane;
    }

    void setNearClippingPlane(float distance)
    {
        m_NearClippingPlane = std::move(distance);
    }

    float getFarClippingPlane() const
    {
        return m_FarClippingPlane;
    }

    void setFarClippingPlane(float distance)
    {
        m_FarClippingPlane = std::move(distance);
    }

    CameraClearFlags getClearFlags() const
    {
        return m_ClearFlags;
    }

    void setClearFlags(CameraClearFlags flags)
    {
        m_ClearFlags = std::move(flags);
    }

    std::optional<RenderTexture> getTexture() const
    {
        return m_MaybeTexture;
    }

    void setTexture(RenderTexture&& t)
    {
        m_MaybeTexture.emplace(std::move(t));
    }

    void setTexture(RenderTextureDescriptor t)
    {
        if (m_MaybeTexture)
        {
            m_MaybeTexture->reformat(std::move(t));
        }
        else
        {
            m_MaybeTexture.emplace(std::move(t));
        }
    }

    void setTexture()
    {
        m_MaybeTexture = std::nullopt;
    }

    void swapTexture(std::optional<RenderTexture>& other)
    {
        std::swap(m_MaybeTexture, other);
    }

    Rect getPixelRect() const
    {
        if (m_MaybeScreenPixelRect)
        {
            return *m_MaybeScreenPixelRect;
        }
        else if (m_MaybeTexture)
        {
            return Rect{{}, glm::vec2{m_MaybeTexture->getDimensions()}};
        }
        else
        {
            return Rect{{}, App::get().dims()};
        }
    }

    void setPixelRect(Rect const& rect)
    {
        m_MaybeScreenPixelRect = rect;
    }

    void setPixelRect()
    {
        m_MaybeScreenPixelRect.reset();
    }

    float getAspectRatio() const
    {
        return AspectRatio(getiDims());
    }

    std::optional<Rect> getScissorRect() const
    {
        return m_MaybeScissorRect;
    }

    void setScissorRect(Rect const& rect)
    {
        m_MaybeScissorRect = rect;
    }

    void setScissorRect()
    {
        m_MaybeScissorRect = std::nullopt;
    }

    glm::vec3 getPosition() const
    {
        return m_Position;
    }

    void setPosition(glm::vec3 const& position)
    {
        m_Position = position;
    }

    glm::quat getRotation() const
    {
        return m_Rotation;
    }

    void setRotation(glm::quat const& rotation)
    {
        m_Rotation = rotation;
    }

    glm::vec3 getDirection() const
    {
        return m_Rotation * glm::vec3{0.0f, 0.0f, -1.0f};
    }

    void setDirection(glm::vec3 const& d)
    {
        m_Rotation = glm::rotation(glm::vec3{0.0f, 0.0f, -1.0f}, d);
    }

    glm::vec3 getUpwardsDirection() const
    {
        return m_Rotation * glm::vec3{0.0f, 1.0f, 0.0f};
    }

    glm::mat4 getViewMatrix() const
    {
        if (m_MaybeViewMatrixOverride)
        {
            return *m_MaybeViewMatrixOverride;
        }
        else
        {
            return glm::lookAt(m_Position, m_Position + getDirection(), getUpwardsDirection());
        }
    }

    void setViewMatrix(glm::mat4 const& m)
    {
        m_MaybeViewMatrixOverride = m;
    }

    void resetViewMatrix()
    {
        m_MaybeViewMatrixOverride.reset();
    }

    glm::mat4 getProjectionMatrix() const
    {
        if (m_MaybeProjectionMatrixOverride)
        {
            return *m_MaybeProjectionMatrixOverride;
        }
        else if (m_CameraProjection == CameraProjection::Perspective)
        {
            return glm::perspective(m_PerspectiveFov, getAspectRatio(), m_NearClippingPlane, m_FarClippingPlane);
        }
        else
        {
            float const height = m_OrthographicSize;
            float const width = height * getAspectRatio();

            float const right = 0.5f * width;
            float const left = -right;
            float const top = 0.5f * height;
            float const bottom = -top;

            return glm::ortho(left, right, bottom, top, m_NearClippingPlane, m_FarClippingPlane);
        }
    }

    void setProjectionMatrix(glm::mat4 const& m)
    {
        m_MaybeProjectionMatrixOverride = m;
    }

    void resetProjectionMatrix()
    {
        m_MaybeProjectionMatrixOverride.reset();
    }

    glm::mat4 getViewProjectionMatrix() const
    {
        return getProjectionMatrix() * getViewMatrix();
    }

    glm::mat4 getInverseViewProjectionMatrix() const
    {
        return glm::inverse(getViewProjectionMatrix());
    }

    void render()
    {
        GraphicsBackend::RenderScene(*this);
    }

private:
    glm::ivec2 getiDims() const
    {
        if (m_MaybeTexture)
        {
            return m_MaybeTexture->getDimensions();
        }
        else if (m_MaybeScreenPixelRect)
        {
            return glm::ivec2(Dimensions(*m_MaybeScreenPixelRect));
        }
        else
        {
            return App::get().idims();
        }
    }

    glm::vec2 viewportDimensions() const
    {
        if (m_MaybeTexture)
        {
            return {m_MaybeTexture->getDimensions()};
        }
        else
        {
            return App::get().dims();
        }
    }

    friend class GraphicsBackend;

    std::optional<RenderTexture> m_MaybeTexture = std::nullopt;
    glm::vec4 m_BackgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
    CameraProjection m_CameraProjection = CameraProjection::Perspective;
    float m_OrthographicSize = 2.0f;
    float m_PerspectiveFov = fpi2;
    float m_NearClippingPlane = 1.0f;
    float m_FarClippingPlane = -1.0f;
    CameraClearFlags m_ClearFlags = CameraClearFlags::Default;
    std::optional<Rect> m_MaybeScreenPixelRect = std::nullopt;
    std::optional<Rect> m_MaybeScissorRect = std::nullopt;
    glm::vec3 m_Position = {};
    glm::quat m_Rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    std::optional<glm::mat4> m_MaybeViewMatrixOverride;
    std::optional<glm::mat4> m_MaybeProjectionMatrixOverride;
    std::vector<RenderObject> m_RenderQueue;
};

std::ostream& osc::operator<<(std::ostream& o, CameraProjection cp)
{
    return o << g_CameraProjectionStrings.at(static_cast<size_t>(cp));
}

osc::Camera::Camera() :
    m_Impl{new Impl{}}
{
}

osc::Camera::Camera(RenderTexture t) :
    m_Impl{new Impl{std::move(t)}}
{
}

osc::Camera::Camera(Camera const&) = default;
osc::Camera::Camera(Camera&&) noexcept = default;
osc::Camera& osc::Camera::operator=(Camera const&) = default;
osc::Camera& osc::Camera::operator=(Camera&&) noexcept = default;
osc::Camera::~Camera() noexcept = default;

glm::vec4 osc::Camera::getBackgroundColor() const
{
    return m_Impl->getBackgroundColor();
}

void osc::Camera::setBackgroundColor(glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBackgroundColor(v);
}

osc::CameraProjection osc::Camera::getCameraProjection() const
{
    return m_Impl->getCameraProjection();
}

void osc::Camera::setCameraProjection(CameraProjection projection)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraProjection(std::move(projection));
}

float osc::Camera::getOrthographicSize() const
{
    return m_Impl->getOrthographicSize();
}

void osc::Camera::setOrthographicSize(float sz)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setOrthographicSize(std::move(sz));
}

float osc::Camera::getCameraFOV() const
{
    return m_Impl->getCameraFOV();
}

void osc::Camera::setCameraFOV(float fov)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraFOV(std::move(fov));
}

float osc::Camera::getNearClippingPlane() const
{
    return m_Impl->getNearClippingPlane();
}

void osc::Camera::setNearClippingPlane(float d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNearClippingPlane(std::move(d));
}

float osc::Camera::getFarClippingPlane() const
{
    return m_Impl->getFarClippingPlane();
}

void osc::Camera::setFarClippingPlane(float d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFarClippingPlane(std::move(d));
}

osc::CameraClearFlags osc::Camera::getClearFlags() const
{
    return m_Impl->getClearFlags();
}

void osc::Camera::setClearFlags(CameraClearFlags flags)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setClearFlags(std::move(flags));
}

std::optional<osc::RenderTexture> osc::Camera::getTexture() const
{
    return m_Impl->getTexture();
}

void osc::Camera::setTexture(RenderTexture&& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(t));
}

void osc::Camera::setTexture(RenderTextureDescriptor desc)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(desc));
}

void osc::Camera::setTexture()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture();
}

void osc::Camera::swapTexture(std::optional<RenderTexture>& other)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->swapTexture(other);
}

osc::Rect osc::Camera::getPixelRect() const
{
    return m_Impl->getPixelRect();
}

void osc::Camera::setPixelRect(Rect const& rect)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPixelRect(rect);
}

void osc::Camera::setPixelRect()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPixelRect();
}

float osc::Camera::getAspectRatio() const
{
    return m_Impl->getAspectRatio();
}

std::optional<osc::Rect> osc::Camera::getScissorRect() const
{
    return m_Impl->getScissorRect();
}

void osc::Camera::setScissorRect(Rect const& rect)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect(rect);
}

void osc::Camera::setScissorRect()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect();
}

glm::vec3 osc::Camera::getPosition() const
{
    return m_Impl->getPosition();
}

void osc::Camera::setPosition(glm::vec3 const& p)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPosition(p);
}

glm::quat osc::Camera::getRotation() const
{
    return m_Impl->getRotation();
}

void osc::Camera::setRotation(glm::quat const& rotation)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setRotation(rotation);
}

glm::vec3 osc::Camera::getDirection() const
{
    return m_Impl->getDirection();
}

void osc::Camera::setDirection(glm::vec3 const& d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDirection(d);
}

glm::vec3 osc::Camera::getUpwardsDirection() const
{
    return m_Impl->getUpwardsDirection();
}

glm::mat4 osc::Camera::getViewMatrix() const
{
    return m_Impl->getViewMatrix();
}

void osc::Camera::setViewMatrix(glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setViewMatrix(m);
}

void osc::Camera::resetViewMatrix()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->resetViewMatrix();
}

glm::mat4 osc::Camera::getProjectionMatrix() const
{
    return m_Impl->getProjectionMatrix();
}

void osc::Camera::setProjectionMatrix(glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setProjectionMatrix(m);
}

void osc::Camera::resetProjectionMatrix()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->resetProjectionMatrix();
}

glm::mat4 osc::Camera::getViewProjectionMatrix() const
{
    return m_Impl->getViewProjectionMatrix();
}

glm::mat4 osc::Camera::getInverseViewProjectionMatrix() const
{
    return m_Impl->getInverseViewProjectionMatrix();
}

void osc::Camera::render()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->render();
}

bool osc::operator==(Camera const& a, Camera const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::operator!=(Camera const& a, Camera const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::operator<(Camera const& a, Camera const& b)
{
    return a.m_Impl < b.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, Camera const& camera)
{
    return o << "Camera(position = " << camera.getPosition() << ", direction = " << camera.getDirection() << ", projection = " << camera.getCameraProjection() << ')';
}


/////////////////////////////
//
// graphics context
//
/////////////////////////////

namespace
{
    // create an OpenGL context for an application window
    sdl::GLContext CreateOpenGLContext(SDL_Window* window)
    {
        osc::log::info("initializing application OpenGL context");

        sdl::GLContext ctx = sdl::GL_CreateContext(window);

        // enable the context
        if (SDL_GL_MakeCurrent(window, ctx) != 0)
        {
            throw std::runtime_error{std::string{"SDL_GL_MakeCurrent failed: "} + SDL_GetError()};
        }

        // enable vsync by default
        //
        // vsync can feel a little laggy on some systems, but vsync reduces CPU usage
        // on *constrained* systems (e.g. laptops, which the majority of users are using)
        if (SDL_GL_SetSwapInterval(-1) != 0)
        {
            SDL_GL_SetSwapInterval(1);
        }

        // initialize GLEW
        //
        // effectively, enables the OpenGL API used by this application
        if (auto const err = glewInit(); err != GLEW_OK)
        {
            std::stringstream ss;
            ss << "glewInit() failed: ";
            ss << glewGetErrorString(err);
            throw std::runtime_error{ss.str()};
        }

        // depth testing used to ensure geometry overlaps correctly
        glEnable(GL_DEPTH_TEST);

        // MSXAA is used to smooth out the model
        glEnable(GL_MULTISAMPLE);

        // print OpenGL information if in debug mode
        osc::log::info(
            "OpenGL initialized: info: %s, %s, (%s), GLSL %s",
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER),
            glGetString(GL_VERSION),
            glGetString(GL_SHADING_LANGUAGE_VERSION)
        );

        return ctx;
    }

    // returns the maximum numbers of MSXAA samples the active OpenGL context supports
    GLint GetOpenGLMaxMSXAASamples(sdl::GLContext const&)
    {
        GLint v = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &v);

        // OpenGL spec: "the value must be at least 4"
        // see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml
        if (v < 4)
        {
            static bool g_ShowWarningOnce = [&]()
            {
                osc::log::warn("the current OpenGl backend only supports %i samples. Technically, this is invalid (4 *should* be the minimum)", v);
                return true;
            }();
            (void)g_ShowWarningOnce;
        }
        OSC_ASSERT(v < 1<<16 && "number of samples is greater than the maximum supported by the application");

        return v;
    }

    // maps an OpenGL debug message severity level to a log level
    constexpr osc::log::level::LevelEnum OpenGLDebugSevToLogLvl(GLenum sev) noexcept
    {
        switch (sev)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            return osc::log::level::err;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return osc::log::level::warn;
        case GL_DEBUG_SEVERITY_LOW:
            return osc::log::level::debug;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return osc::log::level::trace;
        default:
            return osc::log::level::info;
        }
    }

    // returns a string representation of an OpenGL debug message severity level
    constexpr char const* OpenGLDebugSevToCStr(GLenum sev) noexcept
    {
        switch (sev)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            return "GL_DEBUG_SEVERITY_HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "GL_DEBUG_SEVERITY_MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:
            return "GL_DEBUG_SEVERITY_LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "GL_DEBUG_SEVERITY_NOTIFICATION";
        default:
            return "GL_DEBUG_SEVERITY_UNKNOWN";
        }
    }

    // returns a string representation of an OpenGL debug message source
    constexpr char const* OpenGLDebugSrcToCStr(GLenum src) noexcept
    {
        switch (src)
        {
        case GL_DEBUG_SOURCE_API:
            return "GL_DEBUG_SOURCE_API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "GL_DEBUG_SOURCE_SHADER_COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "GL_DEBUG_SOURCE_THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "GL_DEBUG_SOURCE_APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
            return "GL_DEBUG_SOURCE_OTHER";
        default:
            return "GL_DEBUG_SOURCE_UNKNOWN";
        }
    }

    // returns a string representation of an OpenGL debug message type
    constexpr char const* OpenGLDebugTypeToCStr(GLenum type) noexcept
    {
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            return "GL_DEBUG_TYPE_ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "GL_DEBUG_TYPE_PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "GL_DEBUG_TYPE_PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:
            return "GL_DEBUG_TYPE_MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "GL_DEBUG_TYPE_PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "GL_DEBUG_TYPE_POP_GROUP";
        case GL_DEBUG_TYPE_OTHER:
            return "GL_DEBUG_TYPE_OTHER";
        default:
            return "GL_DEBUG_TYPE_UNKNOWN";
        }
    }

    // returns `true` if current OpenGL context is in debug mode
    bool IsOpenGLInDebugMode()
    {
        // if context is not debug-mode, then some of the glGet*s below can fail
        // (e.g. GL_DEBUG_OUTPUT_SYNCHRONOUS on apple).
        {
            GLint flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT))
            {
                return false;
            }
        }

        {
            GLboolean b = false;
            glGetBooleanv(GL_DEBUG_OUTPUT, &b);
            if (!b)
            {
                return false;
            }
        }

        {
            GLboolean b = false;
            glGetBooleanv(GL_DEBUG_OUTPUT_SYNCHRONOUS, &b);
            if (!b)
            {
                return false;
            }
        }

        return true;
    }

    // raw handler function that can be used with `glDebugMessageCallback`
    void OpenGLDebugMessageHandler(
        GLenum source,
        GLenum type,
        unsigned int id,
        GLenum severity,
        GLsizei,
        const char* message,
        void const*)
    {
        osc::log::level::LevelEnum lvl = OpenGLDebugSevToLogLvl(severity);
        char const* const sourceCStr = OpenGLDebugSrcToCStr(source);
        char const* const typeCStr = OpenGLDebugTypeToCStr(type);
        char const* const severityCStr = OpenGLDebugSevToCStr(severity);

        osc::log::log(lvl,
            R"(OpenGL Debug message:
id = %u
message = %s
source = %s
type = %s
severity = %s
)", id, message, sourceCStr, typeCStr, severityCStr);
    }

    // enable OpenGL API debugging
    void EnableOpenGLDebugMessages()
    {
        if (IsOpenGLInDebugMode())
        {
            osc::log::info("application appears to already be in OpenGL debug mode: skipping enabling it");
            return;
        }

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(OpenGLDebugMessageHandler, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            osc::log::info("enabled OpenGL debug mode");
        }
        else
        {
            osc::log::error("cannot enable OpenGL debug mode: the context does not have GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }

    // disable OpenGL API debugging
    void DisableOpenGLDebugMessages()
    {
        if (!IsOpenGLInDebugMode())
        {
            osc::log::info("application does not need to disable OpenGL debug mode: already in it: skipping");
            return;
        }

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glDisable(GL_DEBUG_OUTPUT);
            osc::log::info("disabled OpenGL debug mode");
        }
        else
        {
            osc::log::error("cannot disable OpenGL debug mode: the context does not have a GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }
}

class osc::GraphicsContext::Impl final {
public:
    Impl(SDL_Window* window) : m_GLContext{CreateOpenGLContext(window)}
    {
        m_QuadMaterial.setDepthTested(false);  // it's for fullscreen rendering
    }

    int getMaxMSXAASamples() const
    {
        return m_MaxMSXAASamples;
    }

    bool isVsyncEnabled() const
    {
        // adaptive vsync (-1) and vsync (1) are treated as "vsync is enabled"
        return SDL_GL_GetSwapInterval() != 0;
    }

    void enableVsync()
    {
        // try using adaptive vsync
        if (SDL_GL_SetSwapInterval(-1) == 0)
        {
            return;
        }

        // if adaptive vsync doesn't work, then try normal vsync
        if (SDL_GL_SetSwapInterval(1) == 0)
        {
            return;
        }

        // otherwise, setting vsync isn't supported by the system
    }

    void disableVsync()
    {
        SDL_GL_SetSwapInterval(0);
    }

    bool isInDebugMode() const
    {
        return m_DebugModeEnabled;
    }

    void enableDebugMode()
    {
        if (IsOpenGLInDebugMode())
        {
            return;  // already in debug mode
        }

        log::info("enabling debug mode");
        EnableOpenGLDebugMessages();
        m_DebugModeEnabled = true;
    }
    void disableDebugMode()
    {

        if (!IsOpenGLInDebugMode())
        {
            return;  // already not in debug mode
        }

        log::info("disabling debug mode");
        DisableOpenGLDebugMessages();
        m_DebugModeEnabled = false;
    }

    void clearProgram()
    {
        gl::UseProgram();
    }

    void clearScreen(glm::vec4 const& color)
    {
        gl::ClearColor(color.r, color.g, color.b, color.a);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void* updRawGLContextHandle()
    {
        return m_GLContext;
    }

    std::future<Image> requestScreenshot()
    {
        return m_ActiveScreenshotRequests.emplace_back().get_future();
    }

    void doSwapBuffers(SDL_Window* window)
    {
        // flush outstanding screenshot requests
        if (!m_ActiveScreenshotRequests.empty())
        {
            // copy GPU-side window framebuffer into CPU-side `osc::Image` object
            gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
            glm::ivec2 const dims = osc::App::get().idims();

            std::vector<uint8_t> pixels(4*dims.x*dims.y);
            glReadPixels(0, 0, dims.x, dims.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            Image screenshot{dims, pixels, 4};

            // copy image to requests [0..n-2]
            for (int i = 0, len = static_cast<int>(m_ActiveScreenshotRequests.size())-1; i < len; ++i)
            {
                m_ActiveScreenshotRequests[i].set_value(screenshot);
            }
            // move image to request `n-1`
            m_ActiveScreenshotRequests.back().set_value(std::move(screenshot));
            m_ActiveScreenshotRequests.clear();
        }

        SDL_GL_SwapWindow(window);
    }

    std::string getBackendVendorString() const
    {
        GLubyte const* const s = glGetString(GL_VENDOR);
        static_assert(sizeof(GLubyte) == sizeof(char));
        return reinterpret_cast<char const*>(s);
    }

    std::string getBackendRendererString() const
    {
        GLubyte const* const s = glGetString(GL_RENDERER);
        static_assert(sizeof(GLubyte) == sizeof(char));
        return reinterpret_cast<char const*>(s);
    }

    std::string getBackendVersionString() const
    {
        GLubyte const* const s = glGetString(GL_VERSION);
        static_assert(sizeof(GLubyte) == sizeof(char));
        return reinterpret_cast<char const*>(s);
    }

    std::string getBackendShadingLanguageVersionString() const
    {
        GLubyte const* const s = glGetString(GL_SHADING_LANGUAGE_VERSION);
        static_assert(sizeof(GLubyte) == sizeof(char));
        return reinterpret_cast<char const*>(s);
    }

private:

    // active OpenGL context for the application
    sdl::GLContext m_GLContext;

    // maximum number of samples supported by this hardware's OpenGL MSXAA API
    int m_MaxMSXAASamples = GetOpenGLMaxMSXAASamples(m_GLContext);

    // true if OpenGL's debug mode is enabled
    bool m_DebugModeEnabled = false;

    // a "queue" of active screenshot requests
    std::vector<std::promise<Image>> m_ActiveScreenshotRequests;

public:

    // a generic quad rendering material: used for some blitting operations
    Material m_QuadMaterial
    {
        Shader
        {
            g_QuadVertexShaderSrc,
            g_QuadFragmentShaderSrc,
        }
    };

    // a generic quad mesh: two triangles covering NDC @ Z=0
    Mesh m_QuadMesh = GenTexturedQuad();
};

static std::unique_ptr<osc::GraphicsContext::Impl> g_GraphicsContextImpl = nullptr;

osc::GraphicsContext::GraphicsContext(SDL_Window* window)
{
    if (g_GraphicsContextImpl)
    {
        throw std::runtime_error{"a graphics context has already been initialized: you cannot initialize a second"};
    }

    g_GraphicsContextImpl = std::make_unique<GraphicsContext::Impl>(window);
}

osc::GraphicsContext::~GraphicsContext() noexcept
{
    g_GraphicsContextImpl.reset();
}

int osc::GraphicsContext::getMaxMSXAASamples() const
{
    return g_GraphicsContextImpl->getMaxMSXAASamples();
}

bool osc::GraphicsContext::isVsyncEnabled() const
{
    return g_GraphicsContextImpl->isVsyncEnabled();
}

void osc::GraphicsContext::enableVsync()
{
    g_GraphicsContextImpl->enableVsync();
}

void osc::GraphicsContext::disableVsync()
{
    g_GraphicsContextImpl->disableVsync();
}

bool osc::GraphicsContext::isInDebugMode() const
{
    return g_GraphicsContextImpl->isInDebugMode();
}

void osc::GraphicsContext::enableDebugMode()
{
    g_GraphicsContextImpl->enableDebugMode();
}

void osc::GraphicsContext::disableDebugMode()
{
    g_GraphicsContextImpl->disableDebugMode();
}

void osc::GraphicsContext::clearProgram()
{
    g_GraphicsContextImpl->clearProgram();
}

void osc::GraphicsContext::clearScreen(glm::vec4 const& color)
{
    g_GraphicsContextImpl->clearScreen(color);
}

void* osc::GraphicsContext::updRawGLContextHandle()
{
    return g_GraphicsContextImpl->updRawGLContextHandle();
}

void osc::GraphicsContext::doSwapBuffers(SDL_Window* window)
{
    g_GraphicsContextImpl->doSwapBuffers(window);
}

std::future<osc::Image> osc::GraphicsContext::requestScreenshot()
{
    return g_GraphicsContextImpl->requestScreenshot();
}

std::string osc::GraphicsContext::getBackendVendorString() const
{
    return g_GraphicsContextImpl->getBackendVendorString();
}

std::string osc::GraphicsContext::getBackendRendererString() const
{
    return g_GraphicsContextImpl->getBackendRendererString();
}

std::string osc::GraphicsContext::getBackendVersionString() const
{
    return g_GraphicsContextImpl->getBackendVersionString();
}

std::string osc::GraphicsContext::getBackendShadingLanguageVersionString() const
{
    return g_GraphicsContextImpl->getBackendShadingLanguageVersionString();
}


/////////////////////////////
//
// drawing commands
//
/////////////////////////////

void osc::Graphics::DrawMesh(
    Mesh const& mesh,
    Transform const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, std::move(maybeMaterialPropertyBlock));
}

void osc::Graphics::DrawMesh(
    Mesh const& mesh,
    glm::mat4 const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, std::move(maybeMaterialPropertyBlock));
}

void osc::Graphics::Blit(Texture2D const& source, RenderTexture& dest)
{
    GraphicsBackend::Blit(source, dest);
}

void osc::Graphics::ReadPixels(RenderTexture const& source, Image& dest)
{
    GraphicsBackend::ReadPixels(source, dest);
}

void osc::Graphics::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    BlitFlags flags)
{
    GraphicsBackend::BlitToScreen(t, rect, std::move(flags));
}

void osc::Graphics::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    Material const& material,
    BlitFlags flags)
{
    GraphicsBackend::BlitToScreen(t, rect, material, std::move(flags));
}

/////////////////////////
//
// backend implementation
//
/////////////////////////

// helper: upload instancing data for a batch
std::optional<InstancingState> osc::GraphicsBackend::UploadInstanceData(
    nonstd::span<RenderObject const> els,
    osc::Shader::Impl const& shaderImpl)
{
    // preemptively upload instancing data
    std::optional<InstancingState> maybeInstancingState;

    if (shaderImpl.m_MaybeInstancedModelMatAttr || shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        // compute the stride between each instance
        size_t byteStride = 0;
        if (shaderImpl.m_MaybeInstancedModelMatAttr)
        {
            if (shaderImpl.m_MaybeInstancedModelMatAttr->Type == osc::ShaderType::Mat4)
            {
                byteStride += sizeof(float) * 16;
            }
        }
        if (shaderImpl.m_MaybeInstancedNormalMatAttr)
        {
            if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == osc::ShaderType::Mat4)
            {
                byteStride += sizeof(float) * 16;
            }
            else if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == osc::ShaderType::Mat3)
            {
                byteStride += sizeof(float) * 9;
            }
        }

        // write the instance data into a CPU-side buffer
        std::unique_ptr<float[]> const buf{new float[els.size() * (byteStride/sizeof(float))]};

        size_t floatOffset = 0;
        for (RenderObject const& el : els)
        {
            if (shaderImpl.m_MaybeInstancedModelMatAttr)
            {
                if (shaderImpl.m_MaybeInstancedModelMatAttr->Type == osc::ShaderType::Mat4)
                {
                    static_assert(alignof(glm::mat4) == alignof(float) && sizeof(glm::mat4) == 16 * sizeof(float));
                    reinterpret_cast<glm::mat4&>(buf[floatOffset]) = ModelMatrix(el);
                    floatOffset += 16;
                }
            }
            if (shaderImpl.m_MaybeInstancedNormalMatAttr)
            {
                if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == osc::ShaderType::Mat4)
                {
                    static_assert(alignof(glm::mat4) == alignof(float) && sizeof(glm::mat4) == 16 * sizeof(float));
                    reinterpret_cast<glm::mat4&>(buf[floatOffset]) = NormalMatrix4(el);
                    floatOffset += 16;
                }
                else if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == osc::ShaderType::Mat3)
                {
                    static_assert(alignof(glm::mat3) == alignof(float) && sizeof(glm::mat3) == 9 * sizeof(float));
                    reinterpret_cast<glm::mat3&>(buf[floatOffset]) = NormalMatrix(el);
                    floatOffset += 9;
                }
            }
        }
        OSC_ASSERT(sizeof(float)*floatOffset == els.size() * byteStride);

        OSC_PERF("GraphicsBackend::UploadInstanceData");
        gl::ArrayBuffer<float>& vbo = maybeInstancingState.emplace(byteStride).Buf;
        gl::BindBuffer(vbo);
        gl::BufferData(vbo.BufferType, sizeof(float) * floatOffset, buf.get(), GL_STREAM_DRAW);
    }
    return maybeInstancingState;
}

// helper: binds to instanced attributes (per-drawcall)
void osc::GraphicsBackend::BindToInstancedAttributes(
        Shader::Impl const& shaderImpl,
        InstancingState& ins)
{
    gl::BindBuffer(ins.Buf);

    size_t byteOffset = 0;
    if (shaderImpl.m_MaybeInstancedModelMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->Type == ShaderType::Mat4)
        {
            gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->Location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.Stride, ins.BaseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            byteOffset += sizeof(float) * 16;
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == ShaderType::Mat4)
        {
            gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->Location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.Stride, ins.BaseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            byteOffset += sizeof(float) * 16;
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == ShaderType::Mat3)
        {
            gl::AttributeMat3 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->Location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.Stride, ins.BaseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            byteOffset += sizeof(float) * 9;
        }
    }
}

// helper: unbinds from instanced attributes (per-drawcall)
void osc::GraphicsBackend::UnbindFromInstancedAttributes(
    Shader::Impl const& shaderImpl,
    InstancingState& ins)
{
    if (shaderImpl.m_MaybeInstancedModelMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->Type == ShaderType::Mat4)
        {
            gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->Location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == ShaderType::Mat4)
        {
            gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->Location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->Type == ShaderType::Mat3)
        {
            gl::AttributeMat3 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->Location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
    }
}

// helper: draw a batch of render objects that have the same material, material block, and mesh
void osc::GraphicsBackend::HandleBatchWithSameMesh(
    nonstd::span<RenderObject const> els,
    std::optional<InstancingState>& ins)
{
    OSC_PERF("GraphicsBackend::HandleBatchWithSameMesh");

    auto& meshImpl = const_cast<Mesh::Impl&>(*els.front().mesh.m_Impl);
    Shader::Impl& shaderImpl = *els.front().material.m_Impl->m_Shader.m_Impl;

    // in OpenGL 3.3, you *need* a VAO in order to render
    //
    // the rendering pipeline does this as late as possible to ensure that the
    // VAO isn't polluted with b.s. from other drawcalls (it can cause all kinds
    // of nasty bugs)
    gl::VertexArray vao;
    gl::BindVertexArray(vao);
    BindAndEnable(meshImpl.getOpenGLData());

    // if the shader requires per-instance uniforms, then we *have* to render one
    // instance at a time
    if (shaderImpl.m_MaybeModelMatUniform || shaderImpl.m_MaybeNormalMatUniform)
    {
        for (RenderObject const& el : els)
        {
            // try binding to uModel (standard)
            if (shaderImpl.m_MaybeModelMatUniform)
            {
                if (shaderImpl.m_MaybeModelMatUniform->Type == ShaderType::Mat4)
                {
                    gl::UniformMat4 u{shaderImpl.m_MaybeModelMatUniform->Location};
                    gl::Uniform(u, ModelMatrix(el));
                }
            }

            // try binding to uNormalMat (standard)
            if (shaderImpl.m_MaybeNormalMatUniform)
            {
                if (shaderImpl.m_MaybeNormalMatUniform->Type == osc::ShaderType::Mat3)
                {
                    gl::UniformMat3 u{shaderImpl.m_MaybeNormalMatUniform->Location};
                    gl::Uniform(u, NormalMatrix(el));
                }
                else if (shaderImpl.m_MaybeNormalMatUniform->Type == osc::ShaderType::Mat4)
                {
                    gl::UniformMat4 u{shaderImpl.m_MaybeNormalMatUniform->Location};
                    gl::Uniform(u, NormalMatrix4(el));
                }
            }

            if (ins)
            {
                BindToInstancedAttributes(shaderImpl, *ins);
            }
            meshImpl.drawInstanced(1);
            if (ins)
            {
                UnbindFromInstancedAttributes(shaderImpl, *ins);
                ins->BaseOffset += 1 * ins->Stride;
            }
        }
    }
    else
    {
        if (ins)
        {
            BindToInstancedAttributes(shaderImpl, *ins);
        }
        meshImpl.drawInstanced(els.size());
        if (ins)
        {
            UnbindFromInstancedAttributes(shaderImpl, *ins);
            ins->BaseOffset += els.size() * ins->Stride;
        }
    }

    UnbindAndDisable(meshImpl.getOpenGLData());
    gl::BindVertexArray();
}

// helper: draw a batch of render objects that have the same material and material block
void osc::GraphicsBackend::HandleBatchWithSameMaterialPropertyBlock(
    nonstd::span<RenderObject const> els,
    int& textureSlot,
    std::optional<InstancingState>& ins)
{
    OSC_PERF("GraphicsBackend::HandleBatchWithSameMaterialPropertyBlock");

    Material::Impl& matImpl = const_cast<Material::Impl&>(*els.front().material.m_Impl);
    Shader::Impl& shaderImpl = const_cast<Shader::Impl&>(*matImpl.m_Shader.m_Impl);
    robin_hood::unordered_map<std::string, ShaderElement> const& uniforms = shaderImpl.getUniforms();

    // bind property block variables (if applicable)
    if (els.front().maybePropBlock)
    {
        for (auto const& [name, value] : els.front().maybePropBlock->m_Impl->m_Values)
        {
            auto const it = uniforms.find(name);
            if (it != uniforms.end())
            {
                TryBindMaterialValueToShaderElement(it->second, value, &textureSlot);
            }
        }
    }

    // batch by mesh
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMesh{batchIt->mesh});
        HandleBatchWithSameMesh({batchIt, batchEnd}, ins);
        batchIt = batchEnd;
    }
}


void osc::GraphicsBackend::TryBindMaterialValueToShaderElement(ShaderElement const& se, MaterialValue const& v, int* textureSlot)
{
    if (GetShaderType(v) != se.Type)
    {
        return;  // mismatched types
    }

    switch (v.index())
    {
    case VariantIndex<MaterialValue, float>():
    {
        gl::UniformFloat u{se.Location};
        gl::Uniform(u, std::get<float>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<float>>():
    {
        auto const& vals = std::get<std::vector<float>>(v);
        int const numToAssign = std::min(se.Size, static_cast<int>(vals.size()));
        for (int i = 0; i < numToAssign; ++i)
        {
            gl::UniformFloat u{se.Location + i};
            gl::Uniform(u, vals[i]);
        }
        break;
    }
    case VariantIndex<MaterialValue, glm::vec2>():
    {
        gl::UniformVec2 u{se.Location};
        gl::Uniform(u, std::get<glm::vec2>(v));
        break;
    }
    case VariantIndex<MaterialValue, glm::vec3>():
    {
        gl::UniformVec3 u{se.Location};
        gl::Uniform(u, std::get<glm::vec3>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<glm::vec3>>():
    {
        auto const& vals = std::get<std::vector<glm::vec3>>(v);
        int const numToAssign = std::min(se.Size, static_cast<int>(vals.size()));
        for (int i = 0; i < numToAssign; ++i)
        {
            gl::UniformVec3 u{se.Location + i};
            gl::Uniform(u, vals[i]);
        }
        break;
    }
    case VariantIndex<MaterialValue, glm::vec4>():
    {
        gl::UniformVec4 u{se.Location};
        gl::Uniform(u, std::get<glm::vec4>(v));
        break;
    }
    case VariantIndex<MaterialValue, glm::mat3>():
    {
        gl::UniformMat3 u{se.Location};
        gl::Uniform(u, std::get<glm::mat3>(v));
        break;
    }
    case VariantIndex<MaterialValue, glm::mat4>():
    {
        gl::UniformMat4 u{se.Location};
        gl::Uniform(u, std::get<glm::mat4>(v));
        break;
    }
    case VariantIndex<MaterialValue, int>():
    {
        gl::UniformInt u{se.Location};
        gl::Uniform(u, std::get<int>(v));
        break;
    }
    case VariantIndex<MaterialValue, bool>():
    {
        gl::UniformBool u{se.Location};
        gl::Uniform(u, std::get<bool>(v));
        break;
    }
    case VariantIndex<MaterialValue, Texture2D>():
    {
        Texture2D::Impl& impl = *std::get<Texture2D>(v).m_Impl;
        gl::Texture2D& texture = impl.updTexture();

        gl::ActiveTexture(GL_TEXTURE0 + *textureSlot);
        gl::BindTexture(texture);
        gl::UniformSampler2D u{se.Location};
        gl::Uniform(u, *textureSlot);

        ++(*textureSlot);
        break;
    }
    case VariantIndex<MaterialValue, RenderTexture>():
    {
        RenderTexture::Impl& impl = *std::get<RenderTexture>(v).m_Impl;
        gl::Texture2D& texture = impl.getOutputTexture();

        gl::ActiveTexture(GL_TEXTURE0 + *textureSlot);
        gl::BindTexture(texture);
        gl::UniformSampler2D u{se.Location};
        gl::Uniform(u, *textureSlot);

        ++(*textureSlot);
        break;
    }
    default:
    {
        break;
    }
    }
}

// helper: draw a batch of render objects that have the same material
void osc::GraphicsBackend::HandleBatchWithSameMaterial(
    SceneState const& scene,
    nonstd::span<RenderObject const> els)
{
    OSC_PERF("GraphicsBackend::HandleBatchWithSameMaterial");

    Material::Impl& matImpl = const_cast<Material::Impl&>(*els.front().material.m_Impl);
    Shader::Impl& shaderImpl = const_cast<Shader::Impl&>(*matImpl.m_Shader.m_Impl);
    robin_hood::unordered_map<std::string, ShaderElement> const& uniforms = shaderImpl.getUniforms();

    // preemptively upload instance data
    std::optional<InstancingState> maybeInstances = UploadInstanceData(els, shaderImpl);

    // updated by various batches (which may bind to textures etc.)
    int textureSlot = 0;

    gl::UseProgram(shaderImpl.updProgram());

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // bind material variables
    {
        // try binding to uView (standard)
        if (shaderImpl.m_MaybeViewMatUniform)
        {
            if (shaderImpl.m_MaybeViewMatUniform->Type == ShaderType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewMatUniform->Location};
                gl::Uniform(u, scene.ViewMatrix);
            }
        }

        // try binding to uProjection (standard)
        if (shaderImpl.m_MaybeProjMatUniform)
        {
            if (shaderImpl.m_MaybeProjMatUniform->Type == ShaderType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeProjMatUniform->Location};
                gl::Uniform(u, scene.ProjectionMatrix);
            }
        }

        if (shaderImpl.m_MaybeViewProjMatUniform)
        {
            if (shaderImpl.m_MaybeViewProjMatUniform->Type == ShaderType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewProjMatUniform->Location};
                gl::Uniform(u, scene.ViewProjectionMatrix);
            }
        }

        // bind material values
        for (auto const& [name, value] : matImpl.m_Values)
        {
            if (ShaderElement const* e = TryGetValue(uniforms, name))
            {
                TryBindMaterialValueToShaderElement(*e, value, &textureSlot);
            }
        }
    }

    // batch by material property block
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMaterialPropertyBlock{batchIt->maybePropBlock});
        HandleBatchWithSameMaterialPropertyBlock({batchIt, batchEnd}, textureSlot, maybeInstances);
        batchIt = batchEnd;
    }

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// helper: draw a sequence of render objects (no presumptions)
void osc::GraphicsBackend::DrawBatchedByMaterial(
    SceneState const& scene,
    nonstd::span<RenderObject const> els)
{
    OSC_PERF("GraphicsBackend::DrawBatchedByMaterial");

    // batch by material
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMaterial{batchIt->material});
        HandleBatchWithSameMaterial(scene, {batchIt, batchEnd});
        batchIt = batchEnd;
    }
}

void osc::GraphicsBackend::DrawBatchedByOpaqueness(
    SceneState const& scene,
    nonstd::span<RenderObject const> els)
{
    OSC_PERF("GraphicsBackend::DrawBatchedByOpaqueness");

    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto batchEnd = std::find_if_not(batchIt, els.end(), IsOpaque);

        if (batchEnd != batchIt)
        {
            // opaque elements
            gl::Disable(GL_BLEND);
            DrawBatchedByMaterial(scene, {batchIt, batchEnd});
        }

        if (batchEnd != els.end())
        {
            auto transparentEnd = std::find_if(batchEnd, els.end(), IsOpaque);

            // transparent elements (assumed already sorted)
            gl::Enable(GL_BLEND);
            DrawBatchedByMaterial(scene, {batchEnd, transparentEnd});

            batchEnd = transparentEnd;
        }

        batchIt = batchEnd;
    }
}

void osc::GraphicsBackend::FlushRenderQueue(Camera::Impl& camera)
{
    OSC_PERF("GraphicsBackend::FlushRenderQueue");

    // flush the render queue in batches based on what's being rendered:
    //
    // - not-depth-tested elements (can't be reordered)
    // - depth-tested elements (can be reordered):
    //   - opaqueness (opaque first, then transparent back-to-front)
    //   - material
    //   - material property block
    //   - mesh

    std::vector<RenderObject>& queue = camera.m_RenderQueue;

    if (queue.empty())
    {
        return;
    }

    // precompute any scene state used by the rendering algs
    SceneState const scene
    {
        camera.getPosition(),
        camera.getViewMatrix(),
        camera.getProjectionMatrix(),
    };

    gl::Enable(GL_DEPTH_TEST);

    // draw by reordering depth-tested elements around the not-depth-tested elements
    auto batchIt = queue.begin();
    while (batchIt != queue.end())
    {
        auto end = std::find_if_not(batchIt, queue.end(), IsDepthTested);

        // these elements are depth-tested and, therefore, elegible for reordering
        SortRenderQueue(batchIt, end, scene.CameraPos);
        DrawBatchedByOpaqueness(scene, {batchIt, end});

        if (end != queue.end())
        {
            auto const ignoreDepthTestEnd = std::find_if(end, queue.end(), IsDepthTested);

            // these elements aren't depth-tested and should just be drawn as-is
            gl::Disable(GL_DEPTH_TEST);
            DrawBatchedByOpaqueness(scene, {end, ignoreDepthTestEnd});
            gl::Enable(GL_DEPTH_TEST);
            end = ignoreDepthTestEnd;
        }
        batchIt = end;
    }

    // queue flushed: clear it
    queue.clear();
}

void osc::GraphicsBackend::RenderScene(Camera::Impl& camera)
{
    OSC_PERF("GraphicsBackend::RenderScene");

    // setup generic pipeline state
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // setup output viewport
    {
        Rect const cameraRect = camera.getPixelRect();  // in "usual" screen space - topleft
        glm::vec2 const cameraRectBottomLeft = BottomLeft(cameraRect);
        glm::vec2 const viewportDims = camera.viewportDimensions();
        glm::ivec2 const outputDimensions = Dimensions(cameraRect);

        gl::Viewport(
            static_cast<GLsizei>(cameraRectBottomLeft.x),
            static_cast<GLsizei>(viewportDims.y - cameraRectBottomLeft.y),
            static_cast<GLsizei>(outputDimensions.x),
            static_cast<GLsizei>(outputDimensions.y)
        );
    }

    // setup scissor testing (if applicable)
    if (camera.m_MaybeScissorRect)
    {
        Rect const scissorRect = *camera.m_MaybeScissorRect;
        glm::ivec2 const scissorDims = Dimensions(scissorRect);

        gl::Enable(GL_SCISSOR_TEST);
        glScissor(
            static_cast<GLint>(scissorRect.p1.x),
            static_cast<GLint>(scissorRect.p1.y),
            scissorDims.x,
            scissorDims.y
        );
    }
    else
    {
        gl::Disable(GL_SCISSOR_TEST);
    }

    if (camera.m_ClearFlags != CameraClearFlags::Nothing)
    {
        // bind to output and clear it

        GLenum const clearFlags = camera.m_ClearFlags == CameraClearFlags::SolidColor ? 
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT :
            GL_DEPTH_BUFFER_BIT;

        gl::ClearColor(
            camera.m_BackgroundColor.r,
            camera.m_BackgroundColor.g,
            camera.m_BackgroundColor.b,
            camera.m_BackgroundColor.a
        );

        if (camera.m_MaybeTexture)
        {
            DoCopyOnWrite(camera.m_MaybeTexture->m_Impl);

            // clear the MSXAA-resolved output texture
            gl::BindFramebuffer(GL_FRAMEBUFFER, camera.m_MaybeTexture->m_Impl->getOutputFrameBuffer());
            gl::Clear(clearFlags);

            // clear the written-to MSXAA texture
            gl::BindFramebuffer(GL_FRAMEBUFFER, camera.m_MaybeTexture->m_Impl->getFrameBuffer());
            gl::Clear(clearFlags);
        }
        else
        {
            gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
            gl::Clear(clearFlags);
        }
    }
    else
    {
        // just bind to the output, but don't clear it

        if (camera.m_MaybeTexture)
        {
            gl::BindFramebuffer(GL_FRAMEBUFFER, camera.m_MaybeTexture->m_Impl->getFrameBuffer());
        }
        else
        {
            gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
        }
    }

    // DRAW: flush the render queue
    {
        FlushRenderQueue(camera);
    }

    // blit output to resolve MSXAA samples (if appliicable)
    if (camera.m_MaybeTexture)
    {
        OSC_PERF("GraphicsBackend::RenderScene/blit output (resolve MSXAA)");

        glm::ivec2 const dimensions = camera.m_MaybeTexture->m_Impl->m_Descriptor.getDimensions();

        // blit multisampled scene render to not-multisampled texture
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, (*camera.m_MaybeTexture->m_Impl->m_MaybeGPUBuffers)->MultisampledFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, (*camera.m_MaybeTexture->m_Impl->m_MaybeGPUBuffers)->SingleSampledFBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(
            0,
            0,
            dimensions.x,
            dimensions.y,
            0,
            0,
            dimensions.x,
            dimensions.y,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
            GL_NEAREST
        );

        // rebind to the screen (the start of RenderScene bound to the output texture)
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }

    if (camera.m_MaybeScissorRect)
    {
        gl::Disable(GL_SCISSOR_TEST);
    }

    // cleanup
    gl::UseProgram();
}

void osc::GraphicsBackend::DrawMesh(
    Mesh const& mesh,
    Transform const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock)
{
    DoCopyOnWrite(camera.m_Impl);
    camera.m_Impl->m_RenderQueue.emplace_back(mesh, transform, material, std::move(maybeMaterialPropertyBlock));
}

void osc::GraphicsBackend::DrawMesh(
    Mesh const& mesh,
    glm::mat4 const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> maybeMaterialPropertyBlock)
{
    DoCopyOnWrite(camera.m_Impl);
    camera.m_Impl->m_RenderQueue.emplace_back(mesh, transform, material, std::move(maybeMaterialPropertyBlock));
}

void osc::GraphicsBackend::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    BlitFlags flags)
{
    OSC_ASSERT(g_GraphicsContextImpl);
    OSC_ASSERT(*t.m_Impl->m_MaybeGPUBuffers && "the input texture has not been rendered to");

    if (flags == BlitFlags::AlphaBlend)
    {
        Camera c;
        c.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
        c.setPixelRect(rect);
        c.setProjectionMatrix(glm::mat4{1.0f});
        c.setViewMatrix(glm::mat4{1.0f});
        c.setClearFlags(CameraClearFlags::Nothing);

        g_GraphicsContextImpl->m_QuadMaterial.setRenderTexture("uTexture", t);
        Graphics::DrawMesh(g_GraphicsContextImpl->m_QuadMesh, Transform{}, g_GraphicsContextImpl->m_QuadMaterial, c);
        c.render();
        g_GraphicsContextImpl->m_QuadMaterial.clearRenderTexture("uTexture");
    }
    else
    {
        // rect is currently top-left, must be converted to bottom-left

        int const windowHeight = App::get().idims().y;
        int const rectHeight = static_cast<int>(rect.p2.y - rect.p1.y);
        int const p1y = static_cast<int>((windowHeight - rect.p1.y) - rectHeight);
        int const p2y = static_cast<int>(windowHeight - rect.p1.y);
        glm::ivec2 texDimensions = t.getDimensions();

        // blit multisampled scene render to not-multisampled texture
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, (*t.m_Impl->m_MaybeGPUBuffers)->SingleSampledFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, gl::windowFbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(
            0,
            0,
            texDimensions.x,
            texDimensions.y,
            static_cast<GLint>(rect.p1.x),
            static_cast<GLint>(p1y),
            static_cast<GLint>(rect.p2.x),
            static_cast<GLint>(p2y),
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );

        // rebind to the screen (the start bound to the output texture)
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }
}

void osc::GraphicsBackend::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    Material const& material,
    BlitFlags)
{
    OSC_ASSERT(g_GraphicsContextImpl);
    OSC_ASSERT(*t.m_Impl->m_MaybeGPUBuffers && "the input texture has not been rendered to");

    Camera c;
    c.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
    c.setPixelRect(rect);
    c.setProjectionMatrix(glm::mat4{1.0f});
    c.setViewMatrix(glm::mat4{1.0f});
    c.setClearFlags(CameraClearFlags::Nothing);

    Material copy{material};

    copy.setRenderTexture("uTexture", t);
    Graphics::DrawMesh(g_GraphicsContextImpl->m_QuadMesh, Transform{}, copy, c);
    c.render();
    copy.clearRenderTexture("uTexture");
}

void osc::GraphicsBackend::Blit(Texture2D const& source, RenderTexture& dest)
{
    Camera c;
    c.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
    c.setProjectionMatrix(glm::mat4{1.0f});
    c.setViewMatrix(glm::mat4{1.0f});

    g_GraphicsContextImpl->m_QuadMaterial.setTexture("uTexture", source);

    std::optional<RenderTexture> swapTex{std::move(dest)};
    c.swapTexture(swapTex);
    Graphics::DrawMesh(g_GraphicsContextImpl->m_QuadMesh, Transform{}, g_GraphicsContextImpl->m_QuadMaterial, c);
    c.render();
    c.swapTexture(swapTex);

    g_GraphicsContextImpl->m_QuadMaterial.clearTexture("uTexture");

    dest = std::move(swapTex).value();
}

void osc::GraphicsBackend::ReadPixels(RenderTexture const& source, Image& dest)
{
    glm::ivec2 const dims = source.getDimensions();
    int const channels = GetNumChannels(source.getColorFormat());

    std::vector<uint8_t> pixels(channels*dims.x*dims.y);

    gl::BindFramebuffer(GL_FRAMEBUFFER, source.m_Impl->getOutputFrameBuffer());
    glViewport(0, 0, dims.x, dims.y);
    glReadPixels(0, 0, dims.x, dims.y, ToOpenGLColorFormat(source.getColorFormat()), GL_UNSIGNED_BYTE, pixels.data());
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);

    dest = Image{dims, pixels, channels};
}
