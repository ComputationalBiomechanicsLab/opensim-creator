// these are the things that this file "implements"

#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/CameraClearFlags.hpp"
#include "oscar/Graphics/CameraProjection.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Cubemap.hpp"
#include "oscar/Graphics/DepthStencilFormat.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsContext.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MaterialPropertyBlock.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/MeshTopology.hpp"
#include "oscar/Graphics/RenderBuffer.hpp"
#include "oscar/Graphics/RenderBufferLoadAction.hpp"
#include "oscar/Graphics/RenderBufferStoreAction.hpp"
#include "oscar/Graphics/RenderTarget.hpp"
#include "oscar/Graphics/RenderTargetColorAttachment.hpp"
#include "oscar/Graphics/RenderTargetDepthAttachment.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/RenderTextureDescriptor.hpp"
#include "oscar/Graphics/RenderTextureFormat.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Graphics/TextureWrapMode.hpp"
#include "oscar/Graphics/TextureFilterMode.hpp"
#include "oscar/Graphics/TextureFormat.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/ShaderType.hpp"

// other includes...

#include "oscar/Bindings/Gl.hpp"
#include "oscar/Bindings/GlGlm.hpp"
#include "oscar/Bindings/GlmHelpers.hpp"
#include "oscar/Bindings/SDL2Helpers.hpp"
#include "oscar/Graphics/Image.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Rgba32.hpp"
#include "oscar/Graphics/ShaderLocationIndex.hpp"
#include "oscar/Maths/AABB.hpp"
#include "oscar/Maths/BVH.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Platform/Log.hpp"
#include "oscar/Utils/Assertions.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/DefaultConstructOnCopy.hpp"
#include "oscar/Utils/Perf.hpp"
#include "oscar/Utils/UID.hpp"
#include "oscar/Utils/VariantHelpers.hpp"

#include <ankerl/unordered_dense.h>
#include <GL/glew.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
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
static osc::CStringView constexpr c_QuadVertexShaderSrc = R"(
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
static osc::CStringView constexpr c_QuadFragmentShaderSrc = R"(
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
    void PushAsBytes(T const& v, std::vector<std::byte>& out)
    {
        out.insert(
            out.end(),
            reinterpret_cast<std::byte const*>(&v),  // it is always safe to cast to std::byte/char
            reinterpret_cast<std::byte const*>(&v) + sizeof(T)
        );
    }
}

// material value storage
//
// materials can store a variety of stuff (colors, positions, offsets, textures, etc.). This
// code defines how it's actually stored at runtime
namespace
{
    using MaterialValue = std::variant<
        osc::Color,
        std::vector<osc::Color>,
        float,
        std::vector<float>,
        glm::vec2,
        glm::vec3,
        std::vector<glm::vec3>,
        glm::vec4,
        glm::mat3,
        glm::mat4,
        std::vector<glm::mat4>,
        int32_t,
        bool,
        osc::Texture2D,
        osc::RenderTexture,
        osc::Cubemap
    >;

    osc::ShaderType GetShaderType(MaterialValue const& v)
    {
        using osc::VariantIndex;

        switch (v.index())
        {
        case VariantIndex<MaterialValue, osc::Color>():
        case VariantIndex<MaterialValue, std::vector<osc::Color>>():
            return osc::ShaderType::Vec4;
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
        case VariantIndex<MaterialValue, std::vector<glm::mat4>>():
            return osc::ShaderType::Mat4;
        case VariantIndex<MaterialValue, int32_t>():
            return osc::ShaderType::Int;
        case VariantIndex<MaterialValue, bool>():
            return osc::ShaderType::Bool;
        case VariantIndex<MaterialValue, osc::Texture2D>():
        case VariantIndex<MaterialValue, osc::RenderTexture>():
            return osc::ShaderType::Sampler2D;
        case VariantIndex<MaterialValue, osc::Cubemap>():
            return osc::ShaderType::SamplerCube;
        default:
            return osc::ShaderType::Unknown;
        }
    }
}

// shader (backend stuff)
namespace
{
    // LUT for human-readable form of the above
    auto constexpr c_ShaderTypeInternalStrings = osc::to_array<osc::CStringView>(
    {
        "Float",
        "Vec2",
        "Vec3",
        "Vec4",
        "Mat3",
        "Mat4",
        "Int",
        "Bool",
        "Sampler2D",
        "SamplerCube",
        "Unknown",
    });
    static_assert(c_ShaderTypeInternalStrings.size() == static_cast<size_t>(osc::ShaderType::TOTAL));

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
        case GL_SAMPLER_CUBE:
            return osc::ShaderType::SamplerCube;
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
        ShaderElement(
            int32_t location_,
            osc::ShaderType shaderType_,
            int32_t size_) :

            location{location_},
            shaderType{shaderType_},
            size{size_}
        {
        }

        int32_t location;
        osc::ShaderType shaderType;
        int32_t size;
    };

    void PrintShaderElement(std::ostream& o, std::string_view name, ShaderElement const& se)
    {
        o << "ShadeElement(name = " << name << ", location = " << se.location << ", shaderType = " << se.shaderType << ", size = " << se.size << ')';
    }

    template<typename Key>
    ShaderElement const* TryGetValue(ankerl::unordered_dense::map<std::string, ShaderElement> const& m, Key const& k)
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
            std::optional<osc::MaterialPropertyBlock> const& maybePropBlock_) :

            material{material_},
            mesh{mesh_},
            maybePropBlock{maybePropBlock_},
            transform{transform_},
            worldMidpoint{material.getTransparent() ? osc::TransformPoint(transform_, osc::Midpoint(mesh.getBounds())) : glm::vec3{}}
        {
        }

        RenderObject(
            osc::Mesh const& mesh_,
            glm::mat4 const& transform_,
            osc::Material const& material_,
            std::optional<osc::MaterialPropertyBlock> const& maybePropBlock_) :

            material{material_},
            mesh{mesh_},
            maybePropBlock{maybePropBlock_},
            transform{transform_},
            worldMidpoint{material.getTransparent() ? transform_ * glm::vec4{osc::Midpoint(mesh.getBounds()), 1.0f} : glm::vec3{}}
        {
        }

        friend void swap(RenderObject& a, RenderObject& b)
        {
            using std::swap;

            swap(a.material, b.material);
            swap(a.mesh, b.mesh);
            swap(a.maybePropBlock, b.maybePropBlock);
            swap(a.transform, b.transform);
            swap(a.worldMidpoint, b.worldMidpoint);
        }

        osc::Material material;
        osc::Mesh mesh;
        std::optional<osc::MaterialPropertyBlock> maybePropBlock;
        Mat4OrTransform transform;
        glm::vec3 worldMidpoint;
    };

    bool operator==(RenderObject const& a, RenderObject const& b) noexcept
    {
        return
            a.material == b.material &&
            a.mesh == b.mesh &&
            a.maybePropBlock == b.maybePropBlock &&
            a.transform == b.transform &&
            a.worldMidpoint == b.worldMidpoint;
    }

    bool operator!=(RenderObject const& a, RenderObject const& b) noexcept
    {
        return !(a == b);
    }

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
    class RenderObjectIsFartherFrom final {
    public:
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

    class RenderObjectHasMaterial final {
    public:
        explicit RenderObjectHasMaterial(osc::Material const* material) :
            m_Material{material}
        {
        }

        bool operator()(RenderObject const& ro) const
        {
            return ro.material == *m_Material;
        }
    private:
        osc::Material const* m_Material;
    };

    class RenderObjectHasMaterialPropertyBlock final {
    public:
        explicit RenderObjectHasMaterialPropertyBlock(std::optional<osc::MaterialPropertyBlock> const* mpb) :
            m_Mpb{mpb}
        {
        }

        bool operator()(RenderObject const& ro) const
        {
            return ro.maybePropBlock == *m_Mpb;
        }

    private:
        std::optional<osc::MaterialPropertyBlock> const* m_Mpb;
    };

    class RenderObjectHasMesh final {
    public:
        RenderObjectHasMesh(osc::Mesh const* mesh) :
            m_Mesh{mesh}
        {
        }

        bool operator()(RenderObject const& ro) const
        {
            return ro.mesh == *m_Mesh;
        }
    private:
        osc::Mesh const* m_Mesh;
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
                auto const materialBatchEnd = std::partition(materialBatchStart, opaqueEnd, RenderObjectHasMaterial{&materialBatchStart->material});

                // then sub-sub-partition by material property block
                auto propBatchStart = materialBatchStart;
                while (propBatchStart != materialBatchEnd)
                {
                    auto const propBatchEnd = std::partition(propBatchStart, materialBatchEnd, RenderObjectHasMaterialPropertyBlock{&propBatchStart->maybePropBlock});

                    // then sub-sub-sub-partition by mesh
                    auto meshBatchStart = propBatchStart;
                    while (meshBatchStart != propBatchEnd)
                    {
                        auto const meshBatchEnd = std::partition(meshBatchStart, propBatchEnd, RenderObjectHasMesh{&meshBatchStart->mesh});
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

        SceneState(
            glm::vec3 const& cameraPos_,
            glm::mat4 const& viewMatrix_,
            glm::mat4 const& projectionMatrix_) :

            cameraPos{cameraPos_},
            viewMatrix{viewMatrix_},
            projectionMatrix{projectionMatrix_}
        {
        }

        glm::vec3 cameraPos;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;
    };

    // the OpenGL data associated with an osc::Texture2D
    struct Texture2DOpenGLData final {
        gl::Texture2D texture;
        osc::UID textureParamsVersion;
    };

    struct RenderBufferOpenGLData final {
        gl::RenderBuffer multisampledRBO;
        gl::Texture2D singleSampledTexture;
    };

    // the OpenGL data associated with an osc::Mesh
    struct MeshOpenGLData final {
        osc::UID dataVersion;
        gl::TypedBufferHandle<GL_ARRAY_BUFFER> arrayBuffer;
        gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER> indicesBuffer;
        gl::VertexArray vao;
    };

    struct InstancingState final {
        InstancingState(
            gl::ArrayBuffer<float, GL_STREAM_DRAW>& buf_,
            size_t stride_) :

            buf{buf_},
            stride{stride_}
        {
        }

        gl::ArrayBuffer<float, GL_STREAM_DRAW>& buf;
        size_t stride = 0;
        size_t baseOffset = 0;
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
            int32_t& textureSlot,
            std::optional<InstancingState>& ins
        );

        static std::optional<InstancingState> UploadInstanceData(
            nonstd::span<RenderObject const>,
            osc::Shader::Impl const& shaderImpl
        );

        static void TryBindMaterialValueToShaderElement(
            ShaderElement const& se,
            MaterialValue const& v,
            int32_t& textureSlot
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

        static void ValidateRenderTarget(
            RenderTarget&
        );
        static Rect CalcViewportRect(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static Rect SetupTopLevelPipelineState(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static std::optional<gl::FrameBuffer> BindAndClearRenderBuffers(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static void ResolveRenderBuffers(
            RenderTarget& maybeCustomRenderTarget
        );
        static void FlushRenderQueue(
            Camera::Impl& camera,
            float aspectRatio
        );
        static void TeardownTopLevelPipelineState(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static void RenderScene(
            Camera::Impl& camera,
            RenderTarget* maybeCustomRenderTarget = nullptr
        );


        static void DrawMesh(
            Mesh const&,
            Transform const&,
            Material const&,
            Camera&,
            std::optional<MaterialPropertyBlock> const&
        );

        static void DrawMesh(
            Mesh const&,
            glm::mat4 const&,
            Material const&,
            Camera&,
            std::optional<MaterialPropertyBlock> const&
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

namespace
{
    // returns the memory alignment of data that is to be copied from the
    // CPU (packed) to the GPU (unpacked)
    constexpr GLint ToOpenGLUnpackAlignment(osc::TextureFormat format) noexcept
    {
        static_assert(static_cast<size_t>(osc::TextureFormat::TOTAL) == 4);

        switch (format)
        {
        case osc::TextureFormat::R8:
            return 1;
        case osc::TextureFormat::RGB24:
            return 1;
        case osc::TextureFormat::RGBA32:
            return 4;
        case osc::TextureFormat::RGBAFloat:
            return 4;
        default:
            return 1;
        }
    }

    // returns the format OpenGL will use internally (i.e. on the GPU) to
    // represent the given format+colorspace combo
    constexpr GLenum ToOpenGLInternalFormat(
        osc::TextureFormat format,
        osc::ColorSpace colorSpace) noexcept
    {
        static_assert(static_cast<size_t>(osc::TextureFormat::TOTAL) == 4);
        static_assert(static_cast<size_t>(osc::ColorSpace::TOTAL) == 2);

        switch (format)
        {
        case osc::TextureFormat::R8:
            return GL_R8;
        case osc::TextureFormat::RGB24:
            return colorSpace == osc::ColorSpace::sRGB ? GL_SRGB8 : GL_RGB8;
        case osc::TextureFormat::RGBA32:
            return colorSpace == osc::ColorSpace::sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        case osc::TextureFormat::RGBAFloat:
            return GL_RGBA32F;
        default:
            return GL_RGBA8;
        }
    }

    // used by the texture implementation to keep track of what kind of
    // data it is storing
    enum class CPUDataType {
        UnsignedByte = 0,
        Float,
        UnsignedInt24_8,
        HalfFloat,

        TOTAL,
    };

    constexpr GLenum ToOpenGLDataType(CPUDataType t) noexcept
    {
        static_assert(static_cast<size_t>(CPUDataType::TOTAL) == 4);

        switch (t)
        {
        case CPUDataType::UnsignedByte:
            return GL_UNSIGNED_BYTE;
        case CPUDataType::Float:
            return GL_FLOAT;
        case CPUDataType::UnsignedInt24_8:
            return GL_UNSIGNED_INT_24_8;
        case CPUDataType::HalfFloat:
            return GL_HALF_FLOAT;
        default:
            return GL_UNSIGNED_BYTE;
        }
    }

    constexpr CPUDataType ToEquivalentCPUDataType(osc::TextureFormat format) noexcept
    {
        static_assert(static_cast<size_t>(osc::TextureFormat::TOTAL) == 4);
        static_assert(static_cast<size_t>(CPUDataType::TOTAL) == 4);

        switch (format)
        {
        case osc::TextureFormat::R8:
        case osc::TextureFormat::RGB24:
        case osc::TextureFormat::RGBA32:
            return CPUDataType::UnsignedByte;
        case osc::TextureFormat::RGBAFloat:
            return CPUDataType::Float;
        default:
            return CPUDataType::UnsignedByte;  // static_assert means this isn't actually hit
        }
    }

    //  used by the texture implementation to keep track of what kind of
    // data it is storing
    enum class CPUImageFormat {
        R8 = 0,
        RGB,
        RGBA,
        DepthStencil,

        TOTAL,
    };

    constexpr CPUImageFormat ToEquivalentCPUImageFormat(osc::TextureFormat format) noexcept
    {
        static_assert(static_cast<size_t>(osc::TextureFormat::TOTAL) == 4);
        static_assert(static_cast<size_t>(CPUImageFormat::TOTAL) == 4);

        switch (format)
        {
        case osc::TextureFormat::R8:
            return CPUImageFormat::R8;
        case osc::TextureFormat::RGB24:
            return CPUImageFormat::RGB;
        case osc::TextureFormat::RGBA32:
            return CPUImageFormat::RGBA;
        case osc::TextureFormat::RGBAFloat:
            return CPUImageFormat::RGBA;
        default:
            return CPUImageFormat::RGBA;  // static_assert means this isn't actually hit
        }
    }

    constexpr GLenum ToOpenGLFormat(CPUImageFormat t) noexcept
    {
        static_assert(static_cast<size_t>(CPUImageFormat::TOTAL) == 4);

        switch (t)
        {
        case CPUImageFormat::R8:
            return GL_RED;
        case CPUImageFormat::RGB:
            return GL_RGB;
        case CPUImageFormat::RGBA:
            return GL_RGBA;
        case CPUImageFormat::DepthStencil:
            return GL_DEPTH_STENCIL;
        default:
            return GL_RGBA;
        }
    }
}

//////////////////////////////////
//
// cubemap stuff
//
//////////////////////////////////

namespace
{
    // the OpenGL data associated with an osc::Texture2D
    struct CubemapOpenGLData final {
        gl::TextureCubemap texture;
    };
}

class osc::Cubemap::Impl final {
public:
    Impl(int32_t width, TextureFormat format) :
        m_Width{width},
        m_Format{format}
    {
        OSC_THROWING_ASSERT(m_Width > 0 && "the width of a cubemap must be a positive number");

        auto const numFaces = static_cast<size_t>(osc::CubemapFace::TOTAL);
        size_t const numPixelsPerFace = static_cast<size_t>(m_Width*m_Width)*NumBytesPerPixel(m_Format);
        m_Data.resize(numFaces * numPixelsPerFace);
    }

    int32_t getWidth() const
    {
        return m_Width;
    }

    TextureFormat getTextureFormat() const
    {
        return m_Format;
    }

    void setPixelData(CubemapFace face, nonstd::span<uint8_t const> data)
    {
        auto const faceIndex = static_cast<size_t>(face);
        auto const numPixels = static_cast<size_t>(m_Width) * static_cast<size_t>(m_Width);
        size_t const numBytesPerCubeFace = numPixels * NumBytesPerPixel(m_Format);
        size_t const destinationDataStart = faceIndex * numBytesPerCubeFace;
        size_t const destinationDataEnd = destinationDataStart + numBytesPerCubeFace;

        OSC_ASSERT(faceIndex < static_cast<size_t>(CubemapFace::TOTAL) && "invalid cubemap face passed to Cubemap::setPixelData");
        OSC_THROWING_ASSERT(data.size() == numBytesPerCubeFace && "incorrect amount of data passed to Cubemap::setPixelData: the data must match the dimensions and texture format of the cubemap");
        OSC_ASSERT(destinationDataEnd <= m_Data.size() && "out of range assignment detected: this should be handled in the constructor");

        std::copy(data.begin(), data.end(), m_Data.begin() + destinationDataStart);
    }

    gl::TextureCubemap& updCubemap()
    {
        if (!*m_MaybeGPUTexture)
        {
            uploadToGPU();
        }
        OSC_ASSERT(*m_MaybeGPUTexture);

        CubemapOpenGLData& bufs = **m_MaybeGPUTexture;

        return bufs.texture;
    }
private:
    void uploadToGPU()
    {
        // create new OpenGL handle(s)
        *m_MaybeGPUTexture = CubemapOpenGLData{};

        // calculate CPU-to-GPU data transfer parameters
        size_t const numBytesPerPixel = NumBytesPerPixel(m_Format);
        size_t const numBytesPerRow = m_Width * numBytesPerPixel;
        size_t const numBytesPerFace = m_Width * numBytesPerRow;
        auto const numFaces = static_cast<size_t>(osc::CubemapFace::TOTAL);
        size_t const numBytesInCubemap = numFaces * numBytesPerFace;
        CPUDataType const cpuDataType = ToEquivalentCPUDataType(m_Format);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        CPUImageFormat const cpuChannelLayout = ToEquivalentCPUImageFormat(m_Format);  // TextureFormat's layout == CPU formats's layout for cubemaps
        GLint const unpackAlignment = ToOpenGLUnpackAlignment(m_Format);

        // sanity-check before doing anything with OpenGL
        OSC_ASSERT(numBytesPerRow % unpackAlignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(reinterpret_cast<intptr_t>(m_Data.data()) % unpackAlignment == 0 && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(numBytesInCubemap <= m_Data.size() && "the number of bytes in the cubemap (CPU-side) is less than expected: this is a developer bug");
        static_assert(NumTextureFormats() == 4, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");

        // upload cubemap to GPU
        gl::BindTexture((*m_MaybeGPUTexture)->texture);
        gl::PixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        for (GLint faceIdx = 0; faceIdx < static_cast<GLint>(osc::CubemapFace::TOTAL); ++faceIdx)
        {
            size_t const faceBytesBegin = faceIdx * numBytesPerFace;
            gl::TexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx,
                0,
                ToOpenGLInternalFormat(m_Format, osc::ColorSpace::sRGB),  // cubemaps are always sRGB
                m_Width,
                m_Width,
                0,
                ToOpenGLFormat(cpuChannelLayout),
                ToOpenGLDataType(cpuDataType),
                m_Data.data() + faceBytesBegin
            );
        }

        // set texture parameters
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // cleanup OpenGL binding state
        gl::BindTexture();
    }

    int32_t m_Width;
    TextureFormat m_Format;
    std::vector<uint8_t> m_Data;

    DefaultConstructOnCopy<std::optional<CubemapOpenGLData>> m_MaybeGPUTexture;
};

osc::Cubemap::Cubemap(int32_t width, TextureFormat format) :
    m_Impl{make_cow<Impl>(width, format)}
{
}
osc::Cubemap::Cubemap(Cubemap const&) = default;
osc::Cubemap::Cubemap(Cubemap&&) noexcept = default;
osc::Cubemap& osc::Cubemap::operator=(Cubemap const&) = default;
osc::Cubemap& osc::Cubemap::operator=(Cubemap&&) noexcept = default;
osc::Cubemap::~Cubemap() noexcept = default;

int32_t osc::Cubemap::getWidth() const
{
    return m_Impl->getWidth();
}

osc::TextureFormat osc::Cubemap::getTextureFormat() const
{
    return m_Impl->getTextureFormat();
}

void osc::Cubemap::setPixelData(CubemapFace face, nonstd::span<uint8_t const> channelsRowByRow)
{
    m_Impl.upd()->setPixelData(face, channelsRowByRow);
}


//////////////////////////////////
//
// texture stuff
//
//////////////////////////////////

namespace
{
    auto constexpr c_TextureWrapModeStrings = osc::to_array<osc::CStringView>(
    {
        "Repeat",
        "Clamp",
        "Mirror",
    });
    static_assert(c_TextureWrapModeStrings.size() == static_cast<size_t>(osc::TextureWrapMode::TOTAL));

    auto constexpr c_TextureFilterModeStrings = osc::to_array<osc::CStringView>(
    {
        "Nearest",
        "Linear",
        "Mipmap",
    });
    static_assert(c_TextureFilterModeStrings.size() == static_cast<size_t>(osc::TextureFilterMode::TOTAL));

    GLint ToGLTextureMinFilterParam(osc::TextureFilterMode m)
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

    GLint ToGLTextureMagFilterParam(osc::TextureFilterMode m)
    {
        switch (m)
        {
        case osc::TextureFilterMode::Nearest:
            return GL_NEAREST;
        case osc::TextureFilterMode::Linear:
        case osc::TextureFilterMode::Mipmap:
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
    Impl(
        glm::ivec2 dimensions,
        nonstd::span<Rgba32 const> pixelsRowByRow,
        ColorSpace colorSpace) :

        Impl
        {
            dimensions,
            TextureFormat::RGBA32,
            nonstd::span<uint8_t const>{&pixelsRowByRow[0].r, 4 * pixelsRowByRow.size()},
            colorSpace,
        }
    {
    }

    Impl(
        glm::ivec2 dimensions,
        TextureFormat format,
        nonstd::span<uint8_t const> channelsRowByRow,
        ColorSpace colorSpace) :

        m_Dimensions{dimensions},
        m_Format{format},
        m_ColorSpace{colorSpace},
        m_PixelData(channelsRowByRow.data(), channelsRowByRow.data() + channelsRowByRow.size())
    {
        OSC_THROWING_ASSERT(m_Dimensions.x >= 0 && m_Dimensions.y >= 0);
        OSC_THROWING_ASSERT(static_cast<ptrdiff_t>(m_Dimensions.x * m_Dimensions.y) == static_cast<ptrdiff_t>(m_PixelData.size()/NumBytesPerPixel(m_Format)));
    }

    glm::ivec2 getDimensions() const
    {
        return m_Dimensions;
    }

    float getAspectRatio() const
    {
        return AspectRatio(m_Dimensions);
    }

    ColorSpace getColorSpace() const
    {
        return m_ColorSpace;
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
        m_WrapModeU = twm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeV() const
    {
        return m_WrapModeV;
    }

    void setWrapModeV(TextureWrapMode twm)
    {
        m_WrapModeV = twm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeW() const
    {
        return m_WrapModeW;
    }

    void setWrapModeW(TextureWrapMode twm)
    {
        m_WrapModeW = twm;
        m_TextureParamsVersion.reset();
    }

    TextureFilterMode getFilterMode() const
    {
        return m_FilterMode;
    }

    void setFilterMode(TextureFilterMode tfm)
    {
        m_FilterMode = tfm;
        m_TextureParamsVersion.reset();
    }

    void* getTextureHandleHACK() const
    {
        // yes, this is a shitshow of casting, const-casting, etc. - it's purely here until and osc-specific
        // ImGui backend is written
        return reinterpret_cast<void*>(static_cast<uintptr_t>(const_cast<Impl&>(*this).updTexture().get()));
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

        if (bufs.textureParamsVersion != m_TextureParamsVersion)
        {
            setTextureParams(bufs);
        }

        return bufs.texture;
    }

private:
    void uploadToGPU()
    {
        *m_MaybeGPUTexture = Texture2DOpenGLData{};

        size_t const numBytesPerPixel = NumBytesPerPixel(m_Format);
        size_t const numBytesPerRow = m_Dimensions.x * numBytesPerPixel;
        GLint const unpackAlignment = ToOpenGLUnpackAlignment(m_Format);
        CPUDataType const cpuDataType = ToEquivalentCPUDataType(m_Format);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        CPUImageFormat const cpuChannelLayout = ToEquivalentCPUImageFormat(m_Format);  // TextureFormat's layout == CPU formats's layout for cubemaps

        static_assert(NumTextureFormats() == 4, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");
        OSC_ASSERT(numBytesPerRow % unpackAlignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(reinterpret_cast<intptr_t>(m_PixelData.data()) % unpackAlignment == 0 && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");

        // one-time upload, because pixels cannot be altered
        gl::BindTexture((*m_MaybeGPUTexture)->texture);
        gl::PixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        gl::TexImage2D(
            GL_TEXTURE_2D,
            0,
            ToOpenGLInternalFormat(m_Format, m_ColorSpace),
            m_Dimensions.x,
            m_Dimensions.y,
            0,
            ToOpenGLFormat(cpuChannelLayout),
            ToOpenGLDataType(cpuDataType),
            m_PixelData.data()
        );
        glGenerateMipmap((*m_MaybeGPUTexture)->texture.type);
        gl::BindTexture();
    }

    void setTextureParams(Texture2DOpenGLData& bufs)
    {
        gl::BindTexture(bufs.texture);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ToGLTextureTextureWrapParam(m_WrapModeU));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ToGLTextureTextureWrapParam(m_WrapModeV));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, ToGLTextureTextureWrapParam(m_WrapModeW));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ToGLTextureMinFilterParam(m_FilterMode));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ToGLTextureMagFilterParam(m_FilterMode));
        gl::BindTexture();
        bufs.textureParamsVersion = m_TextureParamsVersion;
    }

    friend class GraphicsBackend;

    glm::ivec2 m_Dimensions;
    TextureFormat m_Format;
    ColorSpace m_ColorSpace;
    std::vector<uint8_t> m_PixelData;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Nearest;
    UID m_TextureParamsVersion;

    DefaultConstructOnCopy<std::optional<Texture2DOpenGLData>> m_MaybeGPUTexture;
};

std::ostream& osc::operator<<(std::ostream& o, TextureWrapMode twm)
{
    return o << c_TextureWrapModeStrings.at(static_cast<size_t>(twm));
}

std::ostream& osc::operator<<(std::ostream& o, TextureFilterMode twm)
{
    return o << c_TextureFilterModeStrings.at(static_cast<size_t>(twm));
}

size_t osc::NumChannels(TextureFormat format) noexcept
{
    static_assert(NumTextureFormats() == 4);

    switch (format)
    {
    case TextureFormat::R8: return 1;
    case TextureFormat::RGBA32: return 4;
    case TextureFormat::RGB24: return 3;
    case TextureFormat::RGBAFloat: return 4;
    default: return 4;  // static_assert ensure this shouldn't be hit
    }
}

size_t osc::NumBytesPerChannel(TextureFormat format) noexcept
{
    static_assert(NumTextureFormats() == 4);

    switch (format)
    {
    case TextureFormat::R8: return 1;
    case TextureFormat::RGBA32: return 1;
    case TextureFormat::RGB24: return 1;
    case TextureFormat::RGBAFloat: return 4;
    default: return 1;  // static_assert ensure this shouldn't be hit
    }
}

size_t osc::NumBytesPerPixel(TextureFormat format) noexcept
{
    return NumChannels(format) * NumBytesPerChannel(format);
}

template<> std::optional<osc::TextureFormat> osc::ToTextureFormat<uint8_t>(size_t numChannels) noexcept
{
    static_assert(NumTextureFormats() == 4);

    switch (numChannels)
    {
    case 1: return TextureFormat::R8;
    case 3: return TextureFormat::RGB24;
    case 4: return TextureFormat::RGBA32;
    default: return std::nullopt;
    }
}

template<> std::optional<osc::TextureFormat> osc::ToTextureFormat<float>(size_t numChannels) noexcept
{
    static_assert(NumTextureFormats() == 4);

    switch (numChannels)
    {
    case 4: return osc::TextureFormat::RGBAFloat;
    default: return std::nullopt;
    }
}

osc::Texture2D::Texture2D(
    glm::ivec2 dimensions,
    nonstd::span<Rgba32 const> pixels,
    ColorSpace colorSpace) :

    m_Impl{make_cow<Impl>(dimensions, pixels, colorSpace)}
{
}

osc::Texture2D::Texture2D(
    glm::ivec2 dimensions,
    TextureFormat format,
    nonstd::span<uint8_t const> channelsRowByRow,
    ColorSpace colorSpace) :

    m_Impl{make_cow<Impl>(dimensions, format, channelsRowByRow, colorSpace)}
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

osc::ColorSpace osc::Texture2D::getColorSpace() const
{
    return m_Impl->getColorSpace();
}

osc::TextureWrapMode osc::Texture2D::getWrapMode() const
{
    return m_Impl->getWrapMode();
}

void osc::Texture2D::setWrapMode(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapMode(twm);
}

osc::TextureWrapMode osc::Texture2D::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::Texture2D::setWrapModeU(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapModeU(twm);
}

osc::TextureWrapMode osc::Texture2D::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::Texture2D::setWrapModeV(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapModeV(twm);
}

osc::TextureWrapMode osc::Texture2D::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::Texture2D::setWrapModeW(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapModeW(twm);
}

osc::TextureFilterMode osc::Texture2D::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::Texture2D::setFilterMode(TextureFilterMode twm)
{
    m_Impl.upd()->setFilterMode(twm);
}

void* osc::Texture2D::getTextureHandleHACK() const
{
    return m_Impl->getTextureHandleHACK();
}

std::ostream& osc::operator<<(std::ostream& o, Texture2D const&)
{
    return o << "Texture2D()";
}


//////////////////////////////////
//
// render texture
//
//////////////////////////////////

namespace
{
    auto constexpr c_RenderTextureFormatStrings = osc::to_array<osc::CStringView>(
    {
        "ARGB32",
        "RED",
        "ARGBHalf",
    });
    static_assert(c_RenderTextureFormatStrings.size() == static_cast<size_t>(osc::RenderTextureFormat::TOTAL));

    auto constexpr c_DepthStencilFormatStrings = osc::to_array<osc::CStringView>(
    {
        "D24_UNorm_S8_UInt",
    });
    static_assert(c_DepthStencilFormatStrings.size() == static_cast<size_t>(osc::DepthStencilFormat::TOTAL));

    GLenum ToInternalOpenGLColorFormat(
        osc::RenderBufferType type,
        osc::RenderTextureDescriptor const& desc)
    {
        static_assert(static_cast<size_t>(osc::RenderBufferType::TOTAL) == 2, "review code below, which treats RenderBufferType as a bool");
        if (type == osc::RenderBufferType::Depth)
        {
            return GL_DEPTH24_STENCIL8;
        }
        else
        {
            static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);
            static_assert(static_cast<size_t>(osc::RenderTextureReadWrite::TOTAL) == 2);

            switch (desc.getColorFormat())
            {
            case osc::RenderTextureFormat::RED:
                return GL_RED;
            case osc::RenderTextureFormat::ARGBHalf:
                return GL_RGBA16F;
            case osc::RenderTextureFormat::ARGB32:
            default:
                return desc.getReadWrite() == osc::RenderTextureReadWrite::sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            }
        }
    }

    constexpr CPUImageFormat ToEquivalentCPUImageFormat(
        osc::RenderBufferType type,
        osc::RenderTextureDescriptor const& desc) noexcept
    {
        static_assert(static_cast<size_t>(osc::RenderBufferType::TOTAL) == 2);
        static_assert(static_cast<size_t>(osc::DepthStencilFormat::TOTAL) == 1);
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);
        static_assert(static_cast<size_t>(CPUImageFormat::TOTAL) == 4);

        if (type == osc::RenderBufferType::Depth)
        {
            return CPUImageFormat::DepthStencil;
        }
        else
        {
            switch (desc.getColorFormat())
            {
            case osc::RenderTextureFormat::ARGB32:
                return CPUImageFormat::RGBA;
            case osc::RenderTextureFormat::RED:
                return CPUImageFormat::R8;
            case osc::RenderTextureFormat::ARGBHalf:
                return CPUImageFormat::RGBA;
            default:
                return CPUImageFormat::RGBA;  // shouldn't be hit (static asserts)
            }
        }
    }

    constexpr CPUDataType ToEquivalentCPUDataType(
        osc::RenderBufferType type,
        osc::RenderTextureDescriptor const& desc) noexcept
    {
        static_assert(static_cast<size_t>(osc::RenderBufferType::TOTAL) == 2);
        static_assert(static_cast<size_t>(osc::DepthStencilFormat::TOTAL) == 1);
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);
        static_assert(static_cast<size_t>(CPUDataType::TOTAL) == 4);

        if (type == osc::RenderBufferType::Depth)
        {
            return CPUDataType::UnsignedInt24_8;
        }
        else
        {
            switch (desc.getColorFormat())
            {
            case osc::RenderTextureFormat::ARGB32:
                return CPUDataType::UnsignedByte;
            case osc::RenderTextureFormat::RED:
                return CPUDataType::UnsignedByte;
            case osc::RenderTextureFormat::ARGBHalf:
                return CPUDataType::HalfFloat;
            default:
                return CPUDataType::UnsignedByte;  // shouldn't be hit
            }
        }
    }

    GLenum ToOpenGLPixelDataType(osc::RenderTextureFormat f)
    {
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);

        switch (f)
        {
        case osc::RenderTextureFormat::ARGBHalf:
            return GL_HALF_FLOAT;
        case osc::RenderTextureFormat::RED:
        case osc::RenderTextureFormat::ARGB32:
        default:
            return GL_UNSIGNED_BYTE;
        }
    }

    GLenum ToImageColorFormat(osc::RenderTextureFormat f)
    {
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);

        switch (f)
        {
        case osc::RenderTextureFormat::RED:
            return GL_RED;
        case osc::RenderTextureFormat::ARGBHalf:
        case osc::RenderTextureFormat::ARGB32:
        default:
            return GL_RGBA;
        }
    }

    GLint ToImagePixelPackAlignment(osc::RenderTextureFormat f)
    {
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);

        switch (f)
        {
        case osc::RenderTextureFormat::ARGBHalf:
        case osc::RenderTextureFormat::ARGB32:
            return 4;
        case osc::RenderTextureFormat::RED:
        default:
            return 1;
        }
    }

    GLenum ToImageDataType(osc::RenderTextureFormat)
    {
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);

        return GL_UNSIGNED_BYTE;
    }

    int32_t GetNumChannels(osc::RenderTextureFormat f)
    {
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3);

        switch (f)
        {
        case osc::RenderTextureFormat::RED:
            return 1;
        case osc::RenderTextureFormat::ARGBHalf:
        case osc::RenderTextureFormat::ARGB32:
        default:
            return 4;
        }
    }
}

std::ostream& osc::operator<<(std::ostream& o, RenderTextureFormat f)
{
    return o << c_RenderTextureFormatStrings.at(static_cast<size_t>(f));
}

std::ostream& osc::operator<<(std::ostream& o, DepthStencilFormat f)
{
    return o << c_DepthStencilFormatStrings.at(static_cast<size_t>(f));
}

osc::RenderTextureDescriptor::RenderTextureDescriptor(glm::ivec2 dimensions) :
    m_Dimensions{Max(dimensions, glm::ivec2{0, 0})},
    m_Dimension{TextureDimension::Tex2D},
    m_AnialiasingLevel{1},
    m_ColorFormat{RenderTextureFormat::ARGB32},
    m_DepthStencilFormat{DepthStencilFormat::D24_UNorm_S8_UInt},
    m_ReadWrite{RenderTextureReadWrite::Default}
{
}

glm::ivec2 osc::RenderTextureDescriptor::getDimensions() const
{
    return m_Dimensions;
}

void osc::RenderTextureDescriptor::setDimensions(glm::ivec2 d)
{
    OSC_THROWING_ASSERT(d.x >= 0 && d.y >= 0);
    m_Dimensions = d;
}

osc::TextureDimension osc::RenderTextureDescriptor::getDimension() const
{
    return m_Dimension;
}

void osc::RenderTextureDescriptor::setDimension(TextureDimension newDimension)
{
    m_Dimension = newDimension;
}

int32_t osc::RenderTextureDescriptor::getAntialiasingLevel() const
{
    return m_AnialiasingLevel;
}

void osc::RenderTextureDescriptor::setAntialiasingLevel(int32_t level)
{
    OSC_THROWING_ASSERT(level <= 64 && osc::popcount(static_cast<uint32_t>(level)) == 1);
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

osc::RenderTextureReadWrite osc::RenderTextureDescriptor::getReadWrite() const
{
    return m_ReadWrite;
}

void osc::RenderTextureDescriptor::setReadWrite(RenderTextureReadWrite rw)
{
    m_ReadWrite = rw;
}

bool osc::operator==(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return
        a.m_Dimensions == b.m_Dimensions &&
        a.m_Dimension == b.m_Dimension &&
        a.m_AnialiasingLevel == b.m_AnialiasingLevel &&
        a.m_ColorFormat == b.m_ColorFormat &&
        a.m_DepthStencilFormat == b.m_DepthStencilFormat &&
        a.m_ReadWrite == b.m_ReadWrite;
}

bool osc::operator!=(RenderTextureDescriptor const& a, RenderTextureDescriptor const& b)
{
    return !(a == b);
}

std::ostream& osc::operator<<(std::ostream& o, RenderTextureDescriptor const& rtd)
{
    return o <<
        "RenderTextureDescriptor(width = " << rtd.m_Dimensions.x
        << ", height = " << rtd.m_Dimensions.y
        << ", aa = " << rtd.m_AnialiasingLevel
        << ", colorFormat = " << rtd.m_ColorFormat
        << ", depthFormat = " << rtd.m_DepthStencilFormat
        << ")";
}

class osc::RenderBuffer::Impl final {
public:
    Impl(
        RenderTextureDescriptor const& descriptor_,
        RenderBufferType type_) :

        m_Descriptor{descriptor_},
        m_BufferType{type_}
    {
        OSC_THROWING_ASSERT((getDimension() != TextureDimension::Cube || getDimensions().x == getDimensions().y) && "cannot construct a Cube renderbuffer with non-square dimensions");
        OSC_THROWING_ASSERT((getDimension() != TextureDimension::Cube || getAntialiasingLevel() == 1) && "cannot construct a Cube renderbuffer that is anti-aliased (not supported by backends like OpenGL)");
    }

    void reformat(RenderTextureDescriptor const& newDescriptor)
    {
        OSC_THROWING_ASSERT((newDescriptor.getDimension() != TextureDimension::Cube || newDescriptor.getDimensions().x == newDescriptor.getDimensions().y) && "cannot reformat a render buffer to a Cube dimensionality with non-square dimensions");
        OSC_THROWING_ASSERT((newDescriptor.getDimension() != TextureDimension::Cube || newDescriptor.getAntialiasingLevel() == 1) && "cannot reformat a renderbuffer to a Cube dimensionality with is anti-aliased (not supported by backends like OpenGL)");

        if (m_Descriptor != newDescriptor)
        {
            m_Descriptor = newDescriptor;
            m_MaybeOpenGLData->reset();
        }
    }

    RenderTextureDescriptor const& getDescriptor() const
    {
        return m_Descriptor;
    }

    glm::ivec2 getDimensions() const
    {
        return m_Descriptor.getDimensions();
    }

    void setDimensions(glm::ivec2 newDims)
    {
        OSC_THROWING_ASSERT((getDimension() != TextureDimension::Cube || newDims.x == newDims.y) && "cannot set a cubemap to have non-square dimensions");

        if (newDims != getDimensions())
        {
            m_Descriptor.setDimensions(newDims);
            m_MaybeOpenGLData->reset();
        }
    }

    TextureDimension getDimension() const
    {
        return m_Descriptor.getDimension();
    }

    void setDimension(TextureDimension newDimension)
    {
        OSC_THROWING_ASSERT((newDimension != osc::TextureDimension::Cube || getDimensions().x == getDimensions().y) && "cannot set dimensionality to Cube for non-square render buffer");
        OSC_THROWING_ASSERT((newDimension != TextureDimension::Cube || getAntialiasingLevel() == 1) && "cannot set dimensionality to Cube for an anti-aliased render buffer (not supported by backends like OpenGL)");

        if (newDimension != getDimension())
        {
            m_Descriptor.setDimension(newDimension);
            m_MaybeOpenGLData->reset();
        }
    }

    RenderTextureFormat getColorFormat() const
    {
        return m_Descriptor.getColorFormat();
    }

    void setColorFormat(RenderTextureFormat newFormat)
    {
        if (newFormat != getColorFormat())
        {
            m_Descriptor.setColorFormat(newFormat);
            m_MaybeOpenGLData->reset();
        }
    }

    int32_t getAntialiasingLevel() const
    {
        return m_Descriptor.getAntialiasingLevel();
    }

    void setAntialiasingLevel(int32_t newLevel)
    {
        OSC_THROWING_ASSERT((getDimension() != TextureDimension::Cube || newLevel == 1) && "cannot set anti-aliasing level >1 on a cube render buffer (it is not supported by backends like OpenGL)");

        if (newLevel != getAntialiasingLevel())
        {
            m_Descriptor.setAntialiasingLevel(newLevel);
            m_MaybeOpenGLData->reset();
        }
    }

    DepthStencilFormat getDepthStencilFormat() const
    {
        return m_Descriptor.getDepthStencilFormat();
    }

    void setDepthStencilFormat(DepthStencilFormat newDepthStencilFormat)
    {
        if (newDepthStencilFormat != getDepthStencilFormat())
        {
            m_Descriptor.setDepthStencilFormat(newDepthStencilFormat);
            m_MaybeOpenGLData->reset();
        }
    }

    RenderTextureReadWrite getReadWrite() const
    {
        return m_Descriptor.getReadWrite();
    }

    void setReadWrite(RenderTextureReadWrite newReadWrite)
    {
        if (newReadWrite != m_Descriptor.getReadWrite())
        {
            m_Descriptor.setReadWrite(newReadWrite);
            m_MaybeOpenGLData->reset();
        }
    }

    gl::RenderBuffer& updRBO()
    {
        if (!*m_MaybeOpenGLData)
        {
            uploadToGPU();
        }
        return (*m_MaybeOpenGLData)->multisampledRBO;
    }

    gl::Texture2D& updResolvedTexture()
    {
        if (!*m_MaybeOpenGLData)
        {
            uploadToGPU();
        }
        return (*m_MaybeOpenGLData)->singleSampledTexture;
    }

    void uploadToGPU()
    {
        // TODO: figure out how to upload cubemaps

        RenderBufferOpenGLData& data = m_MaybeOpenGLData->emplace();
        glm::ivec2 const dimensions = m_Descriptor.getDimensions();

        // setup multisampled RBO
        gl::BindRenderBuffer(data.multisampledRBO);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            m_Descriptor.getAntialiasingLevel(),
            ToInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
            dimensions.x,
            dimensions.y
        );
        gl::BindRenderBuffer();

        // setup resolved texture
        static_assert(static_cast<size_t>(osc::RenderTextureFormat::TOTAL) == 3, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");
        static_assert(static_cast<size_t>(osc::RenderBufferType::TOTAL) == 2, "review code below, which treats RenderBufferType as a bool");
        gl::BindTexture(data.singleSampledTexture);
        gl::TexImage2D(
            GL_TEXTURE_2D,
            0,
            ToInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
            dimensions.x,
            dimensions.y,
            0,
            ToOpenGLFormat(ToEquivalentCPUImageFormat(m_BufferType, m_Descriptor)),
            ToOpenGLDataType(ToEquivalentCPUDataType(m_BufferType, m_Descriptor)),
            nullptr
        );
        gl::TexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );
        gl::TexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );
        gl::TexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );
        gl::TexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );
        gl::TexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE
        );
        gl::BindTexture();
    }

    bool hasBeenRenderedTo() const
    {
        return m_MaybeOpenGLData->has_value();
    }

private:
    RenderTextureDescriptor m_Descriptor;
    RenderBufferType m_BufferType;
    DefaultConstructOnCopy<std::optional<RenderBufferOpenGLData>> m_MaybeOpenGLData;
};

osc::RenderBuffer::RenderBuffer(
    RenderTextureDescriptor const& descriptor_,
    RenderBufferType type_) :

    m_Impl{std::make_unique<Impl>(descriptor_, type_)}
{
}

osc::RenderBuffer::~RenderBuffer() noexcept = default;

class osc::RenderTexture::Impl final {
public:
    Impl() :
        Impl{glm::ivec2{1, 1}}
    {
    }

    Impl(glm::ivec2 dimensions) :
        Impl{RenderTextureDescriptor{dimensions}}
    {
    }

    Impl(RenderTextureDescriptor const& descriptor) :
        m_ColorBuffer{std::make_shared<RenderBuffer>(descriptor, RenderBufferType::Color)},
        m_DepthBuffer{std::make_shared<RenderBuffer>(descriptor, RenderBufferType::Depth)}
    {
    }

    glm::ivec2 getDimensions() const
    {
        return m_ColorBuffer->m_Impl->getDimensions();
    }

    void setDimensions(glm::ivec2 newDims)
    {
        if (newDims != getDimensions())
        {
            m_ColorBuffer->m_Impl->setDimensions(newDims);
            m_DepthBuffer->m_Impl->setDimensions(newDims);
        }
    }

    TextureDimension getDimension() const
    {
        return m_ColorBuffer->m_Impl->getDimension();
    }

    void setDimension(TextureDimension newDimension)
    {
        if (newDimension != getDimension())
        {
            m_ColorBuffer->m_Impl->setDimension(newDimension);
            m_DepthBuffer->m_Impl->setDimension(newDimension);
        }
    }

    RenderTextureFormat getColorFormat() const
    {
        return m_ColorBuffer->m_Impl->getColorFormat();
    }

    void setColorFormat(RenderTextureFormat newFormat)
    {
        if (newFormat != getColorFormat())
        {
            m_ColorBuffer->m_Impl->setColorFormat(newFormat);
            m_DepthBuffer->m_Impl->setColorFormat(newFormat);
        }
    }

    int32_t getAntialiasingLevel() const
    {
        return m_ColorBuffer->m_Impl->getAntialiasingLevel();
    }

    void setAntialiasingLevel(int32_t newLevel)
    {
        if (newLevel != getAntialiasingLevel())
        {
            m_ColorBuffer->m_Impl->setAntialiasingLevel(newLevel);
            m_DepthBuffer->m_Impl->setAntialiasingLevel(newLevel);
        }
    }

    DepthStencilFormat getDepthStencilFormat() const
    {
        return m_ColorBuffer->m_Impl->getDepthStencilFormat();
    }

    void setDepthStencilFormat(DepthStencilFormat newFormat)
    {
        if (newFormat != getDepthStencilFormat())
        {
            m_ColorBuffer->m_Impl->setDepthStencilFormat(newFormat);
            m_DepthBuffer->m_Impl->setDepthStencilFormat(newFormat);
        }
    }

    RenderTextureReadWrite getReadWrite() const
    {
        return m_ColorBuffer->m_Impl->getReadWrite();
    }

    void setReadWrite(RenderTextureReadWrite rw)
    {
        if (rw != getReadWrite())
        {
            m_ColorBuffer->m_Impl->setReadWrite(rw);
            m_DepthBuffer->m_Impl->setReadWrite(rw);
        }
    }

    void reformat(RenderTextureDescriptor const& d)
    {
        if (d != m_ColorBuffer->m_Impl->getDescriptor())
        {
            m_ColorBuffer->m_Impl->reformat(d);
            m_DepthBuffer->m_Impl->reformat(d);
        }
    }

    gl::RenderBuffer& getColorRBO()
    {
        return m_ColorBuffer->m_Impl->updRBO();
    }

    gl::Texture2D& getResolvedColorTexture()
    {
        return m_ColorBuffer->m_Impl->updResolvedTexture();
    }

    gl::RenderBuffer& getDepthStencilRBO()
    {
        return m_DepthBuffer->m_Impl->updRBO();
    }

    gl::Texture2D& getResolvedDepthTexture()
    {
        return m_DepthBuffer->m_Impl->updResolvedTexture();
    }

    void* getTextureHandleHACK() const
    {
        // yes, this is a shitshow of casting, const-casting, etc. - it's purely here until and osc-specific
        // ImGui backend is written
        return
            reinterpret_cast<void*>(
                static_cast<uintptr_t>(
                    const_cast<Impl&>(*this).getResolvedColorTexture().get()
                )
            );
    }

    bool hasBeenRenderedTo() const
    {
        return m_ColorBuffer->m_Impl->hasBeenRenderedTo();
    }

    std::shared_ptr<RenderBuffer> updColorBuffer()
    {
        return m_ColorBuffer;
    }

    std::shared_ptr<RenderBuffer> updDepthBuffer()
    {
        return m_DepthBuffer;
    }

private:
    friend class GraphicsBackend;

    std::shared_ptr<RenderBuffer> m_ColorBuffer;
    std::shared_ptr<RenderBuffer> m_DepthBuffer;
};

osc::RenderTexture::RenderTexture() :
    m_Impl{make_cow<Impl>()}
{
}

osc::RenderTexture::RenderTexture(glm::ivec2 dimensions) :
    m_Impl{make_cow<Impl>(dimensions)}
{
}

osc::RenderTexture::RenderTexture(RenderTextureDescriptor const& desc) :
    m_Impl{make_cow<Impl>(desc)}
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
    m_Impl.upd()->setDimensions(d);
}

osc::TextureDimension osc::RenderTexture::getDimension() const
{
    return m_Impl->getDimension();
}

void osc::RenderTexture::setDimension(TextureDimension newDimension)
{
    m_Impl.upd()->setDimension(newDimension);
}

osc::RenderTextureFormat osc::RenderTexture::getColorFormat() const
{
    return m_Impl->getColorFormat();
}

void osc::RenderTexture::setColorFormat(RenderTextureFormat format)
{
    m_Impl.upd()->setColorFormat(format);
}

int32_t osc::RenderTexture::getAntialiasingLevel() const
{
    return m_Impl->getAntialiasingLevel();
}

void osc::RenderTexture::setAntialiasingLevel(int32_t level)
{
    m_Impl.upd()->setAntialiasingLevel(level);
}

osc::DepthStencilFormat osc::RenderTexture::getDepthStencilFormat() const
{
    return m_Impl->getDepthStencilFormat();
}

void osc::RenderTexture::setDepthStencilFormat(DepthStencilFormat format)
{
    m_Impl.upd()->setDepthStencilFormat(format);
}

osc::RenderTextureReadWrite osc::RenderTexture::getReadWrite() const
{
    return m_Impl->getReadWrite();
}

void osc::RenderTexture::setReadWrite(RenderTextureReadWrite rw)
{
    m_Impl.upd()->setReadWrite(rw);
}

void osc::RenderTexture::reformat(RenderTextureDescriptor const& d)
{
    m_Impl.upd()->reformat(d);
}

std::shared_ptr<osc::RenderBuffer> osc::RenderTexture::updColorBuffer()
{
    return m_Impl.upd()->updColorBuffer();
}

std::shared_ptr<osc::RenderBuffer> osc::RenderTexture::updDepthBuffer()
{
    return m_Impl.upd()->updDepthBuffer();
}

void* osc::RenderTexture::getTextureHandleHACK() const
{
    return m_Impl->getTextureHandleHACK();
}

std::ostream& osc::operator<<(std::ostream& o, RenderTexture const&)
{
    return o << "RenderTexture()";
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

    size_t getPropertyCount() const
    {
        return m_Uniforms.size();
    }

    std::optional<ptrdiff_t> findPropertyIndex(std::string const& propertyName) const
    {
        auto const it = m_Uniforms.find(propertyName);

        if (it != m_Uniforms.end())
        {
            return static_cast<ptrdiff_t>(std::distance(m_Uniforms.begin(), it));
        }
        else
        {
            return std::nullopt;
        }
    }

    std::string const& getPropertyName(ptrdiff_t i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->first;
    }

    ShaderType getPropertyType(ptrdiff_t i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->second.shaderType;
    }

    // non-PIMPL APIs

    gl::Program& updProgram()
    {
        return m_Program;
    }

    ankerl::unordered_dense::map<std::string, ShaderElement> const& getUniforms() const
    {
        return m_Uniforms;
    }

    ankerl::unordered_dense::map<std::string, ShaderElement> const& getAttributes() const
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
            std::array<GLchar, maxNameLen> name{}; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveAttrib(
                m_Program.get() ,
                static_cast<GLuint>(i),
                maxNameLen,
                &length,
                &size,
                &type,
                name.data()
            );

            static_assert(sizeof(GLint) <= sizeof(int32_t));
            m_Attributes.try_emplace(
                NormalizeShaderElementName(name.data()),
                static_cast<int32_t>(glGetAttribLocation(m_Program.get(), name.data())),
                GLShaderTypeToShaderTypeInternal(type),
                static_cast<int32_t>(size)
            );
        }

        m_Uniforms.reserve(numUniforms);
        for (GLint i = 0; i < numUniforms; i++)
        {
            GLint size; // size of the variable
            GLenum type; // type of the variable (float, vec3 or mat4, etc)
            std::array<GLchar, maxNameLen> name{}; // variable name in GLSL
            GLsizei length; // name length
            glGetActiveUniform(
                m_Program.get(),
                static_cast<GLuint>(i),
                maxNameLen,
                &length,
                &size,
                &type,
                name.data()
            );

            static_assert(sizeof(GLint) <= sizeof(int32_t));
            m_Uniforms.try_emplace(
                NormalizeShaderElementName(name.data()),
                static_cast<int32_t>(glGetUniformLocation(m_Program.get(), name.data())),
                GLShaderTypeToShaderTypeInternal(type),
                static_cast<int32_t>(size)
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
    ankerl::unordered_dense::map<std::string, ShaderElement> m_Uniforms;
    ankerl::unordered_dense::map<std::string, ShaderElement> m_Attributes;
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
    return o << c_ShaderTypeInternalStrings.at(static_cast<size_t>(shaderType));
}

osc::Shader::Shader(CStringView vertexShader, CStringView fragmentShader) :
    m_Impl{make_cow<Impl>(vertexShader, fragmentShader)}
{
}

osc::Shader::Shader(CStringView vertexShader, CStringView geometryShader, CStringView fragmentShader) :
    m_Impl{make_cow<Impl>(vertexShader, geometryShader, fragmentShader)}
{
}

osc::Shader::Shader(Shader const&) = default;
osc::Shader::Shader(Shader&&) noexcept = default;
osc::Shader& osc::Shader::operator=(Shader const&) = default;
osc::Shader& osc::Shader::operator=(Shader&&) noexcept = default;
osc::Shader::~Shader() noexcept = default;

size_t osc::Shader::getPropertyCount() const
{
    return m_Impl->getPropertyCount();
}

std::optional<ptrdiff_t> osc::Shader::findPropertyIndex(std::string const& propertyName) const
{
    return m_Impl->findPropertyIndex(propertyName);
}

std::string const& osc::Shader::getPropertyName(ptrdiff_t propertyIndex) const
{
    return m_Impl->getPropertyName(propertyIndex);
}

osc::ShaderType osc::Shader::getPropertyType(ptrdiff_t propertyIndex) const
{
    return m_Impl->getPropertyType(propertyIndex);
}

std::ostream& osc::operator<<(std::ostream& o, Shader const& shader)
{
    o << "Shader(\n";
    {
        o << "    uniforms = [";

        std::string_view const delim = "\n        ";
        for (auto const& [name, data] : shader.m_Impl->getUniforms())
        {
            o << delim;
            PrintShaderElement(o, name, data);
        }

        o << "\n    ],\n";
    }

    {
        o << "    attributes = [";

        std::string_view const delim = "\n        ";
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

namespace
{
    GLenum ToGLDepthFunc(osc::DepthFunction f)
    {
        static_assert(static_cast<size_t>(osc::DepthFunction::TOTAL) == 2);
        switch (f)
        {
        case osc::DepthFunction::LessOrEqual:
            return GL_LEQUAL;
        case osc::DepthFunction::Less:
        default:
            return GL_LESS;
        }
    }
}

class osc::Material::Impl final {
public:
    Impl(Shader shader) : m_Shader{std::move(shader)}
    {
    }

    Shader const& getShader() const
    {
        return m_Shader;
    }

    std::optional<Color> getColor(std::string_view propertyName) const
    {
        return getValue<Color>(propertyName);
    }

    void setColor(std::string_view propertyName, Color const& color)
    {
        setValue(propertyName, color);
    }

    std::optional<nonstd::span<Color const>> getColorArray(std::string_view propertyName) const
    {
        return getValue<std::vector<osc::Color>, nonstd::span<osc::Color const>>(propertyName);
    }

    void setColorArray(std::string_view propertyName, nonstd::span<Color const> colors)
    {
        setValue<std::vector<osc::Color>>(propertyName, std::vector<osc::Color>(colors.begin(), colors.end()));
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(propertyName);
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(propertyName, value);
    }

    std::optional<nonstd::span<float const>> getFloatArray(std::string_view propertyName) const
    {
        return getValue<std::vector<float>, nonstd::span<float const>>(propertyName);
    }

    void setFloatArray(std::string_view propertyName, nonstd::span<float const> v)
    {
        setValue<std::vector<float>>(propertyName, std::vector<float>(v.begin(), v.end()));
    }

    std::optional<glm::vec2> getVec2(std::string_view propertyName) const
    {
        return getValue<glm::vec2>(propertyName);
    }

    void setVec2(std::string_view propertyName, glm::vec2 value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<glm::vec3>(propertyName);
    }

    void setVec3(std::string_view propertyName, glm::vec3 value)
    {
        setValue(propertyName, value);
    }

    std::optional<nonstd::span<glm::vec3 const>> getVec3Array(std::string_view propertyName) const
    {
        return getValue<std::vector<glm::vec3>, nonstd::span<glm::vec3 const>>(propertyName);
    }

    void setVec3Array(std::string_view propertyName, nonstd::span<glm::vec3 const> value)
    {
        setValue(propertyName, std::vector<glm::vec3>(value.begin(), value.end()));
    }

    std::optional<glm::vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<glm::vec4>(propertyName);
    }

    void setVec4(std::string_view propertyName, glm::vec4 value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<glm::mat3>(propertyName);
    }

    void setMat3(std::string_view propertyName, glm::mat3 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<glm::mat4>(propertyName);
    }

    void setMat4(std::string_view propertyName, glm::mat4 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<nonstd::span<glm::mat4 const>> getMat4Array(std::string_view propertyName) const
    {
        return getValue<std::vector<glm::mat4>, nonstd::span<glm::mat4 const>>(propertyName);
    }

    void setMat4Array(std::string_view propertyName, nonstd::span<glm::mat4 const> mats)
    {
        setValue(propertyName, std::vector<glm::mat4>(mats.begin(), mats.end()));
    }

    std::optional<int32_t> getInt(std::string_view propertyName) const
    {
        return getValue<int32_t>(propertyName);
    }

    void setInt(std::string_view propertyName, int32_t value)
    {
        setValue(propertyName, value);
    }

    std::optional<bool> getBool(std::string_view propertyName) const
    {
        return getValue<bool>(propertyName);
    }

    void setBool(std::string_view propertyName, bool value)
    {
        setValue(propertyName, value);
    }

    std::optional<Texture2D> getTexture(std::string_view propertyName) const
    {
        return getValue<Texture2D>(propertyName);
    }

    void setTexture(std::string_view propertyName, Texture2D t)
    {
        setValue(propertyName, std::move(t));
    }

    void clearTexture(std::string_view propertyName)
    {
        m_Values.erase(std::string{propertyName});
    }

    std::optional<RenderTexture> getRenderTexture(std::string_view propertyName) const
    {
        return getValue<RenderTexture>(propertyName);
    }

    void setRenderTexture(std::string_view propertyName, RenderTexture t)
    {
        setValue(propertyName, std::move(t));
    }

    void clearRenderTexture(std::string_view propertyName)
    {
        m_Values.erase(std::string{propertyName});
    }

    std::optional<Cubemap> getCubemap(std::string_view propertyName) const
    {
        return getValue<Cubemap>(propertyName);
    }

    void setCubemap(std::string_view propertyName, Cubemap cubemap)
    {
        setValue(propertyName, std::move(cubemap));
    }

    void clearCubemap(std::string_view propertyName)
    {
        m_Values.erase(std::string{propertyName});
    }

    bool getTransparent() const
    {
        return m_IsTransparent;
    }

    void setTransparent(bool v)
    {
        m_IsTransparent = v;
    }

    bool getDepthTested() const
    {
        return m_IsDepthTested;
    }

    void setDepthTested(bool v)
    {
        m_IsDepthTested = v;
    }

    DepthFunction getDepthFunction() const
    {
        return m_DepthFunction;
    }

    void setDepthFunction(DepthFunction f)
    {
        m_DepthFunction = f;
    }

    bool getWireframeMode() const
    {
        return m_IsWireframeMode;
    }

    void setWireframeMode(bool v)
    {
        m_IsWireframeMode = v;
    }

private:
    template<typename T, typename TConverted = T>
    std::optional<TConverted> getValue(std::string_view propertyName) const
    {
        auto const it = m_Values.find(std::string{propertyName});

        if (it == m_Values.end())
        {
            return std::nullopt;
        }

        if (!std::holds_alternative<T>(it->second))
        {
            return std::nullopt;
        }

        return TConverted{std::get<T>(it->second)};
    }

    template<typename T>
    void setValue(std::string_view propertyName, T&& v)
    {
        m_Values.insert_or_assign(std::string{propertyName}, std::forward<T>(v));
    }

    friend class GraphicsBackend;

    Shader m_Shader;
    ankerl::unordered_dense::map<std::string, MaterialValue> m_Values;
    bool m_IsTransparent = false;
    bool m_IsDepthTested = true;
    bool m_IsWireframeMode = false;
    DepthFunction m_DepthFunction = osc::DepthFunction::Default;
};

osc::Material::Material(Shader shader) :
    m_Impl{make_cow<Impl>(std::move(shader))}
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

std::optional<osc::Color> osc::Material::getColor(std::string_view propertyName) const
{
    return m_Impl->getColor(propertyName);
}

void osc::Material::setColor(std::string_view propertyName, Color const& color)
{
    m_Impl.upd()->setColor(propertyName, color);
}

std::optional<nonstd::span<osc::Color const>> osc::Material::getColorArray(std::string_view propertyName) const
{
    return m_Impl->getColorArray(propertyName);
}

void osc::Material::setColorArray(std::string_view propertyName, nonstd::span<osc::Color const> colors)
{
    m_Impl.upd()->setColorArray(propertyName, colors);
}

std::optional<float> osc::Material::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(propertyName);
}

void osc::Material::setFloat(std::string_view propertyName, float value)
{
    m_Impl.upd()->setFloat(propertyName, value);
}

std::optional<nonstd::span<float const>> osc::Material::getFloatArray(std::string_view propertyName) const
{
    return m_Impl->getFloatArray(propertyName);
}

void osc::Material::setFloatArray(std::string_view propertyName, nonstd::span<float const> vs)
{
    m_Impl.upd()->setFloatArray(propertyName, vs);
}

std::optional<glm::vec2> osc::Material::getVec2(std::string_view propertyName) const
{
    return m_Impl->getVec2(propertyName);
}

void osc::Material::setVec2(std::string_view propertyName, glm::vec2 value)
{
    m_Impl.upd()->setVec2(propertyName, value);
}

std::optional<nonstd::span<glm::vec3 const>> osc::Material::getVec3Array(std::string_view propertyName) const
{
    return m_Impl->getVec3Array(propertyName);
}

void osc::Material::setVec3Array(std::string_view propertyName, nonstd::span<glm::vec3 const> vs)
{
    m_Impl.upd()->setVec3Array(propertyName, vs);
}

std::optional<glm::vec3> osc::Material::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(propertyName);
}

void osc::Material::setVec3(std::string_view propertyName, glm::vec3 value)
{
    m_Impl.upd()->setVec3(propertyName, value);
}

std::optional<glm::vec4> osc::Material::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(propertyName);
}

void osc::Material::setVec4(std::string_view propertyName, glm::vec4 value)
{
    m_Impl.upd()->setVec4(propertyName, value);
}

std::optional<glm::mat3> osc::Material::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(propertyName);
}

void osc::Material::setMat3(std::string_view propertyName, glm::mat3 const& mat)
{
    m_Impl.upd()->setMat3(propertyName, mat);
}

std::optional<glm::mat4> osc::Material::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(propertyName);
}

void osc::Material::setMat4(std::string_view propertyName, glm::mat4 const& mat)
{
    m_Impl.upd()->setMat4(propertyName, mat);
}

std::optional<nonstd::span<glm::mat4 const>> osc::Material::getMat4Array(std::string_view propertyName) const
{
    return m_Impl->getMat4Array(propertyName);
}

void osc::Material::setMat4Array(std::string_view propertyName, nonstd::span<glm::mat4 const> mats)
{
    m_Impl.upd()->setMat4Array(propertyName, mats);
}

std::optional<int32_t> osc::Material::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(propertyName);
}

void osc::Material::setInt(std::string_view propertyName, int32_t value)
{
    m_Impl.upd()->setInt(propertyName, value);
}

std::optional<bool> osc::Material::getBool(std::string_view propertyName) const
{
    return m_Impl->getBool(propertyName);
}

void osc::Material::setBool(std::string_view propertyName, bool value)
{
    m_Impl.upd()->setBool(propertyName, value);
}

std::optional<osc::Texture2D> osc::Material::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(propertyName);
}

void osc::Material::setTexture(std::string_view propertyName, Texture2D t)
{
    m_Impl.upd()->setTexture(propertyName, std::move(t));
}

void osc::Material::clearTexture(std::string_view propertyName)
{
    m_Impl.upd()->clearTexture(propertyName);
}

std::optional<osc::RenderTexture> osc::Material::getRenderTexture(std::string_view propertyName) const
{
    return m_Impl->getRenderTexture(propertyName);
}

void osc::Material::setRenderTexture(std::string_view propertyName, RenderTexture t)
{
    m_Impl.upd()->setRenderTexture(propertyName, std::move(t));
}

void osc::Material::clearRenderTexture(std::string_view propertyName)
{
    m_Impl.upd()->clearRenderTexture(propertyName);
}

std::optional<osc::Cubemap> osc::Material::getCubemap(std::string_view propertyName) const
{
    return m_Impl->getCubemap(propertyName);
}

void osc::Material::setCubemap(std::string_view propertyName, Cubemap cubemap)
{
    m_Impl.upd()->setCubemap(propertyName, std::move(cubemap));
}

void osc::Material::clearCubemap(std::string_view propertyName)
{
    m_Impl.upd()->clearCubemap(propertyName);
}

bool osc::Material::getTransparent() const
{
    return m_Impl->getTransparent();
}

void osc::Material::setTransparent(bool v)
{
    m_Impl.upd()->setTransparent(v);
}

bool osc::Material::getDepthTested() const
{
    return m_Impl->getDepthTested();
}

void osc::Material::setDepthTested(bool v)
{
    m_Impl.upd()->setDepthTested(v);
}

osc::DepthFunction osc::Material::getDepthFunction() const
{
    return m_Impl->getDepthFunction();
}

void osc::Material::setDepthFunction(DepthFunction f)
{
    m_Impl.upd()->setDepthFunction(f);
}

bool osc::Material::getWireframeMode() const
{
    return m_Impl->getWireframeMode();
}

void osc::Material::setWireframeMode(bool v)
{
    m_Impl.upd()->setWireframeMode(v);
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

    std::optional<Color> getColor(std::string_view propertyName) const
    {
        return getValue<Color>(propertyName);
    }

    void setColor(std::string_view propertyName, Color const& color)
    {
        setValue(propertyName, color);
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(propertyName);
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<glm::vec3>(propertyName);
    }

    void setVec3(std::string_view propertyName, glm::vec3 value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<glm::vec4>(propertyName);
    }

    void setVec4(std::string_view propertyName, glm::vec4 value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<glm::mat3>(propertyName);
    }

    void setMat3(std::string_view propertyName, glm::mat3 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<glm::mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<glm::mat4>(propertyName);
    }

    void setMat4(std::string_view propertyName, glm::mat4 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<int32_t> getInt(std::string_view propertyName) const
    {
        return getValue<int32_t>(propertyName);
    }

    void setInt(std::string_view propertyName, int32_t value)
    {
        setValue(propertyName, value);
    }

    std::optional<bool> getBool(std::string_view propertyName) const
    {
        return getValue<bool>(propertyName);
    }

    void setBool(std::string_view propertyName, bool value)
    {
        setValue(propertyName, value);
    }

    std::optional<Texture2D> getTexture(std::string_view propertyName) const
    {
        return getValue<Texture2D>(propertyName);
    }

    void setTexture(std::string_view propertyName, Texture2D t)
    {
        setValue(propertyName, std::move(t));
    }

    bool operator==(Impl const& other) const
    {
        return m_Values == other.m_Values;
    }

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        auto const it = m_Values.find(std::string{propertyName});

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
        m_Values.insert_or_assign(std::string{propertyName}, std::forward<T>(v));
    }

    friend class GraphicsBackend;

    ankerl::unordered_dense::map<std::string, MaterialValue> m_Values;
};

osc::MaterialPropertyBlock::MaterialPropertyBlock()
{
    static CopyOnUpdPtr<Impl> const s_EmptyPropertyBlockImpl = make_cow<Impl>();
    m_Impl = s_EmptyPropertyBlockImpl;
}

osc::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock const&) = default;
osc::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept = default;
osc::MaterialPropertyBlock& osc::MaterialPropertyBlock::operator=(MaterialPropertyBlock const&) = default;
osc::MaterialPropertyBlock& osc::MaterialPropertyBlock::operator=(MaterialPropertyBlock&&) noexcept = default;
osc::MaterialPropertyBlock::~MaterialPropertyBlock() noexcept = default;

void osc::MaterialPropertyBlock::clear()
{
    m_Impl.upd()->clear();
}

bool osc::MaterialPropertyBlock::isEmpty() const
{
    return m_Impl->isEmpty();
}

std::optional<osc::Color> osc::MaterialPropertyBlock::getColor(std::string_view propertyName) const
{
    return m_Impl->getColor(propertyName);
}

void osc::MaterialPropertyBlock::setColor(std::string_view propertyName, Color const& color)
{
    m_Impl.upd()->setColor(propertyName, color);
}

std::optional<float> osc::MaterialPropertyBlock::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(propertyName);
}

void osc::MaterialPropertyBlock::setFloat(std::string_view propertyName, float value)
{
    m_Impl.upd()->setFloat(propertyName, value);
}

std::optional<glm::vec3> osc::MaterialPropertyBlock::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(propertyName);
}

void osc::MaterialPropertyBlock::setVec3(std::string_view propertyName, glm::vec3 value)
{
    m_Impl.upd()->setVec3(propertyName, value);
}

std::optional<glm::vec4> osc::MaterialPropertyBlock::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(propertyName);
}

void osc::MaterialPropertyBlock::setVec4(std::string_view propertyName, glm::vec4 value)
{
    m_Impl.upd()->setVec4(propertyName, value);
}

std::optional<glm::mat3> osc::MaterialPropertyBlock::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(propertyName);
}

void osc::MaterialPropertyBlock::setMat3(std::string_view propertyName, glm::mat3 const& value)
{
    m_Impl.upd()->setMat3(propertyName, value);
}

std::optional<glm::mat4> osc::MaterialPropertyBlock::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(propertyName);
}

void osc::MaterialPropertyBlock::setMat4(std::string_view propertyName, glm::mat4 const& value)
{
    m_Impl.upd()->setMat4(propertyName, value);
}

std::optional<int32_t> osc::MaterialPropertyBlock::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(propertyName);
}

void osc::MaterialPropertyBlock::setInt(std::string_view propertyName, int32_t value)
{
    m_Impl.upd()->setInt(propertyName, value);
}

std::optional<bool> osc::MaterialPropertyBlock::getBool(std::string_view propertyName) const
{
    return m_Impl->getBool(propertyName);
}

void osc::MaterialPropertyBlock::setBool(std::string_view propertyName, bool value)
{
    m_Impl.upd()->setBool(propertyName, value);
}

std::optional<osc::Texture2D> osc::MaterialPropertyBlock::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(propertyName);
}

void osc::MaterialPropertyBlock::setTexture(std::string_view propertyName, Texture2D t)
{
    m_Impl.upd()->setTexture(propertyName, std::move(t));
}

bool osc::operator==(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b) noexcept
{
    return a.m_Impl == b.m_Impl || *a.m_Impl == *b.m_Impl;
}

bool osc::operator!=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b) noexcept
{
    return a.m_Impl != b.m_Impl;
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
    auto constexpr c_MeshTopologyStrings = osc::to_array<osc::CStringView>(
    {
        "Triangles",
        "Lines",
    });
    static_assert(c_MeshTopologyStrings.size() == static_cast<size_t>(osc::MeshTopology::TOTAL));

    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
    static_assert(alignof(PackedIndex) == alignof(uint32_t));

    GLenum ToOpenGLTopology(osc::MeshTopology t)
    {
        switch (t)
        {
        case osc::MeshTopology::Triangles:
            return GL_TRIANGLES;
        case osc::MeshTopology::Lines:
            return GL_LINES;
        default:
            return GL_TRIANGLES;
        }
    }
}

class osc::Mesh::Impl final {
public:

    MeshTopology getTopology() const
    {
        return m_Topology;
    }

    void setTopology(MeshTopology newTopology)
    {
        m_Topology = newTopology;
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

    void transformVerts(std::function<void(nonstd::span<glm::vec3>)> const& f)
    {
        f(m_Vertices);

        recalculateBounds();
        m_Version->reset();
    }

    void transformVerts(Transform const& t)
    {
        for (glm::vec3& v : m_Vertices)
        {
            v = t * v;
        }
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

    void transformNormals(std::function<void(nonstd::span<glm::vec3>)> const& f)
    {
        f(m_Normals);
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

    nonstd::span<Rgba32 const> getColors() const
    {
        return m_Colors;
    }

    void setColors(nonstd::span<Rgba32 const> colors)
    {
        m_Colors.assign(colors.begin(), colors.end());

        m_Version.reset();
    }

    nonstd::span<glm::vec4 const> getTangents() const
    {
        return m_Tangents;
    }

    void setTangents(nonstd::span<glm::vec4 const> newTangents)
    {
        m_Tangents.assign(newTangents.begin(), newTangents.end());

        m_Version->reset();
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
            for (size_t i = 0; i < vs.size(); ++i)
            {
                (&m_IndicesData.front().u16.a)[i] = static_cast<uint16_t>(vs[i]);
            }
        }

        recalculateBounds();
        m_Version->reset();
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    BVH const& getBVH() const
    {
        return m_TriangleBVH;
    }

    void clear()
    {
        m_Version->reset();
        m_Topology = MeshTopology::Triangles;
        m_Vertices.clear();
        m_Normals.clear();
        m_TexCoords.clear();
        m_Colors.clear();
        m_Tangents.clear();
        m_IndicesAre32Bit = false;
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = {};
    }

    // non-PIMPL methods

    gl::VertexArray& updVertexArray()
    {
        if (!*m_MaybeGPUBuffers || (*m_MaybeGPUBuffers)->dataVersion != *m_Version)
        {
            uploadToGPU();
        }
        return (*m_MaybeGPUBuffers)->vao;
    }

    void draw()
    {
        gl::DrawElements(
            ToOpenGLTopology(m_Topology),
            static_cast<GLsizei>(m_NumIndices),
            m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            nullptr
        );
    }

    void drawInstanced(size_t n)
    {
        glDrawElementsInstanced(
            ToOpenGLTopology(m_Topology),
            static_cast<GLsizei>(m_NumIndices),
            m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            nullptr,
            static_cast<GLsizei>(n)
        );
    }

private:

    void recalculateBounds()
    {
        OSC_PERF("bounds/BVH computation");

        if (m_NumIndices == 0)
        {
            m_AABB = {};
        }
        else if (m_IndicesAre32Bit)
        {
            nonstd::span<uint32_t const> const indices(&m_IndicesData.front().u32, m_NumIndices);

            if (m_Topology == MeshTopology::Triangles)
            {
                m_TriangleBVH.buildFromIndexedTriangles(m_Vertices, indices);
                m_AABB = m_TriangleBVH.getRootAABB().value_or(AABB{});
            }
            else
            {
                m_TriangleBVH.clear();
                m_AABB = AABBFromIndexedVerts(m_Vertices, indices);
            }
        }
        else
        {
            nonstd::span<uint16_t const> const indices(&m_IndicesData.front().u16.a, m_NumIndices);

            if (m_Topology == MeshTopology::Triangles)
            {
                m_TriangleBVH.buildFromIndexedTriangles(m_Vertices, indices);
                m_AABB = m_TriangleBVH.getRootAABB().value_or(AABB{});
            }
            else
            {
                m_TriangleBVH.clear();
                m_AABB = AABBFromIndexedVerts(m_Vertices, indices);
            }
        }
    }

    void uploadToGPU()
    {
        bool const hasNormals = !m_Normals.empty();
        bool const hasTexCoords = !m_TexCoords.empty();
        bool const hasColors = !m_Colors.empty();
        bool const hasTangents = !m_Tangents.empty();

        // `sizeof(decltype(T)::value_type)` is used in this function
        //
        // check at compile-time that the resulting type is as-expected
        static_assert(sizeof(decltype(m_Vertices)::value_type) == 3*sizeof(float));
        static_assert(sizeof(decltype(m_Normals)::value_type) == 3*sizeof(float));
        static_assert(sizeof(decltype(m_TexCoords)::value_type) == 2*sizeof(float));
        static_assert(sizeof(decltype(m_Colors)::value_type) == 4*sizeof(uint8_t));
        static_assert(sizeof(decltype(m_Tangents)::value_type) == 4*sizeof(float));

        // calculate the number of bytes between each entry in the packed VBO
        GLsizei byteStride = sizeof(decltype(m_Vertices)::value_type);
        if (hasNormals)
        {
            byteStride += sizeof(decltype(m_Normals)::value_type);
        }
        if (hasTexCoords)
        {
            byteStride += sizeof(decltype(m_TexCoords)::value_type);
        }
        if (hasColors)
        {
            byteStride += sizeof(decltype(m_Colors)::value_type);
        }
        if (hasTangents)
        {
            byteStride += sizeof(decltype(m_Tangents)::value_type);
        }

        // check that the data stored in this mesh object is valid before indexing into it
        OSC_ASSERT_ALWAYS((!hasNormals || m_Normals.size() == m_Vertices.size()) && "number of normals != number of verts");
        OSC_ASSERT_ALWAYS((!hasTexCoords || m_TexCoords.size() == m_Vertices.size()) && "number of uvs != number of verts");
        OSC_ASSERT_ALWAYS((!hasColors || m_Colors.size() == m_Vertices.size()) && "number of colors != number of verts");
        OSC_ASSERT_ALWAYS((!hasTangents || m_Tangents.size() == m_Vertices.size()) && "number of tangents != number of verts");

        // allocate+pack mesh data into CPU-side vector
        std::vector<std::byte> data;
        data.reserve(byteStride * m_Vertices.size());
        for (size_t i = 0; i < m_Vertices.size(); ++i)
        {
            PushAsBytes(m_Vertices[i], data);
            if (hasNormals)
            {
                PushAsBytes(m_Normals[i], data);
            }
            if (hasTexCoords)
            {
                PushAsBytes(m_TexCoords[i], data);
            }
            if (hasColors)
            {
                PushAsBytes(m_Colors[i], data);
            }
            if (hasTangents)
            {
                PushAsBytes(m_Tangents[i], data);
            }
        }

        // check that the above packing procedure worked as expected
        OSC_ASSERT(data.size() == byteStride*m_Vertices.size() && "error packing mesh data into a CPU buffer: unexpected final size");

        // allocate GPU-side buffers (or re-use the last ones)
        if (!(*m_MaybeGPUBuffers))
        {
            *m_MaybeGPUBuffers = MeshOpenGLData{};
        }
        MeshOpenGLData& buffers = **m_MaybeGPUBuffers;

        // upload CPU-side vector data into the GPU-side buffer
        static_assert(alignof(float) == alignof(GLfloat), "OpenGL: glBufferData: clients must align data elements consistently with the requirements of the client platform");
        gl::BindBuffer(GL_ARRAY_BUFFER, buffers.arrayBuffer);
        gl::BufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(data.size()), data.data(), GL_STATIC_DRAW);

        // check that the indices stored in this mesh object are all valid
        //
        // this is to ensure nothing bizzare happens in the GPU at runtime (e.g. indexing
        // into invalid locations in the VBO - #460)
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

        // upload CPU-side element data into the GPU-side buffer
        size_t const eboNumBytes = m_NumIndices * (m_IndicesAre32Bit ? sizeof(uint32_t) : sizeof(uint16_t));
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indicesBuffer);
        gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(eboNumBytes), m_IndicesData.data(), GL_STATIC_DRAW);

        // configure mesh-level VAO
        gl::BindVertexArray(buffers.vao);
        gl::BindBuffer(GL_ARRAY_BUFFER, buffers.arrayBuffer);
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indicesBuffer);

        // activate relevant attributes based on buffer layout
        int64_t byteOffset = 0;

        if (true)  // mesh always has vertices
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, byteStride, reinterpret_cast<void*>(static_cast<uintptr_t>(byteOffset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_POSITION);
            byteOffset += sizeof(decltype(m_Vertices)::value_type);
        }
        if (hasNormals)
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, byteStride, reinterpret_cast<void*>(static_cast<uintptr_t>(byteOffset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_NORMAL);
            byteOffset += sizeof(decltype(m_Normals)::value_type);
        }
        if (hasTexCoords)
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_TEXCOORD01, 2, GL_FLOAT, GL_FALSE, byteStride, reinterpret_cast<void*>(static_cast<uintptr_t>(byteOffset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_TEXCOORD01);
            byteOffset += sizeof(decltype(m_TexCoords)::value_type);
        }
        if (hasColors)
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, byteStride, reinterpret_cast<void*>(static_cast<uintptr_t>(byteOffset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_COLOR);
            byteOffset += sizeof(decltype(m_Colors)::value_type);
        }
        if (hasTangents)
        {
            glVertexAttribPointer(SHADER_LOC_VERTEX_TANGENT, 3, GL_FLOAT, GL_FALSE, byteStride, reinterpret_cast<void*>(static_cast<uintptr_t>(byteOffset)));
            glEnableVertexAttribArray(SHADER_LOC_VERTEX_TANGENT);
            // unused: byteOffset += sizeof(decltype(m_Tangents)::value_type);
        }
        gl::BindVertexArray();  // VAO configuration complete

        buffers.dataVersion = *m_Version;
    }

    DefaultConstructOnCopy<UID> m_UID;
    DefaultConstructOnCopy<UID> m_Version;
    MeshTopology m_Topology = MeshTopology::Triangles;
    std::vector<glm::vec3> m_Vertices;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    std::vector<glm::vec4> m_Tangents;
    std::vector<Rgba32> m_Colors;

    bool m_IndicesAre32Bit = false;
    size_t m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;

    AABB m_AABB = {};
    BVH m_TriangleBVH;

    DefaultConstructOnCopy<std::optional<MeshOpenGLData>> m_MaybeGPUBuffers;
};

std::ostream& osc::operator<<(std::ostream& o, MeshTopology mt)
{
    return o << c_MeshTopologyStrings.at(static_cast<size_t>(mt));
}

osc::Mesh::Mesh() :
    m_Impl{make_cow<Impl>()}
{
}

osc::Mesh::Mesh(Mesh const&) = default;
osc::Mesh::Mesh(Mesh&&) noexcept = default;
osc::Mesh& osc::Mesh::operator=(Mesh const&) = default;
osc::Mesh& osc::Mesh::operator=(Mesh&&) noexcept = default;
osc::Mesh::~Mesh() noexcept = default;

osc::MeshTopology osc::Mesh::getTopology() const
{
    return m_Impl->getTopology();
}

void osc::Mesh::setTopology(MeshTopology topology)
{
    m_Impl.upd()->setTopology(topology);
}

nonstd::span<glm::vec3 const> osc::Mesh::getVerts() const
{
    return m_Impl->getVerts();
}

void osc::Mesh::setVerts(nonstd::span<glm::vec3 const> verts)
{
    m_Impl.upd()->setVerts(verts);
}

void osc::Mesh::transformVerts(std::function<void(nonstd::span<glm::vec3>)> const& f)
{
    m_Impl.upd()->transformVerts(f);
}

void osc::Mesh::transformVerts(Transform const& t)
{
    m_Impl.upd()->transformVerts(t);
}

nonstd::span<glm::vec3 const> osc::Mesh::getNormals() const
{
    return m_Impl->getNormals();
}

void osc::Mesh::setNormals(nonstd::span<glm::vec3 const> verts)
{
    m_Impl.upd()->setNormals(verts);
}

void osc::Mesh::transformNormals(std::function<void(nonstd::span<glm::vec3>)> const& f)
{
    m_Impl.upd()->transformNormals(f);
}

nonstd::span<glm::vec2 const> osc::Mesh::getTexCoords() const
{
    return m_Impl->getTexCoords();
}

void osc::Mesh::setTexCoords(nonstd::span<glm::vec2 const> coords)
{
    m_Impl.upd()->setTexCoords(coords);
}

nonstd::span<osc::Rgba32 const> osc::Mesh::getColors() const
{
    return m_Impl->getColors();
}

void osc::Mesh::setColors(nonstd::span<osc::Rgba32 const> colors)
{
    m_Impl.upd()->setColors(colors);
}

nonstd::span<glm::vec4 const> osc::Mesh::getTangents() const
{
    return m_Impl->getTangents();
}

void osc::Mesh::setTangents(nonstd::span<glm::vec4 const> newTangents)
{
    m_Impl.upd()->setTangents(newTangents);
}

osc::MeshIndicesView osc::Mesh::getIndices() const
{
    return m_Impl->getIndices();
}

void osc::Mesh::setIndices(MeshIndicesView indices)
{
    m_Impl.upd()->setIndices(indices);
}

void osc::Mesh::setIndices(nonstd::span<uint16_t const> indices)
{
    m_Impl.upd()->setIndices(indices);
}

void osc::Mesh::setIndices(nonstd::span<uint32_t const> indices)
{
    m_Impl.upd()->setIndices(indices);
}

osc::AABB const& osc::Mesh::getBounds() const
{
    return m_Impl->getBounds();
}

osc::BVH const& osc::Mesh::getBVH() const
{
    return m_Impl->getBVH();
}

void osc::Mesh::clear()
{
    m_Impl.upd()->clear();
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
    auto constexpr c_CameraProjectionStrings = osc::to_array<osc::CStringView>(
    {
        "Perspective",
        "Orthographic",
    });
    static_assert(c_CameraProjectionStrings.size() == static_cast<size_t>(osc::CameraProjection::TOTAL));
}

class osc::Camera::Impl final {
public:

    void reset()
    {
        Impl newImpl;
        std::swap(*this, newImpl);
        m_RenderQueue = std::move(newImpl.m_RenderQueue);
    }

    Color getBackgroundColor() const
    {
        return m_BackgroundColor;
    }

    void setBackgroundColor(Color const& color)
    {
        m_BackgroundColor = color;
    }

    CameraProjection getCameraProjection() const
    {
        return m_CameraProjection;
    }

    void setCameraProjection(CameraProjection projection)
    {
        m_CameraProjection = projection;
    }

    float getOrthographicSize() const
    {
        return m_OrthographicSize;
    }

    void setOrthographicSize(float size)
    {
        m_OrthographicSize = size;
    }

    float getCameraFOV() const
    {
        return m_PerspectiveFov;
    }

    void setCameraFOV(float size)
    {
        m_PerspectiveFov = size;
    }

    float getNearClippingPlane() const
    {
        return m_NearClippingPlane;
    }

    void setNearClippingPlane(float distance)
    {
        m_NearClippingPlane = distance;
    }

    float getFarClippingPlane() const
    {
        return m_FarClippingPlane;
    }

    void setFarClippingPlane(float distance)
    {
        m_FarClippingPlane = distance;
    }

    CameraClearFlags getClearFlags() const
    {
        return m_ClearFlags;
    }

    void setClearFlags(CameraClearFlags flags)
    {
        m_ClearFlags = flags;
    }

    std::optional<Rect> getPixelRect() const
    {
        return m_MaybeScreenPixelRect;
    }

    void setPixelRect(std::optional<Rect> maybePixelRect)
    {
        m_MaybeScreenPixelRect = maybePixelRect;
    }

    std::optional<Rect> getScissorRect() const
    {
        return m_MaybeScissorRect;
    }

    void setScissorRect(std::optional<Rect> maybeScissorRect)
    {
        m_MaybeScissorRect = maybeScissorRect;
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

    std::optional<glm::mat4> getViewMatrixOverride() const
    {
        return m_MaybeViewMatrixOverride;
    }

    void setViewMatrixOverride(std::optional<glm::mat4> maybeViewMatrixOverride)
    {
        m_MaybeViewMatrixOverride = maybeViewMatrixOverride;
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const
    {
        if (m_MaybeProjectionMatrixOverride)
        {
            return *m_MaybeProjectionMatrixOverride;
        }
        else if (m_CameraProjection == CameraProjection::Perspective)
        {
            return glm::perspective(m_PerspectiveFov, aspectRatio, m_NearClippingPlane, m_FarClippingPlane);
        }
        else
        {
            float const height = m_OrthographicSize;
            float const width = height * aspectRatio;

            float const right = 0.5f * width;
            float const left = -right;
            float const top = 0.5f * height;
            float const bottom = -top;

            return glm::ortho(left, right, bottom, top, m_NearClippingPlane, m_FarClippingPlane);
        }
    }

    std::optional<glm::mat4> getProjectionMatrixOverride() const
    {
        return m_MaybeProjectionMatrixOverride;
    }

    void setProjectionMatrixOverride(std::optional<glm::mat4> maybeProjectionMatrixOverride)
    {
        m_MaybeProjectionMatrixOverride = maybeProjectionMatrixOverride;
    }

    glm::mat4 getViewProjectionMatrix(float aspectRatio) const
    {
        return getProjectionMatrix(aspectRatio) * getViewMatrix();
    }

    glm::mat4 getInverseViewProjectionMatrix(float aspectRatio) const
    {
        return glm::inverse(getViewProjectionMatrix(aspectRatio));
    }

    void renderToScreen()
    {
        GraphicsBackend::RenderScene(*this);
    }

    void renderTo(RenderTexture& renderTexture)
    {
        static_assert(static_cast<size_t>(CameraClearFlags::TOTAL) == 3);
        static_assert(static_cast<size_t>(RenderTextureReadWrite::TOTAL) == 2);

        RenderTarget renderTargetThatWritesToRenderTexture
        {
            {
                RenderTargetColorAttachment
                {
                    // attach to render texture's color buffer
                    renderTexture.updColorBuffer(),

                    // load the color buffer based on this camera's clear flags
                    getClearFlags() == CameraClearFlags::SolidColor ?
                        RenderBufferLoadAction::Clear :
                        RenderBufferLoadAction::Load,

                    RenderBufferStoreAction::Resolve,

                    // ensure clear color matches colorspace of render texture
                    renderTexture.getReadWrite() == RenderTextureReadWrite::sRGB ?
                        ToLinear(getBackgroundColor()) :
                        getBackgroundColor(),
                },
            },
            RenderTargetDepthAttachment
            {
                // attach to the render texture's depth buffer
                renderTexture.updDepthBuffer(),

                // load the depth buffer based on this camera's clear flags
                //
                // TODO/BUG/HACK: it doesn't look like the flags in CameraClearFlags are
                // combine-able, so clear it using the solid color flag also
                (getClearFlags() == CameraClearFlags::SolidColor || getClearFlags() == CameraClearFlags::Depth) ?
                    RenderBufferLoadAction::Clear :
                    RenderBufferLoadAction::Load,

                RenderBufferStoreAction::DontCare,
            },
        };

        renderTo(renderTargetThatWritesToRenderTexture);
    }

    void renderTo(RenderTarget& renderTarget)
    {
        GraphicsBackend::RenderScene(*this, &renderTarget);
    }

    bool operator==(Impl const& other) const noexcept
    {
        return
            m_BackgroundColor == other.m_BackgroundColor &&
            m_CameraProjection == other.m_CameraProjection &&
            m_OrthographicSize == other.m_OrthographicSize &&
            m_PerspectiveFov == other.m_PerspectiveFov &&
            m_NearClippingPlane == other.m_NearClippingPlane &&
            m_FarClippingPlane == other.m_FarClippingPlane &&
            m_ClearFlags == other.m_ClearFlags &&
            m_MaybeScreenPixelRect == other.m_MaybeScreenPixelRect &&
            m_MaybeScissorRect == other.m_MaybeScissorRect &&
            m_Position == other.m_Position &&
            m_Rotation == other.m_Rotation &&
            m_MaybeViewMatrixOverride == other.m_MaybeViewMatrixOverride &&
            m_MaybeProjectionMatrixOverride == other.m_MaybeProjectionMatrixOverride &&
            m_RenderQueue == other.m_RenderQueue;
    }

private:

    friend class GraphicsBackend;

    Color m_BackgroundColor = Color::clear();
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
    return o << c_CameraProjectionStrings.at(static_cast<size_t>(cp));
}

osc::Camera::Camera() :
    m_Impl{make_cow<Impl>()}
{
}

osc::Camera::Camera(Camera const&) = default;
osc::Camera::Camera(Camera&&) noexcept = default;
osc::Camera& osc::Camera::operator=(Camera const&) = default;
osc::Camera& osc::Camera::operator=(Camera&&) noexcept = default;
osc::Camera::~Camera() noexcept = default;

void osc::Camera::reset()
{
    m_Impl.upd()->reset();
}

osc::Color osc::Camera::getBackgroundColor() const
{
    return m_Impl->getBackgroundColor();
}

void osc::Camera::setBackgroundColor(Color const& color)
{
    m_Impl.upd()->setBackgroundColor(color);
}

osc::CameraProjection osc::Camera::getCameraProjection() const
{
    return m_Impl->getCameraProjection();
}

void osc::Camera::setCameraProjection(CameraProjection projection)
{
    m_Impl.upd()->setCameraProjection(projection);
}

float osc::Camera::getOrthographicSize() const
{
    return m_Impl->getOrthographicSize();
}

void osc::Camera::setOrthographicSize(float sz)
{
    m_Impl.upd()->setOrthographicSize(sz);
}

float osc::Camera::getCameraFOV() const
{
    return m_Impl->getCameraFOV();
}

void osc::Camera::setCameraFOV(float fov)
{
    m_Impl.upd()->setCameraFOV(fov);
}

float osc::Camera::getNearClippingPlane() const
{
    return m_Impl->getNearClippingPlane();
}

void osc::Camera::setNearClippingPlane(float d)
{
    m_Impl.upd()->setNearClippingPlane(d);
}

float osc::Camera::getFarClippingPlane() const
{
    return m_Impl->getFarClippingPlane();
}

void osc::Camera::setFarClippingPlane(float d)
{
    m_Impl.upd()->setFarClippingPlane(d);
}

osc::CameraClearFlags osc::Camera::getClearFlags() const
{
    return m_Impl->getClearFlags();
}

void osc::Camera::setClearFlags(CameraClearFlags flags)
{
    m_Impl.upd()->setClearFlags(flags);
}

std::optional<osc::Rect> osc::Camera::getPixelRect() const
{
    return m_Impl.get()->getPixelRect();
}

void osc::Camera::setPixelRect(std::optional<Rect> maybePixelRect)
{
    m_Impl.upd()->setPixelRect(maybePixelRect);
}

std::optional<osc::Rect> osc::Camera::getScissorRect() const
{
    return m_Impl->getScissorRect();
}

void osc::Camera::setScissorRect(std::optional<Rect> maybeScissorRect)
{
    m_Impl.upd()->setScissorRect(maybeScissorRect);
}

glm::vec3 osc::Camera::getPosition() const
{
    return m_Impl->getPosition();
}

void osc::Camera::setPosition(glm::vec3 const& p)
{
    m_Impl.upd()->setPosition(p);
}

glm::quat osc::Camera::getRotation() const
{
    return m_Impl->getRotation();
}

void osc::Camera::setRotation(glm::quat const& rotation)
{
    m_Impl.upd()->setRotation(rotation);
}

glm::vec3 osc::Camera::getDirection() const
{
    return m_Impl->getDirection();
}

void osc::Camera::setDirection(glm::vec3 const& d)
{
    m_Impl.upd()->setDirection(d);
}

glm::vec3 osc::Camera::getUpwardsDirection() const
{
    return m_Impl->getUpwardsDirection();
}

glm::mat4 osc::Camera::getViewMatrix() const
{
    return m_Impl->getViewMatrix();
}

std::optional<glm::mat4> osc::Camera::getViewMatrixOverride() const
{
    return m_Impl->getViewMatrixOverride();
}

void osc::Camera::setViewMatrixOverride(std::optional<glm::mat4> maybeViewMatrixOverride)
{
    m_Impl.upd()->setViewMatrixOverride(maybeViewMatrixOverride);
}

glm::mat4 osc::Camera::getProjectionMatrix(float aspectRatio) const
{
    return m_Impl->getProjectionMatrix(aspectRatio);
}

std::optional<glm::mat4> osc::Camera::getProjectionMatrixOverride() const
{
    return m_Impl->getProjectionMatrixOverride();
}

void osc::Camera::setProjectionMatrixOverride(std::optional<glm::mat4> maybeProjectionMatrixOverride)
{
    m_Impl.upd()->setProjectionMatrixOverride(maybeProjectionMatrixOverride);
}

glm::mat4 osc::Camera::getViewProjectionMatrix(float aspectRatio) const
{
    return m_Impl->getViewProjectionMatrix(aspectRatio);
}

glm::mat4 osc::Camera::getInverseViewProjectionMatrix(float aspectRatio) const
{
    return m_Impl->getInverseViewProjectionMatrix(aspectRatio);
}

void osc::Camera::renderToScreen()
{
    m_Impl.upd()->renderToScreen();
}

void osc::Camera::renderTo(RenderTexture& renderTexture)
{
    m_Impl.upd()->renderTo(renderTexture);
}

void osc::Camera::renderTo(RenderTarget& renderTarget)
{
    m_Impl.upd()->renderTo(renderTarget);
}

std::ostream& osc::operator<<(std::ostream& o, Camera const& camera)
{
    return o << "Camera(position = " << camera.getPosition() << ", direction = " << camera.getDirection() << ", projection = " << camera.getCameraProjection() << ')';
}

bool osc::operator==(Camera const& a, Camera const& b) noexcept
{
    return a.m_Impl == b.m_Impl || *a.m_Impl == *b.m_Impl;
}

bool osc::operator!=(Camera const& a, Camera const& b) noexcept
{
    return !(a == b);
}


/////////////////////////////
//
// graphics context
//
/////////////////////////////

namespace
{
    // create an OpenGL context for an application window
    sdl::GLContext CreateOpenGLContext(SDL_Window& window)
    {
        osc::log::info("initializing OpenGL context");

        sdl::GLContext ctx = sdl::GL_CreateContext(&window);

        // enable the context
        if (SDL_GL_MakeCurrent(&window, ctx.get()) != 0)
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

        // shader calculations are done in linear space, but writes to framebuffers
        // should respect whether the framebuffer is using an sRGB internal format
        glEnable(GL_FRAMEBUFFER_SRGB);

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
    int32_t GetOpenGLMaxMSXAASamples(sdl::GLContext const&)
    {
        GLint v = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &v);

        // OpenGL spec: "the value must be at least 4"
        // see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml
        if (v < 4)
        {
            static bool const s_ShowWarningOnce = [&]()
            {
                osc::log::warn("the current OpenGl backend only supports %i samples. Technically, this is invalid (4 *should* be the minimum)", v);
                return true;
            }();
            (void)s_ShowWarningOnce;
        }
        OSC_ASSERT_ALWAYS(v < 1<<16 && "number of samples is greater than the maximum supported by the application");

        return static_cast<int32_t>(v);
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
        GLuint id,
        GLenum severity,
        GLsizei,
        const GLchar* message,
        void const*)
    {
        osc::log::level::LevelEnum const lvl = OpenGLDebugSevToLogLvl(severity);
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
            osc::log::info("OpenGL debug mode appears to already be enabled: skipping enabling it");
            return;
        }

        GLint flags;
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
            osc::log::info("OpenGL debug mode appears to already be disabled: skipping disabling it");
            return;
        }

        GLint flags{};
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
    Impl(SDL_Window& window) : m_GLContext{CreateOpenGLContext(window)}
    {
        m_QuadMaterial.setDepthTested(false);  // it's for fullscreen rendering
    }

    int32_t getMaxMSXAASamples() const
    {
        return m_MaxMSXAASamples;
    }

    bool isVsyncEnabled() const
    {
        return m_VSyncEnabled;
    }

    void enableVsync()
    {
        if (SDL_GL_SetSwapInterval(-1) == 0)
        {
            // adaptive vsync enabled
        }
        else if (SDL_GL_SetSwapInterval(1) == 0)
        {
            // normal vsync enabled
        }

        // always read the vsync state back from SDL
        m_VSyncEnabled = SDL_GL_GetSwapInterval() != 0;
    }

    void disableVsync()
    {
        SDL_GL_SetSwapInterval(0);
        m_VSyncEnabled = SDL_GL_GetSwapInterval() != 0;
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
        m_DebugModeEnabled = IsOpenGLInDebugMode();
    }
    void disableDebugMode()
    {
        if (!IsOpenGLInDebugMode())
        {
            return;  // already not in debug mode
        }

        log::info("disabling debug mode");
        DisableOpenGLDebugMessages();
        m_DebugModeEnabled = IsOpenGLInDebugMode();
    }

    void clearProgram()
    {
        gl::UseProgram();
    }

    void clearScreen(Color const& color)
    {
        // clear color is in sRGB, but the framebuffer is sRGB-corrected (GL_FRAMEBUFFER_SRGB)
        // and assumes that the given colors are in linear space
        Color const linearColor = ToLinear(color);

        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, gl::windowFbo);
        gl::ClearColor(linearColor.r, linearColor.g, linearColor.b, linearColor.a);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void* updRawGLContextHandle()
    {
        return m_GLContext.get();
    }

    std::future<Image> requestScreenshot()
    {
        return m_ActiveScreenshotRequests.emplace_back().get_future();
    }

    void doSwapBuffers(SDL_Window& window)
    {
        // ensure window FBO is bound (see: SDL_GL_SwapWindow's note about MacOS requiring 0 is bound)
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);

        // flush outstanding screenshot requests
        if (!m_ActiveScreenshotRequests.empty())
        {
            // copy GPU-side window framebuffer into CPU-side `osc::Image` object
            glm::ivec2 const dims = osc::App::get().idims();

            std::vector<uint8_t> pixels(static_cast<size_t>(4*dims.x*dims.y));
            OSC_ASSERT(reinterpret_cast<uintptr_t>(pixels.data()) % 4 == 0 && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
            gl::PixelStorei(GL_PACK_ALIGNMENT, 4);
            glReadPixels(
                0,
                0,
                dims.x,
                dims.y,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels.data()
            );

            Image screenshot{dims, pixels, 4, ColorSpace::sRGB};

            // copy image to requests [0..n-2]
            for (ptrdiff_t i = 0, len = osc::ssize(m_ActiveScreenshotRequests)-1; i < len; ++i)
            {
                m_ActiveScreenshotRequests[i].set_value(screenshot);
            }
            // move image to request `n-1`
            m_ActiveScreenshotRequests.back().set_value(std::move(screenshot));
            m_ActiveScreenshotRequests.clear();
        }

        SDL_GL_SwapWindow(&window);
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
    int32_t m_MaxMSXAASamples = GetOpenGLMaxMSXAASamples(m_GLContext);

    bool m_VSyncEnabled = SDL_GL_GetSwapInterval() != 0;

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
            c_QuadVertexShaderSrc,
            c_QuadFragmentShaderSrc,
        }
    };

    // a generic quad mesh: two triangles covering NDC @ Z=0
    Mesh m_QuadMesh = GenTexturedQuad();

    // storage for instance data
    std::vector<float> m_InstanceCPUBuffer;
    gl::ArrayBuffer<float, GL_STREAM_DRAW> m_InstanceGPUBuffer;
};

static std::unique_ptr<osc::GraphicsContext::Impl> g_GraphicsContextImpl = nullptr;

osc::GraphicsContext::GraphicsContext(SDL_Window& window)
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

int32_t osc::GraphicsContext::getMaxMSXAASamples() const
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

void osc::GraphicsContext::clearScreen(Color const& color)
{
    g_GraphicsContextImpl->clearScreen(color);
}

void* osc::GraphicsContext::updRawGLContextHandle()
{
    return g_GraphicsContextImpl->updRawGLContextHandle();
}

void osc::GraphicsContext::doSwapBuffers(SDL_Window& window)
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
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, maybeMaterialPropertyBlock);
}

void osc::Graphics::DrawMesh(
    Mesh const& mesh,
    glm::mat4 const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, maybeMaterialPropertyBlock);
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
    GraphicsBackend::BlitToScreen(t, rect, flags);
}

void osc::Graphics::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    Material const& material,
    BlitFlags flags)
{
    GraphicsBackend::BlitToScreen(t, rect, material, flags);
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
            if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == osc::ShaderType::Mat4)
            {
                byteStride += sizeof(float) * 16;
            }
        }
        if (shaderImpl.m_MaybeInstancedNormalMatAttr)
        {
            if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == osc::ShaderType::Mat4)
            {
                byteStride += sizeof(float) * 16;
            }
            else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == osc::ShaderType::Mat3)
            {
                byteStride += sizeof(float) * 9;
            }
        }

        // write the instance data into a CPU-side buffer

        OSC_PERF("GraphicsBackend::UploadInstanceData");
        std::vector<float>& buf = g_GraphicsContextImpl->m_InstanceCPUBuffer;
        buf.resize(els.size() * (byteStride/sizeof(float)));

        size_t floatOffset = 0;
        for (RenderObject const& el : els)
        {
            if (shaderImpl.m_MaybeInstancedModelMatAttr)
            {
                if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == osc::ShaderType::Mat4)
                {
                    static_assert(alignof(glm::mat4) == alignof(float) && sizeof(glm::mat4) == 16 * sizeof(float));
                    reinterpret_cast<glm::mat4&>(buf[floatOffset]) = ModelMatrix(el);
                    floatOffset += 16;
                }
            }
            if (shaderImpl.m_MaybeInstancedNormalMatAttr)
            {
                if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == osc::ShaderType::Mat4)
                {
                    static_assert(alignof(glm::mat4) == alignof(float) && sizeof(glm::mat4) == 16 * sizeof(float));
                    reinterpret_cast<glm::mat4&>(buf[floatOffset]) = NormalMatrix4(el);
                    floatOffset += 16;
                }
                else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == osc::ShaderType::Mat3)
                {
                    static_assert(alignof(glm::mat3) == alignof(float) && sizeof(glm::mat3) == 9 * sizeof(float));
                    reinterpret_cast<glm::mat3&>(buf[floatOffset]) = NormalMatrix(el);
                    floatOffset += 9;
                }
            }
        }
        OSC_ASSERT_ALWAYS(sizeof(float)*floatOffset == els.size() * byteStride);

        auto& vbo = maybeInstancingState.emplace(g_GraphicsContextImpl->m_InstanceGPUBuffer, byteStride).buf;
        vbo.assign(nonstd::span<float const>{buf.data(), floatOffset});
    }
    return maybeInstancingState;
}

// helper: binds to instanced attributes (per-drawcall)
void osc::GraphicsBackend::BindToInstancedAttributes(
        Shader::Impl const& shaderImpl,
        InstancingState& ins)
{
    gl::BindBuffer(ins.buf);

    size_t byteOffset = 0;
    if (shaderImpl.m_MaybeInstancedModelMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == ShaderType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.stride, ins.baseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            byteOffset += sizeof(float) * 16;
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.stride, ins.baseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            // unused: byteOffset += sizeof(float) * 16;
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderType::Mat3)
        {
            gl::AttributeMat3 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.stride, ins.baseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            // unused: byteOffset += sizeof(float) * 9;
        }
    }
}

// helper: unbinds from instanced attributes (per-drawcall)
void osc::GraphicsBackend::UnbindFromInstancedAttributes(
    Shader::Impl const& shaderImpl,
    InstancingState&)
{
    if (shaderImpl.m_MaybeInstancedModelMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == ShaderType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderType::Mat3)
        {
            gl::AttributeMat3 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
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
    Shader::Impl const& shaderImpl = *els.front().material.m_Impl->m_Shader.m_Impl;

    gl::BindVertexArray(meshImpl.updVertexArray());

    // if the shader requires per-instance uniforms, then we *have* to render one
    // instance at a time
    if (shaderImpl.m_MaybeModelMatUniform || shaderImpl.m_MaybeNormalMatUniform)
    {
        for (RenderObject const& el : els)
        {
            // try binding to uModel (standard)
            if (shaderImpl.m_MaybeModelMatUniform)
            {
                if (shaderImpl.m_MaybeModelMatUniform->shaderType == ShaderType::Mat4)
                {
                    gl::UniformMat4 u{shaderImpl.m_MaybeModelMatUniform->location};
                    gl::Uniform(u, ModelMatrix(el));
                }
            }

            // try binding to uNormalMat (standard)
            if (shaderImpl.m_MaybeNormalMatUniform)
            {
                if (shaderImpl.m_MaybeNormalMatUniform->shaderType == osc::ShaderType::Mat3)
                {
                    gl::UniformMat3 u{shaderImpl.m_MaybeNormalMatUniform->location};
                    gl::Uniform(u, NormalMatrix(el));
                }
                else if (shaderImpl.m_MaybeNormalMatUniform->shaderType == osc::ShaderType::Mat4)
                {
                    gl::UniformMat4 u{shaderImpl.m_MaybeNormalMatUniform->location};
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
                ins->baseOffset += 1 * ins->stride;
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
            ins->baseOffset += els.size() * ins->stride;
        }
    }

    gl::BindVertexArray();
}

// helper: draw a batch of render objects that have the same material and material block
void osc::GraphicsBackend::HandleBatchWithSameMaterialPropertyBlock(
    nonstd::span<RenderObject const> els,
    int32_t& textureSlot,
    std::optional<InstancingState>& ins)
{
    OSC_PERF("GraphicsBackend::HandleBatchWithSameMaterialPropertyBlock");

    Material::Impl const& matImpl = *els.front().material.m_Impl;
    Shader::Impl const& shaderImpl = *matImpl.m_Shader.m_Impl;
    ankerl::unordered_dense::map<std::string, ShaderElement> const& uniforms = shaderImpl.getUniforms();

    // bind property block variables (if applicable)
    if (els.front().maybePropBlock)
    {
        for (auto const& [name, value] : els.front().maybePropBlock->m_Impl->m_Values)
        {
            auto const it = uniforms.find(name);
            if (it != uniforms.end())
            {
                TryBindMaterialValueToShaderElement(it->second, value, textureSlot);
            }
        }
    }

    // batch by mesh
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMesh{&batchIt->mesh});
        HandleBatchWithSameMesh({batchIt, batchEnd}, ins);
        batchIt = batchEnd;
    }
}


void osc::GraphicsBackend::TryBindMaterialValueToShaderElement(
    ShaderElement const& se,
    MaterialValue const& v,
    int32_t& textureSlot)
{
    if (GetShaderType(v) != se.shaderType)
    {
        return;  // mismatched types
    }

    switch (v.index())
    {
    case VariantIndex<MaterialValue, osc::Color>():
    {
        // colors are converted from sRGB to linear when passed to
        // the shader

        glm::vec4 const linearColor = osc::ToLinear(std::get<osc::Color>(v));
        gl::UniformVec4 u{se.location};
        gl::Uniform(u, linearColor);
        break;
    }
    case VariantIndex<MaterialValue, std::vector<osc::Color>>():
    {
        auto const& colors = std::get<std::vector<osc::Color>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(colors.size()));
        for (int32_t i = 0; i < numToAssign; ++i)
        {
            glm::vec4 const linearColor = osc::ToLinear(colors[i]);
            gl::UniformVec4 u{se.location + i};
            gl::Uniform(u, linearColor);
        }
        break;
    }
    case VariantIndex<MaterialValue, float>():
    {
        gl::UniformFloat u{se.location};
        gl::Uniform(u, std::get<float>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<float>>():
    {
        auto const& vals = std::get<std::vector<float>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(vals.size()));
        for (int32_t i = 0; i < numToAssign; ++i)
        {
            gl::UniformFloat u{se.location + i};
            gl::Uniform(u, vals[i]);
        }
        break;
    }
    case VariantIndex<MaterialValue, glm::vec2>():
    {
        gl::UniformVec2 u{se.location};
        gl::Uniform(u, std::get<glm::vec2>(v));
        break;
    }
    case VariantIndex<MaterialValue, glm::vec3>():
    {
        gl::UniformVec3 u{se.location};
        gl::Uniform(u, std::get<glm::vec3>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<glm::vec3>>():
    {
        auto const& vals = std::get<std::vector<glm::vec3>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(vals.size()));
        for (int32_t i = 0; i < numToAssign; ++i)
        {
            gl::UniformVec3 u{se.location + i};
            gl::Uniform(u, vals[i]);
        }
        break;
    }
    case VariantIndex<MaterialValue, glm::vec4>():
    {
        gl::UniformVec4 u{se.location};
        gl::Uniform(u, std::get<glm::vec4>(v));
        break;
    }
    case VariantIndex<MaterialValue, glm::mat3>():
    {
        gl::UniformMat3 u{se.location};
        gl::Uniform(u, std::get<glm::mat3>(v));
        break;
    }
    case VariantIndex<MaterialValue, glm::mat4>():
    {
        gl::UniformMat4 u{se.location};
        gl::Uniform(u, std::get<glm::mat4>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<glm::mat4>>():
    {
        auto const& vals = std::get<std::vector<glm::mat4>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(vals.size()));
        for (int32_t i = 0; i < numToAssign; ++i)
        {
            gl::UniformMat4 u{se.location + i};
            gl::Uniform(u, vals[i]);
        }
        break;
    }
    case VariantIndex<MaterialValue, int32_t>():
    {
        gl::UniformInt u{se.location};
        gl::Uniform(u, std::get<int32_t>(v));
        break;
    }
    case VariantIndex<MaterialValue, bool>():
    {
        gl::UniformBool u{se.location};
        gl::Uniform(u, std::get<bool>(v));
        break;
    }
    case VariantIndex<MaterialValue, Texture2D>():
    {
        auto& impl = const_cast<Texture2D::Impl&>(*std::get<Texture2D>(v).m_Impl);
        gl::Texture2D& texture = impl.updTexture();

        gl::ActiveTexture(GL_TEXTURE0 + textureSlot);
        gl::BindTexture(texture);
        gl::UniformSampler2D u{se.location};
        gl::Uniform(u, textureSlot);

        ++textureSlot;
        break;
    }
    case VariantIndex<MaterialValue, RenderTexture>():
    {
        auto& impl = const_cast<RenderTexture::Impl&>(*std::get<RenderTexture>(v).m_Impl);
        gl::Texture2D const& texture = impl.getResolvedColorTexture();

        gl::ActiveTexture(GL_TEXTURE0 + textureSlot);
        gl::BindTexture(texture);
        gl::UniformSampler2D u{se.location};
        gl::Uniform(u, textureSlot);

        ++textureSlot;
        break;
    }
    case VariantIndex<MaterialValue, Cubemap>():
    {
        auto& impl = const_cast<Cubemap::Impl&>(*std::get<Cubemap>(v).m_Impl);
        gl::TextureCubemap const& texture = impl.updCubemap();

        gl::ActiveTexture(GL_TEXTURE0 + textureSlot);
        gl::BindTexture(texture);
        gl::UniformSamplerCube u{se.location};
        gl::Uniform(u, textureSlot);

        ++textureSlot;
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

    auto& matImpl = const_cast<Material::Impl&>(*els.front().material.m_Impl);
    auto& shaderImpl = const_cast<Shader::Impl&>(*matImpl.m_Shader.m_Impl);
    ankerl::unordered_dense::map<std::string, ShaderElement> const& uniforms = shaderImpl.getUniforms();

    // preemptively upload instance data
    std::optional<InstancingState> maybeInstances = UploadInstanceData(els, shaderImpl);

    // updated by various batches (which may bind to textures etc.)
    int32_t textureSlot = 0;

    gl::UseProgram(shaderImpl.updProgram());

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (matImpl.getDepthFunction() != DepthFunction::Default)
    {
        glDepthFunc(ToGLDepthFunc(matImpl.getDepthFunction()));
    }

    // bind material variables
    {
        // try binding to uView (standard)
        if (shaderImpl.m_MaybeViewMatUniform)
        {
            if (shaderImpl.m_MaybeViewMatUniform->shaderType == ShaderType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewMatUniform->location};
                gl::Uniform(u, scene.viewMatrix);
            }
        }

        // try binding to uProjection (standard)
        if (shaderImpl.m_MaybeProjMatUniform)
        {
            if (shaderImpl.m_MaybeProjMatUniform->shaderType == ShaderType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeProjMatUniform->location};
                gl::Uniform(u, scene.projectionMatrix);
            }
        }

        if (shaderImpl.m_MaybeViewProjMatUniform)
        {
            if (shaderImpl.m_MaybeViewProjMatUniform->shaderType == ShaderType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewProjMatUniform->location};
                gl::Uniform(u, scene.viewProjectionMatrix);
            }
        }

        // bind material values
        for (auto const& [name, value] : matImpl.m_Values)
        {
            if (ShaderElement const* e = TryGetValue(uniforms, name))
            {
                TryBindMaterialValueToShaderElement(*e, value, textureSlot);
            }
        }
    }

    // batch by material property block
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMaterialPropertyBlock{&batchIt->maybePropBlock});
        HandleBatchWithSameMaterialPropertyBlock({batchIt, batchEnd}, textureSlot, maybeInstances);
        batchIt = batchEnd;
    }

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (matImpl.getDepthFunction() != DepthFunction::Default)
    {
        glDepthFunc(ToGLDepthFunc(DepthFunction::Default));
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
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMaterial{&batchIt->material});
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
        auto const opaqueEnd = std::find_if_not(batchIt, els.end(), IsOpaque);

        if (opaqueEnd != batchIt)
        {
            // [batchIt..opaqueEnd] contains opaque elements
            gl::Disable(GL_BLEND);
            DrawBatchedByMaterial(scene, {batchIt, opaqueEnd});

            batchIt = opaqueEnd;
        }

        if (opaqueEnd != els.end())
        {
            // [opaqueEnd..els.end()] contains transparent elements
            auto const transparentEnd = std::find_if(opaqueEnd, els.end(), IsOpaque);
            gl::Enable(GL_BLEND);
            DrawBatchedByMaterial(scene, {opaqueEnd, transparentEnd});

            batchIt = transparentEnd;
        }
    }
}

void osc::GraphicsBackend::FlushRenderQueue(Camera::Impl& camera, float aspectRatio)
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
        camera.getProjectionMatrix(aspectRatio),
    };

    gl::Enable(GL_DEPTH_TEST);

    // draw by reordering depth-tested elements around the not-depth-tested elements
    auto batchIt = queue.begin();
    while (batchIt != queue.end())
    {
        auto const depthTestedEnd = std::find_if_not(batchIt, queue.end(), IsDepthTested);

        if (depthTestedEnd != batchIt)
        {
            // there are >0 depth-tested elements that are elegible for reordering

            SortRenderQueue(batchIt, depthTestedEnd, scene.cameraPos);
            DrawBatchedByOpaqueness(scene, {batchIt, depthTestedEnd});

            batchIt = depthTestedEnd;
        }

        if (depthTestedEnd != queue.end())
        {
            // there are >0 not-depth-tested elements that cannot be reordered

            auto const ignoreDepthTestEnd = std::find_if(depthTestedEnd, queue.end(), IsDepthTested);

            // these elements aren't depth-tested and should just be drawn as-is
            gl::Disable(GL_DEPTH_TEST);
            DrawBatchedByOpaqueness(scene, {depthTestedEnd, ignoreDepthTestEnd});
            gl::Enable(GL_DEPTH_TEST);

            batchIt = ignoreDepthTestEnd;
        }
    }

    // queue flushed: clear it
    queue.clear();
}

void osc::GraphicsBackend::ValidateRenderTarget(RenderTarget& renderTarget)
{
    // ensure there is at least one color attachment
    OSC_THROWING_ASSERT(!renderTarget.colors.empty() && "a render target must have one or more color attachments");

    OSC_THROWING_ASSERT(renderTarget.colors.front().buffer != nullptr && "a color attachment must have a non-null render buffer");
    glm::ivec2 const firstColorBufferDimensions = renderTarget.colors.front().buffer->m_Impl->getDimensions();
    int32_t const firstColorBufferSamples = renderTarget.colors.front().buffer->m_Impl->getAntialiasingLevel();

    // validate other buffers against the first
    for (auto it = renderTarget.colors.begin()+1; it != renderTarget.colors.end(); ++it)
    {
        RenderTargetColorAttachment const& colorAttachment = *it;
        OSC_THROWING_ASSERT(colorAttachment.buffer != nullptr);
        OSC_THROWING_ASSERT(colorAttachment.buffer->m_Impl->getDimensions() == firstColorBufferDimensions);
        OSC_THROWING_ASSERT(colorAttachment.buffer->m_Impl->getAntialiasingLevel() == firstColorBufferSamples);
    }
    OSC_THROWING_ASSERT(renderTarget.depth.buffer != nullptr);
    OSC_THROWING_ASSERT(renderTarget.depth.buffer->m_Impl->getDimensions() == firstColorBufferDimensions);
    OSC_THROWING_ASSERT(renderTarget.depth.buffer->m_Impl->getAntialiasingLevel() == firstColorBufferSamples);
}

osc::Rect osc::GraphicsBackend::CalcViewportRect(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    glm::vec2 const targetDims = maybeCustomRenderTarget ?
        maybeCustomRenderTarget->colors.front().buffer->m_Impl->getDimensions() :
        App::get().idims();

    Rect const cameraRect = camera.getPixelRect() ?
        *camera.getPixelRect() :
        Rect{{}, targetDims};

    glm::vec2 const cameraRectBottomLeft = BottomLeft(cameraRect);
    glm::vec2 const outputDimensions = Dimensions(cameraRect);
    glm::vec2 const topLeft = {cameraRectBottomLeft.x, targetDims.y - cameraRectBottomLeft.y};

    return Rect{topLeft, topLeft + outputDimensions};
}

osc::Rect osc::GraphicsBackend::SetupTopLevelPipelineState(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    Rect const viewportRect = CalcViewportRect(camera, maybeCustomRenderTarget);
    glm::vec2 const viewportDims = Dimensions(viewportRect);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl::Viewport(
        static_cast<GLsizei>(viewportRect.p1.x),
        static_cast<GLsizei>(viewportRect.p1.y),
        static_cast<GLsizei>(viewportDims.x),
        static_cast<GLsizei>(viewportDims.y)
    );

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

    return viewportRect;
}

void osc::GraphicsBackend::TeardownTopLevelPipelineState(
    Camera::Impl& camera,
    RenderTarget*)
{
    if (camera.m_MaybeScissorRect)
    {
        gl::Disable(GL_SCISSOR_TEST);
    }
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    gl::UseProgram();
}

std::optional<gl::FrameBuffer> osc::GraphicsBackend::BindAndClearRenderBuffers(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    // if necessary, create pass-specific FBO
    std::optional<gl::FrameBuffer> maybeRenderFBO;

    if (maybeCustomRenderTarget)
    {
        gl::FrameBuffer& fbo = maybeRenderFBO.emplace();
        gl::BindFramebuffer(
            GL_DRAW_FRAMEBUFFER,
            fbo
        );

        // attach buffers
        for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
        {
            RenderTargetColorAttachment& colorAttachment = maybeCustomRenderTarget->colors[i];

            gl::FramebufferRenderbuffer(
                GL_DRAW_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                colorAttachment.buffer->m_Impl->updRBO()
            );
        }
        gl::FramebufferRenderbuffer(
            GL_DRAW_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            maybeCustomRenderTarget->depth.buffer->m_Impl->updRBO()
        );

        // tell OpenGL to use all buffers when drawing/clearing
        std::vector<GLenum> drawBuffers;
        drawBuffers.reserve(maybeCustomRenderTarget->colors.size());
        for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
        {
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i));
        }
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

        // clear buffers based on buffer flags
        static_assert(static_cast<size_t>(osc::RenderBufferLoadAction::TOTAL) == 2);
        for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
        {
            RenderTargetColorAttachment& colorAttachment = maybeCustomRenderTarget->colors[i];
            if (colorAttachment.loadAction == osc::RenderBufferLoadAction::Clear)
            {
                glm::vec4 const color = colorAttachment.clearColor;
                glClearBufferfv(GL_COLOR, static_cast<GLint>(i), glm::value_ptr(color));
            }
        }
        if (maybeCustomRenderTarget->depth.loadAction == osc::RenderBufferLoadAction::Clear)
        {
            gl::Clear(GL_DEPTH_BUFFER_BIT);
        }
    }
    else
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);

        // we're rendering to the window
        if (camera.m_ClearFlags != CameraClearFlags::Nothing)
        {
            // clear window
            GLenum const clearFlags = camera.m_ClearFlags == CameraClearFlags::SolidColor ?
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT :
                GL_DEPTH_BUFFER_BIT;

            // clear color is in sRGB, but the window's framebuffer is sRGB-corrected
            // and assume that clear colors are in linear space
            Color const linearColor = ToLinear(camera.m_BackgroundColor);
            gl::ClearColor(
                linearColor.r,
                linearColor.g,
                linearColor.b,
                linearColor.a
            );
            gl::Clear(clearFlags);
        }
    }

    return maybeRenderFBO;
}

void osc::GraphicsBackend::ResolveRenderBuffers(RenderTarget& renderTarget)
{
    static_assert(static_cast<size_t>(RenderBufferStoreAction::TOTAL) == 2, "check 'if's etc. in this code");

    OSC_PERF("RenderTexture::resolveBuffers");

    glm::ivec2 const dimensions = renderTarget.colors.at(0).buffer->m_Impl->getDimensions();

    // setup FBOs
    gl::FrameBuffer multisampledReadFBO;
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, multisampledReadFBO);

    gl::FrameBuffer resolvedDrawFBO;
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedDrawFBO);

    // resolve color buffers with a blit
    for (size_t i = 0; i < renderTarget.colors.size(); ++i)
    {
        RenderTargetColorAttachment const& attachment = renderTarget.colors[i];

        if (attachment.storeAction == RenderBufferStoreAction::Resolve)
        {
            GLint const attachmentLoc = GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i);

            gl::FramebufferRenderbuffer(
                GL_READ_FRAMEBUFFER,
                attachmentLoc,
                attachment.buffer->m_Impl->updRBO()
            );
            glReadBuffer(attachmentLoc);

            gl::FramebufferTexture2D(
                GL_DRAW_FRAMEBUFFER,
                attachmentLoc,
                attachment.buffer->m_Impl->updResolvedTexture(),
                0
            );
            glDrawBuffer(attachmentLoc);

            gl::BlitFramebuffer(
                0,
                0,
                dimensions.x,
                dimensions.y,
                0,
                0,
                dimensions.x,
                dimensions.y,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );
        }
    }

    // resolve depth buffer with a blit
    if (renderTarget.depth.storeAction == RenderBufferStoreAction::Resolve)
    {
        GLint const attachmentLoc = GL_DEPTH_ATTACHMENT;

        gl::FramebufferRenderbuffer(
            GL_READ_FRAMEBUFFER,
            attachmentLoc,
            renderTarget.depth.buffer->m_Impl->updRBO()
        );
        glReadBuffer(attachmentLoc);

        gl::FramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            attachmentLoc,
            renderTarget.depth.buffer->m_Impl->updResolvedTexture(),
            0
        );
        glDrawBuffer(attachmentLoc);

        gl::BlitFramebuffer(
            0,
            0,
            dimensions.x,
            dimensions.y,
            0,
            0,
            dimensions.x,
            dimensions.y,
            GL_DEPTH_BUFFER_BIT,
            GL_NEAREST
        );
    }
}

void osc::GraphicsBackend::RenderScene(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    OSC_PERF("GraphicsBackend::RenderScene");

    if (maybeCustomRenderTarget)
    {
        ValidateRenderTarget(*maybeCustomRenderTarget);
    }

    Rect const viewportRect = SetupTopLevelPipelineState(
        camera,
        maybeCustomRenderTarget
    );

    {
        std::optional<gl::FrameBuffer> const maybeTmpFBO =
            BindAndClearRenderBuffers(camera, maybeCustomRenderTarget);
        FlushRenderQueue(camera, AspectRatio(viewportRect));
    }

    if (maybeCustomRenderTarget)
    {
        ResolveRenderBuffers(*maybeCustomRenderTarget);
    }

    TeardownTopLevelPipelineState(
        camera,
        maybeCustomRenderTarget
    );
}

void osc::GraphicsBackend::DrawMesh(
    Mesh const& mesh,
    Transform const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock)
{
    camera.m_Impl.upd()->m_RenderQueue.emplace_back(mesh, transform, material, maybeMaterialPropertyBlock);
}

void osc::GraphicsBackend::DrawMesh(
    Mesh const& mesh,
    glm::mat4 const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock)
{
    camera.m_Impl.upd()->m_RenderQueue.emplace_back(mesh, transform, material, maybeMaterialPropertyBlock);
}

void osc::GraphicsBackend::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    BlitFlags flags)
{
    OSC_ASSERT(g_GraphicsContextImpl);
    OSC_ASSERT(t.m_Impl->hasBeenRenderedTo() && "the input texture has not been rendered to");

    if (flags == BlitFlags::AlphaBlend)
    {
        Camera c;
        c.setBackgroundColor(Color::clear());
        c.setPixelRect(rect);
        c.setProjectionMatrixOverride(glm::mat4{1.0f});
        c.setViewMatrixOverride(glm::mat4{1.0f});
        c.setClearFlags(CameraClearFlags::Nothing);

        g_GraphicsContextImpl->m_QuadMaterial.setRenderTexture("uTexture", t);
        Graphics::DrawMesh(g_GraphicsContextImpl->m_QuadMesh, Transform{}, g_GraphicsContextImpl->m_QuadMaterial, c);
        c.renderToScreen();
        g_GraphicsContextImpl->m_QuadMaterial.clearRenderTexture("uTexture");
    }
    else
    {
        // rect is currently top-left, must be converted to bottom-left

        int32_t const windowHeight = App::get().idims().y;
        auto const rectHeight = static_cast<int32_t>(rect.p2.y - rect.p1.y);
        auto const p1y = static_cast<int32_t>((windowHeight - static_cast<int32_t>(rect.p1.y)) - rectHeight);
        auto const p2y = static_cast<int32_t>(windowHeight - static_cast<int32_t>(rect.p1.y));
        glm::ivec2 const texDimensions = t.getDimensions();

        // blit multisampled scene render to not-multisampled texture
        gl::FrameBuffer fbo;
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        gl::FramebufferTexture2D(
            GL_READ_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            const_cast<RenderTexture::Impl&>(*t.m_Impl).getResolvedColorTexture(),
            0
        );
        gl::FramebufferTexture2D(
            GL_READ_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            const_cast<RenderTexture::Impl&>(*t.m_Impl).getResolvedDepthTexture(),
            0
        );
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, gl::windowFbo);

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
    OSC_ASSERT(t.m_Impl->hasBeenRenderedTo() && "the input texture has not been rendered to");

    Camera c;
    c.setBackgroundColor(Color::clear());
    c.setPixelRect(rect);
    c.setProjectionMatrixOverride(glm::mat4{1.0f});
    c.setViewMatrixOverride(glm::mat4{1.0f});
    c.setClearFlags(CameraClearFlags::Nothing);

    Material copy{material};

    copy.setRenderTexture("uTexture", t);
    Graphics::DrawMesh(g_GraphicsContextImpl->m_QuadMesh, Transform{}, copy, c);
    c.renderToScreen();
    copy.clearRenderTexture("uTexture");
}

void osc::GraphicsBackend::Blit(Texture2D const& source, RenderTexture& dest)
{
    Camera c;
    c.setBackgroundColor(Color::clear());
    c.setProjectionMatrixOverride(glm::mat4{1.0f});
    c.setViewMatrixOverride(glm::mat4{1.0f});

    g_GraphicsContextImpl->m_QuadMaterial.setTexture("uTexture", source);

    Graphics::DrawMesh(g_GraphicsContextImpl->m_QuadMesh, Transform{}, g_GraphicsContextImpl->m_QuadMaterial, c);

    c.renderTo(dest);

    g_GraphicsContextImpl->m_QuadMaterial.clearTexture("uTexture");
}

void osc::GraphicsBackend::ReadPixels(RenderTexture const& source, Image& dest)
{
    glm::ivec2 const dims = source.getDimensions();
    int32_t const channels = GetNumChannels(source.getColorFormat());

    std::vector<uint8_t> pixels(static_cast<size_t>(channels*dims.x*dims.y));

    gl::Viewport(0, 0, dims.x, dims.y);

    gl::FrameBuffer fbo;
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    gl::FramebufferTexture2D(
        GL_READ_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        const_cast<RenderTexture::Impl&>(*source.m_Impl).getResolvedColorTexture(),
        0
    );
    gl::FramebufferTexture2D(
        GL_READ_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        const_cast<RenderTexture::Impl&>(*source.m_Impl).getResolvedDepthTexture(),
        0
    );
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    GLint const packFormat = ToImagePixelPackAlignment(source.getColorFormat());
    OSC_ASSERT(reinterpret_cast<uintptr_t>(pixels.data()) % packFormat == 0 && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
    gl::PixelStorei(GL_PACK_ALIGNMENT, packFormat);
    glReadPixels(
        0,
        0,
        dims.x,
        dims.y,
        ToImageColorFormat(source.getColorFormat()),
        ToImageDataType(source.getColorFormat()),
        pixels.data()
    );

    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);

    dest = Image{dims, pixels, channels, ColorSpace::sRGB};
}
