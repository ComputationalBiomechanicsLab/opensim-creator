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
#include <iostream>
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

        DAEGeometry(std::string geometryID_, Mesh mesh_) :
            geometryID{std::move(geometryID_)},
            mesh{std::move(mesh_)}
        {}

        std::string geometryID;
        Mesh mesh;
    };

    struct DAEMaterial final {

        DAEMaterial(
            std::string materialID_,
            const Color& color_) :

            materialID{std::move(materialID_)},
            color{color_}
        {}

        std::string materialID;
        Color color;
    };

    struct DAEInstance final {

        DAEInstance(
            std::string instanceID_,
            std::string geometryID_,
            std::string materialID_,
            const Transform& transform_) :

            instanceID{std::move(instanceID_)},
            geometryID{std::move(geometryID_)},
            materialID{std::move(materialID_)},
            transform{transform_}
        {}

        std::string instanceID;
        std::string geometryID;
        std::string materialID;
        Transform transform;
    };

    // internal representation of a datastructure that more closely resembles
    // how DAE files are structured
    struct DAESceneGraph final {
        std::vector<DAEGeometry> geometries;
        std::vector<DAEMaterial> materials;
        std::vector<DAEInstance> instances;
    };

    DAESceneGraph toDAESceneGraph(std::span<const SceneDecoration> els)
    {
        DAESceneGraph rv;

        size_t latestMesh = 0;
        size_t latestMaterial = 0;
        std::unordered_map<Mesh, std::string> mesh2id;
        std::unordered_map<Color, std::string> color2materialid;
        size_t latestInstance = 0;

        for (const SceneDecoration& el : els) {
            if (el.mesh.getTopology() != MeshTopology::Triangles) {
                continue;  // unsupported
            }

            auto [meshIt, meshInserted] = mesh2id.try_emplace(el.mesh, std::string{});
            if (meshInserted) {
                std::stringstream id;
                id << "mesh_" << latestMesh++;
                meshIt->second = std::move(id).str();

                rv.geometries.emplace_back(meshIt->second, meshIt->first);
            }

            auto [materialIt, materialInserted] = color2materialid.try_emplace(el.color, std::string{});
            if (materialInserted) {
                std::stringstream id;
                id << "material_" << latestMaterial++;
                materialIt->second = std::move(id).str();

                rv.materials.emplace_back(materialIt->second, materialIt->first);
            }

            std::stringstream instanceID;
            instanceID << "instance_" << latestInstance++;
            rv.instances.emplace_back(std::move(instanceID).str(), meshIt->second, materialIt->second, el.transform);
        }

        return rv;
    }
}

// graph-writing stuff
namespace
{
    std::span<const float> toFloatSpan(std::span<const Vec2> s)
    {
        return {value_ptr(s[0]), 2 * s.size()};
    }

    std::span<const float> toFloatSpan(std::span<const Vec3> s)
    {
        return {value_ptr(s[0]), 3 * s.size()};
    }

    std::span<const float> toFloatSpan(const Color& v)
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

    void writeXMLHeader(std::ostream& o)
    {
        o << R"(<?xml version="1.0" encoding="utf-8"?>)";
        o << '\n';
    }

    void writeCOLLADARootNodeBEGIN(std::ostream& o)
    {
        o << R"(<COLLADA xmlns = "http://www.collada.org/2005/11/COLLADASchema" version = "1.4.1" xmlns:xsi = "http://www.w3.org/2001/XMLSchema-instance">)";
        o << '\n';
    }

    void writeCOLLADARootNodeEND(std::ostream& o)
    {
        o << R"(</COLLADA>)";
        o << '\n';
    }

    void writeTopLevelAssetBlock(std::ostream& o, const DAEMetadata& metadata)
    {
        o << "  <asset>\n";
        o << "    <contributor>\n";
        o << "      <author>" << metadata.author << "</author>\n";
        o << "      <authoring_tool>" << metadata.authoringTool << "</authoring_tool>\n";
        o << "    </contributor>\n";
        o << "    <created>" << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << "</created>\n";
        o << "    <modified>" << std::put_time(&metadata.modificationTime, "%Y-%m-%d %H:%M:%S") << "</modified>\n";
        o << "    <unit name=\"meter\" meter=\"1\" />\n";
        o << "    <up_axis>Y_UP</up_axis>\n";
        o << "  </asset>\n";
    }

    void writeEffect(std::ostream& o, const DAEMaterial& material)
    {
        o << "    <effect id=\"" << material.materialID << "-effect\">\n";
        o << "      <profile_COMMON>\n";
        o << "        <technique sid=\"common\">\n";
        o << "          <lambert>\n";
        o << "            <emission>\n";
        o << "              <color sid=\"emission\">0 0 0 1</color>\n";
        o << "            </emission>\n";
        o << "            <diffuse>\n";
        o << "              <color sid=\"diffuse\">" << toFloatSpan(material.color) << "</color>\n";
        o << "            </diffuse>\n";
        o << "            <reflectivity>\n";
        o << "              <float sid=\"specular\">0.0</float>\n";
        o << "            </reflectivity>\n";
        o << "          </lambert>\n";
        o << "        </technique>\n";
        o << "      </profile_COMMON>\n";
        o << "    </effect>\n";
    }

    void writeLibraryEffects(std::ostream& o, std::span<const DAEMaterial> materials)
    {
        o << "  <library_effects>\n";
        for (const DAEMaterial& material : materials) {
            writeEffect(o, material);
        }
        o << "  </library_effects>\n";
    }

    void writeMaterial(std::ostream& o, const DAEMaterial& material)
    {
        o << "    <material id=\"" << material.materialID << "-material\" name=\"" << material.materialID << "\">\n";
        o << "      <instance_effect url=\"#" << material.materialID << "-effect\"/>\n";
        o << "    </material>\n";
    }

    void writeLibraryMaterials(std::ostream& o, std::span<const DAEMaterial> materials)
    {
        o << "  <library_materials>\n";
        for (const DAEMaterial& material : materials) {
            writeMaterial(o, material);
        }
        o << "  </library_materials>\n";
    }

    void writeMeshPositionsSource(std::ostream& o, const DAEGeometry& geom)
    {
        const auto vals = geom.mesh.getVerts();
        const size_t floatCount = 3 * vals.size();
        const size_t vertCount = vals.size();

        o << "        <source id=\"" << geom.geometryID << "-positions\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-positions-array\" count=\"" << floatCount << "\">" << toFloatSpan(vals) << "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometryID << "-positions-array\" count=\"" << vertCount << "\" stride=\"3\">\n";
        o << "              <param name=\"X\" type=\"float\"/>\n";
        o << "              <param name=\"Y\" type=\"float\"/>\n";
        o << "              <param name=\"Z\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void writeMeshNormalsSource(std::ostream& o, const DAEGeometry& geom)
    {
        const auto vals = geom.mesh.getNormals();
        const size_t floatCount = 3 * vals.size();
        const size_t normalCount = vals.size();

        o << "        <source id=\""  << geom.geometryID << "-normals\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-normals-array\" count=\"" << floatCount << "\">" << toFloatSpan(vals) << "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometryID << "-normals-array\" count=\"" << normalCount << "\" stride=\"3\">\n";
        o << "              <param name=\"X\" type=\"float\"/>\n";
        o << "              <param name=\"Y\" type=\"float\"/>\n";
        o << "              <param name=\"Z\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void writeMeshTextureCoordsSource(std::ostream& o, const DAEGeometry& geom)
    {
        const auto vals = geom.mesh.getTexCoords();
        const size_t floatCount = 2 * vals.size();
        const size_t coordCount = vals.size();

        o << "        <source id=\"" << geom.geometryID << "-map-0\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-map-0-array\" count=\"" << floatCount << "\">" << toFloatSpan(vals) <<  "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometryID << "-map-0-array\" count=\"" << coordCount << "\" stride=\"2\">\n";
        o << "              <param name=\"S\" type=\"float\"/>\n";
        o << "              <param name=\"T\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void writeMeshVertices(std::ostream& o, const DAEGeometry& geom)
    {
        o << "        <vertices id=\"" << geom.geometryID << "-vertices\">\n";
        o << R"(           <input semantic="POSITION" source="#)" << geom.geometryID << "-positions\"/>\n";
        o << "        </vertices>\n";
    }

    void writeMeshTriangles(std::ostream& o, const DAEGeometry& geom)
    {
        const auto indices = geom.mesh.getIndices();
        const size_t numTriangles = indices.size() / 3;

        o << "        <triangles count=\"" << numTriangles << "\">\n";
        o << R"(            <input semantic="VERTEX" source="#)" << geom.geometryID << "-vertices\" offset=\"0\" />\n";
        if (geom.mesh.hasNormals()) {
            o << R"(            <input semantic="NORMAL" source="#)" << geom.geometryID << "-normals\" offset=\"0\" />\n";
        }
        if (geom.mesh.hasTexCoords()) {
            o << R"(            <input semantic="TEXCOORD" source="#)" << geom.geometryID << "-map-0\" offset=\"0\" set=\"0\"/>\n";
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

    void writeMesh(std::ostream& o, const DAEGeometry& geom)
    {
        o << R"(      <mesh>)";
        o << '\n';

        writeMeshPositionsSource(o, geom);
        if (geom.mesh.hasNormals()) {
            writeMeshNormalsSource(o, geom);
        }
        if (geom.mesh.hasTexCoords()) {
            writeMeshTextureCoordsSource(o, geom);
        }
        writeMeshVertices(o, geom);
        writeMeshTriangles(o, geom);

        o << R"(      </mesh>)";
        o << '\n';
    }

    void writeGeometry(std::ostream& o, const DAEGeometry& geom)
    {
        o << "    <geometry id=\"" << geom.geometryID << "\" name=\"" << geom.geometryID << "\">\n";
        writeMesh(o, geom);
        o << "    </geometry>\n";
    }

    void writeLibraryGeometries(std::ostream& o, std::span<const DAEGeometry> geoms)
    {
        o << "  <library_geometries>\n";
        for (const DAEGeometry& geom : geoms) {
            writeGeometry(o, geom);
        }
        o << "  </library_geometries>\n";
    }

    void writeTransformMatrix(std::ostream& o, const Transform& t)
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

    void writeInstanceGeometryBindMaterial(std::ostream& o, const DAEInstance& instance)
    {
        o << "          <bind_material>\n";
        o << "            <technique_common>\n";
        o << "              <instance_material symbol=\"" << instance.materialID << "-material\" target=\"#" << instance.materialID << "-material\" />\n";
        o << "            </technique_common>\n";
        o << "          </bind_material>\n";
    }

    void writeNodeInstanceGeometry(std::ostream& o, const DAEInstance& instance)
    {
        o << "        <instance_geometry url=\"#" << instance.geometryID << "\" name=\"" << instance.geometryID << "\">\n";
        writeInstanceGeometryBindMaterial(o, instance);
        o << "        </instance_geometry>\n";
    }

    void writeSceneNode(std::ostream& o, const DAEInstance& instance)
    {
        o << "      <node id=\"" << instance.instanceID << "\" name=\"" << instance.instanceID << "\" type=\"NODE\">\n";
        writeTransformMatrix(o, instance.transform);
        writeNodeInstanceGeometry(o, instance);
        o << "      </node>\n";
    }

    void writeMainScene(std::ostream& o, const DAESceneGraph& graph)
    {
        o << R"(  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">)";
        o << '\n';

        for (const DAEInstance& ins : graph.instances) {
            writeSceneNode(o, ins);
        }

        o << R"(    </visual_scene>
  </library_visual_scenes>)";
        o << '\n';
    }
}


// public API

osc::DAEMetadata::DAEMetadata(
    std::string_view author_,
    std::string_view authoringTool_) :
    author{author_},
    authoringTool{authoringTool_},
    creationTime{GetSystemCalendarTime()},
    modificationTime{creationTime}
{}

void osc::writeDecorationsAsDAE(
    std::ostream& o,
    std::span<const SceneDecoration> els,
    const DAEMetadata& metadata)
{
    const DAESceneGraph graph = toDAESceneGraph(els);

    writeXMLHeader(o);
    writeCOLLADARootNodeBEGIN(o);
    writeTopLevelAssetBlock(o, metadata);
    writeLibraryEffects(o, graph.materials);
    writeLibraryMaterials(o, graph.materials);
    writeLibraryGeometries(o, graph.geometries);
    writeMainScene(o, graph);
    writeCOLLADARootNodeEND(o);
}
