#include "SimTKDecorationGenerator.hpp"

#include <OpenSimCreator/Bindings/SimTKMeshLoader.hpp>
#include <OpenSimCreator/Bindings/SimTKHelpers.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Segment.hpp>
#include <oscar/Maths/Triangle.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Scene/SimpleSceneDecoration.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <simbody/internal/common.h>
#include <simbody/internal/MobilizedBody.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/State.h>
#include <SimTKcommon/internal/PolygonalMesh.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

using osc::Vec3;

// helper functions
namespace
{
    inline constexpr float c_LineThickness = 0.005f;
    inline constexpr float c_FrameAxisLengthRescale = 0.25f;
    inline constexpr float c_FrameAxisThickness = 0.0025f;

    // extracts scale factors from geometry
    Vec3 GetScaleFactors(SimTK::DecorativeGeometry const& geom)
    {
        SimTK::Vec3 sf = geom.getScaleFactors();

        for (int i = 0; i < 3; ++i)
        {
            sf[i] = sf[i] <= 0.0 ? 1.0 : sf[i];
        }

        return osc::ToVec3(sf);
    }

    // extracts RGBA color from geometry
    osc::Color GetColor(SimTK::DecorativeGeometry const& geom)
    {
        SimTK::Vec3 const& rgb = geom.getColor();

        auto ar = static_cast<float>(geom.getOpacity());
        ar = ar < 0.0f ? 1.0f : ar;

        return osc::Color{osc::ToVec4(rgb, ar)};
    }

    // creates a geometry-to-ground transform for the given geometry
    osc::Transform ToOscTransform(
        SimTK::SimbodyMatterSubsystem const& matter,
        SimTK::State const& state,
        SimTK::DecorativeGeometry const& g)
    {
        SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
        SimTK::Transform const& body2ground = mobod.getBodyTransform(state);
        SimTK::Transform const& decoration2body = g.getTransform();

        osc::Transform rv = osc::ToTransform(body2ground * decoration2body);
        rv.scale = GetScaleFactors(g);

        return rv;
    }

    size_t HashOf(SimTK::Vec3 const& v)
    {
        return osc::HashOf(v[0], v[1], v[2]);
    }

    size_t HashOf(SimTK::PolygonalMesh const& mesh)
    {
        size_t hash = 0;

        // combine vertex data into hash
        int const numVerts = mesh.getNumVertices();
        hash = osc::HashCombine(hash, osc::HashOf(numVerts));
        for (int vert = 0; vert < numVerts; ++vert)
        {
            hash = osc::HashCombine(hash, HashOf(mesh.getVertexPosition(vert)));
        }

        // combine face indices into mesh
        int const numFaces = mesh.getNumFaces();
        hash = osc::HashCombine(hash, osc::HashOf(numFaces));
        for (int face = 0; face < numFaces; ++face)
        {
            int const numVertsInFace = mesh.getNumVerticesForFace(face);
            for (int faceVert = 0; faceVert < numVertsInFace; ++faceVert)
            {
                hash = osc::HashCombine(hash, osc::HashOf(mesh.getFaceVertex(face, faceVert)));
            }
        }

        return hash;
    }

    // an implementation of SimTK::DecorativeGeometryImplementation that emits generic
    // triangle-mesh-based SystemDecorations that can be consumed by the rest of the UI
    class GeometryImpl final : public SimTK::DecorativeGeometryImplementation {
    public:
        GeometryImpl(
            osc::MeshCache& meshCache,
            SimTK::SimbodyMatterSubsystem const& matter,
            SimTK::State const& st,
            float fixupScaleFactor,
            std::function<void(osc::SimpleSceneDecoration&&)> const& out) :

            m_MeshCache{meshCache},
            m_Matter{matter},
            m_State{st},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Consumer{out}
        {
        }

    private:
        osc::Transform ToOscTransform(SimTK::DecorativeGeometry const& d) const
        {
            return ::ToOscTransform(m_Matter, m_State, d);
        }

        void implementPointGeometry(SimTK::DecorativePoint const&) final
        {
            [[maybe_unused]] static bool const s_ShownWarningOnce = []()
            {
                osc::log::warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementLineGeometry(SimTK::DecorativeLine const& d) final
        {
            osc::Transform const t = ToOscTransform(d);

            Vec3 const p1 = TransformPoint(t, osc::ToVec3(d.getPoint1()));
            Vec3 const p2 = TransformPoint(t, osc::ToVec3(d.getPoint2()));

            float const thickness = c_LineThickness * m_FixupScaleFactor;

            osc::Transform cylinderXform = osc::YToYCylinderToSegmentTransform({p1, p2}, thickness);
            cylinderXform.scale *= t.scale;

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getCylinderMesh(),
                cylinderXform,
                GetColor(d),
            });
        }

        void implementBrickGeometry(SimTK::DecorativeBrick const& d) final
        {
            osc::Transform t = ToOscTransform(d);
            t.scale *= osc::ToVec3(d.getHalfLengths());

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getBrickMesh(),
                t,
                GetColor(d),
            });
        }

        void implementCylinderGeometry(SimTK::DecorativeCylinder const& d) final
        {
            auto const radius = static_cast<float>(d.getRadius());

            osc::Transform t = ToOscTransform(d);
            t.scale.x *= radius;
            t.scale.y *= static_cast<float>(d.getHalfHeight());
            t.scale.z *= radius;

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getCylinderMesh(),
                t,
                GetColor(d),
            });
        }

        void implementCircleGeometry(SimTK::DecorativeCircle const& d) final
        {
            auto const radius = static_cast<float>(d.getRadius());

            osc::Transform t = ToOscTransform(d);
            t.scale.x *= radius;
            t.scale.y *= radius;

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getCircleMesh(),
                t,
                GetColor(d),
            });
        }

        void implementSphereGeometry(SimTK::DecorativeSphere const& d) final
        {
            osc::Transform t = ToOscTransform(d);
            t.scale *= m_FixupScaleFactor * static_cast<float>(d.getRadius());

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getSphereMesh(),
                t,
                GetColor(d),
            });
        }

        void implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& d) final
        {
            osc::Transform t = ToOscTransform(d);
            t.scale *= osc::ToVec3(d.getRadii());

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getSphereMesh(),
                t,
                GetColor(d),
            });
        }

        void implementFrameGeometry(SimTK::DecorativeFrame const& d) final
        {
            osc::Transform const t = ToOscTransform(d);

            // emit origin sphere
            {
                float const radius = 0.05f * c_FrameAxisLengthRescale * m_FixupScaleFactor;
                osc::Transform const sphereXform = t.withScale(radius);

                m_Consumer(osc::SimpleSceneDecoration
                {
                    m_MeshCache.getSphereMesh(),
                    sphereXform,
                    osc::Color::white(),
                });
            }

            // emit leg cylinders
            Vec3 const axisLengths = t.scale * static_cast<float>(d.getAxisLength());
            float const legLen = c_FrameAxisLengthRescale * m_FixupScaleFactor;
            float const legThickness = c_FrameAxisThickness * m_FixupScaleFactor;
            for (int axis = 0; axis < 3; ++axis)
            {
                Vec3 direction = {0.0f, 0.0f, 0.0f};
                direction[axis] = 1.0f;

                osc::Segment const line =
                {
                    t.position,
                    t.position + (legLen * axisLengths[axis] * TransformDirection(t, direction))
                };
                osc::Transform const legXform = YToYCylinderToSegmentTransform(line, legThickness);

                osc::Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[axis] = 1.0f;

                m_Consumer(osc::SimpleSceneDecoration
                {
                    m_MeshCache.getCylinderMesh(),
                    legXform,
                    color,
                });
            }
        }

        void implementTextGeometry(SimTK::DecorativeText const&) final
        {
            [[maybe_unused]] static bool const s_ShownWarningOnce = []()
            {
                osc::log::warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
                return true;
            }();
        }

        void implementMeshGeometry(SimTK::DecorativeMesh const& d) final
        {
            // the ID of an in-memory mesh is derived from the hash of its data
            //
            // (Simbody visualizer uses memory addresses, but this is invalid in
            //  OSC because there's a chance of memory re-use screwing with that
            //  caching mechanism)
            //
            // (and, yes, hash isn't equality, but it's closer than relying on memory
            //  addresses)
            std::string const id = std::to_string(HashOf(d.getMesh()));
            auto const meshLoaderFunc = [&d]() { return osc::ToOscMesh(d.getMesh()); };

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.get(id, meshLoaderFunc),
                ToOscTransform(d),
                GetColor(d),
            });
        }

        void implementMeshFileGeometry(SimTK::DecorativeMeshFile const& d) final
        {
            std::string const& path = d.getMeshFile();
            auto const meshLoader = [&path](){ return osc::LoadMeshViaSimTK(path); };

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.get(path, meshLoader),
                ToOscTransform(d),
                GetColor(d),
            });
        }

        void implementArrowGeometry(SimTK::DecorativeArrow const& d) final
        {
            osc::Transform const t = ToOscTransform(d);

            Vec3 const startBase = osc::ToVec3(d.getStartPoint());
            Vec3 const endBase = osc::ToVec3(d.getEndPoint());

            Vec3 const start = TransformPoint(t, startBase);
            Vec3 const end = TransformPoint(t, endBase);

            Vec3 const direction = osc::Normalize(end - start);

            Vec3 const neckStart = start;
            Vec3 const neckEnd = end - (static_cast<float>(d.getTipLength()) * direction);
            Vec3 const headStart = neckEnd;
            Vec3 const headEnd = end;

            float const neckThickness = m_FixupScaleFactor * static_cast<float>(d.getLineThickness());
            float const headThickness = 1.75f * neckThickness;

            osc::Color const color = GetColor(d);

            // emit neck cylinder
            osc::Transform const neckXform = osc::YToYCylinderToSegmentTransform({neckStart, neckEnd}, neckThickness);
            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getCylinderMesh(),
                neckXform,
                color,
            });

            // emit head cone
            osc::Transform const headXform = osc::YToYCylinderToSegmentTransform({headStart, headEnd}, headThickness);
            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getConeMesh(),
                headXform,
                color,
            });
        }

        void implementTorusGeometry(SimTK::DecorativeTorus const& d) final
        {
            auto const torusCenterToTubeCenterRadius = static_cast<float>(d.getTorusRadius());
            auto const tubeRadius = static_cast<float>(d.getTubeRadius());

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getTorusMesh(torusCenterToTubeCenterRadius, tubeRadius),
                ToOscTransform(d),
                GetColor(d),
            });
        }

        void implementConeGeometry(SimTK::DecorativeCone const& d) final
        {
            osc::Transform const t = ToOscTransform(d);

            Vec3 const posBase = osc::ToVec3(d.getOrigin());
            Vec3 const posDir = osc::ToVec3(d.getDirection());

            Vec3 const pos = TransformPoint(t, posBase);
            Vec3 const direction = TransformDirection(t, posDir);

            auto const radius = static_cast<float>(d.getBaseRadius());
            auto const height = static_cast<float>(d.getHeight());

            osc::Transform coneXform = osc::YToYCylinderToSegmentTransform({pos, pos + height*direction}, radius);
            coneXform.scale *= t.scale;

            m_Consumer(osc::SimpleSceneDecoration
            {
                m_MeshCache.getConeMesh(),
                coneXform,
                GetColor(d),
            });
        }

        osc::MeshCache& m_MeshCache;
        SimTK::SimbodyMatterSubsystem const& m_Matter;
        SimTK::State const& m_State;
        float m_FixupScaleFactor;
        std::function<void(osc::SimpleSceneDecoration&&)> const& m_Consumer;
    };
}

void osc::GenerateDecorations(
    MeshCache& meshCache,
    SimTK::SimbodyMatterSubsystem const& matter,
    SimTK::State const& state,
    SimTK::DecorativeGeometry const& geom,
    float fixupScaleFactor,
    std::function<void(SimpleSceneDecoration&&)> const& out)
{
    GeometryImpl impl{meshCache, matter, state, fixupScaleFactor, out};
    geom.implementGeometry(impl);
}
