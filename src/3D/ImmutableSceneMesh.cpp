#include "ImmutableSceneMesh.hpp"

#include <atomic>

static std::atomic<osc::ImmutableSceneMesh::IdType> g_LatestId = 0;

osc::ImmutableSceneMesh::ImmutableSceneMesh(CPUMesh const& m) :
    m_ID{++g_LatestId},
    m_Mesh{std::move(m)},
    m_AABB{AABBFromVerts(m.verts.data(), m.verts.size())},
    m_BoundingSphere{BoundingSphereFromVerts(m.verts.data(), m.verts.size())},
    m_TriangleBVH{BVH_CreateFromTriangles(m.verts.data(), m.verts.size())} {

}

osc::ImmutableSceneMesh::IdType osc::ImmutableSceneMesh::getID() const noexcept {
    return m_ID;
}

osc::CPUMesh const& osc::ImmutableSceneMesh::getMesh() const noexcept {
    return m_Mesh;
}

nonstd::span<glm::vec3 const> osc::ImmutableSceneMesh::getVerts() const noexcept {
    return m_Mesh.verts;
}

nonstd::span<glm::vec3 const> osc::ImmutableSceneMesh::getNormals() const noexcept {
    return m_Mesh.normals;
}

nonstd::span<glm::vec2 const> osc::ImmutableSceneMesh::getTexCoords() const noexcept {
    return m_Mesh.texcoords;
}

std::vector<uint32_t> const& osc::ImmutableSceneMesh::getIndices() const noexcept {
    return m_Mesh.indices;
}

osc::AABB const& osc::ImmutableSceneMesh::getAABB() const noexcept {
    return m_AABB;
}

osc::Sphere const& osc::ImmutableSceneMesh::getBoundingSphere() const noexcept {
    return m_BoundingSphere;
}

osc::BVH const& osc::ImmutableSceneMesh::getTriangleBVH() const noexcept {
    return m_TriangleBVH;
}
