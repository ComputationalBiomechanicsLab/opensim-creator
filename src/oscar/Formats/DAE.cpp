#include "DAE.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/os.h>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace osc;

// scene-to-graph conversion stuff
namespace
{
    struct DAEGeometry final {

        DAEGeometry(std::string geometry_id_, Mesh mesh_) :
            geometry_id{std::move(geometry_id_)},
            mesh{std::move(mesh_)}
        {}

        std::string geometry_id;
        Mesh mesh;
    };

    struct DAEMaterial final {

        DAEMaterial(
            std::string material_id_,
            const Color& color_) :

            material_id{std::move(material_id_)},
            color{color_}
        {}

        std::string material_id;
        Color color;
    };

    struct DAEInstance final {

        DAEInstance(
            std::string instance_id_,
            std::string geometry_id_,
            std::string material_id_,
            const Transform& transform_) :

            instance_id{std::move(instance_id_)},
            geometry_id{std::move(geometry_id_)},
            material_id{std::move(material_id_)},
            transform{transform_}
        {}

        std::string instance_id;
        std::string geometry_id;
        std::string material_id;
        Transform transform;
    };

    // internal representation of a datastructure that more closely resembles
    // how DAE files are structured
    struct DAESceneGraph final {
        std::vector<DAEGeometry> geometries;
        std::vector<DAEMaterial> materials;
        std::vector<DAEInstance> instances;
    };

    DAESceneGraph to_dae_scene_graph(std::span<const SceneDecoration> decorations)
    {
        DAESceneGraph rv;

        std::unordered_map<Mesh, std::string> mesh_to_id;
        std::unordered_map<Color, std::string> color_to_material_id;
        size_t latest_mesh = 0;
        size_t latest_material = 0;
        size_t latest_instance = 0;

        for (const SceneDecoration& dec : decorations) {
            if (dec.mesh.topology() != MeshTopology::Triangles) {
                continue;  // unsupported
            }

            auto [mesh_iter, mesh_inserted] = mesh_to_id.try_emplace(dec.mesh, std::string{});
            if (mesh_inserted) {
                std::stringstream id;
                id << "mesh_" << latest_mesh++;
                mesh_iter->second = std::move(id).str();

                rv.geometries.emplace_back(mesh_iter->second, mesh_iter->first);
            }

            auto [material_iter, material_inserted] = color_to_material_id.try_emplace(dec.color, std::string{});
            if (material_inserted) {
                std::stringstream id;
                id << "material_" << latest_material++;
                material_iter->second = std::move(id).str();

                rv.materials.emplace_back(material_iter->second, material_iter->first);
            }

            std::stringstream instance_id;
            instance_id << "instance_" << latest_instance++;
            rv.instances.emplace_back(std::move(instance_id).str(), mesh_iter->second, material_iter->second, dec.transform);
        }

        return rv;
    }
}

// graph-writing stuff
namespace
{
    std::span<const float> to_float_span(std::span<const Vec2> s)
    {
        return {value_ptr(s[0]), 2 * s.size()};
    }

    std::span<const float> to_float_span(std::span<const Vec3> s)
    {
        return {value_ptr(s[0]), 3 * s.size()};
    }

    std::span<const float> to_float_span(const Color& v)
    {
        return {value_ptr(v), 4};
    }

    std::ostream& operator<<(std::ostream& out, std::span<const float> vs)
    {
        std::string_view delim;
        for (float v : vs) {
            // note: to_string measures faster than directly streaming the value, because
            //       there's overhead associated with the stream that we don't care about
            //       for the DAE format (locale, etc.)
            out << delim << std::to_string(v);
            delim = " ";
        }
        return out;
    }

    void write_header(std::ostream& o)
    {
        o << R"(<?xml version="1.0" encoding="utf-8"?>)";
        o << '\n';
    }

    void write_collada_root_node_begin(std::ostream& o)
    {
        o << R"(<COLLADA xmlns = "http://www.collada.org/2005/11/COLLADASchema" version = "1.4.1" xmlns:xsi = "http://www.w3.org/2001/XMLSchema-instance">)";
        o << '\n';
    }

    void write_collada_root_node_end(std::ostream& o)
    {
        o << R"(</COLLADA>)";
        o << '\n';
    }

    void write_top_level_asset_node(std::ostream& o, const DAEMetadata& metadata)
    {
        o << "  <asset>\n";
        o << "    <contributor>\n";
        o << "      <author>" << metadata.author << "</author>\n";
        o << "      <authoring_tool>" << metadata.authoring_tool << "</authoring_tool>\n";
        o << "    </contributor>\n";
        o << "    <created>" << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << "</created>\n";
        o << "    <modified>" << std::put_time(&metadata.modification_time, "%Y-%m-%d %H:%M:%S") << "</modified>\n";
        o << "    <unit name=\"meter\" meter=\"1\" />\n";
        o << "    <up_axis>Y_UP</up_axis>\n";
        o << "  </asset>\n";
    }

    void write_effect_node(std::ostream& o, const DAEMaterial& material)
    {
        o << "    <effect id=\"" << material.material_id << "-effect\">\n";
        o << "      <profile_COMMON>\n";
        o << "        <technique sid=\"common\">\n";
        o << "          <lambert>\n";
        o << "            <emission>\n";
        o << "              <color sid=\"emission\">0 0 0 1</color>\n";
        o << "            </emission>\n";
        o << "            <diffuse>\n";
        o << "              <color sid=\"diffuse\">" << to_float_span(material.color) << "</color>\n";
        o << "            </diffuse>\n";
        o << "            <reflectivity>\n";
        o << "              <float sid=\"specular\">0.0</float>\n";
        o << "            </reflectivity>\n";
        o << "          </lambert>\n";
        o << "        </technique>\n";
        o << "      </profile_COMMON>\n";
        o << "    </effect>\n";
    }

    void write_library_effects_node(std::ostream& o, std::span<const DAEMaterial> materials)
    {
        o << "  <library_effects>\n";
        for (const DAEMaterial& material : materials) {
            write_effect_node(o, material);
        }
        o << "  </library_effects>\n";
    }

    void write_material_node(std::ostream& o, const DAEMaterial& material)
    {
        o << "    <material id=\"" << material.material_id << "-material\" name=\"" << material.material_id << "\">\n";
        o << "      <instance_effect url=\"#" << material.material_id << "-effect\"/>\n";
        o << "    </material>\n";
    }

    void write_library_materials_node(std::ostream& o, std::span<const DAEMaterial> materials)
    {
        o << "  <library_materials>\n";
        for (const DAEMaterial& material : materials) {
            write_material_node(o, material);
        }
        o << "  </library_materials>\n";
    }

    void write_mesh_positions_source_node(std::ostream& o, const DAEGeometry& geom)
    {
        const auto vals = geom.mesh.vertices();
        const size_t num_floats = 3 * vals.size();
        const size_t num_vertices = vals.size();

        o << "        <source id=\"" << geom.geometry_id << "-positions\">\n";
        o << "          <float_array id=\"" << geom.geometry_id << "-positions-array\" count=\"" << num_floats << "\">" << to_float_span(vals) << "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometry_id << "-positions-array\" count=\"" << num_vertices << "\" stride=\"3\">\n";
        o << "              <param name=\"X\" type=\"float\"/>\n";
        o << "              <param name=\"Y\" type=\"float\"/>\n";
        o << "              <param name=\"Z\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void write_mesh_normals_source_node(std::ostream& o, const DAEGeometry& geom)
    {
        const auto vals = geom.mesh.normals();
        const size_t num_floats = 3 * vals.size();
        const size_t num_normals = vals.size();

        o << "        <source id=\""  << geom.geometry_id << "-normals\">\n";
        o << "          <float_array id=\"" << geom.geometry_id << "-normals-array\" count=\"" << num_floats << "\">" << to_float_span(vals) << "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometry_id << "-normals-array\" count=\"" << num_normals << "\" stride=\"3\">\n";
        o << "              <param name=\"X\" type=\"float\"/>\n";
        o << "              <param name=\"Y\" type=\"float\"/>\n";
        o << "              <param name=\"Z\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void write_mesh_texture_coordinates_source_node(std::ostream& o, const DAEGeometry& geom)
    {
        const auto vals = geom.mesh.tex_coords();
        const size_t num_floats = 2 * vals.size();
        const size_t num_uvs = vals.size();

        o << "        <source id=\"" << geom.geometry_id << "-map-0\">\n";
        o << "          <float_array id=\"" << geom.geometry_id << "-map-0-array\" count=\"" << num_floats << "\">" << to_float_span(vals) <<  "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometry_id << "-map-0-array\" count=\"" << num_uvs << "\" stride=\"2\">\n";
        o << "              <param name=\"S\" type=\"float\"/>\n";
        o << "              <param name=\"T\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void write_mesh_vertices_node(std::ostream& o, const DAEGeometry& geom)
    {
        o << "        <vertices id=\"" << geom.geometry_id << "-vertices\">\n";
        o << R"(           <input semantic="POSITION" source="#)" << geom.geometry_id << "-positions\"/>\n";
        o << "        </vertices>\n";
    }

    void write_mesh_triangles_node(std::ostream& o, const DAEGeometry& geom)
    {
        const auto indices = geom.mesh.indices();
        const size_t num_triangles = indices.size() / 3;

        o << "        <triangles count=\"" << num_triangles << "\">\n";
        o << R"(            <input semantic="VERTEX" source="#)" << geom.geometry_id << "-vertices\" offset=\"0\" />\n";
        if (geom.mesh.has_normals()) {
            o << R"(            <input semantic="NORMAL" source="#)" << geom.geometry_id << "-normals\" offset=\"0\" />\n";
        }
        if (geom.mesh.has_tex_coords()) {
            o << R"(            <input semantic="TEXCOORD" source="#)" << geom.geometry_id << "-map-0\" offset=\"0\" set=\"0\"/>\n";
        }

        o << "          <p>";
        std::string_view delim;
        for (uint32_t v : indices) {
            o << delim << v;
            delim = " ";
        }
        o << "</p>\n";
        o << "        </triangles>\n";
    }

    void write_mesh_node(std::ostream& o, const DAEGeometry& geom)
    {
        o << R"(      <mesh>)";
        o << '\n';

        write_mesh_positions_source_node(o, geom);
        if (geom.mesh.has_normals()) {
            write_mesh_normals_source_node(o, geom);
        }
        if (geom.mesh.has_tex_coords()) {
            write_mesh_texture_coordinates_source_node(o, geom);
        }
        write_mesh_vertices_node(o, geom);
        write_mesh_triangles_node(o, geom);

        o << R"(      </mesh>)";
        o << '\n';
    }

    void write_geometry_node(std::ostream& o, const DAEGeometry& geom)
    {
        o << "    <geometry id=\"" << geom.geometry_id << "\" name=\"" << geom.geometry_id << "\">\n";
        write_mesh_node(o, geom);
        o << "    </geometry>\n";
    }

    void write_library_geometries_node(std::ostream& o, std::span<const DAEGeometry> geoms)
    {
        o << "  <library_geometries>\n";
        for (const DAEGeometry& geom : geoms) {
            write_geometry_node(o, geom);
        }
        o << "  </library_geometries>\n";
    }

    void write_matrix_node(std::ostream& o, const Transform& t)
    {
        const Mat4 m = mat4_cast(t);

        // row-major
        o << R"(        <matrix sid="transform">)";
        std::string_view delim;
        for (Mat4::size_type row = 0; row < 4; ++row) {
            o << delim << m[0][row];
            delim = " ";
            o << delim << m[1][row];
            o << delim << m[2][row];
            o << delim << m[3][row];
        }
        o << R"(</matrix>)";
        o << '\n';
    }

    void write_instance_bind_material_node(std::ostream& o, const DAEInstance& instance)
    {
        o << "          <bind_material>\n";
        o << "            <technique_common>\n";
        o << "              <instance_material symbol=\"" << instance.material_id << "-material\" target=\"#" << instance.material_id << "-material\" />\n";
        o << "            </technique_common>\n";
        o << "          </bind_material>\n";
    }

    void write_instance_geometry_node(std::ostream& o, const DAEInstance& instance)
    {
        o << "        <instance_geometry url=\"#" << instance.geometry_id << "\" name=\"" << instance.geometry_id << "\">\n";
        write_instance_bind_material_node(o, instance);
        o << "        </instance_geometry>\n";
    }

    void write_scene_node(std::ostream& o, const DAEInstance& instance)
    {
        o << "      <node id=\"" << instance.instance_id << "\" name=\"" << instance.instance_id << "\" type=\"NODE\">\n";
        write_matrix_node(o, instance.transform);
        write_instance_geometry_node(o, instance);
        o << "      </node>\n";
    }

    void write_library_visual_scenes_node(std::ostream& o, const DAESceneGraph& graph)
    {
        o << R"(  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">)";
        o << '\n';

        for (const DAEInstance& ins : graph.instances) {
            write_scene_node(o, ins);
        }

        o << R"(    </visual_scene>
  </library_visual_scenes>)";
        o << '\n';
    }
}


osc::DAEMetadata::DAEMetadata(
    std::string_view author_,
    std::string_view authoring_tool_) :

    author{author_},
    authoring_tool{authoring_tool_},
    creation_time{GetSystemCalendarTime()},
    modification_time{creation_time}
{}

void osc::write_as_dae(
    std::ostream& o,
    std::span<const SceneDecoration> els,
    const DAEMetadata& metadata)
{
    const DAESceneGraph graph = to_dae_scene_graph(els);

    write_header(o);
    write_collada_root_node_begin(o);
    write_top_level_asset_node(o, metadata);
    write_library_effects_node(o, graph.materials);
    write_library_materials_node(o, graph.materials);
    write_library_geometries_node(o, graph.geometries);
    write_library_visual_scenes_node(o, graph);
    write_collada_root_node_end(o);
}
