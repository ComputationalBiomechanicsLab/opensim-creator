#include "DAEWriter.hpp"

#include "osc_config.hpp"

#include "src/Graphics/BasicSceneElement.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Utils/Algorithms.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nonstd/span.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace std
{
    template<>
    class hash<glm::vec4> final {
    public:
        size_t operator()(glm::vec4 const& v) const
        {
            return osc::HashOf(v.x, v.y, v.z, v.w);
        }
    };
}


// internal stuff
namespace
{
    struct DAEGeometry final {
        std::string GeometryID;
        std::shared_ptr<osc::Mesh> Mesh;

        DAEGeometry(std::string geometryID_, std::shared_ptr<osc::Mesh> mesh_) :
            GeometryID{std::move(geometryID_)},
            Mesh{std::move(mesh_)}
        {
        }
    };

    struct DAEMaterial final {
        std::string MaterialID;
        glm::vec4 Color;

        explicit DAEMaterial(std::string materialID_, glm::vec4 const& color_) :
            MaterialID{std::move(materialID_)},
            Color{color_}
        {
        }
    };

    struct DAEInstance final {
        std::string InstanceID;
        std::string GeometryID;
        std::string MaterialID;
        osc::Transform Transform;

        DAEInstance(
            std::string instanceID_,
            std::string geometryID_,
            std::string materialID_,
            osc::Transform const& transform_) :

            InstanceID{std::move(instanceID_)},
            GeometryID{std::move(geometryID_)},
            MaterialID{std::move(materialID_)},
            Transform{transform_}
        {
        }
    };

    // internal representation of a datastructure that more closely resembles
    // how DAE files are structured
    struct DAESceneGraph final {
        std::vector<DAEGeometry> Geometries;
        std::vector<DAEMaterial> Materials;
        std::vector<DAEInstance> Instances;
    };

    DAESceneGraph ToDAESceneGraph(nonstd::span<osc::BasicSceneElement const> els)
    {
        DAESceneGraph rv;

        int latestMesh = 0;
        int latestMaterial = 0;
        std::unordered_map<std::shared_ptr<osc::Mesh>, std::string> mesh2id;
        std::unordered_map<glm::vec4, std::string> color2materialid;
        int latestInstance = 0;

        for (osc::BasicSceneElement const& el : els)
        {
            if (el.mesh->getTopography() != osc::MeshTopography::Triangles)
            {
                continue;  // unsupported
            }

            auto [meshIt, meshInserted] = mesh2id.try_emplace(el.mesh, "");
            if (meshInserted)
            {
                std::stringstream id;
                id << "mesh_" << latestMesh++;
                meshIt->second = std::move(id).str();

                rv.Geometries.emplace_back(meshIt->second, meshIt->first);
            }

            auto [materialIt, materialInserted] = color2materialid.try_emplace(el.color, "");
            if (materialInserted)
            {
                std::stringstream id;
                id << "material_" << latestMaterial++;
                materialIt->second = std::move(id).str();
                
                rv.Materials.emplace_back(materialIt->second, materialIt->first);
            }

            std::stringstream instanceID;
            instanceID << "instance_" << latestInstance++;
            rv.Instances.emplace_back(std::move(instanceID).str(), meshIt->second, materialIt->second, el.transform);
        }

        return rv;
    }

    nonstd::span<float const> ToFloatSpan(nonstd::span<glm::vec2 const> s)
    {
        return {glm::value_ptr(s[0]), 2 * s.size()};
    }

    nonstd::span<float const> ToFloatSpan(nonstd::span<glm::vec3 const> s)
    {
        return {glm::value_ptr(s[0]), 3 * s.size()};
    }

    nonstd::span<float const> ToFloatSpan(nonstd::span<glm::vec4 const> s)
    {
        return {glm::value_ptr(s[0]), 4 * s.size()};
    }

    nonstd::span<float const> ToFloatSpan(glm::vec2 const& v)
    {
        return {glm::value_ptr(v), 2};
    }

    nonstd::span<float const> ToFloatSpan(glm::vec3 const& v)
    {
        return {glm::value_ptr(v), 3};
    }

    nonstd::span<float const> ToFloatSpan(glm::vec4 const& v)
    {
        return {glm::value_ptr(v), 4};
    }

    template<typename T>
    std::string ToDaeList(nonstd::span<T const> vs)
    {
        std::stringstream ss;
        char const* delim = "";
        for (float v : vs)
        {
            ss << delim << v;
            delim = " ";
        }
        return std::move(ss).str();
    }

    void WriteXMLHeader(std::ostream& o)
    {
        o << R"(<?xml version="1.0" encoding="utf-8"?>)";
        o << '\n';
    }

    void WriteCOLLADARootNodeBEGIN(std::ostream& o)
    {
        o << R"(<COLLADA xmlns = "http://www.collada.org/2005/11/COLLADASchema" version = "1.4.1" xmlns:xsi = "http://www.w3.org/2001/XMLSchema-instance">)";
        o << '\n';
    }

    void WriteCOLLADARootNodeEND(std::ostream& o)
    {
        o << R"(</COLLADA>)";
        o << '\n';
    }

    void WriteTopLevelAssetBlock(std::ostream& o)
    {
        auto t = std::chrono::system_clock::now();

        o << fmt::format(
R"(  <asset>
    <contributor>
      <author>OpenSim Creator</author>
      <authoring_tool>OpenSim Creator v{} (build {})</authoring_tool>
    </contributor>
    <created>{}</created>
    <modified>{}</modified>
    <unit name="meter" meter="1"/>
    <up_axis>Y_UP</up_axis>
  </asset>)",
            OSC_VERSION_STRING,
            OSC_BUILD_ID,
            fmt::localtime(t),
            fmt::localtime(t));
        o << '\n';
    }

    void WriteEffect(std::ostream& o, DAEMaterial const& material)
    {
        o << fmt::format(R"(    <effect id="{}-effect">
      <profile_COMMON>
        <technique sid="common">
          <lambert>
            <emission>
              <color sid="emission">0 0 0 1</color>
            </emission>
            <diffuse>
              <color sid="diffuse">{}</color>
            </diffuse>
            <reflectivity>
              <float sid="specular">0.0</float>
            </reflectivity>
          </lambert>
        </technique>
      </profile_COMMON>
    </effect>)", material.MaterialID, ToDaeList(ToFloatSpan(material.Color)));
        o << '\n';
    }

    void WriteLibraryEffects(std::ostream& o, nonstd::span<DAEMaterial const> materials)
    {
        o << "  <library_effects>\n";
        for (DAEMaterial const& material : materials)
        {
            WriteEffect(o, material);
        }
        o << "  </library_effects>\n";
    }

    void WriteMaterial(std::ostream& o, DAEMaterial const& material)
    {
        o << fmt::format(R"(    <material id="{}-material" name="{}">
      <instance_effect url="#{}-effect"/>
    </material>)", material.MaterialID, material.MaterialID, material.MaterialID);
    }

    void WriteLibraryMaterials(std::ostream& o, nonstd::span<DAEMaterial const> materials)
    {
        o << "  <library_materials>\n";
        for (DAEMaterial const& material : materials)
        {
            WriteMaterial(o, material);
        }
        o << "  </library_materials>\n";
    }

    void WriteMeshPositionsSource(std::ostream& o, DAEGeometry const& geom)
    {
        nonstd::span<glm::vec3 const> vals = geom.Mesh->getVerts();
        int floatCount = 3*static_cast<int>(vals.size());
        int vertCount = static_cast<int>(vals.size());

        o << fmt::format(
R"(        <source id="{}-positions">
          <float_array id="{}-positions-array" count="{}">{}</float_array>
          <technique_common>
            <accessor source="#{}-positions-array" count="{}" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>)",
            geom.GeometryID,
            geom.GeometryID,
            floatCount,
            ToDaeList(ToFloatSpan(vals)),
            geom.GeometryID,
            vertCount);
        o << '\n';
    }

    void WriteMeshNormalsSource(std::ostream& o, DAEGeometry const& geom)
    {
        nonstd::span<glm::vec3 const> vals = geom.Mesh->getNormals();
        int floatCount = 3*static_cast<int>(vals.size());
        int normalCount = static_cast<int>(vals.size());

        o << fmt::format(
R"(        <source id="{}-normals">
          <float_array id="{}-normals-array" count="{}">{}</float_array>
          <technique_common>
            <accessor source="#{}-normals-array" count="{}" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>)",
            geom.GeometryID,
            geom.GeometryID,
            floatCount,
            ToDaeList(ToFloatSpan(vals)),
            geom.GeometryID,
            normalCount);
        o << '\n';
    }

    void WriteMeshTextureCoordsSource(std::ostream& o, DAEGeometry const& geom)
    {
        nonstd::span<glm::vec2 const> vals = geom.Mesh->getTexCoords();
        int floatCount = 2*static_cast<int>(vals.size());
        int coordCount = static_cast<int>(vals.size());

o << fmt::format(
R"(        <source id="{}-map-0">
          <float_array id="{}-map-0-array" count="{}">{}</float_array>
          <technique_common>
            <accessor source="#{}-map-0-array" count="{}" stride="2">
              <param name="S" type="float"/>
              <param name="T" type="float"/>
            </accessor>
          </technique_common>
        </source>)",
            geom.GeometryID,
            geom.GeometryID,
            floatCount,
            ToDaeList(ToFloatSpan(vals)),
            geom.GeometryID,
            coordCount);
        o << '\n';
    }

    void WriteMeshVertices(std::ostream& o, DAEGeometry const& geom)
    {
        o << fmt::format(
R"(        <vertices id="{}-vertices">
          <input semantic="POSITION" source="#{}-positions"/>
        </vertices>)",
            geom.GeometryID,
            geom.GeometryID);
        o << '\n';
    }

    void WriteMeshTriangles(std::ostream& o, DAEGeometry const& geom)
    {
        std::vector<uint32_t> indices = geom.Mesh->getIndices();
        int numTriangles = static_cast<int>(indices.size()) / 3;

        o << fmt::format(R"(        <triangles count="{}">)", numTriangles);
        o << '\n';
        o << fmt::format(R"(          <input semantic="VERTEX" source="#{}-vertices" offset="0" />)", geom.GeometryID);
        o << '\n';
        if (!geom.Mesh->getNormals().empty())
        {
            o << fmt::format(R"(          <input semantic="NORMAL" source="#{}-normals" offset="0" />)", geom.GeometryID);
            o << '\n';
        }
        if (!geom.Mesh->getTexCoords().empty())
        {
            o << fmt::format(R"(          <input semantic="TEXCOORD" source="#{}-map-0" offset="0" set="0"/>)", geom.GeometryID);
            o << '\n';
        }

        o << "          <p>";
        char const* delim = "";
        for (uint32_t v : indices)
        {
            o << delim << v;
            delim = " ";
        }
        o << "</p>\n";
        o << "        </triangles>\n";
    }

    void WriteMesh(std::ostream& o, DAEGeometry const& geom)
    {
        o << R"(      <mesh>)";
        o << '\n';

        WriteMeshPositionsSource(o, geom);
        if (!geom.Mesh->getNormals().empty())
        {
            WriteMeshNormalsSource(o, geom);
        }        
        if (!geom.Mesh->getTexCoords().empty())
        {
            WriteMeshTextureCoordsSource(o, geom);
        }        
        WriteMeshVertices(o, geom);
        WriteMeshTriangles(o, geom);

        o << R"(      </mesh>)";
        o << '\n';
    }

    void WriteGeometry(std::ostream& o, DAEGeometry const& geom)
    {
        o << fmt::format(R"(    <geometry id="{}" name="{}">)", geom.GeometryID, geom.GeometryID);
        o << '\n';
        WriteMesh(o, geom);
        o << R"(    </geometry>)";
        o << '\n';
    }

    void WriteLibraryGeometries(std::ostream& o, nonstd::span<DAEGeometry const> geoms)
    {
        o << "  <library_geometries>\n";
        for (DAEGeometry const& geom : geoms)
        {
            WriteGeometry(o, geom);
        }
        o << "  </library_geometries>\n";
    }

    void WriteTransformMatrix(std::ostream& o, osc::Transform const& t)
    {
        glm::mat4 m = osc::ToMat4(t);

        // row-major
        o << R"(        <matrix sid="transform">)";
        char const* delim = "";
        for (int row = 0; row < 4; ++row)
        {
            o << delim << m[0][row];
            delim = " ";
            o << delim << m[1][row];
            o << delim << m[2][row];
            o << delim << m[3][row];
        }
        o << R"(</matrix>)";
        o << '\n';
    }

    void WriteInstanceGeometryBindMaterial(std::ostream& o, DAEInstance const& instance)
    {
        o << fmt::format(R"(          <bind_material>
            <technique_common>
              <instance_material symbol="{}-material" target="#{}-material" />
            </technique_common>
          </bind_material>)", instance.MaterialID, instance.MaterialID);
    }

    void WriteNodeInstanceGeometry(std::ostream& o, DAEInstance const& instance)
    {
        o << fmt::format(R"(        <instance_geometry url="#{}" name="{}">)", instance.GeometryID, instance.GeometryID);
        o << '\n';
        WriteInstanceGeometryBindMaterial(o, instance);
        o << "        </instance_geometry>";
        o << '\n';
    }

    void WriteSceneNode(std::ostream& o, DAEInstance const& instance)
    {
        o << fmt::format(R"(      <node id="{}" name="{}" type="NODE">)", instance.InstanceID, instance.InstanceID);
        o << '\n';
        WriteTransformMatrix(o, instance.Transform);
        WriteNodeInstanceGeometry(o, instance);
        o << R"(      </node>)";
        o << '\n';
    }

    void WriteMainScene(std::ostream& o, DAESceneGraph const& graph)
    {
        o << R"(  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">)";
        o << '\n';

        for (DAEInstance const& ins : graph.Instances)
        {
            WriteSceneNode(o, ins);
        }

        o << R"(    </visual_scene>
  </library_visual_scenes>)";
        o << '\n';
    }

    void WriteSceneList(std::ostream& o)
    {
        o << R"(
  <scene>
    <instance_visual_scene url="#Scene"/>
  </scene>)";
        o << '\n';
    }
}

void osc::WriteDecorationsAsDAE(nonstd::span<BasicSceneElement const> els, std::ostream& o)
{
    DAESceneGraph graph = ToDAESceneGraph(els);

    WriteXMLHeader(o);
    WriteCOLLADARootNodeBEGIN(o);
    WriteTopLevelAssetBlock(o);
    WriteLibraryEffects(o, graph.Materials);
    WriteLibraryMaterials(o, graph.Materials);
    WriteLibraryGeometries(o, graph.Geometries);
    WriteMainScene(o, graph);
    WriteCOLLADARootNodeEND(o);
}