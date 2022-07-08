#include "GraphicsHelpers.hpp"

#include "src/Graphics/Shaders/SolidColorShader.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace
{
    // assumes `pos` is in-bounds
    void DrawBVHRecursive(
        osc::Mesh& cube,
        gl::UniformMat4& mtxUniform,
        osc::BVH const& bvh,
        int pos)
    {
        osc::BVHNode const& n = bvh.nodes[pos];

        glm::vec3 halfWidths = Dimensions(n.bounds) / 2.0f;
        glm::vec3 center = Midpoint(n.bounds);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;
        gl::Uniform(mtxUniform, mmtx);
        cube.Draw();

        if (n.nlhs >= 0)
        {
            // it's an internal node
            DrawBVHRecursive(cube, mtxUniform, bvh, pos+1);
            DrawBVHRecursive(cube, mtxUniform, bvh, pos+n.nlhs+1);
        }
    }

    void DrawGrid(glm::mat4 const& rotationMatrix, glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
    {
        auto& shader = osc::App::shader<osc::SolidColorShader>();

        gl::UseProgram(shader.program);
        gl::Uniform(shader.uModel, glm::scale(rotationMatrix, {50.0f, 50.0f, 1.0f}));
        gl::Uniform(shader.uView, viewMtx);
        gl::Uniform(shader.uProjection, projMtx);
        gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
        auto grid = osc::App::meshes().get100x100GridMesh();
        gl::BindVertexArray(grid->GetVertexArray());
        grid->Draw();
        gl::BindVertexArray();
    }
}

void osc::DrawBVH(
    BVH const& sceneBVH,
    glm::mat4 const& viewMtx,
    glm::mat4 const& projMtx)
{
    if (sceneBVH.nodes.empty())
    {
        return;
    }

    auto& shader = osc::App::shader<osc::SolidColorShader>();

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, projMtx);
    gl::Uniform(shader.uView, viewMtx);
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    auto cube = osc::App::meshes().getCubeWireMesh();
    gl::BindVertexArray(cube->GetVertexArray());
    DrawBVHRecursive(*cube, shader.uModel, sceneBVH, 0);
    gl::BindVertexArray();
}

void osc::DrawAABBs(nonstd::span<AABB const> aabbs, glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
{
    auto& shader = osc::App::shader<osc::SolidColorShader>();

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, projMtx);
    gl::Uniform(shader.uView, viewMtx);
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    auto cube = osc::App::meshes().getCubeWireMesh();
    gl::BindVertexArray(cube->GetVertexArray());

    for (AABB const& aabb : aabbs)
    {
        glm::vec3 halfWidths = Dimensions(aabb) / 2.0f;
        glm::vec3 center = Midpoint(aabb);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;

        gl::Uniform(shader.uModel, mmtx);
        cube->Draw();
    }

    gl::BindVertexArray();
}

void osc::DrawXZFloorLines(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
{
    auto& shader = osc::App::shader<osc::SolidColorShader>();

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, viewMtx);
    gl::Uniform(shader.uView, projMtx);

    auto yline = osc::App::meshes().getYLineMesh();
    gl::BindVertexArray(yline->GetVertexArray());

    // X
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, osc::fpi2, {0.0f, 0.0f, 1.0f}));
    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
    yline->Draw();

    // Z
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, osc::fpi2, {1.0f, 0.0f, 0.0f}));
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
    yline->Draw();

    gl::BindVertexArray();
}

void osc::DrawXZGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
{
    DrawGrid(glm::rotate(glm::mat4{1.0f}, osc::fpi2, {1.0f, 0.0f, 0.0f}), viewMtx, projMtx);
}

void osc::DrawXYGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
{
    DrawGrid(glm::mat4{1.0f}, viewMtx, projMtx);
}

void osc::DrawYZGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx)
{
    DrawGrid(glm::rotate(glm::mat4{1.0f}, osc::fpi2, {0.0f, 1.0f, 0.0f}), viewMtx, projMtx);
}