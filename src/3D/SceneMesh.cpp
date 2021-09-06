#include "SceneMesh.hpp"

#include <atomic>

static std::atomic<int> g_LatestId = 0;

osc::SceneMesh::SceneMesh(Mesh const& m) :
    ID{++g_LatestId},
    mesh{std::move(m)},
    aabb{AABBFromVerts(m.verts.data(), m.verts.size())},
    boundingSphere{BoundingSphereFromVerts(m.verts.data(), m.verts.size())},
    triangleBVH{BVH_CreateFromTriangles(m.verts.data(), m.verts.size())} {

}

int osc::SceneMesh::getID() const noexcept {
    return ID;
}

osc::Mesh const& osc::SceneMesh::getMesh() const noexcept {
    return mesh;
}

std::vector<glm::vec3> const& osc::SceneMesh::getVerts() const noexcept {
    return mesh.verts;
}

std::vector<glm::vec3> const& osc::SceneMesh::getNormals() const noexcept {
    return mesh.normals;
}

std::vector<glm::vec2> const& osc::SceneMesh::getTexCoords() const noexcept {
    return mesh.texcoords;
}

std::vector<unsigned short> const& osc::SceneMesh::getIndices() const noexcept {
    return mesh.indices;
}

osc::AABB const& osc::SceneMesh::getAABB() const noexcept {
    return aabb;
}

osc::Sphere const& osc::SceneMesh::getBoundingSphere() const noexcept {
    return boundingSphere;
}

osc::BVH const& osc::SceneMesh::getBVH() const noexcept {
    return triangleBVH;
}
