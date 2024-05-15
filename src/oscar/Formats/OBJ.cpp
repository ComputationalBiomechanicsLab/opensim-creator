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
    void write_header(std::ostream& out, const ObjMetadata& metadata)
    {
        out << "# " << metadata.authoring_tool << '\n';
        out << "# created: " << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << '\n';
    }

    std::ostream& write_vec3(std::ostream& out, const Vec3& vec)
    {
        return out << vec.x << ' ' << vec.y << ' ' << vec.z;
    }

    void write_vertices(std::ostream& out, const Mesh& mesh)
    {
        for (const Vec3& vertex : mesh.vertices()) {
            out << "v ";
            write_vec3(out, vertex);
            out << '\n';
        }
    }

    void write_normals(std::ostream& out, const Mesh& mesh)
    {
        for (const Vec3& vertex : mesh.normals()) {
            out << "vn ";
            write_vec3(out, vertex);
            out << '\n';
        }
    }

    void write_faces(std::ostream& out, const Mesh& mesh, ObjWriterFlags flags)
    {
        if (mesh.topology() != MeshTopology::Triangles) {
            return;
        }

        const auto indices = mesh.indices();
        for (ptrdiff_t i = 0; i < std::ssize(indices)-2; i += 3) {
            // vertex indices start at 1 in OBJ
            const uint32_t i0 = indices[i]+1;
            const uint32_t i1 = indices[i+1]+1;
            const uint32_t i2 = indices[i+2]+1;

            if (not (flags & ObjWriterFlags::NoWriteNormals)) {
                out << "f " << i0 << "//" << i0 << ' ' << i1  << "//" << i1 << ' ' << i2 << "//" << i2 << '\n';
            }
            else {
                // ignore the normals and only declare faces dependent on verts
                out << "f " << i0 << ' ' << i1  << ' ' << i2 << '\n';
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
