#include "OBJ.h"

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Platform/os.h>
#include <liboscar/Platform/Strings.h>

#include <cstddef>
#include <iomanip>
#include <ostream>
#include <string_view>

using namespace osc;

namespace
{
    void write_header(std::ostream& out, const OBJMetadata& metadata)
    {
        out << "# " << metadata.authoring_tool << '\n';
        out << "# created: " << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << '\n';
    }

    std::ostream& write_vector3(std::ostream& out, const Vector3& v)
    {
        return out << v.x << ' ' << v.y << ' ' << v.z;
    }

    void write_vertices(std::ostream& out, const Mesh& mesh)
    {
        for (const Vector3& vertex : mesh.vertices()) {
            out << "v ";
            write_vector3(out, vertex);
            out << '\n';
        }
    }

    void write_normals(std::ostream& out, const Mesh& mesh)
    {
        for (const Vector3& vertex : mesh.normals()) {
            out << "vn ";
            write_vector3(out, vertex);
            out << '\n';
        }
    }

    void write_faces(std::ostream& out, const Mesh& mesh, OBJWriterFlags flags)
    {
        if (mesh.topology() != MeshTopology::Triangles) {
            return;  // can only write `MeshTopology::Triangles`
        }

        const auto indices = mesh.indices();

        if (indices.size() < 3) {
            return;  // insufficient primitives
        }

        for (size_t i = 0; i < indices.size()-2; i += 3) {
            // vertex indices start at 1 in OBJ
            const uint32_t i0 = indices[i]+1;
            const uint32_t i1 = indices[i+1]+1;
            const uint32_t i2 = indices[i+2]+1;

            if (not (flags & OBJWriterFlag::NoWriteNormals)) {
                out << "f " << i0 << "//" << i0 << ' ' << i1  << "//" << i1 << ' ' << i2 << "//" << i2 << '\n';
            }
            else {
                // ignore the normals and only declare faces dependent on vertices
                out << "f " << i0 << ' ' << i1  << ' ' << i2 << '\n';
            }
        }
    }
}

osc::OBJMetadata::OBJMetadata() :
    OBJMetadata{strings::library_name()}
{}

osc::OBJMetadata::OBJMetadata(std::string_view authoring_tool_) :
    authoring_tool{authoring_tool_},
    creation_time{system_calendar_time()}
{}

void osc::OBJ::write(
    std::ostream& out,
    const Mesh& mesh,
    const OBJMetadata& metadata,
    OBJWriterFlags flags)
{
    write_header(out, metadata);
    write_vertices(out, mesh);
    if (not (flags & OBJWriterFlag::NoWriteNormals)) {
        write_normals(out, mesh);
    }
    write_faces(out, mesh, flags);
}
