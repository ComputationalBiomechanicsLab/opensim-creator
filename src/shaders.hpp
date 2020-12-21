#pragma once

#include "logl_common.hpp"

struct Shaded_textured_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};
static_assert(sizeof(Shaded_textured_vert) == 8*sizeof(float));

struct Plain_vert final {
    glm::vec3 pos;
};
static_assert(sizeof(Plain_vert) == 3*sizeof(float));

struct Colored_vert final {
    glm::vec3 pos;
    glm::vec3 color;
};
static_assert(sizeof(Colored_vert) == 6*sizeof(float));


static gl::Vertex_array create_vao(
        Blinn_phong_textured_shader& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, norm)));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, uv)));
    gl::EnableVertexAttribArray(s.aTexCoords);

    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with basic texture mapping (no lighting etc.)
struct Plain_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("plain_texture_shader.vert"),
        gl::CompileFragmentShaderResource("plain_texture_shader.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTextureCoord = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");

    gl::Uniform_sampler2d uTexture1 = gl::GetUniformLocation(p, "texture1");
};

static gl::Vertex_array create_vao(
        Plain_texture_shader& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, uv)));
    gl::EnableVertexAttribArray(s.aTextureCoord);

    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with a solid, uniform-defined, color
struct Uniform_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("uniform_color_shader.vert"),
        gl::CompileFragmentShaderResource("uniform_color_shader.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");

    gl::Uniform_vec3 uColor = gl::GetUniformLocation(p, "color");
};

static gl::Vertex_array create_vao(
        Uniform_color_shader& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);

    gl::BindVertexArray();

    return vao;
}

static gl::Vertex_array create_vao(
        Uniform_color_shader& s,
        gl::Sized_array_buffer<Plain_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Plain_vert), reinterpret_cast<void*>(offsetof(Plain_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);

    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with an attribute-defined color
struct Attribute_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("attribute_color_shader.vert"),
        gl::CompileFragmentShaderResource("attribute_color_shader.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aColor = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
};

static gl::Vertex_array create_vao(
        Attribute_color_shader& s,
        gl::Sized_array_buffer<Colored_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Colored_vert), reinterpret_cast<void*>(offsetof(Colored_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aColor, 3, GL_FLOAT, GL_FALSE, sizeof(Colored_vert), reinterpret_cast<void*>(offsetof(Colored_vert, color)));
    gl::EnableVertexAttribArray(s.aColor);

    gl::BindVertexArray();

    return vao;
}

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<Shaded_textured_vert, 36> shaded_textured_cube_verts = {{
    // back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}}, // top-left
    // front face
    {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}, // top-left
    {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    // left face
    {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
    {{-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
    {{-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
    {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
    // right face
    {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
    {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
    {{ 1.0f, -1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-left
    // bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
    {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
    // top face
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
    {{ 1.0f,  1.0f , 1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
    {{-1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}  // bottom-left
}};

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Shaded_textured_vert, 6> shaded_textured_quad_verts = {{
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}  // top-left
}};

static constexpr std::array<Plain_vert, 6> plain_axes_verts = {{
    {{0.0f, 0.0f, 0.0f}}, // x origin
    {{1.0f, 0.0f, 0.0f}}, // x

    {{0.0f, 0.0f, 0.0f}}, // y origin
    {{0.0f, 1.0f, 0.0f}}, // y

    {{0.0f, 0.0f, 0.0f}}, // z origin
    {{0.0f, 0.0f, 1.0f}}  // z
}};

static constexpr std::array<Colored_vert, 6> colored_axes_verts = {{
    // x axis (red)
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},

    // y axis (green)
    {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

    // z axis (blue)
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
}};
