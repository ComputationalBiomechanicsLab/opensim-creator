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
    void writeHeader(std::ostream& o, const ObjMetadata& metadata)
    {
        o << "# " << metadata.authoringTool << '\n';
        o << "# created: " << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << '\n';
    }

    std::ostream& writeVec3(std::ostream& o, const Vec3& v)
    {
        return o << v.x << ' ' << v.y << ' ' << v.z;
    }

    void writeVertices(std::ostream& o, const Mesh& mesh)
    {
        for (const Vec3& v : mesh.getVerts()) {
            o << "v ";
            writeVec3(o, v);
            o << '\n';
        }
    }

    void writeNormals(std::ostream& o, const Mesh& mesh)
    {
        for (const Vec3& v : mesh.getNormals()) {
            o << "vn ";
            writeVec3(o, v);
            o << '\n';
        }
    }

    void writeFaces(std::ostream& o, const Mesh& mesh, ObjWriterFlags flags)
    {
        if (mesh.getTopology() != MeshTopology::Triangles) {
            return;
        }

        auto view = mesh.getIndices();
        for (ptrdiff_t i = 0; i < std::ssize(view)-2; i += 3) {
            // vertex indices start at 1 in OBJ
            const uint32_t i0 = view[i]+1;
            const uint32_t i1 = view[i+1]+1;
            const uint32_t i2 = view[i+2]+1;

            if (not (flags & ObjWriterFlags::NoWriteNormals)) {
                o << "f " << i0 << "//" << i0 << ' ' << i1  << "//" << i1 << ' ' << i2 << "//" << i2 << '\n';
            }
            else {
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
{}


void osc::writeMeshAsObj(
    std::ostream& out,
    const Mesh& mesh,
    const ObjMetadata& metadata,
    ObjWriterFlags flags)
{
    writeHeader(out, metadata);
    writeVertices(out, mesh);
    if (not (flags & ObjWriterFlags::NoWriteNormals)) {
        writeNormals(out, mesh);
    }
    writeFaces(out, mesh, flags);
}
