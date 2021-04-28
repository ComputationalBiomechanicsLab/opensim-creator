#pragma once

#include "gl.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// gl_extensions: stuff that isn't strictly required to use OpenGL, but it nice to have
namespace gl {
    inline void Uniform(Uniform_int const& u, GLsizei n, GLint const* data) {
        glUniform1iv(u.geti(), n, data);
    }

    inline void Uniform(Uniform_mat3& u, glm::mat3 const& mat) noexcept {
        glUniformMatrix3fv(u.geti(), 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_vec4& u, glm::vec4 const& v) noexcept {
        glUniform4fv(u.geti(), 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, glm::vec3 const& v) noexcept {
        glUniform3fv(u.geti(), 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, float x, float y, float z) noexcept {
        glUniform3f(u.geti(), x, y, z);
    }

    inline void Uniform(Uniform_vec3& u, float const vs[3]) {
        glUniform3fv(u.geti(), 1, vs);
    }

    inline void Uniform(Uniform_vec3& u, GLsizei n, glm::vec3 const* vs) noexcept {
        static_assert(sizeof(glm::vec3) == 3 * sizeof(GLfloat));
        glUniform3fv(u.geti(), n, glm::value_ptr(*vs));
    }

    // set a uniform array of vec3s from a userspace container type (e.g. vector<glm::vec3>)
    template<typename Container, size_t N>
    inline std::enable_if_t<std::is_same_v<glm::vec3, typename Container::value_type>, void>
        Uniform(Uniform_array<glsl::vec3, N>& u, Container& container) {
        assert(container.size() == N);
        glUniform3fv(u.geti(), static_cast<GLsizei>(container.size()), glm::value_ptr(*container.data()));
    }

    inline void Uniform(Uniform_mat4& u, glm::mat4 const& mat) noexcept {
        glUniformMatrix4fv(u.geti(), 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_mat4& u, GLsizei n, glm::mat4 const* first) noexcept {
        static_assert(sizeof(glm::mat4) == 16 * sizeof(GLfloat));
        glUniformMatrix4fv(u.geti(), n, false, glm::value_ptr(*first));
    }

    struct Uniform_identity_val_tag {};
    inline Uniform_identity_val_tag identity_val;

    inline void Uniform(Uniform_mat4& u, Uniform_identity_val_tag) noexcept {
        Uniform(u, glm::identity<glm::mat4>());
    }

    inline void Uniform(Uniform_vec2& u, glm::vec2 const& v) noexcept {
        glUniform2fv(u.geti(), 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec2& u, GLsizei n, glm::vec2 const* vs) noexcept {
        static_assert(sizeof(glm::vec2) == 2 * sizeof(GLfloat));
        glUniform2fv(u.geti(), n, glm::value_ptr(*vs));
    }

    template<typename Container, size_t N>
    inline std::enable_if_t<std::is_same_v<glm::vec2, typename Container::value_type>, void>
        Uniform(Uniform_array<glsl::vec2, N>& u, Container const& container) {
        glUniform2fv(u.geti(), static_cast<GLsizei>(container.size()), glm::value_ptr(container.data()));
    }

    inline void Uniform(Uniform_sampler2d& u, GLint v) {
        glUniform1i(u.geti(), v);
    }

    inline void Uniform(Uniform_sampler2DMS& u, GLint v) {
        glUniform1i(u.geti(), v);
    }

    inline void Uniform(Uniform_bool& u, bool v) {
        glUniform1i(u.geti(), v);
    }

    inline void ClearColor(glm::vec4 const& v) {
        ClearColor(v[0], v[1], v[2], v[3]);
    }
}
