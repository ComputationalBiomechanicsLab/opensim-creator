#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/CameraClearFlags.h>
#include <oscar/Graphics/CameraProjection.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/ColorRenderBufferParams.h>
#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/DepthFunction.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Graphics/DepthStencilRenderBufferParams.h>
#include <oscar/Graphics/Detail/CPUDataType.h>
#include <oscar/Graphics/Detail/CPUImageFormat.h>
#include <oscar/Graphics/Detail/DepthStencilRenderBufferFormatHelpers.h>
#include <oscar/Graphics/Detail/ShaderPropertyTypeList.h>
#include <oscar/Graphics/Detail/ShaderPropertyTypeTraits.h>
#include <oscar/Graphics/Detail/TextureFormatList.h>
#include <oscar/Graphics/Detail/TextureFormatTraits.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatHelpers.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatList.h>
#include <oscar/Graphics/Detail/VertexAttributeFormatTraits.h>
#include <oscar/Graphics/Detail/VertexAttributeHelpers.h>
#include <oscar/Graphics/Detail/VertexAttributeList.h>
#include <oscar/Graphics/Geometries/PlaneGeometry.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/GraphicsContext.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshFunctions.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/OpenGL/CPUDataTypeOpenGLTraits.h>
#include <oscar/Graphics/OpenGL/CPUImageFormatOpenGLTraits.h>
#include <oscar/Graphics/OpenGL/DepthStencilRenderBufferFormatOpenGLHelpers.h>
#include <oscar/Graphics/OpenGL/Gl.h>
#include <oscar/Graphics/OpenGL/TextureFormatOpenGLTraits.h>
#include <oscar/Graphics/ColorRenderBufferFormat.h>
#include <oscar/Graphics/RenderBufferLoadAction.h>
#include <oscar/Graphics/RenderBufferStoreAction.h>
#include <oscar/Graphics/RenderTarget.h>
#include <oscar/Graphics/RenderTargetColorAttachment.h>
#include <oscar/Graphics/RenderTargetDepthStencilAttachment.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/RenderTextureParams.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Graphics/SharedColorRenderBuffer.h>
#include <oscar/Graphics/SharedDepthStencilRenderBuffer.h>
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
#include <oscar/Maths/AABBFunctions.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/RectFunctions.h>
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
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Concepts.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/DefaultConstructOnCopy.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/ObjectRepresentation.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/TransparentStringHasher.h>
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
#include <regex>
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
namespace rgs = std::ranges;

// shader source
namespace
{
    // vertex shader source used for blitting a textured quad (common use-case)
    //
    // it's here, rather than in an external resource file, because it is eagerly
    // loaded while the graphics backend is initialized (i.e. potentially before
    // the application is fully loaded)
    constexpr CStringView c_quad_vertex_shader_src = R"(
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
    constexpr CStringView c_quad_fragment_shader_src = R"(
        #version 330 core

        uniform sampler2D uTexture;

        in vec2 TexCoord;
        out vec4 FragColor;

        void main()
        {
            FragColor = texture(uTexture, TexCoord);
        }
    )";

    CStringView opengl_string_to_cstringview(const GLubyte* string_ptr)
    {
        using value_type = CStringView::value_type;

        static_assert(sizeof(GLubyte) == sizeof(value_type));
        static_assert(alignof(GLubyte) == alignof(value_type));
        static_assert(std::is_same_v<value_type, char>, "therefore, the cast below should be ok");
        if (string_ptr) {
            return CStringView{std::launder(reinterpret_cast<const char*>(string_ptr))};  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }
        else {
            return CStringView{};
        }
    }

    CStringView opengl_get_cstringview(GLenum name)
    {
        return opengl_string_to_cstringview(glGetString(name));
    }

    CStringView opengl_get_cstringviewi(GLenum name, GLuint index)
    {
        return opengl_string_to_cstringview(glGetStringi(name, index));
    }

    bool is_aligned_at_least(const void* ptr, GLint required_alignment)
    {
        return cpp20::bit_cast<intptr_t>(ptr) % required_alignment == 0;
    }

    // returns the name strings of all extensions that the OpenGL backend may use
    std::vector<CStringView> get_all_opengl_extensions_used_by_opengl_backend()
    {
        // most entries in this list were initially from a mixture of:
        //
        // - https://www.khronos.org/opengl/wiki/History_of_OpenGL (lists historical extension changes)
        // - Khronos official pages

        // this list isn't comprehensive, it's just things that I reckon the OpenGL backend
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

            // separate blend functions (might be handy with pre-multiplied alpha at some point)
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

    size_t get_num_extensions_supported_by_opengl_backend()
    {
        GLint rv = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &rv);
        return rv >= 0 ? static_cast<size_t>(rv) : 0;
    }

    std::vector<CStringView> get_extensions_supported_by_opengl_backend()
    {
        const size_t num_extensions = get_num_extensions_supported_by_opengl_backend();

        std::vector<CStringView> rv;
        rv.reserve(num_extensions);
        for (size_t i = 0; i < num_extensions; ++i) {
            rv.emplace_back(opengl_get_cstringviewi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        }
        return rv;
    }

    void validate_opengl_backend_extension_support(LogLevel logging_level)
    {
        // note: the OpenGL specification _requires_ that a backend supports
        // (effectively) RGBA, RG, and RED textures with the following data
        // formats for each component:
        //
        // - Unorm8
        // - Snorm8
        // - float
        // - uint8/uint16/uint32 (non-normalized)
        // - int8/int16/int32 (non-normalized)
        //
        // see "Required Formats" in: https://www.khronos.org/opengl/wiki/Image_Format

        if (logging_level < log_level()) {
            return;
        }

        std::vector<CStringView> extensions_needed = get_all_opengl_extensions_used_by_opengl_backend();
        rgs::sort(extensions_needed);

        std::vector<CStringView> extensions_available = get_extensions_supported_by_opengl_backend();
        rgs::sort(extensions_available);

        std::vector<CStringView> extensions_missing;
        extensions_missing.reserve(extensions_needed.size());  // pessimistic guess
        rgs::set_difference(extensions_needed, extensions_available, std::back_inserter(extensions_missing));

        if (not extensions_missing.empty()) {
            log_message(logging_level, "OpenGL: the following OpenGL extensions may be missing from the graphics backend: ");
            for (const auto& extension : extensions_missing) {
                log_message(logging_level, "OpenGL:  - %s", extension.c_str());
            }
            log_message(logging_level, "OpenGL: because extensions may be missing, rendering may behave abnormally");
            log_message(logging_level, "OpenGL: note: some graphics engines can mis-report an extension as missing");
        }

        log_message(logging_level, "OpenGL: here is a list of all of the extensions supported by the graphics backend:");
        for (const auto& extension : extensions_available) {
            log_message(logging_level, "OpenGL:  - %s", extension.c_str());
        }
    }
}


// generic utility functions
namespace
{
    template<BitCastable T>
    void push_as_bytes(const T& v, std::vector<uint8_t>& out)
    {
        const auto bytes = view_object_representation<uint8_t>(v);
        out.insert(out.end(), bytes.begin(), bytes.end());
    }

    template<typename VecOrMat>
    requires BitCastable<typename VecOrMat::element_type>
    std::span<const typename VecOrMat::element_type> to_float_span(const VecOrMat& v)
    {
        return {value_ptr(v), sizeof(VecOrMat)/sizeof(typename VecOrMat::element_type)};
    }
}

// blending functions
namespace
{
    GLenum to_opengl_blend_func(SourceBlendingFactor f)
    {
        static_assert(num_options<SourceBlendingFactor>() == 4);
        switch (f) {
        case SourceBlendingFactor::One:                 return GL_ONE;
        case SourceBlendingFactor::Zero:                return GL_ZERO;
        case SourceBlendingFactor::SourceAlpha:         return GL_SRC_ALPHA;
        case SourceBlendingFactor::OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        default:                                        return GL_ONE;
        }
    }

    GLenum to_opengl_blend_func(DestinationBlendingFactor f)
    {
        static_assert(num_options<DestinationBlendingFactor>() == 4);
        switch (f) {
        case DestinationBlendingFactor::One:                 return GL_ONE;
        case DestinationBlendingFactor::Zero:                return GL_ZERO;
        case DestinationBlendingFactor::SourceAlpha:         return GL_SRC_ALPHA;
        case DestinationBlendingFactor::OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        default:                                             return GL_ONE;
        }
    }

    GLenum to_opengl_blend_equation(BlendingEquation f)
    {
        static_assert(num_options<BlendingEquation>() == 3);
        switch (f) {
        case BlendingEquation::Add: return GL_FUNC_ADD;
        case BlendingEquation::Min: return GL_MIN;
        case BlendingEquation::Max: return GL_MAX;
        default:                    return GL_FUNC_ADD;
        }
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
        int,
        bool,
        Texture2D,
        RenderTexture,
        Cubemap,
        SharedColorRenderBuffer,
        SharedDepthStencilRenderBuffer
    >;

    ShaderPropertyType get_shader_type(const MaterialValue& material_val)
    {
        static_assert(std::variant_size_v<MaterialValue> == 18);

        switch (material_val.index()) {
        case variant_index<MaterialValue, Color>():
        case variant_index<MaterialValue, std::vector<Color>>():
            return ShaderPropertyType::Vec4;
        case variant_index<MaterialValue, Vec2>():
            return ShaderPropertyType::Vec2;
        case variant_index<MaterialValue, float>():
        case variant_index<MaterialValue, std::vector<float>>():
            return ShaderPropertyType::Float;
        case variant_index<MaterialValue, Vec3>():
        case variant_index<MaterialValue, std::vector<Vec3>>():
            return ShaderPropertyType::Vec3;
        case variant_index<MaterialValue, Vec4>():
            return ShaderPropertyType::Vec4;
        case variant_index<MaterialValue, Mat3>():
            return ShaderPropertyType::Mat3;
        case variant_index<MaterialValue, Mat4>():
        case variant_index<MaterialValue, std::vector<Mat4>>():
            return ShaderPropertyType::Mat4;
        case variant_index<MaterialValue, int>():
            return ShaderPropertyType::Int;
        case variant_index<MaterialValue, bool>():
            return ShaderPropertyType::Bool;
        case variant_index<MaterialValue, Texture2D>():
            return ShaderPropertyType::Sampler2D;
        case variant_index<MaterialValue, RenderTexture>(): {

            static_assert(num_options<TextureDimensionality>() == 2);
            return std::get<RenderTexture>(material_val).dimensionality() == TextureDimensionality::Tex2D ?
                ShaderPropertyType::Sampler2D :
                ShaderPropertyType::SamplerCube;
        }
        case variant_index<MaterialValue, Cubemap>():
            return ShaderPropertyType::SamplerCube;
        case variant_index<MaterialValue, SharedColorRenderBuffer>(): {
            static_assert(num_options<TextureDimensionality>() == 2);
            return std::get<SharedColorRenderBuffer>(material_val).dimensionality() == TextureDimensionality::Tex2D ?
                ShaderPropertyType::Sampler2D :
                ShaderPropertyType::SamplerCube;
        }
        case variant_index<MaterialValue, SharedDepthStencilRenderBuffer>(): {
            static_assert(num_options<TextureDimensionality>() == 2);
            return std::get<SharedDepthStencilRenderBuffer>(material_val).dimensionality() == TextureDimensionality::Tex2D ?
                ShaderPropertyType::Sampler2D :
                ShaderPropertyType::SamplerCube;
        }
        default:
            return ShaderPropertyType::Unknown;
        }
    }
}

// shader (backend stuff)
namespace
{
    // convert a GL shader type to an internal shader type
    ShaderPropertyType opengl_shader_type_to_osc_shader_type(GLenum e)
    {
        static_assert(num_options<ShaderPropertyType>() == 11);

        switch (e) {
        case GL_FLOAT:        return ShaderPropertyType::Float;
        case GL_FLOAT_VEC2:   return ShaderPropertyType::Vec2;
        case GL_FLOAT_VEC3:   return ShaderPropertyType::Vec3;
        case GL_FLOAT_VEC4:   return ShaderPropertyType::Vec4;
        case GL_FLOAT_MAT3:   return ShaderPropertyType::Mat3;
        case GL_FLOAT_MAT4:   return ShaderPropertyType::Mat4;
        case GL_INT:          return ShaderPropertyType::Int;
        case GL_BOOL:         return ShaderPropertyType::Bool;
        case GL_SAMPLER_2D:   return ShaderPropertyType::Sampler2D;
        case GL_SAMPLER_CUBE: return ShaderPropertyType::SamplerCube;

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

    std::string normalize_shader_element_name(std::string_view opengl_name)
    {
        std::string s{opengl_name};
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
            ShaderPropertyType shader_type_,
            int32_t size_) :

            location{location_},
            shader_type{shader_type_},
            size{size_}
        {}

        int32_t location;
        ShaderPropertyType shader_type;
        int32_t size;
    };

    void print_shader_element(std::ostream& o, std::string_view name, const ShaderElement& se)
    {
        o << "ShadeElement(name = " << name << ", location = " << se.location << ", shader_type = " << se.shader_type << ", size = " << se.size << ')';
    }

    template<typename Value>
    using FastStringHashtable = ankerl::unordered_dense::map<
        StringName,
        Value,
        TransparentStringHasher,
        std::equal_to<>
    >;
}

namespace
{
    class MaybeIndex final {
    public:
        constexpr MaybeIndex() = default;
        explicit constexpr MaybeIndex(std::optional<size_t> tagged)
        {
            if (tagged) {
                if (tagged.value() == c_senteniel_index_value) {
                    throw std::invalid_argument{"the provided index value is out of range"};
                }
                value_ = *tagged;
            }
        }

        friend bool operator==(MaybeIndex, MaybeIndex) = default;

        explicit operator bool () const { return value_ != c_senteniel_index_value; }
        size_t operator*() const { return value_; }
    private:
        static inline constexpr size_t c_senteniel_index_value = std::numeric_limits<size_t>::max();
        size_t value_ = c_senteniel_index_value;
    };

    // this is what is stored in the renderer's render queue
    struct RenderObject final {

        RenderObject(
            Mesh mesh_,
            const Transform& transform_,
            Material material_,
            std::optional<MaterialPropertyBlock> maybe_prop_block_,
            std::optional<size_t> maybe_submesh_index_) :

            material{std::move(material_)},
            property_block{maybe_prop_block_ ? std::move(maybe_prop_block_).value() : MaterialPropertyBlock{}},
            mesh{std::move(mesh_)},
            maybe_submesh_index{maybe_submesh_index_},
            transform{mat4_cast(transform_)},
            world_centroid{transform_point(transform, centroid_of(mesh.bounds()))}
        {}

        RenderObject(
            Mesh mesh_,
            const Mat4& transform_,
            Material material_,
            std::optional<MaterialPropertyBlock> maybe_prop_block_,
            std::optional<size_t> maybe_submesh_index_) :

            material{std::move(material_)},
            property_block{maybe_prop_block_ ? std::move(maybe_prop_block_).value() : MaterialPropertyBlock{}},
            mesh{std::move(mesh_)},
            maybe_submesh_index{maybe_submesh_index_},
            transform{transform_},
            world_centroid{transform_point(transform_, centroid_of(mesh.bounds()))}
        {}

        friend void swap(RenderObject& a, RenderObject& b) noexcept
        {
            using std::swap;

            swap(a.material, b.material);
            swap(a.property_block, b.property_block);
            swap(a.mesh, b.mesh);
            swap(a.transform, b.transform);
            swap(a.maybe_submesh_index, b.maybe_submesh_index);
            swap(a.world_centroid, b.world_centroid);
        }

        friend bool operator==(const RenderObject&, const RenderObject&) = default;

        Material material;
        MaterialPropertyBlock property_block;
        Mesh mesh;
        MaybeIndex maybe_submesh_index;
        Mat4 transform;
        Vec3 world_centroid;
    };

    static_assert(std::is_nothrow_destructible_v<RenderObject>);

    bool is_opaque(const RenderObject& ro)
    {
        return not ro.material.is_transparent();
    }

    bool is_depth_tested(const RenderObject& ro)
    {
        return ro.material.is_depth_tested();
    }

    const Mat4& model_mat4(const RenderObject& ro)
    {
        return ro.transform;
    }

    Mat3 normal_matrix(const RenderObject& ro)
    {
        return normal_matrix(ro.transform);
    }

    Mat4 normal_matrix4(const RenderObject& ro)
    {
        return normal_matrix4(ro.transform);
    }

    const Vec3& worldspace_centroid(const RenderObject& ro)
    {
        return ro.world_centroid;
    }

    // function object that returns true if the first argument is farther from the second
    //
    // (handy for depth sorting)
    class RenderObjectIsFartherFrom final {
    public:
        explicit RenderObjectIsFartherFrom(const Vec3& pos) : pos_{pos} {}

        bool operator()(const RenderObject& a, const RenderObject& b) const
        {
            const Vec3 centroid_a = worldspace_centroid(a);
            const Vec3 centroid_b = worldspace_centroid(b);
            const Vec3 pos_to_a = centroid_a - pos_;
            const Vec3 pos_to_b = centroid_b - pos_;
            return length2(pos_to_a) > length2(pos_to_b);
        }
    private:
        Vec3 pos_;
    };

    class RenderObjectHasMaterial final {
    public:
        explicit RenderObjectHasMaterial(const Material* material) :
            material_{material}
        {
            OSC_ASSERT(material_ != nullptr);
        }

        bool operator()(const RenderObject& ro) const
        {
            return ro.material == *material_;
        }
    private:
        const Material* material_;
    };

    class RenderObjectHasMaterialPropertyBlock final {
    public:
        explicit RenderObjectHasMaterialPropertyBlock(const MaterialPropertyBlock& mpb) :
            mpb_{&mpb}
        {}

        bool operator()(const RenderObject& ro) const
        {
            return ro.property_block == *mpb_;
        }

    private:
        const MaterialPropertyBlock* mpb_;
    };

    class RenderObjectHasMesh final {
    public:
        explicit RenderObjectHasMesh(const Mesh* mesh) :
            mesh_{mesh}
        {
            OSC_ASSERT(mesh_ != nullptr);
        }

        bool operator()(const RenderObject& ro) const
        {
            return ro.mesh == *mesh_;
        }
    private:
        const Mesh* mesh_;
    };

    class RenderObjectHasSubMeshIndex final {
    public:
        explicit RenderObjectHasSubMeshIndex(MaybeIndex maybe_submesh_index) :
            maybe_submesh_index_{maybe_submesh_index}
        {}

        bool operator()(const RenderObject& ro) const
        {
            return ro.maybe_submesh_index == maybe_submesh_index_;
        }
    private:
        MaybeIndex maybe_submesh_index_;
    };

    // sort a sequence of `RenderObject`s for optimal drawing
    std::vector<RenderObject>::iterator sort_render_queue(
        std::vector<RenderObject>::iterator queue_begin,
        std::vector<RenderObject>::iterator queue_end,
        Vec3 camera_pos)
    {
        // partition the render queue into `[opaque_objs | transparent_objs]`
        const auto opaque_objs_end = std::partition(queue_begin, queue_end, is_opaque);

        // optimize the `opaque_objs` partition (it can be reordered safely)
        //
        // first, batch `opaque_objs` into `RenderObject`s that have the same `Material`
        auto material_batch_start = queue_begin;
        while (material_batch_start != opaque_objs_end) {

            const auto material_batch_end = std::partition(
                material_batch_start,
                opaque_objs_end,
                RenderObjectHasMaterial{&material_batch_start->material}
            );

            // second, batch `RenderObject`s with the same `Material` into sub-batches
            // with the same `MaterialPropertyBlock`
            auto props_batch_start = material_batch_start;
            while (props_batch_start != material_batch_end) {

                const auto props_batch_end = std::partition(
                    props_batch_start,
                    material_batch_end,
                    RenderObjectHasMaterialPropertyBlock{props_batch_start->property_block}
                );

                // third, batch `RenderObject`s with the same `Material` and `MaterialPropertyBlock`s
                // into sub-batches with the same `Mesh`
                auto mesh_batch_start = props_batch_start;
                while (mesh_batch_start != props_batch_end) {

                    const auto mesh_batch_end = std::partition(
                        mesh_batch_start,
                        props_batch_end,
                        RenderObjectHasMesh{&mesh_batch_start->mesh}
                    );

                    // fourth, batch `RenderObject`s with the same `Material`, `MaterialPropertyBlock`,
                    // and `Mesh` into sub-batches with the same sub-mesh index
                    auto submesh_batch_start = mesh_batch_start;
                    while (submesh_batch_start != mesh_batch_end) {

                        const auto submesh_batch_end = std::partition(
                            submesh_batch_start,
                            mesh_batch_end,
                            RenderObjectHasSubMeshIndex{submesh_batch_start->maybe_submesh_index}
                        );

                        submesh_batch_start = submesh_batch_end;
                    }
                    mesh_batch_start = mesh_batch_end;
                }
                props_batch_start = props_batch_end;
            }
            material_batch_start = material_batch_end;
        }

        // sort the transparent partition by distance from camera (back-to-front)
        std::sort(opaque_objs_end, queue_end, RenderObjectIsFartherFrom{camera_pos});

        return opaque_objs_end;
    }

    // top-level state for a single call to `render`
    struct RenderPassState final {

        RenderPassState(
            const Vec3& camera_pos_,
            const Mat4& view_matrix_,
            const Mat4& projection_matrix_) :

            camera_pos{camera_pos_},
            view_matrix{view_matrix_},
            projection_matrix{projection_matrix_}
        {}

        Vec3 camera_pos;
        Mat4 view_matrix;
        Mat4 projection_matrix;
        Mat4 view_projection_matrix = projection_matrix * view_matrix;
    };

    // the OpenGL data associated with a `Texture2D`
    struct Texture2DOpenGLData final {
        gl::Texture2D texture;
        UID texture_params_version;
    };


    // the OpenGL data associated with a `RenderBuffer`
    struct SingleSampledTexture final {
        gl::Texture2D texture2D;
    };
    struct MultisampledRBOAndResolvedTexture final {
        gl::RenderBuffer multisampled_rbo;
        gl::Texture2D single_sampled_texture2D;
    };
    struct SingleSampledCubemap final {
        gl::TextureCubemap cubemap;
    };
    using RenderBufferOpenGLData = std::variant<
        SingleSampledTexture,
        MultisampledRBOAndResolvedTexture,
        SingleSampledCubemap
    >;

    // the OpenGL data associated with a `Mesh`
    struct MeshOpenGLData final {
        UID data_version;
        gl::TypedBufferHandle<GL_ARRAY_BUFFER> array_buffer;
        gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER> indices_buffer;
        gl::VertexArray vao;
    };

    struct InstancingState final {
        InstancingState(
            gl::ArrayBuffer<float, GL_STREAM_DRAW>& buf_,
            size_t stride_) :

            buffer{buf_},
            stride{stride_}
        {}

        gl::ArrayBuffer<float, GL_STREAM_DRAW>& buffer;
        size_t stride = 0;
        size_t base_offset = 0;
    };
}

namespace osc
{
    class GraphicsBackend final {
    public:
        // internal methods

        static void bind_to_instanced_attributes(
            const Shader::Impl&,
            InstancingState&
        );

        static void unbind_from_instanced_attributes(
            const Shader::Impl&,
            InstancingState&
        );

        static std::optional<InstancingState> upload_instance_data(
            std::span<const RenderObject>,
            const Shader::Impl&
        );

        static void try_bind_material_value_to_shader_element(
            const ShaderElement&,
            const MaterialValue&,
            int32_t& texture_slot
        );

        static void handle_batch_with_same_submesh(
            std::span<const RenderObject>,
            std::optional<InstancingState>& instancing_state
        );

        static void handle_batch_with_same_mesh(
            std::span<const RenderObject>,
            std::optional<InstancingState>& instancing_state
        );

        static void handle_batch_with_same_material_property_block(
            std::span<const RenderObject>,
            int32_t& texture_slot,
            std::optional<InstancingState>& instancing_state
        );

        static void handle_batch_with_same_material(
            const RenderPassState&,
            std::span<const RenderObject>
        );

        static void draw_render_objects(
            const RenderPassState&,
            std::span<const RenderObject>
        );

        static void draw_batched_by_opaqueness(
            const RenderPassState&,
            std::span<const RenderObject>
        );

        struct ViewportGeometry final {
            Vec2 bottom_left;
            Vec2 dimensions;
        };
        static ViewportGeometry calc_viewport_geometry(
            Camera::Impl&,
            const RenderTarget* maybe_custom_render_target
        );
        static float setup_top_level_pipeline_state(
            Camera::Impl&,
            const RenderTarget* maybe_custom_render_target
        );
        static std::optional<gl::FrameBuffer> bind_and_clear_render_buffers(
            Camera::Impl&,
            const RenderTarget* maybe_custom_render_target
        );
        static void resolve_render_buffers(
            const RenderTarget& maybe_custom_render_target
        );
        static void flush_render_queue(
            Camera::Impl& camera,
            float aspect_ratio
        );
        static void teardown_top_level_pipeline_state(
            Camera::Impl&,
            const RenderTarget* maybe_custom_render_target
        );
        static void render_camera_queue(
            Camera::Impl& camera,
            const RenderTarget* maybe_custom_render_target = nullptr
        );


        // public (forwarded) API

        static void draw(
            const Mesh&,
            const Transform&,
            const Material&,
            Camera&,
            const std::optional<MaterialPropertyBlock>&,
            std::optional<size_t>
        );

        static void draw(
            const Mesh&,
            const Mat4&,
            const Material&,
            Camera&,
            const std::optional<MaterialPropertyBlock>&,
            std::optional<size_t>
        );

        static void blit(
            const Texture2D&,
            RenderTexture&
        );

        static void blit_to_screen(
            const RenderTexture&,
            const Rect&,
            BlitFlags
        );

        static void blit_to_screen(
            const RenderTexture&,
            const Rect&,
            const Material&,
            BlitFlags
        );

        static void blit_to_screen(
            const Texture2D&,
            const Rect&
        );

        static void copy_texture(
            const RenderTexture&,
            Texture2D&
        );

        static void copy_texture(
            const RenderTexture&,
            Texture2D&,
            CubemapFace
        );

        static void copy_texture(
            const RenderTexture&,
            Cubemap&,
            size_t
        );
    };
}

namespace
{
    // returns the memory alignment of data that is to be copied from the
    // CPU (packed) to the GPU (unpacked)
    constexpr GLint opengl_unpack_alignment_of(TextureFormat texture_format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::unpack_alignment... });
        }(TextureFormatList{});

        return lut.at(to_index(texture_format));
    }

    // returns the format OpenGL will use internally (i.e. on the GPU) to
    // represent the given format+colorspace combo
    constexpr GLenum opengl_internal_format_of(TextureFormat texture_format, ColorSpace color_space)
    {
        constexpr auto srgb_lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::internal_format_srgb... });
        }(TextureFormatList{});

        constexpr auto linear_lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::internal_format_linear... });
        }(TextureFormatList{});

        static_assert(num_options<ColorSpace>() == 2);
        if (color_space == ColorSpace::sRGB) {
            return srgb_lut.at(to_index(texture_format));
        }
        else {
            return linear_lut.at(to_index(texture_format));
        }
    }

    constexpr GLenum opengl_data_type_of(CPUDataType cpu_datatype)
    {
        constexpr auto lut = []<CPUDataType... DataTypes>(OptionList<CPUDataType, DataTypes...>)
        {
            return std::to_array({ CPUDataTypeOpenGLTraits<DataTypes>::opengl_data_type... });
        }(CPUDataTypeList{});

        return lut.at(to_index(cpu_datatype));
    }

    constexpr CPUDataType equivalent_cpu_datatype_of(TextureFormat texture_format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatTraits<Formats>::equivalent_cpu_datatype... });
        }(TextureFormatList{});

        return lut.at(to_index(texture_format));
    }

    constexpr CPUImageFormat equivalent_cpu_image_format_of(TextureFormat texture_format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatTraits<Formats>::equivalent_cpu_image_format... });
        }(TextureFormatList{});

        return lut.at(to_index(texture_format));
    }

    constexpr GLenum opengl_format_of(CPUImageFormat cpu_format)
    {
        constexpr auto lut = []<CPUImageFormat... Formats>(OptionList<CPUImageFormat, Formats...>)
        {
            return std::to_array({ CPUImageFormatOpenGLTraits<Formats>::opengl_format... });
        }(CPUImageFormatList{});

        return lut.at(to_index(cpu_format));
    }

    constexpr GLenum to_opengl_texture_cubemap_enum(CubemapFace cubemap_face)
    {
        static_assert(num_options<CubemapFace>() == 6);
        static_assert(static_cast<GLenum>(CubemapFace::PositiveX) == 0);
        static_assert(static_cast<GLenum>(CubemapFace::NegativeZ) == 5);
        static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

        return GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(cubemap_face);
    }

    GLint to_opengl_texturewrap_enum(TextureWrapMode texture_wrap_mode)
    {
        static_assert(num_options<TextureWrapMode>() == 3);

        switch (texture_wrap_mode) {
        case TextureWrapMode::Repeat: return GL_REPEAT;
        case TextureWrapMode::Clamp:  return GL_CLAMP_TO_EDGE;
        case TextureWrapMode::Mirror: return GL_MIRRORED_REPEAT;
        default:                      return GL_REPEAT;
        }
    }

    constexpr auto c_texture_wrap_mode_strings = std::to_array<CStringView>(
    {
        "Repeat",
        "Clamp",
        "Mirror",
    });
    static_assert(c_texture_wrap_mode_strings.size() == num_options<TextureWrapMode>());

    constexpr auto c_texture_filter_mode_strings = std::to_array<CStringView>(
    {
        "Nearest",
        "Linear",
        "Mipmap",
    });
    static_assert(c_texture_filter_mode_strings.size() == num_options<TextureFilterMode>());

    GLint to_opengl_texture_min_filter_param(TextureFilterMode texture_filter_mode)
    {
        static_assert(num_options<TextureFilterMode>() == 3);

        switch (texture_filter_mode) {
        case TextureFilterMode::Nearest: return GL_NEAREST;
        case TextureFilterMode::Linear:  return GL_LINEAR;
        case TextureFilterMode::Mipmap:  return GL_LINEAR_MIPMAP_LINEAR;
        default:                         return GL_LINEAR;
        }
    }

    GLint to_opengl_texture_mag_filter_param(TextureFilterMode texture_filter_mode)
    {
        static_assert(num_options<TextureFilterMode>() == 3);

        switch (texture_filter_mode) {
        case TextureFilterMode::Nearest: return GL_NEAREST;
        case TextureFilterMode::Linear:  return GL_LINEAR;
        case TextureFilterMode::Mipmap:  return GL_LINEAR;
        default:                         return GL_LINEAR;
        }
    }
}

namespace
{
    // the OpenGL data associated with a `Texture2D`
    struct CubemapOpenGLData final {
        gl::TextureCubemap texture;
        UID source_data_version;
        UID source_params_version;
    };
}

class osc::Cubemap::Impl final {
public:
    Impl(int32_t width, TextureFormat format) :
        width_{width},
        format_{format}
    {
        OSC_ASSERT(width_ > 0 && "the width of a cubemap must be a positive number");

        const auto num_bytes_per_pixel = num_bytes_per_pixel_in(format_);
        const auto num_pixels_per_face = static_cast<size_t>(width_) * static_cast<size_t>(width_);
        const auto num_bytes_per_face = num_bytes_per_pixel * num_pixels_per_face;
        data_.resize(num_options<CubemapFace>() * num_bytes_per_face);
    }

    int32_t width() const
    {
        return width_;
    }

    TextureFormat texture_format() const
    {
        return format_;
    }

    TextureWrapMode wrap_mode() const
    {
        return wrap_mode_u_;
    }

    void set_wrap_mode(TextureWrapMode wrap_mode)
    {
        wrap_mode_u_ = wrap_mode;
        wrap_mode_v_ = wrap_mode;
        wrap_mode_w_ = wrap_mode;
        texture_params_version_.reset();
    }

    TextureWrapMode get_wrap_mode_u() const
    {
        return wrap_mode_u_;
    }

    void set_wrap_mode_u(TextureWrapMode wrap_mode_u)
    {
        wrap_mode_u_ = wrap_mode_u;
        texture_params_version_.reset();
    }

    TextureWrapMode get_wrap_mode_v() const
    {
        return wrap_mode_v_;
    }

    void set_wrap_mode_v(TextureWrapMode wrap_mode_v)
    {
        wrap_mode_v_ = wrap_mode_v;
        texture_params_version_.reset();
    }

    TextureWrapMode wrap_mode_w() const
    {
        return wrap_mode_w_;
    }

    void set_wrap_mode_w(TextureWrapMode wrap_mode_w)
    {
        wrap_mode_w_ = wrap_mode_w;
        texture_params_version_.reset();
    }

    TextureFilterMode filter_mode() const
    {
        return filter_mode_;
    }

    void set_filter_mode(TextureFilterMode filter_mode)
    {
        filter_mode_ = filter_mode;
        texture_params_version_.reset();
    }

    void set_pixel_data(CubemapFace face, std::span<const uint8_t> pixel_components_row_by_row)
    {
        const size_t face_index = to_index(face);
        const auto num_pixels_per_face = static_cast<size_t>(width_) * static_cast<size_t>(width_);
        const size_t num_bytes_per_face = num_pixels_per_face * num_bytes_per_pixel_in(format_);
        const size_t destination_data_begin = face_index * num_bytes_per_face;
        const size_t destination_data_end = destination_data_begin + num_bytes_per_face;

        OSC_ASSERT(face_index < num_options<CubemapFace>() && "invalid cubemap face passed to Cubemap::set_pixel_data");
        OSC_ASSERT(pixel_components_row_by_row.size() == num_bytes_per_face && "incorrect amount of data passed to Cubemap::set_pixel_data: the data must match the dimensions and texture format of the cubemap");
        OSC_ASSERT(destination_data_end <= data_.size() && "out of range assignment detected: this should be handled in the constructor");

        rgs::copy(pixel_components_row_by_row, data_.begin() + destination_data_begin);
        data_version_.reset();
    }

    gl::TextureCubemap& upd_cubemap()
    {
        if (not *maybe_gpu_texture_) {
            *maybe_gpu_texture_ = CubemapOpenGLData{};
        }

        CubemapOpenGLData& opengl_data = **maybe_gpu_texture_;

        if (opengl_data.source_data_version != data_version_) {
            upload_to_gpu(opengl_data);
        }

        if (opengl_data.source_params_version != texture_params_version_) {
            update_opengl_texture_params(opengl_data);
        }

        return opengl_data.texture;
    }
private:
    void upload_to_gpu(CubemapOpenGLData& opengl_data)
    {
        // calculate CPU-to-GPU data transfer parameters
        const size_t num_bytes_per_pixel = num_bytes_per_pixel_in(format_);
        const size_t num_bytes_per_row = width_ * num_bytes_per_pixel;
        const size_t num_bytes_per_face = width_ * num_bytes_per_row;
        const size_t num_bytes_in_cubemap = num_options<CubemapFace>() * num_bytes_per_face;
        const CPUDataType cpu_data_type = equivalent_cpu_datatype_of(format_);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        const CPUImageFormat cpu_component_format = equivalent_cpu_image_format_of(format_);  // TextureFormat's layout == CPU formats's layout for cubemaps
        const GLint opengl_unpack_alignment = opengl_unpack_alignment_of(format_);

        // sanity-check before doing anything with OpenGL
        OSC_ASSERT(num_bytes_per_row % opengl_unpack_alignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(is_aligned_at_least(data_.data(), opengl_unpack_alignment) && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(num_bytes_in_cubemap <= data_.size() && "the number of bytes in the cubemap (CPU-side) is less than expected: this is an implementation bug that should be reported");
        static_assert(num_options<TextureFormat>() == 7, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");

        // upload cubemap to GPU
        static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);
        gl::bind_texture(opengl_data.texture);
        gl::pixel_store_i(GL_UNPACK_ALIGNMENT, opengl_unpack_alignment);
        for (GLint face_index = 0; face_index < static_cast<GLint>(num_options<CubemapFace>()); ++face_index) {

            const size_t face_bytes_begin = face_index * num_bytes_per_face;

            gl::tex_image2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index,
                0,
                opengl_internal_format_of(format_, ColorSpace::sRGB),  // cubemaps are always sRGB
                width_,
                width_,
                0,
                opengl_format_of(cpu_component_format),
                opengl_data_type_of(cpu_data_type),
                data_.data() + face_bytes_begin
            );
        }

        // generate mips (care: they can be uploaded to with `graphics::copy_texture`)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        gl::bind_texture();

        opengl_data.source_data_version = data_version_;
    }

    void update_opengl_texture_params(CubemapOpenGLData& opengl_data)
    {
        gl::bind_texture(opengl_data.texture);

        // set texture parameters
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, to_opengl_texture_mag_filter_param(filter_mode_));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, to_opengl_texture_min_filter_param(filter_mode_));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, to_opengl_texturewrap_enum(wrap_mode_u_));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, to_opengl_texturewrap_enum(wrap_mode_v_));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, to_opengl_texturewrap_enum(wrap_mode_w_));

        // cleanup OpenGL binding state
        gl::bind_texture();

        opengl_data.source_params_version = texture_params_version_;
    }

    int32_t width_;
    TextureFormat format_;
    std::vector<uint8_t> data_;
    UID data_version_;

    TextureWrapMode wrap_mode_u_ = TextureWrapMode::Repeat;
    TextureWrapMode wrap_mode_v_ = TextureWrapMode::Repeat;
    TextureWrapMode wrap_mode_w_ = TextureWrapMode::Repeat;
    TextureFilterMode filter_mode_ = TextureFilterMode::Mipmap;
    UID texture_params_version_;

    DefaultConstructOnCopy<std::optional<CubemapOpenGLData>> maybe_gpu_texture_;
};

osc::Cubemap::Cubemap(int32_t width, TextureFormat format) :
    impl_{make_cow<Impl>(width, format)}
{}

int32_t osc::Cubemap::width() const
{
    return impl_->width();
}

TextureWrapMode osc::Cubemap::wrap_mode() const
{
    return impl_->wrap_mode();
}

void osc::Cubemap::set_wrap_mode(TextureWrapMode wrap_mode)
{
    impl_.upd()->set_wrap_mode(wrap_mode);
}

TextureWrapMode osc::Cubemap::wrap_mode_u() const
{
    return impl_->get_wrap_mode_u();
}

void osc::Cubemap::set_wrap_mode_u(TextureWrapMode wrap_mode_u)
{
    impl_.upd()->set_wrap_mode_u(wrap_mode_u);
}

TextureWrapMode osc::Cubemap::wrap_mode_v() const
{
    return impl_->get_wrap_mode_v();
}

void osc::Cubemap::set_wrap_mode_v(TextureWrapMode wrap_mode_v)
{
    impl_.upd()->set_wrap_mode_v(wrap_mode_v);
}

TextureWrapMode osc::Cubemap::wrap_mode_w() const
{
    return impl_->wrap_mode_w();
}

void osc::Cubemap::set_wrap_mode_w(TextureWrapMode wrap_mode_w)
{
    impl_.upd()->set_wrap_mode_w(wrap_mode_w);
}

TextureFilterMode osc::Cubemap::filter_mode() const
{
    return impl_->filter_mode();
}

void osc::Cubemap::set_filter_mode(TextureFilterMode filter_mode)
{
    impl_.upd()->set_filter_mode(filter_mode);
}

TextureFormat osc::Cubemap::texture_format() const
{
    return impl_->texture_format();
}

void osc::Cubemap::set_pixel_data(CubemapFace face, std::span<const uint8_t> pixel_components_row_by_row)
{
    impl_.upd()->set_pixel_data(face, pixel_components_row_by_row);
}

namespace
{
    std::vector<Color> convert_pixel_bytes_to_color(
        std::span<const uint8_t> pixel_bytes,
        TextureFormat pixel_format)
    {
        const TextureComponentFormat component_format = component_format_of(pixel_format);

        const size_t num_components = num_components_in(pixel_format);
        const size_t num_bytes_per_component = num_bytes_per_component_in(component_format);
        const size_t num_bytes_per_pixel = num_bytes_per_component * num_components;
        const size_t num_pixels = pixel_bytes.size() / num_bytes_per_pixel;

        OSC_ASSERT(pixel_bytes.size() % num_bytes_per_pixel == 0);

        std::vector<Color> rv;
        rv.reserve(num_pixels);

        static_assert(num_options<TextureComponentFormat>() == 2);
        if (component_format == TextureComponentFormat::Uint8) {

            // unpack 8-bit component bytes into floating-point Color components
            for (size_t pixel = 0; pixel < num_pixels; ++pixel) {
                const size_t pixel_begin = num_bytes_per_pixel * pixel;

                Color color = Color::black();
                for (size_t component = 0; component < num_components; ++component) {
                    const size_t component_begin = pixel_begin + component;
                    color[component] = Unorm8{pixel_bytes[component_begin]}.normalized_value();
                }
                rv.push_back(color);
            }
        }
        else if (component_format == TextureComponentFormat::Float32 and num_bytes_per_component == sizeof(float)) {

            // read 32-bit component floats into Color components
            for (size_t pixel = 0; pixel < num_pixels; ++pixel) {
                const size_t pixel_begin = num_bytes_per_pixel * pixel;

                Color color = Color::black();
                for (size_t component = 0; component < num_components; ++component) {
                    const size_t component_begin = pixel_begin + component*num_bytes_per_component;

                    std::span<const uint8_t> component_span{pixel_bytes.data() + component_begin, sizeof(float)};
                    std::array<uint8_t, sizeof(float)> tmp_array{};
                    rgs::copy(component_span, tmp_array.begin());

                    color[component] = cpp20::bit_cast<float>(tmp_array);
                }
                rv.push_back(color);
            }
        }
        else {
            OSC_ASSERT(false && "unsupported texture component format or bytes per component detected");
        }

        return rv;
    }

    std::vector<Color32> convert_pixel_bytes_to_color32(
        std::span<const uint8_t> pixel_bytes,
        TextureFormat pixel_format)
    {
        const TextureComponentFormat component_format = component_format_of(pixel_format);

        const size_t num_components = num_components_in(pixel_format);
        const size_t num_bytes_per_component = num_bytes_per_component_in(component_format);
        const size_t num_bytes_per_pixel = num_bytes_per_component * num_components;
        const size_t num_pixels = pixel_bytes.size() / num_bytes_per_pixel;

        std::vector<Color32> rv;
        rv.reserve(num_pixels);

        static_assert(num_options<TextureComponentFormat>() == 2);
        if (component_format == TextureComponentFormat::Uint8) {

            // read 8-bit component bytes into 8-bit Color32 color components
            for (size_t pixel = 0; pixel < num_pixels; ++pixel) {
                const size_t pixel_begin = num_bytes_per_pixel * pixel;

                Color32 color = {0x00, 0x00, 0x00, 0xff};
                for (size_t component = 0; component < num_components; ++component) {
                    const size_t component_begin = pixel_begin + component;
                    color[component] = pixel_bytes[component_begin];
                }
                rv.push_back(color);
            }
        }
        else {
            static_assert(std::is_same_v<Color::value_type, float>);
            OSC_ASSERT(num_bytes_per_component == sizeof(float));

            // pack 32-bit component floats into 8-bit Color32 color components
            for (size_t pixel = 0; pixel < num_pixels; ++pixel) {
                const size_t pixel_begin = num_bytes_per_pixel * pixel;

                Color32 color = {0x00, 0x00, 0x00, 0xff};
                for (size_t component = 0; component < num_components; ++component) {
                    const size_t component_begin = pixel_begin + component*sizeof(float);

                    std::span<const uint8_t> component_span{pixel_bytes.data() + component_begin, sizeof(float)};
                    std::array<uint8_t, sizeof(float)> tmp_array{};
                    rgs::copy(component_span, tmp_array.begin());
                    const auto component_float = cpp20::bit_cast<float>(tmp_array);

                    color[component] = Unorm8{component_float};
                }
                rv.push_back(color);
            }
        }

        return rv;
    }

    void convert_colors_to_pixel_bytes(
        std::span<const Color> colors,
        TextureFormat desired_pixel_format,
        std::vector<uint8_t>& pixel_bytes_out)
    {
        const TextureComponentFormat component_format = component_format_of(desired_pixel_format);

        const size_t num_components = num_components_in(desired_pixel_format);
        const size_t num_bytes_per_component = num_bytes_per_component_in(component_format);
        const size_t num_bytes_per_pixel = num_bytes_per_component * num_components;
        const size_t num_pixels = colors.size();
        const size_t num_output_bytes = num_bytes_per_pixel * num_pixels;

        pixel_bytes_out.clear();
        pixel_bytes_out.reserve(num_output_bytes);

        OSC_ASSERT(num_components <= std::tuple_size_v<Color>);
        static_assert(num_options<TextureComponentFormat>() == 2);
        if (component_format == TextureComponentFormat::Uint8) {

            // clamp pixels, convert them to bytes, add them to pixel data buffer
            for (const Color& color : colors) {
                for (size_t component = 0; component < num_components; ++component) {
                    pixel_bytes_out.push_back(Unorm8{color[component]}.raw_value());
                }
            }
        }
        else {
            // write pixels to pixel data buffer as-is (they're floats already)
            for (const Color& color : colors) {
                for (size_t component = 0; component < num_components; ++component) {
                    push_as_bytes(color[component], pixel_bytes_out);
                }
            }
        }
    }

    void convert_color32s_to_pixel_bytes(
        std::span<const Color32> colors,
        TextureFormat desired_pixel_format,
        std::vector<uint8_t>& pixel_data_out)
    {
        const TextureComponentFormat component_format = component_format_of(desired_pixel_format);

        const size_t num_components = num_components_in(desired_pixel_format);
        const size_t num_bytes_per_component = num_bytes_per_component_in(component_format);
        const size_t num_bytes_per_pixel = num_bytes_per_component * num_components;
        const size_t num_pixels = colors.size();
        const size_t num_output_bytes = num_bytes_per_pixel * num_pixels;

        pixel_data_out.clear();
        pixel_data_out.reserve(num_output_bytes);

        OSC_ASSERT(num_components <= std::tuple_size_v<Color32>);
        static_assert(num_options<TextureComponentFormat>() == 2);
        if (component_format == TextureComponentFormat::Uint8) {
            // write pixels to pixel data buffer as-is (they're bytes already)
            for (const Color32& color : colors) {
                for (size_t component = 0; component < num_components; ++component) {
                    pixel_data_out.push_back(color[component].raw_value());
                }
            }
        }
        else {
            // upscale pixels to float32s and write the floats to the pixel buffer
            for (const Color32& color : colors) {
                for (size_t component = 0; component < num_components; ++component) {
                    const float pixel_float_value = Unorm8{color[component]}.normalized_value();
                    push_as_bytes(pixel_float_value, pixel_data_out);
                }
            }
        }
    }
}

class osc::Texture2D::Impl final {
public:
    Impl(
        Vec2i dimensions,
        TextureFormat texture_format,
        ColorSpace color_space,
        TextureWrapMode wrap_mode,
        TextureFilterMode filter_mode) :

        dimensions_{dimensions},
        texture_format_{texture_format},
        color_space_{color_space},
        wrap_mode_u_{wrap_mode},
        wrap_mode_v_{wrap_mode},
        wrap_mode_w_{wrap_mode},
        filter_mode_{filter_mode}
    {
        OSC_ASSERT(dimensions_.x > 0 and dimensions_.y > 0);
    }

    Vec2i dimensions() const
    {
        return dimensions_;
    }

    TextureFormat texture_format() const
    {
        return texture_format_;
    }

    ColorSpace color_space() const
    {
        return color_space_;
    }

    TextureWrapMode wrap_mode() const
    {
        return get_wrap_mode_u();
    }

    void set_wrap_mode(TextureWrapMode wrap_mode)
    {
        set_wrap_mode_u(wrap_mode);
        set_wrap_mode_v(wrap_mode);
        set_wrap_mode_w(wrap_mode);
        texture_params_version_.reset();
    }

    TextureWrapMode get_wrap_mode_u() const
    {
        return wrap_mode_u_;
    }

    void set_wrap_mode_u(TextureWrapMode wrap_mode_u)
    {
        wrap_mode_u_ = wrap_mode_u;
        texture_params_version_.reset();
    }

    TextureWrapMode get_wrap_mode_v() const
    {
        return wrap_mode_v_;
    }

    void set_wrap_mode_v(TextureWrapMode wrap_mode_v)
    {
        wrap_mode_v_ = wrap_mode_v;
        texture_params_version_.reset();
    }

    TextureWrapMode wrap_mode_w() const
    {
        return wrap_mode_w_;
    }

    void set_wrap_mode_w(TextureWrapMode wrap_mode_w)
    {
        wrap_mode_w_ = wrap_mode_w;
        texture_params_version_.reset();
    }

    TextureFilterMode filter_mode() const
    {
        return filter_mode_;
    }

    void set_filter_mode(TextureFilterMode filter_mode)
    {
        filter_mode_ = filter_mode;
        texture_params_version_.reset();
    }

    std::vector<Color> pixels() const
    {
        return convert_pixel_bytes_to_color(pixel_data_, texture_format_);
    }

    void set_pixels(std::span<const Color> pixels)
    {
        OSC_ASSERT(ssize(pixels) == area_of(dimensions_));
        convert_colors_to_pixel_bytes(pixels, texture_format_, pixel_data_);
    }

    std::vector<Color32> pixels32() const
    {
        return convert_pixel_bytes_to_color32(pixel_data_, texture_format_);
    }

    void set_pixels32(std::span<const Color32> pixels)
    {
        OSC_ASSERT(ssize(pixels) == area_of(dimensions_));
        convert_color32s_to_pixel_bytes(pixels, texture_format_, pixel_data_);
    }

    std::span<const uint8_t> pixel_data() const
    {
        return pixel_data_;
    }

    void set_pixel_data(std::span<const uint8_t> pixel_components_row_by_row)
    {
        OSC_ASSERT(pixel_components_row_by_row.size() == num_bytes_per_pixel_in(texture_format_)*dimensions_.x*dimensions_.y && "incorrect number of bytes passed to Texture2D::set_pixel_data");
        OSC_ASSERT(pixel_components_row_by_row.size() == pixel_data_.size());

        rgs::copy(pixel_components_row_by_row, pixel_data_.begin());
    }

    // non PIMPL method

    gl::Texture2D& updTexture()
    {
        if (not *maybe_opengl_data_) {
            upload_to_gpu();
        }
        OSC_ASSERT(*maybe_opengl_data_);

        Texture2DOpenGLData& buffers = **maybe_opengl_data_;

        if (buffers.texture_params_version != texture_params_version_) {
            update_opengl_texture_params(buffers);
        }

        return buffers.texture;
    }

private:
    void upload_to_gpu()
    {
        *maybe_opengl_data_ = Texture2DOpenGLData{};

        const size_t num_bytes_per_pixel = num_bytes_per_pixel_in(texture_format_);
        const size_t num_bytes_per_row = dimensions_.x * num_bytes_per_pixel;
        const GLint unpack_alignment = opengl_unpack_alignment_of(texture_format_);
        const CPUDataType cpu_data_type = equivalent_cpu_datatype_of(texture_format_);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        const CPUImageFormat cpu_component_layout = equivalent_cpu_image_format_of(texture_format_);  // TextureFormat's layout == CPU formats's layout for cubemaps

        static_assert(num_options<TextureFormat>() == 7, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");
        OSC_ASSERT(num_bytes_per_row % unpack_alignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(is_aligned_at_least(pixel_data_.data(), unpack_alignment) && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");

        // one-time upload, because pixels cannot be altered
        gl::bind_texture((*maybe_opengl_data_)->texture);
        gl::pixel_store_i(GL_UNPACK_ALIGNMENT, unpack_alignment);
        gl::tex_image2D(
            GL_TEXTURE_2D,
            0,
            opengl_internal_format_of(texture_format_, color_space_),
            dimensions_.x,
            dimensions_.y,
            0,
            opengl_format_of(cpu_component_layout),
            opengl_data_type_of(cpu_data_type),
            pixel_data_.data()
        );
        glGenerateMipmap(GL_TEXTURE_2D);
        gl::bind_texture();
    }

    void update_opengl_texture_params(Texture2DOpenGLData& buffers)
    {
        gl::bind_texture(buffers.texture);
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_opengl_texturewrap_enum(wrap_mode_u_));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_opengl_texturewrap_enum(wrap_mode_v_));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, to_opengl_texturewrap_enum(wrap_mode_w_));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_opengl_texture_min_filter_param(filter_mode_));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_opengl_texture_mag_filter_param(filter_mode_));
        gl::bind_texture();
        buffers.texture_params_version = texture_params_version_;
    }

    friend class GraphicsBackend;

    Vec2i dimensions_;
    TextureFormat texture_format_;
    ColorSpace color_space_;
    TextureWrapMode wrap_mode_u_ = TextureWrapMode::Repeat;
    TextureWrapMode wrap_mode_v_ = TextureWrapMode::Repeat;
    TextureWrapMode wrap_mode_w_ = TextureWrapMode::Repeat;
    TextureFilterMode filter_mode_ = TextureFilterMode::Nearest;
    std::vector<uint8_t> pixel_data_ = std::vector<uint8_t>(num_bytes_per_pixel_in(texture_format_) * dimensions_.x * dimensions_.y, 0xff);
    UID texture_params_version_;
    DefaultConstructOnCopy<std::optional<Texture2DOpenGLData>> maybe_opengl_data_;
};

std::ostream& osc::operator<<(std::ostream& o, TextureWrapMode wrap_mode)
{
    return o << c_texture_wrap_mode_strings.at(to_index(wrap_mode));
}

std::ostream& osc::operator<<(std::ostream& o, TextureFilterMode filter_mode)
{
    return o << c_texture_filter_mode_strings.at(to_index(filter_mode));
}

size_t osc::num_components_in(TextureFormat format)
{
    constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
    {
        return std::to_array({ TextureFormatTraits<Formats>::num_components... });
    }(TextureFormatList{});

    return lut.at(to_index(format));
}

TextureComponentFormat osc::component_format_of(TextureFormat format)
{
    constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
    {
        return std::to_array({ TextureFormatTraits<Formats>::component_format... });
    }(TextureFormatList{});

    return lut.at(to_index(format));
}

size_t osc::num_bytes_per_pixel_in(TextureFormat format)
{
    return num_components_in(format) * num_bytes_per_component_in(component_format_of(format));
}

std::optional<TextureFormat> osc::to_texture_format(size_t num_components, TextureComponentFormat component_format)
{
    static_assert(num_options<TextureComponentFormat>() == 2);
    const bool format_is_byte_oriented = component_format == TextureComponentFormat::Uint8;

    static_assert(num_options<TextureFormat>() == 7);
    switch (num_components) {
    case 1: return format_is_byte_oriented ? TextureFormat::R8     : std::optional<TextureFormat>{};
    case 2: return format_is_byte_oriented ? TextureFormat::RG16   : TextureFormat::RGFloat;
    case 3: return format_is_byte_oriented ? TextureFormat::RGB24  : TextureFormat::RGBFloat;
    case 4: return format_is_byte_oriented ? TextureFormat::RGBA32 : TextureFormat::RGBAFloat;
    default: return std::nullopt;
    }
}

size_t osc::num_bytes_per_component_in(TextureComponentFormat component_format)
{
    static_assert(num_options<TextureComponentFormat>() == 2);
    switch (component_format) {
    case TextureComponentFormat::Uint8:   return 1;
    case TextureComponentFormat::Float32: return 4;
    default:                            return 1;
    }
}

osc::Texture2D::Texture2D(
    Vec2i dimensions,
    TextureFormat texture_format,
    ColorSpace color_space,
    TextureWrapMode wrap_mode,
    TextureFilterMode filter_mode) :

    impl_{make_cow<Impl>(dimensions, texture_format, color_space, wrap_mode, filter_mode)}
{}

Vec2i osc::Texture2D::dimensions() const
{
    return impl_->dimensions();
}

TextureFormat osc::Texture2D::texture_format() const
{
    return impl_->texture_format();
}

ColorSpace osc::Texture2D::color_space() const
{
    return impl_->color_space();
}

TextureWrapMode osc::Texture2D::wrap_mode() const
{
    return impl_->wrap_mode();
}

void osc::Texture2D::set_wrap_mode(TextureWrapMode wrap_mode)
{
    impl_.upd()->set_wrap_mode(wrap_mode);
}

TextureWrapMode osc::Texture2D::wrap_mode_u() const
{
    return impl_->get_wrap_mode_u();
}

void osc::Texture2D::set_wrap_mode_u(TextureWrapMode wrap_mode_u)
{
    impl_.upd()->set_wrap_mode_u(wrap_mode_u);
}

TextureWrapMode osc::Texture2D::wrap_mode_v() const
{
    return impl_->get_wrap_mode_v();
}

void osc::Texture2D::set_wrap_mode_v(TextureWrapMode wrap_mode_v)
{
    impl_.upd()->set_wrap_mode_v(wrap_mode_v);
}

TextureWrapMode osc::Texture2D::wrap_mode_w() const
{
    return impl_->wrap_mode_w();
}

void osc::Texture2D::set_wrap_mode_w(TextureWrapMode wrap_mode_w)
{
    impl_.upd()->set_wrap_mode_w(wrap_mode_w);
}

TextureFilterMode osc::Texture2D::filter_mode() const
{
    return impl_->filter_mode();
}

void osc::Texture2D::set_filter_mode(TextureFilterMode filter_mode)
{
    impl_.upd()->set_filter_mode(filter_mode);
}

std::vector<Color> osc::Texture2D::pixels() const
{
    return impl_->pixels();
}

void osc::Texture2D::set_pixels(std::span<const Color> pixels)
{
    impl_.upd()->set_pixels(pixels);
}

std::vector<Color32> osc::Texture2D::pixels32() const
{
    return impl_->pixels32();
}

void osc::Texture2D::set_pixels32(std::span<Color32 const> pixels)
{
    impl_.upd()->set_pixels32(pixels);
}

std::span<const uint8_t> osc::Texture2D::pixel_data() const
{
    return impl_->pixel_data();
}

void osc::Texture2D::set_pixel_data(std::span<const uint8_t> pixel_components_row_by_row)
{
    impl_.upd()->set_pixel_data(pixel_components_row_by_row);
}

std::ostream& osc::operator<<(std::ostream& o, const Texture2D&)
{
    return o << "Texture2D()";
}

namespace
{
    constexpr auto c_render_texture_format_strings = std::to_array<CStringView>({
        "R8_UNORM",
        "R8G8B8A8_UNORM",
        "R8G8B8A8_SRGB",
        "R16G16_SFLOAT",
        "R16G16B16_SFLOAT",
        "R16G16B16A16_SFLOAT",

        "R32_SFLOAT",
    });
    static_assert(c_render_texture_format_strings.size() == num_options<ColorRenderBufferFormat>());

    constexpr GLenum to_opengl_internal_color_format_enum(const ColorRenderBufferParams& params)
    {
        static_assert(num_options<ColorRenderBufferFormat>() == 7);

        switch (params.format) {
        case ColorRenderBufferFormat::R8_UNORM:            return GL_RED;
        case ColorRenderBufferFormat::R8G8B8A8_UNORM:      return GL_RGBA8;
        case ColorRenderBufferFormat::R8G8B8A8_SRGB:       return GL_SRGB8_ALPHA8;
        case ColorRenderBufferFormat::R16G16_SFLOAT:       return GL_RG16F;
        case ColorRenderBufferFormat::R16G16B16_SFLOAT:    return GL_RGB16F;
        case ColorRenderBufferFormat::R16G16B16A16_SFLOAT: return GL_RGBA16F;
        case ColorRenderBufferFormat::R32_SFLOAT:          return GL_R32F;
        default:                                           return GL_SRGB8_ALPHA8;
        }
    }

    constexpr GLenum to_opengl_internal_color_format_enum(const DepthStencilRenderBufferParams& params)
    {
        return detail::to_opengl_internal_color_format_enum(params.format);
    }

    constexpr CPUImageFormat equivalent_cpu_image_format_of(const ColorRenderBufferFormat& format)
    {
        static_assert(num_options<ColorRenderBufferFormat>() == 7);
        static_assert(num_options<CPUImageFormat>() == 6);

        switch (format) {
        case ColorRenderBufferFormat::R8_UNORM:            return CPUImageFormat::R8;
        case ColorRenderBufferFormat::R8G8B8A8_UNORM:      return CPUImageFormat::RGBA;
        case ColorRenderBufferFormat::R8G8B8A8_SRGB:       return CPUImageFormat::RGBA;
        case ColorRenderBufferFormat::R16G16_SFLOAT:       return CPUImageFormat::RG;
        case ColorRenderBufferFormat::R16G16B16_SFLOAT:    return CPUImageFormat::RGB;
        case ColorRenderBufferFormat::R16G16B16A16_SFLOAT: return CPUImageFormat::RGBA;
        case ColorRenderBufferFormat::R32_SFLOAT:          return CPUImageFormat::R8;
        default:                                           return CPUImageFormat::RGBA;
        }
    }

    constexpr CPUDataType equivalent_cpu_datatype_of(const ColorRenderBufferFormat& format)
    {
        static_assert(num_options<ColorRenderBufferFormat>() == 7);
        static_assert(num_options<CPUDataType>() == 4);

        switch (format) {
        case ColorRenderBufferFormat::R8_UNORM:            return CPUDataType::UnsignedByte;
        case ColorRenderBufferFormat::R8G8B8A8_UNORM:      return CPUDataType::UnsignedByte;
        case ColorRenderBufferFormat::R8G8B8A8_SRGB:       return CPUDataType::UnsignedByte;
        case ColorRenderBufferFormat::R16G16_SFLOAT:       return CPUDataType::HalfFloat;
        case ColorRenderBufferFormat::R16G16B16_SFLOAT:    return CPUDataType::HalfFloat;
        case ColorRenderBufferFormat::R16G16B16A16_SFLOAT: return CPUDataType::HalfFloat;
        case ColorRenderBufferFormat::R32_SFLOAT:          return CPUDataType::Float;
        default:                                           return CPUDataType::UnsignedByte;
        }
    }

    constexpr GLenum to_opengl_image_color_format_enum(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::image_color_format... });
        }(TextureFormatList{});

        return lut.at(to_index(format));
    }

    constexpr GLint to_opengl_image_pixel_pack_alignment(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::pixel_pack_alignment... });
        }(TextureFormatList{});

        return lut.at(to_index(format));
    }

    constexpr GLenum to_opengl_image_data_type_enum(TextureFormat)
    {
        static_assert(num_options<TextureFormat>() == 7);
        return GL_UNSIGNED_BYTE;
    }
}

std::ostream& osc::operator<<(std::ostream& o, ColorRenderBufferFormat render_texture_format)
{
    return o << c_render_texture_format_strings.at(to_index(render_texture_format));
}

std::ostream& osc::operator<<(std::ostream& o, DepthStencilRenderBufferFormat depth_stencil_format)
{
    return o << detail::get_label(depth_stencil_format);
}

std::ostream& osc::operator<<(std::ostream& o, const RenderTextureParams& params)
{
    return o <<
        "RenderTextureParams(width = " << params.dimensions.x
        << ", height = " << params.dimensions.y
        << ", antialiasing_level = " << params.anti_aliasing_level
        << ", color_format = " << params.color_format
        << ", depth_stencil_format = " << params.depth_stencil_format
        << ")";
}

namespace
{
    template<IsAnyOf<ColorRenderBufferParams, DepthStencilRenderBufferParams> RenderBufferParams>
    class RenderBufferImpl {
    public:
        explicit RenderBufferImpl(const RenderBufferParams& params) : params_{params}
        {
            OSC_ASSERT_ALWAYS((dimensionality() != TextureDimensionality::Cube or dimensions().x == dimensions().y) && "cannot construct a Cube renderbuffer with non-square dimensions");
            OSC_ASSERT_ALWAYS((dimensionality() != TextureDimensionality::Cube or anti_aliasing_level() == AntiAliasingLevel::none()) && "cannot construct a Cube renderbuffer that is anti-aliased (not supported by backends like OpenGL)");
        }

        void reformat(const RenderBufferParams& params)
        {
            OSC_ASSERT((params.dimensionality != TextureDimensionality::Cube or params.dimensions.x == params.dimensions.y) && "cannot reformat a render buffer to a Cube dimensionality with non-square dimensions");
            OSC_ASSERT((params.dimensionality != TextureDimensionality::Cube or params.anti_aliasing_level == AntiAliasingLevel::none()) && "cannot reformat a renderbuffer to a Cube dimensionality with is anti-aliased (not supported by backends like OpenGL)");

            if (params_ != params) {
                params_ = params;
                maybe_opengl_data_->reset();
            }
        }

        const RenderBufferParams& parameters() const { return params_; }

        Vec2i dimensions() const { return params_.dimensions; }

        void set_dimensions(Vec2i new_dimensions)
        {
            OSC_ASSERT((dimensionality() != TextureDimensionality::Cube or new_dimensions.x == new_dimensions.y) && "cannot set a cubemap to have non-square dimensions");

            if (new_dimensions != dimensions()) {
                params_.dimensions = new_dimensions;
                maybe_opengl_data_->reset();
            }
        }

        TextureDimensionality dimensionality() const { return params_.dimensionality; }

        void set_dimensionality(TextureDimensionality new_dimensionality)
        {
            OSC_ASSERT((new_dimensionality != TextureDimensionality::Cube or dimensions().x == dimensions().y) && "cannot set dimensionality to Cube for non-square render buffer");
            OSC_ASSERT((new_dimensionality != TextureDimensionality::Cube or anti_aliasing_level() == AntiAliasingLevel{1}) && "cannot set dimensionality to Cube for an anti-aliased render buffer (not supported by backends like OpenGL)");

            if (new_dimensionality != dimensionality()) {
                params_.dimensionality = new_dimensionality;
                maybe_opengl_data_->reset();
            }
        }

        AntiAliasingLevel anti_aliasing_level() const { return params_.anti_aliasing_level; }

        void set_anti_aliasing_level(AntiAliasingLevel aa_level)
        {
            OSC_ASSERT((dimensionality() != TextureDimensionality::Cube or aa_level == AntiAliasingLevel{1}) && "cannot set anti-aliasing log_level_ >1 on a cube render buffer (it is not supported by backends like OpenGL)");

            if (aa_level != anti_aliasing_level()) {
                params_.anti_aliasing_level = aa_level;
                maybe_opengl_data_->reset();
            }
        }

        RenderBufferOpenGLData& upd_opengl_data()
        {
            if (not *maybe_opengl_data_) {
                upload_to_gpu();
            }
            return **maybe_opengl_data_;
        }

        void upload_to_gpu()
        {
            // dispatch _which_ texture handles are created based on render buffer params

            static_assert(num_options<TextureDimensionality>() == 2);

            if (dimensionality() == TextureDimensionality::Tex2D) {
                if (params_.anti_aliasing_level <= AntiAliasingLevel{1}) {
                    auto& t = std::get<SingleSampledTexture>((*maybe_opengl_data_).emplace(SingleSampledTexture{}));
                    configure_texture(t);
                }
                else {
                    auto& t = std::get<MultisampledRBOAndResolvedTexture>((*maybe_opengl_data_).emplace(MultisampledRBOAndResolvedTexture{}));
                    configure_texture(t);
                }
            }
            else {
                auto& t = std::get<SingleSampledCubemap>((*maybe_opengl_data_).emplace(SingleSampledCubemap{}));
                configure_texture(t);
            }
        }

        void configure_texture(SingleSampledTexture& single_sampled_texture)
        {
            using detail::equivalent_cpu_image_format_of;
            using ::equivalent_cpu_image_format_of;
            using detail::equivalent_cpu_datatype_of;
            using ::equivalent_cpu_datatype_of;

            const Vec2i dimensions = params_.dimensions;

            // setup resolved texture
            gl::bind_texture(single_sampled_texture.texture2D);
            gl::tex_image2D(
                GL_TEXTURE_2D,
                0,
                to_opengl_internal_color_format_enum(params_),
                dimensions.x,
                dimensions.y,
                0,
                opengl_format_of(equivalent_cpu_image_format_of(params_.format)),
                opengl_data_type_of(equivalent_cpu_datatype_of(params_.format)),
                nullptr
            );
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            gl::bind_texture();
        }

        void configure_texture(MultisampledRBOAndResolvedTexture& multisampled_rbo_and_texture)
        {
            using detail::equivalent_cpu_image_format_of;
            using ::equivalent_cpu_image_format_of;
            using detail::equivalent_cpu_datatype_of;
            using ::equivalent_cpu_datatype_of;

            const Vec2i dimensions = params_.dimensions;

            // setup multisampled RBO
            gl::bind_renderbuffer(multisampled_rbo_and_texture.multisampled_rbo);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                params_.anti_aliasing_level.template get_as<GLsizei>(),
                to_opengl_internal_color_format_enum(params_),
                dimensions.x,
                dimensions.y
            );
            gl::bind_renderbuffer();

            // setup resolved texture
            gl::bind_texture(multisampled_rbo_and_texture.single_sampled_texture2D);
            gl::tex_image2D(
                GL_TEXTURE_2D,
                0,
                to_opengl_internal_color_format_enum(params_),
                dimensions.x,
                dimensions.y,
                0,
                opengl_format_of(equivalent_cpu_image_format_of(params_.format)),
                opengl_data_type_of(equivalent_cpu_datatype_of(params_.format)),
                nullptr
            );
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            gl::bind_texture();
        }

        void configure_texture(SingleSampledCubemap& single_sampled_cubemap)
        {
            using detail::equivalent_cpu_image_format_of;
            using ::equivalent_cpu_image_format_of;
            using detail::equivalent_cpu_datatype_of;
            using ::equivalent_cpu_datatype_of;

            const Vec2i dimensions = params_.dimensions;

            // setup resolved texture
            gl::bind_texture(single_sampled_cubemap.cubemap);
            for (int i = 0; i < 6; ++i)
            {
                gl::tex_image2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0,
                    to_opengl_internal_color_format_enum(params_),
                    dimensions.x,
                    dimensions.y,
                    0,
                    opengl_format_of(equivalent_cpu_image_format_of(params_.format)),
                    opengl_data_type_of(equivalent_cpu_datatype_of(params_.format)),
                    nullptr
                );
            }
            gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }

        bool has_been_rendered_to() const
        {
            return maybe_opengl_data_->has_value();
        }

    protected:
        RenderBufferParams& upd_parameters() { return params_; }
        void reset_opengl_data() { maybe_opengl_data_->reset(); }
    private:
        RenderBufferParams params_;
        DefaultConstructOnCopy<std::optional<RenderBufferOpenGLData>> maybe_opengl_data_;
    };
}

class osc::SharedColorRenderBuffer::ColorRenderBuffer final : public RenderBufferImpl<ColorRenderBufferParams> {
public:
    using RenderBufferImpl<ColorRenderBufferParams>::RenderBufferImpl;

    ColorRenderBufferFormat color_format() const { return parameters().format; }

    void set_color_format(ColorRenderBufferFormat new_color_format)
    {
        if (new_color_format != color_format()) {
            upd_parameters().format = new_color_format;
            reset_opengl_data();
        }
    }
};

osc::SharedColorRenderBuffer::SharedColorRenderBuffer() :
    SharedColorRenderBuffer{ColorRenderBufferParams{}}
{}

osc::SharedColorRenderBuffer::SharedColorRenderBuffer(const ColorRenderBufferParams& params) :
    impl_{std::make_shared<ColorRenderBuffer>(params)}
{}

osc::SharedColorRenderBuffer::SharedColorRenderBuffer(const ColorRenderBuffer& impl) :
    impl_{std::make_shared<ColorRenderBuffer>(impl)}
{}

SharedColorRenderBuffer osc::SharedColorRenderBuffer::clone() const
{
    return SharedColorRenderBuffer{*impl_};
}

Vec2i osc::SharedColorRenderBuffer::dimensions() const
{
    return impl_->dimensions();
}

TextureDimensionality osc::SharedColorRenderBuffer::dimensionality() const
{
    return impl_->dimensionality();
}

AntiAliasingLevel osc::SharedColorRenderBuffer::anti_aliasing_level() const
{
    return impl_->anti_aliasing_level();
}

class osc::SharedDepthStencilRenderBuffer::DepthStencilRenderBuffer final : public RenderBufferImpl<DepthStencilRenderBufferParams> {
public:
    using RenderBufferImpl<DepthStencilRenderBufferParams>::RenderBufferImpl;

    DepthStencilRenderBufferFormat depth_stencil_format() const
    {
        return parameters().format;
    }

    void set_depth_stencil_format(DepthStencilRenderBufferFormat new_depth_stencil_format)
    {
        if (new_depth_stencil_format != depth_stencil_format()) {
            upd_parameters().format = new_depth_stencil_format;
            reset_opengl_data();
        }
    }
};

template<>
struct osc::Converter<RenderTextureParams, ColorRenderBufferParams> final {
    ColorRenderBufferParams operator()(const RenderTextureParams& params) const
    {
        return ColorRenderBufferParams{
            .dimensions = params.dimensions,
            .dimensionality = params.dimensionality,
            .anti_aliasing_level = params.anti_aliasing_level,
            .format = params.color_format,
        };
    }
};

template<>
struct osc::Converter<RenderTextureParams, DepthStencilRenderBufferParams> final {
    DepthStencilRenderBufferParams operator()(const RenderTextureParams& params) const
    {
        return DepthStencilRenderBufferParams{
            .dimensions = params.dimensions,
            .dimensionality = params.dimensionality,
            .anti_aliasing_level = params.anti_aliasing_level,
            .format = params.depth_stencil_format,
        };
    }
};

osc::SharedDepthStencilRenderBuffer::SharedDepthStencilRenderBuffer() :
    SharedDepthStencilRenderBuffer{DepthStencilRenderBufferParams{}}
{}

osc::SharedDepthStencilRenderBuffer::SharedDepthStencilRenderBuffer(const DepthStencilRenderBufferParams& params) :
    impl_{std::make_shared<DepthStencilRenderBuffer>(params)}
{}

osc::SharedDepthStencilRenderBuffer::SharedDepthStencilRenderBuffer(const DepthStencilRenderBuffer& impl) :
    impl_{std::make_shared<DepthStencilRenderBuffer>(impl)}
{}

SharedDepthStencilRenderBuffer osc::SharedDepthStencilRenderBuffer::clone() const
{
    return SharedDepthStencilRenderBuffer{*impl_};
}

Vec2i osc::SharedDepthStencilRenderBuffer::dimensions() const
{
    return impl_->dimensions();
}

TextureDimensionality osc::SharedDepthStencilRenderBuffer::dimensionality() const
{
    return impl_->dimensionality();
}

AntiAliasingLevel osc::SharedDepthStencilRenderBuffer::anti_aliasing_level() const
{
    return impl_->anti_aliasing_level();
}

DepthStencilRenderBufferFormat osc::SharedDepthStencilRenderBuffer::format() const
{
    return impl_->depth_stencil_format();
}

class osc::RenderTexture::Impl final {
public:
    Impl() : Impl{RenderTextureParams{}} {}

    explicit Impl(Vec2i dimensions) : Impl{RenderTextureParams{.dimensions = dimensions}} {}

    explicit Impl(const RenderTextureParams& params) :
        color_buffer_{to<ColorRenderBufferParams>(params)},
        depth_buffer_{to<DepthStencilRenderBufferParams>(params)}
    {}

    // note: independent `RenderTexture::Impl` should have independent data, so value-copy
    //       the underlying `RenderBuffer` here
    Impl(const Impl& src) :
        color_buffer_{src.color_buffer_.clone()},
        depth_buffer_{src.depth_buffer_.clone()}
    {}

    Impl(Impl&&) noexcept = default;

    Impl& operator=(const Impl& src)
    {
        if (&src == this) {
            return *this;
        }
        color_buffer_ = src.color_buffer_.clone();
        depth_buffer_ = src.depth_buffer_.clone();
        return *this;
    }

    Impl& operator=(Impl&&) noexcept = default;

    ~Impl() noexcept = default;

    Vec2i dimensions() const
    {
        return color_buffer_.impl_->dimensions();
    }

    void set_dimensions(Vec2i new_dimensions)
    {
        if (new_dimensions != dimensions()) {
            color_buffer_.impl_->set_dimensions(new_dimensions);
            depth_buffer_.impl_->set_dimensions(new_dimensions);
        }
    }

    TextureDimensionality dimensionality() const
    {
        return color_buffer_.impl_->dimensionality();
    }

    void set_dimensionality(TextureDimensionality new_dimensionality)
    {
        if (new_dimensionality != dimensionality()) {
            color_buffer_.impl_->set_dimensionality(new_dimensionality);
            depth_buffer_.impl_->set_dimensionality(new_dimensionality);
        }
    }

    ColorRenderBufferFormat color_format() const
    {
        return color_buffer_.impl_->color_format();
    }

    void set_color_format(ColorRenderBufferFormat new_color_format)
    {
        if (new_color_format != color_format()) {
            color_buffer_.impl_->set_color_format(new_color_format);
        }
    }

    AntiAliasingLevel anti_aliasing_level() const
    {
        return color_buffer_.impl_->anti_aliasing_level();
    }

    void set_anti_aliasing_level(AntiAliasingLevel aa_level)
    {
        if (aa_level != anti_aliasing_level()) {
            color_buffer_.impl_->set_anti_aliasing_level(aa_level);
            depth_buffer_.impl_->set_anti_aliasing_level(aa_level);
        }
    }

    DepthStencilRenderBufferFormat depth_stencil_format() const
    {
        return depth_buffer_.impl_->depth_stencil_format();
    }

    void set_depth_stencil_format(DepthStencilRenderBufferFormat new_depth_stencil_format)
    {
        if (new_depth_stencil_format != depth_stencil_format()) {
            depth_buffer_.impl_->set_depth_stencil_format(new_depth_stencil_format);
        }
    }

    void reformat(const RenderTextureParams& params)
    {
        color_buffer_.impl_->reformat(to<ColorRenderBufferParams>(params));
        depth_buffer_.impl_->reformat(to<DepthStencilRenderBufferParams>(params));
    }

    RenderBufferOpenGLData& get_color_render_buffer_data()
    {
        return color_buffer_.impl_->upd_opengl_data();
    }

    RenderBufferOpenGLData& get_depth_stencil_render_buffer_data()
    {
        return depth_buffer_.impl_->upd_opengl_data();
    }

    bool has_been_rendered_to() const
    {
        return color_buffer_.impl_->has_been_rendered_to();
    }

    SharedColorRenderBuffer upd_color_buffer()
    {
        return color_buffer_;
    }

    SharedDepthStencilRenderBuffer upd_depth_buffer()
    {
        return depth_buffer_;
    }

private:
    friend class GraphicsBackend;

    SharedColorRenderBuffer color_buffer_;
    SharedDepthStencilRenderBuffer depth_buffer_;
};

osc::RenderTexture::RenderTexture() :
    impl_{make_cow<Impl>()}
{}

osc::RenderTexture::RenderTexture(const RenderTextureParams& params) :
    impl_{make_cow<Impl>(params)}
{}

Vec2i osc::RenderTexture::dimensions() const
{
    return impl_->dimensions();
}

void osc::RenderTexture::set_dimensions(Vec2i new_dimensions)
{
    impl_.upd()->set_dimensions(new_dimensions);
}

TextureDimensionality osc::RenderTexture::dimensionality() const
{
    return impl_->dimensionality();
}

void osc::RenderTexture::set_dimensionality(TextureDimensionality dimensionality)
{
    impl_.upd()->set_dimensionality(dimensionality);
}

ColorRenderBufferFormat osc::RenderTexture::color_format() const
{
    return impl_->color_format();
}

void osc::RenderTexture::set_color_format(ColorRenderBufferFormat format)
{
    impl_.upd()->set_color_format(format);
}

AntiAliasingLevel osc::RenderTexture::anti_aliasing_level() const
{
    return impl_->anti_aliasing_level();
}

void osc::RenderTexture::set_anti_aliasing_level(AntiAliasingLevel aa_level)
{
    impl_.upd()->set_anti_aliasing_level(aa_level);
}

DepthStencilRenderBufferFormat osc::RenderTexture::depth_stencil_format() const
{
    return impl_->depth_stencil_format();
}

void osc::RenderTexture::set_depth_stencil_format(DepthStencilRenderBufferFormat depth_stencil_format)
{
    impl_.upd()->set_depth_stencil_format(depth_stencil_format);
}

void osc::RenderTexture::reformat(const RenderTextureParams& params)
{
    impl_.upd()->reformat(params);
}

SharedColorRenderBuffer osc::RenderTexture::upd_color_buffer()
{
    return impl_.upd()->upd_color_buffer();
}

SharedDepthStencilRenderBuffer osc::RenderTexture::upd_depth_buffer()
{
    return impl_.upd()->upd_depth_buffer();
}

std::ostream& osc::operator<<(std::ostream& o, const RenderTexture&)
{
    return o << "RenderTexture()";
}

namespace
{
    gl::Program compile_program_with_shimming(
        CStringView vertex_shader_src,
        CStringView fragment_shader_src,
        std::optional<CStringView> geometry_shader_src = std::nullopt)
    {
#ifndef EMSCRIPTEN
        if (geometry_shader_src) {
            return gl::create_program_from(
                gl::compile_from_source<gl::VertexShader>(vertex_shader_src.c_str()),
                gl::compile_from_source<gl::FragmentShader>(fragment_shader_src.c_str()),
                gl::compile_from_source<gl::GeometryShader>(geometry_shader_src->c_str())
            );
        }
        else {
            return gl::create_program_from(
                gl::compile_from_source<gl::VertexShader>(vertex_shader_src.c_str()),
                gl::compile_from_source<gl::FragmentShader>(fragment_shader_src.c_str())
            );
        }
#else
        // replace version string, ensure it's on the first line and `#version 300 es`
        // add `precision mediump float;` to frag shader
        const std::string shimmed_vertex_shader_src = [&vertex_shader_src]()
        {
            std::stringstream out;
            std::regex_replace(
                std::ostreambuf_iterator<char>{out},
                vertex_shader_src.begin(),
                vertex_shader_src.end(),
                std::regex{"\\s*#version.+\n"},
                "#version 300 es\n"
            );
            return std::move(out).str();
        }();
        const std::string shimmed_fragment_shader_src = [&fragment_shader_src]()
        {
            std::stringstream out;
            std::regex_replace(
                std::ostreambuf_iterator<char>{out},
                fragment_shader_src.begin(),
                fragment_shader_src.end(),
                std::regex{"\\s*#version.+\n"},
                "#version 300 es\nprecision mediump float;\n"
            );
            return std::move(out).str();
        }();
        if (geometry_shader_src) {
            const std::string shimmed_geometry_shader_src = [&geometry_shader_src]()
            {
                std::stringstream out;
                std::regex_replace(
                    std::ostreambuf_iterator<char>{out},
                    geometry_shader_src->begin(),
                    geometry_shader_src->end(),
                    std::regex{"\\s*#version.+\n"},
                    "#version 300 es\n"
                );
                return std::move(out).str();
            }();
            return gl::create_program_from(
                gl::compile_from_source<gl::VertexShader>(shimmed_vertex_shader_src.c_str()),
                gl::compile_from_source<gl::FragmentShader>(shimmed_fragment_shader_src.c_str()),
                gl::compile_from_source<gl::GeometryShader>(shimmed_geometry_shader_src.c_str())
            );
        }
        else {
            return gl::create_program_from(
                gl::compile_from_source<gl::VertexShader>(shimmed_vertex_shader_src.c_str()),
                gl::compile_from_source<gl::FragmentShader>(shimmed_fragment_shader_src.c_str())
            );
        }
#endif
    }
}

class osc::Shader::Impl final {
public:
    Impl(
        CStringView vertex_shader_src,
        CStringView fragment_shader_src) :

        program_{compile_program_with_shimming(vertex_shader_src, fragment_shader_src)}
    {
        parse_uniforms_and_attributes_from_program();
    }

    Impl(
        CStringView vertex_shader_src,
        CStringView geometry_shader_src,
        CStringView fragment_shader_src) :

        program_{compile_program_with_shimming(vertex_shader_src, fragment_shader_src, geometry_shader_src)}
    {
        parse_uniforms_and_attributes_from_program();
    }

    size_t num_properties() const
    {
        return uniforms_.size();
    }

    std::optional<size_t> property_index(std::string_view property_name) const
    {
        if (const auto it = uniforms_.find(property_name); it != uniforms_.end()) {
            return static_cast<size_t>(std::distance(uniforms_.begin(), it));
        }
        else {
            return std::nullopt;
        }
    }

    std::string_view property_name(size_t pos) const
    {
        auto it = uniforms_.begin();
        std::advance(it, pos);
        return it->first;
    }

    ShaderPropertyType property_type(size_t pos) const
    {
        auto it = uniforms_.begin();
        std::advance(it, pos);
        return it->second.shader_type;
    }

    // non-PIMPL APIs

    const gl::Program& program() const
    {
        return program_;
    }

    const FastStringHashtable<ShaderElement>& uniforms() const
    {
        return uniforms_;
    }

    const FastStringHashtable<ShaderElement>& attributes() const
    {
        return attributes_;
    }

private:
    void parse_uniforms_and_attributes_from_program()
    {
        constexpr GLsizei c_shader_max_name_length = 128;

        GLint num_attrs = 0;
        glGetProgramiv(program_.get(), GL_ACTIVE_ATTRIBUTES, &num_attrs);

        GLint num_uniforms = 0;
        glGetProgramiv(program_.get(), GL_ACTIVE_UNIFORMS, &num_uniforms);

        attributes_.reserve(num_attrs);
        for (GLint attr_idx = 0; attr_idx < num_attrs; attr_idx++) {

            GLint size = 0;                                       // size of the variable
            GLenum type = 0;                                      // type of the variable (float, vec3 or mat4, etc)
            std::array<GLchar, c_shader_max_name_length> name{};  // variable name in GLSL
            GLsizei length = 0;                                   // name length
            glGetActiveAttrib(
                program_.get() ,
                static_cast<GLuint>(attr_idx),
                static_cast<GLsizei>(name.size()),
                &length,
                &size,
                &type,
                name.data()
            );

            static_assert(sizeof(GLint) <= sizeof(int32_t));
            attributes_.try_emplace<std::string>(
                normalize_shader_element_name(name.data()),
                static_cast<int32_t>(glGetAttribLocation(program_.get(), name.data())),
                opengl_shader_type_to_osc_shader_type(type),
                static_cast<int32_t>(size)
            );
        }

        uniforms_.reserve(num_uniforms);
        for (GLint uniform_idx = 0; uniform_idx < num_uniforms; uniform_idx++) {
            GLint size = 0;                                       // size of the variable
            GLenum type = 0;                                      // type of the variable (float, vec3 or mat4, etc)
            std::array<GLchar, c_shader_max_name_length> name{};  // variable name in GLSL
            GLsizei length = 0;                                   // name length
            glGetActiveUniform(
                program_.get(),
                static_cast<GLuint>(uniform_idx),
                static_cast<GLsizei>(name.size()),
                &length,
                &size,
                &type,
                name.data()
            );

            static_assert(sizeof(GLint) <= sizeof(int32_t));
            uniforms_.try_emplace<std::string>(
                normalize_shader_element_name(name.data()),
                static_cast<int32_t>(glGetUniformLocation(program_.get(), name.data())),
                opengl_shader_type_to_osc_shader_type(type),
                static_cast<int32_t>(size)
            );
        }

        // cache commonly-used "automatic" shader elements
        //
        // it's a perf optimization: the renderer uses this to skip lookups
        maybe_model_mat_uniform_ = lookup_or_nullopt(uniforms_, "uModelMat");
        maybe_normal_mat_uniform_ = lookup_or_nullopt(uniforms_, "uNormalMat");
        maybe_view_mat_uniform_ = lookup_or_nullopt(uniforms_, "uViewMat");
        maybe_proj_mat_uniform_ = lookup_or_nullopt(uniforms_, "uProjMat");
        maybe_view_proj_mat_uniform_ = lookup_or_nullopt(uniforms_, "uViewProjMat");
        maybe_instanced_model_mat_attr_ = lookup_or_nullopt(attributes_, "aModelMat");
        maybe_instanced_normal_mat_attr_ = lookup_or_nullopt(attributes_, "aNormalMat");
    }

    friend class GraphicsBackend;

    UID id_;
    gl::Program program_;
    FastStringHashtable<ShaderElement> uniforms_;
    FastStringHashtable<ShaderElement> attributes_;
    std::optional<ShaderElement> maybe_model_mat_uniform_;
    std::optional<ShaderElement> maybe_normal_mat_uniform_;
    std::optional<ShaderElement> maybe_view_mat_uniform_;
    std::optional<ShaderElement> maybe_proj_mat_uniform_;
    std::optional<ShaderElement> maybe_view_proj_mat_uniform_;
    std::optional<ShaderElement> maybe_instanced_model_mat_attr_;
    std::optional<ShaderElement> maybe_instanced_normal_mat_attr_;
};


std::ostream& osc::operator<<(std::ostream& o, ShaderPropertyType shader_type)
{
    constexpr auto lut = []<ShaderPropertyType... Types>(OptionList<ShaderPropertyType, Types...>)
    {
        return std::to_array({ ShaderPropertyTypeTraits<Types>::name... });
    }(ShaderPropertyTypeList{});

    return o << lut.at(to_index(shader_type));
}

osc::Shader::Shader(
    CStringView vertex_shader_src,
    CStringView fragment_shader_src) :

    impl_{make_cow<Impl>(vertex_shader_src, fragment_shader_src)}
{}

osc::Shader::Shader(
    CStringView vertex_shader_src,
    CStringView geometry_shader_src,
    CStringView fragment_shader_src) :

    impl_{make_cow<Impl>(vertex_shader_src, geometry_shader_src, fragment_shader_src)}
{}

size_t osc::Shader::num_properties() const
{
    return impl_->num_properties();
}

std::optional<size_t> osc::Shader::property_index(std::string_view property_name) const
{
    return impl_->property_index(property_name);
}

std::string_view osc::Shader::property_name(size_t property_index) const
{
    return impl_->property_name(property_index);
}

ShaderPropertyType osc::Shader::property_type(size_t property_index) const
{
    return impl_->property_type(property_index);
}

std::ostream& osc::operator<<(std::ostream& o, const Shader& shader)
{
    o << "Shader(\n";
    {
        o << "    uniforms = [";

        const std::string_view delimiter = "\n        ";
        for (const auto& [name, data] : shader.impl_->uniforms()) {
            o << delimiter;
            print_shader_element(o, name, data);
        }

        o << "\n    ],\n";
    }

    {
        o << "    attributes = [";

        const std::string_view delimiter = "\n        ";
        for (const auto& [name, data] : shader.impl_->attributes()) {
            o << delimiter;
            print_shader_element(o, name, data);
        }

        o << "\n    ]\n";
    }

    o << ')';

    return o;
}

namespace
{
    GLenum to_opengl_depth_function_enum(DepthFunction depth_function)
    {
        static_assert(num_options<DepthFunction>() == 2);

        switch (depth_function) {
        case DepthFunction::LessOrEqual: return GL_LEQUAL;
        case DepthFunction::Less:        return GL_LESS;
        default:                         return GL_LESS;
        }
    }

    GLenum to_opengl_cull_face_enum(CullMode cull_mode)
    {
        static_assert(num_options<CullMode>() == 3);

        switch (cull_mode) {
        case CullMode::Front: return GL_FRONT;
        case CullMode::Back:  return GL_BACK;
        default:              return GL_BACK;
        }
    }
}

class osc::Material::Impl final {
public:
    explicit Impl(Shader shader) : shader_{std::move(shader)} {}

    const Shader& shader() const { return shader_; }

    const MaterialPropertyBlock& properties() const { return properties_; }
    MaterialPropertyBlock& upd_properties() { return properties_; }

    bool is_transparent() const { return is_transparent_; }
    void set_transparent(bool value) { is_transparent_ = value; }

    SourceBlendingFactor source_blending_factor() const { return source_blending_factor_; }
    void set_source_blending_factor(SourceBlendingFactor f) { source_blending_factor_ = f; }

    DestinationBlendingFactor destination_blending_factor() const { return destination_blending_factor_; }
    void set_destination_blending_factor(DestinationBlendingFactor f) { destination_blending_factor_ = f; }

    BlendingEquation blending_equation() const { return blending_equation_; }
    void set_blending_equation(BlendingEquation f) { blending_equation_ = f; }

    bool is_depth_tested() const { return is_depth_tested_; }
    void set_depth_tested(bool value) { is_depth_tested_ = value; }

    DepthFunction depth_function() const { return depth_function_; }
    void set_depth_function(DepthFunction depth_function) { depth_function_ = depth_function; }

    bool is_wireframe() const { return is_wireframe_mode_; }
    void set_wireframe(bool value) { is_wireframe_mode_ = value; }

    CullMode cull_mode() const { return cull_mode_; }
    void set_cull_mode(CullMode cull_mode) { cull_mode_ = cull_mode; }

private:
    friend class GraphicsBackend;

    Shader shader_;
    MaterialPropertyBlock properties_;
    DepthFunction depth_function_ = DepthFunction::Default;
    CullMode cull_mode_ = CullMode::Default;
    SourceBlendingFactor source_blending_factor_ = SourceBlendingFactor::Default;
    DestinationBlendingFactor destination_blending_factor_ = DestinationBlendingFactor::Default;
    BlendingEquation blending_equation_ = BlendingEquation::Default;
    bool is_transparent_ = false;
    bool is_depth_tested_ = true;
    bool is_wireframe_mode_ = false;
};

osc::Material::Material(Shader shader) :
    impl_{make_cow<Impl>(std::move(shader))}
{}

const Shader& osc::Material::shader() const
{
    return impl_->shader();
}

bool osc::Material::is_transparent() const
{
    return impl_->is_transparent();
}

void osc::Material::set_transparent(bool value)
{
    impl_.upd()->set_transparent(value);
}

SourceBlendingFactor osc::Material::source_blending_factor() const
{
    return impl_->source_blending_factor();
}

void osc::Material::set_source_blending_factor(SourceBlendingFactor f)
{
    impl_.upd()->set_source_blending_factor(f);
}

DestinationBlendingFactor osc::Material::destination_blending_factor() const
{
    return impl_->destination_blending_factor();
}

void osc::Material::set_destination_blending_factor(DestinationBlendingFactor f)
{
    impl_.upd()->set_destination_blending_factor(f);
}

BlendingEquation osc::Material::blending_equation() const
{
    return impl_->blending_equation();
}

void osc::Material::set_blending_equation(BlendingEquation f)
{
    impl_.upd()->set_blending_equation(f);
}

bool osc::Material::is_depth_tested() const
{
    return impl_->is_depth_tested();
}

void osc::Material::set_depth_tested(bool value)
{
    impl_.upd()->set_depth_tested(value);
}

DepthFunction osc::Material::depth_function() const
{
    return impl_->depth_function();
}

void osc::Material::set_depth_function(DepthFunction depth_function)
{
    impl_.upd()->set_depth_function(depth_function);
}

bool osc::Material::is_wireframe() const
{
    return impl_->is_wireframe();
}

void osc::Material::set_wireframe(bool value)
{
    impl_.upd()->set_wireframe(value);
}

CullMode osc::Material::cull_mode() const
{
    return impl_->cull_mode();
}

void osc::Material::set_cull_mode(CullMode cull_mode)
{
    impl_.upd()->set_cull_mode(cull_mode);
}

const MaterialPropertyBlock& osc::Material::properties() const
{
    return impl_->properties();
}

MaterialPropertyBlock& osc::Material::upd_properties()
{
    return impl_.upd()->upd_properties();
}

std::ostream& osc::operator<<(std::ostream& o, const Material&)
{
    return o << "Material()";
}


class osc::MaterialPropertyBlock::Impl final {
public:
    friend bool operator==(const Impl&, const Impl&) = default;

    void clear()
    {
        values_.clear();
    }

    bool empty() const
    {
        return values_.empty();
    }

    template<typename T, std::convertible_to<std::string_view> StringLike>
    std::optional<T> get(StringLike&& property_name) const
    {
        return get_value<T>(std::forward<StringLike>(property_name));
    }

    template<typename T, std::convertible_to<std::string_view> StringLike>
    std::optional<std::span<const T>> get_array(StringLike&& property_name) const
    {
        return get_value<std::vector<T>, std::span<const T>>(std::forward<StringLike>(property_name));
    }

    template<typename T, std::convertible_to<std::string_view> StringLike>
    void set(StringLike&& property_name, T&& value)
    {
        values_.insert_or_assign(std::forward<StringLike>(property_name), std::forward<T>(value));
    }

    template<typename T, std::convertible_to<std::string_view> StringLike>
    void set_array(StringLike&& property_name, std::span<const T> values)
    {
        values_.insert_or_assign(std::forward<StringLike>(property_name), std::vector<T>(values.begin(), values.end()));
    }

    template<std::convertible_to<std::string_view> StringLike>
    void unset(StringLike&& property_name)
    {
        values_.erase(std::forward<StringLike>(property_name));
    }

private:
    template<typename T, typename TConverted = T, std::convertible_to<std::string_view> StringLike>
    requires std::convertible_to<T, TConverted>
    std::optional<TConverted> get_value(StringLike&& property_name) const
    {
        const auto it = values_.find(std::forward<StringLike>(property_name));

        if (it == values_.end()) {
            return std::nullopt;  // property doesn't exist in lookup
        }

        const T* v = std::get_if<T>(&it->second);
        if (not v) {
            return std::nullopt;  // property has a different type than requested
        }

        if constexpr (std::same_as<T, TConverted>) {
            return *v;  // return with no conversion
        }
        else {
            return TConverted{*v}; // convert and return
        }
    }

    friend class GraphicsBackend;

    FastStringHashtable<MaterialValue> values_;
};

osc::MaterialPropertyBlock::MaterialPropertyBlock() :
    impl_{[]()
    {
        static const CopyOnUpdPtr<Impl> s_empty_property_block_impl = make_cow<Impl>();
        return s_empty_property_block_impl;
    }()}
{}

void osc::MaterialPropertyBlock::clear()
{
    impl_.upd()->clear();
}

bool osc::MaterialPropertyBlock::empty() const
{
    return impl_->empty();
}

template<>
std::optional<Color> osc::MaterialPropertyBlock::get<Color>(std::string_view property_name) const
{
    return impl_->get<Color>(property_name);
}

template<>
std::optional<Color> osc::MaterialPropertyBlock::get<Color>(const StringName& property_name) const
{
    return impl_->get<Color>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Color>(std::string_view property_name, const Color& color)
{
    impl_.upd()->set(property_name, color);
}

template<>
void osc::MaterialPropertyBlock::set<Color>(const StringName& property_name, const Color& color)
{
    impl_.upd()->set(property_name, color);
}

template<>
std::optional<std::span<const Color>> osc::MaterialPropertyBlock::get_array<Color>(std::string_view property_name) const
{
    return impl_->get_array<Color>(property_name);
}

template<>
std::optional<std::span<const Color>> osc::MaterialPropertyBlock::get_array<Color>(const StringName& property_name) const
{
    return impl_->get_array<Color>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set_array<Color>(std::string_view property_name, std::span<const Color> colors)
{
    impl_.upd()->set_array<Color>(property_name, colors);
}

template<>
void osc::MaterialPropertyBlock::set_array<Color>(const StringName& property_name, std::span<const Color> colors)
{
    impl_.upd()->set_array<Color>(property_name, colors);
}

template<>
std::optional<float> osc::MaterialPropertyBlock::get<float>(std::string_view property_name) const
{
    return impl_->get<float>(property_name);
}

template<>
std::optional<float> osc::MaterialPropertyBlock::get<float>(const StringName& property_name) const
{
    return impl_->get<float>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<float>(std::string_view property_name, const float& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<float>(const StringName& property_name, const float& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<std::span<const float>> osc::MaterialPropertyBlock::get_array<float>(std::string_view property_name) const
{
    return impl_->get_array<float>(property_name);
}

template<>
std::optional<std::span<const float>> osc::MaterialPropertyBlock::get_array<float>(const StringName& property_name) const
{
    return impl_->get_array<float>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set_array<float>(std::string_view property_name, std::span<const float> values)
{
    impl_.upd()->set_array<float>(property_name, values);
}

template<>
void osc::MaterialPropertyBlock::set_array<float>(const StringName& property_name, std::span<const float> values)
{
    impl_.upd()->set_array<float>(property_name, values);
}

template<>
std::optional<Vec2> osc::MaterialPropertyBlock::get<Vec2>(std::string_view property_name) const
{
    return impl_->get<Vec2>(property_name);
}

template<>
std::optional<Vec2> osc::MaterialPropertyBlock::get<Vec2>(const StringName& property_name) const
{
    return impl_->get<Vec2>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Vec2>(std::string_view property_name, const Vec2& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<Vec2>(const StringName& property_name, const Vec2& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<Vec3> osc::MaterialPropertyBlock::get<Vec3>(std::string_view property_name) const
{
    return impl_->get<Vec3>(property_name);
}

template<>
std::optional<Vec3> osc::MaterialPropertyBlock::get<Vec3>(const StringName& property_name) const
{
    return impl_->get<Vec3>(property_name);
}

template<>
std::optional<std::span<const Vec3>> osc::MaterialPropertyBlock::get_array<Vec3>(std::string_view property_name) const
{
    return impl_->get_array<Vec3>(property_name);
}

template<>
std::optional<std::span<const Vec3>> osc::MaterialPropertyBlock::get_array<Vec3>(const StringName& property_name) const
{
    return impl_->get_array<Vec3>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set_array<Vec3>(std::string_view property_name, std::span<const Vec3> values)
{
    impl_.upd()->set_array(property_name, values);
}

template<>
void osc::MaterialPropertyBlock::set_array<Vec3>(const StringName& property_name, std::span<const Vec3> values)
{
    impl_.upd()->set_array<Vec3>(property_name, values);
}

template<>
void osc::MaterialPropertyBlock::set<Vec3>(std::string_view property_name, const Vec3& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<Vec3>(const StringName& property_name, const Vec3& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<Vec4> osc::MaterialPropertyBlock::get<Vec4>(std::string_view property_name) const
{
    return impl_->get<Vec4>(property_name);
}

template<>
std::optional<Vec4> osc::MaterialPropertyBlock::get<Vec4>(const StringName& property_name) const
{
    return impl_->get<Vec4>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Vec4>(std::string_view property_name, const Vec4& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<Vec4>(const StringName& property_name, const Vec4& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<Mat3> osc::MaterialPropertyBlock::get<Mat3>(std::string_view property_name) const
{
    return impl_->get<Mat3>(property_name);
}

template<>
std::optional<Mat3> osc::MaterialPropertyBlock::get<Mat3>(const StringName& property_name) const
{
    return impl_->get<Mat3>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Mat3>(std::string_view property_name, const Mat3& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<Mat3>(const StringName& property_name, const Mat3& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<Mat4> osc::MaterialPropertyBlock::get<Mat4>(std::string_view property_name) const
{
    return impl_->get<Mat4>(property_name);
}

template<>
std::optional<Mat4> osc::MaterialPropertyBlock::get<Mat4>(const StringName& property_name) const
{
    return impl_->get<Mat4>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Mat4>(std::string_view property_name, const Mat4& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<Mat4>(const StringName& property_name, const Mat4& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<std::span<const Mat4>> osc::MaterialPropertyBlock::get_array<Mat4>(std::string_view property_name) const
{
    return impl_->get_array<Mat4>(property_name);
}

template<>
std::optional<std::span<const Mat4>> osc::MaterialPropertyBlock::get_array<Mat4>(const StringName& property_name) const
{
    return impl_->get_array<Mat4>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set_array<Mat4>(std::string_view property_name, std::span<const Mat4> values)
{
    impl_.upd()->set_array(property_name, values);
}

template<>
void osc::MaterialPropertyBlock::set_array<Mat4>(const StringName& property_name, std::span<const Mat4> values)
{
    impl_.upd()->set_array(property_name, values);
}

template<>
std::optional<int> osc::MaterialPropertyBlock::get<int>(std::string_view property_name) const
{
    return impl_->get<int>(property_name);
}

template<>
std::optional<int> osc::MaterialPropertyBlock::get<int>(const StringName& property_name) const
{
    return impl_->get<int>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<int>(std::string_view property_name, const int& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<int>(const StringName& property_name, const int& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<bool> osc::MaterialPropertyBlock::get<bool>(std::string_view property_name) const
{
    return impl_->get<bool>(property_name);
}

template<>
std::optional<bool> osc::MaterialPropertyBlock::get<bool>(const StringName& property_name) const
{
    return impl_->get<bool>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<bool>(std::string_view property_name, bool value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<bool>(const StringName& property_name, bool value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<Texture2D> osc::MaterialPropertyBlock::get<Texture2D>(std::string_view property_name) const
{
    return impl_->get<Texture2D>(property_name);
}

template<>
std::optional<Texture2D> osc::MaterialPropertyBlock::get<Texture2D>(const StringName& property_name) const
{
    return impl_->get<Texture2D>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Texture2D>(std::string_view property_name, const Texture2D& texture)
{
    impl_.upd()->set(property_name, texture);
}

template<>
void osc::MaterialPropertyBlock::set<Texture2D>(const StringName& property_name, const Texture2D& texture)
{
    impl_.upd()->set(property_name, texture);
}

template<>
std::optional<RenderTexture> osc::MaterialPropertyBlock::get<RenderTexture>(std::string_view property_name) const
{
    return impl_->get<RenderTexture>(property_name);
}

template<>
std::optional<RenderTexture> osc::MaterialPropertyBlock::get<RenderTexture>(const StringName& property_name) const
{
    return impl_->get<RenderTexture>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<RenderTexture>(std::string_view property_name, const RenderTexture& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<RenderTexture>(const StringName& property_name, const RenderTexture& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<Cubemap> osc::MaterialPropertyBlock::get<Cubemap>(std::string_view property_name) const
{
    return impl_->get<Cubemap>(property_name);
}

template<>
std::optional<Cubemap> osc::MaterialPropertyBlock::get<Cubemap>(const StringName& property_name) const
{
    return impl_->get<Cubemap>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<Cubemap>(std::string_view property_name, const Cubemap& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<Cubemap>(const StringName& property_name, const Cubemap& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<SharedColorRenderBuffer> osc::MaterialPropertyBlock::get<SharedColorRenderBuffer>(std::string_view property_name) const
{
    return impl_->get<SharedColorRenderBuffer>(property_name);
}

template<>
std::optional<SharedColorRenderBuffer> osc::MaterialPropertyBlock::get<SharedColorRenderBuffer>(const StringName& property_name) const
{
    return impl_->get<SharedColorRenderBuffer>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<SharedColorRenderBuffer>(std::string_view property_name, const SharedColorRenderBuffer& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<SharedColorRenderBuffer>(const StringName& property_name, const SharedColorRenderBuffer& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
std::optional<SharedDepthStencilRenderBuffer> osc::MaterialPropertyBlock::get<SharedDepthStencilRenderBuffer>(std::string_view property_name) const
{
    return impl_->get<SharedDepthStencilRenderBuffer>(property_name);
}

template<>
std::optional<SharedDepthStencilRenderBuffer> osc::MaterialPropertyBlock::get<SharedDepthStencilRenderBuffer>(const StringName& property_name) const
{
    return impl_->get<SharedDepthStencilRenderBuffer>(property_name);
}

template<>
void osc::MaterialPropertyBlock::set<SharedDepthStencilRenderBuffer>(std::string_view property_name, const SharedDepthStencilRenderBuffer& value)
{
    impl_.upd()->set(property_name, value);
}

template<>
void osc::MaterialPropertyBlock::set<SharedDepthStencilRenderBuffer>(const StringName& property_name, const SharedDepthStencilRenderBuffer& value)
{
    impl_.upd()->set(property_name, value);
}

void osc::MaterialPropertyBlock::unset(std::string_view property_name)
{
    impl_.upd()->unset(property_name);
}

void osc::MaterialPropertyBlock::unset(const StringName& property_name)
{
    impl_.upd()->unset(property_name);
}

bool osc::operator==(const MaterialPropertyBlock& lhs, const MaterialPropertyBlock& rhs)
{
    return lhs.impl_ == rhs.impl_ || *lhs.impl_ == *rhs.impl_;
}

std::ostream& osc::operator<<(std::ostream& o, const MaterialPropertyBlock&)
{
    return o << "MaterialPropertyBlock()";
}

// define an `Unorm8` to `Snorm8` `Converter`, so that vertex buffers
// support this (re)encoding pathway
template<>
struct osc::Converter<Unorm8, Snorm8> final {
    Snorm8 operator()(Unorm8 v) const
    {
        return Snorm8{v.normalized_value()};
    }
};

// define a `Snorm8` to `Unorm8` `Converter`, so that vertex buffers
// support this (re)encoding pathway
template<>
struct osc::Converter<Snorm8, Unorm8> final {
    Unorm8 operator()(Snorm8 v) const
    {
        return Unorm8{v.normalized_value()};
    }
};

namespace
{
    constexpr auto c_mesh_topology_strings = std::to_array<CStringView>(
    {
        "Triangles",
        "Lines",
    });
    static_assert(c_mesh_topology_strings.size() == num_options<MeshTopology>());

    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
    static_assert(alignof(PackedIndex) == alignof(uint32_t));

    GLenum to_opengl_topology_enum(MeshTopology mesh_topology)
    {
        static_assert(num_options<MeshTopology>() == 2);

        switch (mesh_topology) {
        case MeshTopology::Triangles: return GL_TRIANGLES;
        case MeshTopology::Lines:     return GL_LINES;
        default:                      return GL_TRIANGLES;
        }
    }

    // types that can be read/written to/from a vertex buffer by higher
    // levels of the API
    template<typename T>
    concept UserFacingVertexData = IsAnyOf<T, Vec2, Vec3, Vec4, Vec<4, Unorm8>, Vec<4, Snorm8>, Color, Color32>;

    // types that are encode-/decode-able into a vertex buffer
    template<typename T>
    concept VertexBufferComponent = IsAnyOf<T, float, Unorm8, Snorm8>;

    // low-level single-component decoder function
    //
    // relies on the fact that every `VertexBufferComponent<T>` has a conversion operation
    // to every other `VertexBufferComponent<T>`
    template<VertexBufferComponent EncodedValue, typename DecodedValue>
    DecodedValue decode(const std::byte* b)
    {
        const EncodedValue& encoded_value = *std::launder(reinterpret_cast<const EncodedValue*>(b));
        return to<DecodedValue>(encoded_value);
    }

    // low-level single-component encoder function
    //
    // relies on the fact that every `VertexBufferComponent<T>` has a conversion operation
    // to every other `VertexBufferComponent<T>`
    template<typename DecodedValue, VertexBufferComponent EncodedValue>
    void encode(std::byte* p, DecodedValue v)
    {
        EncodedValue& encoded_value = *std::launder(reinterpret_cast<EncodedValue*>(p));
        encoded_value = to<EncodedValue>(v);
    }

    // mid-level multi-component decode/encode functions
    template<UserFacingVertexData T, VertexAttributeFormat EncodingFormat>
    void encode_many(std::byte* ptr, const T& values)
    {
        using ComponentType = typename VertexAttributeFormatTraits<EncodingFormat>::component_type;
        constexpr auto num_components = num_components_in(EncodingFormat);
        constexpr auto sizeof_component = component_size(EncodingFormat);
        constexpr auto n = min(std::tuple_size_v<T>, static_cast<typename T::size_type>(num_components));

        for (typename T::size_type i = 0; i < n; ++i) {
            encode<typename T::value_type, ComponentType>(ptr + i*sizeof_component, values[i]);
        }
    }

    template<VertexAttributeFormat EncodingFormat, UserFacingVertexData T>
    T decode_many(const std::byte* ptr)
    {
        using ComponentType = typename VertexAttributeFormatTraits<EncodingFormat>::component_type;
        constexpr auto num_components = num_components_in(EncodingFormat);
        constexpr auto sizeof_component = component_size(EncodingFormat);
        constexpr auto n = min(std::tuple_size_v<T>, static_cast<typename T::size_type>(num_components));

        T rv{};
        for (typename T::size_type i = 0; i < n; ++i) {
            rv[i] = decode<ComponentType, typename T::value_type>(ptr + i*sizeof_component);
        }
        return rv;
    }

    // high-level, compile-time multi-component decode + encode definition
    template<UserFacingVertexData T>
    class MultiComponentEncoding final {
    public:
        explicit MultiComponentEncoding(VertexAttributeFormat attribute_format)
        {
            static_assert(num_options<VertexAttributeFormat>() == 5);

            switch (attribute_format) {
            case VertexAttributeFormat::Float32x2:
                encoder_ = encode_many<T, VertexAttributeFormat::Float32x2>;
                decoder_ = decode_many<VertexAttributeFormat::Float32x2, T>;
                break;
            case VertexAttributeFormat::Float32x3:
                encoder_ = encode_many<T, VertexAttributeFormat::Float32x3>;
                decoder_ = decode_many<VertexAttributeFormat::Float32x3, T>;
                break;
            default:
            case VertexAttributeFormat::Float32x4:
                encoder_ = encode_many<T, VertexAttributeFormat::Float32x4>;
                decoder_ = decode_many<VertexAttributeFormat::Float32x4, T>;
                break;
            case VertexAttributeFormat::Unorm8x4:
                encoder_ = encode_many<T, VertexAttributeFormat::Unorm8x4>;
                decoder_ = decode_many<VertexAttributeFormat::Unorm8x4, T>;
                break;
            case VertexAttributeFormat::Snorm8x4:
                encoder_ = encode_many<T, VertexAttributeFormat::Snorm8x4>;
                decoder_ = decode_many<VertexAttributeFormat::Snorm8x4, T>;
                break;
            }
        }

        void encode(std::byte* ptr, const T& values) const
        {
            encoder_(ptr, values);
        }

        T decode(const std::byte* ptr) const
        {
            return decoder_(ptr);
        }

        friend bool operator==(const MultiComponentEncoding&, const MultiComponentEncoding&) = default;
    private:
        using Encoder = void(*)(std::byte*, const T&);
        Encoder encoder_;

        using Decoder = T(*)(const std::byte*);
        Decoder decoder_;
    };

    // a single compile-time reencoding function
    //
    // decodes in-memory data in a source format, converts it to a destination format, and then
    // writes it to the destination memory
    template<VertexAttributeFormat SourceFormat, VertexAttributeFormat DestinationFormat>
    void reencode_many(std::span<const std::byte> src, std::span<std::byte> dest)
    {
        using SourceCPUFormat = typename VertexAttributeFormatTraits<SourceFormat>::type;
        using DestCPUFormat = typename VertexAttributeFormatTraits<DestinationFormat>::type;
        constexpr auto n = min(std::tuple_size_v<SourceCPUFormat>, std::tuple_size_v<DestCPUFormat>);

        const auto decoded = decode_many<SourceFormat, SourceCPUFormat>(src.data());
        DestCPUFormat converted{};
        for (size_t i = 0; i < n; ++i) {
            converted[i] = to<typename DestCPUFormat::value_type>(decoded[i]);
        }
        encode_many<DestCPUFormat, DestinationFormat>(dest.data(), converted);
    }

    // type-erased (i.e. runtime) reencoder function
    using ReencoderFunction = void(*)(std::span<const std::byte>, std::span<std::byte>);

    // compile-time lookup table (LUT) for runtime reencoder functions
    class ReencoderLut final {
    private:
        static constexpr size_t index_of(VertexAttributeFormat source_format, VertexAttributeFormat destination_format)
        {
            return static_cast<size_t>(source_format)*num_options<VertexAttributeFormat>() + static_cast<size_t>(destination_format);
        }

        template<VertexAttributeFormat... Formats>
        static constexpr void write_entries_top_level(ReencoderLut& lut, OptionList<VertexAttributeFormat, Formats...>)
        {
            (write_entries<Formats, Formats...>(lut), ...);
        }

        template<VertexAttributeFormat SourceFormat, VertexAttributeFormat... DestinationFormats>
        static constexpr void write_entries(ReencoderLut& lut)
        {
            (write_entry<SourceFormat, DestinationFormats>(lut), ...);
        }

        template<VertexAttributeFormat SourceFormat, VertexAttributeFormat DestinationFormat>
        static constexpr void write_entry(ReencoderLut& lut)
        {
            lut.assign(SourceFormat, DestinationFormat, reencode_many<SourceFormat, DestinationFormat>);
        }
    public:
        constexpr ReencoderLut()
        {
            write_entries_top_level(*this, VertexAttributeFormatList{});
        }

        constexpr void assign(
            VertexAttributeFormat source_format,
            VertexAttributeFormat destination_format,
            ReencoderFunction reencoder_function)
        {
            storage_.at(index_of(source_format, destination_format)) = reencoder_function;
        }

        constexpr const ReencoderFunction& lookup(
            VertexAttributeFormat source_format,
            VertexAttributeFormat destination_format) const
        {
            return storage_.at(index_of(source_format, destination_format));
        }

    private:
        std::array<ReencoderFunction, num_options<VertexAttributeFormat>()*num_options<VertexAttributeFormat>()> storage_{};
    };

    constexpr ReencoderLut c_reencoder_lut;

    struct VertexBufferAttributeReencoder final {
        ReencoderFunction reencoder_function;
        size_t source_offset;
        size_t source_stride;
        size_t destination_offset;
        size_t destination_stride;
    };

    std::vector<VertexBufferAttributeReencoder> get_attribute_reencoders(
        const VertexFormat& source_format,
        const VertexFormat& destination_format)
    {
        std::vector<VertexBufferAttributeReencoder> rv;
        rv.reserve(destination_format.num_attributes());  // guess

        for (const auto destination_layout : destination_format.attribute_layouts()) {
            if (const auto source_layout = source_format.attribute_layout(destination_layout.attribute())) {
                rv.push_back({
                    c_reencoder_lut.lookup(source_layout->format(), destination_layout.format()),
                    source_layout->offset(),
                    source_layout->stride(),
                    destination_layout.offset(),
                    destination_layout.stride(),
                });
            }
        }
        return rv;
    }

    void reencode_vertex_buffer(
        std::span<const std::byte> source_bytes,
        const VertexFormat& source_format,
        std::span<std::byte> destination_bytes,
        const VertexFormat& destination_format)
    {
        const size_t source_stride = source_format.stride();
        const size_t destination_stride = destination_format.stride();

        if (source_stride == 0 or destination_stride == 0) {
            return;  // no reencoding necessary
        }

        OSC_ASSERT(source_bytes.size() % source_stride == 0);
        OSC_ASSERT(destination_bytes.size() % destination_stride == 0);

        const size_t n = min(source_bytes.size() / source_stride, destination_bytes.size() / destination_stride);

        const auto reencoders = get_attribute_reencoders(source_format, destination_format);
        for (size_t i = 0; i < n; ++i) {
            const auto source_vertex_data = source_bytes.subspan(i*source_stride);
            const auto destination_vertex_data = destination_bytes.subspan(i*destination_stride);

            for (const auto& reencoder : reencoders) {
                const auto source_attr_data = source_vertex_data.subspan(reencoder.source_offset, reencoder.source_stride);
                const auto destination_attr_data = destination_vertex_data.subspan(reencoder.destination_offset, reencoder.destination_stride);
                reencoder.reencoder_function(source_attr_data, destination_attr_data);
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
            using Byte = std::conditional_t<IsConst, const std::byte, std::byte>;

            AttributeValueProxy(Byte* data, MultiComponentEncoding<T> encoding) :
                data_{data},
                encoding_{encoding}
            {}

            AttributeValueProxy& operator=(const T& value)
                requires (not IsConst)
            {
                encoding_.encode(data_, value);
                return *this;
            }

            operator T () const  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            {
                return encoding_.decode(data_);
            }

            template<typename U>
            requires (not IsConst)
            AttributeValueProxy& operator/=(const U& value)
            {
                return *this = (T{*this} /= value);
            }

            template<typename U>
            requires (not IsConst)
            AttributeValueProxy& operator+=(const U& value)
            {
                return *this = (T{*this} += value);
            }
        private:
            Byte* data_;
            MultiComponentEncoding<T> encoding_;
        };

        // iterator for vertex buffer's contents (via encoders/decoders)
        template<UserFacingVertexData T, bool IsConst>
        class AttributeValueIterator final {
        public:
            using difference_type = ptrdiff_t;
            using value_type = AttributeValueProxy<T, IsConst>;
            using reference = value_type;
            using iterator_category = std::random_access_iterator_tag;

            using Byte = std::conditional_t<IsConst, const std::byte, std::byte>;

            AttributeValueIterator(
                Byte* data,
                size_t stride,
                MultiComponentEncoding<T> encoding) :

                data_{data},
                stride_{stride},
                encoding_{encoding}
            {}

            AttributeValueProxy<T, IsConst> operator*() const
            {
                return AttributeValueProxy<T, IsConst>{data_, encoding_};
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

            AttributeValueIterator& operator+=(difference_type n)
            {
                data_ += n*stride_;
                return *this;
            }

            AttributeValueIterator operator+(difference_type n) const
            {
                auto copy = *this;
                copy += n;
                return copy;
            }

            AttributeValueIterator& operator-=(difference_type n)
            {
                data_ -= n*stride_;
            }

            AttributeValueIterator operator-(difference_type n)
            {
                auto copy = *this;
                copy -= n;
                return copy;
            }

            difference_type operator-(const AttributeValueIterator& rhs) const
            {
                return (data_ - rhs.data_) / stride_;
            }

            AttributeValueProxy<T, IsConst> operator[](difference_type pos) const
            {
                return *(*this + pos);
            }

            bool operator<(const AttributeValueIterator& rhs) const
            {
                return data_ < rhs.data_;
            }

            bool operator>(const AttributeValueIterator& rhs) const
            {
                return data_ > rhs.data_;
            }

            bool operator<=(const AttributeValueIterator& rhs) const
            {
                return data_ <= rhs.data_;
            }

            bool operator>=(const AttributeValueIterator& rhs) const
            {
                return data_ >= rhs.data_;
            }

            friend bool operator==(const AttributeValueIterator&, const AttributeValueIterator&) = default;
        private:
            Byte* data_;
            size_t stride_;
            MultiComponentEncoding<T> encoding_;
        };

        // range (C++20) for vertex buffer's contents
        template<UserFacingVertexData T, bool IsConst>
        class AttributeValueRange final {
        public:
            using Byte = std::conditional_t<IsConst, const std::byte, std::byte>;
            using iterator = AttributeValueIterator<T, IsConst>;
            using value_type = typename iterator::value_type;
            using difference_type = typename iterator::difference_type;

            AttributeValueRange() = default;

            AttributeValueRange(
                std::span<Byte> data,
                size_t stride,
                VertexAttributeFormat format) :

                data_{data},
                stride_{stride},
                encoding_{format}
            {}

            difference_type size() const
            {
                return std::distance(begin(), end());
            }

            iterator begin() const
            {
                return {data_.data(), stride_, encoding_};
            }

            iterator end() const
            {
                return {data_.data() + data_.size(), stride_, encoding_};
            }

            value_type at(difference_type pos) const
            {
                return at(*this, pos);
            }

            value_type at(difference_type pos)
            {
                return at(*this, pos);
            }

            value_type operator[](difference_type pos) const
            {
                return begin()[pos];
            }
        private:
            template<typename AttrValueRange>
            static value_type at(AttrValueRange&& range, difference_type pos)
            {
                const auto beg = range.begin();
                if (pos >= std::distance(beg, range.end())) {
                    throw std::out_of_range{"an attribute value was out-of-range: this is usually because of out-of-range mesh indices"};
                }
                return beg[pos];
            }

            std::span<Byte> data_{};
            size_t stride_ = 1;  // care: divide by zero in an iterator is UB
            MultiComponentEncoding<T> encoding_{VertexAttributeFormat::Float32x3};  // dummy, for default ctor
        };

        // default ctor: make an empty buffer
        VertexBuffer() = default;

        void clear()
        {
            data_.clear();
            vertex_format_.clear();
        }

        size_t num_vertices() const
        {
            return vertex_format_.empty() ? 0 : (data_.size() / vertex_format_.stride());
        }

        size_t num_attributes() const
        {
            return vertex_format_.num_attributes();
        }

        size_t stride() const
        {
            return vertex_format_.stride();
        }

        [[nodiscard]] bool has_vertices() const
        {
            return num_vertices() > 0;
        }

        std::span<const std::byte> bytes() const
        {
            return data_;
        }

        const VertexFormat& format() const
        {
            return vertex_format_;
        }

        auto attribute_layouts() const
        {
            return vertex_format_.attribute_layouts();
        }

        bool has_attribute(VertexAttribute attribute) const
        {
            return vertex_format_.contains(attribute);
        }

        template<UserFacingVertexData T>
        auto iter(VertexAttribute attribute) const
        {
            if (const auto layout = vertex_format_.attribute_layout(attribute)) {
                std::span<const std::byte> offset_span{data_.data() + layout->offset(), data_.size()};

                return AttributeValueRange<T, true>{
                    offset_span,
                    vertex_format_.stride(),
                    layout->format(),
                };
            }
            else {
                return AttributeValueRange<T, true>{};
            }
        }

        template<UserFacingVertexData T>
        auto iter(VertexAttribute attribute)
        {
            if (const auto layout = vertex_format_.attribute_layout(attribute)) {
                std::span<std::byte> offset_span{data_.data() + layout->offset(), data_.size()};
                return AttributeValueRange<T, false>{
                    offset_span,
                    vertex_format_.stride(),
                    layout->format(),
                };
            }
            else {
                return AttributeValueRange<T, false>{};
            }
        }

        template<UserFacingVertexData T>
        std::vector<T> read(VertexAttribute attribute) const
        {
            const auto range = iter<T>(attribute);
            return std::vector<T>(range.begin(), range.end());
        }

        template<UserFacingVertexData T>
        void write(VertexAttribute attribute, std::span<const T> values)
        {
            // edge-case: size == 0 should be treated as "wipe/ignore it"
            if (values.empty()) {
                if (vertex_format_.contains(attribute)) {
                    VertexFormat new_format{vertex_format_};
                    new_format.erase(attribute);
                    set_params(num_vertices(), new_format);
                }
                return;  // ignore/wipe
            }

            if (attribute != VertexAttribute::Position) {
                if (values.size() != num_vertices()) {
                    // non-`Position` attributes must be size-matched
                    return;
                }

                if (not vertex_format_.contains(VertexAttribute::Position)) {
                    // callers must've already assigned `Position` before this
                    // function is able to assign additional attributes
                    return;
                }
            }

            if (not vertex_format_.contains(attribute)) {
                // reformat
                VertexFormat new_format{vertex_format_};
                new_format.insert({attribute, default_format(attribute)});
                set_params(values.size(), new_format);
            }
            else if (values.size() != num_vertices()) {
                // resize
                set_params(values.size(), vertex_format_);
            }

            // write els to vertex buffer
            copy(values.begin(), values.end(), iter<T>(attribute).begin());
        }

        template<UserFacingVertexData T, typename UnaryOperation>
        requires std::invocable<UnaryOperation, T>
        void transform_attribute(VertexAttribute attribute, UnaryOperation transformer)
        {
            for (auto&& proxy : iter<T>(attribute)) {
                proxy = transformer(proxy);
            }
        }

        bool emplace_attribute_descriptor(VertexAttributeDescriptor descriptor)
        {
            if (has_attribute(descriptor.attribute())) {
                return false;
            }

            auto copy = format();
            copy.insert(descriptor);
            set_format(copy);
            return true;
        }

        void set_params(size_t new_num_verts, const VertexFormat& new_format)
        {
            if (data_.empty()) {
                // zero-initialize the buffer in the "new" format
                data_.resize(new_num_verts * new_format.stride());
                vertex_format_ = new_format;
            }

            if (new_format != vertex_format_) {
                // initialize a new buffer and re-encode the old one in the new format
                std::vector<std::byte> new_buffer(new_num_verts * new_format.stride());
                reencode_vertex_buffer(data_, vertex_format_, new_buffer, new_format);
                data_ = std::move(new_buffer);
                vertex_format_ = new_format;
            }
            else if (new_num_verts != num_vertices()) {
                // resize (zero-initialized, if growing) the buffer
                data_.resize(new_num_verts * vertex_format_.stride());
            }
            else {
                // no change in format or size, do nothing
            }
        }

        void set_format(const VertexFormat& new_format)
        {
            set_params(num_vertices(), new_format);
        }

        void set_data(std::span<const std::byte> data)
        {
            OSC_ASSERT(data.size() == data_.size() && "provided data size does not match the size of the vertex buffer");
            data_.assign(data.begin(), data.end());
        }
    private:
        std::vector<std::byte> data_;
        VertexFormat vertex_format_;
    };
}

class osc::Mesh::Impl final {
public:

    MeshTopology topology() const
    {
        return topology_;
    }

    void set_topology(MeshTopology topology)
    {
        topology_ = topology;
        version_->reset();
    }

    size_t num_vertices() const
    {
        return vertex_buffer_.num_vertices();
    }

    bool has_vertices() const
    {
        return vertex_buffer_.has_vertices();
    }

    std::vector<Vec3> vertices() const
    {
        return vertex_buffer_.read<Vec3>(VertexAttribute::Position);
    }

    void set_vertices(std::span<const Vec3> vertices)
    {
        vertex_buffer_.write<Vec3>(VertexAttribute::Position, vertices);

        range_check_indices_and_recalculate_bounds();
        version_->reset();
    }

    void transform_vertices(const std::function<Vec3(Vec3)>& transformer)
    {
        vertex_buffer_.transform_attribute<Vec3>(VertexAttribute::Position, transformer);

        range_check_indices_and_recalculate_bounds();
        version_->reset();
    }

    void transform_vertices(const Transform& transform)
    {
        vertex_buffer_.transform_attribute<Vec3>(VertexAttribute::Position, [&transform](Vec3 vertex)
        {
            return transform * vertex;
        });

        range_check_indices_and_recalculate_bounds();
        version_->reset();
    }

    void transform_vertices(const Mat4& mat4)
    {
        vertex_buffer_.transform_attribute<Vec3>(VertexAttribute::Position, [&mat4](Vec3 vertex)
        {
            return transform_point(mat4, vertex);
        });

        range_check_indices_and_recalculate_bounds();
        version_->reset();
    }

    bool has_normals() const
    {
        return vertex_buffer_.has_attribute(VertexAttribute::Normal);
    }

    std::vector<Vec3> normals() const
    {
        return vertex_buffer_.read<Vec3>(VertexAttribute::Normal);
    }

    void set_normals(std::span<const Vec3> normals)
    {
        vertex_buffer_.write<Vec3>(VertexAttribute::Normal, normals);

        version_->reset();
    }

    void transform_normals(const std::function<Vec3(Vec3)>& transformer)
    {
        vertex_buffer_.transform_attribute<Vec3>(VertexAttribute::Normal, transformer);

        version_->reset();
    }

    bool has_tex_coords() const
    {
        return vertex_buffer_.has_attribute(VertexAttribute::TexCoord0);
    }

    std::vector<Vec2> tex_coords() const
    {
        return vertex_buffer_.read<Vec2>(VertexAttribute::TexCoord0);
    }

    void set_tex_coords(std::span<const Vec2> coords)
    {
        vertex_buffer_.write<Vec2>(VertexAttribute::TexCoord0, coords);

        version_->reset();
    }

    void transform_tex_coords(const std::function<Vec2(Vec2)>& transformer)
    {
        vertex_buffer_.transform_attribute<Vec2>(VertexAttribute::TexCoord0, transformer);

        version_->reset();
    }

    std::vector<Color> colors() const
    {
        return vertex_buffer_.read<Color>(VertexAttribute::Color);
    }

    void set_colors(std::span<const Color> colors)
    {
        vertex_buffer_.write<Color>(VertexAttribute::Color, colors);

        version_.reset();
    }

    std::vector<Vec4> tangents() const
    {
        return vertex_buffer_.read<Vec4>(VertexAttribute::Tangent);
    }

    void set_tangents(std::span<const Vec4> tangents)
    {
        vertex_buffer_.write<Vec4>(VertexAttribute::Tangent, tangents);

        version_->reset();
    }

    size_t num_indices() const
    {
        return num_indices_;
    }

    MeshIndicesView indices() const
    {
        if (num_indices_ <= 0) {
            return {};
        }
        else if (indices_are_32bit_) {
            return {&indices_data_.front().u32, num_indices_};
        }
        else {  // indices are 16-bit
            return {&indices_data_.front().u16.a, num_indices_};
        }
    }

    void set_indices(MeshIndicesView indices, MeshUpdateFlags flags)
    {
        if (indices.is_uint16()) {
            set_indices(indices.to_uint16_span(), flags);
        }
        else {
            set_indices(indices.to_uint32_span(), flags);
        }
    }

    void for_each_indexed_vertex(const std::function<void(Vec3)>& callback) const
    {
        const auto positions = vertex_buffer_.iter<Vec3>(VertexAttribute::Position).begin();
        for (auto index : indices()) {
            callback(positions[index]);
        }
    }

    void for_each_indexed_triangle(const std::function<void(Triangle)>& callback) const
    {
        if (topology_ != MeshTopology::Triangles) {
            return;
        }

        const MeshIndicesView mesh_indices = indices();
        const size_t steps = (mesh_indices.size() / 3) * 3;

        const auto positions = vertex_buffer_.iter<Vec3>(VertexAttribute::Position).begin();
        for (size_t i = 0; i < steps; i += 3) {

            // can use unchecked access here: `mesh_indices` are range-checked on writing
            callback(Triangle{
                positions[mesh_indices[i]],
                positions[mesh_indices[i+1]],
                positions[mesh_indices[i+2]],
            });
        }
    }

    Triangle get_triangle_at(size_t first_index_offset) const
    {
        if (topology_ != MeshTopology::Triangles) {
            throw std::runtime_error{"cannot call get_triangle_at on a non-triangular-topology mesh"};
        }

        const auto mesh_indices = indices();

        if (first_index_offset+2 >= mesh_indices.size()) {
            throw std::runtime_error{"provided first index offset is out-of-bounds"};
        }

        const auto mesh_vertices = vertex_buffer_.iter<Vec3>(VertexAttribute::Position);

        // can use unchecked access here: `mesh_indices` are range-checked on writing
        return Triangle{
            mesh_vertices[mesh_indices[first_index_offset+0]],
            mesh_vertices[mesh_indices[first_index_offset+1]],
            mesh_vertices[mesh_indices[first_index_offset+2]],
        };
    }

    std::vector<Vec3> indexed_vertices() const
    {
        std::vector<Vec3> rv;
        rv.reserve(num_indices());
        for_each_indexed_vertex([&rv](Vec3 v) { rv.push_back(v); });
        return rv;
    }

    const AABB& bounds() const
    {
        return aabb_;
    }

    void clear()
    {
        version_->reset();
        topology_ = MeshTopology::Default;
        vertex_buffer_.clear();
        indices_are_32bit_ = false;
        num_indices_ = 0;
        indices_data_.clear();
        aabb_ = {};
        submesh_descriptors_.clear();
    }

    size_t num_submesh_descriptors() const
    {
        return submesh_descriptors_.size();
    }

    void push_submesh_descriptor(const SubMeshDescriptor& descriptor)
    {
        submesh_descriptors_.push_back(descriptor);
    }

    const SubMeshDescriptor& submesh_descriptor_at(size_t pos) const
    {
        return submesh_descriptors_.at(pos);
    }

    void clear_submesh_descriptors()
    {
        submesh_descriptors_.clear();
    }

    size_t num_vertex_attributes() const
    {
        return vertex_buffer_.num_attributes();
    }

    const VertexFormat& vertex_format() const
    {
        return vertex_buffer_.format();
    }

    void set_vertex_buffer_params(size_t num_vertices, const VertexFormat& format)
    {
        vertex_buffer_.set_params(num_vertices, format);

        range_check_indices_and_recalculate_bounds();
        version_->reset();
    }

    size_t vertex_buffer_stride() const
    {
        return vertex_buffer_.stride();
    }

    void set_vertex_buffer_data(std::span<const uint8_t> data, MeshUpdateFlags flags)
    {
        vertex_buffer_.set_data(std::as_bytes(data));

        range_check_indices_and_recalculate_bounds(flags);
        version_->reset();
    }

    void recalculate_normals()
    {
        if (topology() != MeshTopology::Triangles) {
            // if the mesh isn't triangle-based, do nothing
            return;
        }

        // ensure the vertex buffer has a normal attribute
        vertex_buffer_.emplace_attribute_descriptor({VertexAttribute::Normal, VertexAttributeFormat::Float32x3});

        // calculate normals from triangle faces:
        //
        // - keep a count of the number of times a normal has been assigned
        // - compute the normal from the triangle
        // - if counts[i] == 0 assign it (we can't assume the buffer is zeroed - could be reused)
        // - else, add (accumulate)
        // - ++counts[i]
        // - at the end, if counts[i] > 1, then renormalize that normal (it contains a sum)

        const auto mesh_indices = indices();
        const auto positions = vertex_buffer_.iter<Vec3>(VertexAttribute::Position);
        auto normals = vertex_buffer_.iter<Vec3>(VertexAttribute::Normal);
        std::vector<uint16_t> counts(normals.size());

        for (size_t i = 0, len = 3*(mesh_indices.size()/3); i < len; i+=3) {
            // get triangle indices
            const Vec3uz idxs = {mesh_indices[i], mesh_indices[i+1], mesh_indices[i+2]};

            // get triangle
            const Triangle triangle = {positions[idxs[0]], positions[idxs[1]], positions[idxs[2]]};

            // calculate + validate triangle normal
            const auto normal = triangle_normal(triangle).unwrap();
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

    void recalculate_tangents()
    {
        if (topology() != MeshTopology::Triangles) {
            // if the mesh isn't triangle-based, do nothing
            return;
        }
        if (not vertex_buffer_.has_attribute(VertexAttribute::Normal)) {
            // if the mesh doesn't have normals, do nothing
            return;
        }
        if (not vertex_buffer_.has_attribute(VertexAttribute::TexCoord0)) {
            // if the mesh doesn't have texture coordinates, do nothing
            return;
        }
        if (indices_data_.empty()) {
            // if the mesh has no indices, do nothing
            return;
        }

        // ensure the vertex buffer has space for tangents
        vertex_buffer_.emplace_attribute_descriptor({ VertexAttribute::Tangent, VertexAttributeFormat::Float32x3 });

        // calculate tangents

        const auto vbverts = vertex_buffer_.iter<Vec3>(VertexAttribute::Position);
        const auto vbnormals = vertex_buffer_.iter<Vec3>(VertexAttribute::Normal);
        const auto vbtexcoords = vertex_buffer_.iter<Vec2>(VertexAttribute::TexCoord0);

        const auto tangents = calc_tangent_vectors(
            MeshTopology::Triangles,
            std::vector<Vec3>(vbverts.begin(), vbverts.end()),
            std::vector<Vec3>(vbnormals.begin(), vbnormals.end()),
            std::vector<Vec2>(vbtexcoords.begin(), vbtexcoords.end()),
            indices()
        );

        vertex_buffer_.write<Vec4>(VertexAttribute::Tangent, tangents);
    }

    // non-PIMPL methods

    gl::VertexArray& upd_vertex_array()
    {
        if (not *maybe_gpu_data_ or (*maybe_gpu_data_)->data_version != *version_) {
            upload_to_gpu();
        }
        return (*maybe_gpu_data_)->vao;
    }

    void draw_instanced(
        size_t n,
        MaybeIndex maybe_submesh_index)
    {
        const SubMeshDescriptor descriptor = maybe_submesh_index ?
            submesh_descriptors_.at(*maybe_submesh_index) :         // draw the requested sub-mesh
            SubMeshDescriptor{0, num_indices_, topology_};          // else: draw the entire mesh as a "sub mesh"

        // convert mesh/descriptor data types into OpenGL-compatible formats
        const GLenum mode = to_opengl_topology_enum(descriptor.topology());
        const auto num_indices = static_cast<GLsizei>(descriptor.index_count());
        const GLenum type = indices_are_32bit_ ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;

        const size_t num_bytes_per_index = indices_are_32bit_ ? sizeof(GLint) : sizeof(GLshort);
        const size_t first_index_byte_offset = descriptor.index_start() * num_bytes_per_index;

        const auto num_instances = static_cast<GLsizei>(n);

#ifdef EMSCRIPTEN
        glDrawElementsInstanced(
            mode,
            num_indices,
            type,
            cpp20::bit_cast<void*>(first_index_byte_offset),
            num_instances
        );
#else
        const auto base_vertex = static_cast<GLint>(descriptor.base_vertex());
        glDrawElementsInstancedBaseVertex(
            mode,
            num_indices,
            type,
            cpp20::bit_cast<void*>(first_index_byte_offset),
            num_instances,
            base_vertex
        );
#endif
    }

private:

    void set_indices(std::span<const uint16_t> indices, MeshUpdateFlags flags)
    {
        indices_are_32bit_ = false;
        num_indices_ = indices.size();

        if (not indices.empty()) {
            indices_data_.resize((indices.size()+1)/2);
            rgs::copy(indices, &indices_data_.front().u16.a);
        }
        else {
            indices_data_.clear();
        }

        range_check_indices_and_recalculate_bounds(flags);
        version_->reset();
    }

    void set_indices(std::span<const uint32_t> indices, MeshUpdateFlags flags)
    {
        if (indices.empty()) {
            indices_are_32bit_ = false;
            num_indices_ = 0;
            indices_data_.clear();
        }
        else if (rgs::any_of(indices, [](uint32_t idx) { return idx > std::numeric_limits<uint16_t>::max(); })) {
            indices_are_32bit_ = true;
            num_indices_ = indices.size();
            indices_data_.resize(indices.size());
            rgs::copy(indices, &indices_data_.front().u32);
        }
        else {
            indices_are_32bit_ = false;
            num_indices_ = indices.size();
            indices_data_.resize((indices.size()+1)/2);
            rgs::transform(indices, &indices_data_.front().u16.a, [](uint32_t v) { return static_cast<uint16_t>(v); });
        }

        range_check_indices_and_recalculate_bounds(flags);
        version_->reset();
    }

    void range_check_indices_and_recalculate_bounds(
        MeshUpdateFlags flags = MeshUpdateFlag::Default)
    {
        // note: recalculating bounds will always validate indices anyway, because it's assumed
        //       that the caller's intention is that all indices are valid when computing the
        //       bounds
        const bool should_check_indices = not ((flags & MeshUpdateFlag::DontValidateIndices) and (flags & MeshUpdateFlag::DontRecalculateBounds));

        //       ... but it's perfectly reasonable for the caller to only want the indices to be
        //       validated, leaving the bounds untouched
        const bool should_recalculate_bounds = not (flags & MeshUpdateFlag::DontRecalculateBounds);

        if (should_check_indices and should_recalculate_bounds) {
            if (num_indices_ == 0) {
                aabb_ = {};
                return;
            }

            // recalculate bounds while also checking indices
            aabb_.min = {
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
            };
            aabb_.max = {
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
            };

            auto vertices = vertex_buffer_.iter<Vec3>(VertexAttribute::Position);
            for (auto index : indices()) {
                Vec3 pos = vertices.at(index);  // bounds-check index
                aabb_.min = elementwise_min(aabb_.min, pos);
                aabb_.max = elementwise_max(aabb_.max, pos);
            }
        }
        else if (should_check_indices and not should_recalculate_bounds) {
            for (auto mesh_index : indices()) {
                OSC_ASSERT(mesh_index < vertex_buffer_.num_vertices() && "a mesh index is out of bounds");
            }
        }
        else {
            return;  // do nothing
        }
    }

    static GLuint shader_location_of(VertexAttribute vertex_attribute)
    {
        auto constexpr lut = []<VertexAttribute... Attrs>(OptionList<VertexAttribute, Attrs...>)
        {
            return std::to_array({ VertexAttributeTraits<Attrs>::shader_location... });
        }(VertexAttributeList{});

        return lut.at(to_index(vertex_attribute));
    }

    static GLenum to_opengl_attribute_type_enum(const VertexAttributeFormat& format)
    {
        static_assert(num_options<VertexAttributeFormat>() == 5);

        switch (format) {
        case VertexAttributeFormat::Float32x2: return GL_FLOAT;
        case VertexAttributeFormat::Float32x3: return GL_FLOAT;
        case VertexAttributeFormat::Float32x4: return GL_FLOAT;
        case VertexAttributeFormat::Unorm8x4:  return GL_UNSIGNED_BYTE;
        case VertexAttributeFormat::Snorm8x4:  return GL_BYTE;
        default:                               return GL_FLOAT;  // should never happen (static_assert)
        }
    }

    static GLboolean is_normalized_attribute_type(const VertexAttributeFormat& format)
    {
        static_assert(num_options<VertexAttributeFormat>() == 5);

        switch (format) {
        case VertexAttributeFormat::Float32x2: return GL_FALSE;
        case VertexAttributeFormat::Float32x3: return GL_FALSE;
        case VertexAttributeFormat::Float32x4: return GL_FALSE;
        case VertexAttributeFormat::Unorm8x4:  return GL_TRUE;
        case VertexAttributeFormat::Snorm8x4:  return GL_TRUE;
        default:                               return GL_FALSE;  // should never happen (static_assert)
        }
    }

    static void opengl_bind_vertex_attribute(
        const VertexFormat& format,
        const VertexFormat::VertexAttributeLayout& layout)
    {
        glVertexAttribPointer(
            shader_location_of(layout.attribute()),
            static_cast<GLint>(detail::num_components_in(layout.format())),
            to_opengl_attribute_type_enum(layout.format()),
            is_normalized_attribute_type(layout.format()),
            static_cast<GLsizei>(format.stride()),
            cpp20::bit_cast<void*>(layout.offset())
        );
        glEnableVertexAttribArray(shader_location_of(layout.attribute()));
    }

    void upload_to_gpu()
    {
        // allocate GPU-side buffers (or re-use the last ones)
        if (not *maybe_gpu_data_) {
            *maybe_gpu_data_ = MeshOpenGLData{};
        }
        MeshOpenGLData& buffers = **maybe_gpu_data_;

        // upload CPU-side vector data into the GPU-side buffer
        OSC_ASSERT(cpp20::bit_cast<uintptr_t>(vertex_buffer_.bytes().data()) % alignof(float) == 0);
        gl::bind_buffer(GL_ARRAY_BUFFER, buffers.array_buffer);
        gl::buffer_data(
            GL_ARRAY_BUFFER,
            static_cast<GLsizei>(vertex_buffer_.bytes().size()),
            vertex_buffer_.bytes().data(),
            GL_STATIC_DRAW
        );

        // upload CPU-side element data into the GPU-side buffer
        const size_t num_ebo_bytes = num_indices_ * (indices_are_32bit_ ? sizeof(uint32_t) : sizeof(uint16_t));
        gl::bind_buffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indices_buffer);
        gl::buffer_data(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizei>(num_ebo_bytes),
            indices_data_.data(),
            GL_STATIC_DRAW
        );

        // configure mesh-level VAO
        gl::bind_vertex_array(buffers.vao);
        gl::bind_buffer(GL_ARRAY_BUFFER, buffers.array_buffer);
        gl::bind_buffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indices_buffer);
        for (auto&& layout : vertex_buffer_.attribute_layouts()) {
            opengl_bind_vertex_attribute(vertex_buffer_.format(), layout);
        }
        gl::bind_vertex_array();

        buffers.data_version = *version_;
    }

    DefaultConstructOnCopy<UID> version_;
    MeshTopology topology_ = MeshTopology::Default;
    VertexBuffer vertex_buffer_;

    bool indices_are_32bit_ = false;
    size_t num_indices_ = 0;
    std::vector<PackedIndex> indices_data_;

    AABB aabb_ = {};

    std::vector<SubMeshDescriptor> submesh_descriptors_;

    DefaultConstructOnCopy<std::optional<MeshOpenGLData>> maybe_gpu_data_;
};

std::ostream& osc::operator<<(std::ostream& o, MeshTopology topology)
{
    return o << c_mesh_topology_strings.at(to_index(topology));
}

osc::Mesh::Mesh() :
    impl_{make_cow<Impl>()}
{}

MeshTopology osc::Mesh::topology() const
{
    return impl_->topology();
}

void osc::Mesh::set_topology(MeshTopology topology)
{
    impl_.upd()->set_topology(topology);
}

size_t osc::Mesh::num_vertices() const
{
    return impl_->num_vertices();
}

bool osc::Mesh::has_vertices() const
{
    return impl_->has_vertices();
}

std::vector<Vec3> osc::Mesh::vertices() const
{
    return impl_->vertices();
}

void osc::Mesh::set_vertices(std::span<const Vec3> vertices)
{
    impl_.upd()->set_vertices(vertices);
}

void osc::Mesh::transform_vertices(const std::function<Vec3(Vec3)>& transformer)
{
    impl_.upd()->transform_vertices(transformer);
}

void osc::Mesh::transform_vertices(const Transform& transform)
{
    impl_.upd()->transform_vertices(transform);
}

void osc::Mesh::transform_vertices(const Mat4& mat4)
{
    impl_.upd()->transform_vertices(mat4);
}

bool osc::Mesh::has_normals() const
{
    return impl_->has_normals();
}

std::vector<Vec3> osc::Mesh::normals() const
{
    return impl_->normals();
}

void osc::Mesh::set_normals(std::span<const Vec3> normals)
{
    impl_.upd()->set_normals(normals);
}

void osc::Mesh::transform_normals(const std::function<Vec3(Vec3)>& transformer)
{
    impl_.upd()->transform_normals(transformer);
}

bool osc::Mesh::has_tex_coords() const
{
    return impl_->has_tex_coords();
}

std::vector<Vec2> osc::Mesh::tex_coords() const
{
    return impl_->tex_coords();
}

void osc::Mesh::set_tex_coords(std::span<const Vec2> tex_coords)
{
    impl_.upd()->set_tex_coords(tex_coords);
}

void osc::Mesh::transform_tex_coords(const std::function<Vec2(Vec2)>& transformer)
{
    impl_.upd()->transform_tex_coords(transformer);
}

std::vector<Color> osc::Mesh::colors() const
{
    return impl_->colors();
}

void osc::Mesh::set_colors(std::span<const Color> colors)
{
    impl_.upd()->set_colors(colors);
}

std::vector<Vec4> osc::Mesh::tangents() const
{
    return impl_->tangents();
}

void osc::Mesh::set_tangents(std::span<const Vec4> tangents)
{
    impl_.upd()->set_tangents(tangents);
}

size_t osc::Mesh::num_indices() const
{
    return impl_->num_indices();
}

MeshIndicesView osc::Mesh::indices() const
{
    return impl_->indices();
}

void osc::Mesh::set_indices(MeshIndicesView indices, MeshUpdateFlags flags)
{
    impl_.upd()->set_indices(indices, flags);
}

void osc::Mesh::for_each_indexed_vertex(const std::function<void(Vec3)>& callback) const
{
    impl_->for_each_indexed_vertex(callback);
}

void osc::Mesh::for_each_indexed_triangle(const std::function<void(Triangle)>& callback) const
{
    impl_->for_each_indexed_triangle(callback);
}

Triangle osc::Mesh::get_triangle_at(size_t first_index_offset) const
{
    return impl_->get_triangle_at(first_index_offset);
}

std::vector<Vec3> osc::Mesh::indexed_vertices() const
{
    return impl_->indexed_vertices();
}

const AABB& osc::Mesh::bounds() const
{
    return impl_->bounds();
}

void osc::Mesh::clear()
{
    impl_.upd()->clear();
}

size_t osc::Mesh::num_submesh_descriptors() const
{
    return impl_->num_submesh_descriptors();
}

void osc::Mesh::push_submesh_descriptor(const SubMeshDescriptor& descriptor)
{
    impl_.upd()->push_submesh_descriptor(descriptor);
}

const SubMeshDescriptor& osc::Mesh::submesh_descriptor_at(size_t pos) const
{
    return impl_->submesh_descriptor_at(pos);
}

void osc::Mesh::clear_submesh_descriptors()
{
    impl_.upd()->clear_submesh_descriptors();
}

size_t osc::Mesh::num_vertex_attributes() const
{
    return impl_->num_vertex_attributes();
}

const VertexFormat& osc::Mesh::vertex_format() const
{
    return impl_->vertex_format();
}

void osc::Mesh::set_vertex_buffer_params(size_t n, const VertexFormat& format)
{
    impl_.upd()->set_vertex_buffer_params(n, format);
}

size_t osc::Mesh::vertex_buffer_stride() const
{
    return impl_->vertex_buffer_stride();
}

void osc::Mesh::set_vertex_buffer_data(std::span<const uint8_t> data, MeshUpdateFlags flags)
{
    impl_.upd()->set_vertex_buffer_data(data, flags);
}

void osc::Mesh::recalculate_normals()
{
    impl_.upd()->recalculate_normals();
}

void osc::Mesh::recalculate_tangents()
{
    impl_.upd()->recalculate_tangents();
}

std::ostream& osc::operator<<(std::ostream& o, const Mesh&)
{
    return o << "Mesh()";
}

namespace
{
    // LUT for human-readable form of the above
    constexpr auto c_camera_projection_strings = std::to_array<CStringView>(
    {
        "Perspective",
        "Orthographic",
    });
    static_assert(c_camera_projection_strings.size() == num_options<CameraProjection>());
}

class osc::Camera::Impl final {
public:

    void reset()
    {
        // keep the render queue memory allocation
        auto tmp_queue{std::move(render_queue_)};
        tmp_queue.clear();

        *this = Impl{};
        this->render_queue_ = std::move(tmp_queue);
    }

    Color background_color() const
    {
        return background_color_;
    }

    void set_background_color(const Color& color)
    {
        background_color_ = color;
    }

    CameraProjection projection() const
    {
        return camera_projection_;
    }

    void set_projection(CameraProjection projection)
    {
        camera_projection_ = projection;
    }

    float orthographic_size() const
    {
        return orthographic_size_;
    }

    void set_orthographic_size(float size)
    {
        orthographic_size_ = size;
    }

    Radians vertical_fov() const
    {
        return perspective_fov_;
    }

    void set_vertical_fov(Radians size)
    {
        perspective_fov_ = size;
    }

    CameraClippingPlanes clipping_planes() const
    {
        return clipping_planes_;
    }

    void set_clipping_planes(CameraClippingPlanes planes)
    {
        clipping_planes_ = planes;
    }

    float near_clipping_plane() const
    {
        return clipping_planes_.znear;
    }

    void set_near_clipping_plane(float distance)
    {
        clipping_planes_.znear = distance;
    }

    float far_clipping_plane() const
    {
        return clipping_planes_.zfar;
    }

    void set_far_clipping_plane(float distance)
    {
        clipping_planes_.zfar = distance;
    }

    CameraClearFlags clear_flags() const
    {
        return clear_flags_;
    }

    void set_clear_flags(CameraClearFlags flags)
    {
        clear_flags_ = flags;
    }

    std::optional<Rect> pixel_rect() const
    {
        return maybe_screen_pixel_rect_;
    }

    void set_pixel_rect(std::optional<Rect> maybe_pixel_rect)
    {
        maybe_screen_pixel_rect_ = maybe_pixel_rect;
    }

    std::optional<Rect> scissor_rect() const
    {
        return maybe_scissor_rect_;
    }

    void set_scissor_rect(std::optional<Rect> maybe_scissor_rect)
    {
        maybe_scissor_rect_ = maybe_scissor_rect;
    }

    Vec3 position() const
    {
        return position_;
    }

    void set_position(const Vec3& position)
    {
        position_ = position;
    }

    Quat rotation() const
    {
        return rotation_;
    }

    void set_rotation(const Quat& rotation)
    {
        rotation_ = rotation;
    }

    Vec3 direction() const
    {
        return rotation_ * Vec3{0.0f, 0.0f, -1.0f};
    }

    void set_direction(const Vec3& direction)
    {
        rotation_ = osc::rotation(Vec3{0.0f, 0.0f, -1.0f}, direction);
    }

    Vec3 upwards_direction() const
    {
        return rotation_ * Vec3{0.0f, 1.0f, 0.0f};
    }

    Mat4 view_matrix() const
    {
        if (maybe_view_matrix_override_) {
            return *maybe_view_matrix_override_;
        }
        else {
            return look_at(position_, position_ + direction(), upwards_direction());
        }
    }

    std::optional<Mat4> view_matrix_override() const
    {
        return maybe_view_matrix_override_;
    }

    void set_view_matrix_override(std::optional<Mat4> maybe_view_matrix_override)
    {
        maybe_view_matrix_override_ = maybe_view_matrix_override;
    }

    Mat4 projection_matrix(float aspect_ratio) const
    {
        if (maybe_projection_matrix_override_) {
            return *maybe_projection_matrix_override_;
        }
        else if (camera_projection_ == CameraProjection::Perspective) {
            return perspective(
                perspective_fov_,
                aspect_ratio,
                clipping_planes_.znear,
                clipping_planes_.zfar
            );
        }
        else
        {
            const float height = orthographic_size_;
            const float width = height * aspect_ratio;

            const float right = 0.5f * width;
            const float left = -right;
            const float top = 0.5f * height;
            const float bottom = -top;

            return ortho(left, right, bottom, top, clipping_planes_.znear, clipping_planes_.zfar);
        }
    }

    std::optional<Mat4> projection_matrix_override() const
    {
        return maybe_projection_matrix_override_;
    }

    void set_projection_matrix_override(std::optional<Mat4> maybe_projection_matrix_override)
    {
        maybe_projection_matrix_override_ = maybe_projection_matrix_override;
    }

    Mat4 view_projection_matrix(float aspect_ratio) const
    {
        return projection_matrix(aspect_ratio) * view_matrix();
    }

    Mat4 inverse_view_projection_matrix(float aspect_ratio) const
    {
        return inverse(view_projection_matrix(aspect_ratio));
    }

    void render_to_screen()
    {
        GraphicsBackend::render_camera_queue(*this);
    }

    void render_to(RenderTexture& render_texture)
    {
        static_assert(CameraClearFlag::All == CameraClearFlags{CameraClearFlag::SolidColor, CameraClearFlag::Depth});

        const RenderTarget render_target
        {
            {
                RenderTargetColorAttachment
                {
                    // attach to render texture's color buffer
                    render_texture.upd_color_buffer(),

                    // load the color buffer based on this camera's clear flags
                    clear_flags() & CameraClearFlag::SolidColor ?
                        RenderBufferLoadAction::Clear :
                        RenderBufferLoadAction::Load,

                    RenderBufferStoreAction::Resolve,

                    // ensure clear color matches colorspace of render texture
                    is_srgb_encoded(render_texture.color_format()) ?
                        to_linear_colorspace(background_color()) :
                        background_color(),
                },
            },
            RenderTargetDepthStencilAttachment
            {
                // attach to the render texture's depth buffer
                render_texture.upd_depth_buffer(),

                // load the depth buffer based on this camera's clear flags
                clear_flags() & CameraClearFlag::Depth ?
                    RenderBufferLoadAction::Clear :
                    RenderBufferLoadAction::Load,

                RenderBufferStoreAction::DontCare,
            },
        };

        render_to(render_target);
    }

    void render_to(const RenderTarget& render_target)
    {
        GraphicsBackend::render_camera_queue(*this, &render_target);
    }

    friend bool operator==(const Impl&, const Impl&) = default;

private:
    friend class GraphicsBackend;

    Color background_color_ = Color::clear();
    CameraProjection camera_projection_ = CameraProjection::Default;
    float orthographic_size_ = 2.0f;
    Radians perspective_fov_ = 90_deg;
    CameraClippingPlanes clipping_planes_{1.0f, -1.0f};
    CameraClearFlags clear_flags_ = CameraClearFlag::Default;
    std::optional<Rect> maybe_screen_pixel_rect_ = std::nullopt;
    std::optional<Rect> maybe_scissor_rect_ = std::nullopt;
    Vec3 position_ = {};
    Quat rotation_ = identity<Quat>();
    std::optional<Mat4> maybe_view_matrix_override_;
    std::optional<Mat4> maybe_projection_matrix_override_;
    std::vector<RenderObject> render_queue_;
};



std::ostream& osc::operator<<(std::ostream& o, CameraProjection camera_projection)
{
    return o << c_camera_projection_strings.at(to_index(camera_projection));
}

osc::Camera::Camera() :
    impl_{make_cow<Impl>()}
{}

void osc::Camera::reset()
{
    impl_.upd()->reset();
}

Color osc::Camera::background_color() const
{
    return impl_->background_color();
}

void osc::Camera::set_background_color(const Color& color)
{
    impl_.upd()->set_background_color(color);
}

CameraProjection osc::Camera::projection() const
{
    return impl_->projection();
}

void osc::Camera::set_projection(CameraProjection camera_projection)
{
    impl_.upd()->set_projection(camera_projection);
}

float osc::Camera::orthographic_size() const
{
    return impl_->orthographic_size();
}

void osc::Camera::set_orthographic_size(float size)
{
    impl_.upd()->set_orthographic_size(size);
}

Radians osc::Camera::vertical_fov() const
{
    return impl_->vertical_fov();
}

void osc::Camera::set_vertical_fov(Radians vertical_fov)
{
    impl_.upd()->set_vertical_fov(vertical_fov);
}

CameraClippingPlanes osc::Camera::clipping_planes() const
{
    return impl_->clipping_planes();
}

void osc::Camera::set_clipping_planes(CameraClippingPlanes planes)
{
    impl_.upd()->set_clipping_planes(planes);
}

float osc::Camera::near_clipping_plane() const
{
    return impl_->near_clipping_plane();
}

void osc::Camera::set_near_clipping_plane(float near_clipping_plane)
{
    impl_.upd()->set_near_clipping_plane(near_clipping_plane);
}

float osc::Camera::far_clipping_plane() const
{
    return impl_->far_clipping_plane();
}

void osc::Camera::set_far_clipping_plane(float far_clipping_plane)
{
    impl_.upd()->set_far_clipping_plane(far_clipping_plane);
}

CameraClearFlags osc::Camera::clear_flags() const
{
    return impl_->clear_flags();
}

void osc::Camera::set_clear_flags(CameraClearFlags clear_flags)
{
    impl_.upd()->set_clear_flags(clear_flags);
}

std::optional<Rect> osc::Camera::pixel_rect() const
{
    return impl_->pixel_rect();
}

void osc::Camera::set_pixel_rect(std::optional<Rect> maybe_pixel_rect)
{
    impl_.upd()->set_pixel_rect(maybe_pixel_rect);
}

std::optional<Rect> osc::Camera::scissor_rect() const
{
    return impl_->scissor_rect();
}

void osc::Camera::set_scissor_rect(std::optional<Rect> maybe_scissor_rect)
{
    impl_.upd()->set_scissor_rect(maybe_scissor_rect);
}

Vec3 osc::Camera::position() const
{
    return impl_->position();
}

void osc::Camera::set_position(const Vec3& position)
{
    impl_.upd()->set_position(position);
}

Quat osc::Camera::rotation() const
{
    return impl_->rotation();
}

void osc::Camera::set_rotation(const Quat& rotation)
{
    impl_.upd()->set_rotation(rotation);
}

Vec3 osc::Camera::direction() const
{
    return impl_->direction();
}

void osc::Camera::set_direction(const Vec3& direction)
{
    impl_.upd()->set_direction(direction);
}

Vec3 osc::Camera::upwards_direction() const
{
    return impl_->upwards_direction();
}

Mat4 osc::Camera::view_matrix() const
{
    return impl_->view_matrix();
}

std::optional<Mat4> osc::Camera::view_matrix_override() const
{
    return impl_->view_matrix_override();
}

void osc::Camera::set_view_matrix_override(std::optional<Mat4> maybe_view_matrix_override)
{
    impl_.upd()->set_view_matrix_override(maybe_view_matrix_override);
}

Mat4 osc::Camera::projection_matrix(float aspect_ratio) const
{
    return impl_->projection_matrix(aspect_ratio);
}

std::optional<Mat4> osc::Camera::projection_matrix_override() const
{
    return impl_->projection_matrix_override();
}

void osc::Camera::set_projection_matrix_override(std::optional<Mat4> maybe_projection_matrix_override)
{
    impl_.upd()->set_projection_matrix_override(maybe_projection_matrix_override);
}

Mat4 osc::Camera::view_projection_matrix(float aspect_ratio) const
{
    return impl_->view_projection_matrix(aspect_ratio);
}

Mat4 osc::Camera::inverse_view_projection_matrix(float aspect_ratio) const
{
    return impl_->inverse_view_projection_matrix(aspect_ratio);
}

void osc::Camera::render_to_screen()
{
    impl_.upd()->render_to_screen();
}

void osc::Camera::render_to(RenderTexture& render_texture)
{
    impl_.upd()->render_to(render_texture);
}

void osc::Camera::render_to(const RenderTarget& render_target)
{
    impl_.upd()->render_to(render_target);
}

std::ostream& osc::operator<<(std::ostream& o, const Camera& camera)
{
    return o << "Camera(position = " << camera.position() << ", direction = " << camera.direction() << ", projection = " << camera.projection() << ')';
}

bool osc::operator==(const Camera& lhs, const Camera& rhs)
{
    return lhs.impl_ == rhs.impl_ || *lhs.impl_ == *rhs.impl_;
}


namespace
{
    struct RequiredOpenGLCapability final {
        GLenum id;
        CStringView label;
    };
    constexpr auto c_required_opengl_capabilities = std::to_array<RequiredOpenGLCapability>(
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
    sdl::GLContext create_opengl_context(SDL_Window& window)
    {
        log_debug("initializing OpenGL context");

        // create an OpenGL context for the application
        sdl::GLContext ctx = sdl::GL_CreateContext(&window);

        // enable the OpenGL context
        if (SDL_GL_MakeCurrent(&window, ctx.get()) != 0) {
            throw std::runtime_error{std::string{"SDL_GL_MakeCurrent failed: "} + SDL_GetError()};
        }

        // enable vsync by default
        //
        // vsync can feel a little laggy on some systems, but vsync reduces CPU usage
        // on *constrained* systems (e.g. laptops, which the majority of users are using)
        if (SDL_GL_SetSwapInterval(-1) != 0) {
            SDL_GL_SetSwapInterval(1);
        }

        // initialize GLEW
        //
        // effectively, enables the OpenGL API used by this application
        if (const auto err = glewInit(); err != GLEW_OK) {
            std::stringstream ss;
            ss << "glewInit() failed: ";
            ss << glewGetErrorString(err);
            throw std::runtime_error{ss.str()};
        }

        // validate that the runtime OpenGL backend supports the extensions that OSC
        // relies on
        //
        // reports anything missing to the log at the provided log level
        validate_opengl_backend_extension_support(LogLevel::debug);

        // enable required capabilities
        for (const auto& capability : c_required_opengl_capabilities) {
            glEnable(capability.id);
            if (not glIsEnabled(capability.id)) {
                log_warn("failed to enable %s: this may cause rendering issues", capability.label.c_str());
            }
        }

        // ensure alpha blending functions are defaulted
        glBlendFunc(to_opengl_blend_func(SourceBlendingFactor::Default), to_opengl_blend_func(DestinationBlendingFactor::Default));
        glBlendEquation(to_opengl_blend_equation(BlendingEquation::Default));

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
    AntiAliasingLevel get_opengl_max_aa_level(const sdl::GLContext&)
    {
        GLint v = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &v);
        return AntiAliasingLevel{v};
    }

    // maps an OpenGL debug message severity level to a log level
    constexpr LogLevel opengl_debug_severity_to_log_level(GLenum sev)
    {
        switch (sev) {
        case GL_DEBUG_SEVERITY_HIGH:         return LogLevel::err;
        case GL_DEBUG_SEVERITY_MEDIUM:       return LogLevel::warn;
        case GL_DEBUG_SEVERITY_LOW:          return LogLevel::debug;
        case GL_DEBUG_SEVERITY_NOTIFICATION: return LogLevel::trace;
        default:                             return LogLevel::info;
        }
    }

    // returns a string representation of an OpenGL debug message severity level
    constexpr CStringView opengl_debug_severity_to_cstringview(GLenum debug_severity)
    {
        switch (debug_severity) {
        case GL_DEBUG_SEVERITY_HIGH:         return "GL_DEBUG_SEVERITY_HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:       return "GL_DEBUG_SEVERITY_MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:          return "GL_DEBUG_SEVERITY_LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "GL_DEBUG_SEVERITY_NOTIFICATION";
        default:                             return "GL_DEBUG_SEVERITY_UNKNOWN";
        }
    }

    // returns a string representation of an OpenGL debug message source
    constexpr CStringView opengl_debug_source_to_cstringview(GLenum debug_severity)
    {
        switch (debug_severity) {
        case GL_DEBUG_SOURCE_API:             return "GL_DEBUG_SOURCE_API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "GL_DEBUG_SOURCE_SHADER_COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:     return "GL_DEBUG_SOURCE_THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:     return "GL_DEBUG_SOURCE_APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:           return "GL_DEBUG_SOURCE_OTHER";
        default:                              return "GL_DEBUG_SOURCE_UNKNOWN";
        }
    }

    // returns a string representation of an OpenGL debug message type
    constexpr CStringView opengl_debug_type_to_cstringview(GLenum debug_type)
    {
        switch (debug_type) {
        case GL_DEBUG_TYPE_ERROR:               return "GL_DEBUG_TYPE_ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:         return "GL_DEBUG_TYPE_PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:         return "GL_DEBUG_TYPE_PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:              return "GL_DEBUG_TYPE_MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:          return "GL_DEBUG_TYPE_PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:           return "GL_DEBUG_TYPE_POP_GROUP";
        case GL_DEBUG_TYPE_OTHER:               return "GL_DEBUG_TYPE_OTHER";
        default:                                return "GL_DEBUG_TYPE_UNKNOWN";
        }
    }

    // returns `true` if current OpenGL context is in debug mode
    bool is_opengl_in_debug_mode()
    {
        // if context is not debug-mode, then some of the glGet*s below can fail
        // (e.g. GL_DEBUG_OUTPUT_SYNCHRONOUS on apple).
        {
            GLint context_flags = 0;
            glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
            if (not (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
                return false;
            }
        }

        {
            GLboolean debug_output = GL_FALSE;
            glGetBooleanv(GL_DEBUG_OUTPUT, &debug_output);
            if (not debug_output) {
                return false;
            }
        }

        {
            GLboolean debug_output_synchronous = GL_FALSE;
            glGetBooleanv(GL_DEBUG_OUTPUT_SYNCHRONOUS, &debug_output_synchronous);
            if (not debug_output_synchronous) {
                return false;
            }
        }

        return true;
    }

    // raw handler function that can be used with `glDebugMessageCallback`
    void opengl_debug_message_handler(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei,
        const GLchar* message,
        const void*)
    {
        const LogLevel lvl = opengl_debug_severity_to_log_level(severity);
        const CStringView source_cstr = opengl_debug_source_to_cstringview(source);
        const CStringView type_cstr = opengl_debug_type_to_cstringview(type);
        const CStringView severity_cstr = opengl_debug_severity_to_cstringview(severity);

        log_message(lvl,
            R"(OpenGL Debug message:
id = %u
message = %s
source = %s
type = %s
severity = %s
)", id, message, source_cstr.c_str(), type_cstr.c_str(), severity_cstr.c_str());
    }

    // enables OpenGL API debugging
    void enable_opengl_debug_messages()
    {
        if (is_opengl_in_debug_mode()) {
            log_info("OpenGL debug mode appears to already be enabled: skipping enabling it");
            return;
        }

        GLint context_flags{};
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
        if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(opengl_debug_message_handler, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            log_info("enabled OpenGL debug mode");
        }
        else {
            log_error("cannot enable OpenGL debug mode: the context does not have GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }

    // disable OpenGL API debugging
    void disable_opengl_debug_messages()
    {
        if (not is_opengl_in_debug_mode()) {
            log_info("OpenGL debug mode appears to already be disabled: skipping disabling it");
            return;
        }

        GLint context_flags{};
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
        if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glDisable(GL_DEBUG_OUTPUT);
            log_info("disabled OpenGL debug mode");
        }
        else {
            log_error("cannot disable OpenGL debug mode: the context does not have a GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }
}

class osc::GraphicsContext::Impl final {
public:
    explicit Impl(SDL_Window& window) :
        opengl_context_{create_opengl_context(window)}
    {
        quad_material_.set_depth_tested(false);  // it's for fullscreen rendering
    }

    AntiAliasingLevel max_antialiasing_level() const
    {
        return max_aa_level_;
    }

    bool is_vsync_enabled() const
    {
        return vsync_enabled_;
    }

    void set_vsync_enabled(bool v)
    {
        if (v) {
            // try to enable vsync

            if (SDL_GL_SetSwapInterval(-1) == 0) {
                // adaptive vsync enabled
            }
            else if (SDL_GL_SetSwapInterval(1) == 0) {
                // normal vsync enabled
            }

            // always read the vsync state back from SDL
            vsync_enabled_ = SDL_GL_GetSwapInterval() != 0;
        }
        else {
            // try to disable vsync

            SDL_GL_SetSwapInterval(0);
            vsync_enabled_ = SDL_GL_GetSwapInterval() != 0;
        }
    }

    bool is_in_debug_mode() const
    {
        return debug_mode_enabled_;
    }

    void set_debug_mode(bool v)
    {
        if (v) {
            // enable debug mode

            if (is_opengl_in_debug_mode()) {
                return;  // already in debug mode
            }

            log_info("enabling debug mode");
            enable_opengl_debug_messages();
            debug_mode_enabled_ = is_opengl_in_debug_mode();
        }
        else {
            // disable debug mode

            if (not is_opengl_in_debug_mode()) {
                return;  // already not in debug mode
            }

            log_info("disabling debug mode");
            disable_opengl_debug_messages();
            debug_mode_enabled_ = is_opengl_in_debug_mode();
        }
    }

    void clear_screen(const Color& color)
    {
        // clear color is in sRGB, but the framebuffer is sRGB-corrected (GL_FRAMEBUFFER_SRGB)
        // and assumes that the given colors are in linear space
        const Color linear_color = to_linear_colorspace(color);

        gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, gl::window_framebuffer);
        gl::clear_color(linear_color.r, linear_color.g, linear_color.b, linear_color.a);
        gl::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    std::future<Texture2D> request_screenshot()
    {
        return screenshot_request_queue_.emplace_back().get_future();
    }

    void swap_buffers(SDL_Window& window)
    {
        // ensure window FBO is bound (see: SDL_GL_SwapWindow's note about MacOS requiring 0 is bound)
        gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);

        // flush outstanding screenshot requests
        if (not screenshot_request_queue_.empty()) {

            // copy GPU-side window framebuffer into response
            const Vec2i dims = App::get().main_window_dimensions();

            std::vector<uint8_t> pixels(static_cast<size_t>(4*dims.x*dims.y));
            OSC_ASSERT(is_aligned_at_least(pixels.data(), 4) && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
            gl::pixel_store_i(GL_PACK_ALIGNMENT, 4);
            glReadPixels(
                0,
                0,
                dims.x,
                dims.y,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels.data()
            );

            Texture2D screenshot{dims, TextureFormat::RGBA32, ColorSpace::sRGB};
            screenshot.set_pixel_data(pixels);

            // pump out responses
            for (auto& request : screenshot_request_queue_) {
                request.set_value(screenshot);
            }
            screenshot_request_queue_.clear();
        }

        SDL_GL_SwapWindow(&window);
    }

    std::string backend_vendor_string() const
    {
        return std::string{opengl_get_cstringview(GL_VENDOR)};
    }

    std::string backend_renderer_string() const
    {
        return std::string{opengl_get_cstringview(GL_RENDERER)};
    }

    std::string backend_version_string() const
    {
        return std::string{opengl_get_cstringview(GL_VERSION)};
    }

    std::string backend_shading_language_version_string() const
    {
        return std::string{opengl_get_cstringview(GL_SHADING_LANGUAGE_VERSION)};
    }

    const Material& quad_material() const
    {
        return quad_material_;
    }

    const Mesh& quad_mesh() const
    {
        return quad_mesh_;
    }

    std::vector<float>& upd_instance_cpu_buffer()
    {
        return instance_cpu_buffer_;
    }

    gl::ArrayBuffer<float, GL_STREAM_DRAW>& upd_instance_gpu_buffer()
    {
        return instance_gpu_buffer_;
    }

private:

    // active OpenGL context for the application
    sdl::GLContext opengl_context_;

    // maximum number of antiAliasingLevel supported by this hardware's OpenGL MSXAA API
    AntiAliasingLevel max_aa_level_ = get_opengl_max_aa_level(opengl_context_);

    bool vsync_enabled_ = SDL_GL_GetSwapInterval() != 0;

    // true if OpenGL's debug mode is enabled
    bool debug_mode_enabled_ = false;

    // a "queue" of active screenshot requests
    std::vector<std::promise<Texture2D>> screenshot_request_queue_;

    // a generic quad rendering material: used for some blitting operations
    Material quad_material_{Shader{
        c_quad_vertex_shader_src,
        c_quad_fragment_shader_src,
    }};

    // a generic quad mesh: two triangles covering NDC @ Z=0
    Mesh quad_mesh_ = PlaneGeometry{{.width = 2.0f, .height = 2.0f}};

    // storage for instance data
    std::vector<float> instance_cpu_buffer_;
    gl::ArrayBuffer<float, GL_STREAM_DRAW> instance_gpu_buffer_;
};

static std::unique_ptr<osc::GraphicsContext::Impl> g_graphics_context_impl = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

osc::GraphicsContext::GraphicsContext(SDL_Window& window)
{
    if (g_graphics_context_impl) {
        throw std::runtime_error{"a graphics context has already been initialized: you cannot initialize a second"};
    }

    g_graphics_context_impl = std::make_unique<GraphicsContext::Impl>(window);
}

osc::GraphicsContext::~GraphicsContext() noexcept
{
    g_graphics_context_impl.reset();
}

AntiAliasingLevel osc::GraphicsContext::max_antialiasing_level() const
{
    return g_graphics_context_impl->max_antialiasing_level();
}

bool osc::GraphicsContext::is_vsync_enabled() const
{
    return g_graphics_context_impl->is_vsync_enabled();
}

void osc::GraphicsContext::set_vsync_enabled(bool v)
{
    return g_graphics_context_impl->set_vsync_enabled(v);
}

bool osc::GraphicsContext::is_in_debug_mode() const
{
    return g_graphics_context_impl->is_in_debug_mode();
}

void osc::GraphicsContext::set_debug_mode(bool v)
{
    g_graphics_context_impl->set_debug_mode(v);
}

void osc::GraphicsContext::clear_screen(const Color& color)
{
    g_graphics_context_impl->clear_screen(color);
}

void osc::GraphicsContext::swap_buffers(SDL_Window& window)
{
    g_graphics_context_impl->swap_buffers(window);
}

std::future<Texture2D> osc::GraphicsContext::request_screenshot()
{
    return g_graphics_context_impl->request_screenshot();
}

std::string osc::GraphicsContext::backend_vendor_string() const
{
    return g_graphics_context_impl->backend_vendor_string();
}

std::string osc::GraphicsContext::backend_renderer_string() const
{
    return g_graphics_context_impl->backend_renderer_string();
}

std::string osc::GraphicsContext::backend_version_string() const
{
    return g_graphics_context_impl->backend_version_string();
}

std::string osc::GraphicsContext::backend_shading_language_version_string() const
{
    return g_graphics_context_impl->backend_shading_language_version_string();
}


void osc::graphics::draw(
    const Mesh& mesh,
    const Transform& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybe_material_property_block,
    std::optional<size_t> maybe_submesh_index)
{
    GraphicsBackend::draw(
        mesh,
        transform,
        material,
        camera,
        maybe_material_property_block,
        maybe_submesh_index
    );
}

void osc::graphics::draw(
    const Mesh& mesh,
    const Mat4& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybe_material_property_block,
    std::optional<size_t> maybe_submesh_index)
{
    GraphicsBackend::draw(
        mesh,
        transform,
        material,
        camera,
        maybe_material_property_block,
        maybe_submesh_index
    );
}

void osc::graphics::blit(const Texture2D& source, RenderTexture& destination)
{
    GraphicsBackend::blit(source, destination);
}

void osc::graphics::blit_to_screen(
    const RenderTexture& render_texture,
    const Rect& rect,
    BlitFlags flags)
{
    GraphicsBackend::blit_to_screen(render_texture, rect, flags);
}

void osc::graphics::blit_to_screen(
    const RenderTexture& render_texture,
    const Rect& rect,
    const Material& material,
    BlitFlags flags)
{
    GraphicsBackend::blit_to_screen(render_texture, rect, material, flags);
}

void osc::graphics::blit_to_screen(
    const Texture2D& texture,
    const Rect& rect)
{
    GraphicsBackend::blit_to_screen(texture, rect);
}

void osc::graphics::copy_texture(
    const RenderTexture& source,
    Texture2D& destination)
{
    GraphicsBackend::copy_texture(source, destination);
}

void osc::graphics::copy_texture(
    const RenderTexture& source,
    Texture2D& destination,
    CubemapFace face)
{
    GraphicsBackend::copy_texture(source, destination, face);
}

void osc::graphics::copy_texture(
    const RenderTexture& source,
    Cubemap& destination,
    size_t mip)
{
    GraphicsBackend::copy_texture(source, destination, mip);
}

// helper: binds to instanced attributes (per-drawcall)
void osc::GraphicsBackend::bind_to_instanced_attributes(
    const Shader::Impl& shader_impl,
    InstancingState& instancing_state)
{
    gl::bind_buffer(instancing_state.buffer);

    size_t byte_offset = 0;
    if (shader_impl.maybe_instanced_model_mat_attr_) {
        if (shader_impl.maybe_instanced_model_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 model_matrix_attr{shader_impl.maybe_instanced_model_mat_attr_->location};
            gl::vertex_attrib_pointer(model_matrix_attr, false, instancing_state.stride, instancing_state.base_offset + byte_offset);
            gl::vertex_attrib_divisor(model_matrix_attr, 1);
            gl::enable_vertex_attrib_array(model_matrix_attr);
            byte_offset += sizeof(float) * 16;
        }
    }
    if (shader_impl.maybe_instanced_normal_mat_attr_) {
        if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 normal_matrix_attr{shader_impl.maybe_instanced_normal_mat_attr_->location};
            gl::vertex_attrib_pointer(normal_matrix_attr, false, instancing_state.stride, instancing_state.base_offset + byte_offset);
            gl::vertex_attrib_divisor(normal_matrix_attr, 1);
            gl::enable_vertex_attrib_array(normal_matrix_attr);
            // unused: byteOffset += sizeof(float) * 16;
        }
        else if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat3) {
            const gl::AttributeMat3 normal_matrix_attr{shader_impl.maybe_instanced_normal_mat_attr_->location};
            gl::vertex_attrib_pointer(normal_matrix_attr, false, instancing_state.stride, instancing_state.base_offset + byte_offset);
            gl::vertex_attrib_divisor(normal_matrix_attr, 1);
            gl::enable_vertex_attrib_array(normal_matrix_attr);
            // unused: byteOffset += sizeof(float) * 9;
        }
    }
}

// helper: unbinds from instanced attributes (per-drawcall)
void osc::GraphicsBackend::unbind_from_instanced_attributes(
    const Shader::Impl& shader_impl,
    InstancingState&)
{
    if (shader_impl.maybe_instanced_model_mat_attr_) {
        if (shader_impl.maybe_instanced_model_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 model_matrix_attr{shader_impl.maybe_instanced_model_mat_attr_->location};
            gl::disable_vertex_attrib_array(model_matrix_attr);
        }
    }
    if (shader_impl.maybe_instanced_normal_mat_attr_) {
        if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 normal_matrix_attr{shader_impl.maybe_instanced_normal_mat_attr_->location};
            gl::disable_vertex_attrib_array(normal_matrix_attr);
        }
        else if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat3) {
            const gl::AttributeMat3 normal_matrix_attr{shader_impl.maybe_instanced_normal_mat_attr_->location};
            gl::disable_vertex_attrib_array(normal_matrix_attr);
        }
    }
}

// helper: upload instancing data for a batch
std::optional<InstancingState> osc::GraphicsBackend::upload_instance_data(
    std::span<const RenderObject> render_queue,
    const Shader::Impl& shader_impl)
{
    // preemptively upload instancing data
    std::optional<InstancingState> maybe_instancing_state;

    if (shader_impl.maybe_instanced_model_mat_attr_ or shader_impl.maybe_instanced_normal_mat_attr_) {

        // compute the stride between each instance
        size_t byte_stride = 0;
        if (shader_impl.maybe_instanced_model_mat_attr_) {
            if (shader_impl.maybe_instanced_model_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
                byte_stride += sizeof(float) * 16;
            }
        }
        if (shader_impl.maybe_instanced_normal_mat_attr_) {
            if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
                byte_stride += sizeof(float) * 16;
            }
            else if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat3) {
                byte_stride += sizeof(float) * 9;
            }
        }

        // write the instance data into a CPU-side buffer

        OSC_PERF("GraphicsBackend::uploadInstanceData");
        std::vector<float>& buf = g_graphics_context_impl->upd_instance_cpu_buffer();
        buf.clear();
        buf.reserve(render_queue.size() * (byte_stride/sizeof(float)));

        size_t float_offset = 0;
        for (const RenderObject& render_object : render_queue) {
            if (shader_impl.maybe_instanced_model_mat_attr_) {
                if (shader_impl.maybe_instanced_model_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
                    const Mat4 m = model_mat4(render_object);
                    const std::span<const float> els = to_float_span(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    float_offset += els.size();
                }
            }
            if (shader_impl.maybe_instanced_normal_mat_attr_) {
                if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat4) {
                    const Mat4 m = normal_matrix4(render_object);
                    const std::span<const float> els = to_float_span(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    float_offset += els.size();
                }
                else if (shader_impl.maybe_instanced_normal_mat_attr_->shader_type == ShaderPropertyType::Mat3) {
                    const Mat3 m = normal_matrix(render_object);
                    const std::span<const float> els = to_float_span(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    float_offset += els.size();
                }
            }
        }
        OSC_ASSERT_ALWAYS(sizeof(float)*float_offset == render_queue.size() * byte_stride);

        auto& vbo = maybe_instancing_state.emplace(g_graphics_context_impl->upd_instance_gpu_buffer(), byte_stride).buffer;
        vbo.assign(std::span<const float>{buf.data(), float_offset});
    }
    return maybe_instancing_state;
}

void osc::GraphicsBackend::try_bind_material_value_to_shader_element(
    const ShaderElement& shader_element,
    const MaterialValue& material_value,
    int32_t& texture_slot)
{
    if (get_shader_type(material_value) != shader_element.shader_type) {
        return;  // mismatched types
    }

    static_assert(std::variant_size_v<MaterialValue> == 18);

    switch (material_value.index()) {
    case variant_index<MaterialValue, Color>():
    {
        // colors are converted from sRGB to linear when passed to
        // the shader

        const Vec4 linear_color = to_linear_colorspace(std::get<Color>(material_value));
        gl::UniformVec4 u{shader_element.location};
        gl::set_uniform(u, linear_color);
        break;
    }
    case variant_index<MaterialValue, std::vector<Color>>():
    {
        const auto& colors = std::get<std::vector<Color>>(material_value);
        const int32_t num_colors_to_assign = min(shader_element.size, static_cast<int32_t>(colors.size()));

        if (num_colors_to_assign > 0)
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

            std::vector<Vec4> linear_colors;
            linear_colors.reserve(num_colors_to_assign);
            for (const auto& color : colors) {
                linear_colors.emplace_back(to_linear_colorspace(color));
            }
            static_assert(sizeof(Vec4) == 4*sizeof(float));
            static_assert(alignof(Vec4) <= alignof(float));
            glUniform4fv(shader_element.location, num_colors_to_assign, value_ptr(linear_colors.front()));
        }
        break;
    }
    case variant_index<MaterialValue, float>():
    {
        gl::UniformFloat u{shader_element.location};
        gl::set_uniform(u, std::get<float>(material_value));
        break;
    }
    case variant_index<MaterialValue, std::vector<float>>():
    {
        const auto& vals = std::get<std::vector<float>>(material_value);
        const int32_t num_to_assign = min(shader_element.size, static_cast<int32_t>(vals.size()));

        if (num_to_assign > 0) {
            // CARE: assigning to uniform arrays should be done in one `glUniform` call
            //
            // although many guides on the internet say it's valid to assign each array
            // element one-at-a-time by just calling the one-element version with `location + i`
            // I (AK) have encountered situations where some backends (e.g. MacOS) will behave
            // unusually if assigning this way
            //
            // so, for safety's sake, always upload arrays in one `glUniform*` call

            glUniform1fv(shader_element.location, num_to_assign, vals.data());
        }
        break;
    }
    case variant_index<MaterialValue, Vec2>():
    {
        gl::UniformVec2 u{shader_element.location};
        gl::set_uniform(u, std::get<Vec2>(material_value));
        break;
    }
    case variant_index<MaterialValue, Vec3>():
    {
        gl::UniformVec3 u{shader_element.location};
        gl::set_uniform(u, std::get<Vec3>(material_value));
        break;
    }
    case variant_index<MaterialValue, std::vector<Vec3>>():
    {
        const auto& vals = std::get<std::vector<Vec3>>(material_value);
        const int32_t num_to_assign = min(shader_element.size, static_cast<int32_t>(vals.size()));

        if (num_to_assign > 0) {
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

            glUniform3fv(shader_element.location, num_to_assign, value_ptr(vals.front()));
        }
        break;
    }
    case variant_index<MaterialValue, Vec4>():
    {
        gl::UniformVec4 u{shader_element.location};
        gl::set_uniform(u, std::get<Vec4>(material_value));
        break;
    }
    case variant_index<MaterialValue, Mat3>():
    {
        gl::UniformMat3 u{shader_element.location};
        gl::set_uniform(u, std::get<Mat3>(material_value));
        break;
    }
    case variant_index<MaterialValue, Mat4>():
    {
        gl::UniformMat4 u{shader_element.location};
        gl::set_uniform(u, std::get<Mat4>(material_value));
        break;
    }
    case variant_index<MaterialValue, std::vector<Mat4>>():
    {
        const auto& vals = std::get<std::vector<Mat4>>(material_value);
        const int32_t num_to_assign = min(shader_element.size, static_cast<int32_t>(vals.size()));
        if (num_to_assign > 0) {
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
            glUniformMatrix4fv(shader_element.location, num_to_assign, GL_FALSE, value_ptr(vals.front()));
        }
        break;
    }
    case variant_index<MaterialValue, int>():
    {
        gl::UniformInt u{shader_element.location};
        gl::set_uniform(u, std::get<int>(material_value));
        break;
    }
    case variant_index<MaterialValue, bool>():
    {
        gl::UniformBool u{shader_element.location};
        gl::set_uniform(u, std::get<bool>(material_value));
        break;
    }
    case variant_index<MaterialValue, Texture2D>():
    {
        auto& texture_impl = const_cast<Texture2D::Impl&>(*std::get<Texture2D>(material_value).impl_);
        gl::Texture2D& texture = texture_impl.updTexture();

        gl::active_texture(GL_TEXTURE0 + texture_slot);
        gl::bind_texture(texture);
        gl::UniformSampler2D u{shader_element.location};
        gl::set_uniform(u, texture_slot);

        ++texture_slot;
        break;
    }
    case variant_index<MaterialValue, RenderTexture>():
    {
        static_assert(num_options<TextureDimensionality>() == 2);
        std::visit(Overload{
            [&texture_slot, &shader_element](SingleSampledTexture& sst)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(sst.texture2D);
                gl::UniformSampler2D u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            [&texture_slot, &shader_element](MultisampledRBOAndResolvedTexture& mst)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(mst.single_sampled_texture2D);
                gl::UniformSampler2D u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            [&texture_slot, &shader_element](SingleSampledCubemap& cubemap)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(cubemap.cubemap);
                gl::UniformSamplerCube u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
        }, const_cast<RenderTexture::Impl&>(*std::get<RenderTexture>(material_value).impl_).get_color_render_buffer_data());

        break;
    }
    case variant_index<MaterialValue, SharedColorRenderBuffer>():
    {
        static_assert(num_options<TextureDimensionality>() == 2);
        std::visit(Overload{
            [&texture_slot, &shader_element](SingleSampledTexture& sst)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(sst.texture2D);
                gl::UniformSampler2D u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            [&texture_slot, &shader_element](MultisampledRBOAndResolvedTexture& mst)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(mst.single_sampled_texture2D);
                gl::UniformSampler2D u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            [&texture_slot, &shader_element](SingleSampledCubemap& cubemap)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(cubemap.cubemap);
                gl::UniformSamplerCube u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            }, const_cast<SharedColorRenderBuffer::ColorRenderBuffer&>(*std::get<SharedColorRenderBuffer>(material_value).impl_).upd_opengl_data());

        break;
    }
    case variant_index<MaterialValue, SharedDepthStencilRenderBuffer>():
    {
        static_assert(num_options<TextureDimensionality>() == 2);
        std::visit(Overload{
            [&texture_slot, &shader_element](SingleSampledTexture& sst)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(sst.texture2D);
                gl::UniformSampler2D u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            [&texture_slot, &shader_element](MultisampledRBOAndResolvedTexture& mst)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(mst.single_sampled_texture2D);
                gl::UniformSampler2D u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            [&texture_slot, &shader_element](SingleSampledCubemap& cubemap)
            {
                gl::active_texture(GL_TEXTURE0 + texture_slot);
                gl::bind_texture(cubemap.cubemap);
                gl::UniformSamplerCube u{shader_element.location};
                gl::set_uniform(u, texture_slot);
                ++texture_slot;
            },
            }, const_cast<SharedDepthStencilRenderBuffer::DepthStencilRenderBuffer&>(*std::get<SharedDepthStencilRenderBuffer>(material_value).impl_).upd_opengl_data());

        break;
    }
    case variant_index<MaterialValue, Cubemap>():
    {
        auto& cubemap_impl = const_cast<Cubemap::Impl&>(*std::get<Cubemap>(material_value).impl_);
        const gl::TextureCubemap& texture = cubemap_impl.upd_cubemap();

        gl::active_texture(GL_TEXTURE0 + texture_slot);
        gl::bind_texture(texture);
        gl::UniformSamplerCube u{shader_element.location};
        gl::set_uniform(u, texture_slot);

        ++texture_slot;
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
void osc::GraphicsBackend::handle_batch_with_same_submesh(
    std::span<const RenderObject> batch,
    std::optional<InstancingState>& instancing_state)
{
    OSC_ASSERT(not batch.empty());
    auto& mesh_impl = const_cast<Mesh::Impl&>(*batch.front().mesh.impl_);
    const Shader::Impl& shader_impl = *batch.front().material.impl_->shader_.impl_;
    const MaybeIndex maybe_submesh_index = batch.front().maybe_submesh_index;

    gl::bind_vertex_array(mesh_impl.upd_vertex_array());

    if (shader_impl.maybe_model_mat_uniform_ or shader_impl.maybe_normal_mat_uniform_) {
        // if the shader requires per-instance uniforms, then we *have* to render one
        // instance at a time

        for (const RenderObject& render_object : batch) {

            // try binding to uModel (standard)
            if (shader_impl.maybe_model_mat_uniform_) {
                if (shader_impl.maybe_model_mat_uniform_->shader_type == ShaderPropertyType::Mat4) {
                    gl::UniformMat4 u{shader_impl.maybe_model_mat_uniform_->location};
                    gl::set_uniform(u, model_mat4(render_object));
                }
            }

            // try binding to uNormalMat (standard)
            if (shader_impl.maybe_normal_mat_uniform_) {
                if (shader_impl.maybe_normal_mat_uniform_->shader_type == ShaderPropertyType::Mat3) {
                    gl::UniformMat3 u{shader_impl.maybe_normal_mat_uniform_->location};
                    gl::set_uniform(u, normal_matrix(render_object));
                }
                else if (shader_impl.maybe_normal_mat_uniform_->shader_type == ShaderPropertyType::Mat4) {
                    gl::UniformMat4 u{shader_impl.maybe_normal_mat_uniform_->location};
                    gl::set_uniform(u, normal_matrix4(render_object));
                }
            }

            if (instancing_state) {
                bind_to_instanced_attributes(shader_impl, *instancing_state);
            }
            mesh_impl.draw_instanced(1, maybe_submesh_index);
            if (instancing_state) {
                unbind_from_instanced_attributes(shader_impl, *instancing_state);
                instancing_state->base_offset += 1 * instancing_state->stride;
            }
        }
    }
    else {
        // else: the shader supports instanced data, so we can draw multiple meshes in one call

        if (instancing_state) {
            bind_to_instanced_attributes(shader_impl, *instancing_state);
        }
        mesh_impl.draw_instanced(batch.size(), maybe_submesh_index);
        if (instancing_state) {
            unbind_from_instanced_attributes(shader_impl, *instancing_state);
            instancing_state->base_offset += batch.size() * instancing_state->stride;
        }
    }

    gl::bind_vertex_array();
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
//   - Mesh
void osc::GraphicsBackend::handle_batch_with_same_mesh(
    std::span<const RenderObject> batch,
    std::optional<InstancingState>& instancing_state)
{
    // batch by sub-Mesh index
    auto subbatch_begin = batch.begin();
    while (subbatch_begin != batch.end()) {
        const auto subbatch_end = find_if_not(subbatch_begin, batch.end(), RenderObjectHasSubMeshIndex{subbatch_begin->maybe_submesh_index});
        handle_batch_with_same_submesh({subbatch_begin, subbatch_end}, instancing_state);
        subbatch_begin = subbatch_end;
    }
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
void osc::GraphicsBackend::handle_batch_with_same_material_property_block(
    std::span<const RenderObject> batch,
    int32_t& texture_slot,
    std::optional<InstancingState>& instancing_state)
{
    OSC_PERF("GraphicsBackend::handle_batch_with_same_material_property_block");
    OSC_ASSERT(not batch.empty());

    const Material::Impl& material_impl = *batch.front().material.impl_;
    const Shader::Impl& shader_impl = *material_impl.shader_.impl_;
    const FastStringHashtable<ShaderElement>& uniforms = shader_impl.uniforms();

    // bind property block variables (if applicable)
    for (const auto& [name, value] : batch.front().property_block.impl_->values_) {
        if (const auto* uniform = lookup_or_nullptr(uniforms, name)) {
            try_bind_material_value_to_shader_element(*uniform, value, texture_slot);
        }
    }

    // batch by mesh
    auto subbatch_begin = batch.begin();
    while (subbatch_begin != batch.end()) {
        const auto subbatch_end = find_if_not(subbatch_begin, batch.end(), RenderObjectHasMesh{&subbatch_begin->mesh});
        handle_batch_with_same_mesh({subbatch_begin, subbatch_end}, instancing_state);
        subbatch_begin = subbatch_end;
    }
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
void osc::GraphicsBackend::handle_batch_with_same_material(
    const RenderPassState& render_pass_state,
    std::span<const RenderObject> batch)
{
    OSC_PERF("GraphicsBackend::handle_batch_with_same_material");
    OSC_ASSERT(not batch.empty());

    const auto& material_impl = *batch.front().material.impl_;
    const auto& shader_impl = *material_impl.shader_.impl_;
    const FastStringHashtable<ShaderElement>& uniforms = shader_impl.uniforms();

    // preemptively upload instance data
    std::optional<InstancingState> maybe_instances = upload_instance_data(batch, shader_impl);

    // updated by various batches (which may bind to textures etc.)
    int32_t texture_slot = 0;

    gl::use_program(shader_impl.program());

    if (material_impl.source_blending_factor() != SourceBlendingFactor::Default or
        material_impl.destination_blending_factor() != DestinationBlendingFactor::Default) {

        glBlendFunc(
            to_opengl_blend_func(material_impl.source_blending_factor()),
            to_opengl_blend_func(material_impl.destination_blending_factor())
        );
    }

    if (material_impl.blending_equation() != BlendingEquation::Default) {
        glBlendEquation(to_opengl_blend_equation(material_impl.blending_equation()));
    }

#ifndef EMSCRIPTEN
    if (material_impl.is_wireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
#endif

    if (material_impl.depth_function() != DepthFunction::Default) {
        glDepthFunc(to_opengl_depth_function_enum(material_impl.depth_function()));
    }

    if (material_impl.cull_mode() != CullMode::Off) {
        glEnable(GL_CULL_FACE);
        glCullFace(to_opengl_cull_face_enum(material_impl.cull_mode()));

        // winding order is assumed to be counter-clockwise
        //
        // (it's the initial value as defined by Khronos: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFrontFace.xhtml)
        // glFrontFace(GL_CCW);
    }

    // bind material variables
    {
        // try binding to uView (standard)
        if (shader_impl.maybe_view_mat_uniform_) {
            if (shader_impl.maybe_view_mat_uniform_->shader_type == ShaderPropertyType::Mat4) {
                gl::UniformMat4 u{shader_impl.maybe_view_mat_uniform_->location};
                gl::set_uniform(u, render_pass_state.view_matrix);
            }
        }

        // try binding to uProjection (standard)
        if (shader_impl.maybe_proj_mat_uniform_) {
            if (shader_impl.maybe_proj_mat_uniform_->shader_type == ShaderPropertyType::Mat4) {
                gl::UniformMat4 u{shader_impl.maybe_proj_mat_uniform_->location};
                gl::set_uniform(u, render_pass_state.projection_matrix);
            }
        }

        if (shader_impl.maybe_view_proj_mat_uniform_) {
            if (shader_impl.maybe_view_proj_mat_uniform_->shader_type == ShaderPropertyType::Mat4) {
                gl::UniformMat4 u{shader_impl.maybe_view_proj_mat_uniform_->location};
                gl::set_uniform(u, render_pass_state.view_projection_matrix);
            }
        }

        // bind material values
        for (const auto& [name, value] : material_impl.properties_.impl_->values_) {
            if (const ShaderElement* e = lookup_or_nullptr(uniforms, name)) {
                try_bind_material_value_to_shader_element(*e, value, texture_slot);
            }
        }
    }

    // batch by material property block
    auto subbatch_begin = batch.begin();
    while (subbatch_begin != batch.end())
    {
        const auto subbatch_end = find_if_not(subbatch_begin, batch.end(), RenderObjectHasMaterialPropertyBlock{subbatch_begin->property_block});
        handle_batch_with_same_material_property_block({subbatch_begin, subbatch_end}, texture_slot, maybe_instances);
        subbatch_begin = subbatch_end;
    }

    if (material_impl.cull_mode() != CullMode::Off) {
        glCullFace(GL_BACK);  // default from Khronos docs
        glDisable(GL_CULL_FACE);
    }

    if (material_impl.depth_function() != DepthFunction::Default) {
        glDepthFunc(to_opengl_depth_function_enum(DepthFunction::Default));
    }

#ifndef EMSCRIPTEN
    if (material_impl.is_wireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
#endif

    if (material_impl.blending_equation() != BlendingEquation::Default) {
        glBlendEquation(to_opengl_blend_equation(BlendingEquation::Default));
    }

    if (material_impl.source_blending_factor() != SourceBlendingFactor::Default or
        material_impl.destination_blending_factor() != DestinationBlendingFactor::Default) {

        glBlendFunc(
            to_opengl_blend_func(SourceBlendingFactor::Default),
            to_opengl_blend_func(DestinationBlendingFactor::Default)
        );
    }
}

// helper: draw a sequence of `RenderObject`s
void osc::GraphicsBackend::draw_render_objects(
    const RenderPassState& render_pass_state,
    std::span<const RenderObject> batch)
{
    OSC_PERF("GraphicsBackend::draw_render_objects");

    // batch by material
    auto subbatch_begin = batch.begin();
    while (subbatch_begin != batch.end()) {
        const auto subbatch_end = find_if_not(subbatch_begin, batch.end(), RenderObjectHasMaterial{&subbatch_begin->material});
        handle_batch_with_same_material(render_pass_state, {subbatch_begin, subbatch_end});
        subbatch_begin = subbatch_end;
    }
}

void osc::GraphicsBackend::draw_batched_by_opaqueness(
    const RenderPassState& render_pass_state,
    std::span<const RenderObject> batch)
{
    OSC_PERF("GraphicsBackend::draw_batched_by_opaqueness");

    auto batch_iterator = batch.begin();
    while (batch_iterator != batch.end()) {
        const auto opaque_end = find_if_not(batch_iterator, batch.end(), is_opaque);

        if (opaque_end != batch_iterator) {
            // [batchIt..opaqueEnd] contains opaque elements
            gl::disable(GL_BLEND);
            draw_render_objects(render_pass_state, {batch_iterator, opaque_end});

            batch_iterator = opaque_end;
        }

        if (opaque_end != batch.end()) {
            // [opaqueEnd..els.end()] contains transparent elements
            const auto transparent_end = find_if(opaque_end, batch.end(), is_opaque);
            gl::enable(GL_BLEND);
            draw_render_objects(render_pass_state, {opaque_end, transparent_end});

            batch_iterator = transparent_end;
        }
    }
}

void osc::GraphicsBackend::flush_render_queue(Camera::Impl& camera, float aspect_ratio)
{
    OSC_PERF("GraphicsBackend::flush_render_queue");

    // flush the render queue in batches based on what's being rendered:
    //
    // - not-depth-tested elements (can't be reordered)
    // - depth-tested elements (can be reordered):
    //   - opaqueness (opaque first, then transparent back-to-front)
    //   - material
    //   - material property block
    //   - mesh

    std::vector<RenderObject>& queue = camera.render_queue_;

    if (queue.empty()) {
        return;
    }

    // precompute any render pass state used by the rendering algs
    const RenderPassState renderPassState{
        camera.position(),
        camera.view_matrix(),
        camera.projection_matrix(aspect_ratio),
    };

    gl::enable(GL_DEPTH_TEST);

    // draw by reordering depth-tested elements around the not-depth-tested elements
    auto batch_iterator = queue.begin();
    while (batch_iterator != queue.end()) {
        const auto depth_tested_end = find_if_not(batch_iterator, queue.end(), is_depth_tested);

        if (depth_tested_end != batch_iterator) {
            // there are >0 depth-tested elements that are elegible for reordering

            sort_render_queue(batch_iterator, depth_tested_end, renderPassState.camera_pos);
            draw_batched_by_opaqueness(renderPassState, {batch_iterator, depth_tested_end});

            batch_iterator = depth_tested_end;
        }

        if (depth_tested_end != queue.end()) {
            // there are >0 not-depth-tested elements that cannot be reordered

            const auto ignore_depth_test_end = find_if(depth_tested_end, queue.end(), is_depth_tested);

            // these elements aren't depth-tested and should just be drawn as-is
            gl::disable(GL_DEPTH_TEST);
            draw_batched_by_opaqueness(renderPassState, {depth_tested_end, ignore_depth_test_end});
            gl::enable(GL_DEPTH_TEST);

            batch_iterator = ignore_depth_test_end;
        }
    }

    // queue flushed: clear it
    queue.clear();
}

osc::GraphicsBackend::ViewportGeometry osc::GraphicsBackend::calc_viewport_geometry(
    Camera::Impl& camera,
    const RenderTarget* maybe_custom_render_target)
{
    if (auto pixel_rect = camera.pixel_rect()) {
        return {pixel_rect->p1, dimensions_of(*pixel_rect)};
    }
    else if (maybe_custom_render_target) {
        return {{}, maybe_custom_render_target->dimensions()};
    }
    else {
        return {{}, App::get().main_window_dimensions()};
    }
}

float osc::GraphicsBackend::setup_top_level_pipeline_state(
    Camera::Impl& camera,
    const RenderTarget* maybe_custom_render_target)
{
    const auto viewport_geom = calc_viewport_geometry(camera, maybe_custom_render_target);

    gl::viewport(
        static_cast<GLsizei>(viewport_geom.bottom_left.x),
        static_cast<GLsizei>(viewport_geom.bottom_left.y),
        static_cast<GLsizei>(viewport_geom.dimensions.x),
        static_cast<GLsizei>(viewport_geom.dimensions.y)
    );

    if (camera.maybe_scissor_rect_) {
        const Rect scissor_rect = *camera.maybe_scissor_rect_;
        const Vec2i scissor_dimensions = dimensions_of(scissor_rect);

        gl::enable(GL_SCISSOR_TEST);
        glScissor(
            static_cast<GLint>(scissor_rect.p1.x),
            static_cast<GLint>(scissor_rect.p1.y),
            scissor_dimensions.x,
            scissor_dimensions.y
        );
    }
    else {
        gl::disable(GL_SCISSOR_TEST);
    }

    return aspect_ratio_of(viewport_geom.dimensions);
}

void osc::GraphicsBackend::teardown_top_level_pipeline_state(
    Camera::Impl& camera,
    const RenderTarget*)
{
    if (camera.maybe_scissor_rect_) {
        gl::disable(GL_SCISSOR_TEST);
    }
    gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);
    gl::use_program();
}

std::optional<gl::FrameBuffer> osc::GraphicsBackend::bind_and_clear_render_buffers(
    Camera::Impl& camera,
    const RenderTarget* maybe_custom_render_target)
{
    // if necessary, create pass-specific FBO
    std::optional<gl::FrameBuffer> maybe_render_fbo;

    if (maybe_custom_render_target) {
        // caller wants to render to a custom render target of `n` color
        // buffers and a single depth buffer. Bind them all to one MRT FBO

        gl::FrameBuffer& render_fbo = maybe_render_fbo.emplace();
        gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, render_fbo);

        // attach color buffers to the FBO
        for (size_t i = 0; i < maybe_custom_render_target->color_attachments().size(); ++i) {
            std::visit(Overload
            {
                [i](SingleSampledTexture& t)
                {
                    gl::framebuffer_texture2D(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                        t.texture2D,
                        0
                    );
                },
                [i](MultisampledRBOAndResolvedTexture& t)
                {
                    gl::framebuffer_renderbuffer(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                        t.multisampled_rbo
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
                        t.cubemap.get(),
                        0
                    );
                }
#endif
            }, maybe_custom_render_target->color_attachments()[i].buffer.impl_->upd_opengl_data());
        }

        // attach depth (+stencil) buffer to the FBO
        if (auto const& depth_attachment = maybe_custom_render_target->depth_attachment()) {
            const GLenum attachment = detail::has_stencil_component(depth_attachment->buffer.format()) ?
                GL_DEPTH_STENCIL_ATTACHMENT :
                GL_DEPTH_ATTACHMENT;

            std::visit(Overload
            {
                [attachment](SingleSampledTexture& t)
                {
                    gl::framebuffer_texture2D(
                        GL_DRAW_FRAMEBUFFER,
                        attachment,
                        t.texture2D,
                        0
                    );
                },
                [attachment](MultisampledRBOAndResolvedTexture& t)
                {
                    gl::framebuffer_renderbuffer(
                        GL_DRAW_FRAMEBUFFER,
                        attachment,
                        t.multisampled_rbo
                    );
                },
    #ifdef EMSCRIPTEN
                [](SingleSampledCubemap&) {}
    #else
                [attachment](SingleSampledCubemap& t)
                {
                    glFramebufferTexture(
                        GL_DRAW_FRAMEBUFFER,
                        attachment,
                        t.cubemap.get(),
                        0
                    );
                }
    #endif
            }, depth_attachment->buffer.impl_->upd_opengl_data());
        }

        // Multi-Render Target (MRT) support: tell OpenGL to use all specified
        // render targets when drawing and/or clearing
        {
            const size_t num_color_attachments = maybe_custom_render_target->color_attachments().size();

            std::vector<GLenum> attachments;
            attachments.reserve(num_color_attachments);
            for (size_t i = 0; i < num_color_attachments; ++i) {
                attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i));
            }
            glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());
        }

        // if requested, clear the buffers
        {
            static_assert(num_options<RenderBufferLoadAction>() == 2);

            // if requested, clear color buffers
            for (size_t i = 0; i < maybe_custom_render_target->color_attachments().size(); ++i) {
                const RenderTargetColorAttachment& color_attachment = maybe_custom_render_target->color_attachments()[i];
                if (color_attachment.load_action == RenderBufferLoadAction::Clear)
                {
                    glClearBufferfv(
                        GL_COLOR,
                        static_cast<GLint>(i),
                        value_ptr(static_cast<Vec4>(color_attachment.clear_color))
                    );
                }
            }

            // if requested, clear depth buffer
            if (maybe_custom_render_target->depth_attachment() and maybe_custom_render_target->depth_attachment()->load_action == RenderBufferLoadAction::Clear) {
                gl::clear(GL_DEPTH_BUFFER_BIT);
            }
        }
    }
    else
    {
        gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);

        // we're rendering to the window
        if (camera.clear_flags_ != CameraClearFlag::None) {

            // clear window
            const GLenum clear_flags = camera.clear_flags_ & CameraClearFlag::SolidColor ?
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT :
                GL_DEPTH_BUFFER_BIT;

            // clear color is in sRGB, but the window's framebuffer is sRGB-corrected
            // and assume that clear colors are in linear space
            const Color linear_color = to_linear_colorspace(camera.background_color_);
            gl::clear_color(
                linear_color.r,
                linear_color.g,
                linear_color.b,
                linear_color.a
            );
            gl::clear(clear_flags);
        }
    }

    return maybe_render_fbo;
}

void osc::GraphicsBackend::resolve_render_buffers(
    const RenderTarget& render_target)
{
    static_assert(num_options<RenderBufferStoreAction>() == 2, "check 'if's etc. in this code");

    OSC_PERF("RenderTexture::resolveBuffers");

    // setup FBOs (reused per color buffer)
    gl::FrameBuffer multisampled_read_fbo;
    gl::bind_framebuffer(GL_READ_FRAMEBUFFER, multisampled_read_fbo);

    gl::FrameBuffer resolved_draw_fbo;
    gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, resolved_draw_fbo);

    // resolve each color buffer with a blit
    for (size_t i = 0; i < render_target.color_attachments().size(); ++i) {
        const RenderTargetColorAttachment& attachment = render_target.color_attachments()[i];
        const SharedColorRenderBuffer& buffer = attachment.buffer;
        RenderBufferOpenGLData& buffer_opengl_data = buffer.impl_->upd_opengl_data();

        if (attachment.store_action != RenderBufferStoreAction::Resolve) {
            continue;  // we don't need to resolve this color buffer
        }

        bool can_resolve_buffer = false;  // changes if the underlying buffer data is resolve-able
        std::visit(Overload{
            [](SingleSampledTexture&)
            {
                // don't resolve: it's single-sampled
            },
            [&can_resolve_buffer, i](MultisampledRBOAndResolvedTexture& t)
            {
                const GLenum attachment_location = GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i);

                gl::framebuffer_renderbuffer(
                    GL_READ_FRAMEBUFFER,
                    attachment_location,
                    t.multisampled_rbo
                );
                glReadBuffer(attachment_location);

                gl::framebuffer_texture2D(
                    GL_DRAW_FRAMEBUFFER,
                    attachment_location,
                    t.single_sampled_texture2D,
                    0
                );
                glDrawBuffers(1, &attachment_location);

                can_resolve_buffer = true;
            },
            [](SingleSampledCubemap&)
            {
                // don't resolve: it's single-sampled
            }
        }, buffer_opengl_data);

        if (can_resolve_buffer) {
            const Vec2i dimensions = attachment.buffer.impl_->dimensions();
            gl::blit_framebuffer(
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

    // resolve depth (+stencil) buffer with a blit
    if (render_target.depth_attachment() and render_target.depth_attachment()->store_action == RenderBufferStoreAction::Resolve) {
        bool can_resolve_buffer = false;  // changes if the underlying buffer data is resolve-able
        const GLenum attachment = detail::has_stencil_component(render_target.depth_attachment()->buffer.format()) ?
            GL_DEPTH_STENCIL_ATTACHMENT :
            GL_DEPTH_ATTACHMENT;
        std::visit(Overload
        {
            [](SingleSampledTexture&) { },  // don't resolve: it's single-sampled
            [&can_resolve_buffer, attachment](MultisampledRBOAndResolvedTexture& t)
            {
                gl::framebuffer_renderbuffer(
                    GL_READ_FRAMEBUFFER,
                    attachment,
                    t.multisampled_rbo
                );
                glReadBuffer(attachment);

                gl::framebuffer_texture2D(
                    GL_DRAW_FRAMEBUFFER,
                    attachment,
                    t.single_sampled_texture2D,
                    0
                );
                {
                    const GLenum buf = attachment;
                    glDrawBuffers(1, &buf);
                }

                can_resolve_buffer = true;
            },
            [](SingleSampledCubemap&) {}  // don't resolve: it's single-sampled
        }, render_target.depth_attachment()->buffer.impl_->upd_opengl_data());

        if (can_resolve_buffer)
        {
            const Vec2i dimensions = render_target.depth_attachment()->buffer.impl_->dimensions();
            gl::blit_framebuffer(
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

void osc::GraphicsBackend::render_camera_queue(
    Camera::Impl& camera,
    const RenderTarget* maybe_custom_render_target)
{
    OSC_PERF("GraphicsBackend::render_camera_queue");

    if (maybe_custom_render_target) {
        maybe_custom_render_target->validate_or_throw();
    }

    const float output_aspect_ratio = setup_top_level_pipeline_state(
        camera,
        maybe_custom_render_target
    );

    {
        const std::optional<gl::FrameBuffer> maybe_tmp_fbo_KEEPALIVE =
            bind_and_clear_render_buffers(camera, maybe_custom_render_target);
        flush_render_queue(camera, output_aspect_ratio);
    }

    if (maybe_custom_render_target) {
        resolve_render_buffers(*maybe_custom_render_target);
    }

    teardown_top_level_pipeline_state(
        camera,
        maybe_custom_render_target
    );
}

void osc::GraphicsBackend::draw(
    const Mesh& mesh,
    const Transform& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybe_material_property_block,
    std::optional<size_t> maybe_submesh_index)
{
    if (maybe_submesh_index and *maybe_submesh_index >= mesh.num_submesh_descriptors()) {
        throw std::out_of_range{"the given sub-mesh index was out of range (i.e. the given mesh does not have that many sub-meshes)"};
    }

    camera.impl_.upd()->render_queue_.emplace_back(
        mesh,
        transform,
        material,
        maybe_material_property_block,
        maybe_submesh_index
    );
}

void osc::GraphicsBackend::draw(
    const Mesh& mesh,
    const Mat4& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybe_material_property_block,
    std::optional<size_t> maybe_submesh_index)
{
    if (maybe_submesh_index and *maybe_submesh_index >= mesh.num_submesh_descriptors()) {
        throw std::out_of_range{"the given sub-mesh index was out of range (i.e. the given mesh does not have that many sub-meshes)"};
    }

    camera.impl_.upd()->render_queue_.emplace_back(
        mesh,
        transform,
        material,
        maybe_material_property_block,
        maybe_submesh_index
    );
}

void osc::GraphicsBackend::blit(
    const Texture2D& source,
    RenderTexture& destination)
{
    Camera camera;
    camera.set_background_color(Color::clear());
    camera.set_projection_matrix_override(identity<Mat4>());
    camera.set_view_matrix_override(identity<Mat4>());

    Material material = g_graphics_context_impl->quad_material();
    material.set("uTexture", source);

    graphics::draw(g_graphics_context_impl->quad_mesh(), Transform{}, material, camera);
    camera.render_to(destination);
}

void osc::GraphicsBackend::blit_to_screen(
    const RenderTexture& source,
    const Rect& rect,
    BlitFlags flags)
{
    blit_to_screen(source, rect, g_graphics_context_impl->quad_material(), flags);
}

void osc::GraphicsBackend::blit_to_screen(
    const RenderTexture& source,
    const Rect& rect,
    const Material& material,
    BlitFlags)
{
    OSC_ASSERT(g_graphics_context_impl);
    OSC_ASSERT(source.impl_->has_been_rendered_to() && "the input texture has not been rendered to");

    Camera camera;
    camera.set_background_color(Color::clear());
    camera.set_pixel_rect(rect);
    camera.set_projection_matrix_override(identity<Mat4>());
    camera.set_view_matrix_override(identity<Mat4>());
    camera.set_clear_flags(CameraClearFlag::None);

    Material material_copy{material};
    material_copy.set("uTexture", source);
    graphics::draw(g_graphics_context_impl->quad_mesh(), Transform{}, material_copy, camera);
    camera.render_to_screen();
    material_copy.unset("uTexture");
}

void osc::GraphicsBackend::blit_to_screen(
    const Texture2D& source,
    const Rect& rect)
{
    OSC_ASSERT(g_graphics_context_impl);

    Camera camera;
    camera.set_background_color(Color::clear());
    camera.set_pixel_rect(rect);
    camera.set_projection_matrix_override(identity<Mat4>());
    camera.set_view_matrix_override(identity<Mat4>());
    camera.set_clear_flags(CameraClearFlag::None);

    Material material_copy{g_graphics_context_impl->quad_material()};
    material_copy.set("uTexture", source);
    graphics::draw(g_graphics_context_impl->quad_mesh(), Transform{}, material_copy, camera);
    camera.render_to_screen();
    material_copy.unset("uTexture");
}

void osc::GraphicsBackend::copy_texture(
    const RenderTexture& source,
    Texture2D& destination)
{
    copy_texture(source, destination, CubemapFace::PositiveX);
}

void osc::GraphicsBackend::copy_texture(
    const RenderTexture& source,
    Texture2D& destination,
    CubemapFace face)
{
    OSC_ASSERT(g_graphics_context_impl);
    OSC_ASSERT(source.impl_->has_been_rendered_to() && "the input texture has not been rendered to");

    // create a source (read) framebuffer for blitting from the source render texture
    gl::FrameBuffer read_fbo;
    gl::bind_framebuffer(GL_READ_FRAMEBUFFER, read_fbo);
    std::visit(Overload{  // attach source texture depending on rendertexture's type
        [](SingleSampledTexture& t)
        {
            gl::framebuffer_texture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                t.texture2D,
                0
            );
        },
        [](MultisampledRBOAndResolvedTexture& t)
        {
            gl::framebuffer_texture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                t.single_sampled_texture2D,
                0
            );
        },
        [face](SingleSampledCubemap& t)
        {
            glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                to_opengl_texture_cubemap_enum(face),
                t.cubemap.get(),
                0
            );
        }
    }, const_cast<RenderTexture::Impl&>(*source.impl_).get_color_render_buffer_data());
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // create a destination (draw) framebuffer for blitting to the destination render texture
    gl::FrameBuffer draw_fbo;
    gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
    gl::framebuffer_texture2D(
        GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        destination.impl_.upd()->updTexture(),
        0
    );
    {
        const GLenum buf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &buf);
    }

    // blit the read framebuffer to the draw framebuffer
    gl::blit_framebuffer(
        0,
        0,
        source.dimensions().x,
        source.dimensions().y,
        0,
        0,
        destination.dimensions().x,
        destination.dimensions().y,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR  // the two texture may have different dimensions (avoid GL_NEAREST)
    );

    // then download the blitted data into the texture's CPU buffer
    {
        std::vector<uint8_t>& cpu_buffer = destination.impl_.upd()->pixel_data_;
        const GLint pack_format = to_opengl_image_pixel_pack_alignment(destination.texture_format());

        OSC_ASSERT(is_aligned_at_least(cpu_buffer.data(), pack_format) && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(cpu_buffer.size() == area_of(destination.dimensions())*num_bytes_per_pixel_in(destination.texture_format()));

        gl::viewport(0, 0, destination.dimensions().x, destination.dimensions().y);
        gl::bind_framebuffer(GL_READ_FRAMEBUFFER, draw_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::pixel_store_i(GL_PACK_ALIGNMENT, pack_format);
        glReadPixels(
            0,
            0,
            destination.dimensions().x,
            destination.dimensions().y,
            to_opengl_image_color_format_enum(destination.texture_format()),
            to_opengl_image_data_type_enum(destination.texture_format()),
            cpu_buffer.data()
        );
    }
    gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);
}

void osc::GraphicsBackend::copy_texture(
    const RenderTexture& source,
    Cubemap& destination,
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
    const auto max_mipmap_level = static_cast<size_t>(max(
        0,
        static_cast<int>(std::bit_width(static_cast<size_t>(destination.width()))) - 1
    ));

    OSC_ASSERT(source.dimensionality() == TextureDimensionality::Cube && "provided render texture must be a cubemap to call this method");
    OSC_ASSERT(mip <= max_mipmap_level);

    // blit each face of the source cubemap into the output cubemap
    for (size_t face = 0; face < 6; ++face) {
        gl::FrameBuffer readFBO;
        gl::bind_framebuffer(GL_READ_FRAMEBUFFER, readFBO);
        std::visit(Overload  // attach source texture depending on `RenderTexture`'s type
        {
            [](SingleSampledTexture&)
            {
                OSC_ASSERT(false && "cannot call copy_texture (Cubemap --> Cubemap) with a 2D render");
            },
            [](MultisampledRBOAndResolvedTexture&)
            {
                OSC_ASSERT(false && "cannot call copy_texture (Cubemap --> Cubemap) with a 2D render");
            },
            [face](SingleSampledCubemap& t)
            {
                glFramebufferTexture2D(
                    GL_READ_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face),
                    t.cubemap.get(),
                    0
                );
            }
        }, const_cast<RenderTexture::Impl&>(*source.impl_).get_color_render_buffer_data());
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        gl::FrameBuffer draw_fbo;
        gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face),
            destination.impl_.upd()->upd_cubemap().get(),
            static_cast<GLint>(mip)
        );
        {
            const GLenum buf = GL_COLOR_ATTACHMENT0;
            glDrawBuffers(1, &buf);
        }

        // blit the read framebuffer to the draw framebuffer
        gl::blit_framebuffer(
            0,
            0,
            source.dimensions().x,
            source.dimensions().y,
            0,
            0,
            destination.width() / (1<<mip),
            destination.width() / (1<<mip),
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR  // the two texture may have different dimensions (avoid GL_NEAREST)
        );
    }

    // TODO: should be copied into CPU memory if mip==0? (won't store mipmaps in the CPU but
    // maybe it makes sense to store the mip==0 in CPU?)
}
