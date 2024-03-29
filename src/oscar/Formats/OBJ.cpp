#include "OBJ.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/os.h>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string_view>

using namespace osc;

namespace
{
    void WriteHeader(std::ostream& o, ObjMetadata const& metadata)
    {
        o << "# " << metadata.authoringTool << '\n';
        o << "# created: " << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << '\n';
    }

    std::ostream& WriteVec3(std::ostream& o, Vec3 const& v)
    {
        return o << v.x << ' ' << v.y << ' ' << v.z;
    }

    void WriteVertices(std::ostream& o, Mesh const& mesh)
    {
        for (Vec3 const& v : mesh.getVerts())
        {
            o << "v ";
            WriteVec3(o, v);
            o << '\n';
        }
    }

    void WriteNormals(std::ostream& o, Mesh const& mesh)
    {
        for (Vec3 const& v : mesh.getNormals())
        {
            o << "vn ";
            WriteVec3(o, v);
            o << '\n';
        }
    }

    void WriteFaces(std::ostream& o, Mesh const& mesh, ObjWriterFlags flags)
    {
        if (mesh.getTopology() != MeshTopology::Triangles)
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

            if (!(flags & ObjWriterFlags::NoWriteNormals))
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
