#include "OpenSimDecorationGenerator.hpp"

#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Segment.hpp"
#include "src/Maths/Transform.hpp"
#include "src/OpenSimBindings/Rendering/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Rendering/SimTKDecorationGenerator.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/SimTKHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/Perf.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <SimTKcommon.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // helper: convert a physical frame's transform to ground into an osc::Transform
    osc::Transform TransformInGround(OpenSim::Frame const& frame, SimTK::State const& state)
    {
        return osc::ToTransform(frame.getTransformInGround(state));
    }

    // returns value between [0.0f, 1.0f]
    float GetMuscleColorFactor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        osc::MuscleColoringStyle s)
    {
        switch (s) {
        case osc::MuscleColoringStyle::Activation:
            return static_cast<float>(musc.getActivation(st));
        case osc::MuscleColoringStyle::Excitation:
            return static_cast<float>(musc.getExcitation(st));
        case osc::MuscleColoringStyle::Force:
            return static_cast<float>(musc.getActuation(st)) / static_cast<float>(musc.getMaxIsometricForce());
        case osc::MuscleColoringStyle::FiberLength:
        {
            float nfl = static_cast<float>(musc.getNormalizedFiberLength(st));  // 1.0f == ideal length
            float fl = nfl - 1.0f;
            fl = std::abs(fl);
            fl = std::min(fl, 1.0f);
            return fl;
        }
        default:
            return 1.0f;
        }
    }

    // helper: returns the color a muscle should have, based on a variety of options (style, user-defined stuff in OpenSim, etc.)
    //
    // this is just a rough estimation of how SCONE is coloring things
    glm::vec4 GetMuscleColor(
        OpenSim::Muscle const& musc,
        SimTK::State const& st,
        osc::MuscleColoringStyle s)
    {
        if (s == osc::MuscleColoringStyle::OpenSimAppearanceProperty)
        {
            SimTK::Vec3 c = musc.getGeometryPath().getDefaultColor();
            return glm::vec4{osc::ToVec3(c), 1.0f};
        }
        else if (s == osc::MuscleColoringStyle::OpenSim)
        {
            // use the same color that OpenSim emits (which is usually just activation-based, but might
            // change in the future)
            SimTK::Vec3 c = musc.getGeometryPath().getColor(st);
            return glm::vec4{osc::ToVec3(c), 1.0f};
        }
        else
        {
            // compute the color from the chosen color option
            glm::vec4 const zeroColor = {50.0f/255.0f, 50.0f/255.0f, 166.0f/255.0f, 1.0f};
            glm::vec4 const fullColor = {255.0f/255.0f, 25.0f/255.0f, 25.0f/255.0f, 1.0f};
            float const factor = GetMuscleColorFactor(musc, st, s);
            return zeroColor + factor * (fullColor - zeroColor);
        }
    }

    // helper: calculates the radius of a muscle based on isometric force
    //
    // similar to how SCONE does it, so that users can compare between the two apps
    float GetSconeStyleAutomaticMuscleRadiusCalc(OpenSim::Muscle const& m)
    {
        float f = static_cast<float>(m.getMaxIsometricForce());
        float specificTension = 0.25e6f;  // magic number?
        float pcsa = f / specificTension;
        float widthFactor = 0.25f;
        return widthFactor * std::sqrt(pcsa / osc::fpi);
    }

    // helper: returns the size (radius) of a muscle based on caller-provided sizing flags
    float GetMuscleSize(
        OpenSim::Muscle const& musc,
        float fixupScaleFactor,
        osc::MuscleSizingStyle s)
    {
        switch (s) {
        case osc::MuscleSizingStyle::PcsaDerived:
            return GetSconeStyleAutomaticMuscleRadiusCalc(musc) * fixupScaleFactor;
        case osc::MuscleSizingStyle::OpenSim:
        default:
            return 0.005f * fixupScaleFactor;
        }
    }
}

// geometry handlers
namespace
{
    // struct that's passed to each rendering function
    class RendererState final {
    public:
        RendererState(
            osc::MeshCache& meshCache,
            OpenSim::Model const& model,
            SimTK::State const& state,
            osc::CustomDecorationOptions const& opts,
            float fixupScaleFactor,
            std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& out) :

            m_MeshCache{meshCache},
            m_Model{model},
            m_State{state},
            m_Opts{opts},
            m_FixupScaleFactor{fixupScaleFactor},
            m_Out{out}
        {
        }

        osc::MeshCache& updMeshCache()
        {
            return m_MeshCache;
        }

        osc::Mesh const& getSphereMesh() const
        {
            return m_SphereMesh;
        }

        osc::Mesh const& getCylinderMesh() const
        {
            return m_CylinderMesh;
        }

        OpenSim::ModelDisplayHints const& getModelDisplayHints() const
        {
            return m_ModelDisplayHints;
        }

        bool getShowPathPoints() const
        {
            return m_ShowPathPoints;
        }

        SimTK::SimbodyMatterSubsystem const& getMatterSubsystem() const
        {
            return m_MatterSubsystem;
        }

        SimTK::State const& getState() const
        {
            return m_State;
        }

        osc::CustomDecorationOptions const& getOptions() const
        {
            return m_Opts;
        }

        float getFixupScaleFactor() const
        {
            return m_FixupScaleFactor;
        }

        void consume(OpenSim::Component const& component, osc::SceneDecoration&& dec)
        {
            m_Out(component, std::move(dec));
        }

        void emitGenericDecorations(
            OpenSim::Component const& componentToRender,
            OpenSim::Component const& componentToLinkTo)
        {
            std::function<void(osc::SimpleSceneDecoration&&)> callback = [this, &componentToLinkTo](osc::SimpleSceneDecoration&& dec)
            {
                consume(componentToLinkTo, osc::SceneDecoration{dec});
            };

            m_GeomList.clear();
            componentToRender.generateDecorations(
                true,
                getModelDisplayHints(),
                getState(),
                m_GeomList
            );
            for (SimTK::DecorativeGeometry const& geom : m_GeomList)
            {
                osc::GenerateDecorations(
                    updMeshCache(),
                    getMatterSubsystem(),
                    getState(),
                    geom,
                    getFixupScaleFactor(),
                    callback
                );
            }

            m_GeomList.clear();
            componentToRender.generateDecorations(
                false,
                getModelDisplayHints(),
                getState(),
                m_GeomList
            );
            for (SimTK::DecorativeGeometry const& geom : m_GeomList)
            {
                osc::GenerateDecorations(
                    updMeshCache(),
                    getMatterSubsystem(),
                    getState(),
                    geom,
                    getFixupScaleFactor(),
                    callback
                );
            }
        }

    private:
        osc::MeshCache& m_MeshCache;
        osc::Mesh m_SphereMesh = m_MeshCache.getSphereMesh();
        osc::Mesh m_CylinderMesh = m_MeshCache.getCylinderMesh();
        OpenSim::Model const& m_Model;
        OpenSim::ModelDisplayHints const& m_ModelDisplayHints = m_Model.getDisplayHints();
        bool m_ShowPathPoints = m_ModelDisplayHints.get_show_path_points();
        SimTK::SimbodyMatterSubsystem const& m_MatterSubsystem = m_Model.getSystem().getMatterSubsystem();
        SimTK::State const& m_State;
        osc::CustomDecorationOptions const& m_Opts;
        float m_FixupScaleFactor;
        std::function<void(OpenSim::Component const&, osc::SceneDecoration&&)> const& m_Out;
        SimTK::Array_<SimTK::DecorativeGeometry> m_GeomList;
    };

    // OSC-specific decoration handler for `OpenSim::PointToPointSpring`
    void HandlePointToPointSpring(
        RendererState& rs,
        OpenSim::PointToPointSpring const& p2p)
    {
        if (!rs.getOptions().getShouldShowPointToPointSprings())
        {
            return;
        }

        glm::vec3 const p1 = TransformInGround(p2p.getBody1(), rs.getState()) * osc::ToVec3(p2p.getPoint1());
        glm::vec3 const p2 = TransformInGround(p2p.getBody2(), rs.getState()) * osc::ToVec3(p2p.getPoint2());

        float const radius = 0.005f * rs.getFixupScaleFactor();
        osc::Transform const cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, radius);

        rs.consume(p2p, osc::SceneDecoration
        {
            rs.getCylinderMesh(),
            cylinderXform,
            glm::vec4{0.7f, 0.7f, 0.7f, 1.0f},
        });
    }

    // OSC-specific decoration handler for `OpenSim::Station`
    void HandleStation(
        RendererState& rs,
        OpenSim::Station const& s)
    {
        float radius = rs.getFixupScaleFactor() * 0.0045f;  // care: must be smaller than muscle caps (Tutorial 4)

        osc::Transform xform;
        xform.position = osc::ToVec3(s.getLocationInGround(rs.getState()));
        xform.scale = {radius, radius, radius};

        rs.consume(s, osc::SceneDecoration
        {
            rs.getSphereMesh(),
            xform,
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
        });
    }

    // OSC-specific decoration handler for `OpenSim::ScapulothoracicJoint`
    void HandleScapulothoracicJoint(
        RendererState& rs,
        OpenSim::ScapulothoracicJoint const& scapuloJoint)
    {
        osc::Transform t = TransformInGround(scapuloJoint.getParentFrame(), rs.getState());
        t.scale = osc::ToVec3(scapuloJoint.get_thoracic_ellipsoid_radii_x_y_z());

        rs.consume(scapuloJoint, osc::SceneDecoration
        {
            rs.getSphereMesh(),
            t,
            glm::vec4{1.0f, 1.0f, 0.0f, 0.2f},
        });
    }

    // OSC-specific decoration handler for `OpenSim::Body`
    void HandleBody(
        RendererState& rs,
        OpenSim::Body const& b)
    {
        // bodies are drawn normally but *also* draw a center-of-mass sphere if they are
        // currently hovered
        if (rs.getOptions().getShouldShowCentersOfMass() && b.getMassCenter() != SimTK::Vec3{0.0, 0.0, 0.0})
        {
            float radius = rs.getFixupScaleFactor() * 0.005f;
            osc::Transform t = TransformInGround(b, rs.getState());
            t.position = osc::TransformPoint(t, osc::ToVec3(b.getMassCenter()));
            t.scale = {radius, radius, radius};

            rs.consume(b, osc::SceneDecoration
            {
                rs.getSphereMesh(),
                t,
                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
            });
        }

        rs.emitGenericDecorations(b, b);
    }

    // OSC-specific decoration handler for `OpenSim::Muscle` ("SCONE"-style: i.e. tendons + muscle)
    void HandleMuscleFibersAndTendons(
        RendererState& rs,
        OpenSim::Muscle const& muscle)
    {
        float const fixupScaleFactor = rs.getFixupScaleFactor();
        std::vector<osc::GeometryPathPoint> const pps = osc::GetAllPathPoints(muscle.getGeometryPath(), rs.getState());

        if (pps.empty())
        {
            // edge-case: there are no points in the muscle path
            return;
        }

        float const fiberUiRadius = GetMuscleSize(
            muscle,
            fixupScaleFactor,
            rs.getOptions().getMuscleSizingStyle()
        );
        float const tendonUiRadius = 0.618f * fiberUiRadius;  // or fixupScaleFactor * 0.005f;

        glm::vec4 const fiberColor = GetMuscleColor(
            muscle,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );
        glm::vec4 const tendonColor = {204.0f/255.0f, 203.0f/255.0f, 200.0f/255.0f, 1.0f};

        osc::SceneDecoration fiberSpherePrototype =
        {
            rs.getSphereMesh(),
            osc::Transform{},
            fiberColor,
        };
        fiberSpherePrototype.transform.scale = {fiberUiRadius, fiberUiRadius, fiberUiRadius};

        osc::SceneDecoration tendonSpherePrototype{fiberSpherePrototype};
        tendonSpherePrototype.transform.scale = {tendonUiRadius, tendonUiRadius, tendonUiRadius};
        tendonSpherePrototype.color = tendonColor;

        auto emitTendonSphere = [&](glm::vec3 const& pos)
        {
            osc::SceneDecoration copy{tendonSpherePrototype};
            copy.transform.position = pos;
            rs.consume(muscle, std::move(copy));
        };
        auto emitTendonCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, tendonUiRadius);

            rs.consume(muscle, osc::SceneDecoration
            {
                rs.getCylinderMesh(),
                cylinderXform,
                tendonColor,
            });
        };
        auto emitFiberSphere = [&](glm::vec3 const& pos)
        {
            osc::SceneDecoration copy{fiberSpherePrototype};
            copy.transform.position = pos;
            rs.consume(muscle, std::move(copy));
        };
        auto emitFiberCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            rs.consume(muscle, osc::SceneDecoration
            {
                rs.getCylinderMesh(),
                cylinderXform,
                fiberColor,
            });
        };

        if (pps.size() == 1)
        {
            // edge-case: the muscle is a single point in space: just emit a sphere
            //
            // (this really should never happen, but you never know)
            emitFiberSphere(pps.front().locationInGround);
            return;
        }

        // else: the path is >= 2 points, so it's possible to measure a traversal
        //       length along it
        float tendonLen = static_cast<float>(muscle.getTendonLength(rs.getState()) * 0.5);
        tendonLen = std::clamp(tendonLen, 0.0f, tendonLen);
        float fiberLen = static_cast<float>(muscle.getFiberLength(rs.getState()));
        fiberLen = std::clamp(fiberLen, 0.0f, fiberLen);
        float const fiberEnd = tendonLen + fiberLen;

        size_t i = 1;
        osc::GeometryPathPoint prevPoint = pps.front();
        float prevTraversalPos = 0.0f;

        // draw first tendon
        if (prevTraversalPos < tendonLen)
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint.locationInGround);
        }
        while (i < pps.size() && prevTraversalPos < tendonLen)
        {
            // emit remaining tendon cylinder + spheres

            osc::GeometryPathPoint const& point = pps[i];
            glm::vec3 const prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - tendonLen;

            if (excess > 0.0f)
            {
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 tendonEnd = prevPoint.locationInGround + scaler * prevToPos;

                emitTendonCylinder(prevPoint.locationInGround, tendonEnd);
                emitTendonSphere(tendonEnd);

                prevPoint.locationInGround = tendonEnd;
                prevTraversalPos = tendonLen;
            }
            else
            {
                emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
                emitTendonSphere(point.locationInGround);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw fiber
        if (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit first fiber sphere
            emitFiberSphere(prevPoint.locationInGround);
        }
        while (i < pps.size() && prevTraversalPos < fiberEnd)
        {
            // emit remaining fiber cylinder + spheres

            osc::GeometryPathPoint const& point = pps[i];
            glm::vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;
            float excess = traversalPos - fiberEnd;

            if (excess > 0.0f)
            {
                // emit end point and then exit
                float scaler = (prevToPosLen - excess)/prevToPosLen;
                glm::vec3 fiberEndPos = prevPoint.locationInGround + scaler * prevToPos;

                emitFiberCylinder(prevPoint.locationInGround, fiberEndPos);
                emitFiberSphere(fiberEndPos);

                prevPoint.locationInGround = fiberEndPos;
                prevTraversalPos = fiberEnd;
            }
            else
            {
                emitFiberCylinder(prevPoint.locationInGround, point.locationInGround);
                emitFiberSphere(point.locationInGround);

                i++;
                prevPoint = point;
                prevTraversalPos = traversalPos;
            }
        }

        // draw second tendon
        if (i < pps.size())
        {
            // emit first tendon sphere
            emitTendonSphere(prevPoint.locationInGround);
        }
        while (i < pps.size())
        {
            // emit remaining fiber cylinder + spheres

            osc::GeometryPathPoint const& point = pps[i];
            glm::vec3 prevToPos = point.locationInGround - prevPoint.locationInGround;
            float prevToPosLen = glm::length(prevToPos);
            float traversalPos = prevTraversalPos + prevToPosLen;

            emitTendonCylinder(prevPoint.locationInGround, point.locationInGround);
            emitTendonSphere(point.locationInGround);

            i++;
            prevPoint = point;
            prevTraversalPos = traversalPos;
        }
    }

    // OSC-specific decoration handler for `OpenSim::Muscle`
    void HandleMuscleOpenSimStyle(
        RendererState& rs,
        OpenSim::Muscle const& musc)
    {
        std::vector<osc::GeometryPathPoint> const pps = osc::GetAllPathPoints(musc.getGeometryPath(), rs.getState());

        if (pps.empty())
        {
            return;
        }

        float const fiberUiRadius = GetMuscleSize(
            musc,
            rs.getFixupScaleFactor(),
            rs.getOptions().getMuscleSizingStyle()
        );
        glm::vec4 const fiberColor = GetMuscleColor(
            musc,
            rs.getState(),
            rs.getOptions().getMuscleColoringStyle()
        );

        auto emitSphere = [&](osc::GeometryPathPoint const& pp)
        {
            // ensure that user-defined path points are independently selectable (#425)
            //
            // TODO: SCONE-style etc. should also support this
            OpenSim::Component const& c = pp.maybeUnderlyingUserPathPoint ?
                *pp.maybeUnderlyingUserPathPoint :
                static_cast<OpenSim::Component const&>(musc);

            osc::Transform t;
            t.scale *= fiberUiRadius;
            t.position = pp.locationInGround;

            rs.consume(c, osc::SceneDecoration
            {
                rs.getSphereMesh(),
                t,
                fiberColor,
            });
        };

        auto emitCylinder = [&](glm::vec3 const& p1, glm::vec3 const& p2)
        {
            osc::Transform cylinderXform = osc::SimbodyCylinderToSegmentTransform({p1, p2}, fiberUiRadius);

            rs.consume(musc, osc::SceneDecoration
            {
                rs.getCylinderMesh(),
                cylinderXform,
                fiberColor,
            });
        };

        bool const showPathPoints = rs.getShowPathPoints();
        if (showPathPoints)
        {
            emitSphere(pps.front());
        }
        for (size_t i = 1; i < pps.size(); ++i)
        {
            emitCylinder(pps[i - 1].locationInGround, pps[i].locationInGround);

            if (showPathPoints)
            {
                emitSphere(pps[i]);
            }
        }
    }

    void DrawLineOfActionArrow(
        RendererState& rs,
        OpenSim::Muscle const& muscle,
        osc::PointDirection const& loaPointDirection,
        glm::vec4 const& color)
    {
        float const fixupScaleFactor = rs.getFixupScaleFactor();

        osc::ArrowProperties p;
        p.worldspaceStart = loaPointDirection.point;
        p.worldspaceEnd = loaPointDirection.point + (fixupScaleFactor*0.1f)*loaPointDirection.direction;
        p.tipLength = (fixupScaleFactor*0.015f);
        p.headThickness = (fixupScaleFactor*0.01f);
        p.neckThickness = (fixupScaleFactor*0.006f);
        p.color = color;

        osc::DrawArrow(rs.updMeshCache(), p, [&muscle, &rs](osc::SceneDecoration&& d)
        {
            rs.consume(muscle, std::move(d));
        });
    }

    void HandleLinesOfAction(
        RendererState& rs,
        OpenSim::Muscle const& musc)
    {
        glm::vec4 constexpr c_EffectiveLineOfActionColor = {0.0f, 1.0f, 0.0f, 1.0f};
        glm::vec4 constexpr c_AnatomicalLineOfActionColor = {1.0f, 0.0f, 0.0f, 1.0f};

        // if options request, render effective muscle lines of action
        if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForOrigin() ||
            rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForInsertion())
        {
            if (std::optional<osc::LinesOfAction> const loas = osc::GetEffectiveLinesOfActionInGround(musc, rs.getState()))
            {
                if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForOrigin())
                {
                    DrawLineOfActionArrow(rs, musc, loas->origin, c_EffectiveLineOfActionColor);
                }

                if (rs.getOptions().getShouldShowEffectiveMuscleLineOfActionForInsertion())
                {
                    DrawLineOfActionArrow(rs, musc, loas->insertion, c_EffectiveLineOfActionColor);
                }
            }
        }

        // if options request, render anatomical muscle lines of action
        if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForOrigin() ||
            rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForInsertion())
        {
            if (std::optional<osc::LinesOfAction> const loas = osc::GetAnatomicalLinesOfActionInGround(musc, rs.getState()))
            {
                if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForOrigin())
                {
                    DrawLineOfActionArrow(rs, musc, loas->origin, c_AnatomicalLineOfActionColor);
                }

                if (rs.getOptions().getShouldShowAnatomicalMuscleLineOfActionForInsertion())
                {
                    DrawLineOfActionArrow(rs, musc, loas->insertion, c_AnatomicalLineOfActionColor);
                }
            }
        }
    }

    // OSC-specific decoration handler for `OpenSim::GeometryPath`
    void HandleGeometryPath(
        RendererState& rs,
        OpenSim::GeometryPath const& gp)
    {
        if (!gp.get_Appearance().get_visible())
        {
            // even custom muscle decoration implementations *must* obey the visibility
            // flag on `GeometryPath` (#414)
            return;
        }

        if (!gp.hasOwner())
        {
            // it's a standalone path that's not part of a muscle
            rs.emitGenericDecorations(gp, gp);
            return;
        }

        // the `GeometryPath` has a owner, which might be a muscle or path actuator
        if (auto const* musc = dynamic_cast<OpenSim::Muscle const*>(&gp.getOwner()); musc)
        {
            // owner is a muscle, coerce selection "hit" to the muscle

            HandleLinesOfAction(rs, *musc);

            switch (rs.getOptions().getMuscleDecorationStyle())
            {
            case osc::MuscleDecorationStyle::FibersAndTendons:
                HandleMuscleFibersAndTendons(rs, *musc);
                return;
            case osc::MuscleDecorationStyle::Hidden:
                return;  // just don't generate them
            case osc::MuscleDecorationStyle::OpenSim:
            default:
                HandleMuscleOpenSimStyle(rs, *musc);
                return;
            }
        }
        else if (auto const* pa = dynamic_cast<OpenSim::PathActuator const*>(&gp.getOwner()); pa)
        {
            // owner is a path actuator, coerce selection "hit" to the path actuator (#519)
            rs.emitGenericDecorations(gp, *pa);
            return;
        }
        else
        {
            // it's a path in some non-muscular context
            rs.emitGenericDecorations(gp, gp);
            return;
        }
    }

    void HandleFrameGeometry(
        RendererState& rs,
        OpenSim::FrameGeometry const& frameGeometry)
    {
        // promote current component to the parent of the frame geometry, because
        // a user is probably more interested in the thing the frame geometry
        // represents (e.g. an offset frame) than the geometry itself (#506)
        OpenSim::Component const& componentToLinkTo = frameGeometry.hasOwner() ?
            frameGeometry.getOwner() :
            frameGeometry;

        rs.emitGenericDecorations(frameGeometry, componentToLinkTo);
    }
}

void osc::GenerateModelDecorations(
    MeshCache& meshCache,
    OpenSim::Model const& model,
    SimTK::State const& state,
    CustomDecorationOptions const& opts,
    float fixupScaleFactor,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out)
{
    OSC_PERF("OpenSimRenderer/GenerateModelDecorations");

    RendererState rendererState
    {
        meshCache,
        model,
        state,
        opts,
        fixupScaleFactor,
        out,
    };

    for (OpenSim::Component const& c : model.getComponentList())
    {
        // handle OSC-specific decoration specializations, or fallback to generic
        // component decoration handling
        if (!osc::ShouldShowInUI(c))
        {
            continue;
        }
        else if (typeid(c) == typeid(OpenSim::GeometryPath))
        {
            HandleGeometryPath(rendererState, static_cast<OpenSim::GeometryPath const&>(c));
        }
        else if (typeid(c) == typeid(OpenSim::Body))
        {
            HandleBody(rendererState, static_cast<OpenSim::Body const&>(c));
        }
        else if (typeid(c) == typeid(OpenSim::FrameGeometry))
        {
            HandleFrameGeometry(rendererState, static_cast<OpenSim::FrameGeometry const&>(c));
        }
        else if (opts.getShouldShowPointToPointSprings() && typeid(c) == typeid(OpenSim::PointToPointSpring))
        {
            HandlePointToPointSpring(rendererState, static_cast<OpenSim::PointToPointSpring const&>(c));
        }
        else if (typeid(c) == typeid(OpenSim::Station))
        {
            // CARE: it's a typeid comparison because OpenSim::Marker inherits from OpenSim::Station
            HandleStation(rendererState, static_cast<OpenSim::Station const&>(c));
        }
        else if (opts.getShouldShowScapulo() && typeid(c) == typeid(OpenSim::ScapulothoracicJoint))
        {
            HandleScapulothoracicJoint(rendererState, static_cast<OpenSim::ScapulothoracicJoint const&>(c));
        }
        else
        {
            rendererState.emitGenericDecorations(c, c);
        }
    }
}

float osc::GetRecommendedScaleFactor(
    MeshCache& meshCache,
    OpenSim::Model const& model,
    SimTK::State const& state,
    CustomDecorationOptions const& opts,
    float fixupScaleFactor)
{
    // generate decorations as if they were empty-sized and union their
    // AABBs to get an idea of what the "true" scale of the model probably
    // is (without the model containing oversized frames, etc.)
    std::vector<SceneDecoration> decs;
    GenerateModelDecorations(
        meshCache,
        model,
        state,
        opts,
        fixupScaleFactor,
        [&decs](OpenSim::Component const&, SceneDecoration&& dec)
        {
            decs.push_back(std::move(dec));
        }
    );

    if (decs.empty())
    {
        return 1.0f;
    }

    AABB aabb = GetWorldspaceAABB(decs[0]);
    for (SceneDecoration const& dec : decs)
    {
        aabb = Union(aabb, GetWorldspaceAABB(dec));
    }

    float longest = LongestDim(aabb);
    float rv = 1.0f;

    while (longest < 0.1)
    {
        longest *= 10.0f;
        rv /= 10.0f;
    }

    return rv;
}