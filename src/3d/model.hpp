#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>

#include <iosfwd>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>

// 3d: low-level 3D rendering primitives based on OpenGL
//
// these are the low-level datastructures/functions used for rendering
// 3D elements in OSC. The renderer is not dependent on SimTK/OpenSim at
// all and has a very low-level view of view of things (verts, drawlists)
namespace osc {

    // glm printing utilities - handy for debugging
    std::ostream& operator<<(std::ostream&, glm::vec2 const&);
    std::ostream& operator<<(std::ostream&, glm::vec3 const&);
    std::ostream& operator<<(std::ostream&, glm::vec4 const&);
    std::ostream& operator<<(std::ostream&, glm::mat3 const&);
    std::ostream& operator<<(std::ostream&, glm::mat4x3 const&);
    std::ostream& operator<<(std::ostream&, glm::mat4 const&);

    // returns true if the provided vectors are at the same location
    bool is_colocated(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing min(a[dim], b[dim]) for each dimension
    glm::vec3 vec_min(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a vector containing max(a[dim], b[dim]) for each dimension
    glm::vec3 vec_max(glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns the *index* of a vectors longest dimension
    glm::vec3::length_type vec_longest_dim_idx(glm::vec3 const&) noexcept;

    // returns a normal vector of the supplied (pointed to) triangle (i.e. (v[1]-v[0]) x (v[2]-v[0]))
    glm::vec3 triangle_normal(glm::vec3 const*) noexcept;

    // returns a normal vector of the supplied triangle (i.e. (B-A) x (C-A))
    glm::vec3 triangle_normal(glm::vec3 const&, glm::vec3 const&, glm::vec3 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 normal_matrix(glm::mat4 const&) noexcept;

    // returns a normal matrix created from the supplied xform matrix
    glm::mat3 normal_matrix(glm::mat4x3 const&) noexcept;

    // returns matrix that rotates dir1 to point in the same direction as dir2
    glm::mat4 rot_dir1_to_dir2(glm::vec3 const& dir1, glm::vec3 const& dir2) noexcept;

    struct AABB final {
        glm::vec3 min;
        glm::vec3 max;
    };

    // prints the AABB in a human-readable format
    std::ostream& operator<<(std::ostream&, AABB const&);

    // returns the centerpoint of an AABB
    glm::vec3 aabb_center(AABB const&) noexcept;

    // returns the dimensions of an AABB
    glm::vec3 aabb_dims(AABB const&) noexcept;

    // returns the smallest AABB that spans both of the provided AABBs
    AABB aabb_union(AABB const&, AABB const&) noexcept;

    // returns true if the AABB has an effective volume of 0
    bool aabb_is_empty(AABB const&) noexcept;

    // returns the *index* of the longest dimension of an AABB
    glm::vec3::length_type aabb_longest_dim_idx(AABB const&) noexcept;

    // returns the eight corner points of the cuboid representation of the AABB
    std::array<glm::vec3, 8> aabb_verts(AABB const&) noexcept;

    // apply a transformation matrix to the AABB
    //
    // note: don't do this repeatably, because it can keep growing the AABB
    AABB aabb_apply_xform(AABB const&, glm::mat4 const&) noexcept;

    // computes an AABB from points in space
    AABB aabb_from_points(glm::vec3 const*, size_t n) noexcept;


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

    struct Segment final {
        glm::vec3 p1;
        glm::vec3 p2;
    };

    // prints the supplied geometry in a human-readable format
    std::ostream& operator<<(std::ostream&, Sphere const&);
    std::ostream& operator<<(std::ostream&, Line const&);
    std::ostream& operator<<(std::ostream&, Plane const&);
    std::ostream& operator<<(std::ostream&, Disc const&);

    // analytical geometry calculations
    Sphere sphere_bounds_of_points(glm::vec3 const*, size_t n) noexcept;
    AABB sphere_aabb(Sphere const&) noexcept;

    // helpful for mapping analytical geometry into a scene
    glm::mat4 disc_to_disc_xform(Disc const&, Disc const&) noexcept;
    glm::mat4 sphere_to_sphere_xform(Sphere const&, Sphere const&) noexcept;
    glm::mat4 segment_to_segment_xform(Segment const&, Segment const&) noexcept;


    struct Ray_collision final {
        bool hit;
        float distance;
    };

    // collision tests
    Ray_collision get_ray_collision_sphere(Line const&, Sphere const&) noexcept;
    Ray_collision get_ray_collision_AABB(Line const&, AABB const&) noexcept;
    Ray_collision get_ray_collision_plane(Line const&, Plane const&) noexcept;
    Ray_collision get_ray_collision_disc(Line const&, Disc const&) noexcept;
    Ray_collision get_ray_collision_triangle(Line const&, glm::vec3 const*) noexcept;


    struct Rgba32 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    struct Rgb24 final {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    // float-/double-based inputs assume linear color range (i.e. 0 to 1)
    Rgba32 rgba32_from_vec4(glm::vec4 const&) noexcept;
    Rgba32 rgba32_from_f4(float, float, float, float) noexcept;
    Rgba32 rgba32_from_u32(uint32_t) noexcept;  // R at MSB

    // CPU-side mesh. note: some of the vectors can be empty (unsupplied)
    struct NewMesh {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<unsigned short> indices;

        void clear();
        void reserve(size_t);
    };

    // prints top-level mesh information (eg amount of each thing) to the stream
    std::ostream& operator<<(std::ostream&, NewMesh const&);


    // generates a textured quad with:
    //
    // - positions: Z == 0, X == [-1, 1], and Y == [-1, 1]
    // - texcoords: (0, 0) to (1, 1)
    NewMesh gen_textured_quad();

    // generates UV sphere centered at (0,0,0) with radius = 1
    NewMesh gen_untextured_uv_sphere(size_t sectors, size_t stacks);

    // generates a "Simbody" cylinder, where the bottom/top are -1.0f/+1.0f in Y
    NewMesh gen_untextured_simbody_cylinder(size_t nsides);

    // generates a "Simbody" cone, where the bottom/top are -1.0f/+1.0f in Y
    NewMesh gen_untextured_simbody_cone(size_t nsides);

    // generates 2D grid lines at Z == 0, X/Y == [-1,+1]
    NewMesh gen_NxN_grid(size_t nticks);

    // generates a single two-point line from (0,-1,0) to (0,+1,0)
    NewMesh gen_y_line();

    // generates a cube with [-1,+1] in each dimension
    NewMesh gen_cube();

    // generates the *lines* of a cube with [-1,+1] in each dimension
    NewMesh gen_cube_lines();

    // generates a circle at Z == 0, X/Y == [-1, +1] (r = 1)
    NewMesh gen_circle(size_t nsides);

    // converts a topleft-origin RELATIVE `pos` (0 to 1 in XY starting topleft) into an
    // XY location in NDC (-1 to +1 in XY starting in the middle)
    glm::vec2 topleft_relpos_to_ndcxy(glm::vec2 relpos);

    // converts a topleft-origin RELATIVE `pos` (0 to 1 in XY, starting topleft) into
    // the equivalent POINT on the front of the NDC cube (i.e. "as if" a viewer was there)
    //
    // i.e. {X_ndc, Y_ndc, -1.0f, 1.0f}
    glm::vec4 topleft_relpos_to_ndc_cube(glm::vec2 relpos);

    // camera that swivels around a focal point (e.g. 3D model viewers)
    struct Polar_perspective_camera final {
        float radius;
        float theta;
        float phi;
        glm::vec3 pan;
        float fov;
        float znear;
        float zfar;

        Polar_perspective_camera();

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

        // converts a `pos` (top-left) in the output `dims` into a line in worldspace by unprojection
        Line screenpos_to_world_ray(glm::vec2 pos, glm::vec2 dims) const noexcept;
    };

    // camera that moves freely through space (e.g. FPS games)
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
}
