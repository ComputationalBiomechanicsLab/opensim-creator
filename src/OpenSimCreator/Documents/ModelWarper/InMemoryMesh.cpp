#include "InMemoryMesh.h"

#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>

#include <cstdint>
#include <span>

osc::InMemoryMesh::InMemoryMesh(std::span<Vec3 const> vertices, MeshIndicesView indices)
{
    AssignIndexedVerts(m_MeshData, vertices, indices);
}

void osc::InMemoryMesh::implementCreateDecorativeGeometry(SimTK::Array_<SimTK::DecorativeGeometry>& out) const
{
    out.push_back(SimTK::DecorativeMesh{m_MeshData});
}
