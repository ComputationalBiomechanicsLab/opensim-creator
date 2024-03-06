// these are the things that this file "implements"

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/CameraClearFlags.h>
#include <oscar/Graphics/CameraProjection.h>
#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/Detail/ShaderPropertyTypeList.h>
#include <oscar/Graphics/Detail/ShaderPropertyTypeTraits.h>
#include <oscar/Graphics/Detail/TextureFormatList.h>
#include <oscar/Graphics/Detail/TextureFormatTraits.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatList.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <oscar/Graphics/Detail/VertexAttributeHelpers.h>
#include <oscar/Graphics/Detail/VertexAttributeList.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/GraphicsContext.h>
#include <oscar/Graphics/GraphicsHelpers.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshGenerators.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/OpenGL/CPUDataTypeOpenGLTraits.h>
#include <oscar/Graphics/OpenGL/CPUImageFormatOpenGLTraits.h>
#include <oscar/Graphics/OpenGL/Gl.h>
#include <oscar/Graphics/OpenGL/TextureFormatOpenGLTraits.h>
#include <oscar/Graphics/RenderBuffer.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTarget.h>
#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthAttachment.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/RenderTextureDescriptor.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Graphics/TextureWrapMode.h>
#include <oscar/Graphics/Unorm8.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeDescriptor.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Graphics/VertexFormat.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Detail/SDL2Helpers.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Concepts.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/DefaultConstructOnCopy.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/ObjectRepresentation.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/UID.h>

#include <GL/glew.h>
#include <ankerl/unordered_dense.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace osc::detail;
using namespace osc::literals;
using namespace osc;
namespace cpp20 = osc::cpp20;
namespace gl = osc::gl;
namespace sdl = osc::sdl;

// shader source
namespace
{
    // vertex shader source used for blitting a textured quad (common use-case)
    //
    // it's here, rather than in an external resource file, because it is eagerly
    // loaded while the graphics backend is initialized (i.e. potentially before
    // the application is fully loaded)
    constexpr CStringView c_QuadVertexShaderSrc = R"(
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
    constexpr CStringView c_QuadFragmentShaderSrc = R"(
        #version 330 core

        uniform sampler2D uTexture;

        in vec2 TexCoord;
        out vec4 FragColor;

        void main()
        {
            FragColor = texture(uTexture, TexCoord);
        }
    )";

    CStringView GLStringToCStringView(GLubyte const* stringPtr)
    {
        using value_type = CStringView::value_type;

        static_assert(sizeof(GLubyte) == sizeof(value_type));
        static_assert(alignof(GLubyte) == alignof(value_type));
        if (stringPtr) {
            return CStringView{std::launder(reinterpret_cast<value_type const*>(stringPtr))};
        }
        else {
            return CStringView{};
        }
    }

    CStringView GLGetCStringView(GLenum name)
    {
        return GLStringToCStringView(glGetString(name));
    }

    CStringView GLGetCStringViewi(GLenum name, GLuint index)
    {
        return GLStringToCStringView(glGetStringi(name, index));
    }

    bool IsAlignedAtLeast(void const* ptr, GLint requiredAlignment)
    {
        return cpp20::bit_cast<intptr_t>(ptr) % requiredAlignment == 0;
    }

    // returns the `Name String`s of all extensions that OSC's OpenGL backend might use
    std::vector<CStringView> GetAllOpenGLExtensionsUsedByOSC()
    {
        // most entries in this list were initially from a mixture of:
        //
        // - https://www.khronos.org/opengl/wiki/History_of_OpenGL (lists historical extension changes)
        // - Khronos official pages

        // this list isn't comprehensive, it's just things that I reakon the OSC backend
        // wants, so that, at runtime, the graphics backend can emit user-facing warning
        // messages so that it's a little bit easier to spot production bugs

        return {
            // framebuffer objects, blitting, multisampled renderbuffer objects, and
            // packed depth+stencil image formats
            //
            // core in OpenGL 3.0
            "GL_ARB_framebuffer_object",

            // VAOs
            //
            // core in OpenGL 3.0
            "GL_ARB_vertex_array_object",

            // GL_HALF_FLOAT as a texture pixel format (e.g. HDR textures)
            //
            // core in OpenGL 3.0
            "GL_ARB_half_float_pixel",

            // floating point color and depth internal formats for textures
            // and render buffers
            //
            // core in OpenGL 3.0
            "GL_ARB_color_buffer_float",
            "GL_ARB_texture_float",

            // hardware support for automatic sRGB/linear color conversion via
            // framebuffers and GL_FRAMEBUFFER_SRGB
            //
            // core in OpenGL 3.0
            "GL_EXT_framebuffer_sRGB",

            "GL_EXT_texture_sRGB",

            // shaders
            //
            // core in OpenGL 2.0
            "GL_ARB_shader_objects",
            "GL_ARB_vertex_shader",
            "GL_ARB_fragment_shader",

            // multi-render target (MRT) support
            //
            // core in OpenGL 2.0
            "GL_ARB_draw_buffers",

            // non-power-of-2 texture sizes
            //
            // core in OpenGL 2.0
            "GL_ARB_texture_non_power_of_two",

            // VBOs
            //
            // core in OpenGL 1.5
            "GL_ARB_vertex_buffer_object",

            // mipmap generation
            //
            // core in OpenGL 1.4
            "GL_SGIS_generate_mipmap",

            // depth textures
            //
            // core in OpenGL 1.4
            "GL_ARB_depth_texture",

            // separate blend functions (might be handy with premultiplied alpha at some point)
            //
            // core in OpenGL 1.4
            "GL_EXT_blend_func_separate",

            // mirrored repeating of textures
            //
            // core in OpenGL 1.4
            "GL_ARB_texture_mirrored_repeat",

            // cubemap support
            //
            // core in OpenGL 1.3
            "GL_ARB_texture_cube_map",

            // MSXAA support
            //
            // core in OpenGL 1.3
            "GL_ARB_multisample",

            // core in OpenGL 1.3
            "GL_ARB_texture_border_clamp",

            // core in OpenGL 1.2
            "GL_EXT_texture3D",

            // core in OpenGL 1.1
            "GL_EXT_vertex_array",
            "GL_EXT_texture_object",

            // also from OpenGL 1.1, but don't seem to be reported
            // by the NVIDIA backend?
            //
            // "GL_EXT_blend_logic_op",
            // "GL_EXT_texture",
            // "GL_EXT_copy_texture",
            // "GL_EXT_subtexture",
        };
    }

    size_t GetNumExtensionsSupportedByBackend()
    {
        GLint numExtensionsSupportedByBackend = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensionsSupportedByBackend);
        return numExtensionsSupportedByBackend >= 0 ? static_cast<size_t>(numExtensionsSupportedByBackend) : 0;
    }

    std::vector<CStringView> GetAllExtensionsSupportedByCurrentOpenGLBackend()
    {
        size_t const numExtensions = GetNumExtensionsSupportedByBackend();

        std::vector<CStringView> rv;
        rv.reserve(numExtensions);
        for (size_t i = 0; i < numExtensions; ++i) {
            rv.emplace_back(GLGetCStringViewi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        }
        return rv;
    }

    void ValidateOpenGLBackendExtensionSupport(LogLevel logLevel)
    {
        // note: the OpenGL specification _requires_ that a backend supports
        // (effectively) RGBA, RG, and RED textures with the following data
        // formats for each channel:
        //
        // - uint8 (normalized)
        // - int8 (normalized)
        // - float32
        // - uint8/uint16/uint32 (non-normalized)
        // - int8/int16/int32 (non-normalized)
        //
        // see "Required Formats" in: https://www.khronos.org/opengl/wiki/Image_Format

        if (logLevel < log_level()) {
            return;
        }

        std::vector<CStringView> extensionsRequiredByOSC = GetAllOpenGLExtensionsUsedByOSC();
        std::sort(extensionsRequiredByOSC.begin(), extensionsRequiredByOSC.end());

        std::vector<CStringView> extensionSupportedByBackend = GetAllExtensionsSupportedByCurrentOpenGLBackend();
        std::sort(extensionSupportedByBackend.begin(), extensionSupportedByBackend.end());

        std::vector<CStringView> missingExtensions;
        missingExtensions.reserve(extensionsRequiredByOSC.size());  // pessimistic guess
        std::set_difference(
            extensionsRequiredByOSC.begin(),
            extensionsRequiredByOSC.end(),
            extensionSupportedByBackend.begin(),
            extensionSupportedByBackend.end(),
            std::back_inserter(missingExtensions)
        );

        if (!missingExtensions.empty()) {
            log_message(logLevel, "OpenGL: the following OpenGL extensions may be missing from the graphics backend: ");
            for (auto const& missingExtension : missingExtensions)
            {
                log_message(logLevel, "OpenGL:  - %s", missingExtension.c_str());
            }
            log_message(logLevel, "OpenGL: because extensions may be missing, rendering may behave abnormally");
            log_message(logLevel, "OpenGL: note: some graphics engines can mis-report an extension as missing");
        }

        log_message(logLevel, "OpenGL: here is a list of all of the extensions supported by the graphics backend:");
        for (auto const& ext : extensionSupportedByBackend) {
            log_message(logLevel, "OpenGL:  - %s", ext.c_str());
        }
    }
}


// generic utility functions
namespace
{
    template<BitCastable T>
    void PushAsBytes(T const& v, std::vector<uint8_t>& out)
    {
        auto const bytes = ViewObjectRepresentation<uint8_t>(v);
        out.insert(out.end(), bytes.begin(), bytes.end());
    }

    template<typename VecOrMat>
    std::span<typename VecOrMat::element_type const> ToFloatSpan(VecOrMat const& v)
        requires BitCastable<typename VecOrMat::element_type>
    {
        return {value_ptr(v), sizeof(VecOrMat)/sizeof(typename VecOrMat::element_type)};
    }
}

// material value storage
//
// materials can store a variety of stuff (colors, positions, offsets, textures, etc.). This
// code defines how it's actually stored at runtime
namespace
{
    using MaterialValue = std::variant<
        Color,
        std::vector<Color>,
        float,
        std::vector<float>,
        Vec2,
        Vec3,
        std::vector<Vec3>,
        Vec4,
        Mat3,
        Mat4,
        std::vector<Mat4>,
        int32_t,
        bool,
        Texture2D,
        RenderTexture,
        Cubemap
    >;

    ShaderPropertyType GetShaderType(MaterialValue const& v)
    {
        switch (v.index()) {
        case VariantIndex<MaterialValue, Color>():
        case VariantIndex<MaterialValue, std::vector<Color>>():
            return ShaderPropertyType::Vec4;
        case VariantIndex<MaterialValue, Vec2>():
            return ShaderPropertyType::Vec2;
        case VariantIndex<MaterialValue, float>():
        case VariantIndex<MaterialValue, std::vector<float>>():
            return ShaderPropertyType::Float;
        case VariantIndex<MaterialValue, Vec3>():
        case VariantIndex<MaterialValue, std::vector<Vec3>>():
            return ShaderPropertyType::Vec3;
        case VariantIndex<MaterialValue, Vec4>():
            return ShaderPropertyType::Vec4;
        case VariantIndex<MaterialValue, Mat3>():
            return ShaderPropertyType::Mat3;
        case VariantIndex<MaterialValue, Mat4>():
        case VariantIndex<MaterialValue, std::vector<Mat4>>():
            return ShaderPropertyType::Mat4;
        case VariantIndex<MaterialValue, int32_t>():
            return ShaderPropertyType::Int;
        case VariantIndex<MaterialValue, bool>():
            return ShaderPropertyType::Bool;
        case VariantIndex<MaterialValue, Texture2D>():
            return ShaderPropertyType::Sampler2D;
        case VariantIndex<MaterialValue, RenderTexture>(): {

            static_assert(NumOptions<TextureDimensionality>() == 2);
            return std::get<RenderTexture>(v).getDimensionality() == TextureDimensionality::Tex2D ?
                ShaderPropertyType::Sampler2D :
                ShaderPropertyType::SamplerCube;
        }
        case VariantIndex<MaterialValue, Cubemap>():
            return ShaderPropertyType::SamplerCube;
        default:
            return ShaderPropertyType::Unknown;
        }
    }
}

// shader (backend stuff)
namespace
{
    // convert a GL shader type to an internal shader type
    ShaderPropertyType GLShaderTypeToShaderTypeInternal(GLenum e)
    {
        static_assert(NumOptions<ShaderPropertyType>() == 11);

        switch (e) {
        case GL_FLOAT:
            return ShaderPropertyType::Float;
        case GL_FLOAT_VEC2:
            return ShaderPropertyType::Vec2;
        case GL_FLOAT_VEC3:
            return ShaderPropertyType::Vec3;
        case GL_FLOAT_VEC4:
            return ShaderPropertyType::Vec4;
        case GL_FLOAT_MAT3:
            return ShaderPropertyType::Mat3;
        case GL_FLOAT_MAT4:
            return ShaderPropertyType::Mat4;
        case GL_INT:
            return ShaderPropertyType::Int;
        case GL_BOOL:
            return ShaderPropertyType::Bool;
        case GL_SAMPLER_2D:
            return ShaderPropertyType::Sampler2D;
        case GL_SAMPLER_CUBE:
            return ShaderPropertyType::SamplerCube;
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
            return ShaderPropertyType::Unknown;
        }
    }

    std::string NormalizeShaderElementName(std::string_view openGLName)
    {
        std::string s{openGLName};
        auto loc = s.find('[');
        if (loc != std::string::npos) {
            s.erase(loc);
        }
        return s;
    }

    // parsed-out description of a shader "element" (uniform/attribute)
    struct ShaderElement final {
        ShaderElement(
            int32_t location_,
            ShaderPropertyType shaderType_,
            int32_t size_) :

            location{location_},
            shaderType{shaderType_},
            size{size_}
        {
        }

        int32_t location;
        ShaderPropertyType shaderType;
        int32_t size;
    };

    void PrintShaderElement(std::ostream& o, std::string_view name, ShaderElement const& se)
    {
        o << "ShadeElement(name = " << name << ", location = " << se.location << ", shaderType = " << se.shaderType << ", size = " << se.size << ')';
    }

    // see: ankerl/unordered_dense documentation for heterogeneous lookups
    struct transparent_string_hash final {
        using is_transparent = void;
        using is_avalanching = void;

        [[nodiscard]] auto operator()(std::string_view str) const -> uint64_t {
            return ankerl::unordered_dense::hash<std::string_view>{}(str);
        }
    };

    template<typename Value>
    using FastStringHashtable = ankerl::unordered_dense::map<
        std::string,
        Value,
        transparent_string_hash,
        std::equal_to<>
    >;

    ShaderElement const* TryGetValue(FastStringHashtable<ShaderElement> const& m, std::string_view k)
    {
        auto const it = m.find(k);
        return it != m.end() ? &it->second : nullptr;
    }
}

namespace
{
    // transform storage: either as a matrix or a transform
    //
    // calling code is allowed to submit transforms as either Transform (preferred) or
    // `Mat4` (can be handier)
    //
    // these need to be stored as-is, because that's the smallest possible representation and
    // the drawing algorithm needs to traverse + sort the render objects at runtime (so size
    // is important)
    using Mat4OrTransform = std::variant<Mat4, Transform>;

    Mat4 mat4_cast(Mat4OrTransform const& matrixOrTransform)
    {
        return std::visit(Overload{
            [](Mat4 const& matrix) { return matrix; },
            [](Transform const& transform) { return mat4_cast(transform); }
        }, matrixOrTransform);
    }

    Mat4 ToNormalMat4(Mat4OrTransform const& matrixOrTransform)
    {
        return std::visit(Overload{
            [](Mat4 const& matrix) { return normal_matrix4(matrix); },
            [](Transform const& transform) { return normal_matrix_4x4(transform); }
        }, matrixOrTransform);
    }

    Mat3 ToNormalMat3(Mat4OrTransform const& matrixOrTransform)
    {
        return std::visit(Overload{
            [](Mat4 const& matrix) { return normal_matrix(matrix); },
            [](Transform const& transform) { return normal_matrix(transform); }
        }, matrixOrTransform);
    }

    // this is what is stored in the renderer's render queue
    struct RenderObject final {

        RenderObject(
            Mesh mesh_,
            Transform const& transform_,
            Material material_,
            std::optional<MaterialPropertyBlock> maybePropBlock_,
            std::optional<size_t> maybeSubMeshIndex_) :

            material{std::move(material_)},
            mesh{std::move(mesh_)},
            maybePropBlock{std::move(maybePropBlock_)},
            transform{transform_},
            worldMidpoint{material.getTransparent() ? transform_point(transform_, centroid(mesh.getBounds())) : Vec3{}},
            maybeSubMeshIndex{maybeSubMeshIndex_}
        {
        }

        RenderObject(
            Mesh mesh_,
            Mat4 const& transform_,
            Material material_,
            std::optional<MaterialPropertyBlock> maybePropBlock_,
            std::optional<size_t> maybeSubMeshIndex_) :

            material{std::move(material_)},
            mesh{std::move(mesh_)},
            maybePropBlock{std::move(maybePropBlock_)},
            transform{transform_},
            worldMidpoint{material.getTransparent() ? transform_ * Vec4{centroid(mesh.getBounds()), 1.0f} : Vec3{}},
            maybeSubMeshIndex{maybeSubMeshIndex_}
        {
        }

        friend void swap(RenderObject& a, RenderObject& b) noexcept
        {
            using std::swap;

            swap(a.material, b.material);
            swap(a.mesh, b.mesh);
            swap(a.maybePropBlock, b.maybePropBlock);
            swap(a.transform, b.transform);
            swap(a.worldMidpoint, b.worldMidpoint);
            swap(a.maybeSubMeshIndex, b.maybeSubMeshIndex);
        }

        friend bool operator==(RenderObject const&, RenderObject const&) = default;

        Material material;
        Mesh mesh;
        std::optional<MaterialPropertyBlock> maybePropBlock;
        Mat4OrTransform transform;
        Vec3 worldMidpoint;
        std::optional<size_t> maybeSubMeshIndex;
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

    Mat4 ModelMatrix(RenderObject const& ro)
    {
        return mat4_cast(ro.transform);
    }

    Mat3 NormalMatrix(RenderObject const& ro)
    {
        return ToNormalMat3(ro.transform);
    }

    Mat4 NormalMatrix4(RenderObject const& ro)
    {
        return ToNormalMat4(ro.transform);
    }

    Vec3 const& WorldMidpoint(RenderObject const& ro)
    {
        return ro.worldMidpoint;
    }

    // function object that returns true if the first argument is farther from the second
    //
    // (handy for depth sorting)
    class RenderObjectIsFartherFrom final {
    public:
        explicit RenderObjectIsFartherFrom(Vec3 const& pos) : m_Pos{pos} {}

        bool operator()(RenderObject const& a, RenderObject const& b) const
        {
            Vec3 const aMidpointWorldSpace = WorldMidpoint(a);
            Vec3 const bMidpointWorldSpace = WorldMidpoint(b);
            Vec3 const camera2a = aMidpointWorldSpace - m_Pos;
            Vec3 const camera2b = bMidpointWorldSpace - m_Pos;
            float const camera2aDistanceSquared = dot(camera2a, camera2a);
            float const camera2bDistanceSquared = dot(camera2b, camera2b);
            return camera2aDistanceSquared > camera2bDistanceSquared;
        }
    private:
        Vec3 m_Pos;
    };

    class RenderObjectHasMaterial final {
    public:
        explicit RenderObjectHasMaterial(Material const* material) :
            m_Material{material}
        {
            OSC_ASSERT(m_Material != nullptr);
        }

        bool operator()(RenderObject const& ro) const
        {
            return ro.material == *m_Material;
        }
    private:
        Material const* m_Material;
    };

    class RenderObjectHasMaterialPropertyBlock final {
    public:
        explicit RenderObjectHasMaterialPropertyBlock(std::optional<MaterialPropertyBlock> const* mpb) :
            m_Mpb{mpb}
        {
            OSC_ASSERT(m_Mpb != nullptr);
        }

        bool operator()(RenderObject const& ro) const
        {
            return ro.maybePropBlock == *m_Mpb;
        }

    private:
        std::optional<MaterialPropertyBlock> const* m_Mpb;
    };

    class RenderObjectHasMesh final {
    public:
        explicit RenderObjectHasMesh(Mesh const* mesh) :
            m_Mesh{mesh}
        {
            OSC_ASSERT(m_Mesh != nullptr);
        }

        bool operator()(RenderObject const& ro) const
        {
            return ro.mesh == *m_Mesh;
        }
    private:
        Mesh const* m_Mesh;
    };

    class RenderObjectHasSubMeshIndex final {
    public:
        explicit RenderObjectHasSubMeshIndex(std::optional<size_t> maybeSubMeshIndex_) :
            m_MaybeSubMeshIndex{maybeSubMeshIndex_}
        {}

        bool operator()(RenderObject const& ro) const
        {
            return ro.maybeSubMeshIndex == m_MaybeSubMeshIndex;
        }
    private:
        std::optional<size_t> m_MaybeSubMeshIndex;
    };

    // sort a sequence of `RenderObject`s for optimal drawing
    std::vector<RenderObject>::iterator SortRenderQueue(
        std::vector<RenderObject>::iterator renderQueueBegin,
        std::vector<RenderObject>::iterator renderQueueEnd,
        Vec3 cameraPos)
    {
        // partition the render queue into [opaqueObjects | transparentObjects]
        auto const opaqueObjectsEnd = std::partition(renderQueueBegin, renderQueueEnd, IsOpaque);

        // optimize the opaqueObjects partition (it can be reordered safely)
        //
        // first, batch opaqueObjects into `RenderObject`s that have the same `Material`
        auto materialBatchStart = renderQueueBegin;
        while (materialBatchStart != opaqueObjectsEnd) {

            auto const materialBatchEnd = std::partition(
                materialBatchStart,
                opaqueObjectsEnd,
                RenderObjectHasMaterial{&materialBatchStart->material}
            );

            // second, batch `RenderObject`s with the same `Material` into sub-batches
            // with the same `MaterialPropertyBlock`
            auto materialPropBlockBatchStart = materialBatchStart;
            while (materialPropBlockBatchStart != materialBatchEnd) {

                auto const materialPropBlockBatchEnd = std::partition(
                    materialPropBlockBatchStart,
                    materialBatchEnd,
                    RenderObjectHasMaterialPropertyBlock{&materialPropBlockBatchStart->maybePropBlock}
                );

                // third, batch `RenderObject`s with the same `Material` and `MaterialPropertyBlock`s
                // into sub-batches with the same `Mesh`
                auto meshBatchStart = materialPropBlockBatchStart;
                while (meshBatchStart != materialPropBlockBatchEnd) {

                    auto const meshBatchEnd = std::partition(
                        meshBatchStart,
                        materialPropBlockBatchEnd,
                        RenderObjectHasMesh{&meshBatchStart->mesh}
                    );

                    // fourth, batch `RenderObject`s with the same `Material`, `MaterialPropertyBlock`,
                    // and `Mesh` into sub-batches with the same sub-mesh index
                    auto subMeshBatchStart = meshBatchStart;
                    while (subMeshBatchStart != meshBatchEnd) {

                        auto const subMeshBatchEnd = std::partition(
                            subMeshBatchStart,
                            meshBatchEnd,
                            RenderObjectHasSubMeshIndex{subMeshBatchStart->maybeSubMeshIndex}
                        );
                        subMeshBatchStart = subMeshBatchEnd;
                    }
                    meshBatchStart = meshBatchEnd;
                }
                materialPropBlockBatchStart = materialPropBlockBatchEnd;
            }
            materialBatchStart = materialBatchEnd;
        }

        // sort the transparent partition by distance from camera (back-to-front)
        std::sort(opaqueObjectsEnd, renderQueueEnd, RenderObjectIsFartherFrom{cameraPos});

        return opaqueObjectsEnd;
    }

    // top-level state for a single call to `render`
    struct RenderPassState final {

        RenderPassState(
            Vec3 const& cameraPos_,
            Mat4 const& viewMatrix_,
            Mat4 const& projectionMatrix_) :

            cameraPos{cameraPos_},
            viewMatrix{viewMatrix_},
            projectionMatrix{projectionMatrix_}
        {
        }

        Vec3 cameraPos;
        Mat4 viewMatrix;
        Mat4 projectionMatrix;
        Mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;
    };

    // the OpenGL data associated with an Texture2D
    struct Texture2DOpenGLData final {
        gl::Texture2D texture;
        UID textureParamsVersion;
    };


    // the OpenGL data associated with an osc::RenderBuffer
    struct SingleSampledTexture final {
        gl::Texture2D texture2D;
    };
    struct MultisampledRBOAndResolvedTexture final {
        gl::RenderBuffer multisampledRBO;
        gl::Texture2D singleSampledTexture;
    };
    struct SingleSampledCubemap final {
        gl::TextureCubemap textureCubemap;
    };
    using RenderBufferOpenGLData = std::variant<
        SingleSampledTexture,
        MultisampledRBOAndResolvedTexture,
        SingleSampledCubemap
    >;

    // the OpenGL data associated with an Mesh
    struct MeshOpenGLData final {
        UID dataVersion;
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
        {}

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
        // internal methods

        static void BindToInstancedAttributes(
            Shader::Impl const& shaderImpl,
            InstancingState& ins
        );

        static void UnbindFromInstancedAttributes(
            Shader::Impl const& shaderImpl,
            InstancingState& ins
        );

        static std::optional<InstancingState> UploadInstanceData(
            std::span<RenderObject const>,
            Shader::Impl const& shaderImpl
        );

        static void TryBindMaterialValueToShaderElement(
            ShaderElement const& se,
            MaterialValue const& v,
            int32_t& textureSlot
        );

        static void HandleBatchWithSameSubMesh(
            std::span<RenderObject const>,
            std::optional<InstancingState>& ins
        );

        static void HandleBatchWithSameMesh(
            std::span<RenderObject const>,
            std::optional<InstancingState>& ins
        );

        static void HandleBatchWithSameMaterialPropertyBlock(
            std::span<RenderObject const>,
            int32_t& textureSlot,
            std::optional<InstancingState>& ins
        );

        static void HandleBatchWithSameMaterial(
            RenderPassState const&,
            std::span<RenderObject const>
        );

        static void DrawRenderObjects(
            RenderPassState const&,
            std::span<RenderObject const>
        );

        static void DrawBatchedByOpaqueness(
            RenderPassState const&,
            std::span<RenderObject const>
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
        static void RenderCameraQueue(
            Camera::Impl& camera,
            RenderTarget* maybeCustomRenderTarget = nullptr
        );


        // public (forwarded) API

        static void DrawMesh(
            Mesh const&,
            Transform const&,
            Material const&,
            Camera&,
            std::optional<MaterialPropertyBlock> const&,
            std::optional<size_t>
        );

        static void DrawMesh(
            Mesh const&,
            Mat4 const&,
            Material const&,
            Camera&,
            std::optional<MaterialPropertyBlock> const&,
            std::optional<size_t>
        );

        static void Blit(
            Texture2D const&,
            RenderTexture&
        );

        static void BlitToScreen(
            RenderTexture const&,
            Rect const&,
            BlitFlags
        );

        static void BlitToScreen(
            RenderTexture const&,
            Rect const&,
            Material const&,
            BlitFlags
        );

        static void BlitToScreen(
            Texture2D const&,
            Rect const&
        );

        static void CopyTexture(
            RenderTexture const&,
            Texture2D&
        );

        static void CopyTexture(
            RenderTexture const&,
            Texture2D&,
            CubemapFace
        );

        static void CopyTexture(
            RenderTexture const&,
            Cubemap&,
            size_t
        );
    };
}

namespace
{
    // returns the memory alignment of data that is to be copied from the
    // CPU (packed) to the GPU (unpacked)
    constexpr GLint ToOpenGLUnpackAlignment(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::unpack_alignment... });
        }(TextureFormatList{});

        return lut.at(ToIndex(format));
    }

    // returns the format OpenGL will use internally (i.e. on the GPU) to
    // represent the given format+colorspace combo
    constexpr GLenum ToOpenGLInternalFormat(
        TextureFormat format,
        ColorSpace colorSpace)
    {
        constexpr auto srgbLUT = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::internal_format_srgb... });
        }(TextureFormatList{});

        constexpr auto linearLUT = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::internal_format_linear... });
        }(TextureFormatList{});

        static_assert(NumOptions<ColorSpace>() == 2);
        if (colorSpace == ColorSpace::sRGB) {
            return srgbLUT.at(ToIndex(format));
        }
        else {
            return linearLUT.at(ToIndex(format));
        }
    }

    constexpr GLenum ToOpenGLDataType(CPUDataType t)
    {
        constexpr auto lut = []<CPUDataType... DataTypes>(OptionList<CPUDataType, DataTypes...>)
        {
            return std::to_array({ CPUDataTypeOpenGLTraits<DataTypes>::opengl_data_type... });
        }(CPUDataTypeList{});

        return lut.at(ToIndex(t));
    }

    constexpr CPUDataType ToEquivalentCPUDataType(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatTraits<Formats>::equivalent_cpu_datatype... });
        }(TextureFormatList{});

        return lut.at(ToIndex(format));
    }

    constexpr CPUImageFormat ToEquivalentCPUImageFormat(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatTraits<Formats>::equivalent_cpu_image_format... });
        }(TextureFormatList{});

        return lut.at(ToIndex(format));
    }

    constexpr GLenum ToOpenGLFormat(CPUImageFormat t)
    {
        constexpr auto lut = []<CPUImageFormat... Formats>(OptionList<CPUImageFormat, Formats...>)
        {
            return std::to_array({ CPUImageFormatOpenGLTraits<Formats>::opengl_format... });
        }(CPUImageFormatList{});

        return lut.at(ToIndex(t));
    }

    constexpr GLenum ToOpenGLTextureEnum(CubemapFace f)
    {
        static_assert(NumOptions<CubemapFace>() == 6);
        static_assert(static_cast<GLenum>(CubemapFace::PositiveX) == 0);
        static_assert(static_cast<GLenum>(CubemapFace::NegativeZ) == 5);
        static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

        return GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(f);
    }

    GLint ToGLTextureTextureWrapParam(TextureWrapMode m)
    {
        static_assert(NumOptions<TextureWrapMode>() == 3);

        switch (m)
        {
        case TextureWrapMode::Repeat:
            return GL_REPEAT;
        case TextureWrapMode::Clamp:
            return GL_CLAMP_TO_EDGE;
        case TextureWrapMode::Mirror:
            return GL_MIRRORED_REPEAT;
        default:
            return GL_REPEAT;
        }
    }

    constexpr auto c_TextureWrapModeStrings = std::to_array<CStringView>(
    {
        "Repeat",
        "Clamp",
        "Mirror",
    });
    static_assert(c_TextureWrapModeStrings.size() == NumOptions<TextureWrapMode>());

    constexpr auto c_TextureFilterModeStrings = std::to_array<CStringView>(
    {
        "Nearest",
        "Linear",
        "Mipmap",
    });
    static_assert(c_TextureFilterModeStrings.size() == NumOptions<TextureFilterMode>());

    GLint ToGLTextureMinFilterParam(TextureFilterMode m)
    {
        static_assert(NumOptions<TextureFilterMode>() == 3);

        switch (m)
        {
        case TextureFilterMode::Nearest:
            return GL_NEAREST;
        case TextureFilterMode::Linear:
            return GL_LINEAR;
        case TextureFilterMode::Mipmap:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            return GL_LINEAR;
        }
    }

    GLint ToGLTextureMagFilterParam(TextureFilterMode m)
    {
        static_assert(NumOptions<TextureFilterMode>() == 3);

        switch (m)
        {
        case TextureFilterMode::Nearest:
            return GL_NEAREST;
        case TextureFilterMode::Linear:
        case TextureFilterMode::Mipmap:
        default:
            return GL_LINEAR;
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
    // the OpenGL data associated with an Texture2D
    struct CubemapOpenGLData final {
        gl::TextureCubemap texture;
        UID dataVersion;
        UID parametersVersion;
    };
}

class osc::Cubemap::Impl final {
public:
    Impl(int32_t width, TextureFormat format) :
        m_Width{width},
        m_Format{format}
    {
        OSC_ASSERT(m_Width > 0 && "the width of a cubemap must be a positive number");

        size_t const numPixelsPerFace = static_cast<size_t>(m_Width*m_Width)*NumBytesPerPixel(m_Format);
        m_Data.resize(NumOptions<CubemapFace>() * numPixelsPerFace);
    }

    int32_t getWidth() const
    {
        return m_Width;
    }

    TextureFormat getTextureFormat() const
    {
        return m_Format;
    }

    TextureWrapMode getWrapMode() const
    {
        return m_WrapModeU;
    }

    void setWrapMode(TextureWrapMode wm)
    {
        m_WrapModeU = wm;
        m_WrapModeV = wm;
        m_WrapModeW = wm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeU() const
    {
        return m_WrapModeU;
    }

    void setWrapModeU(TextureWrapMode wm)
    {
        m_WrapModeU = wm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeV() const
    {
        return m_WrapModeV;
    }

    void setWrapModeV(TextureWrapMode wm)
    {
        m_WrapModeV = wm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode getWrapModeW() const
    {
        return m_WrapModeW;
    }

    void setWrapModeW(TextureWrapMode wm)
    {
        m_WrapModeW = wm;
        m_TextureParamsVersion.reset();
    }

    TextureFilterMode getFilterMode() const
    {
        return m_FilterMode;
    }

    void setFilterMode(TextureFilterMode fm)
    {
        m_FilterMode = fm;
        m_TextureParamsVersion.reset();
    }

    void setPixelData(CubemapFace face, std::span<uint8_t const> data)
    {
        size_t const faceIndex = ToIndex(face);
        auto const numPixels = static_cast<size_t>(m_Width) * static_cast<size_t>(m_Width);
        size_t const numBytesPerCubeFace = numPixels * NumBytesPerPixel(m_Format);
        size_t const destinationDataStart = faceIndex * numBytesPerCubeFace;
        size_t const destinationDataEnd = destinationDataStart + numBytesPerCubeFace;

        OSC_ASSERT(faceIndex < NumOptions<CubemapFace>() && "invalid cubemap face passed to Cubemap::setPixelData");
        OSC_ASSERT(data.size() == numBytesPerCubeFace && "incorrect amount of data passed to Cubemap::setPixelData: the data must match the dimensions and texture format of the cubemap");
        OSC_ASSERT(destinationDataEnd <= m_Data.size() && "out of range assignment detected: this should be handled in the constructor");

        std::copy(data.begin(), data.end(), m_Data.begin() + destinationDataStart);
        m_DataVersion.reset();
    }

    gl::TextureCubemap& updCubemap()
    {
        if (!*m_MaybeGPUTexture) {
            *m_MaybeGPUTexture = CubemapOpenGLData{};
        }

        CubemapOpenGLData& buf = **m_MaybeGPUTexture;

        if (buf.dataVersion != m_DataVersion) {
            uploadPixelData(buf);
        }

        if (buf.parametersVersion != m_TextureParamsVersion) {
            updateTextureParameters(buf);
        }

        return buf.texture;
    }
private:
    void uploadPixelData(CubemapOpenGLData& buf)
    {
        // calculate CPU-to-GPU data transfer parameters
        size_t const numBytesPerPixel = NumBytesPerPixel(m_Format);
        size_t const numBytesPerRow = m_Width * numBytesPerPixel;
        size_t const numBytesPerFace = m_Width * numBytesPerRow;
        size_t const numBytesInCubemap = NumOptions<CubemapFace>() * numBytesPerFace;
        CPUDataType const cpuDataType = ToEquivalentCPUDataType(m_Format);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        CPUImageFormat const cpuChannelLayout = ToEquivalentCPUImageFormat(m_Format);  // TextureFormat's layout == CPU formats's layout for cubemaps
        GLint const unpackAlignment = ToOpenGLUnpackAlignment(m_Format);

        // sanity-check before doing anything with OpenGL
        OSC_ASSERT(numBytesPerRow % unpackAlignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(IsAlignedAtLeast(m_Data.data(), unpackAlignment) && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(numBytesInCubemap <= m_Data.size() && "the number of bytes in the cubemap (CPU-side) is less than expected: this is a developer bug");
        static_assert(NumOptions<TextureFormat>() == 7, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");

        // upload cubemap to GPU
        static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);
        gl::BindTexture(buf.texture);
        gl::PixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        for (GLint faceIdx = 0; faceIdx < static_cast<GLint>(NumOptions<CubemapFace>()); ++faceIdx)
        {
            size_t const faceBytesBegin = faceIdx * numBytesPerFace;

            gl::TexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx,
                0,
                ToOpenGLInternalFormat(m_Format, ColorSpace::sRGB),  // cubemaps are always sRGB
                m_Width,
                m_Width,
                0,
                ToOpenGLFormat(cpuChannelLayout),
                ToOpenGLDataType(cpuDataType),
                m_Data.data() + faceBytesBegin
            );
        }

        // generate mips (care: they can be uploaded to with Graphics::CopyTexture)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        gl::BindTexture();

        buf.dataVersion = m_DataVersion;
    }

    void updateTextureParameters(CubemapOpenGLData& buf)
    {
        gl::BindTexture(buf.texture);

        // set texture parameters
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, ToGLTextureMagFilterParam(m_FilterMode));
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, ToGLTextureMinFilterParam(m_FilterMode));
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, ToGLTextureTextureWrapParam(m_WrapModeU));
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, ToGLTextureTextureWrapParam(m_WrapModeV));
        gl::TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, ToGLTextureTextureWrapParam(m_WrapModeW));

        // cleanup OpenGL binding state
        gl::BindTexture();

        buf.parametersVersion = m_TextureParamsVersion;
    }

    int32_t m_Width;
    TextureFormat m_Format;
    std::vector<uint8_t> m_Data;
    UID m_DataVersion;

    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Mipmap;
    UID m_TextureParamsVersion;

    DefaultConstructOnCopy<std::optional<CubemapOpenGLData>> m_MaybeGPUTexture;
};

osc::Cubemap::Cubemap(int32_t width, TextureFormat format) :
    m_Impl{make_cow<Impl>(width, format)}
{
}

int32_t osc::Cubemap::getWidth() const
{
    return m_Impl->getWidth();
}

TextureWrapMode osc::Cubemap::getWrapMode() const
{
    return m_Impl->getWrapMode();
}

void osc::Cubemap::setWrapMode(TextureWrapMode wm)
{
    m_Impl.upd()->setWrapMode(wm);
}

TextureWrapMode osc::Cubemap::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::Cubemap::setWrapModeU(TextureWrapMode wm)
{
    m_Impl.upd()->setWrapModeU(wm);
}

TextureWrapMode osc::Cubemap::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::Cubemap::setWrapModeV(TextureWrapMode wm)
{
    m_Impl.upd()->setWrapModeV(wm);
}

TextureWrapMode osc::Cubemap::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::Cubemap::setWrapModeW(TextureWrapMode wm)
{
    m_Impl.upd()->setWrapModeW(wm);
}

TextureFilterMode osc::Cubemap::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::Cubemap::setFilterMode(TextureFilterMode fm)
{
    m_Impl.upd()->setFilterMode(fm);
}

TextureFormat osc::Cubemap::getTextureFormat() const
{
    return m_Impl->getTextureFormat();
}

void osc::Cubemap::setPixelData(CubemapFace face, std::span<uint8_t const> channelsRowByRow)
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
    std::vector<Color> ReadPixelDataAsColor(
        std::span<uint8_t const> pixelData,
        TextureFormat pixelDataFormat)
    {
        TextureChannelFormat const channelFormat = ChannelFormat(pixelDataFormat);

        size_t const numChannels = NumChannels(pixelDataFormat);
        size_t const bytesPerChannel = NumBytesPerChannel(channelFormat);
        size_t const bytesPerPixel = bytesPerChannel * numChannels;
        size_t const numPixels = pixelData.size() / bytesPerPixel;

        OSC_ASSERT(pixelData.size() % bytesPerPixel == 0);

        std::vector<Color> rv;
        rv.reserve(numPixels);

        static_assert(NumOptions<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8)
        {
            // unpack 8-bit channel bytes into floating-point Color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel)
            {
                size_t const pixelStart = bytesPerPixel * pixel;

                Color color = Color::black();
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    size_t const channelStart = pixelStart + channel;
                    color[channel] = Unorm8{pixelData[channelStart]}.normalized_value();
                }
                rv.push_back(color);
            }
        }
        else if (channelFormat == TextureChannelFormat::Float32 && bytesPerChannel == sizeof(float))
        {
            // read 32-bit channel floats into Color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel)
            {
                size_t const pixelStart = bytesPerPixel * pixel;

                Color color = Color::black();
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    size_t const channelStart = pixelStart + channel*bytesPerChannel;

                    std::span<uint8_t const> src{pixelData.data() + channelStart, sizeof(float)};
                    std::array<uint8_t, sizeof(float)> dest{};
                    std::copy(src.begin(), src.end(), dest.begin());

                    color[channel] = cpp20::bit_cast<float>(dest);
                }
                rv.push_back(color);
            }
        }
        else
        {
            OSC_ASSERT(false && "unsupported texture channel format or bytes per channel detected");
        }

        return rv;
    }

    std::vector<Color32> ReadPixelDataAsColor32(
        std::span<uint8_t const> pixelData,
        TextureFormat pixelDataFormat)
    {
        TextureChannelFormat const channelFormat = ChannelFormat(pixelDataFormat);

        size_t const numChannels = NumChannels(pixelDataFormat);
        size_t const bytesPerChannel = NumBytesPerChannel(channelFormat);
        size_t const bytesPerPixel = bytesPerChannel * numChannels;
        size_t const numPixels = pixelData.size() / bytesPerPixel;

        std::vector<Color32> rv;
        rv.reserve(numPixels);

        static_assert(NumOptions<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8)
        {
            // read 8-bit channel bytes into 8-bit Color32 color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel)
            {
                size_t const pixelStart = bytesPerPixel * pixel;

                Color32 color = {0x00, 0x00, 0x00, 0xff};
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    size_t const channelStart = pixelStart + channel;
                    color[channel] = pixelData[channelStart];
                }
                rv.push_back(color);
            }
        }
        else
        {
            static_assert(std::is_same_v<Color::value_type, float>);
            OSC_ASSERT(bytesPerChannel == sizeof(float));

            // pack 32-bit channel floats into 8-bit Color32 color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel)
            {
                size_t const pixelStart = bytesPerPixel * pixel;

                Color32 color = {0x00, 0x00, 0x00, 0xff};
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    size_t const channelStart = pixelStart + channel*sizeof(float);

                    std::span<uint8_t const> src{pixelData.data() + channelStart, sizeof(float)};
                    std::array<uint8_t, sizeof(float)> dest{};
                    std::copy(src.begin(), src.end(), dest.begin());
                    auto const channelFloat = cpp20::bit_cast<float>(dest);

                    color[channel] = Unorm8{channelFloat};
                }
                rv.push_back(color);
            }
        }

        return rv;
    }

    void EncodePixelsInDesiredFormat(
        std::span<Color const> pixels,
        TextureFormat pixelDataFormat,
        std::vector<uint8_t>& pixelData)
    {
        TextureChannelFormat const channelFormat = ChannelFormat(pixelDataFormat);

        size_t const numChannels = NumChannels(pixelDataFormat);
        size_t const bytesPerChannel = NumBytesPerChannel(channelFormat);
        size_t const bytesPerPixel = bytesPerChannel * numChannels;
        size_t const numPixels = pixels.size();
        size_t const numOutputBytes = bytesPerPixel * numPixels;

        pixelData.clear();
        pixelData.reserve(numOutputBytes);

        OSC_ASSERT(numChannels <= std::tuple_size_v<Color>);
        static_assert(NumOptions<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8)
        {
            // clamp pixels, convert them to bytes, add them to pixel data buffer
            for (Color const& pixel : pixels)
            {
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    pixelData.push_back(Unorm8{pixel[channel]}.raw_value());
                }
            }
        }
        else
        {
            // write pixels to pixel data buffer as-is (they're floats already)
            for (Color const& pixel : pixels)
            {
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    PushAsBytes(pixel[channel], pixelData);
                }
            }
        }
    }

    void EncodePixels32InDesiredFormat(
        std::span<Color32 const> pixels,
        TextureFormat pixelDataFormat,
        std::vector<uint8_t>& pixelData)
    {
        TextureChannelFormat const channelFormat = ChannelFormat(pixelDataFormat);

        size_t const numChannels = NumChannels(pixelDataFormat);
        size_t const bytesPerChannel = NumBytesPerChannel(channelFormat);
        size_t const bytesPerPixel = bytesPerChannel * numChannels;
        size_t const numPixels = pixels.size();
        size_t const numOutputBytes = bytesPerPixel * numPixels;

        pixelData.clear();
        pixelData.reserve(numOutputBytes);

        OSC_ASSERT(numChannels <= Color32::length());
        static_assert(NumOptions<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8)
        {
            // write pixels to pixel data buffer as-is (they're bytes already)
            for (Color32 const& pixel : pixels)
            {
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    pixelData.push_back(pixel[channel].raw_value());
                }
            }
        }
        else
        {
            // upscale pixels to float32s and write the floats to the pixel buffer
            for (Color32 const& pixel : pixels)
            {
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    float const pixelFloatVal = Unorm8{pixel[channel]}.normalized_value();
                    PushAsBytes(pixelFloatVal, pixelData);
                }
            }
        }
    }
}

class osc::Texture2D::Impl final {
public:
    Impl(
        Vec2i dimensions,
        TextureFormat format,
        ColorSpace colorSpace,
        TextureWrapMode wrapMode,
        TextureFilterMode filterMode) :

        m_Dimensions{dimensions},
        m_Format{format},
        m_ColorSpace{colorSpace},
        m_WrapModeU{wrapMode},
        m_WrapModeV{wrapMode},
        m_WrapModeW{wrapMode},
        m_FilterMode{filterMode}
    {
        OSC_ASSERT(m_Dimensions.x > 0 && m_Dimensions.y > 0);
    }

    Vec2i getDimensions() const
    {
        return m_Dimensions;
    }

    TextureFormat getTextureFormat() const
    {
        return m_Format;
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

    std::vector<Color> getPixels() const
    {
        return ReadPixelDataAsColor(m_PixelData, m_Format);
    }

    void setPixels(std::span<Color const> pixels)
    {
        OSC_ASSERT(std::ssize(pixels) == static_cast<ptrdiff_t>(m_Dimensions.x*m_Dimensions.y));
        EncodePixelsInDesiredFormat(pixels, m_Format, m_PixelData);
    }

    std::vector<Color32> getPixels32() const
    {
        return ReadPixelDataAsColor32(m_PixelData, m_Format);
    }

    void setPixels32(std::span<Color32 const> pixels)
    {
        OSC_ASSERT(std::ssize(pixels) == static_cast<ptrdiff_t>(m_Dimensions.x*m_Dimensions.y));
        EncodePixels32InDesiredFormat(pixels, m_Format, m_PixelData);
    }

    std::span<uint8_t const> getPixelData() const
    {
        return m_PixelData;
    }

    void setPixelData(std::span<uint8_t const> pixelData)
    {
        OSC_ASSERT(pixelData.size() == NumBytesPerPixel(m_Format)*m_Dimensions.x*m_Dimensions.y && "incorrect number of bytes passed to Texture2D::setPixelData");
        OSC_ASSERT(pixelData.size() == m_PixelData.size());

        std::copy(pixelData.begin(), pixelData.end(), m_PixelData.begin());
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

        static_assert(NumOptions<TextureFormat>() == 7, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");
        OSC_ASSERT(numBytesPerRow % unpackAlignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(IsAlignedAtLeast(m_PixelData.data(), unpackAlignment) && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");

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
        glGenerateMipmap(GL_TEXTURE_2D);
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

    Vec2i m_Dimensions;
    TextureFormat m_Format;
    ColorSpace m_ColorSpace;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Nearest;
    std::vector<uint8_t> m_PixelData = std::vector<uint8_t>(NumBytesPerPixel(m_Format) * m_Dimensions.x * m_Dimensions.y, 0xff);
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

size_t osc::NumChannels(TextureFormat format)
{
    constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
    {
        return std::to_array({ TextureFormatTraits<Formats>::num_channels... });
    }(TextureFormatList{});

    return lut.at(ToIndex(format));
}

TextureChannelFormat osc::ChannelFormat(TextureFormat f)
{
    constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
    {
        return std::to_array({ TextureFormatTraits<Formats>::channel_format... });
    }(TextureFormatList{});

    return lut.at(ToIndex(f));
}

size_t osc::NumBytesPerPixel(TextureFormat format)
{
    return NumChannels(format) * NumBytesPerChannel(ChannelFormat(format));
}

std::optional<TextureFormat> osc::ToTextureFormat(size_t numChannels, TextureChannelFormat channelFormat)
{
    static_assert(NumOptions<TextureChannelFormat>() == 2);
    bool const isByteOriented = channelFormat == TextureChannelFormat::Uint8;

    static_assert(NumOptions<TextureFormat>() == 7);
    switch (numChannels) {
    case 1:
        return isByteOriented ? TextureFormat::R8 : std::optional<TextureFormat>{};
    case 2:
        return isByteOriented ? TextureFormat::RG16 : TextureFormat::RGFloat;
    case 3:
        return isByteOriented ? TextureFormat::RGB24 : TextureFormat::RGBFloat;
    case 4:
        return isByteOriented ? TextureFormat::RGBA32 : TextureFormat::RGBAFloat;
    default:
        return std::nullopt;
    }
}

size_t osc::NumBytesPerChannel(TextureChannelFormat f)
{
    static_assert(NumOptions<TextureChannelFormat>() == 2);
    switch (f)
    {
    case TextureChannelFormat::Uint8: return 1;
    case TextureChannelFormat::Float32: return 4;
    default: return 1;
    }
}

osc::Texture2D::Texture2D(
    Vec2i dimensions,
    TextureFormat format,
    ColorSpace colorSpace,
    TextureWrapMode wrapMode,
    TextureFilterMode filterMode) :

    m_Impl{make_cow<Impl>(dimensions, format, colorSpace, wrapMode, filterMode)}
{
}


Vec2i osc::Texture2D::getDimensions() const
{
    return m_Impl->getDimensions();
}

TextureFormat osc::Texture2D::getTextureFormat() const
{
    return m_Impl->getTextureFormat();
}

ColorSpace osc::Texture2D::getColorSpace() const
{
    return m_Impl->getColorSpace();
}

TextureWrapMode osc::Texture2D::getWrapMode() const
{
    return m_Impl->getWrapMode();
}

void osc::Texture2D::setWrapMode(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapMode(twm);
}

TextureWrapMode osc::Texture2D::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::Texture2D::setWrapModeU(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapModeU(twm);
}

TextureWrapMode osc::Texture2D::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::Texture2D::setWrapModeV(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapModeV(twm);
}

TextureWrapMode osc::Texture2D::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::Texture2D::setWrapModeW(TextureWrapMode twm)
{
    m_Impl.upd()->setWrapModeW(twm);
}

TextureFilterMode osc::Texture2D::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::Texture2D::setFilterMode(TextureFilterMode twm)
{
    m_Impl.upd()->setFilterMode(twm);
}

std::vector<Color> osc::Texture2D::getPixels() const
{
    return m_Impl->getPixels();
}

void osc::Texture2D::setPixels(std::span<Color const> pixels)
{
    m_Impl.upd()->setPixels(pixels);
}

std::vector<Color32> osc::Texture2D::getPixels32() const
{
    return m_Impl->getPixels32();
}

void osc::Texture2D::setPixels32(std::span<Color32 const> pixels)
{
    m_Impl.upd()->setPixels32(pixels);
}

std::span<uint8_t const> osc::Texture2D::getPixelData() const
{
    return m_Impl->getPixelData();
}

void osc::Texture2D::setPixelData(std::span<uint8_t const> pixelData)
{
    m_Impl.upd()->setPixelData(pixelData);
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
    constexpr auto c_RenderTextureFormatStrings = std::to_array<CStringView>({
        "Red8",
        "ARGB32",

        "RGFloat16",
        "RGBFloat16",
        "ARGBFloat16",

        "Depth",
    });
    static_assert(c_RenderTextureFormatStrings.size() == NumOptions<RenderTextureFormat>());

    constexpr auto c_DepthStencilFormatStrings = std::to_array<CStringView>({
        "D24_UNorm_S8_UInt",
    });
    static_assert(c_DepthStencilFormatStrings.size() == NumOptions<DepthStencilFormat>());

    GLenum ToInternalOpenGLColorFormat(
        RenderBufferType type,
        RenderTextureDescriptor const& desc)
    {
        static_assert(NumOptions<RenderBufferType>() == 2, "review code below, which treats RenderBufferType as a bool");
        if (type == RenderBufferType::Depth)
        {
            return GL_DEPTH24_STENCIL8;
        }
        else
        {
            static_assert(NumOptions<RenderTextureFormat>() == 6);
            static_assert(NumOptions<RenderTextureReadWrite>() == 2);

            switch (desc.getColorFormat()) {
            case RenderTextureFormat::Red8:
                return GL_RED;
            default:
            case RenderTextureFormat::ARGB32:
                return desc.getReadWrite() == RenderTextureReadWrite::sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            case RenderTextureFormat::RGFloat16:
                return GL_RG16F;
            case RenderTextureFormat::RGBFloat16:
                return GL_RGB16F;
            case RenderTextureFormat::ARGBFloat16:
                return GL_RGBA16F;
            case RenderTextureFormat::Depth:
                return GL_R32F;
            }
        }
    }

    constexpr CPUImageFormat ToEquivalentCPUImageFormat(
        RenderBufferType type,
        RenderTextureDescriptor const& desc)
    {
        static_assert(NumOptions<RenderBufferType>() == 2);
        static_assert(NumOptions<DepthStencilFormat>() == 1);
        static_assert(NumOptions<RenderTextureFormat>() == 6);
        static_assert(NumOptions<CPUImageFormat>() == 5);

        if (type == RenderBufferType::Depth)
        {
            return CPUImageFormat::DepthStencil;
        }
        else
        {
            switch (desc.getColorFormat()) {
            case RenderTextureFormat::Red8:
                return CPUImageFormat::R8;
            default:
            case RenderTextureFormat::ARGB32:
                return CPUImageFormat::RGBA;
            case RenderTextureFormat::RGFloat16:
                return CPUImageFormat::RG;
            case RenderTextureFormat::RGBFloat16:
                return CPUImageFormat::RGB;
            case RenderTextureFormat::ARGBFloat16:
                return CPUImageFormat::RGBA;
            case RenderTextureFormat::Depth:
                return CPUImageFormat::R8;
            }
        }
    }

    constexpr CPUDataType ToEquivalentCPUDataType(
        RenderBufferType type,
        RenderTextureDescriptor const& desc)
    {
        static_assert(NumOptions<RenderBufferType>() == 2);
        static_assert(NumOptions<DepthStencilFormat>() == 1);
        static_assert(NumOptions<RenderTextureFormat>() == 6);
        static_assert(NumOptions<CPUDataType>() == 4);

        if (type == RenderBufferType::Depth)
        {
            return CPUDataType::UnsignedInt24_8;
        }
        else
        {
            switch (desc.getColorFormat()) {
            case RenderTextureFormat::Red8:
                return CPUDataType::UnsignedByte;
            default:
            case RenderTextureFormat::ARGB32:
                return CPUDataType::UnsignedByte;
            case RenderTextureFormat::RGFloat16:
                return CPUDataType::HalfFloat;
            case RenderTextureFormat::RGBFloat16:
                return CPUDataType::HalfFloat;
            case RenderTextureFormat::ARGBFloat16:
                return CPUDataType::HalfFloat;
            case RenderTextureFormat::Depth:
                return CPUDataType::Float;
            }
        }
    }

    constexpr GLenum ToImageColorFormat(TextureFormat f)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::image_color_format... });
        }(TextureFormatList{});

        return lut.at(ToIndex(f));
    }

    constexpr GLint ToImagePixelPackAlignment(TextureFormat f)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::pixel_pack_alignment... });
        }(TextureFormatList{});

        return lut.at(ToIndex(f));
    }

    constexpr GLenum ToImageDataType(TextureFormat)
    {
        static_assert(NumOptions<TextureFormat>() == 7);
        return GL_UNSIGNED_BYTE;
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

osc::RenderTextureDescriptor::RenderTextureDescriptor(Vec2i dimensions) :
    m_Dimensions{elementwise_max(dimensions, Vec2i{0, 0})},
    m_Dimension{TextureDimensionality::Tex2D},
    m_AnialiasingLevel{1},
    m_ColorFormat{RenderTextureFormat::ARGB32},
    m_DepthStencilFormat{DepthStencilFormat::D24_UNorm_S8_UInt},
    m_ReadWrite{RenderTextureReadWrite::Default}
{
}

Vec2i osc::RenderTextureDescriptor::getDimensions() const
{
    return m_Dimensions;
}

void osc::RenderTextureDescriptor::setDimensions(Vec2i d)
{
    OSC_ASSERT(d.x >= 0 && d.y >= 0);
    m_Dimensions = d;
}

TextureDimensionality osc::RenderTextureDescriptor::getDimensionality() const
{
    return m_Dimension;
}

void osc::RenderTextureDescriptor::setDimensionality(TextureDimensionality newDimension)
{
    m_Dimension = newDimension;
}

AntiAliasingLevel osc::RenderTextureDescriptor::getAntialiasingLevel() const
{
    return m_AnialiasingLevel;
}

void osc::RenderTextureDescriptor::setAntialiasingLevel(AntiAliasingLevel level)
{
    m_AnialiasingLevel = level;
}

RenderTextureFormat osc::RenderTextureDescriptor::getColorFormat() const
{
    return m_ColorFormat;
}

void osc::RenderTextureDescriptor::setColorFormat(RenderTextureFormat f)
{
    m_ColorFormat = f;
}

DepthStencilFormat osc::RenderTextureDescriptor::getDepthStencilFormat() const
{
    return m_DepthStencilFormat;
}

void osc::RenderTextureDescriptor::setDepthStencilFormat(DepthStencilFormat f)
{
    m_DepthStencilFormat = f;
}

RenderTextureReadWrite osc::RenderTextureDescriptor::getReadWrite() const
{
    return m_ReadWrite;
}

void osc::RenderTextureDescriptor::setReadWrite(RenderTextureReadWrite rw)
{
    m_ReadWrite = rw;
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
        OSC_ASSERT((getDimensionality() != TextureDimensionality::Cube || getDimensions().x == getDimensions().y) && "cannot construct a Cube renderbuffer with non-square dimensions");
        OSC_ASSERT((getDimensionality() != TextureDimensionality::Cube || getAntialiasingLevel() == AntiAliasingLevel::none()) && "cannot construct a Cube renderbuffer that is anti-aliased (not supported by backends like OpenGL)");
    }

    void reformat(RenderTextureDescriptor const& newDescriptor)
    {
        OSC_ASSERT((newDescriptor.getDimensionality() != TextureDimensionality::Cube || newDescriptor.getDimensions().x == newDescriptor.getDimensions().y) && "cannot reformat a render buffer to a Cube dimensionality with non-square dimensions");
        OSC_ASSERT((newDescriptor.getDimensionality() != TextureDimensionality::Cube || newDescriptor.getAntialiasingLevel() == AntiAliasingLevel::none()) && "cannot reformat a renderbuffer to a Cube dimensionality with is anti-aliased (not supported by backends like OpenGL)");

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

    Vec2i getDimensions() const
    {
        return m_Descriptor.getDimensions();
    }

    void setDimensions(Vec2i newDims)
    {
        OSC_ASSERT((getDimensionality() != TextureDimensionality::Cube || newDims.x == newDims.y) && "cannot set a cubemap to have non-square dimensions");

        if (newDims != getDimensions())
        {
            m_Descriptor.setDimensions(newDims);
            m_MaybeOpenGLData->reset();
        }
    }

    TextureDimensionality getDimensionality() const
    {
        return m_Descriptor.getDimensionality();
    }

    void setDimensionality(TextureDimensionality newDimension)
    {
        OSC_ASSERT((newDimension != TextureDimensionality::Cube || getDimensions().x == getDimensions().y) && "cannot set dimensionality to Cube for non-square render buffer");
        OSC_ASSERT((newDimension != TextureDimensionality::Cube || getAntialiasingLevel() == AntiAliasingLevel{1}) && "cannot set dimensionality to Cube for an anti-aliased render buffer (not supported by backends like OpenGL)");

        if (newDimension != getDimensionality())
        {
            m_Descriptor.setDimensionality(newDimension);
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

    AntiAliasingLevel getAntialiasingLevel() const
    {
        return m_Descriptor.getAntialiasingLevel();
    }

    void setAntialiasingLevel(AntiAliasingLevel newLevel)
    {
        OSC_ASSERT((getDimensionality() != TextureDimensionality::Cube || newLevel == AntiAliasingLevel{1}) && "cannot set anti-aliasing level >1 on a cube render buffer (it is not supported by backends like OpenGL)");

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

    RenderBufferOpenGLData& updRenderBufferData()
    {
        if (!*m_MaybeOpenGLData)
        {
            uploadToGPU();
        }
        return **m_MaybeOpenGLData;
    }

    void uploadToGPU()
    {
        // dispatch _which_ texture handles are created based on render buffer params

        static_assert(NumOptions<TextureDimensionality>() == 2);
        if (getDimensionality() == TextureDimensionality::Tex2D)
        {
            if (m_Descriptor.getAntialiasingLevel() <= AntiAliasingLevel{1})
            {
                auto& t = std::get<SingleSampledTexture>((*m_MaybeOpenGLData).emplace(SingleSampledTexture{}));
                configureData(t);
            }
            else
            {
                auto& t = std::get<MultisampledRBOAndResolvedTexture>((*m_MaybeOpenGLData).emplace(MultisampledRBOAndResolvedTexture{}));
                configureData(t);
            }
        }
        else
        {
            auto& t = std::get<SingleSampledCubemap>((*m_MaybeOpenGLData).emplace(SingleSampledCubemap{}));
            configureData(t);
        }
    }

    void configureData(SingleSampledTexture& t)
    {
        Vec2i const dimensions = m_Descriptor.getDimensions();

        // setup resolved texture
        gl::BindTexture(t.texture2D);
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

    void configureData(MultisampledRBOAndResolvedTexture& data)
    {
        Vec2i const dimensions = m_Descriptor.getDimensions();

        // setup multisampled RBO
        gl::BindRenderBuffer(data.multisampledRBO);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            m_Descriptor.getAntialiasingLevel().getU32(),
            ToInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
            dimensions.x,
            dimensions.y
        );
        gl::BindRenderBuffer();

        // setup resolved texture
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

    void configureData(SingleSampledCubemap& t)
    {
        Vec2i const dimensions = m_Descriptor.getDimensions();

        // setup resolved texture
        gl::BindTexture(t.textureCubemap);
        for (int i = 0; i < 6; ++i)
        {
            gl::TexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                ToInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
                dimensions.x,
                dimensions.y,
                0,
                ToOpenGLFormat(ToEquivalentCPUImageFormat(m_BufferType, m_Descriptor)),
                ToOpenGLDataType(ToEquivalentCPUDataType(m_BufferType, m_Descriptor)),
                nullptr
            );
        }
        gl::TexParameteri(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );
        gl::TexParameteri(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );
        gl::TexParameteri(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );
        gl::TexParameteri(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );
        gl::TexParameteri(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE
        );
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
    Impl() : Impl{Vec2i{1, 1}}
    {
    }

    explicit Impl(Vec2i dimensions) :
        Impl{RenderTextureDescriptor{dimensions}}
    {
    }

    explicit Impl(RenderTextureDescriptor const& descriptor) :
        m_ColorBuffer{std::make_shared<RenderBuffer>(descriptor, RenderBufferType::Color)},
        m_DepthBuffer{std::make_shared<RenderBuffer>(descriptor, RenderBufferType::Depth)}
    {
    }

    Vec2i getDimensions() const
    {
        return m_ColorBuffer->m_Impl->getDimensions();
    }

    void setDimensions(Vec2i newDims)
    {
        if (newDims != getDimensions())
        {
            m_ColorBuffer->m_Impl->setDimensions(newDims);
            m_DepthBuffer->m_Impl->setDimensions(newDims);
        }
    }

    TextureDimensionality getDimensionality() const
    {
        return m_ColorBuffer->m_Impl->getDimensionality();
    }

    void setDimensionality(TextureDimensionality newDimension)
    {
        if (newDimension != getDimensionality())
        {
            m_ColorBuffer->m_Impl->setDimensionality(newDimension);
            m_DepthBuffer->m_Impl->setDimensionality(newDimension);
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

    AntiAliasingLevel getAntialiasingLevel() const
    {
        return m_ColorBuffer->m_Impl->getAntialiasingLevel();
    }

    void setAntialiasingLevel(AntiAliasingLevel newLevel)
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

    RenderBufferOpenGLData& getColorRenderBufferData()
    {
        return m_ColorBuffer->m_Impl->updRenderBufferData();
    }

    RenderBufferOpenGLData& getDepthStencilRenderBufferData()
    {
        return m_DepthBuffer->m_Impl->updRenderBufferData();
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

osc::RenderTexture::RenderTexture(Vec2i dimensions) :
    m_Impl{make_cow<Impl>(dimensions)}
{
}

osc::RenderTexture::RenderTexture(RenderTextureDescriptor const& desc) :
    m_Impl{make_cow<Impl>(desc)}
{
}

Vec2i osc::RenderTexture::getDimensions() const
{
    return m_Impl->getDimensions();
}

void osc::RenderTexture::setDimensions(Vec2i d)
{
    m_Impl.upd()->setDimensions(d);
}

TextureDimensionality osc::RenderTexture::getDimensionality() const
{
    return m_Impl->getDimensionality();
}

void osc::RenderTexture::setDimensionality(TextureDimensionality newDimension)
{
    m_Impl.upd()->setDimensionality(newDimension);
}

RenderTextureFormat osc::RenderTexture::getColorFormat() const
{
    return m_Impl->getColorFormat();
}

void osc::RenderTexture::setColorFormat(RenderTextureFormat format)
{
    m_Impl.upd()->setColorFormat(format);
}

AntiAliasingLevel osc::RenderTexture::getAntialiasingLevel() const
{
    return m_Impl->getAntialiasingLevel();
}

void osc::RenderTexture::setAntialiasingLevel(AntiAliasingLevel level)
{
    m_Impl.upd()->setAntialiasingLevel(level);
}

DepthStencilFormat osc::RenderTexture::getDepthStencilFormat() const
{
    return m_Impl->getDepthStencilFormat();
}

void osc::RenderTexture::setDepthStencilFormat(DepthStencilFormat format)
{
    m_Impl.upd()->setDepthStencilFormat(format);
}

RenderTextureReadWrite osc::RenderTexture::getReadWrite() const
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

std::shared_ptr<RenderBuffer> osc::RenderTexture::updColorBuffer()
{
    return m_Impl.upd()->updColorBuffer();
}

std::shared_ptr<RenderBuffer> osc::RenderTexture::updDepthBuffer()
{
    return m_Impl.upd()->updDepthBuffer();
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

    std::optional<ptrdiff_t> findPropertyIndex(std::string_view propertyName) const
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

    std::string_view getPropertyName(ptrdiff_t i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->first;
    }

    ShaderPropertyType getPropertyType(ptrdiff_t i) const
    {
        auto it = m_Uniforms.begin();
        std::advance(it, i);
        return it->second.shaderType;
    }

    // non-PIMPL APIs

    gl::Program const& getProgram() const
    {
        return m_Program;
    }

    FastStringHashtable<ShaderElement> const& getUniforms() const
    {
        return m_Uniforms;
    }

    FastStringHashtable<ShaderElement> const& getAttributes() const
    {
        return m_Attributes;
    }

private:
    void parseUniformsAndAttributesFromProgram()
    {
        constexpr GLsizei c_ShaderMaxNameLength = 128;

        GLint numAttrs = 0;
        glGetProgramiv(m_Program.get(), GL_ACTIVE_ATTRIBUTES, &numAttrs);

        GLint numUniforms = 0;
        glGetProgramiv(m_Program.get(), GL_ACTIVE_UNIFORMS, &numUniforms);

        m_Attributes.reserve(numAttrs);
        for (GLint i = 0; i < numAttrs; i++)
        {
            GLint size = 0; // size of the variable
            GLenum type = 0; // type of the variable (float, vec3 or mat4, etc)
            std::array<GLchar, c_ShaderMaxNameLength> name{}; // variable name in GLSL
            GLsizei length = 0; // name length
            glGetActiveAttrib(
                m_Program.get() ,
                static_cast<GLuint>(i),
                static_cast<GLsizei>(name.size()),
                &length,
                &size,
                &type,
                name.data()
            );

            static_assert(sizeof(GLint) <= sizeof(int32_t));
            m_Attributes.try_emplace<std::string>(
                NormalizeShaderElementName(name.data()),
                static_cast<int32_t>(glGetAttribLocation(m_Program.get(), name.data())),
                GLShaderTypeToShaderTypeInternal(type),
                static_cast<int32_t>(size)
            );
        }

        m_Uniforms.reserve(numUniforms);
        for (GLint i = 0; i < numUniforms; i++)
        {
            GLint size = 0; // size of the variable
            GLenum type = 0; // type of the variable (float, vec3 or mat4, etc)
            std::array<GLchar, c_ShaderMaxNameLength> name{}; // variable name in GLSL
            GLsizei length = 0; // name length
            glGetActiveUniform(
                m_Program.get(),
                static_cast<GLuint>(i),
                static_cast<GLsizei>(name.size()),
                &length,
                &size,
                &type,
                name.data()
            );

            static_assert(sizeof(GLint) <= sizeof(int32_t));
            m_Uniforms.try_emplace<std::string>(
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
    FastStringHashtable<ShaderElement> m_Uniforms;
    FastStringHashtable<ShaderElement> m_Attributes;
    std::optional<ShaderElement> m_MaybeModelMatUniform;
    std::optional<ShaderElement> m_MaybeNormalMatUniform;
    std::optional<ShaderElement> m_MaybeViewMatUniform;
    std::optional<ShaderElement> m_MaybeProjMatUniform;
    std::optional<ShaderElement> m_MaybeViewProjMatUniform;
    std::optional<ShaderElement> m_MaybeInstancedModelMatAttr;
    std::optional<ShaderElement> m_MaybeInstancedNormalMatAttr;
};


std::ostream& osc::operator<<(std::ostream& o, ShaderPropertyType shaderType)
{
    constexpr auto lut = []<ShaderPropertyType... Types>(OptionList<ShaderPropertyType, Types...>)
    {
        return std::to_array({ ShaderPropertyTypeTraits<Types>::name... });
    }(ShaderPropertyTypeList{});

    return o << lut.at(ToIndex(shaderType));
}

osc::Shader::Shader(CStringView vertexShader, CStringView fragmentShader) :
    m_Impl{make_cow<Impl>(vertexShader, fragmentShader)}
{
}

osc::Shader::Shader(CStringView vertexShader, CStringView geometryShader, CStringView fragmentShader) :
    m_Impl{make_cow<Impl>(vertexShader, geometryShader, fragmentShader)}
{
}

size_t osc::Shader::getPropertyCount() const
{
    return m_Impl->getPropertyCount();
}

std::optional<ptrdiff_t> osc::Shader::findPropertyIndex(std::string_view propertyName) const
{
    return m_Impl->findPropertyIndex(propertyName);
}

std::string_view osc::Shader::getPropertyName(ptrdiff_t propertyIndex) const
{
    return m_Impl->getPropertyName(propertyIndex);
}

ShaderPropertyType osc::Shader::getPropertyType(ptrdiff_t propertyIndex) const
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
    GLenum ToGLDepthFunc(DepthFunction f)
    {
        static_assert(NumOptions<DepthFunction>() == 2);

        switch (f) {
        case DepthFunction::LessOrEqual:
            return GL_LEQUAL;
        case DepthFunction::Less:
        default:
            return GL_LESS;
        }
    }

    GLenum ToGLCullFaceEnum(CullMode cullMode)
    {
        static_assert(NumOptions<CullMode>() == 3);

        switch (cullMode) {
        case CullMode::Front:
            return GL_FRONT;
        case CullMode::Back:
        default:
            return GL_BACK;
        }
    }
}

class osc::Material::Impl final {
public:
    explicit Impl(Shader shader) : m_Shader{std::move(shader)}
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

    std::optional<std::span<Color const>> getColorArray(std::string_view propertyName) const
    {
        return getValue<std::vector<Color>, std::span<Color const>>(propertyName);
    }

    void setColorArray(std::string_view propertyName, std::span<Color const> colors)
    {
        setValue<std::vector<Color>>(propertyName, std::vector<Color>(colors.begin(), colors.end()));
    }

    std::optional<float> getFloat(std::string_view propertyName) const
    {
        return getValue<float>(propertyName);
    }

    void setFloat(std::string_view propertyName, float value)
    {
        setValue(propertyName, value);
    }

    std::optional<std::span<float const>> getFloatArray(std::string_view propertyName) const
    {
        return getValue<std::vector<float>, std::span<float const>>(propertyName);
    }

    void setFloatArray(std::string_view propertyName, std::span<float const> v)
    {
        setValue<std::vector<float>>(propertyName, std::vector<float>(v.begin(), v.end()));
    }

    std::optional<Vec2> getVec2(std::string_view propertyName) const
    {
        return getValue<Vec2>(propertyName);
    }

    void setVec2(std::string_view propertyName, Vec2 value)
    {
        setValue(propertyName, value);
    }

    std::optional<Vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<Vec3>(propertyName);
    }

    void setVec3(std::string_view propertyName, Vec3 value)
    {
        setValue(propertyName, value);
    }

    std::optional<std::span<Vec3 const>> getVec3Array(std::string_view propertyName) const
    {
        return getValue<std::vector<Vec3>, std::span<Vec3 const>>(propertyName);
    }

    void setVec3Array(std::string_view propertyName, std::span<Vec3 const> value)
    {
        setValue(propertyName, std::vector<Vec3>(value.begin(), value.end()));
    }

    std::optional<Vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<Vec4>(propertyName);
    }

    void setVec4(std::string_view propertyName, Vec4 value)
    {
        setValue(propertyName, value);
    }

    std::optional<Mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<Mat3>(propertyName);
    }

    void setMat3(std::string_view propertyName, Mat3 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<Mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<Mat4>(propertyName);
    }

    void setMat4(std::string_view propertyName, Mat4 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<std::span<Mat4 const>> getMat4Array(std::string_view propertyName) const
    {
        return getValue<std::vector<Mat4>, std::span<Mat4 const>>(propertyName);
    }

    void setMat4Array(std::string_view propertyName, std::span<Mat4 const> mats)
    {
        setValue(propertyName, std::vector<Mat4>(mats.begin(), mats.end()));
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
        m_Values.erase(propertyName);
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
        m_Values.erase(propertyName);
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
        m_Values.erase(propertyName);
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

    CullMode getCullMode() const
    {
        return m_CullMode;
    }

    void setCullMode(CullMode newCullMode)
    {
        m_CullMode = newCullMode;
    }

private:
    template<typename T, typename TConverted = T>
    std::optional<TConverted> getValue(std::string_view propertyName) const
        requires std::convertible_to<T, TConverted>
    {
        auto const it = m_Values.find(propertyName);

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
        m_Values.insert_or_assign(propertyName, std::forward<T>(v));
    }

    friend class GraphicsBackend;

    Shader m_Shader;
    FastStringHashtable<MaterialValue> m_Values;
    bool m_IsTransparent = false;
    bool m_IsDepthTested = true;
    bool m_IsWireframeMode = false;
    DepthFunction m_DepthFunction = DepthFunction::Default;
    CullMode m_CullMode = CullMode::Default;
};

osc::Material::Material(Shader shader) :
    m_Impl{make_cow<Impl>(std::move(shader))}
{
}

Shader const& osc::Material::getShader() const
{
    return m_Impl->getShader();
}

std::optional<Color> osc::Material::getColor(std::string_view propertyName) const
{
    return m_Impl->getColor(propertyName);
}

void osc::Material::setColor(std::string_view propertyName, Color const& color)
{
    m_Impl.upd()->setColor(propertyName, color);
}

std::optional<std::span<Color const>> osc::Material::getColorArray(std::string_view propertyName) const
{
    return m_Impl->getColorArray(propertyName);
}

void osc::Material::setColorArray(std::string_view propertyName, std::span<osc::Color const> colors)
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

std::optional<std::span<float const>> osc::Material::getFloatArray(std::string_view propertyName) const
{
    return m_Impl->getFloatArray(propertyName);
}

void osc::Material::setFloatArray(std::string_view propertyName, std::span<float const> vs)
{
    m_Impl.upd()->setFloatArray(propertyName, vs);
}

std::optional<Vec2> osc::Material::getVec2(std::string_view propertyName) const
{
    return m_Impl->getVec2(propertyName);
}

void osc::Material::setVec2(std::string_view propertyName, Vec2 value)
{
    m_Impl.upd()->setVec2(propertyName, value);
}

std::optional<std::span<Vec3 const>> osc::Material::getVec3Array(std::string_view propertyName) const
{
    return m_Impl->getVec3Array(propertyName);
}

void osc::Material::setVec3Array(std::string_view propertyName, std::span<Vec3 const> vs)
{
    m_Impl.upd()->setVec3Array(propertyName, vs);
}

std::optional<Vec3> osc::Material::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(propertyName);
}

void osc::Material::setVec3(std::string_view propertyName, Vec3 value)
{
    m_Impl.upd()->setVec3(propertyName, value);
}

std::optional<Vec4> osc::Material::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(propertyName);
}

void osc::Material::setVec4(std::string_view propertyName, Vec4 value)
{
    m_Impl.upd()->setVec4(propertyName, value);
}

std::optional<Mat3> osc::Material::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(propertyName);
}

void osc::Material::setMat3(std::string_view propertyName, Mat3 const& mat)
{
    m_Impl.upd()->setMat3(propertyName, mat);
}

std::optional<Mat4> osc::Material::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(propertyName);
}

void osc::Material::setMat4(std::string_view propertyName, Mat4 const& mat)
{
    m_Impl.upd()->setMat4(propertyName, mat);
}

std::optional<std::span<Mat4 const>> osc::Material::getMat4Array(std::string_view propertyName) const
{
    return m_Impl->getMat4Array(propertyName);
}

void osc::Material::setMat4Array(std::string_view propertyName, std::span<Mat4 const> mats)
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

std::optional<Texture2D> osc::Material::getTexture(std::string_view propertyName) const
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

std::optional<RenderTexture> osc::Material::getRenderTexture(std::string_view propertyName) const
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

std::optional<Cubemap> osc::Material::getCubemap(std::string_view propertyName) const
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

DepthFunction osc::Material::getDepthFunction() const
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

CullMode osc::Material::getCullMode() const
{
    return m_Impl->getCullMode();
}

void osc::Material::setCullMode(CullMode newMode)
{
    m_Impl.upd()->setCullMode(newMode);
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

    std::optional<Vec3> getVec3(std::string_view propertyName) const
    {
        return getValue<Vec3>(propertyName);
    }

    void setVec3(std::string_view propertyName, Vec3 value)
    {
        setValue(propertyName, value);
    }

    std::optional<Vec4> getVec4(std::string_view propertyName) const
    {
        return getValue<Vec4>(propertyName);
    }

    void setVec4(std::string_view propertyName, Vec4 value)
    {
        setValue(propertyName, value);
    }

    std::optional<Mat3> getMat3(std::string_view propertyName) const
    {
        return getValue<Mat3>(propertyName);
    }

    void setMat3(std::string_view propertyName, Mat3 const& value)
    {
        setValue(propertyName, value);
    }

    std::optional<Mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<Mat4>(propertyName);
    }

    void setMat4(std::string_view propertyName, Mat4 const& value)
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

    friend bool operator==(Impl const&, Impl const&) = default;

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        auto const it = m_Values.find(propertyName);

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
        m_Values.insert_or_assign(propertyName, std::forward<T>(v));
    }

    friend class GraphicsBackend;

    FastStringHashtable<MaterialValue> m_Values;
};

osc::MaterialPropertyBlock::MaterialPropertyBlock() :
    m_Impl{[]()
    {
        static CopyOnUpdPtr<Impl> const s_EmptyPropertyBlockImpl = make_cow<Impl>();
        return s_EmptyPropertyBlockImpl;
    }()}
{
}

void osc::MaterialPropertyBlock::clear()
{
    m_Impl.upd()->clear();
}

bool osc::MaterialPropertyBlock::isEmpty() const
{
    return m_Impl->isEmpty();
}

std::optional<Color> osc::MaterialPropertyBlock::getColor(std::string_view propertyName) const
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

std::optional<Vec3> osc::MaterialPropertyBlock::getVec3(std::string_view propertyName) const
{
    return m_Impl->getVec3(propertyName);
}

void osc::MaterialPropertyBlock::setVec3(std::string_view propertyName, Vec3 value)
{
    m_Impl.upd()->setVec3(propertyName, value);
}

std::optional<Vec4> osc::MaterialPropertyBlock::getVec4(std::string_view propertyName) const
{
    return m_Impl->getVec4(propertyName);
}

void osc::MaterialPropertyBlock::setVec4(std::string_view propertyName, Vec4 value)
{
    m_Impl.upd()->setVec4(propertyName, value);
}

std::optional<Mat3> osc::MaterialPropertyBlock::getMat3(std::string_view propertyName) const
{
    return m_Impl->getMat3(propertyName);
}

void osc::MaterialPropertyBlock::setMat3(std::string_view propertyName, Mat3 const& value)
{
    m_Impl.upd()->setMat3(propertyName, value);
}

std::optional<Mat4> osc::MaterialPropertyBlock::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(propertyName);
}

void osc::MaterialPropertyBlock::setMat4(std::string_view propertyName, Mat4 const& value)
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

std::optional<Texture2D> osc::MaterialPropertyBlock::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(propertyName);
}

void osc::MaterialPropertyBlock::setTexture(std::string_view propertyName, Texture2D t)
{
    m_Impl.upd()->setTexture(propertyName, std::move(t));
}

bool osc::operator==(MaterialPropertyBlock const& lhs, MaterialPropertyBlock const& rhs)
{
    return lhs.m_Impl == rhs.m_Impl || *lhs.m_Impl == *rhs.m_Impl;
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
    constexpr auto c_MeshTopologyStrings = std::to_array<CStringView>(
    {
        "Triangles",
        "Lines",
    });
    static_assert(c_MeshTopologyStrings.size() == NumOptions<MeshTopology>());

    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
    static_assert(alignof(PackedIndex) == alignof(uint32_t));

    GLenum ToOpenGLTopology(MeshTopology t)
    {
        static_assert(NumOptions<MeshTopology>() == 2);

        switch (t)
        {
        case MeshTopology::Triangles:
            return GL_TRIANGLES;
        case MeshTopology::Lines:
            return GL_LINES;
        default:
            return GL_TRIANGLES;
        }
    }

    // types that can be read/written to/from a vertex buffer by higher
    // levels of the API
    template<typename T>
    concept UserFacingVertexData = IsAnyOf<T, Vec2, Vec3, Vec4, Vec<4, Unorm8>, Color, Color32>;

    // types that are encode-/decode-able into a vertex buffer
    template<typename T>
    concept VertexBufferComponent = IsAnyOf<T, float, Unorm8>;

    // low-level single-component Decode/Encode functions
    template<VertexBufferComponent EncodedValue, typename DecodedValue>
    DecodedValue Decode(std::byte const*);

    template<typename DecodedValue, VertexBufferComponent EncodedValue>
    void Encode(std::byte*, DecodedValue);

    template<>
    float Decode<float, float>(std::byte const* p)
    {
        return *std::launder(reinterpret_cast<float const*>(p));
    }

    template<>
    void Encode<float, float>(std::byte* p, float v)
    {
        *std::launder(reinterpret_cast<float*>(p)) = v;
    }

    template<>
    float Decode<Unorm8, float>(std::byte const* p)
    {
        return Unorm8{*p}.normalized_value();
    }

    template<>
    void Encode<float, Unorm8>(std::byte* p, float v)
    {
        *p = Unorm8{v}.byte();
    }

    template<>
    Unorm8 Decode<Unorm8, Unorm8>(std::byte const* p)
    {
        return Unorm8{*p};
    }

    template<>
    void Encode<Unorm8, Unorm8>(std::byte* p, Unorm8 v)
    {
        *p = v.byte();
    }

    // mid-level multi-component Decode/Encode functions
    template<UserFacingVertexData T, VertexAttributeFormat EncodingFormat>
    void EncodeMany(std::byte* p, T const& v)
    {
        using ComponentType = typename VertexAttributeFormatTraits<EncodingFormat>::component_type;
        constexpr auto numComponents = NumComponents(EncodingFormat);
        constexpr auto sizeOfComponent = SizeOfComponent(EncodingFormat);
        constexpr auto n = std::min(std::tuple_size_v<T>, static_cast<typename T::size_type>(numComponents));

        for (typename T::size_type i = 0; i < n; ++i)
        {
            Encode<typename T::value_type, ComponentType>(p + i*sizeOfComponent, v[i]);
        }
    }

    template<VertexAttributeFormat EncodingFormat, UserFacingVertexData T>
    T DecodeMany(std::byte const* p)
    {
        using ComponentType = typename VertexAttributeFormatTraits<EncodingFormat>::component_type;
        constexpr auto numComponents = NumComponents(EncodingFormat);
        constexpr auto sizeOfComponent = SizeOfComponent(EncodingFormat);
        constexpr auto n = std::min(std::tuple_size_v<T>, static_cast<typename T::size_type>(numComponents));

        T rv{};
        for (typename T::size_type i = 0; i < n; ++i)
        {
            rv[i] = Decode<ComponentType, typename T::value_type>(p + i*sizeOfComponent);
        }
        return rv;
    }

    // high-level, compile-time multi-component Decode + Encode definition
    template<UserFacingVertexData T>
    class MultiComponentEncoding final {
    public:
        explicit MultiComponentEncoding(VertexAttributeFormat f)
        {
            static_assert(NumOptions<VertexAttributeFormat>() == 4);

            switch (f) {
            case VertexAttributeFormat::Float32x2:
                m_Encoder = EncodeMany<T, VertexAttributeFormat::Float32x2>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Float32x2, T>;
                break;
            case VertexAttributeFormat::Float32x3:
                m_Encoder = EncodeMany<T, VertexAttributeFormat::Float32x3>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Float32x3, T>;
                break;
            default:
            case VertexAttributeFormat::Float32x4:
                m_Encoder = EncodeMany<T, VertexAttributeFormat::Float32x4>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Float32x4, T>;
                break;
            case VertexAttributeFormat::Unorm8x4:
                m_Encoder = EncodeMany<T, VertexAttributeFormat::Unorm8x4>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Unorm8x4, T>;
                break;
            }
        }

        void encode(std::byte* b, T const& v) const
        {
            m_Encoder(b, v);
        }

        T decode(std::byte const* b) const
        {
            return m_Decoder(b);
        }

        friend bool operator==(MultiComponentEncoding const&, MultiComponentEncoding const&) = default;
    private:
        using Encoder = void(*)(std::byte*, T const&);
        Encoder m_Encoder;

        using Decoder = T(*)(std::byte const*);
        Decoder m_Decoder;
    };

    // a single compile-time reencoding function
    //
    // decodes in-memory data in a source format, converts it to a desination format, and then
    // writes it to the destination memory
    template<VertexAttributeFormat SourceFormat, VertexAttributeFormat DestinationFormat>
    void Reencode(std::span<std::byte const> src, std::span<std::byte> dest)
    {
        using SourceCPUFormat = typename VertexAttributeFormatTraits<SourceFormat>::type;
        using DestCPUFormat = typename VertexAttributeFormatTraits<DestinationFormat>::type;
        constexpr auto n = std::min(std::tuple_size_v<SourceCPUFormat>, std::tuple_size_v<DestCPUFormat>);

        auto const decoded = DecodeMany<SourceFormat, SourceCPUFormat>(src.data());
        DestCPUFormat converted{};
        for (size_t i = 0; i < n; ++i)
        {
            converted[i] = typename DestCPUFormat::value_type{decoded[i]};
        }
        EncodeMany<DestCPUFormat, DestinationFormat>(dest.data(), converted);
    }

    // type-erased (i.e. runtime) reencoder function
    using ReencoderFunction = void(*)(std::span<std::byte const>, std::span<std::byte>);

    // compile-time lookup table (LUT) for runtime reencoder functions
    class ReencoderLut final {
    private:
        static constexpr size_t indexOf(VertexAttributeFormat sourceFormat, VertexAttributeFormat destinationFormat)
        {
            return static_cast<size_t>(sourceFormat)*NumOptions<VertexAttributeFormat>() + static_cast<size_t>(destinationFormat);
        }

        template<VertexAttributeFormat... Formats>
        static constexpr void WriteEntriesTopLevel(ReencoderLut& lut, OptionList<VertexAttributeFormat, Formats...>)
        {
            (WriteEntries<Formats, Formats...>(lut), ...);
        }

        template<VertexAttributeFormat SourceFormat, VertexAttributeFormat... DestinationFormats>
        static constexpr void WriteEntries(ReencoderLut& lut)
        {
            (WriteEntry<SourceFormat, DestinationFormats>(lut), ...);
        }

        template<VertexAttributeFormat SourceFormat, VertexAttributeFormat DestinationFormat>
        static constexpr void WriteEntry(ReencoderLut& lut)
        {
            lut.assign(SourceFormat, DestinationFormat, Reencode<SourceFormat, DestinationFormat>);
        }
    public:
        constexpr ReencoderLut()
        {
            WriteEntriesTopLevel(*this, VertexAttributeFormatList{});
        }

        constexpr void assign(VertexAttributeFormat sourceFormat, VertexAttributeFormat destinationFormat, ReencoderFunction f)
        {
            m_Storage.at(indexOf(sourceFormat, destinationFormat)) = f;
        }

        constexpr ReencoderFunction const& lookup(VertexAttributeFormat sourceFormat, VertexAttributeFormat destinationFormat) const
        {
            return m_Storage.at(indexOf(sourceFormat, destinationFormat));
        }

    private:
        std::array<ReencoderFunction, NumOptions<VertexAttributeFormat>()*NumOptions<VertexAttributeFormat>()> m_Storage{};
    };

    constexpr ReencoderLut c_ReencoderLUT;

    struct VertexBufferAttributeReencoder final {
        ReencoderFunction reencode;
        size_t sourceOffset;
        size_t sourceStride;
        size_t destintionOffset;
        size_t destinationStride;
    };

    std::vector<VertexBufferAttributeReencoder> GetReencoders(VertexFormat const& srcFormat, VertexFormat const& destFormat)
    {
        std::vector<VertexBufferAttributeReencoder> rv;
        rv.reserve(destFormat.numAttributes());  // guess

        for (auto const destLayout : destFormat.attributeLayouts())
        {
            if (auto const srcLayout = srcFormat.attributeLayout(destLayout.attribute()))
            {
                rv.push_back({
                    c_ReencoderLUT.lookup(srcLayout->format(), destLayout.format()),
                    srcLayout->offset(),
                    srcLayout->stride(),
                    destLayout.offset(),
                    destLayout.stride(),
                });
            }
        }
        return rv;
    }

    void ReEncodeVertexBuffer(
        std::span<std::byte const> src,
        VertexFormat const& srcFormat,
        std::span<std::byte> dest,
        VertexFormat const& destFormat)
    {
        size_t const srcStride = srcFormat.stride();
        size_t const destStride = destFormat.stride();

        if (srcStride == 0 || destStride == 0)
        {
            return;  // no reencoding necessary
        }
        OSC_ASSERT(src.size() % srcStride == 0);
        OSC_ASSERT(dest.size() % destStride == 0);

        size_t const n = std::min(src.size() / srcStride, dest.size() / destStride);

        auto const reencoders = GetReencoders(srcFormat, destFormat);
        for (size_t i = 0; i < n; ++i)
        {
            auto const srcData = src.subspan(i*srcStride);
            auto const destData = dest.subspan(i*destStride);

            for (auto const& reencoder : reencoders)
            {
                auto const srcAttrData = srcData.subspan(reencoder.sourceOffset, reencoder.sourceStride);
                auto const destAttrData = destData.subspan(reencoder.destintionOffset, reencoder.destinationStride);
                reencoder.reencode(srcAttrData, destAttrData);
            }
        }
    }

    // reperesents vertex data on the CPU
    class VertexBuffer final {
    public:

        // proxies (via encoders/decoders) access to a value in the vertex buffer's bytes
        template<UserFacingVertexData T, bool IsConst>
        class AttributeValueProxy final {
        public:
            using Byte = std::conditional_t<IsConst, std::byte const, std::byte>;

            AttributeValueProxy(Byte* data_, MultiComponentEncoding<T> encoding_) :
                m_Data{data_},
                m_Encoding{encoding_}
            {
            }

            AttributeValueProxy& operator=(T const& v) requires (!IsConst)
            {
                m_Encoding.encode(m_Data, v);
                return *this;
            }

            operator T () const  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            {
                return m_Encoding.decode(m_Data);
            }

            template<typename U>
            AttributeValueProxy& operator/=(U const& v) requires (!IsConst)
            {
                return *this = (T{*this} /= v);
            }

            template<typename U>
            AttributeValueProxy& operator+=(U const& v) requires (!IsConst)
            {
                return *this = (T{*this} += v);
            }
        private:
            Byte* m_Data;
            MultiComponentEncoding<T> m_Encoding;
        };

        // iterator for vertex buffer's contents (via encoders/decoders)
        template<UserFacingVertexData T, bool IsConst>
        class AttributeValueIterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = AttributeValueProxy<T, IsConst>;
            using reference = value_type;
            using iterator_category = std::random_access_iterator_tag;

            using Byte = std::conditional_t<IsConst, std::byte const, std::byte>;

            AttributeValueIterator(
                Byte* data_,
                size_t stride_,
                MultiComponentEncoding<T> encoding_) :

                m_Data{data_},
                m_Stride{stride_},
                m_Encoding{encoding_}
            {
            }

            AttributeValueProxy<T, IsConst> operator*() const
            {
                return AttributeValueProxy<T, IsConst>{m_Data, m_Encoding};
            }

            AttributeValueIterator& operator++()
            {
                *this += 1;
                return *this;
            }

            AttributeValueIterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            AttributeValueIterator& operator--()
            {
                *this -= 1;
                return *this;
            }

            AttributeValueIterator operator--(int)
            {
                auto tmp = *this;
                --(*this);
                return tmp;
            }

            AttributeValueIterator& operator+=(difference_type i)
            {
                m_Data += i*m_Stride;
                return *this;
            }

            AttributeValueIterator operator+(difference_type i) const
            {
                auto copy = *this;
                copy += i;
                return copy;
            }

            AttributeValueIterator& operator-=(difference_type i)
            {
                m_Data -= i*m_Stride;
            }

            AttributeValueIterator operator-(difference_type i)
            {
                auto copy = *this;
                copy -= i;
                return copy;
            }

            difference_type operator-(AttributeValueIterator const& rhs) const
            {
                return (m_Data - rhs.m_Data) / m_Stride;
            }

            AttributeValueProxy<T, IsConst> operator[](difference_type n) const
            {
                return *(*this + n);
            }

            bool operator<(AttributeValueIterator const& rhs) const
            {
                return m_Data < rhs.m_Data;
            }

            bool operator>(AttributeValueIterator const& rhs) const
            {
                return m_Data > rhs.m_Data;
            }

            bool operator<=(AttributeValueIterator const& rhs) const
            {
                return m_Data <= rhs.m_Data;
            }

            bool operator>=(AttributeValueIterator const& rhs) const
            {
                return m_Data >= rhs.m_Data;
            }

            friend bool operator==(AttributeValueIterator const&, AttributeValueIterator const&) = default;
        private:
            Byte* m_Data;
            size_t m_Stride;
            MultiComponentEncoding<T> m_Encoding;
        };

        // range (C++20) for vertex buffer's contents
        template<UserFacingVertexData T, bool IsConst>
        class AttributeValueRange final {
        public:
            using Byte = std::conditional_t<IsConst, std::byte const, std::byte>;
            using iterator = AttributeValueIterator<T, IsConst>;
            using value_type = typename iterator::value_type;
            using difference_type = typename iterator::difference_type;

            AttributeValueRange() = default;

            AttributeValueRange(
                std::span<Byte> data_,
                size_t stride_,
                VertexAttributeFormat format_) :

                m_Data{data_},
                m_Stride{stride_},
                m_Encoding{format_}
            {
            }

            difference_type size() const
            {
                return std::distance(begin(), end());
            }

            iterator begin() const
            {
                return {m_Data.data(), m_Stride, m_Encoding};
            }

            iterator end() const
            {
                return {m_Data.data() + m_Data.size(), m_Stride, m_Encoding};
            }

            value_type at(difference_type i) const
            {
                return at(*this, i);
            }

            value_type at(difference_type i)
            {
                return at(*this, i);
            }

            value_type operator[](difference_type i) const
            {
                return begin()[i];
            }
        private:
            template<typename AttrValueRange>
            static value_type at(AttrValueRange&& range, difference_type i)
            {
                auto const beg = range.begin();
                if (i >= std::distance(beg, range.end()))
                {
                    throw std::out_of_range{"an attribute value was out-of-range: this is usually because of out-of-range mesh indices"};
                }
                return beg[i];
            }

            std::span<Byte> m_Data{};
            size_t m_Stride = 1;  // care: divide by zero in an iterator is UB
            MultiComponentEncoding<T> m_Encoding{VertexAttributeFormat::Float32x3};  // dummy, for default ctor
        };

        // default ctor: make an empty buffer
        VertexBuffer() = default;

        // formatted ctor: make a buffer of the specified size+format
        VertexBuffer(size_t numVerts, VertexFormat const& format) :
            m_Data(numVerts * format.stride()),
            m_VertexFormat{format}
        {
        }

        void clear()
        {
            m_Data.clear();
            m_VertexFormat.clear();
        }

        size_t numVerts() const
        {
            return !m_VertexFormat.empty() ? (m_Data.size() / m_VertexFormat.stride()) : 0;
        }

        size_t numAttributes() const
        {
            return m_VertexFormat.numAttributes();
        }

        size_t stride() const
        {
            return m_VertexFormat.stride();
        }

        [[nodiscard]] bool hasVerts() const
        {
            return numVerts() > 0;
        }

        std::span<std::byte const> bytes() const
        {
            return m_Data;
        }

        VertexFormat const& format() const
        {
            return m_VertexFormat;
        }

        auto attributeLayouts() const
        {
            return m_VertexFormat.attributeLayouts();
        }

        bool hasAttribute(VertexAttribute attr) const
        {
            return m_VertexFormat.contains(attr);
        }

        template<UserFacingVertexData T>
        auto iter(VertexAttribute attr) const
        {
            if (auto const layout = m_VertexFormat.attributeLayout(attr))
            {
                std::span<std::byte const> offsetSpan{m_Data.data() + layout->offset(), m_Data.size()};
                return AttributeValueRange<T, true>
                {
                    offsetSpan,
                    m_VertexFormat.stride(),
                    layout->format(),
                };
            }
            else
            {
                return AttributeValueRange<T, true>{};
            }
        }

        template<UserFacingVertexData T>
        auto iter(VertexAttribute attr)
        {
            if (auto const layout = m_VertexFormat.attributeLayout(attr))
            {
                std::span<std::byte> offsetSpan{m_Data.data() + layout->offset(), m_Data.size()};
                return AttributeValueRange<T, false>
                {
                    offsetSpan,
                    m_VertexFormat.stride(),
                    layout->format(),
                };
            }
            else
            {
                return AttributeValueRange<T, false>{};
            }
        }

        template<UserFacingVertexData T>
        std::vector<T> read(VertexAttribute attr) const
        {
            auto range = iter<T>(attr);
            return std::vector<T>(range.begin(), range.end());
        }

        template<UserFacingVertexData T>
        void write(VertexAttribute attr, std::span<T const> els)
        {
            // edge-case: size == 0 should be treated as "wipe/ignore it"
            if (els.empty()) {
                if (m_VertexFormat.contains(attr)) {
                    VertexFormat newFormat{m_VertexFormat};
                    newFormat.erase(attr);
                    setParams(numVerts(), newFormat);
                }
                return;  // ignore/wipe
            }

            if (attr != VertexAttribute::Position)
            {
                if (els.size() != numVerts())
                {
                    // non-`Position` attributes must be size-matched
                    return;
                }

                if (!m_VertexFormat.contains(VertexAttribute::Position))
                {
                    // callers must've already assigned `Position` before this
                    // function is able to assign additional attributes
                    return;
                }
            }

            if (!m_VertexFormat.contains(attr))
            {
                // reformat
                VertexFormat newFormat{m_VertexFormat};
                newFormat.insert({attr, DefaultFormat(attr)});
                setParams(els.size(), newFormat);
            }
            else if (els.size() != numVerts())
            {
                // resize
                setParams(els.size(), m_VertexFormat);
            }

            // write els to vertex buffer
            std::copy(els.begin(), els.end(), iter<T>(attr).begin());
        }

        template<UserFacingVertexData T, typename UnaryOperation>
        void transformAttribute(VertexAttribute attr, UnaryOperation f)
            requires std::invocable<UnaryOperation, T>
        {
            for (auto&& proxy : iter<T>(attr))
            {
                proxy = f(proxy);
            }
        }

        bool emplaceAttributeDescriptor(VertexAttributeDescriptor desc)
        {
            if (hasAttribute(desc.attribute())) {
                return false;
            }

            auto copy = format();
            copy.insert(desc);
            setFormat(copy);
            return true;
        }

        void setParams(size_t newNumVerts, VertexFormat const& newFormat)
        {
            if (m_Data.empty())
            {
                // zero-initialize the buffer in the "new" format
                m_Data.resize(newNumVerts * newFormat.stride());
                m_VertexFormat = newFormat;
            }
            if (newFormat != m_VertexFormat)
            {
                // initialize a new buffer and re-encode the old one in the new format
                std::vector<std::byte> newBuf(newNumVerts * newFormat.stride());
                ReEncodeVertexBuffer(m_Data, m_VertexFormat, newBuf, newFormat);
                m_Data = std::move(newBuf);
                m_VertexFormat = newFormat;
            }
            else if (newNumVerts != numVerts())
            {
                // resize (zero-initialized, if growing) the buffer
                m_Data.resize(newNumVerts * m_VertexFormat.stride());
            }
            else
            {
                // no change in format or size, do nothing
            }
        }

        void setFormat(VertexFormat const& newFormat)
        {
            setParams(numVerts(), newFormat);
        }

        void setData(std::span<std::byte const> newData)
        {
            OSC_ASSERT(newData.size() == m_Data.size() && "provided data size does not match the size of the vertex buffer");
            m_Data.assign(newData.begin(), newData.end());
        }
    private:
        std::vector<std::byte> m_Data;
        VertexFormat m_VertexFormat;
    };
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

    size_t getNumVerts() const
    {
        return m_VertexBuffer.numVerts();
    }

    bool hasVerts() const
    {
        return m_VertexBuffer.hasVerts();
    }

    std::vector<Vec3> getVerts() const
    {
        return m_VertexBuffer.read<Vec3>(VertexAttribute::Position);
    }

    void setVerts(std::span<Vec3 const> verts)
    {
        m_VertexBuffer.write<Vec3>(VertexAttribute::Position, verts);

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    void transformVerts(std::function<Vec3(Vec3)> const& f)
    {
        m_VertexBuffer.transformAttribute<Vec3>(VertexAttribute::Position, f);

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    void transformVerts(Transform const& t)
    {
        m_VertexBuffer.transformAttribute<Vec3>(VertexAttribute::Position, [&t](Vec3 v)
        {
            return t * v;
        });

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    void transformVerts(Mat4 const& m)
    {
        m_VertexBuffer.transformAttribute<Vec3>(VertexAttribute::Position, [&m](Vec3 v)
        {
            return Vec3{m * Vec4{v, 1.0f}};
        });

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    bool hasNormals() const
    {
        return m_VertexBuffer.hasAttribute(VertexAttribute::Normal);
    }

    std::vector<Vec3> getNormals() const
    {
        return m_VertexBuffer.read<Vec3>(VertexAttribute::Normal);
    }

    void setNormals(std::span<Vec3 const> normals)
    {
        m_VertexBuffer.write<Vec3>(VertexAttribute::Normal, normals);

        m_Version->reset();
    }

    void transformNormals(std::function<Vec3(Vec3)> const& f)
    {
        m_VertexBuffer.transformAttribute<Vec3>(VertexAttribute::Normal, f);

        m_Version->reset();
    }

    bool hasTexCoords() const
    {
        return m_VertexBuffer.hasAttribute(VertexAttribute::TexCoord0);
    }

    std::vector<Vec2> getTexCoords() const
    {
        return m_VertexBuffer.read<Vec2>(VertexAttribute::TexCoord0);
    }

    void setTexCoords(std::span<Vec2 const> coords)
    {
        m_VertexBuffer.write<Vec2>(VertexAttribute::TexCoord0, coords);

        m_Version->reset();
    }

    void transformTexCoords(std::function<Vec2(Vec2)> const& f)
    {
        m_VertexBuffer.transformAttribute<Vec2>(VertexAttribute::TexCoord0, f);

        m_Version->reset();
    }

    std::vector<Color> getColors() const
    {
        return m_VertexBuffer.read<Color>(VertexAttribute::Color);
    }

    void setColors(std::span<Color const> colors)
    {
        m_VertexBuffer.write<Color>(VertexAttribute::Color, colors);

        m_Version.reset();
    }

    std::vector<Vec4> getTangents() const
    {
        return m_VertexBuffer.read<Vec4>(VertexAttribute::Tangent);
    }

    void setTangents(std::span<Vec4 const> newTangents)
    {
        m_VertexBuffer.write<Vec4>(VertexAttribute::Tangent, newTangents);

        m_Version->reset();
    }

    size_t getNumIndices() const
    {
        return m_NumIndices;
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

    void setIndices(MeshIndicesView indices, MeshUpdateFlags flags)
    {
        indices.isU16() ? setIndices(indices.toU16Span(), flags) : setIndices(indices.toU32Span(), flags);
    }

    void forEachIndexedVert(std::function<void(Vec3)> const& f) const
    {
        auto const positions = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position).begin();
        for (auto idx : getIndices())
        {
            f(positions[idx]);
        }
    }

    void forEachIndexedTriangle(std::function<void(Triangle)> const& f) const
    {
        if (m_Topology != MeshTopology::Triangles)
        {
            return;
        }

        MeshIndicesView const indices = getIndices();
        size_t const steps = (indices.size() / 3) * 3;

        auto const positions = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position).begin();
        for (size_t i = 0; i < steps; i += 3)
        {
            f(Triangle{
                positions[indices[i]],
                positions[indices[i+1]],
                positions[indices[i+2]],
            });
        }
    }

    Triangle getTriangleAt(size_t firstIndexOffset) const
    {
        if (m_Topology != MeshTopology::Triangles)
        {
            throw std::runtime_error{"cannot call getTriangleAt on a non-triangular-topology mesh"};
        }

        auto const indices = getIndices();

        if (firstIndexOffset+2 >= indices.size())
        {
            throw std::runtime_error{"provided first index offset is out-of-bounds"};
        }

        auto const verts = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);

        // can use unchecked access here: `indices` are range-checked on writing
        return Triangle{
            verts[indices[firstIndexOffset+0]],
            verts[indices[firstIndexOffset+1]],
            verts[indices[firstIndexOffset+2]],
        };
    }

    std::vector<Vec3> getIndexedVerts() const
    {
        std::vector<Vec3> rv;
        rv.reserve(getNumIndices());
        forEachIndexedVert([&rv](Vec3 v) { rv.push_back(v); });
        return rv;
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    void clear()
    {
        m_Version->reset();
        m_Topology = MeshTopology::Triangles;
        m_VertexBuffer.clear();
        m_IndicesAre32Bit = false;
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = {};
        m_SubMeshDescriptors.clear();
    }

    size_t getSubMeshCount() const
    {
        return m_SubMeshDescriptors.size();
    }

    void pushSubMeshDescriptor(SubMeshDescriptor const& desc)
    {
        m_SubMeshDescriptors.push_back(desc);
    }

    SubMeshDescriptor const& getSubMeshDescriptor(size_t i) const
    {
        return m_SubMeshDescriptors.at(i);
    }

    void clearSubMeshDescriptors()
    {
        m_SubMeshDescriptors.clear();
    }

    size_t getVertexAttributeCount() const
    {
        return m_VertexBuffer.numAttributes();
    }

    VertexFormat const& getVertexAttributes() const
    {
        return m_VertexBuffer.format();
    }

    void setVertexBufferParams(size_t newNumVerts, VertexFormat const& newFormat)
    {
        m_VertexBuffer.setParams(newNumVerts, newFormat);

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    size_t getVertexBufferStride() const
    {
        return m_VertexBuffer.stride();
    }

    void setVertexBufferData(std::span<uint8_t const> newData, MeshUpdateFlags flags)
    {
        m_VertexBuffer.setData(std::as_bytes(newData));

        rangeCheckIndicesAndRecalculateBounds(flags);
        m_Version->reset();
    }

    void recalculateNormals()
    {
        if (getTopology() != MeshTopology::Triangles) {
            // if the mesh isn't triangle-based, do nothing
            return;
        }

        // ensure the vertex buffer has a normal attribute
        m_VertexBuffer.emplaceAttributeDescriptor({VertexAttribute::Normal, VertexAttributeFormat::Float32x3});

        // calculate normals from triangle faces:
        //
        // - keep a count of the number of times a normal has been assigned
        // - compute the normal from the triangle
        // - if counts[i] == 0 assign it (we can't assume the buffer is zeroed - could be reused)
        // - else, add (accumulate)
        // - ++counts[i]
        // - at the end, if counts[i] > 1, then renormalize that normal (it contains a sum)

        auto const indices = getIndices();
        auto const positions = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);
        auto normals = m_VertexBuffer.iter<Vec3>(VertexAttribute::Normal);
        std::vector<uint16_t> counts(normals.size());

        for (size_t i = 0, len = 3*(indices.size()/3); i < len; i+=3) {
            // get triangle indices
            Vec3uz const idxs = {indices[i], indices[i+1], indices[i+2]};

            // get triangle
            Triangle const triangle = {positions[idxs[0]], positions[idxs[1]], positions[idxs[2]]};

            // calculate + validate triangle normal
            auto const normal = triangle_normal(triangle).unwrap();
            if (any_of(isnan(normal))) {
                continue;  // probably co-located, or invalid: don't accumulate it
            }

            // accumulate
            for (auto idx : idxs) {
                if (counts[idx] == 0) {
                    normals[idx] = normal;
                }
                else {
                    normals[idx] += normal;
                }
                ++counts[idx];
            }
        }

        // renormalize shared normals
        for (size_t i = 0; i < counts.size(); ++i) {
            if (counts[i] > 1) {
                normals[i] = normalize(Vec3{normals[i]});
            }
        }
    }

    void recalculateTangents()
    {
        if (getTopology() != MeshTopology::Triangles) {
            // if the mesh isn't triangle-based, do nothing
            return;
        }
        if (!m_VertexBuffer.hasAttribute(VertexAttribute::Normal)) {
            // if the mesh doesn't have normals, do nothing
            return;
        }
        if (!m_VertexBuffer.hasAttribute(VertexAttribute::TexCoord0)) {
            // if the mesh doesn't have texture coordinates, do nothing
            return;
        }
        if (m_IndicesData.empty()) {
            // if the mesh has no indices, do nothing
            return;
        }

        // ensure the vertex buffer has space for tangents
        m_VertexBuffer.emplaceAttributeDescriptor({ VertexAttribute::Tangent, VertexAttributeFormat::Float32x3 });

        // calculate tangents

        auto const vbverts = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);
        auto const vbnormals = m_VertexBuffer.iter<Vec3>(VertexAttribute::Normal);
        auto const vbtexcoords = m_VertexBuffer.iter<Vec2>(VertexAttribute::TexCoord0);

        auto const tangents = CalcTangentVectors(
            MeshTopology::Triangles,
            std::vector<Vec3>(vbverts.begin(), vbverts.end()),
            std::vector<Vec3>(vbnormals.begin(), vbnormals.end()),
            std::vector<Vec2>(vbtexcoords.begin(), vbtexcoords.end()),
            getIndices()
        );

        m_VertexBuffer.write<Vec4>(VertexAttribute::Tangent, tangents);
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

    void drawInstanced(
        size_t n,
        std::optional<size_t> maybeSubMeshIndex)
    {
        SubMeshDescriptor const descriptor = maybeSubMeshIndex ?
            m_SubMeshDescriptors.at(*maybeSubMeshIndex) :         // draw the requested sub-mesh
            SubMeshDescriptor{0, m_NumIndices, m_Topology};       // else: draw the entire mesh as a "sub mesh"

        // convert mesh/descriptor data types into OpenGL-compatible formats
        GLenum const mode = ToOpenGLTopology(descriptor.getTopology());
        auto const count = static_cast<GLsizei>(descriptor.getIndexCount());
        GLenum const type = m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;

        size_t const bytesPerIndex = m_IndicesAre32Bit ? sizeof(GLint) : sizeof(GLshort);
        size_t const firstIndexByteOffset = descriptor.getIndexStart() * bytesPerIndex;
        void const* indices = cpp20::bit_cast<void*>(firstIndexByteOffset);

        auto const instanceCount = static_cast<GLsizei>(n);

        glDrawElementsInstanced(
            mode,
            count,
            type,
            indices,
            instanceCount
        );
    }

private:

    void setIndices(std::span<uint16_t const> indices, MeshUpdateFlags flags)
    {
        m_IndicesAre32Bit = false;
        m_NumIndices = indices.size();
        m_IndicesData.resize((indices.size()+1)/2);
        std::copy(indices.begin(), indices.end(), &m_IndicesData.front().u16.a);

        rangeCheckIndicesAndRecalculateBounds(flags);
        m_Version->reset();
    }

    void setIndices(std::span<std::uint32_t const> vs, MeshUpdateFlags flags)
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

        rangeCheckIndicesAndRecalculateBounds(flags);
        m_Version->reset();
    }

    void rangeCheckIndicesAndRecalculateBounds(
        MeshUpdateFlags flags = MeshUpdateFlags::Default)
    {
        // note: recalculating bounds will always validate indices anyway, because it's assumed
        //       that the caller's intention is that all indices are valid when computing the
        //       bounds
        bool const checkIndices = !((flags & MeshUpdateFlags::DontValidateIndices) && (flags & MeshUpdateFlags::DontRecalculateBounds));

        //       ... but it's perfectly reasonable for the caller to only want the indices to be
        //       validated, leaving the bounds untouched
        bool const recalculateBounds = !(flags & MeshUpdateFlags::DontRecalculateBounds);

        if (checkIndices && recalculateBounds)
        {
            if (m_NumIndices == 0)
            {
                m_AABB = {};
                return;
            }

            // recalculate bounds while also checking indices
            m_AABB.min =
            {
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
            };

            m_AABB.max =
            {
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
            };

            auto range = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);
            for (auto idx : getIndices())
            {
                Vec3 pos = range.at(idx);  // bounds-check index
                m_AABB.min = elementwise_min(m_AABB.min, pos);
                m_AABB.max = elementwise_max(m_AABB.max, pos);
            }
        }
        else if (checkIndices && !recalculateBounds)
        {
            for (auto meshIndex : getIndices())
            {
                OSC_ASSERT(meshIndex < m_VertexBuffer.numVerts() && "a mesh index is out of bounds");
            }
        }
        else
        {
            return;  // do nothing
        }
    }

    static GLuint GetVertexAttributeIndex(VertexAttribute attr)
    {
        auto constexpr lut = []<VertexAttribute... Attrs>(OptionList<VertexAttribute, Attrs...>)
        {
            return std::to_array({ VertexAttributeTraits<Attrs>::shader_location... });
        }(VertexAttributeList{});

        return lut.at(ToIndex(attr));
    }

    static GLint GetVertexAttributeSize(VertexAttributeFormat const& format)
    {
        return static_cast<GLint>(NumComponents(format));
    }

    static GLenum GetVertexAttributeType(VertexAttributeFormat const& format)
    {
        static_assert(NumOptions<VertexAttributeFormat>() == 4);

        switch (format) {
        case VertexAttributeFormat::Float32x2:
        case VertexAttributeFormat::Float32x3:
        case VertexAttributeFormat::Float32x4:
            return GL_FLOAT;
        case VertexAttributeFormat::Unorm8x4:
            return GL_UNSIGNED_BYTE;
        default:
            throw std::runtime_error{"nyi"};
        }
    }

    static GLboolean GetVertexAttributeNormalized(VertexAttributeFormat const& format)
    {
        static_assert(NumOptions<VertexAttributeFormat>() == 4);

        switch (format) {
        case VertexAttributeFormat::Float32x2:
        case VertexAttributeFormat::Float32x3:
        case VertexAttributeFormat::Float32x4:
            return GL_FALSE;
        case VertexAttributeFormat::Unorm8x4:
            return GL_TRUE;
        default:
            throw std::runtime_error{"nyi"};
        }
    }

    static void OpenGLBindVertexAttribute(VertexFormat const& format, VertexFormat::VertexAttributeLayout const& layout)
    {
        glVertexAttribPointer(
            GetVertexAttributeIndex(layout.attribute()),
            GetVertexAttributeSize(layout.format()),
            GetVertexAttributeType(layout.format()),
            GetVertexAttributeNormalized(layout.format()),
            static_cast<GLsizei>(format.stride()),
            cpp20::bit_cast<void*>(layout.offset())
        );
        glEnableVertexAttribArray(GetVertexAttributeIndex(layout.attribute()));
    }

    void uploadToGPU()
    {
        // allocate GPU-side buffers (or re-use the last ones)
        if (!(*m_MaybeGPUBuffers))
        {
            *m_MaybeGPUBuffers = MeshOpenGLData{};
        }
        MeshOpenGLData& buffers = **m_MaybeGPUBuffers;

        // upload CPU-side vector data into the GPU-side buffer
        OSC_ASSERT(cpp20::bit_cast<uintptr_t>(m_VertexBuffer.bytes().data()) % alignof(float) == 0);
        gl::BindBuffer(
            GL_ARRAY_BUFFER,
            buffers.arrayBuffer
        );
        gl::BufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizei>(m_VertexBuffer.bytes().size()),
            m_VertexBuffer.bytes().data(),
            GL_STATIC_DRAW
        );

        // upload CPU-side element data into the GPU-side buffer
        size_t const eboNumBytes = m_NumIndices * (m_IndicesAre32Bit ? sizeof(uint32_t) : sizeof(uint16_t));
        gl::BindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            buffers.indicesBuffer
        );
        gl::BufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizei>(eboNumBytes),
            m_IndicesData.data(),
            GL_STATIC_DRAW
        );

        // configure mesh-level VAO
        gl::BindVertexArray(buffers.vao);
        gl::BindBuffer(GL_ARRAY_BUFFER, buffers.arrayBuffer);
        gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indicesBuffer);
        for (auto&& layout : m_VertexBuffer.attributeLayouts())
        {
            OpenGLBindVertexAttribute(m_VertexBuffer.format(), layout);
        }
        gl::BindVertexArray();

        buffers.dataVersion = *m_Version;
    }

    DefaultConstructOnCopy<UID> m_Version;
    MeshTopology m_Topology = MeshTopology::Triangles;
    VertexBuffer m_VertexBuffer;

    bool m_IndicesAre32Bit = false;
    size_t m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;

    AABB m_AABB = {};

    std::vector<SubMeshDescriptor> m_SubMeshDescriptors;

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

MeshTopology osc::Mesh::getTopology() const
{
    return m_Impl->getTopology();
}

void osc::Mesh::setTopology(MeshTopology topology)
{
    m_Impl.upd()->setTopology(topology);
}

size_t osc::Mesh::getNumVerts() const
{
    return m_Impl->getNumVerts();
}

bool osc::Mesh::hasVerts() const
{
    return m_Impl->hasVerts();
}

std::vector<Vec3> osc::Mesh::getVerts() const
{
    return m_Impl->getVerts();
}

void osc::Mesh::setVerts(std::span<Vec3 const> verts)
{
    m_Impl.upd()->setVerts(verts);
}

void osc::Mesh::transformVerts(std::function<Vec3(Vec3)> const& f)
{
    m_Impl.upd()->transformVerts(f);
}

void osc::Mesh::transformVerts(Transform const& t)
{
    m_Impl.upd()->transformVerts(t);
}

void osc::Mesh::transformVerts(Mat4 const& m)
{
    m_Impl.upd()->transformVerts(m);
}

bool osc::Mesh::hasNormals() const
{
    return m_Impl->hasNormals();
}

std::vector<Vec3> osc::Mesh::getNormals() const
{
    return m_Impl->getNormals();
}

void osc::Mesh::setNormals(std::span<Vec3 const> verts)
{
    m_Impl.upd()->setNormals(verts);
}

void osc::Mesh::transformNormals(std::function<Vec3(Vec3)> const& f)
{
    m_Impl.upd()->transformNormals(f);
}

bool osc::Mesh::hasTexCoords() const
{
    return m_Impl->hasTexCoords();
}

std::vector<Vec2> osc::Mesh::getTexCoords() const
{
    return m_Impl->getTexCoords();
}

void osc::Mesh::setTexCoords(std::span<Vec2 const> coords)
{
    m_Impl.upd()->setTexCoords(coords);
}

void osc::Mesh::transformTexCoords(std::function<Vec2(Vec2)> const& f)
{
    m_Impl.upd()->transformTexCoords(f);
}

std::vector<Color> osc::Mesh::getColors() const
{
    return m_Impl->getColors();
}

void osc::Mesh::setColors(std::span<Color const> colors)
{
    m_Impl.upd()->setColors(colors);
}

std::vector<Vec4> osc::Mesh::getTangents() const
{
    return m_Impl->getTangents();
}

void osc::Mesh::setTangents(std::span<Vec4 const> newTangents)
{
    m_Impl.upd()->setTangents(newTangents);
}

size_t osc::Mesh::getNumIndices() const
{
    return m_Impl->getNumIndices();
}

MeshIndicesView osc::Mesh::getIndices() const
{
    return m_Impl->getIndices();
}

void osc::Mesh::setIndices(MeshIndicesView indices, MeshUpdateFlags flags)
{
    m_Impl.upd()->setIndices(indices, flags);
}

void osc::Mesh::forEachIndexedVert(std::function<void(Vec3)> const& f) const
{
    m_Impl->forEachIndexedVert(f);
}

void osc::Mesh::forEachIndexedTriangle(std::function<void(Triangle)> const& f) const
{
    m_Impl->forEachIndexedTriangle(f);
}

Triangle osc::Mesh::getTriangleAt(size_t firstIndexOffset) const
{
    return m_Impl->getTriangleAt(firstIndexOffset);
}

std::vector<Vec3> osc::Mesh::getIndexedVerts() const
{
    return m_Impl->getIndexedVerts();
}

AABB const& osc::Mesh::getBounds() const
{
    return m_Impl->getBounds();
}

void osc::Mesh::clear()
{
    m_Impl.upd()->clear();
}

size_t osc::Mesh::getSubMeshCount() const
{
    return m_Impl->getSubMeshCount();
}

void osc::Mesh::pushSubMeshDescriptor(SubMeshDescriptor const& desc)
{
    m_Impl.upd()->pushSubMeshDescriptor(desc);
}

SubMeshDescriptor const& osc::Mesh::getSubMeshDescriptor(size_t i) const
{
    return m_Impl->getSubMeshDescriptor(i);
}

void osc::Mesh::clearSubMeshDescriptors()
{
    m_Impl.upd()->clearSubMeshDescriptors();
}

size_t osc::Mesh::getVertexAttributeCount() const
{
    return m_Impl->getVertexAttributeCount();
}

VertexFormat const& osc::Mesh::getVertexAttributes() const
{
    return m_Impl->getVertexAttributes();
}

void osc::Mesh::setVertexBufferParams(size_t n, VertexFormat const& format)
{
    m_Impl.upd()->setVertexBufferParams(n, format);
}

size_t osc::Mesh::getVertexBufferStride() const
{
    return m_Impl->getVertexBufferStride();
}

void osc::Mesh::setVertexBufferData(std::span<uint8_t const> data, MeshUpdateFlags flags)
{
    m_Impl.upd()->setVertexBufferData(data, flags);
}

void osc::Mesh::recalculateNormals()
{
    m_Impl.upd()->recalculateNormals();
}

void osc::Mesh::recalculateTangents()
{
    m_Impl.upd()->recalculateTangents();
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
    constexpr auto c_CameraProjectionStrings = std::to_array<CStringView>(
    {
        "Perspective",
        "Orthographic",
    });
    static_assert(c_CameraProjectionStrings.size() == NumOptions<CameraProjection>());
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

    Radians getVerticalFOV() const
    {
        return m_PerspectiveFov;
    }

    void setVerticalFOV(Radians size)
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

    Vec3 getPosition() const
    {
        return m_Position;
    }

    void setPosition(Vec3 const& position)
    {
        m_Position = position;
    }

    Quat getRotation() const
    {
        return m_Rotation;
    }

    void setRotation(Quat const& rotation)
    {
        m_Rotation = rotation;
    }

    Vec3 getDirection() const
    {
        return m_Rotation * Vec3{0.0f, 0.0f, -1.0f};
    }

    void setDirection(Vec3 const& d)
    {
        m_Rotation = rotation(Vec3{0.0f, 0.0f, -1.0f}, d);
    }

    Vec3 getUpwardsDirection() const
    {
        return m_Rotation * Vec3{0.0f, 1.0f, 0.0f};
    }

    Mat4 getViewMatrix() const
    {
        if (m_MaybeViewMatrixOverride)
        {
            return *m_MaybeViewMatrixOverride;
        }
        else
        {
            return look_at(m_Position, m_Position + getDirection(), getUpwardsDirection());
        }
    }

    std::optional<Mat4> getViewMatrixOverride() const
    {
        return m_MaybeViewMatrixOverride;
    }

    void setViewMatrixOverride(std::optional<Mat4> maybeViewMatrixOverride)
    {
        m_MaybeViewMatrixOverride = maybeViewMatrixOverride;
    }

    Mat4 getProjectionMatrix(float aspectRatio) const
    {
        if (m_MaybeProjectionMatrixOverride)
        {
            return *m_MaybeProjectionMatrixOverride;
        }
        else if (m_CameraProjection == CameraProjection::Perspective)
        {
            return perspective(
                m_PerspectiveFov,
                aspectRatio,
                m_NearClippingPlane,
                m_FarClippingPlane
            );
        }
        else
        {
            float const height = m_OrthographicSize;
            float const width = height * aspectRatio;

            float const right = 0.5f * width;
            float const left = -right;
            float const top = 0.5f * height;
            float const bottom = -top;

            return ortho(left, right, bottom, top, m_NearClippingPlane, m_FarClippingPlane);
        }
    }

    std::optional<Mat4> getProjectionMatrixOverride() const
    {
        return m_MaybeProjectionMatrixOverride;
    }

    void setProjectionMatrixOverride(std::optional<Mat4> maybeProjectionMatrixOverride)
    {
        m_MaybeProjectionMatrixOverride = maybeProjectionMatrixOverride;
    }

    Mat4 getViewProjectionMatrix(float aspectRatio) const
    {
        return getProjectionMatrix(aspectRatio) * getViewMatrix();
    }

    Mat4 getInverseViewProjectionMatrix(float aspectRatio) const
    {
        return inverse(getViewProjectionMatrix(aspectRatio));
    }

    void renderToScreen()
    {
        GraphicsBackend::RenderCameraQueue(*this);
    }

    void renderTo(RenderTexture& renderTexture)
    {
        static_assert(CameraClearFlags::All == (CameraClearFlags::SolidColor | CameraClearFlags::Depth));
        static_assert(NumOptions<RenderTextureReadWrite>() == 2);

        RenderTarget renderTargetThatWritesToRenderTexture
        {
            {
                RenderTargetColorAttachment
                {
                    // attach to render texture's color buffer
                    renderTexture.updColorBuffer(),

                    // load the color buffer based on this camera's clear flags
                    getClearFlags() & CameraClearFlags::SolidColor ?
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
                getClearFlags() & CameraClearFlags::Depth ?
                    RenderBufferLoadAction::Clear :
                    RenderBufferLoadAction::Load,

                RenderBufferStoreAction::DontCare,
            },
        };

        renderTo(renderTargetThatWritesToRenderTexture);
    }

    void renderTo(RenderTarget& renderTarget)
    {
        GraphicsBackend::RenderCameraQueue(*this, &renderTarget);
    }

    friend bool operator==(Impl const&, Impl const&) = default;

private:
    friend class GraphicsBackend;

    Color m_BackgroundColor = Color::clear();
    CameraProjection m_CameraProjection = CameraProjection::Perspective;
    float m_OrthographicSize = 2.0f;
    Radians m_PerspectiveFov = 90_deg;
    float m_NearClippingPlane = 1.0f;
    float m_FarClippingPlane = -1.0f;
    CameraClearFlags m_ClearFlags = CameraClearFlags::Default;
    std::optional<Rect> m_MaybeScreenPixelRect = std::nullopt;
    std::optional<Rect> m_MaybeScissorRect = std::nullopt;
    Vec3 m_Position = {};
    Quat m_Rotation = identity<Quat>();
    std::optional<Mat4> m_MaybeViewMatrixOverride;
    std::optional<Mat4> m_MaybeProjectionMatrixOverride;
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

void osc::Camera::reset()
{
    m_Impl.upd()->reset();
}

Color osc::Camera::getBackgroundColor() const
{
    return m_Impl->getBackgroundColor();
}

void osc::Camera::setBackgroundColor(Color const& color)
{
    m_Impl.upd()->setBackgroundColor(color);
}

CameraProjection osc::Camera::getCameraProjection() const
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

Radians osc::Camera::getVerticalFOV() const
{
    return m_Impl->getVerticalFOV();
}

void osc::Camera::setVerticalFOV(Radians vertical_fov)
{
    m_Impl.upd()->setVerticalFOV(vertical_fov);
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

CameraClearFlags osc::Camera::getClearFlags() const
{
    return m_Impl->getClearFlags();
}

void osc::Camera::setClearFlags(CameraClearFlags flags)
{
    m_Impl.upd()->setClearFlags(flags);
}

std::optional<Rect> osc::Camera::getPixelRect() const
{
    return m_Impl->getPixelRect();
}

void osc::Camera::setPixelRect(std::optional<Rect> maybePixelRect)
{
    m_Impl.upd()->setPixelRect(maybePixelRect);
}

std::optional<Rect> osc::Camera::getScissorRect() const
{
    return m_Impl->getScissorRect();
}

void osc::Camera::setScissorRect(std::optional<Rect> maybeScissorRect)
{
    m_Impl.upd()->setScissorRect(maybeScissorRect);
}

Vec3 osc::Camera::getPosition() const
{
    return m_Impl->getPosition();
}

void osc::Camera::setPosition(Vec3 const& p)
{
    m_Impl.upd()->setPosition(p);
}

Quat osc::Camera::getRotation() const
{
    return m_Impl->getRotation();
}

void osc::Camera::setRotation(Quat const& rotation)
{
    m_Impl.upd()->setRotation(rotation);
}

Vec3 osc::Camera::getDirection() const
{
    return m_Impl->getDirection();
}

void osc::Camera::setDirection(Vec3 const& d)
{
    m_Impl.upd()->setDirection(d);
}

Vec3 osc::Camera::getUpwardsDirection() const
{
    return m_Impl->getUpwardsDirection();
}

Mat4 osc::Camera::getViewMatrix() const
{
    return m_Impl->getViewMatrix();
}

std::optional<Mat4> osc::Camera::getViewMatrixOverride() const
{
    return m_Impl->getViewMatrixOverride();
}

void osc::Camera::setViewMatrixOverride(std::optional<Mat4> maybeViewMatrixOverride)
{
    m_Impl.upd()->setViewMatrixOverride(maybeViewMatrixOverride);
}

Mat4 osc::Camera::getProjectionMatrix(float aspectRatio) const
{
    return m_Impl->getProjectionMatrix(aspectRatio);
}

std::optional<Mat4> osc::Camera::getProjectionMatrixOverride() const
{
    return m_Impl->getProjectionMatrixOverride();
}

void osc::Camera::setProjectionMatrixOverride(std::optional<Mat4> maybeProjectionMatrixOverride)
{
    m_Impl.upd()->setProjectionMatrixOverride(maybeProjectionMatrixOverride);
}

Mat4 osc::Camera::getViewProjectionMatrix(float aspectRatio) const
{
    return m_Impl->getViewProjectionMatrix(aspectRatio);
}

Mat4 osc::Camera::getInverseViewProjectionMatrix(float aspectRatio) const
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

bool osc::operator==(Camera const& lhs, Camera const& rhs)
{
    return lhs.m_Impl == rhs.m_Impl || *lhs.m_Impl == *rhs.m_Impl;
}


/////////////////////////////
//
// graphics context
//
/////////////////////////////

namespace
{
    struct RequiredOpenGLCapability final {
        GLenum id;
        CStringView label;
    };
    constexpr auto c_RequiredOpenGLCapabilities = std::to_array<RequiredOpenGLCapability>(
    {
        // ensures geometry is occlusion-culled correctly
        {GL_DEPTH_TEST, "GL_DEPTH_TEST"},

        // used to reduce pixel aliasing (jaggies)
        {GL_MULTISAMPLE, "GL_MULTISAMPLE"},

        // enables linear color rendering workflow
        //
        // in oscar, shader calculations are done in linear space, but reads/writes
        // from framebuffers respect whether they are internally using an sRGB format
        {GL_FRAMEBUFFER_SRGB, "GL_FRAMEBUFFER_SRGB"},

        // enable seamless cubemap sampling when sampling
        //
        // handy in Physically Based Rendering (PBR) workflows, which do advanced rendering
        // tricks, like writing to specific mip levels in cubemaps for irradiance sampling etc.
        {GL_TEXTURE_CUBE_MAP_SEAMLESS, "GL_TEXTURE_CUBE_MAP_SEAMLESS"},
    });

    // create an OpenGL context for an application window
    sdl::GLContext CreateOpenGLContext(SDL_Window& window)
    {
        log_debug("initializing OpenGL context");

        // create an OpenGL context for the application
        sdl::GLContext ctx = sdl::GL_CreateContext(&window);

        // enable the OpenGL context
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

        // validate that the runtime OpenGL backend supports the extensions that OSC
        // relies on
        //
        // reports anything missing to the log at the provided log level
        ValidateOpenGLBackendExtensionSupport(LogLevel::debug);

        for (auto const& capability : c_RequiredOpenGLCapabilities)
        {
            glEnable(capability.id);
            if (!glIsEnabled(capability.id))
            {
                log_warn("failed to enable %s: this may cause rendering issues", capability.label.c_str());
            }
        }

        // print OpenGL information to console (handy for debugging user's rendering
        // issues)
        log_info(
            "OpenGL initialized: info: %s, %s, (%s), GLSL %s",
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER),
            glGetString(GL_VERSION),
            glGetString(GL_SHADING_LANGUAGE_VERSION)
        );

        return ctx;
    }

    // returns the maximum numbers of MSXAA antiAliasingLevel the active OpenGL context supports
    AntiAliasingLevel GetOpenGLMaxMSXAASamples(sdl::GLContext const&)
    {
        GLint v = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &v);
        return AntiAliasingLevel{v};
    }

    // maps an OpenGL debug message severity level to a log level
    constexpr LogLevel OpenGLDebugSevToLogLvl(GLenum sev)
    {
        switch (sev) {
        case GL_DEBUG_SEVERITY_HIGH:
            return LogLevel::err;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return LogLevel::warn;
        case GL_DEBUG_SEVERITY_LOW:
            return LogLevel::debug;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return LogLevel::trace;
        default:
            return LogLevel::info;
        }
    }

    // returns a string representation of an OpenGL debug message severity level
    constexpr CStringView OpenGLDebugSevToStrView(GLenum sev)
    {
        switch (sev) {
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
    constexpr CStringView OpenGLDebugSrcToStrView(GLenum src)
    {
        switch (src) {
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
    constexpr CStringView OpenGLDebugTypeToStrView(GLenum type)
    {
        switch (type) {
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
            GLint flags = 0;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT))
            {
                return false;
            }
        }

        {
            GLboolean b = GL_FALSE;
            glGetBooleanv(GL_DEBUG_OUTPUT, &b);
            if (!b)
            {
                return false;
            }
        }

        {
            GLboolean b = GL_FALSE;
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
        LogLevel const lvl = OpenGLDebugSevToLogLvl(severity);
        CStringView const sourceCStr = OpenGLDebugSrcToStrView(source);
        CStringView const typeCStr = OpenGLDebugTypeToStrView(type);
        CStringView const severityCStr = OpenGLDebugSevToStrView(severity);

        log_message(lvl,
            R"(OpenGL Debug message:
id = %u
message = %s
source = %s
type = %s
severity = %s
)", id, message, sourceCStr.c_str(), typeCStr.c_str(), severityCStr.c_str());
    }

    // enable OpenGL API debugging
    void EnableOpenGLDebugMessages()
    {
        if (IsOpenGLInDebugMode())
        {
            log_info("OpenGL debug mode appears to already be enabled: skipping enabling it");
            return;
        }

        GLint flags = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(OpenGLDebugMessageHandler, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            log_info("enabled OpenGL debug mode");
        }
        else
        {
            log_error("cannot enable OpenGL debug mode: the context does not have GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }

    // disable OpenGL API debugging
    void DisableOpenGLDebugMessages()
    {
        if (!IsOpenGLInDebugMode())
        {
            log_info("OpenGL debug mode appears to already be disabled: skipping disabling it");
            return;
        }

        GLint flags{};
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glDisable(GL_DEBUG_OUTPUT);
            log_info("disabled OpenGL debug mode");
        }
        else
        {
            log_error("cannot disable OpenGL debug mode: the context does not have a GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }
}

class osc::GraphicsContext::Impl final {
public:
    explicit Impl(SDL_Window& window) : m_GLContext{CreateOpenGLContext(window)}
    {
        m_QuadMaterial.setDepthTested(false);  // it's for fullscreen rendering
    }

    AntiAliasingLevel getMaxAntialiasingLevel() const
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

        log_info("enabling debug mode");
        EnableOpenGLDebugMessages();
        m_DebugModeEnabled = IsOpenGLInDebugMode();
    }
    void disableDebugMode()
    {
        if (!IsOpenGLInDebugMode())
        {
            return;  // already not in debug mode
        }

        log_info("disabling debug mode");
        DisableOpenGLDebugMessages();
        m_DebugModeEnabled = IsOpenGLInDebugMode();
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

    void* updRawGLContextHandleHACK()
    {
        return m_GLContext.get();
    }

    std::future<Texture2D> requestScreenshot()
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
            Vec2i const dims = App::get().dims();

            std::vector<uint8_t> pixels(static_cast<size_t>(4*dims.x*dims.y));
            OSC_ASSERT(IsAlignedAtLeast(pixels.data(), 4) && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
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

            Texture2D screenshot
            {
                dims,
                TextureFormat::RGBA32,
                ColorSpace::sRGB
            };
            screenshot.setPixelData(pixels);

            // copy image to requests [0..n-2]
            for (ptrdiff_t i = 0, len = std::ssize(m_ActiveScreenshotRequests)-1; i < len; ++i)
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
        return std::string{GLGetCStringView(GL_VENDOR)};
    }

    std::string getBackendRendererString() const
    {
        return std::string{GLGetCStringView(GL_RENDERER)};
    }

    std::string getBackendVersionString() const
    {
        return std::string{GLGetCStringView(GL_VERSION)};
    }

    std::string getBackendShadingLanguageVersionString() const
    {
        return std::string{GLGetCStringView(GL_SHADING_LANGUAGE_VERSION)};
    }

    Material const& getQuadMaterial() const
    {
        return m_QuadMaterial;
    }

    Mesh const& getQuadMesh() const
    {
        return m_QuadMesh;
    }

    std::vector<float>& updInstanceCPUBuffer()
    {
        return m_InstanceCPUBuffer;
    }

    gl::ArrayBuffer<float, GL_STREAM_DRAW>& updInstanceGPUBuffer()
    {
        return m_InstanceGPUBuffer;
    }

private:

    // active OpenGL context for the application
    sdl::GLContext m_GLContext;

    // maximum number of antiAliasingLevel supported by this hardware's OpenGL MSXAA API
    AntiAliasingLevel m_MaxMSXAASamples = GetOpenGLMaxMSXAASamples(m_GLContext);

    bool m_VSyncEnabled = SDL_GL_GetSwapInterval() != 0;

    // true if OpenGL's debug mode is enabled
    bool m_DebugModeEnabled = false;

    // a "queue" of active screenshot requests
    std::vector<std::promise<Texture2D>> m_ActiveScreenshotRequests;

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
    Mesh m_QuadMesh = GeneratePlaneMesh2(2.0f, 2.0f, 1, 1);

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

AntiAliasingLevel osc::GraphicsContext::getMaxAntialiasingLevel() const
{
    return g_GraphicsContextImpl->getMaxAntialiasingLevel();
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

void osc::GraphicsContext::clearScreen(Color const& color)
{
    g_GraphicsContextImpl->clearScreen(color);
}

void* osc::GraphicsContext::updRawGLContextHandleHACK()
{
    return g_GraphicsContextImpl->updRawGLContextHandleHACK();
}

void osc::GraphicsContext::doSwapBuffers(SDL_Window& window)
{
    g_GraphicsContextImpl->doSwapBuffers(window);
}

std::future<Texture2D> osc::GraphicsContext::requestScreenshot()
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
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock,
    std::optional<size_t> maybeSubMeshIndex)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, maybeMaterialPropertyBlock, maybeSubMeshIndex);
}

void osc::Graphics::DrawMesh(
    Mesh const& mesh,
    Mat4 const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock,
    std::optional<size_t> maybeSubMeshIndex)
{
    GraphicsBackend::DrawMesh(mesh, transform, material, camera, maybeMaterialPropertyBlock, maybeSubMeshIndex);
}

void osc::Graphics::Blit(Texture2D const& source, RenderTexture& dest)
{
    GraphicsBackend::Blit(source, dest);
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

void osc::Graphics::BlitToScreen(
    Texture2D const& t,
    Rect const& rect)
{
    GraphicsBackend::BlitToScreen(t, rect);
}

void osc::Graphics::CopyTexture(
    RenderTexture const& src,
    Texture2D& dest)
{
    GraphicsBackend::CopyTexture(src, dest);
}

void osc::Graphics::CopyTexture(
    RenderTexture const& src,
    Texture2D& dest,
    CubemapFace face)
{
    GraphicsBackend::CopyTexture(src, dest, face);
}

void osc::Graphics::CopyTexture(
    RenderTexture const& sourceRenderTexture,
    Cubemap& destinationCubemap,
    size_t mip)
{
    GraphicsBackend::CopyTexture(sourceRenderTexture, destinationCubemap, mip);
}

/////////////////////////
//
// backend implementation
//
/////////////////////////


// helper: binds to instanced attributes (per-drawcall)
void osc::GraphicsBackend::BindToInstancedAttributes(
        Shader::Impl const& shaderImpl,
        InstancingState& ins)
{
    gl::BindBuffer(ins.buf);

    size_t byteOffset = 0;
    if (shaderImpl.m_MaybeInstancedModelMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == ShaderPropertyType::Mat4)
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
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::VertexAttribPointer(mmtxAttr, false, ins.stride, ins.baseOffset + byteOffset);
            gl::VertexAttribDivisor(mmtxAttr, 1);
            gl::EnableVertexAttribArray(mmtxAttr);
            // unused: byteOffset += sizeof(float) * 16;
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat3)
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
        if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == ShaderPropertyType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat4)
        {
            gl::AttributeMat4 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat3)
        {
            gl::AttributeMat3 const mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::DisableVertexAttribArray(mmtxAttr);
        }
    }
}

// helper: upload instancing data for a batch
std::optional<InstancingState> osc::GraphicsBackend::UploadInstanceData(
    std::span<RenderObject const> renderObjects,
    Shader::Impl const& shaderImpl)
{
    // preemptively upload instancing data
    std::optional<InstancingState> maybeInstancingState;

    if (shaderImpl.m_MaybeInstancedModelMatAttr || shaderImpl.m_MaybeInstancedNormalMatAttr)
    {
        // compute the stride between each instance
        size_t byteStride = 0;
        if (shaderImpl.m_MaybeInstancedModelMatAttr)
        {
            if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == ShaderPropertyType::Mat4)
            {
                byteStride += sizeof(float) * 16;
            }
        }
        if (shaderImpl.m_MaybeInstancedNormalMatAttr)
        {
            if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat4)
            {
                byteStride += sizeof(float) * 16;
            }
            else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat3)
            {
                byteStride += sizeof(float) * 9;
            }
        }

        // write the instance data into a CPU-side buffer

        OSC_PERF("GraphicsBackend::UploadInstanceData");
        std::vector<float>& buf = g_GraphicsContextImpl->updInstanceCPUBuffer();
        buf.clear();
        buf.reserve(renderObjects.size() * (byteStride/sizeof(float)));

        size_t floatOffset = 0;
        for (RenderObject const& el : renderObjects)
        {
            if (shaderImpl.m_MaybeInstancedModelMatAttr)
            {
                if (shaderImpl.m_MaybeInstancedModelMatAttr->shaderType == ShaderPropertyType::Mat4)
                {
                    Mat4 const m = ModelMatrix(el);
                    std::span<float const> const els = ToFloatSpan(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    floatOffset += els.size();
                }
            }
            if (shaderImpl.m_MaybeInstancedNormalMatAttr)
            {
                if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat4)
                {
                    Mat4 const m = NormalMatrix4(el);
                    std::span<float const> const els = ToFloatSpan(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    floatOffset += els.size();
                }
                else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shaderType == ShaderPropertyType::Mat3)
                {
                    Mat3 const m = NormalMatrix(el);
                    std::span<float const> const els = ToFloatSpan(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    floatOffset += els.size();
                }
            }
        }
        OSC_ASSERT_ALWAYS(sizeof(float)*floatOffset == renderObjects.size() * byteStride);

        auto& vbo = maybeInstancingState.emplace(g_GraphicsContextImpl->updInstanceGPUBuffer(), byteStride).buf;
        vbo.assign(std::span<float const>{buf.data(), floatOffset});
    }
    return maybeInstancingState;
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
    case VariantIndex<MaterialValue, Color>():
    {
        // colors are converted from sRGB to linear when passed to
        // the shader

        Vec4 const linearColor = ToLinear(std::get<Color>(v));
        gl::UniformVec4 u{se.location};
        gl::Uniform(u, linearColor);
        break;
    }
    case VariantIndex<MaterialValue, std::vector<Color>>():
    {
        auto const& colors = std::get<std::vector<Color>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(colors.size()));

        if (numToAssign > 0)
        {
            // CARE: assigning to uniform arrays should be done in one `glUniform` call
            //
            // although many guides on the internet say it's valid to assign each array
            // element one-at-a-time by just calling the one-element version with `location + i`
            // I (AK) have encountered situations where some backends (e.g. MacOS) will behave
            // unusually if assigning this way
            //
            // so, for safety's sake, always upload arrays in one `glUniform*` call

            // CARE #2: colors should always be converted from sRGB-to-linear when passed to
            // a shader. OSC's rendering pipeline assumes that all color values in a shader
            // are linearized

            std::vector<Vec4> linearColors;
            linearColors.reserve(numToAssign);
            for (auto const& color : colors)
            {
                linearColors.emplace_back(ToLinear(color));
            }
            static_assert(sizeof(Vec4) == 4*sizeof(float));
            static_assert(alignof(Vec4) <= alignof(float));
            glUniform4fv(se.location, numToAssign, value_ptr(linearColors.front()));
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

        if (numToAssign > 0)
        {
            // CARE: assigning to uniform arrays should be done in one `glUniform` call
            //
            // although many guides on the internet say it's valid to assign each array
            // element one-at-a-time by just calling the one-element version with `location + i`
            // I (AK) have encountered situations where some backends (e.g. MacOS) will behave
            // unusually if assigning this way
            //
            // so, for safety's sake, always upload arrays in one `glUniform*` call

            glUniform1fv(se.location, numToAssign, vals.data());
        }
        break;
    }
    case VariantIndex<MaterialValue, Vec2>():
    {
        gl::UniformVec2 u{se.location};
        gl::Uniform(u, std::get<Vec2>(v));
        break;
    }
    case VariantIndex<MaterialValue, Vec3>():
    {
        gl::UniformVec3 u{se.location};
        gl::Uniform(u, std::get<Vec3>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<Vec3>>():
    {
        auto const& vals = std::get<std::vector<Vec3>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(vals.size()));

        if (numToAssign > 0)
        {
            // CARE: assigning to uniform arrays should be done in one `glUniform` call
            //
            // although many guides on the internet say it's valid to assign each array
            // element one-at-a-time by just calling the one-element version with `location + i`
            // I (AK) have encountered situations where some backends (e.g. MacOS) will behave
            // unusually if assigning this way
            //
            // so, for safety's sake, always upload arrays in one `glUniform*` call

            static_assert(sizeof(Vec3) == 3*sizeof(float));
            static_assert(alignof(Vec3) <= alignof(float));

            glUniform3fv(se.location, numToAssign, value_ptr(vals.front()));
        }
        break;
    }
    case VariantIndex<MaterialValue, Vec4>():
    {
        gl::UniformVec4 u{se.location};
        gl::Uniform(u, std::get<Vec4>(v));
        break;
    }
    case VariantIndex<MaterialValue, Mat3>():
    {
        gl::UniformMat3 u{se.location};
        gl::Uniform(u, std::get<Mat3>(v));
        break;
    }
    case VariantIndex<MaterialValue, Mat4>():
    {
        gl::UniformMat4 u{se.location};
        gl::Uniform(u, std::get<Mat4>(v));
        break;
    }
    case VariantIndex<MaterialValue, std::vector<Mat4>>():
    {
        auto const& vals = std::get<std::vector<Mat4>>(v);
        int32_t const numToAssign = std::min(se.size, static_cast<int32_t>(vals.size()));
        if (numToAssign > 0)
        {
            // CARE: assigning to uniform arrays should be done in one `glUniform` call
            //
            // although many guides on the internet say it's valid to assign each array
            // element one-at-a-time by just calling the one-element version with `location + i`
            // I (AK) have encountered situations where some backends (e.g. MacOS) will behave
            // unusually if assigning this way
            //
            // so, for safety's sake, always upload arrays in one `glUniform*` call

            static_assert(sizeof(Mat4) == 16*sizeof(float));
            static_assert(alignof(Mat4) <= alignof(float));
            glUniformMatrix4fv(se.location, numToAssign, GL_FALSE, value_ptr(vals.front()));
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
        static_assert(NumOptions<TextureDimensionality>() == 2);
        std::visit(Overload
            {
                [&textureSlot, &se](SingleSampledTexture& sst)
            {
                gl::ActiveTexture(GL_TEXTURE0 + textureSlot);
                gl::BindTexture(sst.texture2D);
                gl::UniformSampler2D u{se.location};
                gl::Uniform(u, textureSlot);
                ++textureSlot;
            },
            [&textureSlot, &se](MultisampledRBOAndResolvedTexture& mst)
            {
                gl::ActiveTexture(GL_TEXTURE0 + textureSlot);
                gl::BindTexture(mst.singleSampledTexture);
                gl::UniformSampler2D u{se.location};
                gl::Uniform(u, textureSlot);
                ++textureSlot;
            },
            [&textureSlot, &se](SingleSampledCubemap& cubemap)
            {
                gl::ActiveTexture(GL_TEXTURE0 + textureSlot);
                gl::BindTexture(cubemap.textureCubemap);
                gl::UniformSamplerCube u{se.location};
                gl::Uniform(u, textureSlot);
                ++textureSlot;
            },
            }, const_cast<RenderTexture::Impl&>(*std::get<RenderTexture>(v).m_Impl).getColorRenderBufferData());

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

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
//   - Mesh
//   - sub-Mesh index (can be std::nullopt, to mean 'the entire mesh')
void osc::GraphicsBackend::HandleBatchWithSameSubMesh(
    std::span<RenderObject const> els,
    std::optional<InstancingState>& ins)
{
    auto& meshImpl = const_cast<Mesh::Impl&>(*els.front().mesh.m_Impl);
    Shader::Impl const& shaderImpl = *els.front().material.m_Impl->m_Shader.m_Impl;
    std::optional<size_t> const maybeSubMeshIndex = els.front().maybeSubMeshIndex;

    gl::BindVertexArray(meshImpl.updVertexArray());

    if (shaderImpl.m_MaybeModelMatUniform || shaderImpl.m_MaybeNormalMatUniform)
    {
        // if the shader requires per-instance uniforms, then we *have* to render one
        // instance at a time

        for (RenderObject const& el : els)
        {
            // try binding to uModel (standard)
            if (shaderImpl.m_MaybeModelMatUniform)
            {
                if (shaderImpl.m_MaybeModelMatUniform->shaderType == ShaderPropertyType::Mat4)
                {
                    gl::UniformMat4 u{shaderImpl.m_MaybeModelMatUniform->location};
                    gl::Uniform(u, ModelMatrix(el));
                }
            }

            // try binding to uNormalMat (standard)
            if (shaderImpl.m_MaybeNormalMatUniform)
            {
                if (shaderImpl.m_MaybeNormalMatUniform->shaderType == ShaderPropertyType::Mat3)
                {
                    gl::UniformMat3 u{shaderImpl.m_MaybeNormalMatUniform->location};
                    gl::Uniform(u, NormalMatrix(el));
                }
                else if (shaderImpl.m_MaybeNormalMatUniform->shaderType == ShaderPropertyType::Mat4)
                {
                    gl::UniformMat4 u{shaderImpl.m_MaybeNormalMatUniform->location};
                    gl::Uniform(u, NormalMatrix4(el));
                }
            }

            if (ins)
            {
                BindToInstancedAttributes(shaderImpl, *ins);
            }
            meshImpl.drawInstanced(1, maybeSubMeshIndex);
            if (ins)
            {
                UnbindFromInstancedAttributes(shaderImpl, *ins);
                ins->baseOffset += 1 * ins->stride;
            }
        }
    }
    else
    {
        // else: the shader supports instanced data, so we can draw multiple meshes in one call

        if (ins)
        {
            BindToInstancedAttributes(shaderImpl, *ins);
        }
        meshImpl.drawInstanced(els.size(), maybeSubMeshIndex);
        if (ins)
        {
            UnbindFromInstancedAttributes(shaderImpl, *ins);
            ins->baseOffset += els.size() * ins->stride;
        }
    }

    gl::BindVertexArray();
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
//   - Mesh
void osc::GraphicsBackend::HandleBatchWithSameMesh(
    std::span<RenderObject const> els,
    std::optional<InstancingState>& ins)
{
    // batch by sub-Mesh index
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasSubMeshIndex{batchIt->maybeSubMeshIndex});
        HandleBatchWithSameSubMesh({batchIt, batchEnd}, ins);
        batchIt = batchEnd;
    }
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
void osc::GraphicsBackend::HandleBatchWithSameMaterialPropertyBlock(
    std::span<RenderObject const> els,
    int32_t& textureSlot,
    std::optional<InstancingState>& ins)
{
    OSC_PERF("GraphicsBackend::HandleBatchWithSameMaterialPropertyBlock");

    Material::Impl const& matImpl = *els.front().material.m_Impl;
    Shader::Impl const& shaderImpl = *matImpl.m_Shader.m_Impl;
    FastStringHashtable<ShaderElement> const& uniforms = shaderImpl.getUniforms();

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

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
void osc::GraphicsBackend::HandleBatchWithSameMaterial(
    RenderPassState const& renderPassState,
    std::span<RenderObject const> els)
{
    OSC_PERF("GraphicsBackend::HandleBatchWithSameMaterial");

    auto const& matImpl = *els.front().material.m_Impl;
    auto const& shaderImpl = *matImpl.m_Shader.m_Impl;
    FastStringHashtable<ShaderElement> const& uniforms = shaderImpl.getUniforms();

    // preemptively upload instance data
    std::optional<InstancingState> maybeInstances = UploadInstanceData(els, shaderImpl);

    // updated by various batches (which may bind to textures etc.)
    int32_t textureSlot = 0;

    gl::UseProgram(shaderImpl.getProgram());

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (matImpl.getDepthFunction() != DepthFunction::Default)
    {
        glDepthFunc(ToGLDepthFunc(matImpl.getDepthFunction()));
    }

    if (matImpl.getCullMode() != CullMode::Off)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(ToGLCullFaceEnum(matImpl.getCullMode()));

        // winding order is assumed to be counter-clockwise
        //
        // (it's the initial value as defined by Khronos: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFrontFace.xhtml)
        // glFrontFace(GL_CCW);
    }

    // bind material variables
    {
        // try binding to uView (standard)
        if (shaderImpl.m_MaybeViewMatUniform)
        {
            if (shaderImpl.m_MaybeViewMatUniform->shaderType == ShaderPropertyType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewMatUniform->location};
                gl::Uniform(u, renderPassState.viewMatrix);
            }
        }

        // try binding to uProjection (standard)
        if (shaderImpl.m_MaybeProjMatUniform)
        {
            if (shaderImpl.m_MaybeProjMatUniform->shaderType == ShaderPropertyType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeProjMatUniform->location};
                gl::Uniform(u, renderPassState.projectionMatrix);
            }
        }

        if (shaderImpl.m_MaybeViewProjMatUniform)
        {
            if (shaderImpl.m_MaybeViewProjMatUniform->shaderType == ShaderPropertyType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewProjMatUniform->location};
                gl::Uniform(u, renderPassState.viewProjectionMatrix);
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

    if (matImpl.getCullMode() != CullMode::Off)
    {
        glCullFace(GL_BACK);  // default from Khronos docs
        glDisable(GL_CULL_FACE);
    }

    if (matImpl.getDepthFunction() != DepthFunction::Default)
    {
        glDepthFunc(ToGLDepthFunc(DepthFunction::Default));
    }

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// helper: draw a sequence of `RenderObject`s
void osc::GraphicsBackend::DrawRenderObjects(
    RenderPassState const& renderPassState,
    std::span<RenderObject const> els)
{
    OSC_PERF("GraphicsBackend::DrawRenderObjects");

    // batch by material
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        auto const batchEnd = std::find_if_not(batchIt, els.end(), RenderObjectHasMaterial{&batchIt->material});
        HandleBatchWithSameMaterial(renderPassState, {batchIt, batchEnd});
        batchIt = batchEnd;
    }
}

void osc::GraphicsBackend::DrawBatchedByOpaqueness(
    RenderPassState const& renderPassState,
    std::span<RenderObject const> els)
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
            DrawRenderObjects(renderPassState, {batchIt, opaqueEnd});

            batchIt = opaqueEnd;
        }

        if (opaqueEnd != els.end())
        {
            // [opaqueEnd..els.end()] contains transparent elements
            auto const transparentEnd = std::find_if(opaqueEnd, els.end(), IsOpaque);
            gl::Enable(GL_BLEND);
            DrawRenderObjects(renderPassState, {opaqueEnd, transparentEnd});

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

    // precompute any render pass state used by the rendering algs
    RenderPassState const renderPassState
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

            SortRenderQueue(batchIt, depthTestedEnd, renderPassState.cameraPos);
            DrawBatchedByOpaqueness(renderPassState, {batchIt, depthTestedEnd});

            batchIt = depthTestedEnd;
        }

        if (depthTestedEnd != queue.end())
        {
            // there are >0 not-depth-tested elements that cannot be reordered

            auto const ignoreDepthTestEnd = std::find_if(depthTestedEnd, queue.end(), IsDepthTested);

            // these elements aren't depth-tested and should just be drawn as-is
            gl::Disable(GL_DEPTH_TEST);
            DrawBatchedByOpaqueness(renderPassState, {depthTestedEnd, ignoreDepthTestEnd});
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
    OSC_ASSERT(!renderTarget.colors.empty() && "a render target must have one or more color attachments");

    OSC_ASSERT(renderTarget.colors.front().buffer != nullptr && "a color attachment must have a non-null render buffer");
    Vec2i const firstColorBufferDimensions = renderTarget.colors.front().buffer->m_Impl->getDimensions();
    AntiAliasingLevel const firstColorBufferSamples = renderTarget.colors.front().buffer->m_Impl->getAntialiasingLevel();

    // validate other buffers against the first
    for (auto it = renderTarget.colors.begin()+1; it != renderTarget.colors.end(); ++it)
    {
        RenderTargetColorAttachment const& colorAttachment = *it;
        OSC_ASSERT(colorAttachment.buffer != nullptr);
        OSC_ASSERT(colorAttachment.buffer->m_Impl->getDimensions() == firstColorBufferDimensions);
        OSC_ASSERT(colorAttachment.buffer->m_Impl->getAntialiasingLevel() == firstColorBufferSamples);
    }
    OSC_ASSERT(renderTarget.depth.buffer != nullptr);
    OSC_ASSERT(renderTarget.depth.buffer->m_Impl->getDimensions() == firstColorBufferDimensions);
    OSC_ASSERT(renderTarget.depth.buffer->m_Impl->getAntialiasingLevel() == firstColorBufferSamples);
}

Rect osc::GraphicsBackend::CalcViewportRect(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    Vec2 const targetDims = maybeCustomRenderTarget ?
        Vec2{maybeCustomRenderTarget->colors.front().buffer->m_Impl->getDimensions()} :
        App::get().dims();

    Rect const cameraRect = camera.getPixelRect() ?
        *camera.getPixelRect() :
        Rect{{}, targetDims};

    Vec2 const cameraRectBottomLeft = BottomLeft(cameraRect);
    Vec2 const outputDimensions = dimensions(cameraRect);
    Vec2 const topLeft = {cameraRectBottomLeft.x, targetDims.y - cameraRectBottomLeft.y};

    return Rect{topLeft, topLeft + outputDimensions};
}

Rect osc::GraphicsBackend::SetupTopLevelPipelineState(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    Rect const viewportRect = CalcViewportRect(camera, maybeCustomRenderTarget);
    Vec2 const viewportDims = dimensions(viewportRect);

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
        Vec2i const scissorDims = dimensions(scissorRect);

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
        // caller wants to render to a custom render target of `n` color
        // buffers and a single depth buffer. Bind them all to one MRT FBO

        gl::FrameBuffer& rendererFBO = maybeRenderFBO.emplace();
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, rendererFBO);

        // attach color buffers to the FBO
        for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
        {
            std::visit(Overload
            {
                [i](SingleSampledTexture& t)
                {
                    gl::FramebufferTexture2D(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                        t.texture2D,
                        0
                    );
                },
                [i](MultisampledRBOAndResolvedTexture& t)
                {
                    gl::FramebufferRenderbuffer(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                        t.multisampledRBO
                    );
                },
#ifdef EMSCRIPTEN
                [](SingleSampledCubemap&) {}
#else
                [i](SingleSampledCubemap& t)
                {
                    glFramebufferTexture(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                        t.textureCubemap.get(),
                        0
                    );
                }
#endif
            }, maybeCustomRenderTarget->colors[i].buffer->m_Impl->updRenderBufferData());
        }

        // attach depth buffer to the FBO
        std::visit(Overload
        {
            [](SingleSampledTexture& t)
            {
                gl::FramebufferTexture2D(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    t.texture2D,
                    0
                );
            },
            [](MultisampledRBOAndResolvedTexture& t)
            {
                gl::FramebufferRenderbuffer(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    t.multisampledRBO
                );
            },
#ifdef EMSCRIPTEN
            [](SingleSampledCubemap&) {}
#else
            [](SingleSampledCubemap& t)
            {
                glFramebufferTexture(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    t.textureCubemap.get(),
                    0
                );
            }
#endif
        }, maybeCustomRenderTarget->depth.buffer->m_Impl->updRenderBufferData());

        // Multi-Render Target (MRT) support: tell OpenGL to use all specified
        // render targets when drawing and/or clearing
        {
            size_t const numColorAttachments = maybeCustomRenderTarget->colors.size();

            std::vector<GLenum> attachments;
            attachments.reserve(numColorAttachments);
            for (size_t i = 0; i < numColorAttachments; ++i)
            {
                attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i));
            }
            glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());
        }

        // if requested, clear the buffers
        {
            static_assert(NumOptions<RenderBufferLoadAction>() == 2);

            // if requested, clear color buffers
            for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
            {
                RenderTargetColorAttachment& colorAttachment = maybeCustomRenderTarget->colors[i];
                if (colorAttachment.loadAction == RenderBufferLoadAction::Clear)
                {
                    glClearBufferfv(
                        GL_COLOR,
                        static_cast<GLint>(i),
                        value_ptr(static_cast<Vec4>(colorAttachment.clearColor))
                    );
                }
            }

            // if requested, clear depth buffer
            if (maybeCustomRenderTarget->depth.loadAction == RenderBufferLoadAction::Clear)
            {
                gl::Clear(GL_DEPTH_BUFFER_BIT);
            }
        }
    }
    else
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);

        // we're rendering to the window
        if (camera.m_ClearFlags != CameraClearFlags::Nothing)
        {
            // clear window
            GLenum const clearFlags = camera.m_ClearFlags & CameraClearFlags::SolidColor ?
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
    static_assert(NumOptions<RenderBufferStoreAction>() == 2, "check 'if's etc. in this code");

    OSC_PERF("RenderTexture::resolveBuffers");

    // setup FBOs (reused per color buffer)
    gl::FrameBuffer multisampledReadFBO;
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, multisampledReadFBO);

    gl::FrameBuffer resolvedDrawFBO;
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedDrawFBO);

    // resolve each color buffer with a blit
    for (size_t i = 0; i < renderTarget.colors.size(); ++i)
    {
        RenderTargetColorAttachment const& attachment = renderTarget.colors[i];
        RenderBuffer& buffer = *attachment.buffer;
        RenderBufferOpenGLData& bufferOpenGLData = buffer.m_Impl->updRenderBufferData();

        if (attachment.storeAction != RenderBufferStoreAction::Resolve)
        {
            continue;  // we don't need to resolve this color buffer
        }

        bool bufferIsResolveable = false;  // changes if the underlying buffer data is resolve-able
        std::visit(Overload
        {
            [](SingleSampledTexture&)
            {
                // don't resolve: it's single-sampled
            },
            [&bufferIsResolveable, i](MultisampledRBOAndResolvedTexture& t)
            {
                GLint const attachmentLoc = GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i);

                gl::FramebufferRenderbuffer(
                    GL_READ_FRAMEBUFFER,
                    attachmentLoc,
                    t.multisampledRBO
                );
                glReadBuffer(attachmentLoc);

                gl::FramebufferTexture2D(
                    GL_DRAW_FRAMEBUFFER,
                    attachmentLoc,
                    t.singleSampledTexture,
                    0
                );
                glDrawBuffer(attachmentLoc);

                bufferIsResolveable = true;
            },
            [](SingleSampledCubemap&)
            {
                // don't resolve: it's single-sampled
            }
        }, bufferOpenGLData);

        if (bufferIsResolveable)
        {
            Vec2i const dimensions = attachment.buffer->m_Impl->getDimensions();
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
        bool bufferIsResolveable = false;  // changes if the underlying buffer data is resolve-able
        std::visit(Overload
        {
            [](SingleSampledTexture&)
            {
                // don't resolve: it's single-sampled
            },
            [&bufferIsResolveable](MultisampledRBOAndResolvedTexture& t)
            {
                gl::FramebufferRenderbuffer(
                    GL_READ_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    t.multisampledRBO
                );
                glReadBuffer(GL_DEPTH_ATTACHMENT);

                gl::FramebufferTexture2D(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    t.singleSampledTexture,
                    0
                );
                glDrawBuffer(GL_DEPTH_ATTACHMENT);

                bufferIsResolveable = true;
            },
            [](SingleSampledCubemap&)
            {
                // don't resolve: it's single-sampled
            }
        }, renderTarget.depth.buffer->m_Impl->updRenderBufferData());

        if (bufferIsResolveable)
        {
            Vec2i const dimensions = renderTarget.depth.buffer->m_Impl->getDimensions();
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
}

void osc::GraphicsBackend::RenderCameraQueue(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    OSC_PERF("GraphicsBackend::RenderCameraQueue");

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
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock,
    std::optional<size_t> maybeSubMeshIndex)
{
    if (maybeSubMeshIndex && *maybeSubMeshIndex >= mesh.getSubMeshCount())
    {
        throw std::out_of_range{"the given sub-mesh index was out of range (i.e. the given mesh does not have that many sub-meshes)"};
    }

    camera.m_Impl.upd()->m_RenderQueue.emplace_back(
        mesh,
        transform,
        material,
        maybeMaterialPropertyBlock,
        maybeSubMeshIndex
    );
}

void osc::GraphicsBackend::DrawMesh(
    Mesh const& mesh,
    Mat4 const& transform,
    Material const& material,
    Camera& camera,
    std::optional<MaterialPropertyBlock> const& maybeMaterialPropertyBlock,
    std::optional<size_t> maybeSubMeshIndex)
{
    if (maybeSubMeshIndex && *maybeSubMeshIndex >= mesh.getSubMeshCount())
    {
        throw std::out_of_range{"the given sub-mesh index was out of range (i.e. the given mesh does not have that many sub-meshes)"};
    }

    camera.m_Impl.upd()->m_RenderQueue.emplace_back(
        mesh,
        transform,
        material,
        maybeMaterialPropertyBlock,
        maybeSubMeshIndex
    );
}

void osc::GraphicsBackend::Blit(
    Texture2D const& source,
    RenderTexture& dest)
{
    Camera c;
    c.setBackgroundColor(Color::clear());
    c.setProjectionMatrixOverride(identity<Mat4>());
    c.setViewMatrixOverride(identity<Mat4>());

    Material m = g_GraphicsContextImpl->getQuadMaterial();
    m.setTexture("uTexture", source);

    Graphics::DrawMesh(g_GraphicsContextImpl->getQuadMesh(), Transform{}, m, c);
    c.renderTo(dest);
}

void osc::GraphicsBackend::BlitToScreen(
    RenderTexture const& t,
    Rect const& rect,
    BlitFlags flags)
{
    BlitToScreen(t, rect, g_GraphicsContextImpl->getQuadMaterial(), flags);
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
    c.setProjectionMatrixOverride(identity<Mat4>());
    c.setViewMatrixOverride(identity<Mat4>());
    c.setClearFlags(CameraClearFlags::Nothing);

    Material copy{material};
    copy.setRenderTexture("uTexture", t);
    Graphics::DrawMesh(g_GraphicsContextImpl->getQuadMesh(), Transform{}, copy, c);
    c.renderToScreen();
    copy.clearRenderTexture("uTexture");
}

void osc::GraphicsBackend::BlitToScreen(
    Texture2D const& t,
    Rect const& rect)
{
    OSC_ASSERT(g_GraphicsContextImpl);

    Camera c;
    c.setBackgroundColor(Color::clear());
    c.setPixelRect(rect);
    c.setProjectionMatrixOverride(identity<Mat4>());
    c.setViewMatrixOverride(identity<Mat4>());
    c.setClearFlags(CameraClearFlags::Nothing);

    Material copy{g_GraphicsContextImpl->getQuadMaterial()};
    copy.setTexture("uTexture", t);
    Graphics::DrawMesh(g_GraphicsContextImpl->getQuadMesh(), Transform{}, copy, c);
    c.renderToScreen();
    copy.clearTexture("uTexture");
}

void osc::GraphicsBackend::CopyTexture(
    RenderTexture const& src,
    Texture2D& dest)
{
    CopyTexture(src, dest, CubemapFace::PositiveX);
}

void osc::GraphicsBackend::CopyTexture(
    RenderTexture const& src,
    Texture2D& dest,
    CubemapFace face)
{
    OSC_ASSERT(g_GraphicsContextImpl);
    OSC_ASSERT(src.m_Impl->hasBeenRenderedTo() && "the input texture has not been rendered to");

    // create a source (read) framebuffer for blitting from the source render texture
    gl::FrameBuffer readFBO;
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
    std::visit(Overload  // attach source texture depending on rendertexture's type
    {
        [](SingleSampledTexture& t)
        {
            gl::FramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                t.texture2D,
                0
            );
        },
        [](MultisampledRBOAndResolvedTexture& t)
        {
            gl::FramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                t.singleSampledTexture,
                0
            );
        },
        [face](SingleSampledCubemap& t)
        {
            glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                ToOpenGLTextureEnum(face),
                t.textureCubemap.get(),
                0
            );
        }
    }, const_cast<RenderTexture::Impl&>(*src.m_Impl).getColorRenderBufferData());
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // create a destination (draw) framebuffer for blitting to the destination render texture
    gl::FrameBuffer drawFBO;
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
    gl::FramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        dest.m_Impl.upd()->updTexture(),
        0
    );
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // blit the read framebuffer to the draw framebuffer
    gl::BlitFramebuffer(
        0,
        0,
        src.getDimensions().x,
        src.getDimensions().y,
        0,
        0,
        dest.getDimensions().x,
        dest.getDimensions().y,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR  // the two texture may have different dimensions (avoid GL_NEAREST)
    );

    // then download the blitted data into the texture's CPU buffer
    {
        std::vector<uint8_t>& cpuBuffer = dest.m_Impl.upd()->m_PixelData;
        GLint const packFormat = ToImagePixelPackAlignment(dest.getTextureFormat());

        OSC_ASSERT(IsAlignedAtLeast(cpuBuffer.data(), packFormat) && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(cpuBuffer.size() == static_cast<ptrdiff_t>(dest.getDimensions().x*dest.getDimensions().y)*NumBytesPerPixel(dest.getTextureFormat()));

        gl::Viewport(0, 0, dest.getDimensions().x, dest.getDimensions().y);
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, drawFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::PixelStorei(GL_PACK_ALIGNMENT, packFormat);
        glReadPixels(
            0,
            0,
            dest.getDimensions().x,
            dest.getDimensions().y,
            ToImageColorFormat(dest.getTextureFormat()),
            ToImageDataType(dest.getTextureFormat()),
            cpuBuffer.data()
        );
    }
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
}

void osc::GraphicsBackend::CopyTexture(
    RenderTexture const& sourceRenderTexture,
    Cubemap& destinationCubemap,
    size_t mip)
{
    // from: https://registry.khronos.org/OpenGL-Refpages/es2.0/xhtml/glTexParameter.xml
    //
    // > To define the mipmap levels, call glTexImage2D, glCompressedTexImage2D, or glCopyTexImage2D
    // > with the level argument indicating the order of the mipmaps. Level 0 is the original texture;
    // > level floor(log2(max(w, h))) is the final 1 x 1 mipmap.
    //
    // related:
    //
    // - https://registry.khronos.org/OpenGL-Refpages/es2.0/xhtml/glTexImage2D.xml
    size_t const maxMipmapLevel = static_cast<size_t>(std::max(
        0,
        cpp20::bit_width(static_cast<size_t>(destinationCubemap.getWidth())) - 1
    ));

    OSC_ASSERT(sourceRenderTexture.getDimensionality() == TextureDimensionality::Cube && "provided render texture must be a cubemap to call this method");
    OSC_ASSERT(mip <= maxMipmapLevel);

    // blit each face of the source cubemap into the output cubemap
    for (size_t face = 0; face < 6; ++face)
    {
        gl::FrameBuffer readFBO;
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
        std::visit(Overload  // attach source texture depending on rendertexture's type
        {
            [](SingleSampledTexture&)
            {
                OSC_ASSERT(false && "cannot call CopyTexture (Cubemap --> Cubemap) with a 2D render");
            },
            [](MultisampledRBOAndResolvedTexture&)
            {
                OSC_ASSERT(false && "cannot call CopyTexture (Cubemap --> Cubemap) with a 2D render");
            },
            [face](SingleSampledCubemap& t)
            {
                glFramebufferTexture2D(
                    GL_READ_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face),
                    t.textureCubemap.get(),
                    0
                );
            }
        }, const_cast<RenderTexture::Impl&>(*sourceRenderTexture.m_Impl).getColorRenderBufferData());
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        gl::FrameBuffer drawFBO;
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face),
            destinationCubemap.m_Impl.upd()->updCubemap().get(),
            static_cast<GLint>(mip)
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        // blit the read framebuffer to the draw framebuffer
        gl::BlitFramebuffer(
            0,
            0,
            sourceRenderTexture.getDimensions().x,
            sourceRenderTexture.getDimensions().y,
            0,
            0,
            destinationCubemap.getWidth() / (1<<mip),
            destinationCubemap.getWidth() / (1<<mip),
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR  // the two texture may have different dimensions (avoid GL_NEAREST)
        );
    }

    // TODO: should be copied into CPU memory if mip==0? (won't store mipmaps in the CPU but
    // maybe it makes sense to store the mip==0 in CPU?)
}
