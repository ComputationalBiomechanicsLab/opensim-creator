#include "OBJ.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/os.hpp>

#include <cstddef>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
    void WriteHeader(std::ostream& o, osc::ObjMetadata const& metadata)
    {
        o << "# " << metadata.authoringTool << '\n';
        o << "# created: " << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << '\n';
    }

    std::ostream& WriteVec3(std::ostream& o, osc::Vec3 const& v)
    {
        return o << v.x << ' ' << v.y << ' ' << v.z;
    }

    void WriteVertices(std::ostream& o, osc::Mesh const& mesh)
    {
        for (osc::Vec3 const& v : mesh.getVerts())
        {
            o << "v ";
            WriteVec3(o, v);
            o << '\n';
        }
    }

    void WriteNormals(std::ostream& o, osc::Mesh const& mesh)
    {
        for (osc::Vec3 const& v : mesh.getNormals())
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
        for (ptrdiff_t i = 0; i < std::ssize(view)-2; i += 3)
        {
            // vertex indices start at 1 in OBJ
            uint32_t const i0 = view[i]+1;
            uint32_t const i1 = view[i+1]+1;
            uint32_t const i2 = view[i+2]+1;

            if (!(flags & osc::ObjWriterFlags::NoWriteNormals))
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

// public API

osc::ObjMetadata::ObjMetadata(std::string_view authoringTool_) :
    authoringTool{authoringTool_},
    creationTime{GetSystemCalendarTime()}
{
}


void osc::WriteMeshAsObj(
    std::ostream& out,
    Mesh const& mesh,
    ObjMetadata const& metadata,
    ObjWriterFlags flags)
{
    WriteHeader(out, metadata);
    WriteVertices(out, mesh);
    if (!(flags & ObjWriterFlags::NoWriteNormals))
    {
        WriteNormals(out, mesh);
    }
    WriteFaces(out, mesh, flags);
}
