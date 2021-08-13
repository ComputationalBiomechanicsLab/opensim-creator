#pragma once

#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <limits>
#include <stdexcept>
#include <array>
#include <iosfwd>

// 3d: low-level 3D rendering primitives based on OpenGL
//
// these are the low-level datastructures/functions used for rendering
// 3D elements in OSC. The renderer is not dependent on SimTK/OpenSim at
// all and has a very low-level view of view of things (verts, drawlists)
namespace osc {



    // modelling

    std::ostream& operator<<(std::ostream&, glm::vec3 const&);
    std::ostream& operator<<(std::ostream&, glm::vec2 const&);
    std::ostream& operator<<(std::ostream&, glm::vec4 const&);

    struct Textured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    struct Untextured_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
    };

    // raw mesh data, held in main-memory and accessible to the CPU
    template<typename TVert>
    struct Mesh {
        std::vector<TVert> verts;
        std::vector<GLushort> indices;

        void clear() {
            verts.clear();
            indices.clear();
        }
    };

    // mesh specializations
    struct Untextured_mesh : public Mesh<Untextured_vert> {};
    struct Textured_mesh : public Mesh<Textured_vert> {};

    // returns true if the provided vectors are at the same location
    bool is_colocated(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::vec3 vec_min(glm::vec3 const& a, glm::vec3 const& b) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    glm::vec3 vec_max(glm::vec3 const& a, glm::vec3 const& b) noexcept;

    // returns the *index* of a vectors longest dimension
    glm::vec3::length_type vec_longest_dim_idx(glm::vec3 const&) noexcept;

    // returns a normal vector of the supplied triangle
    //
    // effectively, (B-A) x (C-A)
    glm::vec3 triangle_normal(glm::vec3 const&, glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 normal_matrix(glm::mat4 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 normal_matrix(glm::mat4x3 const&) noexcept;



    // AABBs

    // axis-aligned bounding box (AABB)
    struct AABB final {
        glm::vec3 min;
        glm::vec3 max;
    };

    // print AABB in a human-readable format
    std::ostream& operator<<(std::ostream& o, AABB const& aabb);

    // returns the centerpoint of an AABB
    glm::vec3 aabb_center(AABB const& a) noexcept;

    // returns the dimensions of an AABB
    glm::vec3 aabb_dims(AABB const& a) noexcept;

    // returns the smallest AABB that spans both of the provided AABBs
    AABB aabb_union(AABB const& a, AABB const& b) noexcept;

    // returns true if the AABB has a volume of 0
    bool aabb_is_empty(AABB const&) noexcept;

    // returns the *index* of the longest dimension of an AABB
    glm::vec3::length_type aabb_longest_dim_idx(AABB const&) noexcept;

    // returns the smallest AABB that spans all AABBs in the range [it, end)
    //
    // `AABBGetter` should be a callable with signature: `AABB const&(*)(decltype(*it))`
    template<typename ForwardIterator, typename AABBGetter>
    inline constexpr AABB aabb_union(ForwardIterator it, ForwardIterator end, AABBGetter getter) noexcept {
        if (it == end) {
            return AABB{};  // instead of throwing, which might be better behavior...
        }

        AABB rv = getter(*it++);
        while (it != end) {
            rv = aabb_union(rv, getter(*it++));
        }
        return rv;
    }

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<glm::vec3, 8> aabb_verts(AABB const& aabb) noexcept;

    // apply a transformation matrix to the AABB, returning a new AABB that
    // spans the existing AABB's cuboid
    //
    // note: repeatably doing this can keep growing the AABB, even if the
    // underlying data wouldn't logically grow. Think about what would happen
    // if you rotated an AABB 45 degrees in one axis, successively returning
    // a new AABB that had to expand to accomodate the diagonal growth.
    AABB aabb_xform(AABB const&, glm::mat4 const&) noexcept;

    // computes an AABB from a mesh
    AABB aabb_from_mesh(Untextured_mesh const&) noexcept;

    // computes an AABB from points in space
    AABB aabb_from_points(glm::vec3 const* first, size_t n);

    // computes an AABB from points of a triangle
    AABB aabb_from_triangle(glm::vec3 const* first);



    // analytical geometry

    struct Sphere final {
        glm::vec3 origin;
        float radius;
    };

    struct Line final {
        glm::vec3 o;  // origin
        glm::vec3 d;  // direction - should be normalized
    };

    struct Plane final {
        glm::vec3 origin;
        glm::vec3 normal;
    };

    struct Disc final {
        glm::vec3 origin;
        glm::vec3 normal;
        float radius;
    };

    // debug printers for the above
    std::ostream& operator<<(std::ostream&, Sphere const&);
    std::ostream& operator<<(std::ostream&, Line const&);
    std::ostream& operator<<(std::ostream&, Plane const&);
    std::ostream& operator<<(std::ostream&, Disc const&);

    // analytical geometry calculations
    Sphere sphere_bounds_of_mesh(Untextured_mesh const&) noexcept;
    Sphere sphere_bounds_of_points(glm::vec3 const*, size_t) noexcept;
    AABB sphere_aabb(Sphere const&) noexcept;
    glm::mat4 circle_to_disc_xform(Disc const& d) noexcept;



    // hit testing

    struct Line_sphere_hittest_result final {
        bool intersected;
        float t0;
        float t1;
    };

    struct Line_AABB_hittest_result final {
        bool intersected;
        float t0;
        float t1;
    };

    struct Line_plane_hittest_result final {
        bool intersected;
        float t;
    };

    struct Line_disc_hittest_result final {
        bool intersected;
        float t;
    };

    struct Line_triangle_hittest_result final {
        bool intersected;
        float t;
    };

    Line_sphere_hittest_result line_intersects_sphere(Sphere const&, Line const&) noexcept;
    Line_AABB_hittest_result line_intersects_AABB(AABB const&, Line const&) noexcept;
    Line_plane_hittest_result line_intersects_plane(Plane const&, Line const&) noexcept;
    Line_disc_hittest_result line_intersects_disc(Disc const&, Line const&) noexcept;
    Line_triangle_hittest_result line_intersects_triangle(glm::vec3 const*, Line const&) noexcept;



    // coloring

    [[nodiscard]] inline constexpr GLubyte f32_to_u8_color(float v) noexcept {
        return static_cast<GLubyte>(255.0f * v);
    }

    [[nodiscard]] inline constexpr float u8_to_f32_color(GLubyte b) noexcept {
        return static_cast<float>(b) / 255.0f;
    }

    [[nodiscard]] inline constexpr GLubyte f64_to_u8_color(double v) noexcept {
        return static_cast<GLubyte>(255.0 * v);
    }

    // 4 color channels (RGBA), 8 bits per channel
    struct Rgba32 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;
        GLubyte a;

        [[nodiscard]] static constexpr Rgba32 from_vec4(glm::vec4 const& v) noexcept {
            Rgba32 rv{};
            rv.r = f32_to_u8_color(v.r);
            rv.g = f32_to_u8_color(v.g);
            rv.b = f32_to_u8_color(v.b);
            rv.a = f32_to_u8_color(v.a);
            return rv;
        }

        [[nodiscard]] static constexpr Rgba32 from_d4(double r, double g, double b, double a) noexcept {
            Rgba32 rv{};
            rv.r = f64_to_u8_color(r);
            rv.g = f64_to_u8_color(g);
            rv.b = f64_to_u8_color(b);
            rv.a = f64_to_u8_color(a);
            return rv;
        }

        [[nodiscard]] static constexpr Rgba32 from_f4(float r, float g, float b, float a) noexcept {
            Rgba32 rv{};
            rv.r = f32_to_u8_color(r);
            rv.g = f32_to_u8_color(g);
            rv.b = f32_to_u8_color(b);
            rv.a = f32_to_u8_color(a);
            return rv;
        }

        [[nodiscard]] static constexpr Rgba32 from_u32(uint32_t v) noexcept {
            Rgba32 rv{};
            rv.r = static_cast<GLubyte>((v >> 24) & 0xff);
            rv.g = static_cast<GLubyte>((v >> 16) & 0xff);
            rv.b = static_cast<GLubyte>((v >> 8) & 0xff);
            rv.a = static_cast<GLubyte>(v & 0xff);
            return rv;
        }

        Rgba32() = default;

        constexpr Rgba32(GLubyte r_, GLubyte g_, GLubyte b_, GLubyte a_) noexcept :
            r{r_},
            g{g_},
            b{b_},
            a{a_} {
        }

        [[nodiscard]] constexpr uint32_t to_u32() const noexcept {
            uint32_t rv = static_cast<uint32_t>(r);
            rv |= static_cast<uint32_t>(g) << 8;
            rv |= static_cast<uint32_t>(b) << 16;
            rv |= static_cast<uint32_t>(a) << 24;
            return rv;
        }

        [[nodiscard]] constexpr glm::vec4 to_vec4() const noexcept {
            glm::vec4 rv{};
            rv.r = u8_to_f32_color(r);
            rv.g = u8_to_f32_color(g);
            rv.b = u8_to_f32_color(b);
            rv.a = u8_to_f32_color(a);
            return rv;
        }
    };

    // 3 color channels (RGB), 8 bits per channel
    struct Rgb24 final {
        GLubyte r;
        GLubyte g;
        GLubyte b;

        [[nodiscard]] static constexpr Rgb24 from_vec3(glm::vec3 const& v) noexcept {
            Rgb24 rv{};
            rv.r = static_cast<GLubyte>(255.0f * v.r);
            rv.g = static_cast<GLubyte>(255.0f * v.g);
            rv.b = static_cast<GLubyte>(255.0f * v.b);
            return rv;
        }

        Rgb24() = default;

        constexpr Rgb24(GLubyte r_, GLubyte g_, GLubyte b_) noexcept :
            r{r_},
            g{g_},
            b{b_} {
        }
    };



    // mesh generation


    // generate a quad at Z == 0, X == [-1, 1], and Y == [-1, 1] with texture
    // coords that can be used as an image banner
    template<typename T>
    void generate_banner(T&);

    template<typename T>
    T generate_banner() {
        T rv;
        generate_banner(rv);
        return rv;
    }

    template<>
    void generate_banner(Textured_mesh&);


    // generate a mesh for a unit sphere (radius = 1.0f, origin = (0, 0, 0))
    template<typename T>
    void generate_uv_sphere(T&);

    template<typename T>
    T generate_uv_sphere() {
        T rv;
        generate_uv_sphere(rv);
        return rv;
    }

    template<>
    void generate_uv_sphere(Untextured_mesh&);
    template<>
    void generate_uv_sphere(std::vector<glm::vec3>&);

    // generate a mesh for a cylinder with:
    //
    // - top == {x, 1.0f, z}
    // - bottom == {x, -1.0f, z}
    void generate_simbody_cylinder(size_t num_sides, Untextured_mesh&);

    // generate a 2D grid at Z == 0, min -1.0f, and max +1.0f in XY with `ticks`
    // lines in each dimension
    void generate_NxN_grid(size_t ticks, Untextured_mesh&);

    // generate y line at X = 0, Z = 0, and Y going from -1 to +1
    void generate_y_line(Untextured_mesh&);
    Untextured_mesh generate_y_line();

    template<typename T>
    void generate_cube(T&);

    template<typename T>
    T generate_cube() {
        T rv;
        generate_cube(rv);
        return rv;
    }

    template<>
    void generate_cube(Untextured_mesh&);

    // generates a cube wire mesh (good for visualizing AABBs)
    void generate_cube_lines(Untextured_mesh&);
    Untextured_mesh generate_cube_lines();
    std::vector<glm::vec3> generate_cube_lines_points();

    std::vector<glm::vec3> generate_circle_tris(size_t);



    // camera support

    struct Polar_perspective_camera final {
        // polar coords
        float radius = 1.0f;
        float theta = 0.0f;  // 0.88f;
        float phi = 0.0f;  // 0.4f;

        // how much to pan the scene by, relative to worldspace
        glm::vec3 pan = {0.0f, 0.0f, 0.0f};

        // clipspace properties
        float fov = 120.0f;
        float znear = 0.1f;
        float zfar = 100.0f;

        // note: relative deltas here are relative to whatever "screen" the camera
        // is handling.
        //
        // e.g. moving a mouse 400px in X in a screen that is 800px wide should
        //      have a delta.x of 0.5f

        // pan: pan along the current view plane
        void do_pan(float aspect_ratio, glm::vec2 delta) noexcept;

        // drag: spin the view around the origin, such that the distance between
        //       the camera and the origin remains constant
        void do_drag(glm::vec2 delta) noexcept;

        // autoscale znear and zfar based on the camera's distance from what it's looking at
        //
        // important for looking at extremely small/large scenes. znear and zfar dictates
        // both the culling planes of the camera *and* rescales the Z values of elements
        // in the scene. If the znear-to-zfar range is too large then Z-fighting will happen
        // and the scene will look wrong.
        void do_znear_zfar_autoscale() noexcept;

        [[nodiscard]] glm::mat4 view_matrix() const noexcept;
        [[nodiscard]] glm::mat4 projection_matrix(float aspect_ratio) const noexcept;
        [[nodiscard]] glm::vec3 pos() const noexcept;
    };

    struct Euler_perspective_camera final {
        glm::vec3 pos;
        float pitch;
        float yaw;
        float fov;
        float znear;
        float zfar;

        Euler_perspective_camera();

        [[nodiscard]] glm::vec3 front() const noexcept;
        [[nodiscard]] glm::vec3 up() const noexcept;
        [[nodiscard]] glm::vec3 right() const noexcept;
        [[nodiscard]] glm::mat4 view_matrix() const noexcept;
        [[nodiscard]] glm::mat4 projection_matrix(float aspect_ratio) const noexcept;
    };


    // texturing

    // generate a chequered floor texture
    //
    // this is typically used as a default scene floor for visualization
    gl::Texture_2d generate_chequered_floor_texture();

    // flags for loading textures from disk
    enum Tex_flags {
        TexFlag_None = 0,
        TexFlag_SRGB = 1,

        // BEWARE: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but can cause surprising behavior if the pixels represent vectors
        //
        // therefore, if you are flipping (e.g.) normal maps, you may *also* need
        // to flip the pixel content appropriately (e.g. if RGB represents XYZ then
        // you'll need to negate each G)
        TexFlag_Flip_Pixels_Vertically = 2,
    };

    // an image loaded onto the GPU, plus CPU-side metadata (dimensions, channels)
    struct Image_texture final {
        gl::Texture_2d texture;
        int width;
        int height;
        int channels;  // in most cases, 3 == RGB, 4 == RGBA
    };

    // read an image file (.PNG, .JPEG, etc.) directly into an OpenGL (GPU) texture
    Image_texture load_image_as_texture(char const* path, Tex_flags = TexFlag_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    //
    // useful for skyboxes, precomputed point-shadow maps, etc.
    gl::Texture_cubemap load_cubemap(
        char const* path_pos_x,
        char const* path_neg_x,
        char const* path_pos_y,
        char const* path_neg_y,
        char const* path_pos_z,
        char const* path_neg_z);
}
