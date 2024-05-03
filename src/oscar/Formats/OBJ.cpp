#include "OBJ.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/os.h>

#include <cstddef>
#include <iomanip>
#include <ostream>
#include <string_view>

using namespace osc;

namespace
{
    void write_header(std::ostream& o, const ObjMetadata& metadata)
    {
        o << "# " << metadata.authoring_tool << '\n';
        o << "# created: " << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << '\n';
    }

    std::ostream& write_vec3(std::ostream& o, const Vec3& v)
    {
        return o << v.x << ' ' << v.y << ' ' << v.z;
    }

    void write_vertices(std::ostream& o, const Mesh& mesh)
    {
        for (const Vec3& v : mesh.vertices()) {
            o << "v ";
            write_vec3(o, v);
            o << '\n';
        }
    }

    void write_normals(std::ostream& o, const Mesh& mesh)
    {
        for (const Vec3& v : mesh.normals()) {
            o << "vn ";
            write_vec3(o, v);
            o << '\n';
        }
    }

    void write_faces(std::ostream& o, const Mesh& mesh, ObjWriterFlags flags)
    {
        if (mesh.topology() != MeshTopology::Triangles) {
            return;
        }

        const auto view = mesh.indices();
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


osc::ObjMetadata::ObjMetadata(std::string_view authoring_tool_) :
    authoring_tool{authoring_tool_},
    creation_time{system_calendar_time()}
{}

void osc::write_as_obj(
    std::ostream& out,
    const Mesh& mesh,
    const ObjMetadata& metadata,
    ObjWriterFlags flags)
{
    write_header(out, metadata);
    write_vertices(out, mesh);
    if (not (flags & ObjWriterFlags::NoWriteNormals)) {
        write_normals(out, mesh);
    }
    write_faces(out, mesh, flags);
}
