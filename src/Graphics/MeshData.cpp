#include "MeshData.hpp"

#include <cstddef>
#include <iostream>
#include <vector>

void osc::MeshData::clear()
{
    verts.clear();
    normals.clear();
    texcoords.clear();
    indices.clear();
}

void osc::MeshData::reserve(size_t n)
{
    verts.reserve(n);
    normals.reserve(n);
    texcoords.reserve(n);
    indices.reserve(n);
}

std::ostream& osc::operator<<(std::ostream& o, MeshData const& m)
{
    return o << "Mesh(nverts = " << m.verts.size() << ", nnormals = " << m.normals.size() << ", ntexcoords = " << m.texcoords.size() << ", nindices = " << m.indices.size() << ')';
}
