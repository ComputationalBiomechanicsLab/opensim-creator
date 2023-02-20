#include "OBJ.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Utils/Cpp20Shims.hpp"

#include <cstddef>
#include <iostream>

namespace
{
    void WriteHeader(std::ostream& o)
    {
        o << "# OpenSim Creator\n";
        o << "# www.github.com/ComputationalBiomechanicsLab/opensim-creator\n";
    }

    std::ostream& WriteVec3(std::ostream& o, glm::vec3 const& v)
    {
        return o << v.x << ' ' << v.y << ' ' << v.z;
    }

    void WriteVertices(std::ostream& o, osc::Mesh const& mesh)
    {
        for (glm::vec3 const& v : mesh.getVerts())
        {
            o << "v ";
            WriteVec3(o, v);
            o << '\n';
        }
    }

    void WriteNormals(std::ostream& o, osc::Mesh const& mesh)
    {
        for (glm::vec3 const& v : mesh.getNormals())
        {
            o << "vn ";
            WriteVec3(o, v);
            o << '\n';
        }
    }

    void WriteFaces(std::ostream& o, osc::Mesh const& mesh, osc::ObjWriterFlags flags)
    {
        if (mesh.getTopology() != osc::MeshTopology::Triangles)
        {
            return;
        }

        auto view = mesh.getIndices();
        for (ptrdiff_t i = 0; i < ssize(view) - 2; i += 3)
        {
            // vertex indices start at 1 in OBJ
            uint32_t const i0 = view[i]+1;
            uint32_t const i1 = view[i+1]+1;
            uint32_t const i2 = view[i+2]+1;

            if (!(flags & osc::ObjWriterFlags_IgnoreNormals))
            {
                o << "f " << i0 << "//" << i0 << ' ' << i1  << "//" << i1 << ' ' << i2 << "//" << i2 << '\n';
            }
            else
            {
                // ignore the normals and only declare faces dependent on verts
                o << "f " << i0 << ' ' << i1  << ' ' << i2 << '\n';
            }
        }
    }
}

void osc::ObjWriter::write(Mesh const& mesh, ObjWriterFlags flags)
{
    WriteHeader(*m_OutputStream);
    WriteVertices(*m_OutputStream, mesh);
    if (!(flags & ObjWriterFlags_IgnoreNormals))
    {
        WriteNormals(*m_OutputStream, mesh);
    }
    WriteFaces(*m_OutputStream, mesh, flags);
}