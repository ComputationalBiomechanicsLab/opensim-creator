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
#include <oscar/Graphics/Geometries/PlaneGeometry.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/GraphicsContext.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshFunctions.h>
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
#include <oscar/Utils/Algorithms.h>
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
        if (string_ptr) {
            return CStringView{std::launder(reinterpret_cast<const value_type*>(string_ptr))};
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

        // this list isn't comprehensive, it's just things that I reakon the OpenGL backend
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
        // formats for each channel:
        //
        // - uint8 (normalized)
        // - int8 (normalized)
        // - float32
        // - uint8/uint16/uint32 (non-normalized)
        // - int8/int16/int32 (non-normalized)
        //
        // see "Required Formats" in: https://www.khronos.org/opengl/wiki/Image_Format

        if (logging_level < log_level()) {
            return;
        }

        std::vector<CStringView> extensions_needed = get_all_opengl_extensions_used_by_opengl_backend();
        std::sort(extensions_needed.begin(), extensions_needed.end());

        std::vector<CStringView> extensions_available = get_extensions_supported_by_opengl_backend();
        std::sort(extensions_available.begin(), extensions_available.end());

        std::vector<CStringView> extensions_missing;
        extensions_missing.reserve(extensions_needed.size());  // pessimistic guess
        std::set_difference(
            extensions_needed.begin(),
            extensions_needed.end(),
            extensions_available.begin(),
            extensions_available.end(),
            std::back_inserter(extensions_missing)
        );

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

    ShaderPropertyType get_shader_type(const MaterialValue& material_val)
    {
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
        case variant_index<MaterialValue, int32_t>():
            return ShaderPropertyType::Int;
        case variant_index<MaterialValue, bool>():
            return ShaderPropertyType::Bool;
        case variant_index<MaterialValue, Texture2D>():
            return ShaderPropertyType::Sampler2D;
        case variant_index<MaterialValue, RenderTexture>(): {

            static_assert(num_options<TextureDimensionality>() == 2);
            return std::get<RenderTexture>(material_val).getDimensionality() == TextureDimensionality::Tex2D ?
                ShaderPropertyType::Sampler2D :
                ShaderPropertyType::SamplerCube;
        }
        case variant_index<MaterialValue, Cubemap>():
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
    ShaderPropertyType opengl_shader_type_to_osc_shader_type(GLenum e)
    {
        static_assert(num_options<ShaderPropertyType>() == 11);

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

    const ShaderElement* tryGetValue(const FastStringHashtable<ShaderElement>& m, std::string_view k)
    {
        return try_find(m, k);
    }
}

namespace
{
    // transform storage: either as a matrix or a transform
    //
    // calling code is allowed to submit transforms as either a `Transform` (preferred) or a
    // `Mat4` (can be handier)
    //
    // these need to be stored as-is, because that's the smallest possible representation and
    // the drawing algorithm needs to traverse + sort the render objects at runtime (so size
    // is important)
    using Mat4OrTransform = std::variant<Mat4, Transform>;

    Mat4 mat4_cast(const Mat4OrTransform& mat4_or_transform)
    {
        return std::visit(Overload{
            [](const Mat4& mat4) { return mat4; },
            [](const Transform& transform) { return mat4_cast(transform); }
        }, mat4_or_transform);
    }

    Mat4 normal_matrix4(const Mat4OrTransform& mat4_or_transform)
    {
        return std::visit(Overload{
            [](const Mat4& mat4) { return normal_matrix4(mat4); },
            [](const Transform& transform) { return normal_matrix_4x4(transform); }
        }, mat4_or_transform);
    }

    Mat3 normal_matrix(const Mat4OrTransform& mat4_or_transform)
    {
        return std::visit(Overload{
            [](const Mat4& mat4) { return normal_matrix(mat4); },
            [](const Transform& transform) { return normal_matrix(transform); }
        }, mat4_or_transform);
    }

    // this is what is stored in the renderer's render queue
    struct RenderObject final {

        RenderObject(
            Mesh mesh_,
            const Transform& transform_,
            Material material_,
            std::optional<MaterialPropertyBlock> maybe_prop_block_,
            std::optional<size_t> maybe_submesh_index_) :

            material{std::move(material_)},
            mesh{std::move(mesh_)},
            maybe_prop_block{std::move(maybe_prop_block_)},
            transform{transform_},
            world_centroid{material.getTransparent() ? transform_point(transform_, centroid_of(mesh.getBounds())) : Vec3{}},
            maybe_submesh_index{maybe_submesh_index_}
        {}

        RenderObject(
            Mesh mesh_,
            const Mat4& transform_,
            Material material_,
            std::optional<MaterialPropertyBlock> maybe_prop_block_,
            std::optional<size_t> maybe_submesh_index_) :

            material{std::move(material_)},
            mesh{std::move(mesh_)},
            maybe_prop_block{std::move(maybe_prop_block_)},
            transform{transform_},
            world_centroid{material.getTransparent() ? transform_ * Vec4{centroid_of(mesh.getBounds()), 1.0f} : Vec3{}},
            maybe_submesh_index{maybe_submesh_index_}
        {}

        friend void swap(RenderObject& a, RenderObject& b) noexcept
        {
            using std::swap;

            swap(a.material, b.material);
            swap(a.mesh, b.mesh);
            swap(a.maybe_prop_block, b.maybe_prop_block);
            swap(a.transform, b.transform);
            swap(a.world_centroid, b.world_centroid);
            swap(a.maybe_submesh_index, b.maybe_submesh_index);
        }

        friend bool operator==(const RenderObject&, const RenderObject&) = default;

        Material material;
        Mesh mesh;
        std::optional<MaterialPropertyBlock> maybe_prop_block;
        Mat4OrTransform transform;
        Vec3 world_centroid;
        std::optional<size_t> maybe_submesh_index;
    };

    // returns true if the render object is opaque
    bool is_opaque(const RenderObject& ro)
    {
        return !ro.material.getTransparent();
    }

    bool is_depth_tested(const RenderObject& ro)
    {
        return ro.material.getDepthTested();
    }

    Mat4 model_mat4(const RenderObject& ro)
    {
        return mat4_cast(ro.transform);
    }

    Mat3 normal_matrix(const RenderObject& ro)
    {
        return normal_matrix(ro.transform);
    }

    Mat4 normal_matrix4(const RenderObject& ro)
    {
        return normal_matrix4(ro.transform);
    }

    Vec3 const& worldspace_centroid(const RenderObject& ro)
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
        explicit RenderObjectHasMaterialPropertyBlock(const std::optional<MaterialPropertyBlock>* mpb) :
            mpb_{mpb}
        {
            OSC_ASSERT(mpb_ != nullptr);
        }

        bool operator()(const RenderObject& ro) const
        {
            return ro.maybe_prop_block == *mpb_;
        }

    private:
        const std::optional<MaterialPropertyBlock>* mpb_;
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
        explicit RenderObjectHasSubMeshIndex(std::optional<size_t> maybe_submesh_index) :
            maybe_submesh_index_{maybe_submesh_index}
        {}

        bool operator()(const RenderObject& ro) const
        {
            return ro.maybe_submesh_index == maybe_submesh_index_;
        }
    private:
        std::optional<size_t> maybe_submesh_index_;
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
                    RenderObjectHasMaterialPropertyBlock{&props_batch_start->maybe_prop_block}
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

        static void bindToInstancedAttributes(
            const Shader::Impl& shaderImpl,
            InstancingState& ins
        );

        static void unbindFromInstancedAttributes(
            const Shader::Impl& shaderImpl,
            InstancingState& ins
        );

        static std::optional<InstancingState> uploadInstanceData(
            std::span<const RenderObject>,
            const Shader::Impl& shaderImpl
        );

        static void tryBindMaterialValueToShaderElement(
            const ShaderElement& se,
            const MaterialValue& v,
            int32_t& textureSlot
        );

        static void handleBatchWithSameSubMesh(
            std::span<const RenderObject>,
            std::optional<InstancingState>& ins
        );

        static void handleBatchWithSameMesh(
            std::span<const RenderObject>,
            std::optional<InstancingState>& ins
        );

        static void handleBatchWithSameMaterialPropertyBlock(
            std::span<const RenderObject>,
            int32_t& textureSlot,
            std::optional<InstancingState>& ins
        );

        static void handleBatchWithSameMaterial(
            const RenderPassState&,
            std::span<const RenderObject>
        );

        static void drawRenderObjects(
            const RenderPassState&,
            std::span<const RenderObject>
        );

        static void drawBatchedByOpaqueness(
            const RenderPassState&,
            std::span<const RenderObject>
        );

        static void validateRenderTarget(
            RenderTarget&
        );
        static Rect calcViewportRect(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static Rect setupTopLevelPipelineState(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static std::optional<gl::FrameBuffer> bindAndClearRenderBuffers(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static void resolveRenderBuffers(
            RenderTarget& maybeCustomRenderTarget
        );
        static void flushRenderQueue(
            Camera::Impl& camera,
            float aspectRatio
        );
        static void teardownTopLevelPipelineState(
            Camera::Impl&,
            RenderTarget* maybeCustomRenderTarget
        );
        static void renderCameraQueue(
            Camera::Impl& camera,
            RenderTarget* maybeCustomRenderTarget = nullptr
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

        static void blitToScreen(
            const RenderTexture&,
            const Rect&,
            BlitFlags
        );

        static void blitToScreen(
            const RenderTexture&,
            const Rect&,
            const Material&,
            BlitFlags
        );

        static void blitToScreen(
            const Texture2D&,
            const Rect&
        );

        static void copyTexture(
            const RenderTexture&,
            Texture2D&
        );

        static void copyTexture(
            const RenderTexture&,
            Texture2D&,
            CubemapFace
        );

        static void copyTexture(
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
    constexpr GLint toOpenGLUnpackAlignment(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::unpack_alignment... });
        }(TextureFormatList{});

        return lut.at(ToIndex(format));
    }

    // returns the format OpenGL will use internally (i.e. on the GPU) to
    // represent the given format+colorspace combo
    constexpr GLenum toOpenGLInternalFormat(
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

        static_assert(num_options<ColorSpace>() == 2);
        if (colorSpace == ColorSpace::sRGB) {
            return srgbLUT.at(ToIndex(format));
        }
        else {
            return linearLUT.at(ToIndex(format));
        }
    }

    constexpr GLenum toOpenGLDataType(CPUDataType t)
    {
        constexpr auto lut = []<CPUDataType... DataTypes>(OptionList<CPUDataType, DataTypes...>)
        {
            return std::to_array({ CPUDataTypeOpenGLTraits<DataTypes>::opengl_data_type... });
        }(CPUDataTypeList{});

        return lut.at(ToIndex(t));
    }

    constexpr CPUDataType toEquivalentCPUDataType(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatTraits<Formats>::equivalent_cpu_datatype... });
        }(TextureFormatList{});

        return lut.at(ToIndex(format));
    }

    constexpr CPUImageFormat toEquivalentCPUImageFormat(TextureFormat format)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatTraits<Formats>::equivalent_cpu_image_format... });
        }(TextureFormatList{});

        return lut.at(ToIndex(format));
    }

    constexpr GLenum toOpenGLFormat(CPUImageFormat t)
    {
        constexpr auto lut = []<CPUImageFormat... Formats>(OptionList<CPUImageFormat, Formats...>)
        {
            return std::to_array({ CPUImageFormatOpenGLTraits<Formats>::opengl_format... });
        }(CPUImageFormatList{});

        return lut.at(ToIndex(t));
    }

    constexpr GLenum toOpenGLTextureEnum(CubemapFace f)
    {
        static_assert(num_options<CubemapFace>() == 6);
        static_assert(static_cast<GLenum>(CubemapFace::PositiveX) == 0);
        static_assert(static_cast<GLenum>(CubemapFace::NegativeZ) == 5);
        static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

        return GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(f);
    }

    GLint toGLTextureTextureWrapParam(TextureWrapMode m)
    {
        static_assert(num_options<TextureWrapMode>() == 3);

        switch (m) {
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
    static_assert(c_TextureWrapModeStrings.size() == num_options<TextureWrapMode>());

    constexpr auto c_TextureFilterModeStrings = std::to_array<CStringView>(
    {
        "Nearest",
        "Linear",
        "Mipmap",
    });
    static_assert(c_TextureFilterModeStrings.size() == num_options<TextureFilterMode>());

    GLint toGLTextureMinFilterParam(TextureFilterMode m)
    {
        static_assert(num_options<TextureFilterMode>() == 3);

        switch (m) {
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

    GLint toGLTextureMagFilterParam(TextureFilterMode m)
    {
        static_assert(num_options<TextureFilterMode>() == 3);

        switch (m) {
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

        const size_t numPixelsPerFace = static_cast<size_t>(m_Width*m_Width)*NumBytesPerPixel(m_Format);
        m_Data.resize(num_options<CubemapFace>() * numPixelsPerFace);
    }

    int32_t width() const
    {
        return m_Width;
    }

    TextureFormat texture_format() const
    {
        return m_Format;
    }

    TextureWrapMode wrap_mode() const
    {
        return m_WrapModeU;
    }

    void set_wrap_mode(TextureWrapMode wm)
    {
        m_WrapModeU = wm;
        m_WrapModeV = wm;
        m_WrapModeW = wm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode get_wrap_mode_u() const
    {
        return m_WrapModeU;
    }

    void set_wrap_mode_u(TextureWrapMode wm)
    {
        m_WrapModeU = wm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode get_wrap_mode_v() const
    {
        return m_WrapModeV;
    }

    void set_wrap_mode_v(TextureWrapMode wm)
    {
        m_WrapModeV = wm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode wrap_mode_w() const
    {
        return m_WrapModeW;
    }

    void set_wrap_mode_w(TextureWrapMode wm)
    {
        m_WrapModeW = wm;
        m_TextureParamsVersion.reset();
    }

    TextureFilterMode filter_mode() const
    {
        return m_FilterMode;
    }

    void set_filter_mode(TextureFilterMode fm)
    {
        m_FilterMode = fm;
        m_TextureParamsVersion.reset();
    }

    void set_pixel_data(CubemapFace face, std::span<const uint8_t> data)
    {
        const size_t faceIndex = ToIndex(face);
        const auto numPixels = static_cast<size_t>(m_Width) * static_cast<size_t>(m_Width);
        const size_t numBytesPerCubeFace = numPixels * NumBytesPerPixel(m_Format);
        const size_t destinationDataStart = faceIndex * numBytesPerCubeFace;
        const size_t destinationDataEnd = destinationDataStart + numBytesPerCubeFace;

        OSC_ASSERT(faceIndex < num_options<CubemapFace>() && "invalid cubemap face passed to Cubemap::set_pixel_data");
        OSC_ASSERT(data.size() == numBytesPerCubeFace && "incorrect amount of data passed to Cubemap::set_pixel_data: the data must match the dimensions and texture format of the cubemap");
        OSC_ASSERT(destinationDataEnd <= m_Data.size() && "out of range assignment detected: this should be handled in the constructor");

        copy(data, m_Data.begin() + destinationDataStart);
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
        const size_t numBytesPerPixel = NumBytesPerPixel(m_Format);
        const size_t numBytesPerRow = m_Width * numBytesPerPixel;
        const size_t numBytesPerFace = m_Width * numBytesPerRow;
        const size_t numBytesInCubemap = num_options<CubemapFace>() * numBytesPerFace;
        const CPUDataType cpuDataType = toEquivalentCPUDataType(m_Format);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        const CPUImageFormat cpuChannelLayout = toEquivalentCPUImageFormat(m_Format);  // TextureFormat's layout == CPU formats's layout for cubemaps
        const GLint unpackAlignment = toOpenGLUnpackAlignment(m_Format);

        // sanity-check before doing anything with OpenGL
        OSC_ASSERT(numBytesPerRow % unpackAlignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(is_aligned_at_least(m_Data.data(), unpackAlignment) && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(numBytesInCubemap <= m_Data.size() && "the number of bytes in the cubemap (CPU-side) is less than expected: this is a developer bug");
        static_assert(num_options<TextureFormat>() == 7, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");

        // upload cubemap to GPU
        static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);
        gl::bind_texture(buf.texture);
        gl::pixel_store_i(GL_UNPACK_ALIGNMENT, unpackAlignment);
        for (GLint faceIdx = 0; faceIdx < static_cast<GLint>(num_options<CubemapFace>()); ++faceIdx) {

            const size_t faceBytesBegin = faceIdx * numBytesPerFace;

            gl::tex_image2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx,
                0,
                toOpenGLInternalFormat(m_Format, ColorSpace::sRGB),  // cubemaps are always sRGB
                m_Width,
                m_Width,
                0,
                toOpenGLFormat(cpuChannelLayout),
                toOpenGLDataType(cpuDataType),
                m_Data.data() + faceBytesBegin
            );
        }

        // generate mips (care: they can be uploaded to with graphics::copyTexture)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        gl::bind_texture();

        buf.dataVersion = m_DataVersion;
    }

    void updateTextureParameters(CubemapOpenGLData& buf)
    {
        gl::bind_texture(buf.texture);

        // set texture parameters
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, toGLTextureMagFilterParam(m_FilterMode));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, toGLTextureMinFilterParam(m_FilterMode));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, toGLTextureTextureWrapParam(m_WrapModeU));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, toGLTextureTextureWrapParam(m_WrapModeV));
        gl::tex_parameter_i(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, toGLTextureTextureWrapParam(m_WrapModeW));

        // cleanup OpenGL binding state
        gl::bind_texture();

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

void osc::Cubemap::set_wrap_mode(TextureWrapMode wm)
{
    impl_.upd()->set_wrap_mode(wm);
}

TextureWrapMode osc::Cubemap::wrap_mode_u() const
{
    return impl_->get_wrap_mode_u();
}

void osc::Cubemap::set_wrap_mode_u(TextureWrapMode wm)
{
    impl_.upd()->set_wrap_mode_u(wm);
}

TextureWrapMode osc::Cubemap::wrap_mode_v() const
{
    return impl_->get_wrap_mode_v();
}

void osc::Cubemap::set_wrap_mode_v(TextureWrapMode wm)
{
    impl_.upd()->set_wrap_mode_v(wm);
}

TextureWrapMode osc::Cubemap::wrap_mode_w() const
{
    return impl_->wrap_mode_w();
}

void osc::Cubemap::set_wrap_mode_w(TextureWrapMode wm)
{
    impl_.upd()->set_wrap_mode_w(wm);
}

TextureFilterMode osc::Cubemap::filter_mode() const
{
    return impl_->filter_mode();
}

void osc::Cubemap::set_filter_mode(TextureFilterMode fm)
{
    impl_.upd()->set_filter_mode(fm);
}

TextureFormat osc::Cubemap::texture_format() const
{
    return impl_->texture_format();
}

void osc::Cubemap::set_pixel_data(CubemapFace face, std::span<const uint8_t> channelsRowByRow)
{
    impl_.upd()->set_pixel_data(face, channelsRowByRow);
}


//////////////////////////////////
//
// texture stuff
//
//////////////////////////////////

namespace
{
    std::vector<Color> readPixelDataAsColor(
        std::span<const uint8_t> pixelData,
        TextureFormat pixelDataFormat)
    {
        const TextureChannelFormat channelFormat = ChannelFormat(pixelDataFormat);

        const size_t numChannels = NumChannels(pixelDataFormat);
        const size_t bytesPerChannel = NumBytesPerChannel(channelFormat);
        const size_t bytesPerPixel = bytesPerChannel * numChannels;
        const size_t numPixels = pixelData.size() / bytesPerPixel;

        OSC_ASSERT(pixelData.size() % bytesPerPixel == 0);

        std::vector<Color> rv;
        rv.reserve(numPixels);

        static_assert(num_options<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8) {
            // unpack 8-bit channel bytes into floating-point Color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel) {
                const size_t pixelStart = bytesPerPixel * pixel;

                Color color = Color::black();
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    const size_t channelStart = pixelStart + channel;
                    color[channel] = Unorm8{pixelData[channelStart]}.normalized_value();
                }
                rv.push_back(color);
            }
        }
        else if (channelFormat == TextureChannelFormat::Float32 && bytesPerChannel == sizeof(float)) {
            // read 32-bit channel floats into Color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel) {
                const size_t pixelStart = bytesPerPixel * pixel;

                Color color = Color::black();
                for (size_t channel = 0; channel < numChannels; ++channel)
                {
                    const size_t channelStart = pixelStart + channel*bytesPerChannel;

                    std::span<const uint8_t> src{pixelData.data() + channelStart, sizeof(float)};
                    std::array<uint8_t, sizeof(float)> dest{};
                    copy(src, dest.begin());

                    color[channel] = cpp20::bit_cast<float>(dest);
                }
                rv.push_back(color);
            }
        }
        else {
            OSC_ASSERT(false && "unsupported texture channel format or bytes per channel detected");
        }

        return rv;
    }

    std::vector<Color32> readPixelDataAsColor32(
        std::span<const uint8_t> pixelData,
        TextureFormat pixelDataFormat)
    {
        const TextureChannelFormat channelFormat = ChannelFormat(pixelDataFormat);

        const size_t numChannels = NumChannels(pixelDataFormat);
        const size_t bytesPerChannel = NumBytesPerChannel(channelFormat);
        const size_t bytesPerPixel = bytesPerChannel * numChannels;
        const size_t numPixels = pixelData.size() / bytesPerPixel;

        std::vector<Color32> rv;
        rv.reserve(numPixels);

        static_assert(num_options<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8) {
            // read 8-bit channel bytes into 8-bit Color32 color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel) {
                const size_t pixelStart = bytesPerPixel * pixel;

                Color32 color = {0x00, 0x00, 0x00, 0xff};
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    const size_t channelStart = pixelStart + channel;
                    color[channel] = pixelData[channelStart];
                }
                rv.push_back(color);
            }
        }
        else {
            static_assert(std::is_same_v<Color::value_type, float>);
            OSC_ASSERT(bytesPerChannel == sizeof(float));

            // pack 32-bit channel floats into 8-bit Color32 color channels
            for (size_t pixel = 0; pixel < numPixels; ++pixel) {
                const size_t pixelStart = bytesPerPixel * pixel;

                Color32 color = {0x00, 0x00, 0x00, 0xff};
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    const size_t channelStart = pixelStart + channel*sizeof(float);

                    std::span<const uint8_t> src{pixelData.data() + channelStart, sizeof(float)};
                    std::array<uint8_t, sizeof(float)> dest{};
                    copy(src, dest.begin());
                    const auto channelFloat = cpp20::bit_cast<float>(dest);

                    color[channel] = Unorm8{channelFloat};
                }
                rv.push_back(color);
            }
        }

        return rv;
    }

    void encodePixelsInDesiredFormat(
        std::span<const Color> pixels,
        TextureFormat pixelDataFormat,
        std::vector<uint8_t>& pixelData)
    {
        const TextureChannelFormat channelFormat = ChannelFormat(pixelDataFormat);

        const size_t numChannels = NumChannels(pixelDataFormat);
        const size_t bytesPerChannel = NumBytesPerChannel(channelFormat);
        const size_t bytesPerPixel = bytesPerChannel * numChannels;
        const size_t numPixels = pixels.size();
        const size_t numOutputBytes = bytesPerPixel * numPixels;

        pixelData.clear();
        pixelData.reserve(numOutputBytes);

        OSC_ASSERT(numChannels <= std::tuple_size_v<Color>);
        static_assert(num_options<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8) {
            // clamp pixels, convert them to bytes, add them to pixel data buffer
            for (const Color& pixel : pixels) {
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    pixelData.push_back(Unorm8{pixel[channel]}.raw_value());
                }
            }
        }
        else {
            // write pixels to pixel data buffer as-is (they're floats already)
            for (const Color& pixel : pixels) {
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    push_as_bytes(pixel[channel], pixelData);
                }
            }
        }
    }

    void encodePixels32InDesiredFormat(
        std::span<const Color32> pixels,
        TextureFormat pixelDataFormat,
        std::vector<uint8_t>& pixelData)
    {
        const TextureChannelFormat channelFormat = ChannelFormat(pixelDataFormat);

        const size_t numChannels = NumChannels(pixelDataFormat);
        const size_t bytesPerChannel = NumBytesPerChannel(channelFormat);
        const size_t bytesPerPixel = bytesPerChannel * numChannels;
        const size_t numPixels = pixels.size();
        const size_t numOutputBytes = bytesPerPixel * numPixels;

        pixelData.clear();
        pixelData.reserve(numOutputBytes);

        OSC_ASSERT(numChannels <= Color32::length());
        static_assert(num_options<TextureChannelFormat>() == 2);
        if (channelFormat == TextureChannelFormat::Uint8) {
            // write pixels to pixel data buffer as-is (they're bytes already)
            for (const Color32& pixel : pixels) {
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    pixelData.push_back(pixel[channel].raw_value());
                }
            }
        }
        else
        {
            // upscale pixels to float32s and write the floats to the pixel buffer
            for (const Color32& pixel : pixels) {
                for (size_t channel = 0; channel < numChannels; ++channel) {
                    const float pixelFloatVal = Unorm8{pixel[channel]}.normalized_value();
                    push_as_bytes(pixelFloatVal, pixelData);
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

    TextureFormat texture_format() const
    {
        return m_Format;
    }

    ColorSpace getColorSpace() const
    {
        return m_ColorSpace;
    }

    TextureWrapMode wrap_mode() const
    {
        return get_wrap_mode_u();
    }

    void set_wrap_mode(TextureWrapMode twm)
    {
        set_wrap_mode_u(twm);
        set_wrap_mode_v(twm);
        set_wrap_mode_w(twm);
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode get_wrap_mode_u() const
    {
        return m_WrapModeU;
    }

    void set_wrap_mode_u(TextureWrapMode twm)
    {
        m_WrapModeU = twm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode get_wrap_mode_v() const
    {
        return m_WrapModeV;
    }

    void set_wrap_mode_v(TextureWrapMode twm)
    {
        m_WrapModeV = twm;
        m_TextureParamsVersion.reset();
    }

    TextureWrapMode wrap_mode_w() const
    {
        return m_WrapModeW;
    }

    void set_wrap_mode_w(TextureWrapMode twm)
    {
        m_WrapModeW = twm;
        m_TextureParamsVersion.reset();
    }

    TextureFilterMode filter_mode() const
    {
        return m_FilterMode;
    }

    void set_filter_mode(TextureFilterMode tfm)
    {
        m_FilterMode = tfm;
        m_TextureParamsVersion.reset();
    }

    std::vector<Color> getPixels() const
    {
        return readPixelDataAsColor(m_PixelData, m_Format);
    }

    void setPixels(std::span<const Color> pixels)
    {
        OSC_ASSERT(std::ssize(pixels) == static_cast<ptrdiff_t>(m_Dimensions.x*m_Dimensions.y));
        encodePixelsInDesiredFormat(pixels, m_Format, m_PixelData);
    }

    std::vector<Color32> getPixels32() const
    {
        return readPixelDataAsColor32(m_PixelData, m_Format);
    }

    void setPixels32(std::span<const Color32> pixels)
    {
        OSC_ASSERT(std::ssize(pixels) == static_cast<ptrdiff_t>(m_Dimensions.x*m_Dimensions.y));
        encodePixels32InDesiredFormat(pixels, m_Format, m_PixelData);
    }

    std::span<const uint8_t> getPixelData() const
    {
        return m_PixelData;
    }

    void set_pixel_data(std::span<const uint8_t> pixelData)
    {
        OSC_ASSERT(pixelData.size() == NumBytesPerPixel(m_Format)*m_Dimensions.x*m_Dimensions.y && "incorrect number of bytes passed to Texture2D::set_pixel_data");
        OSC_ASSERT(pixelData.size() == m_PixelData.size());

        copy(pixelData, m_PixelData.begin());
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

        if (bufs.texture_params_version != m_TextureParamsVersion)
        {
            setTextureParams(bufs);
        }

        return bufs.texture;
    }

private:
    void uploadToGPU()
    {
        *m_MaybeGPUTexture = Texture2DOpenGLData{};

        const size_t numBytesPerPixel = NumBytesPerPixel(m_Format);
        const size_t numBytesPerRow = m_Dimensions.x * numBytesPerPixel;
        const GLint unpackAlignment = toOpenGLUnpackAlignment(m_Format);
        const CPUDataType cpuDataType = toEquivalentCPUDataType(m_Format);  // TextureFormat's datatype == CPU format's datatype for cubemaps
        const CPUImageFormat cpuChannelLayout = toEquivalentCPUImageFormat(m_Format);  // TextureFormat's layout == CPU formats's layout for cubemaps

        static_assert(num_options<TextureFormat>() == 7, "careful here, glTexImage2D will not accept some formats (e.g. GL_RGBA16F) as the externally-provided format (must be GL_RGBA format with GL_HALF_FLOAT type)");
        OSC_ASSERT(numBytesPerRow % unpackAlignment == 0 && "the memory alignment of each horizontal line in an OpenGL texture must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(is_aligned_at_least(m_PixelData.data(), unpackAlignment) && "the memory alignment of the supplied pixel memory must match the GL_UNPACK_ALIGNMENT arg (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");

        // one-time upload, because pixels cannot be altered
        gl::bind_texture((*m_MaybeGPUTexture)->texture);
        gl::pixel_store_i(GL_UNPACK_ALIGNMENT, unpackAlignment);
        gl::tex_image2D(
            GL_TEXTURE_2D,
            0,
            toOpenGLInternalFormat(m_Format, m_ColorSpace),
            m_Dimensions.x,
            m_Dimensions.y,
            0,
            toOpenGLFormat(cpuChannelLayout),
            toOpenGLDataType(cpuDataType),
            m_PixelData.data()
        );
        glGenerateMipmap(GL_TEXTURE_2D);
        gl::bind_texture();
    }

    void setTextureParams(Texture2DOpenGLData& bufs)
    {
        gl::bind_texture(bufs.texture);
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGLTextureTextureWrapParam(m_WrapModeU));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGLTextureTextureWrapParam(m_WrapModeV));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, toGLTextureTextureWrapParam(m_WrapModeW));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGLTextureMinFilterParam(m_FilterMode));
        gl::tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGLTextureMagFilterParam(m_FilterMode));
        gl::bind_texture();
        bufs.texture_params_version = m_TextureParamsVersion;
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
    static_assert(num_options<TextureChannelFormat>() == 2);
    const bool isByteOriented = channelFormat == TextureChannelFormat::Uint8;

    static_assert(num_options<TextureFormat>() == 7);
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
    static_assert(num_options<TextureChannelFormat>() == 2);
    switch (f) {
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

TextureFormat osc::Texture2D::texture_format() const
{
    return m_Impl->texture_format();
}

ColorSpace osc::Texture2D::getColorSpace() const
{
    return m_Impl->getColorSpace();
}

TextureWrapMode osc::Texture2D::wrap_mode() const
{
    return m_Impl->wrap_mode();
}

void osc::Texture2D::set_wrap_mode(TextureWrapMode twm)
{
    m_Impl.upd()->set_wrap_mode(twm);
}

TextureWrapMode osc::Texture2D::wrap_mode_u() const
{
    return m_Impl->get_wrap_mode_u();
}

void osc::Texture2D::set_wrap_mode_u(TextureWrapMode twm)
{
    m_Impl.upd()->set_wrap_mode_u(twm);
}

TextureWrapMode osc::Texture2D::wrap_mode_v() const
{
    return m_Impl->get_wrap_mode_v();
}

void osc::Texture2D::set_wrap_mode_v(TextureWrapMode twm)
{
    m_Impl.upd()->set_wrap_mode_v(twm);
}

TextureWrapMode osc::Texture2D::wrap_mode_w() const
{
    return m_Impl->wrap_mode_w();
}

void osc::Texture2D::set_wrap_mode_w(TextureWrapMode twm)
{
    m_Impl.upd()->set_wrap_mode_w(twm);
}

TextureFilterMode osc::Texture2D::filter_mode() const
{
    return m_Impl->filter_mode();
}

void osc::Texture2D::set_filter_mode(TextureFilterMode twm)
{
    m_Impl.upd()->set_filter_mode(twm);
}

std::vector<Color> osc::Texture2D::getPixels() const
{
    return m_Impl->getPixels();
}

void osc::Texture2D::setPixels(std::span<const Color> pixels)
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

std::span<const uint8_t> osc::Texture2D::getPixelData() const
{
    return m_Impl->getPixelData();
}

void osc::Texture2D::set_pixel_data(std::span<const uint8_t> pixelData)
{
    m_Impl.upd()->set_pixel_data(pixelData);
}

std::ostream& osc::operator<<(std::ostream& o, const Texture2D&)
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
    static_assert(c_RenderTextureFormatStrings.size() == num_options<RenderTextureFormat>());

    constexpr auto c_DepthStencilFormatStrings = std::to_array<CStringView>({
        "D24_UNorm_S8_UInt",
    });
    static_assert(c_DepthStencilFormatStrings.size() == num_options<DepthStencilFormat>());

    GLenum toInternalOpenGLColorFormat(
        RenderBufferType type,
        const RenderTextureDescriptor& desc)
    {
        static_assert(num_options<RenderBufferType>() == 2, "review code below, which treats RenderBufferType as a bool");
        if (type == RenderBufferType::Depth) {
            return GL_DEPTH24_STENCIL8;
        }
        else {
            static_assert(num_options<RenderTextureFormat>() == 6);
            static_assert(num_options<RenderTextureReadWrite>() == 2);

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

    constexpr CPUImageFormat toEquivalentCPUImageFormat(
        RenderBufferType type,
        const RenderTextureDescriptor& desc)
    {
        static_assert(num_options<RenderBufferType>() == 2);
        static_assert(num_options<DepthStencilFormat>() == 1);
        static_assert(num_options<RenderTextureFormat>() == 6);
        static_assert(num_options<CPUImageFormat>() == 5);

        if (type == RenderBufferType::Depth) {
            return CPUImageFormat::DepthStencil;
        }
        else {
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

    constexpr CPUDataType toEquivalentCPUDataType(
        RenderBufferType type,
        const RenderTextureDescriptor& desc)
    {
        static_assert(num_options<RenderBufferType>() == 2);
        static_assert(num_options<DepthStencilFormat>() == 1);
        static_assert(num_options<RenderTextureFormat>() == 6);
        static_assert(num_options<CPUDataType>() == 4);

        if (type == RenderBufferType::Depth) {
            return CPUDataType::UnsignedInt24_8;
        }
        else {
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

    constexpr GLenum toImageColorFormat(TextureFormat f)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::image_color_format... });
        }(TextureFormatList{});

        return lut.at(ToIndex(f));
    }

    constexpr GLint toImagePixelPackAlignment(TextureFormat f)
    {
        constexpr auto lut = []<TextureFormat... Formats>(OptionList<TextureFormat, Formats...>)
        {
            return std::to_array({ TextureFormatOpenGLTraits<Formats>::pixel_pack_alignment... });
        }(TextureFormatList{});

        return lut.at(ToIndex(f));
    }

    constexpr GLenum toImageDataType(TextureFormat)
    {
        static_assert(num_options<TextureFormat>() == 7);
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

std::ostream& osc::operator<<(std::ostream& o, const RenderTextureDescriptor& rtd)
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
        const RenderTextureDescriptor& descriptor_,
        RenderBufferType type_) :

        m_Descriptor{descriptor_},
        m_BufferType{type_}
    {
        OSC_ASSERT((getDimensionality() != TextureDimensionality::Cube || getDimensions().x == getDimensions().y) && "cannot construct a Cube renderbuffer with non-square dimensions");
        OSC_ASSERT((getDimensionality() != TextureDimensionality::Cube || getAntialiasingLevel() == AntiAliasingLevel::none()) && "cannot construct a Cube renderbuffer that is anti-aliased (not supported by backends like OpenGL)");
    }

    void reformat(const RenderTextureDescriptor& newDescriptor)
    {
        OSC_ASSERT((newDescriptor.getDimensionality() != TextureDimensionality::Cube || newDescriptor.getDimensions().x == newDescriptor.getDimensions().y) && "cannot reformat a render buffer to a Cube dimensionality with non-square dimensions");
        OSC_ASSERT((newDescriptor.getDimensionality() != TextureDimensionality::Cube || newDescriptor.getAntialiasingLevel() == AntiAliasingLevel::none()) && "cannot reformat a renderbuffer to a Cube dimensionality with is anti-aliased (not supported by backends like OpenGL)");

        if (m_Descriptor != newDescriptor) {
            m_Descriptor = newDescriptor;
            m_MaybeOpenGLData->reset();
        }
    }

    const RenderTextureDescriptor& getDescriptor() const
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

        static_assert(num_options<TextureDimensionality>() == 2);
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
        const Vec2i dimensions = m_Descriptor.getDimensions();

        // setup resolved texture
        gl::bind_texture(t.texture2D);
        gl::tex_image2D(
            GL_TEXTURE_2D,
            0,
            toInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
            dimensions.x,
            dimensions.y,
            0,
            toOpenGLFormat(toEquivalentCPUImageFormat(m_BufferType, m_Descriptor)),
            toOpenGLDataType(toEquivalentCPUDataType(m_BufferType, m_Descriptor)),
            nullptr
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE
        );
        gl::bind_texture();
    }

    void configureData(MultisampledRBOAndResolvedTexture& data)
    {
        const Vec2i dimensions = m_Descriptor.getDimensions();

        // setup multisampled RBO
        gl::bind_renderbuffer(data.multisampled_rbo);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            m_Descriptor.getAntialiasingLevel().get_as<GLsizei>(),
            toInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
            dimensions.x,
            dimensions.y
        );
        gl::bind_renderbuffer();

        // setup resolved texture
        gl::bind_texture(data.single_sampled_texture2D);
        gl::tex_image2D(
            GL_TEXTURE_2D,
            0,
            toInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
            dimensions.x,
            dimensions.y,
            0,
            toOpenGLFormat(toEquivalentCPUImageFormat(m_BufferType, m_Descriptor)),
            toOpenGLDataType(toEquivalentCPUDataType(m_BufferType, m_Descriptor)),
            nullptr
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );
        gl::tex_parameter_i(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_R,
            GL_CLAMP_TO_EDGE
        );
        gl::bind_texture();
    }

    void configureData(SingleSampledCubemap& t)
    {
        const Vec2i dimensions = m_Descriptor.getDimensions();

        // setup resolved texture
        gl::bind_texture(t.cubemap);
        for (int i = 0; i < 6; ++i)
        {
            gl::tex_image2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                toInternalOpenGLColorFormat(m_BufferType, m_Descriptor),
                dimensions.x,
                dimensions.y,
                0,
                toOpenGLFormat(toEquivalentCPUImageFormat(m_BufferType, m_Descriptor)),
                toOpenGLDataType(toEquivalentCPUDataType(m_BufferType, m_Descriptor)),
                nullptr
            );
        }
        gl::tex_parameter_i(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );
        gl::tex_parameter_i(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );
        gl::tex_parameter_i(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );
        gl::tex_parameter_i(
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );
        gl::tex_parameter_i(
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
    const RenderTextureDescriptor& descriptor_,
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

    explicit Impl(const RenderTextureDescriptor& descriptor) :
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

osc::RenderTexture::RenderTexture(const RenderTextureDescriptor& desc) :
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

void osc::RenderTexture::reformat(const RenderTextureDescriptor& d)
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

std::ostream& osc::operator<<(std::ostream& o, const RenderTexture&)
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
        m_Program{gl::create_program_from(gl::compile_from_source<gl::VertexShader>(vertexShader.c_str()), gl::compile_from_source<gl::FragmentShader>(fragmentShader.c_str()))}
    {
        parseUniformsAndAttributesFromProgram();
    }

    Impl(CStringView vertexShader, CStringView geometryShader, CStringView fragmentShader) :
        m_Program{gl::create_program_from(gl::compile_from_source<gl::VertexShader>(vertexShader.c_str()), gl::compile_from_source<gl::FragmentShader>(fragmentShader.c_str()), gl::compile_from_source<gl::GeometryShader>(geometryShader.c_str()))}
    {
        parseUniformsAndAttributesFromProgram();
    }

    size_t getPropertyCount() const
    {
        return m_Uniforms.size();
    }

    std::optional<ptrdiff_t> findPropertyIndex(std::string_view propertyName) const
    {
        if (const auto it = m_Uniforms.find(propertyName); it != m_Uniforms.end()) {
            return static_cast<ptrdiff_t>(std::distance(m_Uniforms.begin(), it));
        }
        else {
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
        return it->second.shader_type;
    }

    // non-PIMPL APIs

    const gl::Program& getProgram() const
    {
        return m_Program;
    }

    const FastStringHashtable<ShaderElement>& getUniforms() const
    {
        return m_Uniforms;
    }

    const FastStringHashtable<ShaderElement>& getAttributes() const
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
                normalize_shader_element_name(name.data()),
                static_cast<int32_t>(glGetAttribLocation(m_Program.get(), name.data())),
                opengl_shader_type_to_osc_shader_type(type),
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
                normalize_shader_element_name(name.data()),
                static_cast<int32_t>(glGetUniformLocation(m_Program.get(), name.data())),
                opengl_shader_type_to_osc_shader_type(type),
                static_cast<int32_t>(size)
            );
        }

        // cache commonly-used "automatic" shader elements
        //
        // it's a perf optimization: the renderer uses this to skip lookups
        if (const ShaderElement* e = tryGetValue(m_Uniforms, "uModelMat")) {
            m_MaybeModelMatUniform = *e;
        }
        if (const ShaderElement* e = tryGetValue(m_Uniforms, "uNormalMat")) {
            m_MaybeNormalMatUniform = *e;
        }
        if (const ShaderElement* e = tryGetValue(m_Uniforms, "uViewMat")) {
            m_MaybeViewMatUniform = *e;
        }
        if (const ShaderElement* e = tryGetValue(m_Uniforms, "uProjMat")) {
            m_MaybeProjMatUniform = *e;
        }
        if (const ShaderElement* e = tryGetValue(m_Uniforms, "uViewProjMat")) {
            m_MaybeViewProjMatUniform = *e;
        }
        if (const ShaderElement* e = tryGetValue(m_Attributes, "aModelMat")) {
            m_MaybeInstancedModelMatAttr = *e;
        }
        if (const ShaderElement* e = tryGetValue(m_Attributes, "aNormalMat")) {
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


std::ostream& osc::operator<<(std::ostream& o, ShaderPropertyType shader_type)
{
    constexpr auto lut = []<ShaderPropertyType... Types>(OptionList<ShaderPropertyType, Types...>)
    {
        return std::to_array({ ShaderPropertyTypeTraits<Types>::name... });
    }(ShaderPropertyTypeList{});

    return o << lut.at(ToIndex(shader_type));
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

std::ostream& osc::operator<<(std::ostream& o, const Shader& shader)
{
    o << "Shader(\n";
    {
        o << "    uniforms = [";

        const std::string_view delim = "\n        ";
        for (const auto& [name, data] : shader.m_Impl->getUniforms()) {
            o << delim;
            print_shader_element(o, name, data);
        }

        o << "\n    ],\n";
    }

    {
        o << "    attributes = [";

        const std::string_view delim = "\n        ";
        for (const auto& [name, data] : shader.m_Impl->getAttributes()) {
            o << delim;
            print_shader_element(o, name, data);
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
    GLenum toGLDepthFunc(DepthFunction f)
    {
        static_assert(num_options<DepthFunction>() == 2);

        switch (f) {
        case DepthFunction::LessOrEqual:
            return GL_LEQUAL;
        case DepthFunction::Less:
        default:
            return GL_LESS;
        }
    }

    GLenum toGLCullFaceEnum(CullMode cullMode)
    {
        static_assert(num_options<CullMode>() == 3);

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
    {}

    const Shader& getShader() const
    {
        return m_Shader;
    }

    std::optional<Color> getColor(std::string_view propertyName) const
    {
        return getValue<Color>(propertyName);
    }

    void setColor(std::string_view propertyName, const Color& color)
    {
        setValue(propertyName, color);
    }

    std::optional<std::span<const Color>> getColorArray(std::string_view propertyName) const
    {
        return getValue<std::vector<Color>, std::span<const Color>>(propertyName);
    }

    void setColorArray(std::string_view propertyName, std::span<const Color> colors)
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

    std::optional<std::span<const float>> getFloatArray(std::string_view propertyName) const
    {
        return getValue<std::vector<float>, std::span<const float>>(propertyName);
    }

    void setFloatArray(std::string_view propertyName, std::span<const float> v)
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

    std::optional<std::span<const Vec3>> getVec3Array(std::string_view propertyName) const
    {
        return getValue<std::vector<Vec3>, std::span<const Vec3>>(propertyName);
    }

    void setVec3Array(std::string_view propertyName, std::span<const Vec3> value)
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

    void setMat3(std::string_view propertyName, const Mat3& value)
    {
        setValue(propertyName, value);
    }

    std::optional<Mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<Mat4>(propertyName);
    }

    void setMat4(std::string_view propertyName, const Mat4& value)
    {
        setValue(propertyName, value);
    }

    std::optional<std::span<const Mat4>> getMat4Array(std::string_view propertyName) const
    {
        return getValue<std::vector<Mat4>, std::span<const Mat4>>(propertyName);
    }

    void setMat4Array(std::string_view propertyName, std::span<const Mat4> mats)
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
    requires std::convertible_to<T, TConverted>
    std::optional<TConverted> getValue(std::string_view propertyName) const
    {
        const auto* value = try_find(m_Values, propertyName);

        if (!value) {
            return std::nullopt;
        }
        if (!std::holds_alternative<T>(*value))
        {
            return std::nullopt;
        }
        return TConverted{std::get<T>(*value)};
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

const Shader& osc::Material::getShader() const
{
    return m_Impl->getShader();
}

std::optional<Color> osc::Material::getColor(std::string_view propertyName) const
{
    return m_Impl->getColor(propertyName);
}

void osc::Material::setColor(std::string_view propertyName, const Color& color)
{
    m_Impl.upd()->setColor(propertyName, color);
}

std::optional<std::span<const Color>> osc::Material::getColorArray(std::string_view propertyName) const
{
    return m_Impl->getColorArray(propertyName);
}

void osc::Material::setColorArray(std::string_view propertyName, std::span<const Color> colors)
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

std::optional<std::span<const float>> osc::Material::getFloatArray(std::string_view propertyName) const
{
    return m_Impl->getFloatArray(propertyName);
}

void osc::Material::setFloatArray(std::string_view propertyName, std::span<const float> vs)
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

std::optional<std::span<const Vec3>> osc::Material::getVec3Array(std::string_view propertyName) const
{
    return m_Impl->getVec3Array(propertyName);
}

void osc::Material::setVec3Array(std::string_view propertyName, std::span<const Vec3> vs)
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

void osc::Material::setMat3(std::string_view propertyName, const Mat3& mat)
{
    m_Impl.upd()->setMat3(propertyName, mat);
}

std::optional<Mat4> osc::Material::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(propertyName);
}

void osc::Material::setMat4(std::string_view propertyName, const Mat4& mat)
{
    m_Impl.upd()->setMat4(propertyName, mat);
}

std::optional<std::span<const Mat4>> osc::Material::getMat4Array(std::string_view propertyName) const
{
    return m_Impl->getMat4Array(propertyName);
}

void osc::Material::setMat4Array(std::string_view propertyName, std::span<const Mat4> mats)
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

std::ostream& osc::operator<<(std::ostream& o, const Material&)
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

    void setMat3(std::string_view propertyName, const Mat3& value)
    {
        setValue(propertyName, value);
    }

    std::optional<Mat4> getMat4(std::string_view propertyName) const
    {
        return getValue<Mat4>(propertyName);
    }

    void setMat4(std::string_view propertyName, const Mat4& value)
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

    friend bool operator==(const Impl&, const Impl&) = default;

private:
    template<typename T>
    std::optional<T> getValue(std::string_view propertyName) const
    {
        const auto it = m_Values.find(propertyName);

        if (it == m_Values.end()) {
            return std::nullopt;
        }

        if (!std::holds_alternative<T>(it->second)) {
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
        static const CopyOnUpdPtr<Impl> s_EmptyPropertyBlockImpl = make_cow<Impl>();
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

void osc::MaterialPropertyBlock::setColor(std::string_view propertyName, const Color& color)
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

void osc::MaterialPropertyBlock::setMat3(std::string_view propertyName, const Mat3& value)
{
    m_Impl.upd()->setMat3(propertyName, value);
}

std::optional<Mat4> osc::MaterialPropertyBlock::getMat4(std::string_view propertyName) const
{
    return m_Impl->getMat4(propertyName);
}

void osc::MaterialPropertyBlock::setMat4(std::string_view propertyName, const Mat4& value)
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

bool osc::operator==(const MaterialPropertyBlock& lhs, const MaterialPropertyBlock& rhs)
{
    return lhs.m_Impl == rhs.m_Impl || *lhs.m_Impl == *rhs.m_Impl;
}

std::ostream& osc::operator<<(std::ostream& o, const MaterialPropertyBlock&)
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
    static_assert(c_MeshTopologyStrings.size() == num_options<MeshTopology>());

    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
    static_assert(alignof(PackedIndex) == alignof(uint32_t));

    GLenum toOpenGLTopology(MeshTopology t)
    {
        static_assert(num_options<MeshTopology>() == 2);

        switch (t) {
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

    // low-level single-component decode/encode functions
    template<VertexBufferComponent EncodedValue, typename DecodedValue>
    DecodedValue decode(const std::byte*);

    template<typename DecodedValue, VertexBufferComponent EncodedValue>
    void encode(std::byte*, DecodedValue);

    template<>
    float decode<float, float>(const std::byte* p)
    {
        return *std::launder(reinterpret_cast<const float*>(p));
    }

    template<>
    void encode<float, float>(std::byte* p, float v)
    {
        *std::launder(reinterpret_cast<float*>(p)) = v;
    }

    template<>
    float decode<Unorm8, float>(const std::byte* p)
    {
        return Unorm8{*p}.normalized_value();
    }

    template<>
    void encode<float, Unorm8>(std::byte* p, float v)
    {
        *p = Unorm8{v}.byte();
    }

    template<>
    Unorm8 decode<Unorm8, Unorm8>(const std::byte* p)
    {
        return Unorm8{*p};
    }

    template<>
    void encode<Unorm8, Unorm8>(std::byte* p, Unorm8 v)
    {
        *p = v.byte();
    }

    // mid-level multi-component decode/encode functions
    template<UserFacingVertexData T, VertexAttributeFormat EncodingFormat>
    void encodeMany(std::byte* p, const T& v)
    {
        using ComponentType = typename VertexAttributeFormatTraits<EncodingFormat>::component_type;
        constexpr auto nComponents = num_components_in(EncodingFormat);
        constexpr auto sizeOfComponent = component_size(EncodingFormat);
        constexpr auto n = min(std::tuple_size_v<T>, static_cast<typename T::size_type>(nComponents));

        for (typename T::size_type i = 0; i < n; ++i) {
            encode<typename T::value_type, ComponentType>(p + i*sizeOfComponent, v[i]);
        }
    }

    template<VertexAttributeFormat EncodingFormat, UserFacingVertexData T>
    T DecodeMany(const std::byte* p)
    {
        using ComponentType = typename VertexAttributeFormatTraits<EncodingFormat>::component_type;
        constexpr auto nComponents = num_components_in(EncodingFormat);
        constexpr auto sizeOfComponent = component_size(EncodingFormat);
        constexpr auto n = min(std::tuple_size_v<T>, static_cast<typename T::size_type>(nComponents));

        T rv{};
        for (typename T::size_type i = 0; i < n; ++i)
        {
            rv[i] = decode<ComponentType, typename T::value_type>(p + i*sizeOfComponent);
        }
        return rv;
    }

    // high-level, compile-time multi-component decode + encode definition
    template<UserFacingVertexData T>
    class MultiComponentEncoding final {
    public:
        explicit MultiComponentEncoding(VertexAttributeFormat f)
        {
            static_assert(num_options<VertexAttributeFormat>() == 4);

            switch (f) {
            case VertexAttributeFormat::Float32x2:
                m_Encoder = encodeMany<T, VertexAttributeFormat::Float32x2>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Float32x2, T>;
                break;
            case VertexAttributeFormat::Float32x3:
                m_Encoder = encodeMany<T, VertexAttributeFormat::Float32x3>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Float32x3, T>;
                break;
            default:
            case VertexAttributeFormat::Float32x4:
                m_Encoder = encodeMany<T, VertexAttributeFormat::Float32x4>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Float32x4, T>;
                break;
            case VertexAttributeFormat::Unorm8x4:
                m_Encoder = encodeMany<T, VertexAttributeFormat::Unorm8x4>;
                m_Decoder = DecodeMany<VertexAttributeFormat::Unorm8x4, T>;
                break;
            }
        }

        void encode(std::byte* b, const T& v) const
        {
            m_Encoder(b, v);
        }

        T decode(const std::byte* b) const
        {
            return m_Decoder(b);
        }

        friend bool operator==(const MultiComponentEncoding&, const MultiComponentEncoding&) = default;
    private:
        using Encoder = void(*)(std::byte*, const T&);
        Encoder m_Encoder;

        using Decoder = T(*)(const std::byte*);
        Decoder m_Decoder;
    };

    // a single compile-time reencoding function
    //
    // decodes in-memory data in a source format, converts it to a desination format, and then
    // writes it to the destination memory
    template<VertexAttributeFormat SourceFormat, VertexAttributeFormat DestinationFormat>
    void reencode(std::span<const std::byte> src, std::span<std::byte> dest)
    {
        using SourceCPUFormat = typename VertexAttributeFormatTraits<SourceFormat>::type;
        using DestCPUFormat = typename VertexAttributeFormatTraits<DestinationFormat>::type;
        constexpr auto n = min(std::tuple_size_v<SourceCPUFormat>, std::tuple_size_v<DestCPUFormat>);

        const auto decoded = DecodeMany<SourceFormat, SourceCPUFormat>(src.data());
        DestCPUFormat converted{};
        for (size_t i = 0; i < n; ++i) {
            converted[i] = typename DestCPUFormat::value_type{decoded[i]};
        }
        encodeMany<DestCPUFormat, DestinationFormat>(dest.data(), converted);
    }

    // type-erased (i.e. runtime) reencoder function
    using ReencoderFunction = void(*)(std::span<const std::byte>, std::span<std::byte>);

    // compile-time lookup table (LUT) for runtime reencoder functions
    class ReencoderLut final {
    private:
        static constexpr size_t indexOf(VertexAttributeFormat sourceFormat, VertexAttributeFormat destinationFormat)
        {
            return static_cast<size_t>(sourceFormat)*num_options<VertexAttributeFormat>() + static_cast<size_t>(destinationFormat);
        }

        template<VertexAttributeFormat... Formats>
        static constexpr void writeEntriesTopLevel(ReencoderLut& lut, OptionList<VertexAttributeFormat, Formats...>)
        {
            (writeEntries<Formats, Formats...>(lut), ...);
        }

        template<VertexAttributeFormat SourceFormat, VertexAttributeFormat... DestinationFormats>
        static constexpr void writeEntries(ReencoderLut& lut)
        {
            (writeEntry<SourceFormat, DestinationFormats>(lut), ...);
        }

        template<VertexAttributeFormat SourceFormat, VertexAttributeFormat DestinationFormat>
        static constexpr void writeEntry(ReencoderLut& lut)
        {
            lut.assign(SourceFormat, DestinationFormat, reencode<SourceFormat, DestinationFormat>);
        }
    public:
        constexpr ReencoderLut()
        {
            writeEntriesTopLevel(*this, VertexAttributeFormatList{});
        }

        constexpr void assign(VertexAttributeFormat sourceFormat, VertexAttributeFormat destinationFormat, ReencoderFunction f)
        {
            m_Storage.at(indexOf(sourceFormat, destinationFormat)) = f;
        }

        constexpr const ReencoderFunction& lookup(VertexAttributeFormat sourceFormat, VertexAttributeFormat destinationFormat) const
        {
            return m_Storage.at(indexOf(sourceFormat, destinationFormat));
        }

    private:
        std::array<ReencoderFunction, num_options<VertexAttributeFormat>()*num_options<VertexAttributeFormat>()> m_Storage{};
    };

    constexpr ReencoderLut c_ReencoderLUT;

    struct VertexBufferAttributeReencoder final {
        ReencoderFunction reencode;
        size_t sourceOffset;
        size_t sourceStride;
        size_t destintionOffset;
        size_t destinationStride;
    };

    std::vector<VertexBufferAttributeReencoder> getReencoders(const VertexFormat& srcFormat, const VertexFormat& destFormat)
    {
        std::vector<VertexBufferAttributeReencoder> rv;
        rv.reserve(destFormat.numAttributes());  // guess

        for (const auto destLayout : destFormat.attributeLayouts()) {
            if (const auto srcLayout = srcFormat.attributeLayout(destLayout.attribute())) {
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

    void reEncodeVertexBuffer(
        std::span<const std::byte> src,
        const VertexFormat& srcFormat,
        std::span<std::byte> dest,
        const VertexFormat& destFormat)
    {
        const size_t srcStride = srcFormat.stride();
        const size_t destStride = destFormat.stride();

        if (srcStride == 0 || destStride == 0) {
            return;  // no reencoding necessary
        }
        OSC_ASSERT(src.size() % srcStride == 0);
        OSC_ASSERT(dest.size() % destStride == 0);

        const size_t n = min(src.size() / srcStride, dest.size() / destStride);

        const auto reencoders = getReencoders(srcFormat, destFormat);
        for (size_t i = 0; i < n; ++i) {
            const auto srcData = src.subspan(i*srcStride);
            const auto destData = dest.subspan(i*destStride);

            for (const auto& reencoder : reencoders) {
                const auto srcAttrData = srcData.subspan(reencoder.sourceOffset, reencoder.sourceStride);
                const auto destAttrData = destData.subspan(reencoder.destintionOffset, reencoder.destinationStride);
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
            using Byte = std::conditional_t<IsConst, const std::byte, std::byte>;

            AttributeValueProxy(Byte* data_, MultiComponentEncoding<T> encoding_) :
                m_Data{data_},
                m_Encoding{encoding_}
            {}

            AttributeValueProxy& operator=(const T& v)
                requires (!IsConst)
            {
                m_Encoding.encode(m_Data, v);
                return *this;
            }

            operator T () const  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            {
                return m_Encoding.decode(m_Data);
            }

            template<typename U>
            requires (!IsConst)
            AttributeValueProxy& operator/=(const U& v)
            {
                return *this = (T{*this} /= v);
            }

            template<typename U>
            requires (!IsConst)
            AttributeValueProxy& operator+=(const U& v)
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

            using Byte = std::conditional_t<IsConst, const std::byte, std::byte>;

            AttributeValueIterator(
                Byte* data_,
                size_t stride_,
                MultiComponentEncoding<T> encoding_) :

                m_Data{data_},
                m_Stride{stride_},
                m_Encoding{encoding_}
            {}

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

            difference_type operator-(const AttributeValueIterator& rhs) const
            {
                return (m_Data - rhs.m_Data) / m_Stride;
            }

            AttributeValueProxy<T, IsConst> operator[](difference_type n) const
            {
                return *(*this + n);
            }

            bool operator<(const AttributeValueIterator& rhs) const
            {
                return m_Data < rhs.m_Data;
            }

            bool operator>(const AttributeValueIterator& rhs) const
            {
                return m_Data > rhs.m_Data;
            }

            bool operator<=(const AttributeValueIterator& rhs) const
            {
                return m_Data <= rhs.m_Data;
            }

            bool operator>=(const AttributeValueIterator& rhs) const
            {
                return m_Data >= rhs.m_Data;
            }

            friend bool operator==(const AttributeValueIterator&, const AttributeValueIterator&) = default;
        private:
            Byte* m_Data;
            size_t m_Stride;
            MultiComponentEncoding<T> m_Encoding;
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
                std::span<Byte> data_,
                size_t stride_,
                VertexAttributeFormat format_) :

                m_Data{data_},
                m_Stride{stride_},
                m_Encoding{format_}
            {}

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
                const auto beg = range.begin();
                if (i >= std::distance(beg, range.end())) {
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
        VertexBuffer(size_t numVerts, const VertexFormat& format) :
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

        std::span<const std::byte> bytes() const
        {
            return m_Data;
        }

        const VertexFormat& format() const
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
            if (const auto layout = m_VertexFormat.attributeLayout(attr)) {
                std::span<const std::byte> offsetSpan{m_Data.data() + layout->offset(), m_Data.size()};
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
            if (const auto layout = m_VertexFormat.attributeLayout(attr)) {
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
        void write(VertexAttribute attr, std::span<const T> els)
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
                newFormat.insert({attr, default_format(attr)});
                setParams(els.size(), newFormat);
            }
            else if (els.size() != numVerts())
            {
                // resize
                setParams(els.size(), m_VertexFormat);
            }

            // write els to vertex buffer
            copy(els.begin(), els.end(), iter<T>(attr).begin());
        }

        template<UserFacingVertexData T, typename UnaryOperation>
        requires std::invocable<UnaryOperation, T>
        void transformAttribute(VertexAttribute attr, UnaryOperation f)
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

        void setParams(size_t newNumVerts, const VertexFormat& newFormat)
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
                reEncodeVertexBuffer(m_Data, m_VertexFormat, newBuf, newFormat);
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

        void setFormat(const VertexFormat& newFormat)
        {
            setParams(numVerts(), newFormat);
        }

        void setData(std::span<const std::byte> newData)
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

    void setVerts(std::span<const Vec3> verts)
    {
        m_VertexBuffer.write<Vec3>(VertexAttribute::Position, verts);

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    void transformVerts(const std::function<Vec3(Vec3)>& f)
    {
        m_VertexBuffer.transformAttribute<Vec3>(VertexAttribute::Position, f);

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    void transformVerts(const Transform& t)
    {
        m_VertexBuffer.transformAttribute<Vec3>(VertexAttribute::Position, [&t](Vec3 v)
        {
            return t * v;
        });

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    void transformVerts(const Mat4& m)
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

    void setNormals(std::span<const Vec3> normals)
    {
        m_VertexBuffer.write<Vec3>(VertexAttribute::Normal, normals);

        m_Version->reset();
    }

    void transformNormals(const std::function<Vec3(Vec3)>& f)
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

    void setTexCoords(std::span<const Vec2> coords)
    {
        m_VertexBuffer.write<Vec2>(VertexAttribute::TexCoord0, coords);

        m_Version->reset();
    }

    void transformTexCoords(const std::function<Vec2(Vec2)>& f)
    {
        m_VertexBuffer.transformAttribute<Vec2>(VertexAttribute::TexCoord0, f);

        m_Version->reset();
    }

    std::vector<Color> getColors() const
    {
        return m_VertexBuffer.read<Color>(VertexAttribute::Color);
    }

    void setColors(std::span<const Color> colors)
    {
        m_VertexBuffer.write<Color>(VertexAttribute::Color, colors);

        m_Version.reset();
    }

    std::vector<Vec4> getTangents() const
    {
        return m_VertexBuffer.read<Vec4>(VertexAttribute::Tangent);
    }

    void setTangents(std::span<const Vec4> newTangents)
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

    void forEachIndexedVert(const std::function<void(Vec3)>& f) const
    {
        const auto positions = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position).begin();
        for (auto idx : getIndices()) {
            f(positions[idx]);
        }
    }

    void forEachIndexedTriangle(const std::function<void(Triangle)>& f) const
    {
        if (m_Topology != MeshTopology::Triangles) {
            return;
        }

        const MeshIndicesView indices = getIndices();
        const size_t steps = (indices.size() / 3) * 3;

        const auto positions = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position).begin();
        for (size_t i = 0; i < steps; i += 3) {
            f(Triangle{
                positions[indices[i]],
                positions[indices[i+1]],
                positions[indices[i+2]],
            });
        }
    }

    Triangle getTriangleAt(size_t firstIndexOffset) const
    {
        if (m_Topology != MeshTopology::Triangles) {
            throw std::runtime_error{"cannot call getTriangleAt on a non-triangular-topology mesh"};
        }

        const auto indices = getIndices();

        if (firstIndexOffset+2 >= indices.size()) {
            throw std::runtime_error{"provided first index offset is out-of-bounds"};
        }

        const auto verts = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);

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

    const AABB& getBounds() const
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

    void pushSubMeshDescriptor(const SubMeshDescriptor& desc)
    {
        m_SubMeshDescriptors.push_back(desc);
    }

    const SubMeshDescriptor& getSubMeshDescriptor(size_t i) const
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

    const VertexFormat& getVertexAttributes() const
    {
        return m_VertexBuffer.format();
    }

    void setVertexBufferParams(size_t newNumVerts, const VertexFormat& newFormat)
    {
        m_VertexBuffer.setParams(newNumVerts, newFormat);

        rangeCheckIndicesAndRecalculateBounds();
        m_Version->reset();
    }

    size_t getVertexBufferStride() const
    {
        return m_VertexBuffer.stride();
    }

    void setVertexBufferData(std::span<const uint8_t> newData, MeshUpdateFlags flags)
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

        const auto indices = getIndices();
        const auto positions = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);
        auto normals = m_VertexBuffer.iter<Vec3>(VertexAttribute::Normal);
        std::vector<uint16_t> counts(normals.size());

        for (size_t i = 0, len = 3*(indices.size()/3); i < len; i+=3) {
            // get triangle indices
            const Vec3uz idxs = {indices[i], indices[i+1], indices[i+2]};

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

        const auto vbverts = m_VertexBuffer.iter<Vec3>(VertexAttribute::Position);
        const auto vbnormals = m_VertexBuffer.iter<Vec3>(VertexAttribute::Normal);
        const auto vbtexcoords = m_VertexBuffer.iter<Vec2>(VertexAttribute::TexCoord0);

        const auto tangents = CalcTangentVectors(
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
        if (!*m_MaybeGPUBuffers || (*m_MaybeGPUBuffers)->data_version != *m_Version)
        {
            uploadToGPU();
        }
        return (*m_MaybeGPUBuffers)->vao;
    }

    void drawInstanced(
        size_t n,
        std::optional<size_t> maybe_submesh_index)
    {
        const SubMeshDescriptor descriptor = maybe_submesh_index ?
            m_SubMeshDescriptors.at(*maybe_submesh_index) :         // draw the requested sub-mesh
            SubMeshDescriptor{0, m_NumIndices, m_Topology};       // else: draw the entire mesh as a "sub mesh"

        // convert mesh/descriptor data types into OpenGL-compatible formats
        const GLenum mode = toOpenGLTopology(descriptor.getTopology());
        const auto count = static_cast<GLsizei>(descriptor.getIndexCount());
        const GLenum type = m_IndicesAre32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;

        const size_t bytesPerIndex = m_IndicesAre32Bit ? sizeof(GLint) : sizeof(GLshort);
        const size_t firstIndexByteOffset = descriptor.getIndexStart() * bytesPerIndex;
        const void* indices = cpp20::bit_cast<void*>(firstIndexByteOffset);

        const auto instanceCount = static_cast<GLsizei>(n);

        glDrawElementsInstanced(
            mode,
            count,
            type,
            indices,
            instanceCount
        );
    }

private:

    void setIndices(std::span<const uint16_t> indices, MeshUpdateFlags flags)
    {
        m_IndicesAre32Bit = false;
        m_NumIndices = indices.size();
        m_IndicesData.resize((indices.size()+1)/2);
        copy(indices, &m_IndicesData.front().u16.a);

        rangeCheckIndicesAndRecalculateBounds(flags);
        m_Version->reset();
    }

    void setIndices(std::span<const uint32_t> vs, MeshUpdateFlags flags)
    {
        const auto isGreaterThanU16Max = [](uint32_t v)
        {
            return v > std::numeric_limits<uint16_t>::max();
        };

        if (any_of(vs, isGreaterThanU16Max))
        {
            m_IndicesAre32Bit = true;
            m_NumIndices = vs.size();
            m_IndicesData.resize(vs.size());
            copy(vs, &m_IndicesData.front().u32);
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
        const bool checkIndices = !((flags & MeshUpdateFlags::DontValidateIndices) && (flags & MeshUpdateFlags::DontRecalculateBounds));

        //       ... but it's perfectly reasonable for the caller to only want the indices to be
        //       validated, leaving the bounds untouched
        const bool recalculateBounds = !(flags & MeshUpdateFlags::DontRecalculateBounds);

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

    static GLint GetVertexAttributeSize(const VertexAttributeFormat& format)
    {
        return static_cast<GLint>(num_components_in(format));
    }

    static GLenum GetVertexAttributeType(const VertexAttributeFormat& format)
    {
        static_assert(num_options<VertexAttributeFormat>() == 4);

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

    static GLboolean GetVertexAttributeNormalized(const VertexAttributeFormat& format)
    {
        static_assert(num_options<VertexAttributeFormat>() == 4);

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

    static void OpenGLBindVertexAttribute(const VertexFormat& format, const VertexFormat::VertexAttributeLayout& layout)
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
        gl::bind_buffer(
            GL_ARRAY_BUFFER,
            buffers.array_buffer
        );
        gl::buffer_data(
            GL_ARRAY_BUFFER,
            static_cast<GLsizei>(m_VertexBuffer.bytes().size()),
            m_VertexBuffer.bytes().data(),
            GL_STATIC_DRAW
        );

        // upload CPU-side element data into the GPU-side buffer
        const size_t eboNumBytes = m_NumIndices * (m_IndicesAre32Bit ? sizeof(uint32_t) : sizeof(uint16_t));
        gl::bind_buffer(
            GL_ELEMENT_ARRAY_BUFFER,
            buffers.indices_buffer
        );
        gl::buffer_data(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizei>(eboNumBytes),
            m_IndicesData.data(),
            GL_STATIC_DRAW
        );

        // configure mesh-level VAO
        gl::bind_vertex_array(buffers.vao);
        gl::bind_buffer(GL_ARRAY_BUFFER, buffers.array_buffer);
        gl::bind_buffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indices_buffer);
        for (auto&& layout : m_VertexBuffer.attributeLayouts())
        {
            OpenGLBindVertexAttribute(m_VertexBuffer.format(), layout);
        }
        gl::bind_vertex_array();

        buffers.data_version = *m_Version;
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

void osc::Mesh::setVerts(std::span<const Vec3> verts)
{
    m_Impl.upd()->setVerts(verts);
}

void osc::Mesh::transformVerts(const std::function<Vec3(Vec3)>& f)
{
    m_Impl.upd()->transformVerts(f);
}

void osc::Mesh::transformVerts(const Transform& t)
{
    m_Impl.upd()->transformVerts(t);
}

void osc::Mesh::transformVerts(const Mat4& m)
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

void osc::Mesh::setNormals(std::span<const Vec3> verts)
{
    m_Impl.upd()->setNormals(verts);
}

void osc::Mesh::transformNormals(const std::function<Vec3(Vec3)>& f)
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

void osc::Mesh::setTexCoords(std::span<const Vec2> coords)
{
    m_Impl.upd()->setTexCoords(coords);
}

void osc::Mesh::transformTexCoords(const std::function<Vec2(Vec2)>& f)
{
    m_Impl.upd()->transformTexCoords(f);
}

std::vector<Color> osc::Mesh::getColors() const
{
    return m_Impl->getColors();
}

void osc::Mesh::setColors(std::span<const Color> colors)
{
    m_Impl.upd()->setColors(colors);
}

std::vector<Vec4> osc::Mesh::getTangents() const
{
    return m_Impl->getTangents();
}

void osc::Mesh::setTangents(std::span<const Vec4> newTangents)
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

void osc::Mesh::forEachIndexedVert(const std::function<void(Vec3)>& f) const
{
    m_Impl->forEachIndexedVert(f);
}

void osc::Mesh::forEachIndexedTriangle(const std::function<void(Triangle)>& f) const
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

const AABB& osc::Mesh::getBounds() const
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

void osc::Mesh::pushSubMeshDescriptor(const SubMeshDescriptor& desc)
{
    m_Impl.upd()->pushSubMeshDescriptor(desc);
}

const SubMeshDescriptor& osc::Mesh::getSubMeshDescriptor(size_t i) const
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

const VertexFormat& osc::Mesh::getVertexAttributes() const
{
    return m_Impl->getVertexAttributes();
}

void osc::Mesh::setVertexBufferParams(size_t n, const VertexFormat& format)
{
    m_Impl.upd()->setVertexBufferParams(n, format);
}

size_t osc::Mesh::getVertexBufferStride() const
{
    return m_Impl->getVertexBufferStride();
}

void osc::Mesh::setVertexBufferData(std::span<const uint8_t> data, MeshUpdateFlags flags)
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

std::ostream& osc::operator<<(std::ostream& o, const Mesh&)
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
    static_assert(c_CameraProjectionStrings.size() == num_options<CameraProjection>());
}

class osc::Camera::Impl final {
public:

    void reset()
    {
        Impl newImpl;
        std::swap(*this, newImpl);
        m_RenderQueue = std::move(newImpl.m_RenderQueue);
    }

    Color background_color() const
    {
        return m_BackgroundColor;
    }

    void set_background_color(const Color& color)
    {
        m_BackgroundColor = color;
    }

    CameraProjection camera_projection() const
    {
        return m_CameraProjection;
    }

    void set_camera_projection(CameraProjection projection)
    {
        m_CameraProjection = projection;
    }

    float orthographic_size() const
    {
        return m_OrthographicSize;
    }

    void set_orthographic_size(float size)
    {
        m_OrthographicSize = size;
    }

    Radians vertical_fov() const
    {
        return m_PerspectiveFov;
    }

    void set_vertical_fov(Radians size)
    {
        m_PerspectiveFov = size;
    }

    float near_clipping_plane() const
    {
        return m_NearClippingPlane;
    }

    void set_near_clipping_plane(float distance)
    {
        m_NearClippingPlane = distance;
    }

    float get_far_clipping_plane() const
    {
        return m_FarClippingPlane;
    }

    void set_far_clipping_plane(float distance)
    {
        m_FarClippingPlane = distance;
    }

    CameraClearFlags clear_flags() const
    {
        return m_ClearFlags;
    }

    void set_clear_flags(CameraClearFlags flags)
    {
        m_ClearFlags = flags;
    }

    std::optional<Rect> pixel_rect() const
    {
        return m_MaybeScreenPixelRect;
    }

    void set_pixel_rect(std::optional<Rect> maybePixelRect)
    {
        m_MaybeScreenPixelRect = maybePixelRect;
    }

    std::optional<Rect> scissor_rect() const
    {
        return m_MaybeScissorRect;
    }

    void set_scissor_rect(std::optional<Rect> maybeScissorRect)
    {
        m_MaybeScissorRect = maybeScissorRect;
    }

    Vec3 position() const
    {
        return m_Position;
    }

    void set_position(const Vec3& position)
    {
        m_Position = position;
    }

    Quat rotation() const
    {
        return m_Rotation;
    }

    void set_rotation(const Quat& rotation)
    {
        m_Rotation = rotation;
    }

    Vec3 direction() const
    {
        return m_Rotation * Vec3{0.0f, 0.0f, -1.0f};
    }

    void set_direction(const Vec3& d)
    {
        m_Rotation = osc::rotation(Vec3{0.0f, 0.0f, -1.0f}, d);
    }

    Vec3 upwards_direction() const
    {
        return m_Rotation * Vec3{0.0f, 1.0f, 0.0f};
    }

    Mat4 view_matrix() const
    {
        if (m_MaybeViewMatrixOverride)
        {
            return *m_MaybeViewMatrixOverride;
        }
        else
        {
            return look_at(m_Position, m_Position + direction(), upwards_direction());
        }
    }

    std::optional<Mat4> view_matrix_override() const
    {
        return m_MaybeViewMatrixOverride;
    }

    void set_view_matrix_override(std::optional<Mat4> maybeViewMatrixOverride)
    {
        m_MaybeViewMatrixOverride = maybeViewMatrixOverride;
    }

    Mat4 projection_matrix(float aspectRatio) const
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
            const float height = m_OrthographicSize;
            const float width = height * aspectRatio;

            const float right = 0.5f * width;
            const float left = -right;
            const float top = 0.5f * height;
            const float bottom = -top;

            return ortho(left, right, bottom, top, m_NearClippingPlane, m_FarClippingPlane);
        }
    }

    std::optional<Mat4> projection_matrix_override() const
    {
        return m_MaybeProjectionMatrixOverride;
    }

    void set_projection_matrix_override(std::optional<Mat4> maybeProjectionMatrixOverride)
    {
        m_MaybeProjectionMatrixOverride = maybeProjectionMatrixOverride;
    }

    Mat4 view_projection_matrix(float aspectRatio) const
    {
        return projection_matrix(aspectRatio) * view_matrix();
    }

    Mat4 inverse_view_projection_matrix(float aspectRatio) const
    {
        return inverse(view_projection_matrix(aspectRatio));
    }

    void render_to_screen()
    {
        GraphicsBackend::renderCameraQueue(*this);
    }

    void render_to(RenderTexture& renderTexture)
    {
        static_assert(CameraClearFlags::All == (CameraClearFlags::SolidColor | CameraClearFlags::Depth));
        static_assert(num_options<RenderTextureReadWrite>() == 2);

        RenderTarget renderTargetThatWritesToRenderTexture
        {
            {
                RenderTargetColorAttachment
                {
                    // attach to render texture's color buffer
                    renderTexture.updColorBuffer(),

                    // load the color buffer based on this camera's clear flags
                    clear_flags() & CameraClearFlags::SolidColor ?
                        RenderBufferLoadAction::Clear :
                        RenderBufferLoadAction::Load,

                    RenderBufferStoreAction::Resolve,

                    // ensure clear color matches colorspace of render texture
                    renderTexture.getReadWrite() == RenderTextureReadWrite::sRGB ?
                        to_linear_colorspace(background_color()) :
                        background_color(),
                },
            },
            RenderTargetDepthAttachment
            {
                // attach to the render texture's depth buffer
                renderTexture.updDepthBuffer(),

                // load the depth buffer based on this camera's clear flags
                clear_flags() & CameraClearFlags::Depth ?
                    RenderBufferLoadAction::Clear :
                    RenderBufferLoadAction::Load,

                RenderBufferStoreAction::DontCare,
            },
        };

        render_to(renderTargetThatWritesToRenderTexture);
    }

    void render_to(RenderTarget& renderTarget)
    {
        GraphicsBackend::renderCameraQueue(*this, &renderTarget);
    }

    friend bool operator==(const Impl&, const Impl&) = default;

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
    impl_{make_cow<Impl>()}
{
}

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

CameraProjection osc::Camera::camera_projection() const
{
    return impl_->camera_projection();
}

void osc::Camera::set_camera_projection(CameraProjection projection)
{
    impl_.upd()->set_camera_projection(projection);
}

float osc::Camera::orthographic_size() const
{
    return impl_->orthographic_size();
}

void osc::Camera::set_orthographic_size(float sz)
{
    impl_.upd()->set_orthographic_size(sz);
}

Radians osc::Camera::vertical_fov() const
{
    return impl_->vertical_fov();
}

void osc::Camera::set_vertical_fov(Radians vertical_fov)
{
    impl_.upd()->set_vertical_fov(vertical_fov);
}

float osc::Camera::near_clipping_plane() const
{
    return impl_->near_clipping_plane();
}

void osc::Camera::set_near_clipping_plane(float d)
{
    impl_.upd()->set_near_clipping_plane(d);
}

float osc::Camera::get_far_clipping_plane() const
{
    return impl_->get_far_clipping_plane();
}

void osc::Camera::set_far_clipping_plane(float d)
{
    impl_.upd()->set_far_clipping_plane(d);
}

CameraClearFlags osc::Camera::clear_flags() const
{
    return impl_->clear_flags();
}

void osc::Camera::set_clear_flags(CameraClearFlags flags)
{
    impl_.upd()->set_clear_flags(flags);
}

std::optional<Rect> osc::Camera::pixel_rect() const
{
    return impl_->pixel_rect();
}

void osc::Camera::set_pixel_rect(std::optional<Rect> maybePixelRect)
{
    impl_.upd()->set_pixel_rect(maybePixelRect);
}

std::optional<Rect> osc::Camera::scissor_rect() const
{
    return impl_->scissor_rect();
}

void osc::Camera::set_scissor_rect(std::optional<Rect> maybeScissorRect)
{
    impl_.upd()->set_scissor_rect(maybeScissorRect);
}

Vec3 osc::Camera::position() const
{
    return impl_->position();
}

void osc::Camera::set_position(const Vec3& p)
{
    impl_.upd()->set_position(p);
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

void osc::Camera::set_direction(const Vec3& d)
{
    impl_.upd()->set_direction(d);
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

void osc::Camera::set_view_matrix_override(std::optional<Mat4> maybeViewMatrixOverride)
{
    impl_.upd()->set_view_matrix_override(maybeViewMatrixOverride);
}

Mat4 osc::Camera::projection_matrix(float aspectRatio) const
{
    return impl_->projection_matrix(aspectRatio);
}

std::optional<Mat4> osc::Camera::projection_matrix_override() const
{
    return impl_->projection_matrix_override();
}

void osc::Camera::set_projection_matrix_override(std::optional<Mat4> maybeProjectionMatrixOverride)
{
    impl_.upd()->set_projection_matrix_override(maybeProjectionMatrixOverride);
}

Mat4 osc::Camera::view_projection_matrix(float aspectRatio) const
{
    return impl_->view_projection_matrix(aspectRatio);
}

Mat4 osc::Camera::inverse_view_projection_matrix(float aspectRatio) const
{
    return impl_->inverse_view_projection_matrix(aspectRatio);
}

void osc::Camera::render_to_screen()
{
    impl_.upd()->render_to_screen();
}

void osc::Camera::render_to(RenderTexture& renderTexture)
{
    impl_.upd()->render_to(renderTexture);
}

void osc::Camera::render_to(RenderTarget& renderTarget)
{
    impl_.upd()->render_to(renderTarget);
}

std::ostream& osc::operator<<(std::ostream& o, const Camera& camera)
{
    return o << "Camera(position = " << camera.position() << ", direction = " << camera.direction() << ", projection = " << camera.camera_projection() << ')';
}

bool osc::operator==(const Camera& lhs, const Camera& rhs)
{
    return lhs.impl_ == rhs.impl_ || *lhs.impl_ == *rhs.impl_;
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

        for (const auto& capability : c_RequiredOpenGLCapabilities) {
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
    AntiAliasingLevel GetOpenGLMaxMSXAASamples(const sdl::GLContext&)
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
        const void*)
    {
        const LogLevel lvl = OpenGLDebugSevToLogLvl(severity);
        const CStringView sourceCStr = OpenGLDebugSrcToStrView(source);
        const CStringView typeCStr = OpenGLDebugTypeToStrView(type);
        const CStringView severityCStr = OpenGLDebugSevToStrView(severity);

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

    AntiAliasingLevel max_antialiasing_level() const
    {
        return m_MaxMSXAASamples;
    }

    bool is_vsync_enabled() const
    {
        return m_VSyncEnabled;
    }

    void enable_vsync()
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

    void disable_vsync()
    {
        SDL_GL_SetSwapInterval(0);
        m_VSyncEnabled = SDL_GL_GetSwapInterval() != 0;
    }

    bool is_in_debug_mode() const
    {
        return m_DebugModeEnabled;
    }

    void enable_debug_mode()
    {
        if (IsOpenGLInDebugMode())
        {
            return;  // already in debug mode
        }

        log_info("enabling debug mode");
        EnableOpenGLDebugMessages();
        m_DebugModeEnabled = IsOpenGLInDebugMode();
    }
    void disable_debug_mode()
    {
        if (!IsOpenGLInDebugMode())
        {
            return;  // already not in debug mode
        }

        log_info("disabling debug mode");
        DisableOpenGLDebugMessages();
        m_DebugModeEnabled = IsOpenGLInDebugMode();
    }

    void clear_screen(const Color& color)
    {
        // clear color is in sRGB, but the framebuffer is sRGB-corrected (GL_FRAMEBUFFER_SRGB)
        // and assumes that the given colors are in linear space
        const Color linearColor = to_linear_colorspace(color);

        gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, gl::window_framebuffer);
        gl::clear_color(linearColor.r, linearColor.g, linearColor.b, linearColor.a);
        gl::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void* upd_raw_opengl_context_handle_HACK()
    {
        return m_GLContext.get();
    }

    std::future<Texture2D> request_screenshot()
    {
        return m_ActiveScreenshotRequests.emplace_back().get_future();
    }

    void swap_buffers(SDL_Window& window)
    {
        // ensure window FBO is bound (see: SDL_GL_SwapWindow's note about MacOS requiring 0 is bound)
        gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);

        // flush outstanding screenshot requests
        if (!m_ActiveScreenshotRequests.empty())
        {
            // copy GPU-side window framebuffer into CPU-side `osc::Image` object
            const Vec2i dims = App::get().dims();

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

            Texture2D screenshot
            {
                dims,
                TextureFormat::RGBA32,
                ColorSpace::sRGB
            };
            screenshot.set_pixel_data(pixels);

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

    const Material& getQuadMaterial() const
    {
        return m_QuadMaterial;
    }

    const Mesh& getQuadMesh() const
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
            c_quad_vertex_shader_src,
            c_quad_fragment_shader_src,
        }
    };

    // a generic quad mesh: two triangles covering NDC @ Z=0
    Mesh m_QuadMesh = PlaneGeometry{2.0f, 2.0f, 1, 1};

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

AntiAliasingLevel osc::GraphicsContext::max_antialiasing_level() const
{
    return g_GraphicsContextImpl->max_antialiasing_level();
}

bool osc::GraphicsContext::is_vsync_enabled() const
{
    return g_GraphicsContextImpl->is_vsync_enabled();
}

void osc::GraphicsContext::enable_vsync()
{
    g_GraphicsContextImpl->enable_vsync();
}

void osc::GraphicsContext::disable_vsync()
{
    g_GraphicsContextImpl->disable_vsync();
}

bool osc::GraphicsContext::is_in_debug_mode() const
{
    return g_GraphicsContextImpl->is_in_debug_mode();
}

void osc::GraphicsContext::enable_debug_mode()
{
    g_GraphicsContextImpl->enable_debug_mode();
}

void osc::GraphicsContext::disable_debug_mode()
{
    g_GraphicsContextImpl->disable_debug_mode();
}

void osc::GraphicsContext::clear_screen(const Color& color)
{
    g_GraphicsContextImpl->clear_screen(color);
}

void* osc::GraphicsContext::upd_raw_opengl_context_handle_HACK()
{
    return g_GraphicsContextImpl->upd_raw_opengl_context_handle_HACK();
}

void osc::GraphicsContext::swap_buffers(SDL_Window& window)
{
    g_GraphicsContextImpl->swap_buffers(window);
}

std::future<Texture2D> osc::GraphicsContext::request_screenshot()
{
    return g_GraphicsContextImpl->request_screenshot();
}

std::string osc::GraphicsContext::backend_vendor_string() const
{
    return g_GraphicsContextImpl->backend_vendor_string();
}

std::string osc::GraphicsContext::backend_renderer_string() const
{
    return g_GraphicsContextImpl->backend_renderer_string();
}

std::string osc::GraphicsContext::backend_version_string() const
{
    return g_GraphicsContextImpl->backend_version_string();
}

std::string osc::GraphicsContext::backend_shading_language_version_string() const
{
    return g_GraphicsContextImpl->backend_shading_language_version_string();
}


/////////////////////////////
//
// drawing commands
//
/////////////////////////////

void osc::graphics::draw(
    const Mesh& mesh,
    const Transform& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybeMaterialPropertyBlock,
    std::optional<size_t> maybe_submesh_index)
{
    GraphicsBackend::draw(mesh, transform, material, camera, maybeMaterialPropertyBlock, maybe_submesh_index);
}

void osc::graphics::draw(
    const Mesh& mesh,
    const Mat4& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybeMaterialPropertyBlock,
    std::optional<size_t> maybe_submesh_index)
{
    GraphicsBackend::draw(mesh, transform, material, camera, maybeMaterialPropertyBlock, maybe_submesh_index);
}

void osc::graphics::blit(const Texture2D& source, RenderTexture& dest)
{
    GraphicsBackend::blit(source, dest);
}

void osc::graphics::blit_to_screen(
    const RenderTexture& t,
    const Rect& rect,
    BlitFlags flags)
{
    GraphicsBackend::blitToScreen(t, rect, flags);
}

void osc::graphics::blit_to_screen(
    const RenderTexture& t,
    const Rect& rect,
    const Material& material,
    BlitFlags flags)
{
    GraphicsBackend::blitToScreen(t, rect, material, flags);
}

void osc::graphics::blit_to_screen(
    const Texture2D& t,
    const Rect& rect)
{
    GraphicsBackend::blitToScreen(t, rect);
}

void osc::graphics::copy_texture(
    const RenderTexture& src,
    Texture2D& dest)
{
    GraphicsBackend::copyTexture(src, dest);
}

void osc::graphics::copy_texture(
    const RenderTexture& src,
    Texture2D& dest,
    CubemapFace face)
{
    GraphicsBackend::copyTexture(src, dest, face);
}

void osc::graphics::copy_texture(
    const RenderTexture& sourceRenderTexture,
    Cubemap& destinationCubemap,
    size_t mip)
{
    GraphicsBackend::copyTexture(sourceRenderTexture, destinationCubemap, mip);
}

/////////////////////////
//
// backend implementation
//
/////////////////////////


// helper: binds to instanced attributes (per-drawcall)
void osc::GraphicsBackend::bindToInstancedAttributes(
    const Shader::Impl& shaderImpl,
    InstancingState& ins)
{
    gl::bind_buffer(ins.buffer);

    size_t byteOffset = 0;
    if (shaderImpl.m_MaybeInstancedModelMatAttr) {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->location};
            gl::vertex_attrib_pointer(mmtxAttr, false, ins.stride, ins.base_offset + byteOffset);
            gl::vertex_attrib_divisor(mmtxAttr, 1);
            gl::enable_vertex_attrib_array(mmtxAttr);
            byteOffset += sizeof(float) * 16;
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr) {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::vertex_attrib_pointer(mmtxAttr, false, ins.stride, ins.base_offset + byteOffset);
            gl::vertex_attrib_divisor(mmtxAttr, 1);
            gl::enable_vertex_attrib_array(mmtxAttr);
            // unused: byteOffset += sizeof(float) * 16;
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat3) {
            const gl::AttributeMat3 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::vertex_attrib_pointer(mmtxAttr, false, ins.stride, ins.base_offset + byteOffset);
            gl::vertex_attrib_divisor(mmtxAttr, 1);
            gl::enable_vertex_attrib_array(mmtxAttr);
            // unused: byteOffset += sizeof(float) * 9;
        }
    }
}

// helper: unbinds from instanced attributes (per-drawcall)
void osc::GraphicsBackend::unbindFromInstancedAttributes(
    const Shader::Impl& shaderImpl,
    InstancingState&)
{
    if (shaderImpl.m_MaybeInstancedModelMatAttr) {
        if (shaderImpl.m_MaybeInstancedModelMatAttr->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedModelMatAttr->location};
            gl::disable_vertex_attrib_array(mmtxAttr);
        }
    }
    if (shaderImpl.m_MaybeInstancedNormalMatAttr) {
        if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat4) {
            const gl::AttributeMat4 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::disable_vertex_attrib_array(mmtxAttr);
        }
        else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat3) {
            const gl::AttributeMat3 mmtxAttr{shaderImpl.m_MaybeInstancedNormalMatAttr->location};
            gl::disable_vertex_attrib_array(mmtxAttr);
        }
    }
}

// helper: upload instancing data for a batch
std::optional<InstancingState> osc::GraphicsBackend::uploadInstanceData(
    std::span<const RenderObject> renderObjects,
    const Shader::Impl& shaderImpl)
{
    // preemptively upload instancing data
    std::optional<InstancingState> maybeInstancingState;

    if (shaderImpl.m_MaybeInstancedModelMatAttr || shaderImpl.m_MaybeInstancedNormalMatAttr) {

        // compute the stride between each instance
        size_t byteStride = 0;
        if (shaderImpl.m_MaybeInstancedModelMatAttr) {
            if (shaderImpl.m_MaybeInstancedModelMatAttr->shader_type == ShaderPropertyType::Mat4) {
                byteStride += sizeof(float) * 16;
            }
        }
        if (shaderImpl.m_MaybeInstancedNormalMatAttr) {
            if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat4) {
                byteStride += sizeof(float) * 16;
            }
            else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat3) {
                byteStride += sizeof(float) * 9;
            }
        }

        // write the instance data into a CPU-side buffer

        OSC_PERF("GraphicsBackend::uploadInstanceData");
        std::vector<float>& buf = g_GraphicsContextImpl->updInstanceCPUBuffer();
        buf.clear();
        buf.reserve(renderObjects.size() * (byteStride/sizeof(float)));

        size_t floatOffset = 0;
        for (const RenderObject& el : renderObjects) {
            if (shaderImpl.m_MaybeInstancedModelMatAttr) {
                if (shaderImpl.m_MaybeInstancedModelMatAttr->shader_type == ShaderPropertyType::Mat4) {
                    const Mat4 m = model_mat4(el);
                    const std::span<const float> els = to_float_span(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    floatOffset += els.size();
                }
            }
            if (shaderImpl.m_MaybeInstancedNormalMatAttr) {
                if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat4) {
                    const Mat4 m = normal_matrix4(el);
                    const std::span<const float> els = to_float_span(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    floatOffset += els.size();
                }
                else if (shaderImpl.m_MaybeInstancedNormalMatAttr->shader_type == ShaderPropertyType::Mat3) {
                    const Mat3 m = normal_matrix(el);
                    const std::span<const float> els = to_float_span(m);
                    buf.insert(buf.end(), els.begin(), els.end());
                    floatOffset += els.size();
                }
            }
        }
        OSC_ASSERT_ALWAYS(sizeof(float)*floatOffset == renderObjects.size() * byteStride);

        auto& vbo = maybeInstancingState.emplace(g_GraphicsContextImpl->updInstanceGPUBuffer(), byteStride).buffer;
        vbo.assign(std::span<const float>{buf.data(), floatOffset});
    }
    return maybeInstancingState;
}

void osc::GraphicsBackend::tryBindMaterialValueToShaderElement(
    const ShaderElement& se,
    const MaterialValue& v,
    int32_t& textureSlot)
{
    if (get_shader_type(v) != se.shader_type) {
        return;  // mismatched types
    }

    switch (v.index()) {
    case variant_index<MaterialValue, Color>():
    {
        // colors are converted from sRGB to linear when passed to
        // the shader

        const Vec4 linearColor = to_linear_colorspace(std::get<Color>(v));
        gl::UniformVec4 u{se.location};
        gl::set_uniform(u, linearColor);
        break;
    }
    case variant_index<MaterialValue, std::vector<Color>>():
    {
        const auto& colors = std::get<std::vector<Color>>(v);
        const int32_t numToAssign = min(se.size, static_cast<int32_t>(colors.size()));

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
            for (const auto& color : colors)
            {
                linearColors.emplace_back(to_linear_colorspace(color));
            }
            static_assert(sizeof(Vec4) == 4*sizeof(float));
            static_assert(alignof(Vec4) <= alignof(float));
            glUniform4fv(se.location, numToAssign, value_ptr(linearColors.front()));
        }
        break;
    }
    case variant_index<MaterialValue, float>():
    {
        gl::UniformFloat u{se.location};
        gl::set_uniform(u, std::get<float>(v));
        break;
    }
    case variant_index<MaterialValue, std::vector<float>>():
    {
        const auto& vals = std::get<std::vector<float>>(v);
        const int32_t numToAssign = min(se.size, static_cast<int32_t>(vals.size()));

        if (numToAssign > 0) {
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
    case variant_index<MaterialValue, Vec2>():
    {
        gl::UniformVec2 u{se.location};
        gl::set_uniform(u, std::get<Vec2>(v));
        break;
    }
    case variant_index<MaterialValue, Vec3>():
    {
        gl::UniformVec3 u{se.location};
        gl::set_uniform(u, std::get<Vec3>(v));
        break;
    }
    case variant_index<MaterialValue, std::vector<Vec3>>():
    {
        const auto& vals = std::get<std::vector<Vec3>>(v);
        const int32_t numToAssign = min(se.size, static_cast<int32_t>(vals.size()));

        if (numToAssign > 0) {
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
    case variant_index<MaterialValue, Vec4>():
    {
        gl::UniformVec4 u{se.location};
        gl::set_uniform(u, std::get<Vec4>(v));
        break;
    }
    case variant_index<MaterialValue, Mat3>():
    {
        gl::UniformMat3 u{se.location};
        gl::set_uniform(u, std::get<Mat3>(v));
        break;
    }
    case variant_index<MaterialValue, Mat4>():
    {
        gl::UniformMat4 u{se.location};
        gl::set_uniform(u, std::get<Mat4>(v));
        break;
    }
    case variant_index<MaterialValue, std::vector<Mat4>>():
    {
        const auto& vals = std::get<std::vector<Mat4>>(v);
        const int32_t numToAssign = min(se.size, static_cast<int32_t>(vals.size()));
        if (numToAssign > 0) {
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
    case variant_index<MaterialValue, int32_t>():
    {
        gl::UniformInt u{se.location};
        gl::set_uniform(u, std::get<int32_t>(v));
        break;
    }
    case variant_index<MaterialValue, bool>():
    {
        gl::UniformBool u{se.location};
        gl::set_uniform(u, std::get<bool>(v));
        break;
    }
    case variant_index<MaterialValue, Texture2D>():
    {
        auto& impl = const_cast<Texture2D::Impl&>(*std::get<Texture2D>(v).m_Impl);
        gl::Texture2D& texture = impl.updTexture();

        gl::active_texture(GL_TEXTURE0 + textureSlot);
        gl::bind_texture(texture);
        gl::UniformSampler2D u{se.location};
        gl::set_uniform(u, textureSlot);

        ++textureSlot;
        break;
    }
    case variant_index<MaterialValue, RenderTexture>():
    {
        static_assert(num_options<TextureDimensionality>() == 2);
        std::visit(Overload
            {
                [&textureSlot, &se](SingleSampledTexture& sst)
            {
                gl::active_texture(GL_TEXTURE0 + textureSlot);
                gl::bind_texture(sst.texture2D);
                gl::UniformSampler2D u{se.location};
                gl::set_uniform(u, textureSlot);
                ++textureSlot;
            },
            [&textureSlot, &se](MultisampledRBOAndResolvedTexture& mst)
            {
                gl::active_texture(GL_TEXTURE0 + textureSlot);
                gl::bind_texture(mst.single_sampled_texture2D);
                gl::UniformSampler2D u{se.location};
                gl::set_uniform(u, textureSlot);
                ++textureSlot;
            },
            [&textureSlot, &se](SingleSampledCubemap& cubemap)
            {
                gl::active_texture(GL_TEXTURE0 + textureSlot);
                gl::bind_texture(cubemap.cubemap);
                gl::UniformSamplerCube u{se.location};
                gl::set_uniform(u, textureSlot);
                ++textureSlot;
            },
            }, const_cast<RenderTexture::Impl&>(*std::get<RenderTexture>(v).m_Impl).getColorRenderBufferData());

        break;
    }
    case variant_index<MaterialValue, Cubemap>():
    {
        auto& impl = const_cast<Cubemap::Impl&>(*std::get<Cubemap>(v).impl_);
        const gl::TextureCubemap& texture = impl.updCubemap();

        gl::active_texture(GL_TEXTURE0 + textureSlot);
        gl::bind_texture(texture);
        gl::UniformSamplerCube u{se.location};
        gl::set_uniform(u, textureSlot);

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
void osc::GraphicsBackend::handleBatchWithSameSubMesh(
    std::span<const RenderObject> els,
    std::optional<InstancingState>& ins)
{
    auto& meshImpl = const_cast<Mesh::Impl&>(*els.front().mesh.m_Impl);
    const Shader::Impl& shaderImpl = *els.front().material.m_Impl->m_Shader.m_Impl;
    const std::optional<size_t> maybe_submesh_index = els.front().maybe_submesh_index;

    gl::bind_vertex_array(meshImpl.updVertexArray());

    if (shaderImpl.m_MaybeModelMatUniform || shaderImpl.m_MaybeNormalMatUniform) {
        // if the shader requires per-instance uniforms, then we *have* to render one
        // instance at a time

        for (const RenderObject& el : els) {

            // try binding to uModel (standard)
            if (shaderImpl.m_MaybeModelMatUniform) {
                if (shaderImpl.m_MaybeModelMatUniform->shader_type == ShaderPropertyType::Mat4) {
                    gl::UniformMat4 u{shaderImpl.m_MaybeModelMatUniform->location};
                    gl::set_uniform(u, model_mat4(el));
                }
            }

            // try binding to uNormalMat (standard)
            if (shaderImpl.m_MaybeNormalMatUniform) {
                if (shaderImpl.m_MaybeNormalMatUniform->shader_type == ShaderPropertyType::Mat3) {
                    gl::UniformMat3 u{shaderImpl.m_MaybeNormalMatUniform->location};
                    gl::set_uniform(u, normal_matrix(el));
                }
                else if (shaderImpl.m_MaybeNormalMatUniform->shader_type == ShaderPropertyType::Mat4) {
                    gl::UniformMat4 u{shaderImpl.m_MaybeNormalMatUniform->location};
                    gl::set_uniform(u, normal_matrix4(el));
                }
            }

            if (ins) {
                bindToInstancedAttributes(shaderImpl, *ins);
            }
            meshImpl.drawInstanced(1, maybe_submesh_index);
            if (ins) {
                unbindFromInstancedAttributes(shaderImpl, *ins);
                ins->base_offset += 1 * ins->stride;
            }
        }
    }
    else {
        // else: the shader supports instanced data, so we can draw multiple meshes in one call

        if (ins) {
            bindToInstancedAttributes(shaderImpl, *ins);
        }
        meshImpl.drawInstanced(els.size(), maybe_submesh_index);
        if (ins) {
            unbindFromInstancedAttributes(shaderImpl, *ins);
            ins->base_offset += els.size() * ins->stride;
        }
    }

    gl::bind_vertex_array();
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
//   - Mesh
void osc::GraphicsBackend::handleBatchWithSameMesh(
    std::span<const RenderObject> els,
    std::optional<InstancingState>& ins)
{
    // batch by sub-Mesh index
    auto batchIt = els.begin();
    while (batchIt != els.end()) {
        const auto batchEnd = find_if_not(batchIt, els.end(), RenderObjectHasSubMeshIndex{batchIt->maybe_submesh_index});
        handleBatchWithSameSubMesh({batchIt, batchEnd}, ins);
        batchIt = batchEnd;
    }
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
//   - MaterialPropertyBlock
void osc::GraphicsBackend::handleBatchWithSameMaterialPropertyBlock(
    std::span<const RenderObject> els,
    int32_t& textureSlot,
    std::optional<InstancingState>& ins)
{
    OSC_PERF("GraphicsBackend::handleBatchWithSameMaterialPropertyBlock");

    const Material::Impl& matImpl = *els.front().material.m_Impl;
    const Shader::Impl& shaderImpl = *matImpl.m_Shader.m_Impl;
    const FastStringHashtable<ShaderElement>& uniforms = shaderImpl.getUniforms();

    // bind property block variables (if applicable)
    if (els.front().maybe_prop_block) {
        for (const auto& [name, value] : els.front().maybe_prop_block->m_Impl->m_Values) {
            if (const auto* uniform = try_find(uniforms, name)) {
                tryBindMaterialValueToShaderElement(*uniform, value, textureSlot);
            }
        }
    }

    // batch by mesh
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        const auto batchEnd = find_if_not(batchIt, els.end(), RenderObjectHasMesh{&batchIt->mesh});
        handleBatchWithSameMesh({batchIt, batchEnd}, ins);
        batchIt = batchEnd;
    }
}

// helper: draw a batch of `RenderObject`s that have the same:
//
//   - Material
void osc::GraphicsBackend::handleBatchWithSameMaterial(
    const RenderPassState& renderPassState,
    std::span<const RenderObject> els)
{
    OSC_PERF("GraphicsBackend::handleBatchWithSameMaterial");

    const auto& matImpl = *els.front().material.m_Impl;
    const auto& shaderImpl = *matImpl.m_Shader.m_Impl;
    const FastStringHashtable<ShaderElement>& uniforms = shaderImpl.getUniforms();

    // preemptively upload instance data
    std::optional<InstancingState> maybeInstances = uploadInstanceData(els, shaderImpl);

    // updated by various batches (which may bind to textures etc.)
    int32_t textureSlot = 0;

    gl::use_program(shaderImpl.getProgram());

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (matImpl.getDepthFunction() != DepthFunction::Default)
    {
        glDepthFunc(toGLDepthFunc(matImpl.getDepthFunction()));
    }

    if (matImpl.getCullMode() != CullMode::Off)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(toGLCullFaceEnum(matImpl.getCullMode()));

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
            if (shaderImpl.m_MaybeViewMatUniform->shader_type == ShaderPropertyType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewMatUniform->location};
                gl::set_uniform(u, renderPassState.view_matrix);
            }
        }

        // try binding to uProjection (standard)
        if (shaderImpl.m_MaybeProjMatUniform)
        {
            if (shaderImpl.m_MaybeProjMatUniform->shader_type == ShaderPropertyType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeProjMatUniform->location};
                gl::set_uniform(u, renderPassState.projection_matrix);
            }
        }

        if (shaderImpl.m_MaybeViewProjMatUniform)
        {
            if (shaderImpl.m_MaybeViewProjMatUniform->shader_type == ShaderPropertyType::Mat4)
            {
                gl::UniformMat4 u{shaderImpl.m_MaybeViewProjMatUniform->location};
                gl::set_uniform(u, renderPassState.view_projection_matrix);
            }
        }

        // bind material values
        for (const auto& [name, value] : matImpl.m_Values) {
            if (const ShaderElement* e = tryGetValue(uniforms, name)) {
                tryBindMaterialValueToShaderElement(*e, value, textureSlot);
            }
        }
    }

    // batch by material property block
    auto batchIt = els.begin();
    while (batchIt != els.end())
    {
        const auto batchEnd = find_if_not(batchIt, els.end(), RenderObjectHasMaterialPropertyBlock{&batchIt->maybe_prop_block});
        handleBatchWithSameMaterialPropertyBlock({batchIt, batchEnd}, textureSlot, maybeInstances);
        batchIt = batchEnd;
    }

    if (matImpl.getCullMode() != CullMode::Off)
    {
        glCullFace(GL_BACK);  // default from Khronos docs
        glDisable(GL_CULL_FACE);
    }

    if (matImpl.getDepthFunction() != DepthFunction::Default)
    {
        glDepthFunc(toGLDepthFunc(DepthFunction::Default));
    }

    if (matImpl.getWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// helper: draw a sequence of `RenderObject`s
void osc::GraphicsBackend::drawRenderObjects(
    const RenderPassState& renderPassState,
    std::span<const RenderObject> els)
{
    OSC_PERF("GraphicsBackend::drawRenderObjects");

    // batch by material
    auto batchIt = els.begin();
    while (batchIt != els.end()) {
        const auto batchEnd = find_if_not(batchIt, els.end(), RenderObjectHasMaterial{&batchIt->material});
        handleBatchWithSameMaterial(renderPassState, {batchIt, batchEnd});
        batchIt = batchEnd;
    }
}

void osc::GraphicsBackend::drawBatchedByOpaqueness(
    const RenderPassState& renderPassState,
    std::span<const RenderObject> els)
{
    OSC_PERF("GraphicsBackend::drawBatchedByOpaqueness");

    auto batchIt = els.begin();
    while (batchIt != els.end()) {
        const auto opaqueEnd = find_if_not(batchIt, els.end(), is_opaque);

        if (opaqueEnd != batchIt) {
            // [batchIt..opaqueEnd] contains opaque elements
            gl::disable(GL_BLEND);
            drawRenderObjects(renderPassState, {batchIt, opaqueEnd});

            batchIt = opaqueEnd;
        }

        if (opaqueEnd != els.end()) {
            // [opaqueEnd..els.end()] contains transparent elements
            const auto transparentEnd = find_if(opaqueEnd, els.end(), is_opaque);
            gl::enable(GL_BLEND);
            drawRenderObjects(renderPassState, {opaqueEnd, transparentEnd});

            batchIt = transparentEnd;
        }
    }
}

void osc::GraphicsBackend::flushRenderQueue(Camera::Impl& camera, float aspectRatio)
{
    OSC_PERF("GraphicsBackend::flushRenderQueue");

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
    const RenderPassState renderPassState{
        camera.position(),
        camera.view_matrix(),
        camera.projection_matrix(aspectRatio),
    };

    gl::enable(GL_DEPTH_TEST);

    // draw by reordering depth-tested elements around the not-depth-tested elements
    auto batchIt = queue.begin();
    while (batchIt != queue.end())
    {
        const auto depthTestedEnd = find_if_not(batchIt, queue.end(), is_depth_tested);

        if (depthTestedEnd != batchIt)
        {
            // there are >0 depth-tested elements that are elegible for reordering

            sort_render_queue(batchIt, depthTestedEnd, renderPassState.camera_pos);
            drawBatchedByOpaqueness(renderPassState, {batchIt, depthTestedEnd});

            batchIt = depthTestedEnd;
        }

        if (depthTestedEnd != queue.end())
        {
            // there are >0 not-depth-tested elements that cannot be reordered

            const auto ignoreDepthTestEnd = find_if(depthTestedEnd, queue.end(), is_depth_tested);

            // these elements aren't depth-tested and should just be drawn as-is
            gl::disable(GL_DEPTH_TEST);
            drawBatchedByOpaqueness(renderPassState, {depthTestedEnd, ignoreDepthTestEnd});
            gl::enable(GL_DEPTH_TEST);

            batchIt = ignoreDepthTestEnd;
        }
    }

    // queue flushed: clear it
    queue.clear();
}

void osc::GraphicsBackend::validateRenderTarget(RenderTarget& renderTarget)
{
    // ensure there is at least one color attachment
    OSC_ASSERT(!renderTarget.colors.empty() && "a render target must have one or more color attachments");

    OSC_ASSERT(renderTarget.colors.front().buffer != nullptr && "a color attachment must have a non-null render buffer");
    const Vec2i firstColorBufferDimensions = renderTarget.colors.front().buffer->m_Impl->getDimensions();
    const AntiAliasingLevel firstColorBufferSamples = renderTarget.colors.front().buffer->m_Impl->getAntialiasingLevel();

    // validate other buffers against the first
    for (auto it = renderTarget.colors.begin()+1; it != renderTarget.colors.end(); ++it)
    {
        const RenderTargetColorAttachment& colorAttachment = *it;
        OSC_ASSERT(colorAttachment.buffer != nullptr);
        OSC_ASSERT(colorAttachment.buffer->m_Impl->getDimensions() == firstColorBufferDimensions);
        OSC_ASSERT(colorAttachment.buffer->m_Impl->getAntialiasingLevel() == firstColorBufferSamples);
    }
    OSC_ASSERT(renderTarget.depth.buffer != nullptr);
    OSC_ASSERT(renderTarget.depth.buffer->m_Impl->getDimensions() == firstColorBufferDimensions);
    OSC_ASSERT(renderTarget.depth.buffer->m_Impl->getAntialiasingLevel() == firstColorBufferSamples);
}

Rect osc::GraphicsBackend::calcViewportRect(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    const Vec2 targetDims = maybeCustomRenderTarget ?
        Vec2{maybeCustomRenderTarget->colors.front().buffer->m_Impl->getDimensions()} :
        App::get().dims();

    const Rect cameraRect = camera.pixel_rect() ?
        *camera.pixel_rect() :
        Rect{{}, targetDims};

    const Vec2 cameraRectBottomLeft = bottom_left_lh(cameraRect);
    const Vec2 outputDimensions = dimensions_of(cameraRect);
    const Vec2 topLeft = {cameraRectBottomLeft.x, targetDims.y - cameraRectBottomLeft.y};

    return Rect{topLeft, topLeft + outputDimensions};
}

Rect osc::GraphicsBackend::setupTopLevelPipelineState(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    const Rect viewportRect = calcViewportRect(camera, maybeCustomRenderTarget);
    const Vec2 viewportDims = dimensions_of(viewportRect);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl::viewport(
        static_cast<GLsizei>(viewportRect.p1.x),
        static_cast<GLsizei>(viewportRect.p1.y),
        static_cast<GLsizei>(viewportDims.x),
        static_cast<GLsizei>(viewportDims.y)
    );

    if (camera.m_MaybeScissorRect)
    {
        const Rect scissorRect = *camera.m_MaybeScissorRect;
        const Vec2i scissorDims = dimensions_of(scissorRect);

        gl::enable(GL_SCISSOR_TEST);
        glScissor(
            static_cast<GLint>(scissorRect.p1.x),
            static_cast<GLint>(scissorRect.p1.y),
            scissorDims.x,
            scissorDims.y
        );
    }
    else
    {
        gl::disable(GL_SCISSOR_TEST);
    }

    return viewportRect;
}

void osc::GraphicsBackend::teardownTopLevelPipelineState(
    Camera::Impl& camera,
    RenderTarget*)
{
    if (camera.m_MaybeScissorRect)
    {
        gl::disable(GL_SCISSOR_TEST);
    }
    gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);
    gl::use_program();
}

std::optional<gl::FrameBuffer> osc::GraphicsBackend::bindAndClearRenderBuffers(
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
        gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, rendererFBO);

        // attach color buffers to the FBO
        for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
        {
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
            }, maybeCustomRenderTarget->colors[i].buffer->m_Impl->updRenderBufferData());
        }

        // attach depth buffer to the FBO
        std::visit(Overload
        {
            [](SingleSampledTexture& t)
            {
                gl::framebuffer_texture2D(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    t.texture2D,
                    0
                );
            },
            [](MultisampledRBOAndResolvedTexture& t)
            {
                gl::framebuffer_renderbuffer(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    t.multisampled_rbo
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
                    t.cubemap.get(),
                    0
                );
            }
#endif
        }, maybeCustomRenderTarget->depth.buffer->m_Impl->updRenderBufferData());

        // Multi-Render Target (MRT) support: tell OpenGL to use all specified
        // render targets when drawing and/or clearing
        {
            const size_t numColorAttachments = maybeCustomRenderTarget->colors.size();

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
            static_assert(num_options<RenderBufferLoadAction>() == 2);

            // if requested, clear color buffers
            for (size_t i = 0; i < maybeCustomRenderTarget->colors.size(); ++i)
            {
                RenderTargetColorAttachment& colorAttachment = maybeCustomRenderTarget->colors[i];
                if (colorAttachment.loadAction == RenderBufferLoadAction::Clear)
                {
                    glClearBufferfv(
                        GL_COLOR,
                        static_cast<GLint>(i),
                        value_ptr(static_cast<Vec4>(colorAttachment.clear_color))
                    );
                }
            }

            // if requested, clear depth buffer
            if (maybeCustomRenderTarget->depth.loadAction == RenderBufferLoadAction::Clear)
            {
                gl::clear(GL_DEPTH_BUFFER_BIT);
            }
        }
    }
    else
    {
        gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);

        // we're rendering to the window
        if (camera.m_ClearFlags != CameraClearFlags::Nothing)
        {
            // clear window
            const GLenum clearFlags = camera.m_ClearFlags & CameraClearFlags::SolidColor ?
                GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT :
                GL_DEPTH_BUFFER_BIT;

            // clear color is in sRGB, but the window's framebuffer is sRGB-corrected
            // and assume that clear colors are in linear space
            const Color linearColor = to_linear_colorspace(camera.m_BackgroundColor);
            gl::clear_color(
                linearColor.r,
                linearColor.g,
                linearColor.b,
                linearColor.a
            );
            gl::clear(clearFlags);
        }
    }

    return maybeRenderFBO;
}

void osc::GraphicsBackend::resolveRenderBuffers(RenderTarget& renderTarget)
{
    static_assert(num_options<RenderBufferStoreAction>() == 2, "check 'if's etc. in this code");

    OSC_PERF("RenderTexture::resolveBuffers");

    // setup FBOs (reused per color buffer)
    gl::FrameBuffer multisampledReadFBO;
    gl::bind_framebuffer(GL_READ_FRAMEBUFFER, multisampledReadFBO);

    gl::FrameBuffer resolvedDrawFBO;
    gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, resolvedDrawFBO);

    // resolve each color buffer with a blit
    for (size_t i = 0; i < renderTarget.colors.size(); ++i)
    {
        const RenderTargetColorAttachment& attachment = renderTarget.colors[i];
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
                const GLint attachmentLoc = GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i);

                gl::framebuffer_renderbuffer(
                    GL_READ_FRAMEBUFFER,
                    attachmentLoc,
                    t.multisampled_rbo
                );
                glReadBuffer(attachmentLoc);

                gl::framebuffer_texture2D(
                    GL_DRAW_FRAMEBUFFER,
                    attachmentLoc,
                    t.single_sampled_texture2D,
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
            const Vec2i dimensions = attachment.buffer->m_Impl->getDimensions();
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
                gl::framebuffer_renderbuffer(
                    GL_READ_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    t.multisampled_rbo
                );
                glReadBuffer(GL_DEPTH_ATTACHMENT);

                gl::framebuffer_texture2D(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    t.single_sampled_texture2D,
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
            const Vec2i dimensions = renderTarget.depth.buffer->m_Impl->getDimensions();
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

void osc::GraphicsBackend::renderCameraQueue(
    Camera::Impl& camera,
    RenderTarget* maybeCustomRenderTarget)
{
    OSC_PERF("GraphicsBackend::renderCameraQueue");

    if (maybeCustomRenderTarget)
    {
        validateRenderTarget(*maybeCustomRenderTarget);
    }

    const Rect viewportRect = setupTopLevelPipelineState(
        camera,
        maybeCustomRenderTarget
    );

    {
        const std::optional<gl::FrameBuffer> maybeTmpFBO =
            bindAndClearRenderBuffers(camera, maybeCustomRenderTarget);
        flushRenderQueue(camera, aspect_ratio(viewportRect));
    }

    if (maybeCustomRenderTarget)
    {
        resolveRenderBuffers(*maybeCustomRenderTarget);
    }

    teardownTopLevelPipelineState(
        camera,
        maybeCustomRenderTarget
    );
}

void osc::GraphicsBackend::draw(
    const Mesh& mesh,
    const Transform& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybeMaterialPropertyBlock,
    std::optional<size_t> maybe_submesh_index)
{
    if (maybe_submesh_index && *maybe_submesh_index >= mesh.getSubMeshCount()) {
        throw std::out_of_range{"the given sub-mesh index was out of range (i.e. the given mesh does not have that many sub-meshes)"};
    }

    camera.impl_.upd()->m_RenderQueue.emplace_back(
        mesh,
        transform,
        material,
        maybeMaterialPropertyBlock,
        maybe_submesh_index
    );
}

void osc::GraphicsBackend::draw(
    const Mesh& mesh,
    const Mat4& transform,
    const Material& material,
    Camera& camera,
    const std::optional<MaterialPropertyBlock>& maybeMaterialPropertyBlock,
    std::optional<size_t> maybe_submesh_index)
{
    if (maybe_submesh_index && *maybe_submesh_index >= mesh.getSubMeshCount())
    {
        throw std::out_of_range{"the given sub-mesh index was out of range (i.e. the given mesh does not have that many sub-meshes)"};
    }

    camera.impl_.upd()->m_RenderQueue.emplace_back(
        mesh,
        transform,
        material,
        maybeMaterialPropertyBlock,
        maybe_submesh_index
    );
}

void osc::GraphicsBackend::blit(
    const Texture2D& source,
    RenderTexture& dest)
{
    Camera c;
    c.set_background_color(Color::clear());
    c.set_projection_matrix_override(identity<Mat4>());
    c.set_view_matrix_override(identity<Mat4>());

    Material m = g_GraphicsContextImpl->getQuadMaterial();
    m.setTexture("uTexture", source);

    graphics::draw(g_GraphicsContextImpl->getQuadMesh(), Transform{}, m, c);
    c.render_to(dest);
}

void osc::GraphicsBackend::blitToScreen(
    const RenderTexture& t,
    const Rect& rect,
    BlitFlags flags)
{
    blitToScreen(t, rect, g_GraphicsContextImpl->getQuadMaterial(), flags);
}

void osc::GraphicsBackend::blitToScreen(
    const RenderTexture& t,
    const Rect& rect,
    const Material& material,
    BlitFlags)
{
    OSC_ASSERT(g_GraphicsContextImpl);
    OSC_ASSERT(t.m_Impl->hasBeenRenderedTo() && "the input texture has not been rendered to");

    Camera c;
    c.set_background_color(Color::clear());
    c.set_pixel_rect(rect);
    c.set_projection_matrix_override(identity<Mat4>());
    c.set_view_matrix_override(identity<Mat4>());
    c.set_clear_flags(CameraClearFlags::Nothing);

    Material copy{material};
    copy.setRenderTexture("uTexture", t);
    graphics::draw(g_GraphicsContextImpl->getQuadMesh(), Transform{}, copy, c);
    c.render_to_screen();
    copy.clearRenderTexture("uTexture");
}

void osc::GraphicsBackend::blitToScreen(
    const Texture2D& t,
    const Rect& rect)
{
    OSC_ASSERT(g_GraphicsContextImpl);

    Camera c;
    c.set_background_color(Color::clear());
    c.set_pixel_rect(rect);
    c.set_projection_matrix_override(identity<Mat4>());
    c.set_view_matrix_override(identity<Mat4>());
    c.set_clear_flags(CameraClearFlags::Nothing);

    Material copy{g_GraphicsContextImpl->getQuadMaterial()};
    copy.setTexture("uTexture", t);
    graphics::draw(g_GraphicsContextImpl->getQuadMesh(), Transform{}, copy, c);
    c.render_to_screen();
    copy.clearTexture("uTexture");
}

void osc::GraphicsBackend::copyTexture(
    const RenderTexture& src,
    Texture2D& dest)
{
    copyTexture(src, dest, CubemapFace::PositiveX);
}

void osc::GraphicsBackend::copyTexture(
    const RenderTexture& src,
    Texture2D& dest,
    CubemapFace face)
{
    OSC_ASSERT(g_GraphicsContextImpl);
    OSC_ASSERT(src.m_Impl->hasBeenRenderedTo() && "the input texture has not been rendered to");

    // create a source (read) framebuffer for blitting from the source render texture
    gl::FrameBuffer readFBO;
    gl::bind_framebuffer(GL_READ_FRAMEBUFFER, readFBO);
    std::visit(Overload  // attach source texture depending on rendertexture's type
    {
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
                toOpenGLTextureEnum(face),
                t.cubemap.get(),
                0
            );
        }
    }, const_cast<RenderTexture::Impl&>(*src.m_Impl).getColorRenderBufferData());
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // create a destination (draw) framebuffer for blitting to the destination render texture
    gl::FrameBuffer drawFBO;
    gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
    gl::framebuffer_texture2D(
        GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        dest.m_Impl.upd()->updTexture(),
        0
    );
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // blit the read framebuffer to the draw framebuffer
    gl::blit_framebuffer(
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
        const GLint packFormat = toImagePixelPackAlignment(dest.texture_format());

        OSC_ASSERT(is_aligned_at_least(cpuBuffer.data(), packFormat) && "glReadPixels must be called with a buffer that is aligned to GL_PACK_ALIGNMENT (see: https://www.khronos.org/opengl/wiki/Common_Mistakes)");
        OSC_ASSERT(cpuBuffer.size() == static_cast<ptrdiff_t>(dest.getDimensions().x*dest.getDimensions().y)*NumBytesPerPixel(dest.texture_format()));

        gl::viewport(0, 0, dest.getDimensions().x, dest.getDimensions().y);
        gl::bind_framebuffer(GL_READ_FRAMEBUFFER, drawFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::pixel_store_i(GL_PACK_ALIGNMENT, packFormat);
        glReadPixels(
            0,
            0,
            dest.getDimensions().x,
            dest.getDimensions().y,
            toImageColorFormat(dest.texture_format()),
            toImageDataType(dest.texture_format()),
            cpuBuffer.data()
        );
    }
    gl::bind_framebuffer(GL_FRAMEBUFFER, gl::window_framebuffer);
}

void osc::GraphicsBackend::copyTexture(
    const RenderTexture& sourceRenderTexture,
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
    const size_t maxMipmapLevel = static_cast<size_t>(max(
        0,
        cpp20::bit_width(static_cast<size_t>(destinationCubemap.width())) - 1
    ));

    OSC_ASSERT(sourceRenderTexture.getDimensionality() == TextureDimensionality::Cube && "provided render texture must be a cubemap to call this method");
    OSC_ASSERT(mip <= maxMipmapLevel);

    // blit each face of the source cubemap into the output cubemap
    for (size_t face = 0; face < 6; ++face)
    {
        gl::FrameBuffer readFBO;
        gl::bind_framebuffer(GL_READ_FRAMEBUFFER, readFBO);
        std::visit(Overload  // attach source texture depending on rendertexture's type
        {
            [](SingleSampledTexture&)
            {
                OSC_ASSERT(false && "cannot call copyTexture (Cubemap --> Cubemap) with a 2D render");
            },
            [](MultisampledRBOAndResolvedTexture&)
            {
                OSC_ASSERT(false && "cannot call copyTexture (Cubemap --> Cubemap) with a 2D render");
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
        }, const_cast<RenderTexture::Impl&>(*sourceRenderTexture.m_Impl).getColorRenderBufferData());
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        gl::FrameBuffer drawFBO;
        gl::bind_framebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face),
            destinationCubemap.impl_.upd()->updCubemap().get(),
            static_cast<GLint>(mip)
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        // blit the read framebuffer to the draw framebuffer
        gl::blit_framebuffer(
            0,
            0,
            sourceRenderTexture.getDimensions().x,
            sourceRenderTexture.getDimensions().y,
            0,
            0,
            destinationCubemap.width() / (1<<mip),
            destinationCubemap.width() / (1<<mip),
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR  // the two texture may have different dimensions (avoid GL_NEAREST)
        );
    }

    // TODO: should be copied into CPU memory if mip==0? (won't store mipmaps in the CPU but
    // maybe it makes sense to store the mip==0 in CPU?)
}
