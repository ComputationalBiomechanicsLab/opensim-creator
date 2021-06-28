#include "src/3d/3d.hpp"

#include "src/3d/shaders.hpp"
#include "src/config.hpp"
#include "src/utils/helpers.hpp"
#include "src/utils/stbi_wrapper.hpp"
#include "src/constants.hpp"
#include "src/3d/gl_glm.hpp"

#include <glm/gtx/norm.hpp>

#include <cstddef>
#include <iostream>
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace osc;

std::ostream& osc::operator<<(std::ostream& o, glm::vec3 const& v) {
    return o << '(' << v.x << ", " << v.y << ", " << v.z << ')';
}

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb) {
    return o << "p1 = " << aabb.p1 << ", p2 = " << aabb.p2;
}

// compute an AABB from a sequence of vertices in 3D space
template<typename TVert>
[[nodiscard]] static constexpr AABB aabb_compute_from_verts(TVert const* vs, size_t n) noexcept {
    AABB rv;

    // edge-case: no points provided
    if (n == 0) {
        rv.p1 = {0.0f, 0.0f, 0.0f};
        rv.p2 = {0.0f, 0.0f, 0.0f};
        return rv;
    }

    rv.p1 = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    rv.p2 = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    // otherwise, compute bounds
    for (size_t i = 0; i < n; ++i) {
        glm::vec3 const& pos = vs[i].pos;

        rv.p1[0] = std::min(rv.p1[0], pos[0]);
        rv.p1[1] = std::min(rv.p1[1], pos[1]);
        rv.p1[2] = std::min(rv.p1[2], pos[2]);

        rv.p2[0] = std::max(rv.p2[0], pos[0]);
        rv.p2[1] = std::max(rv.p2[1], pos[1]);
        rv.p2[2] = std::max(rv.p2[2], pos[2]);
    }

    return rv;
}

// hackily computes the bounding sphere around verts
//
// see: https://en.wikipedia.org/wiki/Bounding_sphere for better algs
template<typename TVert>
[[nodiscard]] static constexpr Sphere sphere_compute_bounding_sphere_from_verts(TVert const* vs, size_t n) noexcept {
    AABB aabb = aabb_compute_from_verts(vs, n);

    Sphere rv;
    rv.origin = (aabb.p1 + aabb.p2) / 2.0f;
    rv.radius = 0.0f;

    // edge-case: no points provided
    if (n == 0) {
        return rv;
    }

    float biggest_r2 = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        glm::vec3 const& pos = vs[i].pos;
        float r2 = glm::distance2(rv.origin, pos);
        biggest_r2 = std::max(biggest_r2, r2);
    }

    rv.radius = glm::sqrt(biggest_r2);

    return rv;
}

osc::AABB osc::aabb_from_mesh(Untextured_mesh const& m) noexcept {
    return aabb_compute_from_verts(m.verts.data(), m.verts.size());
}

osc::Sphere osc::bounding_sphere_from_mesh(Untextured_mesh const& m) noexcept {
    return sphere_compute_bounding_sphere_from_verts(m.verts.data(), m.verts.size());
}

gl::Texture_2d osc::generate_chequered_floor_texture() {
    struct Rgb {
        unsigned char r, g, b;
    };
    constexpr size_t chequer_width = 32;
    constexpr size_t chequer_height = 32;
    constexpr size_t w = 2 * chequer_width;
    constexpr size_t h = 2 * chequer_height;
    constexpr Rgb on_color = {0xe5, 0xe5, 0xe5};
    constexpr Rgb off_color = {0xde, 0xde, 0xde};

    std::array<Rgb, w * h> pixels;
    for (size_t row = 0; row < h; ++row) {
        size_t row_start = row * w;
        bool y_on = (row / chequer_height) % 2 == 0;
        for (size_t col = 0; col < w; ++col) {
            bool x_on = (col / chequer_width) % 2 == 0;
            pixels[row_start + col] = y_on ^ x_on ? on_color : off_color;
        }
    }

    gl::Texture_2d rv;
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(rv.type, rv.handle());
    glTexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(rv.type);
    gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return rv;
}

Image_texture osc::load_image_as_texture(char const* path, Tex_flags flags) {
    gl::Texture_2d t;

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi::set_flip_vertically_on_load(true);
    }

    auto img = stbi::Image::load(path);

    if (!img) {
        std::stringstream ss;
        ss << path << ": error loading image: " << stbi::failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    if (flags & TexFlag_Flip_Pixels_Vertically) {
        stbi::set_flip_vertically_on_load(false);
    }

    GLenum internalFormat;
    GLenum format;
    if (img->channels == 1) {
        internalFormat = GL_RED;
        format = GL_RED;
    } else if (img->channels == 3) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB : GL_RGB;
        format = GL_RGB;
    } else if (img->channels == 4) {
        internalFormat = flags & TexFlag_SRGB ? GL_SRGB_ALPHA : GL_RGBA;
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img->channels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    gl::BindTexture(t.type, t.handle());
    gl::TexImage2D(t.type, 0, internalFormat, img->width, img->height, 0, format, GL_UNSIGNED_BYTE, img->data);
    glGenerateMipmap(t.type);

    return Image_texture{std::move(t), img->width, img->height, img->channels};
}

// helper method: load a file into an image and send it to OpenGL
static void load_cubemap_surface(char const* path, GLenum target) {
    auto img = osc::stbi::Image::load(path);

    if (!img) {
        std::stringstream ss;
        ss << path << ": error loading cubemap surface: " << osc::stbi::failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    GLenum format;
    if (img->channels == 1) {
        format = GL_RED;
    } else if (img->channels == 3) {
        format = GL_RGB;
    } else if (img->channels == 4) {
        format = GL_RGBA;
    } else {
        std::stringstream msg;
        msg << path << ": error: contains " << img->channels
            << " color channels (the implementation doesn't know how to handle this)";
        throw std::runtime_error{std::move(msg).str()};
    }

    glTexImage2D(target, 0, format, img->width, img->height, 0, format, GL_UNSIGNED_BYTE, img->data);
}

gl::Texture_cubemap osc::load_cubemap(
    char const* path_pos_x,
    char const* path_neg_x,
    char const* path_pos_y,
    char const* path_neg_y,
    char const* path_pos_z,
    char const* path_neg_z) {

    stbi::set_flip_vertically_on_load(false);

    gl::Texture_cubemap rv;
    gl::BindTexture(rv);

    load_cubemap_surface(path_pos_x, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    load_cubemap_surface(path_neg_x, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    load_cubemap_surface(path_pos_y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    load_cubemap_surface(path_neg_y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    load_cubemap_surface(path_pos_z, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    load_cubemap_surface(path_neg_z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    /**
     * From: https://learnopengl.com/Advanced-OpenGL/Cubemaps
     *
     * Don't be scared by the GL_TEXTURE_WRAP_R, this simply sets the wrapping
     * method for the texture's R coordinate which corresponds to the texture's
     * 3rd dimension (like z for positions). We set the wrapping method to
     * GL_CLAMP_TO_EDGE since texture coordinates that are exactly between two
     * faces may not hit an exact face (due to some hardware limitations) so
     * by using GL_CLAMP_TO_EDGE OpenGL always returns their edge values
     * whenever we sample between faces.
     */
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return rv;
}

osc::GPU_mesh::GPU_mesh(Untextured_mesh const& um) :
    verts(reinterpret_cast<GLubyte const*>(um.verts.data()), sizeof(Untextured_vert) * um.verts.size()),
    indices(um.indices),
    instances{},
    main_vao(Gouraud_mrt_shader::create_vao<decltype(verts), Untextured_vert>(verts, indices, instances)),
    normal_vao(Normals_shader::create_vao<decltype(verts), Untextured_vert>(verts)),
    is_textured{false} {
}

osc::GPU_mesh::GPU_mesh(Textured_mesh const& tm) :
    verts(reinterpret_cast<GLubyte const*>(tm.verts.data()), sizeof(Textured_vert) * tm.verts.size()),
    indices(tm.indices),
    instances{},
    main_vao(Gouraud_mrt_shader::create_vao<decltype(verts), Textured_vert>(verts, indices, instances)),
    normal_vao(Normals_shader::create_vao<decltype(verts), Textured_vert>(verts)),
    is_textured{true} {
}

// Returns triangles of a "unit" (radius = 1.0f, origin = 0,0,0) sphere
static void unit_sphere_triangles(Untextured_mesh& out) {
    out.clear();

    // this is a shitty alg that produces a shitty UV sphere. I don't have
    // enough time to implement something better, like an isosphere, or
    // something like a patched sphere:
    //
    // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
    //
    // This one is adapted from:
    //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere

    size_t sectors = 12;
    size_t stacks = 12;

    // polar coords, with [0, 0, -1] pointing towards the screen with polar
    // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
    // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
    std::vector<Untextured_vert> points;

    float theta_step = 2.0f * pi_f / sectors;
    float phi_step = pi_f / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = pi_f / 2.0f - static_cast<float>(stack) * phi_step;
        float y = sinf(phi);

        for (unsigned sector = 0; sector <= sectors; ++sector) {
            float theta = sector * theta_step;
            float x = sinf(theta) * cosf(phi);
            float z = -cosf(theta) * cosf(phi);
            glm::vec3 pos{x, y, z};
            glm::vec3 normal{pos};
            points.push_back({pos, normal});
        }
    }

    // the points are not triangles. They are *points of a triangle*, so the
    // points must be triangulated

    for (size_t stack = 0; stack < stacks; ++stack) {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)
            Untextured_vert p1 = points.at(k1);
            Untextured_vert p2 = points.at(k2);
            Untextured_vert p1_plus1 = points.at(k1 + 1u);
            Untextured_vert p2_plus1 = points.at(k2 + 1u);

            if (stack != 0) {
                out.verts.push_back(p1);
                out.verts.push_back(p1_plus1);
                out.verts.push_back(p2);
            }

            if (stack != (stacks - 1)) {
                out.verts.push_back(p1_plus1);
                out.verts.push_back(p2_plus1);
                out.verts.push_back(p2);
            }
        }
    }

    generate_1to1_indices_for_verts(out);
}

static void simbody_cylinder_triangles(Untextured_mesh& out) {
    size_t num_sides = 16;

    out.clear();
    out.verts.reserve(2 * num_sides + 2 * num_sides);

    float step_angle = (2.0f * pi_f) / num_sides;
    float top_y = +1.0f;
    float bottom_y = -1.0f;

    // top
    {
        glm::vec3 normal = {0.0f, 1.0f, 0.0f};
        Untextured_vert top_middle{{0.0f, top_y, 0.0f}, normal};
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: these are wound CCW for backface culling
            out.verts.push_back(top_middle);
            out.verts.push_back({glm::vec3(cosf(theta_end), top_y, sinf(theta_end)), normal});
            out.verts.push_back({glm::vec3(cosf(theta_start), top_y, sinf(theta_start)), normal});
        }
    }

    // bottom
    {
        glm::vec3 bottom_normal{0.0f, -1.0f, 0.0f};
        Untextured_vert top_middle{{0.0f, bottom_y, 0.0f}, bottom_normal};
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = (i + 1) * step_angle;

            // note: these are wound CCW for backface culling
            out.verts.push_back(top_middle);
            out.verts.push_back({
                glm::vec3(cosf(theta_start), bottom_y, sinf(theta_start)),
                bottom_normal,
            });
            out.verts.push_back({
                glm::vec3(cosf(theta_end), bottom_y, sinf(theta_end)),
                bottom_normal,
            });
        }
    }

    // sides
    {
        float norm_start = step_angle / 2.0f;
        for (auto i = 0U; i < num_sides; ++i) {
            float theta_start = i * step_angle;
            float theta_end = theta_start + step_angle;
            float norm_theta = theta_start + norm_start;

            glm::vec3 normal(cosf(norm_theta), 0.0f, sinf(norm_theta));
            glm::vec3 top1(cosf(theta_start), top_y, sinf(theta_start));
            glm::vec3 top2(cosf(theta_end), top_y, sinf(theta_end));

            glm::vec3 bottom1 = top1;
            bottom1.y = bottom_y;
            glm::vec3 bottom2 = top2;
            bottom2.y = bottom_y;

            // draw 2 triangles per quad cylinder side
            //
            // note: these are wound CCW for backface culling
            out.verts.push_back({top1, normal});
            out.verts.push_back({top2, normal});
            out.verts.push_back({bottom1, normal});

            out.verts.push_back({bottom2, normal});
            out.verts.push_back({bottom1, normal});
            out.verts.push_back({top2, normal});
        }
    }

    generate_1to1_indices_for_verts(out);
}

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<Textured_vert, 36> shaded_textured_cube_verts = {{
    // back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},  // top-left
    // front face
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    // left face
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
    {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
    // right face
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-right
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // top-left
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-left
    // bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},  // top-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-left
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // bottom-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // top-right
    // top face
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // top-right
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // bottom-left
}};

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Textured_vert, 6> _shaded_textured_quad_verts = {{
    // CCW winding (culling)
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // bottom-right
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right

    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // top-right
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom-left
}};

static void simbody_brick_triangles(Untextured_mesh& out) {
    out.clear();

    for (Textured_vert v : shaded_textured_cube_verts) {
        out.verts.push_back({v.pos, v.normal});
    }

    generate_1to1_indices_for_verts(out);
}

static void generate_floor_quad(Textured_mesh& out) {
    out.clear();

    for (Textured_vert const& v : _shaded_textured_quad_verts) {
        Textured_vert& tv = out.verts.emplace_back(v);
        tv.texcoord *= 200.0f;
    }

    generate_1to1_indices_for_verts(out);
}

static void generate_NxN_grid(size_t n, Untextured_mesh& out) {
    static constexpr float z = 0.0f;
    static constexpr float min = -1.0f;
    static constexpr float max = 1.0f;

    size_t lines_per_dimension = n;
    float step_size = (max - min) / static_cast<float>(lines_per_dimension - 1);
    size_t num_lines = 2 * lines_per_dimension;
    size_t num_points = 2 * num_lines;

    out.clear();
    out.verts.resize(num_points);

    glm::vec3 normal = {0.0f, 0.0f, 0.0f};  // same for all

    size_t idx = 0;

    // lines parallel to X axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float y = min + i * step_size;

        Untextured_vert& p1 = out.verts[idx++];
        p1.pos = {-1.0f, y, z};
        p1.normal = normal;

        Untextured_vert& p2 = out.verts[idx++];
        p2.pos = {1.0f, y, z};
        p2.normal = normal;
    }

    // lines parallel to Y axis
    for (size_t i = 0; i < lines_per_dimension; ++i) {
        float x = min + i * step_size;

        Untextured_vert& p1 = out.verts[idx++];
        p1.pos = {x, -1.0f, z};
        p1.normal = normal;

        Untextured_vert& p2 = out.verts[idx++];
        p2.pos = {x, 1.0f, z};
        p2.normal = normal;
    }

    generate_1to1_indices_for_verts(out);
}

static void generate_y_line(Untextured_mesh& out) {
    out.clear();
    out.verts.push_back({{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
    out.verts.push_back({{0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
    generate_1to1_indices_for_verts(out);
}

// a cube wire mesh, suitable for GL_LINES drawing
//
// a pair of verts per edge of the cube. The cube has 12 edges, so 24 lines
static constexpr std::array<Untextured_vert, 24> g_cube_edge_lines = {{
    // back

    // back bottom left -> back bottom right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back bottom right -> back top right
    {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back top right -> back top left
    {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back top left -> back bottom left
    {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // front

    // front bottom left -> front bottom right
    {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front bottom right -> front top right
    {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front top right -> front top left
    {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front top left -> front bottom left
    {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front-to-back edges

    // front bottom left -> back bottom left
    {{-1.0f, -1.0f, +1.0f}, {-1.0f, -1.0f, +1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}},

    // front bottom right -> back bottom right
    {{+1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}},

    // front top left -> back top left
    {{-1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f}},

    // front top right -> back top right
    {{+1.0f, +1.0f, +1.0f}, {+1.0f, +1.0f, +1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {+1.0f, +1.0f, -1.0f}}
}};

static void generate_cube_lines(Untextured_mesh& out) {
    out.clear();
    out.indices.reserve(g_cube_edge_lines.size());
    out.verts.reserve(g_cube_edge_lines.size());

    for (size_t i = 0; i < g_cube_edge_lines.size(); ++i) {
        out.indices.push_back(static_cast<elidx_t>(i));
        out.verts.push_back(g_cube_edge_lines[i]);
    }
}

osc::GPU_storage::GPU_storage() :
    // shaders
    shader_gouraud{new Gouraud_mrt_shader{}},
    shader_normals{new Normals_shader{}},
    shader_pts{new Plain_texture_shader{}},
    shader_cpts{new Colormapped_plain_texture_shader{}},
    shader_eds{new Edge_detection_shader{}},
    shader_skip_msxaa{new Skip_msxaa_blitter_shader{}} {

    // untextured preallocated meshes
    {
        Untextured_mesh utm;

        unit_sphere_triangles(utm);
        meshes.emplace_back(utm);
        simbody_sphere_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        simbody_cylinder_triangles(utm);
        meshes.emplace_back(utm);
        simbody_cylinder_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        simbody_brick_triangles(utm);
        meshes.emplace_back(utm);
        simbody_cube_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        generate_NxN_grid(25, utm);
        meshes.emplace_back(utm);
        grid_25x25_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        generate_y_line(utm);
        meshes.emplace_back(utm);
        yline_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        generate_cube_lines(utm);
        meshes.emplace_back(utm);
        cube_lines_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();
    }

    // textured preallocated meshes
    {
        Textured_mesh tm;

        generate_floor_quad(tm);
        meshes.emplace_back(tm);
        floor_quad_idx = Meshidx::from_index(meshes.size() - 1);
        tm.clear();

        for (Textured_vert const& tv : _shaded_textured_quad_verts) {
            tm.verts.push_back(tv);
        }
        generate_1to1_indices_for_verts(tm);
        meshes.emplace_back(tm);
        quad_idx = Meshidx::from_index(meshes.size() - 1);
        quad_vbo = gl::Array_buffer<Textured_vert>(tm.verts);
    }

    // preallocated textures
    textures.push_back(generate_chequered_floor_texture());
    chequer_idx = Texidx::from_index(textures.size() - 1);

    eds_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
    skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
    pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);
    cpts_quad_vao = Colormapped_plain_texture_shader::create_vao(quad_vbo);
}

osc::GPU_storage::GPU_storage(GPU_storage&&) noexcept = default;

osc::GPU_storage& osc::GPU_storage::operator=(GPU_storage&&) noexcept = default;

osc::GPU_storage::~GPU_storage() noexcept = default;

osc::Render_target::Render_target(int w_, int h_, int samples_) :
    w{w_},
    h{h_},
    samples{samples_},

    scene_rgba{[this]() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA, w, h);
        return rv;
    }()},

    scene_passthrough{[this]() {
        gl::Texture_2d_multisample rv;
        gl::BindTexture(rv);
        glTexImage2DMultisample(rv.type, samples, GL_RGB, w, h, GL_TRUE);
        return rv;
    }()},

    scene_depth24stencil8{[this]() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, w, h);
        return rv;
    }()},

    scene_fbo{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_rgba);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, scene_passthrough, 0);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, scene_depth24stencil8);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_nomsxaa{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        return rv;
    }()},

    passthrough_fbo{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, passthrough_nomsxaa, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_pbos{{
        {0x00, 0x00, 0x00, 0x00},  // rgba
        {0x00, 0x00, 0x00, 0x00},  // rgba
    }},

    passthrough_pbo_cur{0},  // 0/1

    scene_tex_resolved{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        return rv;
    }()},

    scene_fbo_resolved{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_tex_resolved, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_tex_resolved{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        return rv;
    }()},

    passthrough_fbo_resolved{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, passthrough_tex_resolved, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    hittest_result{0x00, 0x00, 0x00} {
}

void osc::Render_target::reconfigure(int w_, int h_, int samples_) {
    if (w != w_ || h != h_ || samples != samples_) {
        *this = Render_target{w_, h_, samples_};
    }
}

static bool optimal_orderering(Mesh_instance const& m1, Mesh_instance const& m2) {
    if (m1.texidx != m2.texidx) {
        // third, sort by texture, because even though we *could* render a batch of
        // instances with the same mesh in one draw call, some of those meshes might
        // be textured, and textures can't be instanced (so the drawcall must be split
        // into separate calls etc.)
        return m1.texidx < m2.texidx;
    } else {
        // fourth, sort by flags, because the flags can change a draw call (e.g.
        // although we are drawing the same mesh with the same texture, this
        // partiular *instance* should be drawn with GL_TRIANGLES or GL_POINTS)
        //
        // like textures, if the drawcall-affecting flags are different, we have
        // to split the drawcall (e.g. draw TRIANGLES then draw POINTS)
        return m1.flags < m2.flags;
    }
}

void osc::optimize(Drawlist& drawlist) noexcept {
    for (auto& lst : drawlist._nonopaque_by_meshidx) {
        std::sort(lst.begin(), lst.end(), optimal_orderering);
    }
    for (auto& lst : drawlist._opaque_by_meshidx) {
        std::sort(lst.begin(), lst.end(), optimal_orderering);
    }
}

void osc::draw_scene(GPU_storage& storage, Render_params const& params, Drawlist const& drawlist, Render_target& out) {
    gl::Viewport(0, 0, out.w, out.h);

    // bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    gl::BindFramebuffer(GL_FRAMEBUFFER, out.scene_fbo);

    // clear the scene FBO's draw buffers for a new draw call
    //
    //   - COLOR0: main scene render: fill in background
    //   - COLOR1: RGB passthrough (selection logic + rim alpa): blank out all channels
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(params.background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // handle wireframe mode: should only be enabled for scene + floor render: the other
    // renders will render to a screen-sized quad
    GLenum original_poly_mode = gl::GetEnum(GL_POLYGON_MODE);
    if (params.flags & DrawcallFlags_WireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // render the scene to the FBO using a multiple-render-target (MRT) multisampled
    // (MSXAAed) shader.
    //
    // FBO outputs are:
    //
    // - COLOR0: main target: multisampled scene geometry
    //     - the input color is Gouraud-shaded based on light parameters etc.
    // - COLOR1: RGB passthrough: written to output as-is
    //     - the input color encodes the selected component index (RG) and the rim
    //       alpha (B). It's used in downstream steps
    if (params.flags & DrawcallFlags_DrawSceneGeometry) {
        Gouraud_mrt_shader& shader = *storage.shader_gouraud;

        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);
        gl::Uniform(shader.uLightDir, params.light_dir);
        gl::Uniform(shader.uLightColor, params.light_rgb);
        gl::Uniform(shader.uViewPos, params.view_pos);

        // blending:
        //     COLOR0 should be blended because OpenSim scenes can contain blending
        //     COLOR1 should never be blended: it's a value for the top-most fragment
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);

        if (params.flags & DrawcallFlags_UseInstancedRenderer) {
            auto draw_els_with_same_meshidx = [&](std::vector<Mesh_instance> const& instances) {
                size_t nmeshes = instances.size();
                Mesh_instance const* meshes = instances.data();
                size_t pos = 0;

                // batch instances into as few drawcalls as possible
                while (pos < nmeshes) {
                    Meshidx meshidx = meshes[pos].meshidx;
                    Texidx texidx = meshes[pos].texidx;
                    Instance_flags flags = meshes[pos].flags;
                    size_t end = pos + 1;

                    while (end < nmeshes && meshes[end].texidx == texidx && meshes[end].flags == flags) {
                        ++end;
                    }

                    // [pos, end) contains instances with the same meshid + textureid + flags

                    // assign texture (if necessary)
                    if (texidx.is_valid()) {
                        gl::Uniform(shader.uIsTextured, true);
                        gl::ActiveTexture(GL_TEXTURE0);
                        gl::BindTexture(storage.textures[texidx.as_index()]);
                        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                    } else {
                        gl::Uniform(shader.uIsTextured, false);
                    }

                    // assign flags
                    gl::Uniform(shader.uIsShaded, !flags.skip_shading());
                    gl::Uniform(shader.uSkipVP, flags.skip_vp());

                    // upload instance data
                    GPU_mesh& gm = storage.meshes[meshidx.as_index()];
                    gm.instances.assign(meshes + pos, end - pos);

                    // do drawcall
                    gl::BindVertexArray(gm.main_vao);
                    glDrawElementsInstanced(
                        flags.mode(), gm.indices.sizei(), gl::index_type(gm.indices), nullptr, static_cast<GLsizei>(end - pos));

                    pos = end;
                }
            };

            for (auto const& lst : drawlist._opaque_by_meshidx) {
                draw_els_with_same_meshidx(lst);
            }

            for (auto const& lst : drawlist._nonopaque_by_meshidx) {
                draw_els_with_same_meshidx(lst);
            }

            gl::BindVertexArray();
        } else {
            // perform (slower) one-drawcall-per-item rendering
            //
            // this is here mostly for perf comparison and debugging
            drawlist.for_each([&](Mesh_instance const& mi) {
                // texture-related stuff
                if (mi.texidx.is_valid()) {
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(storage.textures[mi.texidx.as_index()]);
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                // flag-related stuff
                gl::Uniform(shader.uIsShaded, !mi.flags.skip_shading());
                gl::Uniform(shader.uSkipVP, mi.flags.skip_vp());
                GLenum mode = mi.flags.mode();

                GPU_mesh& gm = storage.meshes[mi.meshidx.as_index()];
                gm.instances.assign(&mi, 1);
                gl::BindVertexArray(gm.main_vao);
                glDrawElementsInstanced(mode, gm.indices.sizei(), gl::index_type(gm.indices), nullptr, static_cast<GLsizei>(1));
                gl::BindVertexArray();
            });
        }

        glDisablei(GL_BLEND, 0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

    // (optional): render scene normals into COLOR0
    if (params.flags & DrawcallFlags_ShowMeshNormals) {
        Normals_shader& shader = *storage.shader_normals;
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);

        drawlist.for_each([&](Mesh_instance const& mi) {
            GPU_mesh& gm = storage.meshes[mi.meshidx.as_index()];
            gl::Uniform(shader.uModelMat, mi.model_xform);
            gl::Uniform(shader.uNormalMat, mi.normal_xform);
            gl::BindVertexArray(gm.normal_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, gm.verts.sizei() / (gm.is_textured ? sizeof(Textured_vert) : sizeof(Untextured_vert)));
        });

        gl::BindVertexArray();
    }

    // perform passthrough hit testing
    //
    // in the previous draw call, COLOR1's RGB channels encoded arbitrary passthrough data
    // Extracting that pixel value (without MSXAA blending) and decoding it yields the
    // user-supplied data
    //
    // this makes it possible for renderer users (e.g. OpenSim model renderer) to encode
    // model information (e.g. "a component index") into screenspace

    out.hittest_result = Passthrough_data{0x00, 0x00, 0x00};

    if (params.hittest.x >= 0 &&
        params.hittest.y >= 0 &&
        params.flags & DrawcallFlags_PerformPassthroughHitTest) {

        // (temporarily) set the OpenGL viewport to a small square around the hit testing
        // location
        //
        // this causes the subsequent draw call to only run the fragment shader around where
        // we actually care about
        gl::Viewport(params.hittest.x - 1, params.hittest.y - 1, 3, 3);

        // bind to a non-MSXAAed FBO
        gl::BindFramebuffer(GL_FRAMEBUFFER, out.passthrough_fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // use a specialized shader that is MSXAA-aware to blit exactly one non-blended AA
        // sample from COLOR1 to the output
        //
        // by deliberately avoiding MSXAA, every value in this output should be exactly the
        // same as the passthrough value provided by the caller
        Skip_msxaa_blitter_shader& shader = *storage.shader_skip_msxaa;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(out.scene_passthrough);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(storage.skip_msxaa_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        gl::BindVertexArray();

        // reset viewport
        glViewport(0, 0, out.w, out.h);

        // the FBO now contains non-MSXAAed version of COLOR1

        // read the pixel under the mouse
        //
        // - you *could* just read the value directly from the FBO with `glReadPixels`, which is
        //   what the first iteration of this alg. did (non optimized)
        //
        // - However, that glReadPixels call will screw performance. On my machine (Ryzen1600 /w
        //   Geforce 1060), it costs around 30 % FPS (300 FPS --> 200 FPS)
        //
        // - This isn't because the transfer is expensive--it's just a single pixel, after all--but
        //   because reading the pixel forces the OpenGL driver to flush all pending rendering
        //   operations to the FBO (a "pipeline stall")
        //
        // - So this algorithm uses a crafty trick, which is to use two pixel buffer objects (PBOs)
        //   to asynchronously transfer the pixel *from the previous frame* into CPU memory using
        //   asynchronous DMA. The trick uses two PBOs, which each of which are either:
        //
        //   1. Requesting the pixel value (via glReadPixel). The OpenGL spec does *not* require
        //      that the PBO is populated once `glReadPixel` returns, so this does not cause a
        //      pipeline stall
        //
        //   2. Mapping the PBO that requested a pixel value **on the last frame**. The OpenGL spec
        //      requires that this PBO is populated once the mapping is enabled, so this will
        //      stall the pipeline. However, that pipeline stall will be on the *previous* frame
        //      which is less costly to stall on

        if (params.flags & DrawcallFlags_UseOptimizedButDelayed1FrameHitTest) {
            size_t reader = out.passthrough_pbo_cur % out.passthrough_pbos.size();
            size_t mapper = (out.passthrough_pbo_cur + 1) % out.passthrough_pbos.size();

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(out.passthrough_pbos[reader]);
            glReadPixels(params.hittest.x, params.hittest.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read *last frame's* pixel
            gl::BindBuffer(out.passthrough_pbos[mapper]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            // note: these values are the *last frame*'s
            out.hittest_result.b0 = src[0];
            out.hittest_result.b1 = src[1];
            out.hittest_result.rim_alpha = src[2];

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip PBOs ready for next frame
            out.passthrough_pbo_cur = (out.passthrough_pbo_cur+ 1) % out.passthrough_pbos.size();
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor
            //
            // this is kept here so that people can try it out if selection logic is acting
            // bizzarely (e.g. because it is delayed one frame)

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            GLubyte rgba[3]{};
            glReadPixels(params.hittest.x, params.hittest.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgba);

            out.hittest_result.b0 = rgba[0];
            out.hittest_result.b1 = rgba[1];
            out.hittest_result.rim_alpha = rgba[2];
        }
    }

    // resolve MSXAA in COLOR0 to output texture
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR0. We are "done" with
    // COLOR0. You might expect we can directly blit it to the output, but that
    // seems to explode with some OpenGL drivers (e.g. Intel iGPUs like UHD 620)
    {
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, out.scene_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, out.scene_fbo_resolved);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, out.w, out.h, 0, 0, out.w, out.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // resolve MSXAA in COLOR1
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR1 into non-MSXAAed textures
    // that the edge-detection shader can sample normally
    {
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, out.scene_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, out.passthrough_fbo_resolved);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, out.w, out.h, 0, 0, out.w, out.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // bind to output texture: all further drawing goes onto it
    gl::BindFramebuffer(GL_FRAMEBUFFER, out.scene_fbo_resolved);

    // draw rims highlights onto the output
    //
    // COLOR1's alpha channel contains *filled in shapes* for each element in the scene that
    // should be rim-shaded. Those shapes are exactly the same as the scene geometry, so showing
    // them as-is would be pointless (they'd either entirely occlude, or be occluded by, the scene)
    //
    // rim-highlighting puts a rim around the outer edge of the geometry. There are various tricks
    // for doing this, such as rendering the geometry twice - the second time backface-enlarged
    // slightly, or holding onto two versions of every mesh (normal mesh, normal-scaled mesh),
    // but those techniques each have drawbacks (e.g. more draw calls, fails with non-convex
    // geometry, behaves bizzarely with non-centered meshes)
    //
    // this technique performs rim highlighting in screen-space using a standard edge-detection
    // kernel. The drawback of this is that every single pixel in the screen has to be
    // edge-detected, and the rims are in screen-space, rather than world space (so they don't
    // "zoom out" as if they were "in the scene"). However, GPUs are fairly efficient at running
    // branchless kernel lookups over a screen, so it isn't as expensive as you think
    if (params.flags & DrawcallFlags_DrawRims) {
        Edge_detection_shader& shader = *storage.shader_eds;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(out.passthrough_tex_resolved);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, params.rim_rgba);

        float rim_thickness = 2.0f / std::max(out.w, out.h);

        gl::Uniform(shader.uRimThickness, rim_thickness);

        glEnable(GL_BLEND);  // rims can have alpha
        glDisable(GL_DEPTH_TEST);
        gl::BindVertexArray(storage.eds_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        gl::BindVertexArray();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    // render debug quads onto output (if applicable)
    if (params.flags & DrawcallFlags_DrawDebugQuads) {
        Colormapped_plain_texture_shader& cpts = *storage.shader_cpts;
        gl::UseProgram(cpts.p);
        gl::BindVertexArray(storage.pts_quad_vao);

        // COLOR1 quad (RGB)
        if (true) {
            glm::mat4 row1 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.80f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            gl::Uniform(cpts.uMVP, row1);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(out.passthrough_tex_resolved);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        }

        // COLOR1 quad (A)
        if (true) {
            glm::mat4 row2 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.40f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            glm::mat4 alpha2rgb = {
                // column-major
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
            };

            gl::Uniform(cpts.uMVP, row2);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(out.passthrough_tex_resolved);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, alpha2rgb);
            gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        }

        gl::BindVertexArray();
    }

    // bind back to the original framebuffer (assumed to be window)
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
}
