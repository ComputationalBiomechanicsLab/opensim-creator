#include "FrameDefinitionTab.hpp"

#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"
#include "OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp"
#include "OpenSimCreator/Graphics/OpenSimGraphicsHelpers.hpp"
#include "OpenSimCreator/Graphics/SimTKMeshLoader.hpp"
#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanel.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelLayer.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelParameters.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelRightClickEvent.hpp"
#include "OpenSimCreator/Panels/ModelEditorViewerPanelState.hpp"
#include "OpenSimCreator/Panels/NavigatorPanel.hpp"
#include "OpenSimCreator/Panels/PropertiesPanel.hpp"
#include "OpenSimCreator/Widgets/BasicWidgets.hpp"
#include "OpenSimCreator/Widgets/MainMenu.hpp"
#include "OpenSimCreator/ActionFunctions.hpp"
#include "OpenSimCreator/OpenSimHelpers.hpp"
#include "OpenSimCreator/SimTKHelpers.hpp"
#include "OpenSimCreator/UndoableModelStatePair.hpp"
#include "OpenSimCreator/VirtualConstModelStatePair.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Graphics/SceneRenderer.hpp>
#include <oscar/Graphics/SceneRendererParams.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Panels/LogViewerPanel.hpp>
#include <oscar/Panels/Panel.hpp>
#include <oscar/Panels/PanelManager.hpp>
#include <oscar/Panels/StandardPanel.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Algorithms.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/FilesystemHelpers.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Widgets/Popup.hpp>
#include <oscar/Widgets/PopupManager.hpp>
#include <oscar/Widgets/StandardPopup.hpp>
#include <oscar/Widgets/WindowMenu.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/ModelComponent.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <SDL_events.h>
#include <SimTKcommon/internal/DecorativeGeometry.h>

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

// top-level constants
namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/Experimental/FrameDefinition";
    constexpr double c_SphereDefaultRadius = 0.01;
    constexpr osc::Color c_SphereDefaultColor = {1.0f, 1.0f, 0.75f};
    constexpr osc::Color c_MidpointDefaultColor = {0.75f, 1.0f, 1.0f};
    constexpr osc::Color c_PointToPointEdgeDefaultColor = {0.75f, 1.0f, 1.0f};
    constexpr osc::Color c_CrossProductEdgeDefaultColor = {0.75f, 1.0f, 1.0f};
}

// custom OpenSim components for this screen
namespace OpenSim
{
    // HACK: OpenSim namespace is REQUIRED
    //
    // because OpenSim's property macros etc. assume so much, see:
    //
    //  - https://github.com/opensim-org/opensim-core/pull/3469

    // returns the RGB components of `color`
    SimTK::Vec3 ToRGBVec3(osc::Color const& color)
    {
        return {color.r, color.g, color.b};
    }

    // sets the appearance of `geometry` (SimTK) from `appearance` (OpenSim)
    void SetGeomAppearance(
        SimTK::DecorativeGeometry& geometry,
        OpenSim::Appearance const& appearance)
    {
        geometry.setColor(appearance.get_color());
        geometry.setOpacity(appearance.get_opacity());
        if (appearance.get_visible())
        {
            geometry.setRepresentation(appearance.get_representation());
        }
        else
        {
            geometry.setRepresentation(SimTK::DecorativeGeometry::Hide);
        }
    }

    // sets the color and opacity of `appearance` from `color`
    void SetColorAndOpacity(
        OpenSim::Appearance& appearance,
        osc::Color const& color)
    {
        appearance.set_color(ToRGBVec3(color));
        appearance.set_opacity(color.a);
    }

    // returns a decorative sphere with `radius`, `position`, and `appearance`
    SimTK::DecorativeSphere CreateDecorativeSphere(
        double radius,
        SimTK::Vec3 position,
        OpenSim::Appearance const& appearance)
    {
        SimTK::DecorativeSphere sphere{radius};
        sphere.setTransform(SimTK::Transform{position});
        SetGeomAppearance(sphere, appearance);
        return sphere;
    }

    // returns a decorative line between `startPosition` and `endPosition` with `appearance`
    SimTK::DecorativeLine CreateDecorativeLine(
        SimTK::Vec3 const& startPosition,
        SimTK::Vec3 const& endPosition,
        OpenSim::Appearance const& appearance)
    {
        SimTK::DecorativeLine line{startPosition, endPosition};
        SetGeomAppearance(line, appearance);
        return line;
    }

    // returns a decorative frame based on the provided transform
    SimTK::DecorativeFrame CreateDecorativeFrame(
        SimTK::Transform const& transformInGround)
    {
        // adapted from OpenSim::FrameGeometry (Geometry.cpp)
        SimTK::DecorativeFrame frame(1.0);
        frame.setTransform(transformInGround);
        frame.setScale(0.2);
        frame.setLineThickness(0.004);
        return frame;
    }

    // returns a SimTK::DecorativeMesh reperesentation of the parallelogram formed between
    // two (potentially disconnected) edges, starting at `origin`
    SimTK::DecorativeMesh CreateParallelogramMesh(
        SimTK::Vec3 const& origin,
        SimTK::Vec3 const& firstEdge,
        SimTK::Vec3 const& secondEdge,
        OpenSim::Appearance const& appearance)
    {
        SimTK::PolygonalMesh polygonalMesh;
        {
            std::array<SimTK::Vec3, 4> const verts
            {
                origin,
                origin + firstEdge,
                origin + firstEdge + secondEdge,
                origin + secondEdge,
            };

            SimTK::Array_<int> face;
            for (SimTK::Vec3 const& vert : verts)
            {
                face.push_back(polygonalMesh.addVertex(vert));
            }
            polygonalMesh.addFace(face);
        }

        SimTK::DecorativeMesh rv{polygonalMesh};
        SetGeomAppearance(rv, appearance);
        return rv;
    }

    // a sphere landmark, where the center of the sphere is the point of interest
    class SphereLandmark final : public OpenSim::Station {
        OpenSim_DECLARE_CONCRETE_OBJECT(SphereLandmark, OpenSim::Station)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the sphere (decorative)");
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the sphere (decorative)");

        SphereLandmark()
        {
            constructProperty_radius(c_SphereDefaultRadius);
            constructProperty_Appearance(Appearance{});
            SetColorAndOpacity(upd_Appearance(), c_SphereDefaultColor);
        }

        void generateDecorations(
            bool,
            const ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const final
        {
            appendOut.push_back(CreateDecorativeSphere(
                get_radius(),
                getLocationInGround(state),
                get_Appearance()
            ));
        }
    };

    // a landmark defined as a point between two other points
    class MidpointLandmark final : public OpenSim::Point {
        OpenSim_DECLARE_CONCRETE_OBJECT(MidpointLandmark, OpenSim::Point)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the midpoint (decorative)");
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the midpoint (decorative)");
        OpenSim_DECLARE_SOCKET(pointA, OpenSim::Point, "The first point that the midpoint is between");
        OpenSim_DECLARE_SOCKET(pointB, OpenSim::Point, "The second point that the midpoint is between");

        MidpointLandmark()
        {
            constructProperty_radius(c_SphereDefaultRadius);
            constructProperty_Appearance(Appearance{});
            SetColorAndOpacity(upd_Appearance(), c_MidpointDefaultColor);
        }

        void generateDecorations(
            bool,
            const ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const final
        {
            appendOut.push_back(CreateDecorativeSphere(
                get_radius(),
                getLocationInGround(state),
                get_Appearance()
            ));
        }

    private:
        SimTK::Vec3 calcLocationInGround(const SimTK::State& state) const final
        {
            SimTK::Vec3 const pointALocation = getConnectee<OpenSim::Point>("pointA").getLocationInGround(state);
            SimTK::Vec3 const pointBLocation = getConnectee<OpenSim::Point>("pointB").getLocationInGround(state);
            return 0.5*(pointALocation + pointBLocation);
        }

        SimTK::Vec3 calcVelocityInGround(const SimTK::State& state) const final
        {
            SimTK::Vec3 const pointAVelocity = getConnectee<OpenSim::Point>("pointA").getVelocityInGround(state);
            SimTK::Vec3 const pointBVelocity = getConnectee<OpenSim::Point>("pointB").getVelocityInGround(state);
            return 0.5*(pointAVelocity + pointBVelocity);
        }

        SimTK::Vec3 calcAccelerationInGround(const SimTK::State& state) const final
        {
            SimTK::Vec3 const pointAAcceleration = getConnectee<OpenSim::Point>("pointA").getAccelerationInGround(state);
            SimTK::Vec3 const pointBAcceleration = getConnectee<OpenSim::Point>("pointB").getAccelerationInGround(state);
            return 0.5*(pointAAcceleration + pointBAcceleration);
        }
    };

    // the start and end locations of an edge in 3D space
    struct EdgePoints final {
        SimTK::Vec3 start;
        SimTK::Vec3 end;
    };

    // returns the direction vector between the `start` and `end` points
    SimTK::UnitVec3 CalcDirection(EdgePoints const& a)
    {
        return SimTK::UnitVec3{a.end - a.start};
    }

    // returns points for an edge that:
    //
    // - originates at `a.start`
    // - points in the direction of `a x b`
    // - has a magnitude of min(|a|, |b|) - handy for rendering
    EdgePoints CrossProduct(EdgePoints const& a, EdgePoints const& b)
    {
        // TODO: if cross product isn't possible (e.g. angle between vectors is zero)
        // then this needs to fail or fallback
        SimTK::Vec3 const firstEdge = a.end - a.start;
        SimTK::Vec3 const secondEdge = b.end - b.start;
        SimTK::Vec3 const resultEdge = SimTK::cross(firstEdge, secondEdge).normalize();
        double const resultEdgeLength = std::min(firstEdge.norm(), secondEdge.norm());

        return {a.start, a.start + (resultEdgeLength*resultEdge)};
    }

    // virtual base class for an edge that starts at one location in ground and ends at
    // some other location in ground
    class FDVirtualEdge : public ModelComponent {
        OpenSim_DECLARE_ABSTRACT_OBJECT(FDVirtualEdge, ModelComponent)
    public:
        EdgePoints getEdgePointsInGround(SimTK::State const& state) const
        {
            return implGetEdgePointsInGround(state);
        }
    private:
        virtual EdgePoints implGetEdgePointsInGround(SimTK::State const&) const = 0;
    };

    // an edge that starts at virtual `pointA` and ends at virtual `pointB`
    class FDPointToPointEdge final : public FDVirtualEdge {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDPointToPointEdge, FDVirtualEdge)
    public:
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the edge (decorative)");
        OpenSim_DECLARE_SOCKET(pointA, Point, "The first point that the edge is connected to");
        OpenSim_DECLARE_SOCKET(pointB, Point, "The second point that the edge is connected to");

        FDPointToPointEdge()
        {
            constructProperty_Appearance(Appearance{});
            SetColorAndOpacity(upd_Appearance(), c_PointToPointEdgeDefaultColor);
        }

        void generateDecorations(
            bool,
            const ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const final
        {
            EdgePoints const coords = getEdgePointsInGround(state);

            appendOut.push_back(CreateDecorativeLine(
                coords.start,
                coords.end,
                get_Appearance()
            ));
        }

    private:
        EdgePoints implGetEdgePointsInGround(SimTK::State const& state) const final
        {
            OpenSim::Point const& pointA = getConnectee<OpenSim::Point>("pointA");
            SimTK::Vec3 const pointAGroundLoc = pointA.getLocationInGround(state);

            OpenSim::Point const& pointB = getConnectee<OpenSim::Point>("pointB");
            SimTK::Vec3 const pointBGroundLoc = pointB.getLocationInGround(state);

            return {pointAGroundLoc, pointBGroundLoc};
        }
    };

    // an edge that is computed from `edgeA x edgeB`
    //
    // - originates at `a.start`
    // - points in the direction of `a x b`
    // - has a magnitude of min(|a|, |b|) - handy for rendering
    class FDCrossProductEdge final : public FDVirtualEdge {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDCrossProductEdge, FDVirtualEdge)
    public:
        OpenSim_DECLARE_PROPERTY(showPlane, bool, "Whether to show the plane of the two edges the cross product was created from (decorative)");
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the edge (decorative)");
        OpenSim_DECLARE_SOCKET(edgeA, FDVirtualEdge, "The first edge parameter to the cross product calculation");
        OpenSim_DECLARE_SOCKET(edgeB, FDVirtualEdge, "The second edge parameter to the cross product calculation");

        FDCrossProductEdge()
        {
            constructProperty_showPlane(false);
            constructProperty_Appearance(Appearance{});
            SetColorAndOpacity(upd_Appearance(), c_CrossProductEdgeDefaultColor);
        }

        void generateDecorations(
            bool,
            const ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const final
        {
            EdgePoints const coords = getEdgePointsInGround(state);

            // draw edge
            appendOut.push_back(CreateDecorativeLine(
                coords.start,
                coords.end,
                get_Appearance()
            ));

            // if requested, draw a parallelogram from the two edges
            if (get_showPlane())
            {
                auto const [aPoints, bPoints] = getBothEdgePoints(state);
                appendOut.push_back(CreateParallelogramMesh(
                    coords.start,
                    aPoints.end - aPoints.start,
                    bPoints.end - bPoints.start,
                    get_Appearance()
                ));
            }
        }

    private:
        std::pair<EdgePoints, EdgePoints> getBothEdgePoints(SimTK::State const& state) const
        {
            return
            {
                getConnectee<FDVirtualEdge>("edgeA").getEdgePointsInGround(state),
                getConnectee<FDVirtualEdge>("edgeB").getEdgePointsInGround(state),
            };
        }

        EdgePoints implGetEdgePointsInGround(SimTK::State const& state) const final
        {
            std::pair<EdgePoints, EdgePoints> const edgePoints = getBothEdgePoints(state);
            return  CrossProduct(edgePoints.first, edgePoints.second);
        }
    };

    // enumeration of the possible axes a user may define
    enum class AxisIndex : int32_t {
        X = 0,
        Y,
        Z,
        TOTAL
    };

    // returns the next `AxisIndex` in the circular sequence X -> Y -> Z
    constexpr AxisIndex Next(AxisIndex axis)
    {
        return static_cast<AxisIndex>((static_cast<int32_t>(axis) + 1) % static_cast<int32_t>(AxisIndex::TOTAL));
    }
    static_assert(Next(AxisIndex::X) == AxisIndex::Y);
    static_assert(Next(AxisIndex::Y) == AxisIndex::Z);
    static_assert(Next(AxisIndex::Z) == AxisIndex::X);

    // returns a char representation of the given `AxisIndex`
    char ToChar(AxisIndex axis)
    {
        static_assert(static_cast<size_t>(AxisIndex::TOTAL) == 3);
        switch (axis)
        {
        case AxisIndex::X:
            return 'x';
        case AxisIndex::Y:
            return 'y';
        case AxisIndex::Z:
        default:
            return 'z';
        }
    }

    // returns `c` parsed as an `AxisIndex`, or `std::nullopt` if the given char
    // cannot be parsed as an axis index
    std::optional<AxisIndex> ParseAxisIndex(char c)
    {
        switch (c)
        {
        case 'x':
        case 'X':
            return AxisIndex::X;
        case 'y':
        case 'Y':
            return AxisIndex::Y;
        case 'z':
        case 'Z':
            return AxisIndex::Z;
        default:
            return std::nullopt;
        }
    }

    // returns the integer index equivalent of the given `AxisIndex`
    ptrdiff_t ToIndex(AxisIndex axis)
    {
        return static_cast<ptrdiff_t>(axis);
    }

    // the potentially negated index of an axis in n-dimensional space
    struct MaybeNegatedAxis final {
        MaybeNegatedAxis(
            AxisIndex axisIndex_,
            bool isNegated_) :

            axisIndex{axisIndex_},
            isNegated{isNegated_}
        {
        }
        AxisIndex axisIndex;
        bool isNegated;
    };

    // returns `true` if the arguments are orthogonal to eachover; otherwise, returns `false`
    bool IsOrthogonal(MaybeNegatedAxis const& a, MaybeNegatedAxis const& b)
    {
        return a.axisIndex != b.axisIndex;
    }

    // returns a (possibly negated) `AxisIndex` parsed from the given input
    //
    // if the input is invalid in some way, returns `std::nullopt`
    std::optional<MaybeNegatedAxis> ParseAxisDimension(std::string_view s)
    {
        if (s.empty())
        {
            return std::nullopt;
        }

        // handle (+consume) sign
        bool isNegated = false;
        switch (s.front())
        {
        case '-':
            isNegated = true;
            [[fallthrough]];
        case '+':
            s = s.substr(1);
            break;
        }

        if (s.empty())
        {
            return std::nullopt;
        }

        std::optional<AxisIndex> const maybeAxisIndex = ParseAxisIndex(s.front());

        if (!maybeAxisIndex)
        {
            return std::nullopt;
        }

        return MaybeNegatedAxis{*maybeAxisIndex, isNegated};
    }

    // returns a string representation of the given (possibly negated) axis
    std::string ToString(MaybeNegatedAxis const& ax)
    {
        std::string rv;
        rv.reserve(2);
        rv.push_back(ax.isNegated ? '-' : '+');
        rv.push_back(ToChar(ax.axisIndex));
        return rv;
    }

    // a frame that is defined by:
    //
    // - an "axis" edge
    // - a designation of what axis the "axis" edge lies along
    // - an "other" edge, which should be non-parallel to the "axis" edge
    // - a desgination of what axis the cross product `axis x other` lies along
    // - an "origin" point, which is where the origin of the frame should be defined
    class LandmarkDefinedFrame final : public OpenSim::PhysicalFrame {
        OpenSim_DECLARE_CONCRETE_OBJECT(LandmarkDefinedFrame, Frame)
    public:
        OpenSim_DECLARE_SOCKET(axisEdge, FDVirtualEdge, "The edge from which to create the first axis");
        OpenSim_DECLARE_SOCKET(otherEdge, FDVirtualEdge, "Some other edge that is non-parallel to `axisEdge` and can be used (via a cross product) to define the frame");
        OpenSim_DECLARE_SOCKET(origin, OpenSim::Point, "The origin (position) of the frame");
        OpenSim_DECLARE_PROPERTY(axisEdgeDimension, std::string, "The dimension to assign to `axisEdge`. Can be -x, +x, -y, +y, -z, or +z");
        OpenSim_DECLARE_PROPERTY(secondAxisDimension, std::string, "The dimension to assign to the second axis that is generated from the cross-product of `axisEdge` with `otherEdge`. Can be -x, +x, -y, +y, -z, or +z and must be orthogonal to `axisEdgeDimension`");
        OpenSim_DECLARE_PROPERTY(forceShowingFrame, bool, "Whether to forcibly show the frame's decoration, even if showing frames is disabled at the model-level (decorative)");

        LandmarkDefinedFrame()
        {
            constructProperty_axisEdgeDimension("+x");
            constructProperty_secondAxisDimension("+y");
            constructProperty_forceShowingFrame(true);
        }

    private:
        void generateDecorations(
            bool,
            const ModelDisplayHints&,
            const SimTK::State& state,
            SimTK::Array_<SimTK::DecorativeGeometry>& appendOut) const final
        {
            if (get_forceShowingFrame() &&
                !getModel().get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames())
            {
                appendOut.push_back(CreateDecorativeFrame(
                    getTransformInGround(state)
                ));
            }
        }

        void extendFinalizeFromProperties() final
        {
            // ensure `axisEdge` is a correct property value
            std::optional<MaybeNegatedAxis> const maybeAxisEdge = ParseAxisDimension(get_axisEdgeDimension());
            if (!maybeAxisEdge)
            {
                std::stringstream ss;
                ss << getProperty_axisEdgeDimension().getName() << ": has an invalid value ('" << get_axisEdgeDimension() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
            }
            MaybeNegatedAxis const& axisEdge = *maybeAxisEdge;

            // ensure `otherEdge` is a correct property value
            std::optional<MaybeNegatedAxis> const maybeOtherEdge = ParseAxisDimension(get_secondAxisDimension());
            if (!maybeOtherEdge)
            {
                std::stringstream ss;
                ss << getProperty_secondAxisDimension().getName() << ": has an invalid value ('" << get_secondAxisDimension() << "'): permitted values are -x, +x, -y, +y, -z, or +z";
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
            }
            MaybeNegatedAxis const& otherEdge = *maybeOtherEdge;

            // ensure `axisEdge` is orthogonal to `otherEdge`
            if (!IsOrthogonal(axisEdge, otherEdge))
            {
                std::stringstream ss;
                ss << getProperty_axisEdgeDimension().getName() << " (" << get_axisEdgeDimension() << ") and " << getProperty_secondAxisDimension().getName() << " (" << get_secondAxisDimension() << ") are not orthogonal";
                OPENSIM_THROW_FRMOBJ(OpenSim::Exception, std::move(ss).str());
            }
        }

        SimTK::Transform calcTransformInGround(SimTK::State const& state) const final
        {
            // parse axis dimension string
            std::optional<MaybeNegatedAxis> const ax1 = ParseAxisDimension(get_axisEdgeDimension());
            std::optional<MaybeNegatedAxis> const ax2 = ParseAxisDimension(get_secondAxisDimension());

            // validation check
            if (!ax1 || !ax2 || !IsOrthogonal(*ax1, *ax2))
            {
                return SimTK::Transform{};  // error fallback
            }

            // get other components via sockets
            EdgePoints const axisEdgePoints = getConnectee<FDVirtualEdge>("axisEdge").getEdgePointsInGround(state);
            EdgePoints const otherEdgePoints = getConnectee<FDVirtualEdge>("otherEdge").getEdgePointsInGround(state);
            SimTK::Vec3 const originPointInGround = getConnectee<OpenSim::Point>("origin").getLocationInGround(state);

            // this is what the algorithm must ultimately compute in order to
            // calculate a change-of-basis (rotation) matrix
            std::array<SimTK::UnitVec3, 3> axes{};
            static_assert(axes.size() == static_cast<size_t>(AxisIndex::TOTAL));

            // assign first axis
            SimTK::UnitVec3& firstAxisDir = axes.at(ToIndex(ax1->axisIndex));
            {
                firstAxisDir = CalcDirection(axisEdgePoints);
                if (ax1->isNegated)
                {
                    firstAxisDir = -firstAxisDir;
                }
            }

            // compute second axis (via cross product)
            SimTK::UnitVec3& secondAxisDir = axes.at(ToIndex(ax2->axisIndex));
            {
                SimTK::UnitVec3 const otherEdgeDir = CalcDirection(otherEdgePoints);
                secondAxisDir = SimTK::UnitVec3{SimTK::cross(firstAxisDir, otherEdgeDir)};
                if (ax2->isNegated)
                {
                    secondAxisDir = -secondAxisDir;
                }
            }

            // compute third axis (via cross product)
            struct ThirdEdgeOperands
            {
                SimTK::UnitVec3 const& firstDir;
                SimTK::UnitVec3 const& secondDir;
                AxisIndex resultAxisIndex;
            };
            ThirdEdgeOperands const ops = Next(ax1->axisIndex) == ax2->axisIndex ?
                ThirdEdgeOperands{firstAxisDir, secondAxisDir, Next(ax2->axisIndex)} :
                ThirdEdgeOperands{secondAxisDir, firstAxisDir, Next(ax1->axisIndex)};

            axes.at(ToIndex(ops.resultAxisIndex)) = SimTK::UnitVec3{SimTK::cross(ops.firstDir, ops.secondDir)};

            // create transform from parts
            SimTK::Mat33 rotationMatrix;
            rotationMatrix.col(0) = SimTK::Vec3{axes[0]};
            rotationMatrix.col(1) = SimTK::Vec3{axes[1]};
            rotationMatrix.col(2) = SimTK::Vec3{axes[2]};
            SimTK::Rotation const rotation{rotationMatrix};
            SimTK::Transform const transform{rotation, originPointInGround};

            return transform;
        }

        SimTK::SpatialVec calcVelocityInGround(SimTK::State const& state) const final
        {
            return {};  // TODO: see OffsetFrame::calcVelocityInGround
        }

        SimTK::SpatialVec calcAccelerationInGround(SimTK::State const& state) const final
        {
            return {};  // TODO: see OffsetFrame::calcAccelerationInGround
        }

        Frame const& extendFindBaseFrame() const
        {
            return *this;
        }

        SimTK::Transform extendFindTransformInBaseFrame() const final
        {
            return {};
        }

        void extendAddToSystem(SimTK::MultibodySystem& system) const final
        {
            Super::extendAddToSystem(system);
            setMobilizedBodyIndex(getModel().getGround().getMobilizedBodyIndex()); // TODO: the frame must be associated to a mobod
        }
    };
}

// top-level helper functions
namespace
{
    // custom helper that customizes the OpenSim model defaults to be more
    // suitable for the frame definition UI
    std::shared_ptr<osc::UndoableModelStatePair> MakeSharedUndoableFrameDefinitionModel()
    {
        auto model = std::make_unique<OpenSim::Model>();
        model->updDisplayHints().set_show_frames(false);
        return std::make_shared<osc::UndoableModelStatePair>(std::move(model));
    }

    // gets the next unique suffix numer for geometry
    int32_t GetNextGlobalGeometrySuffix()
    {
        static std::atomic<int32_t> s_GeometryCounter = 0;
        return s_GeometryCounter++;
    }

    bool IsPoint(OpenSim::Component const& component)
    {
        return dynamic_cast<OpenSim::Point const*>(&component);
    }

    bool IsEdge(OpenSim::Component const& component)
    {
        return dynamic_cast<OpenSim::FDVirtualEdge const*>(&component);
    }

    void SetupDefault3DViewportRenderingParams(osc::ModelRendererParams& renderParams)
    {
        renderParams.renderingOptions.setDrawFloor(false);
        renderParams.overlayOptions.setDrawXZGrid(true);
        renderParams.backgroundColor = {48.0f/255.0f, 48.0f/255.0f, 48.0f/255.0f, 1.0f};
    }
}

// choose `n` components UI flow
namespace
{
    // parameters used to create a "choose components" layer
    struct ChooseComponentsEditorLayerParameters final {
        std::string popupHeaderText = "choose something";

        bool userCanChoosePoints = true;
        bool userCanChooseEdges = true;

        // (maybe) the components that the user has already chosen, or is
        // assigning to (and, therefore, should maybe be highlighted but
        // non-selectable)
        std::unordered_set<std::string> componentsBeingAssignedTo;

        size_t numComponentsUserMustChoose = 1;

        std::function<bool(std::unordered_set<std::string> const&)> onUserFinishedChoosing = [](std::unordered_set<std::string> const&)
        {
            return true;
        };
    };

    // top-level shared state for the "choose components" layer
    struct ChooseComponentsEditorLayerSharedState final {

        explicit ChooseComponentsEditorLayerSharedState(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            ChooseComponentsEditorLayerParameters parameters_) :

            model{std::move(model_)},
            popupParams{std::move(parameters_)}
        {
        }

        std::shared_ptr<osc::MeshCache> meshCache = osc::App::singleton<osc::MeshCache>();
        std::shared_ptr<osc::UndoableModelStatePair> model;
        ChooseComponentsEditorLayerParameters popupParams;
        osc::ModelRendererParams renderParams;
        std::string hoveredComponent;
        std::unordered_set<std::string> alreadyChosenComponents;
        bool shouldClosePopup = false;
    };

    // grouping of scene (3D) decorations and an associated scene BVH
    struct BVHedDecorations final {
        void clear()
        {
            decorations.clear();
            bvh.clear();
        }

        std::vector<osc::SceneDecoration> decorations;
        osc::BVH bvh;
    };

    // generates scene decorations for the "choose components" layer
    void GenerateChooseComponentsDecorations(
        ChooseComponentsEditorLayerSharedState const& state,
        BVHedDecorations& out)
    {
        out.clear();

        auto const onModelDecoration = [&state, &out](OpenSim::Component const& component, osc::SceneDecoration&& decoration)
        {
            // update flags based on path
            std::string const absPath = osc::GetAbsolutePathString(component);
            if (osc::Contains(state.popupParams.componentsBeingAssignedTo, absPath))
            {
                decoration.flags |= osc::SceneDecorationFlags_IsSelected;
            }
            if (osc::Contains(state.alreadyChosenComponents, absPath))
            {
                decoration.flags |= osc::SceneDecorationFlags_IsSelected;
            }
            if (absPath == state.hoveredComponent)
            {
                decoration.flags |= osc::SceneDecorationFlags_IsHovered;
            }

            if (state.popupParams.userCanChoosePoints && IsPoint(component))
            {
                decoration.id = absPath;
            }
            else if (state.popupParams.userCanChooseEdges && IsEdge(component))
            {
                decoration.id = absPath;
            }
            else
            {
                decoration.color.a *= 0.2f;  // fade non-selectable objects
            }

            out.decorations.push_back(std::move(decoration));
        };

        osc::GenerateModelDecorations(
            *state.meshCache,
            state.model->getModel(),
            state.model->getState(),
            state.renderParams.decorationOptions,
            state.model->getFixupScaleFactor(),
            onModelDecoration
        );

        osc::UpdateSceneBVH(out.decorations, out.bvh);

        auto const onOverlayDecoration = [&](osc::SceneDecoration&& decoration)
        {
            out.decorations.push_back(std::move(decoration));
        };

        osc::GenerateOverlayDecorations(
            *state.meshCache,
            state.renderParams.overlayOptions,
            out.bvh,
            onOverlayDecoration
        );
    }

    // modal popup that prompts the user to select components in the model (e.g.
    // to define an edge, or a frame)
    class ChooseComponentsEditorLayer final : public osc::ModelEditorViewerPanelLayer {
    public:
        ChooseComponentsEditorLayer(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            ChooseComponentsEditorLayerParameters parameters_) :

            m_State{std::move(model_), std::move(parameters_)},
            m_Renderer{osc::App::get().config(), *osc::App::singleton<osc::MeshCache>(), *osc::App::singleton<osc::ShaderCache>()}
        {
        }

    private:
        bool implHandleKeyboardInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {


            return osc::UpdatePolarCameraFromImGuiKeyboardInputs(
                params.updRenderParams().camera,
                state.viewportRect,
                m_Decorations.bvh.getRootAABB()
            );
        }

        bool implHandleMouseInputs(
            osc::ModelEditorViewerPanelParameters& params,
            osc::ModelEditorViewerPanelState& state) final
        {
            bool rv = osc::UpdatePolarCameraFromImGuiMouseInputs(
                osc::Dimensions(state.viewportRect),
                params.updRenderParams().camera
            );

            if (osc::IsDraggingWithAnyMouseButtonDown())
            {
                m_State.hoveredComponent = {};
            }

            if (m_IsLeftClickReleasedWithoutDragging)
            {
                rv = tryToggleHover() || rv;
            }

            return rv;
        }

        void implOnDraw(
            osc::ModelEditorViewerPanelParameters& panelParams,
            osc::ModelEditorViewerPanelState& panelState) final
        {
            bool const layerIsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

            // update this layer's state from provided state
            m_State.renderParams = panelParams.getRenderParams();
            m_IsLeftClickReleasedWithoutDragging = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
            m_IsRightClickReleasedWithoutDragging = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
            if (ImGui::IsKeyReleased(ImGuiKey_Escape))
            {
                m_State.shouldClosePopup = true;
            }

            // generate decorations + rendering params
            GenerateChooseComponentsDecorations(m_State, m_Decorations);
            osc::SceneRendererParams const rendererParameters = osc::CalcSceneRendererParams(
                m_State.renderParams,
                osc::Dimensions(panelState.viewportRect),
                osc::App::get().getMSXAASamplesRecommended(),
                m_State.model->getFixupScaleFactor()
            );

            // render to a texture (no caching)
            m_Renderer.draw(m_Decorations.decorations, rendererParameters);

            // blit texture as ImGui image
            osc::DrawTextureAsImGuiImage(
                m_Renderer.updRenderTexture(),
                osc::Dimensions(panelState.viewportRect)
            );

            // do hovertest
            if (layerIsHovered)
            {
                std::optional<osc::SceneCollision> const collision = osc::GetClosestCollision(
                    m_Decorations.bvh,
                    m_Decorations.decorations,
                    m_State.renderParams.camera,
                    ImGui::GetMousePos(),
                    panelState.viewportRect
                );
                if (collision)
                {
                    m_State.hoveredComponent = collision->decorationID;
                }
                else
                {
                    m_State.hoveredComponent = {};
                }
            }

            // show tooltip
            if (OpenSim::Component const* c = osc::FindComponent(m_State.model->getModel(), m_State.hoveredComponent))
            {
                osc::DrawComponentHoverTooltip(*c);
            }

            // show header
            ImGui::SetCursorScreenPos(panelState.viewportRect.p1);
            ImGui::TextUnformatted(m_State.popupParams.popupHeaderText.c_str());

            // handle completion state (i.e. user selected enough components)
            if (m_State.alreadyChosenComponents.size() == m_State.popupParams.numComponentsUserMustChoose)
            {
                m_State.popupParams.onUserFinishedChoosing(m_State.alreadyChosenComponents);
                m_State.shouldClosePopup = true;
            }
        }

        float implGetBackgroundAlpha() const final
        {
            return 1.0f;
        }

        bool implShouldClose() const final
        {
            return m_State.shouldClosePopup;
        }

        bool tryToggleHover()
        {
            std::string const& absPath = m_State.hoveredComponent;
            OpenSim::Component const* component = osc::FindComponent(m_State.model->getModel(), absPath);

            if (!component)
            {
                return false;  // nothing hovered
            }
            else if (osc::Contains(m_State.popupParams.componentsBeingAssignedTo, absPath))
            {
                return false;  // cannot be selected
            }
            else if (auto it = m_State.alreadyChosenComponents.find(absPath); it != m_State.alreadyChosenComponents.end())
            {
                m_State.alreadyChosenComponents.erase(it);
                return true;   // de-selected
            }
            else if (
                m_State.alreadyChosenComponents.size() < m_State.popupParams.numComponentsUserMustChoose &&
                (
                    (m_State.popupParams.userCanChoosePoints && IsPoint(*component)) ||
                    (m_State.popupParams.userCanChooseEdges && IsEdge(*component))
                )
            )
            {
                m_State.alreadyChosenComponents.insert(absPath);
                return true;   // selected
            }
            else
            {
                return false;  // don't know how to handle
            }
        }

        ChooseComponentsEditorLayerSharedState m_State;
        BVHedDecorations m_Decorations;
        osc::SceneRenderer m_Renderer;
        bool m_IsLeftClickReleasedWithoutDragging = false;
        bool m_IsRightClickReleasedWithoutDragging = false;
    };
}

// user-enactable actions
namespace
{
    void ActionPromptUserToAddMeshFile(osc::UndoableModelStatePair& model)
    {
        std::optional<std::filesystem::path> const maybeMeshPath =
            osc::PromptUserForFile(osc::GetCommaDelimitedListOfSupportedSimTKMeshFormats());
        if (!maybeMeshPath)
        {
            return;  // user didn't select anything
        }
        std::filesystem::path const& meshPath = *maybeMeshPath;
        std::string const meshName = osc::FileNameWithoutExtension(meshPath);

        OpenSim::Model const& immutableModel = model.getModel();

        // add an offset frame that is connected to ground - this will become
        // the mesh's offset frame
        auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysicalOffsetFrame->setParentFrame(immutableModel.getGround());
        meshPhysicalOffsetFrame->setName(meshName + "_offset");

        // attach the mesh to the frame
        {
            auto mesh = std::make_unique<OpenSim::Mesh>(meshPath.string());
            mesh->setName(meshName);
            meshPhysicalOffsetFrame->attachGeometry(mesh.release());
        }

        // create a human-readable commit message
        std::string const commitMessage = [&meshPath]()
        {
            std::stringstream ss;
            ss << "added " << meshPath.filename();
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            mutableModel.addComponent(meshPhysicalOffsetFrame.release());
            mutableModel.finalizeConnections();

            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.commit(commitMessage);
        }
    }

    void ActionAddSphereInMeshFrame(
        osc::UndoableModelStatePair& model,
        OpenSim::Mesh const& mesh,
        std::optional<glm::vec3> const& maybeClickPosInGround)
    {
        // if the caller requests that the sphere is placed at a particular
        // location in ground, then place it in the correct location w.r.t.
        // the mesh frame
        SimTK::Vec3 translationInMeshFrame = {0.0, 0.0, 0.0};
        if (maybeClickPosInGround)
        {
            SimTK::Transform const mesh2ground = mesh.getFrame().getTransformInGround(model.getState());
            SimTK::Transform const ground2mesh = mesh2ground.invert();
            SimTK::Vec3 const translationInGround = osc::ToSimTKVec3(*maybeClickPosInGround);

            translationInMeshFrame = ground2mesh * translationInGround;
        }

        // generate sphere name
        std::string const sphereName = []()
        {
            std::stringstream ss;
            ss << "sphere_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        OpenSim::Model const& immutableModel = model.getModel();

        // attach the sphere to the mesh's frame
        std::unique_ptr<OpenSim::SphereLandmark> sphere = [&mesh, &translationInMeshFrame, &sphereName]()
        {
            auto sphere = std::make_unique<OpenSim::SphereLandmark>();
            sphere->setName(sphereName);
            sphere->set_location(translationInMeshFrame);
            sphere->connectSocket_parent_frame(mesh.getFrame());
            return sphere;
        }();

        // create a human-readable commit message
        std::string const commitMessage = [&sphereName]()
        {
            std::stringstream ss;
            ss << "added " << sphereName;
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            OpenSim::SphereLandmark const* const spherePtr = sphere.get();

            mutableModel.addComponent(sphere.release());
            mutableModel.finalizeConnections();
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);

            model.setSelected(spherePtr);
            model.commit(commitMessage);
        }
    }

    void ActionAddPointToPointEdge(
        osc::UndoableModelStatePair& model,
        OpenSim::Point const& pointA,
        OpenSim::Point const& pointB)
    {
        // generate edge name
        std::string const edgeName = []()
        {
            std::stringstream ss;
            ss << "edge_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        // create edge
        auto edge = std::make_unique<OpenSim::FDPointToPointEdge>();
        edge->connectSocket_pointA(pointA);
        edge->connectSocket_pointB(pointB);

        // create a human-readable commit message
        std::string const commitMessage = [&edgeName]()
        {
            std::stringstream ss;
            ss << "added " << edgeName;
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            OpenSim::FDPointToPointEdge const* edgePtr = edge.get();

            mutableModel.addComponent(edge.release());
            mutableModel.finalizeConnections();
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(edgePtr);
            model.commit(commitMessage);
        }
    }

    void ActionAddMidpoint(
        osc::UndoableModelStatePair& model,
        OpenSim::Point const& pointA,
        OpenSim::Point const& pointB)
    {
        // generate name
        std::string const midpointName = []()
        {
            std::stringstream ss;
            ss << "midpoint_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        // construct midpoint
        auto midpoint = std::make_unique<OpenSim::MidpointLandmark>();
        midpoint->connectSocket_pointA(pointA);
        midpoint->connectSocket_pointB(pointB);

        // create a human-readable commit message
        std::string const commitMessage = [&midpointName]()
        {
            std::stringstream ss;
            ss << "added " << midpointName;
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            OpenSim::MidpointLandmark const* midpointPtr = midpoint.get();

            mutableModel.addComponent(midpoint.release());
            mutableModel.finalizeConnections();
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(midpointPtr);
            model.commit(commitMessage);
        }
    }

    void ActionAddCrossProductEdge(
        osc::UndoableModelStatePair& model,
        OpenSim::FDVirtualEdge const& edgeA,
        OpenSim::FDVirtualEdge const& edgeB)
    {
        // generate name
        std::string const edgeName = []()
        {
            std::stringstream ss;
            ss << "crossproduct_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        // construct
        auto edge = std::make_unique<OpenSim::FDCrossProductEdge>();
        edge->connectSocket_edgeA(edgeA);
        edge->connectSocket_edgeB(edgeB);

        // create a human-readable commit message
        std::string const commitMessage = [&edgeName]()
        {
            std::stringstream ss;
            ss << "added " << edgeName;
            return std::move(ss).str();
        }();

        // finally, perform the model mutation
        {
            OpenSim::Model& mutableModel = model.updModel();
            OpenSim::FDCrossProductEdge const* edgePtr = edge.get();

            mutableModel.addComponent(edge.release());
            mutableModel.finalizeConnections();
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);
            model.setSelected(edgePtr);
            model.commit(commitMessage);
        }
    }

    void ActionPushCreateEdgeToOtherPointLayer(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::Point const& point,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        osc::ModelEditorViewerPanel* const visualizer =
            editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other point";
        options.userCanChoosePoints = true;
        options.userCanChooseEdges = false;
        options.componentsBeingAssignedTo = {point.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, pointAPath = point.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& pointBPath = *choices.begin();

            OpenSim::Point const* pointA = osc::FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (!pointA)
            {
                osc::log::error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            OpenSim::Point const* pointB = osc::FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (!pointB)
            {
                osc::log::error("point B's component path (%s) does not exist in the model", pointBPath.c_str());
                return false;
            }

            ActionAddPointToPointEdge(*model, *pointA, *pointB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void ActionPushCreateMidpointToAnotherPointLayer(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::Point const& point,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        osc::ModelEditorViewerPanel* const visualizer =
            editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other point";
        options.userCanChoosePoints = true;
        options.userCanChooseEdges = false;
        options.componentsBeingAssignedTo = {point.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, pointAPath = point.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& pointBPath = *choices.begin();

            OpenSim::Point const* pointA = osc::FindComponent<OpenSim::Point>(model->getModel(), pointAPath);
            if (!pointA)
            {
                osc::log::error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            OpenSim::Point const* pointB = osc::FindComponent<OpenSim::Point>(model->getModel(), pointBPath);
            if (!pointB)
            {
                osc::log::error("point B's component path (%s) does not exist in the model", pointBPath.c_str());
                return false;
            }

            ActionAddMidpoint(*model, *pointA, *pointB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void ActionPushCreateCrossProductEdgeLayer(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::FDVirtualEdge const& firstEdge,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        osc::ModelEditorViewerPanel* const visualizer =
            editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other edge";
        options.userCanChoosePoints = false;
        options.userCanChooseEdges = true;
        options.componentsBeingAssignedTo = {firstEdge.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [model, edgeAPath = firstEdge.getAbsolutePathString()](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& edgeBPath = *choices.begin();

            OpenSim::FDVirtualEdge const* edgeA = osc::FindComponent<OpenSim::FDVirtualEdge>(model->getModel(), edgeAPath);
            if (!edgeA)
            {
                osc::log::error("edge A's component path (%s) does not exist in the model", edgeAPath.c_str());
                return false;
            }

            OpenSim::FDVirtualEdge const* edgeB = osc::FindComponent<OpenSim::FDVirtualEdge>(model->getModel(), edgeBPath);
            if (!edgeB)
            {
                osc::log::error("point B's component path (%s) does not exist in the model", edgeBPath.c_str());
                return false;
            }

            ActionAddCrossProductEdge(*model, *edgeA, *edgeB);
            return true;
        };

        visualizer->pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void ActionSwapSocketAssignments(
        osc::UndoableModelStatePair& model,
        OpenSim::ComponentPath componentAbsPath,
        std::string firstSocketName,
        std::string secondSocketName)
    {
        // create commit message
        std::string const commitMessage = [&componentAbsPath, &firstSocketName, &secondSocketName]()
        {
            std::stringstream ss;
            ss << "swapped socket '" << firstSocketName << "' with socket '" << secondSocketName << " in " << componentAbsPath.getComponentName().c_str();
            return std::move(ss).str();
        }();

        // look things up in the mutable model
        OpenSim::Model& mutModel = model.updModel();
        OpenSim::Component* const component = osc::FindComponentMut(mutModel, componentAbsPath);
        if (!component)
        {
            osc::log::error("failed to find %s in model, skipping action", componentAbsPath.toString().c_str());
            return;
        }

        OpenSim::AbstractSocket* const firstSocket = osc::FindSocketMut(*component, firstSocketName);
        if (!firstSocket)
        {
            osc::log::error("failed to find socket %s in %s, skipping action", firstSocketName.c_str(), component->getName().c_str());
            return;
        }

        OpenSim::AbstractSocket* const secondSocket = osc::FindSocketMut(*component, secondSocketName);
        if (!secondSocket)
        {
            osc::log::error("failed to find socket %s in %s, skipping action", secondSocketName.c_str(), component->getName().c_str());
            return;
        }

        // perform swap
        std::string const firstSocketPath = firstSocket->getConnecteePath();
        firstSocket->setConnecteePath(secondSocket->getConnecteePath());
        secondSocket->setConnecteePath(firstSocketPath);

        // finalize and commit
        osc::InitializeModel(mutModel);
        osc::InitializeState(mutModel);
        model.commit(commitMessage);
    }

    void ActionSwapPointToPointEdgeEnds(
        osc::UndoableModelStatePair& model,
        OpenSim::FDPointToPointEdge const& edge)
    {
        ActionSwapSocketAssignments(model, edge.getAbsolutePath(), "pointA", "pointB");
    }

    void ActionAddFrame(
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::FDVirtualEdge const& firstEdge,
        OpenSim::MaybeNegatedAxis firstEdgeAxis,
        OpenSim::FDVirtualEdge const& otherEdge,
        OpenSim::Point const& origin)
    {
        // generate name
        std::string const frameName = []()
        {
            std::stringstream ss;
            ss << "frame_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        // generate commit message
        std::string const commitMessage = [&frameName]()
        {
            std::stringstream ss;
            ss << "added frame (" << frameName << ')';
            return std::move(ss).str();
        }();

        // create the frame
        auto frame = std::make_unique<OpenSim::LandmarkDefinedFrame>();
        frame->set_axisEdgeDimension(OpenSim::ToString(firstEdgeAxis));
        frame->connectSocket_axisEdge(firstEdge);
        frame->connectSocket_otherEdge(otherEdge);
        frame->connectSocket_origin(origin);

        // perform model mutation
        {
            OpenSim::Model& mutModel = model->updModel();
            OpenSim::LandmarkDefinedFrame const* const framePtr = frame.get();

            mutModel.addComponent(frame.release());
            mutModel.finalizeConnections();
            osc::InitializeModel(mutModel);
            osc::InitializeState(mutModel);
            model->setSelected(framePtr);
            model->commit(commitMessage);
        }
    }

    void ActionEnterPickOriginForFrameDefinition(
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::string const& firstEdgeAbsPath,
        OpenSim::MaybeNegatedAxis firstEdgeAxis,
        std::string const& secondEdgeAbsPath)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose frame origin";
        options.userCanChoosePoints = true;
        options.userCanChooseEdges = false;
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            model,
            firstEdgeAbsPath = firstEdgeAbsPath,
            firstEdgeAxis,
            secondEdgeAbsPath
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            if (choices.empty())
            {
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& originPath = *choices.begin();

            OpenSim::FDVirtualEdge const* firstEdge = osc::FindComponent<OpenSim::FDVirtualEdge>(model->getModel(), firstEdgeAbsPath);
            if (!firstEdge)
            {
                osc::log::error("the first edge's component path (%s) does not exist in the model", firstEdgeAbsPath.c_str());
                return false;
            }

            OpenSim::FDVirtualEdge const* otherEdge = osc::FindComponent<OpenSim::FDVirtualEdge>(model->getModel(), secondEdgeAbsPath);
            if (!otherEdge)
            {
                osc::log::error("the second edge's component path (%s) does not exist in the model", secondEdgeAbsPath.c_str());
                return false;
            }

            OpenSim::Point const* originPoint = osc::FindComponent<OpenSim::Point>(model->getModel(), originPath);
            if (!originPoint)
            {
                osc::log::error("the origin's component path (%s) does not exist in the model", originPath.c_str());
                return false;
            }

            ActionAddFrame(
                model,
                *firstEdge,
                firstEdgeAxis,
                *otherEdge,
                *originPoint
            );
            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void ActionEnterPickOtherEdgeStateForFrameDefinition(
        osc::ModelEditorViewerPanel& visualizer,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::FDVirtualEdge const& firstEdge,
        OpenSim::MaybeNegatedAxis firstEdgeAxis)
    {
        ChooseComponentsEditorLayerParameters options;
        options.popupHeaderText = "choose other edge";
        options.userCanChoosePoints = false;
        options.userCanChooseEdges = true;
        options.componentsBeingAssignedTo = {firstEdge.getAbsolutePathString()};
        options.numComponentsUserMustChoose = 1;
        options.onUserFinishedChoosing = [
            visualizerPtr = &visualizer,  // TODO: implement weak_ptr for panel lookup
            model,
            firstEdgeAbsPath = firstEdge.getAbsolutePathString(),
            firstEdgeAxis
        ](std::unordered_set<std::string> const& choices) -> bool
        {
            // go into "pick origin" state

            if (choices.empty())
            {
                osc::log::error("user selections from the 'choose components' layer was empty: this bug should be reported");
                return false;
            }
            if (choices.size() > 1)
            {
                osc::log::warn("number of user selections from 'choose components' layer was greater than expected: this bug should be reported");
            }
            std::string const& otherEdgePath = *choices.begin();

            ActionEnterPickOriginForFrameDefinition(
                *visualizerPtr,  // TODO: unsafe if not guarded by weak_ptr or similar
                model,
                firstEdgeAbsPath,
                firstEdgeAxis,
                otherEdgePath
            );
            return true;
        };

        visualizer.pushLayer(std::make_unique<ChooseComponentsEditorLayer>(model, options));
    }

    void ActionPushCreateFrameLayer(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        OpenSim::FDVirtualEdge const& firstEdge,
        OpenSim::MaybeNegatedAxis firstEdgeAxis,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent)
    {
        if (!maybeSourceEvent)
        {
            return;
        }

        osc::ModelEditorViewerPanel* const visualizer =
            editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);

        if (!visualizer)
        {
            return;  // can't figure out which visualizer to push the layer to
        }

        ActionEnterPickOtherEdgeStateForFrameDefinition(
            *visualizer,
            model,
            firstEdge,
            firstEdgeAxis
        );
    }
}

// context menu
namespace
{
    void DrawGenericRightClickComponentContextMenuActions(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Component const&)
    {
        if (ImGui::BeginMenu(ICON_FA_CAMERA " Focus Camera"))
        {
            if (ImGui::MenuItem("On Ground"))
            {
                osc::ModelEditorViewerPanel* visualizer =
                    editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer)
                {
                    visualizer->focusOn({});
                }
            }

            if (maybeSourceEvent &&
                maybeSourceEvent->maybeClickPositionInGround &&
                ImGui::MenuItem("On Click Position"))
            {
                osc::ModelEditorViewerPanel* visualizer =
                    editor.getPanelManager()->tryUpdPanelByNameT<osc::ModelEditorViewerPanel>(maybeSourceEvent->sourcePanelName);
                if (visualizer)
                {
                    visualizer->focusOn(*maybeSourceEvent->maybeClickPositionInGround);
                }
            }

            ImGui::EndMenu();
        }
    }

    void DrawGenericRightClickEdgeContextMenuActions(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::FDVirtualEdge const& edge)
    {
        if (maybeSourceEvent && ImGui::MenuItem(ICON_FA_TIMES " Create Cross Product Edge"))
        {
            ActionPushCreateCrossProductEdgeLayer(editor, model, edge, maybeSourceEvent);
        }

        if (maybeSourceEvent && ImGui::BeginMenu("     Create frame with this edge as"))
        {
            if (ImGui::MenuItem("+x"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    OpenSim::MaybeNegatedAxis{OpenSim::AxisIndex::X, false},
                    maybeSourceEvent
                );
            }

            if (ImGui::MenuItem("+y"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    OpenSim::MaybeNegatedAxis{OpenSim::AxisIndex::Y, false},
                    maybeSourceEvent
                );
            }

            if (ImGui::MenuItem("+z"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    OpenSim::MaybeNegatedAxis{OpenSim::AxisIndex::Z, false},
                    maybeSourceEvent
                );
            }

            ImGui::Separator();

            if (ImGui::MenuItem("-x"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    OpenSim::MaybeNegatedAxis{OpenSim::AxisIndex::X, true},
                    maybeSourceEvent
                );
            }

            if (ImGui::MenuItem("-y"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    OpenSim::MaybeNegatedAxis{OpenSim::AxisIndex::Y, true},
                    maybeSourceEvent
                );
            }

            if (ImGui::MenuItem("-z"))
            {
                ActionPushCreateFrameLayer(
                    editor,
                    model,
                    edge,
                    OpenSim::MaybeNegatedAxis{OpenSim::AxisIndex::Z, true},
                    maybeSourceEvent
                );
            }

            ImGui::EndMenu();
        }
    }

    void DrawRightClickedNothingContextMenu(
        osc::UndoableModelStatePair& model)
    {
        osc::DrawNothingRightClickedContextMenuHeader();
        osc::DrawContextMenuSeparator();

        if (ImGui::MenuItem(ICON_FA_CUBE " Add Mesh"))
        {
            ActionPromptUserToAddMeshFile(model);
        }
    }

    void DrawRightClickedMeshContextMenu(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Mesh const& mesh)
    {
        osc::DrawRightClickedComponentContextMenuHeader(mesh);
        osc::DrawContextMenuSeparator();

        if (ImGui::MenuItem(ICON_FA_CIRCLE " Add Sphere"))
        {
            ActionAddSphereInMeshFrame(
                *model,
                mesh,
                maybeSourceEvent ? maybeSourceEvent->maybeClickPositionInGround : std::nullopt
            );
        }

        DrawGenericRightClickComponentContextMenuActions(editor, model, maybeSourceEvent, mesh);
    }

    void DrawRightClickedPointContextMenu(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Point const& point)
    {
        osc::DrawRightClickedComponentContextMenuHeader(point);
        osc::DrawContextMenuSeparator();

        if (maybeSourceEvent && ImGui::MenuItem(ICON_FA_GRIP_LINES " Create Edge"))
        {
            ActionPushCreateEdgeToOtherPointLayer(editor, model, point, maybeSourceEvent);
        }

        if (maybeSourceEvent && ImGui::MenuItem(ICON_FA_DOT_CIRCLE " Create Midpoint"))
        {
            ActionPushCreateMidpointToAnotherPointLayer(editor, model, point, maybeSourceEvent);
        }

        DrawGenericRightClickComponentContextMenuActions(editor, model, maybeSourceEvent, point);
    }

    void DrawRightClickedPointToPointEdgeContextMenu(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::FDPointToPointEdge const& edge)
    {
        osc::DrawRightClickedComponentContextMenuHeader(edge);
        osc::DrawContextMenuSeparator();
        DrawGenericRightClickEdgeContextMenuActions(editor, model, maybeSourceEvent, edge);
        if (ImGui::MenuItem(ICON_FA_RECYCLE " Swap Direction"))
        {
            ActionSwapPointToPointEdgeEnds(*model, edge);
        }
        DrawGenericRightClickComponentContextMenuActions(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedCrossProductEdgeContextMenu(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::FDCrossProductEdge const& edge)
    {
        osc::DrawRightClickedComponentContextMenuHeader(edge);
        osc::DrawContextMenuSeparator();
        DrawGenericRightClickEdgeContextMenuActions(editor, model, maybeSourceEvent, edge);
        DrawGenericRightClickComponentContextMenuActions(editor, model, maybeSourceEvent, edge);
    }

    void DrawRightClickedUnknownComponentContextMenu(
        osc::EditorAPI& editor,
        std::shared_ptr<osc::UndoableModelStatePair> model,
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> const& maybeSourceEvent,
        OpenSim::Component const& component)
    {
        osc::DrawRightClickedComponentContextMenuHeader(component);
        osc::DrawContextMenuSeparator();
        DrawGenericRightClickComponentContextMenuActions(editor, model, maybeSourceEvent, component);
    }

    // popup state for the frame definition tab's general context menu
    class FrameDefinitionContextMenu final : public osc::StandardPopup {
    public:
        FrameDefinitionContextMenu(
            std::string_view popupName_,
            osc::EditorAPI* editorAPI_,
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            OpenSim::ComponentPath componentPath_,
            std::optional<osc::ModelEditorViewerPanelRightClickEvent> maybeSourceVisualizerEvent_ = std::nullopt) :

            StandardPopup{popupName_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMove},
            m_EditorAPI{editorAPI_},
            m_Model{std::move(model_)},
            m_ComponentPath{std::move(componentPath_)},
            m_MaybeSourceVisualizerEvent{maybeSourceVisualizerEvent_}
        {
            OSC_ASSERT(m_EditorAPI != nullptr);
            OSC_ASSERT(m_Model != nullptr);

            setModal(false);
        }

    private:
        void implDrawContent() final
        {
            OpenSim::Component const* const maybeComponent = osc::FindComponent(m_Model->getModel(), m_ComponentPath);
            if (!maybeComponent)
            {
                DrawRightClickedNothingContextMenu(*m_Model);
            }
            else if (OpenSim::Mesh const* maybeMesh = dynamic_cast<OpenSim::Mesh const*>(maybeComponent))
            {
                DrawRightClickedMeshContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeMesh);
            }
            else if (OpenSim::Point const* maybePoint = dynamic_cast<OpenSim::Point const*>(maybeComponent))
            {
                DrawRightClickedPointContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybePoint);
            }
            else if (OpenSim::FDPointToPointEdge const* maybeP2PEdge = dynamic_cast<OpenSim::FDPointToPointEdge const*>(maybeComponent))
            {
                DrawRightClickedPointToPointEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeP2PEdge);
            }
            else if (OpenSim::FDCrossProductEdge const* maybeCPEdge = dynamic_cast<OpenSim::FDCrossProductEdge const*>(maybeComponent))
            {
                DrawRightClickedCrossProductEdgeContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeCPEdge);
            }
            else
            {
                DrawRightClickedUnknownComponentContextMenu(*m_EditorAPI, m_Model, m_MaybeSourceVisualizerEvent, *maybeComponent);
            }
        }

        osc::EditorAPI* m_EditorAPI;
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        OpenSim::ComponentPath m_ComponentPath;
        std::optional<osc::ModelEditorViewerPanelRightClickEvent> m_MaybeSourceVisualizerEvent;
    };
}

// other panels/widgets
namespace
{
    class FrameDefinitionTabNavigatorPanel final : public osc::StandardPanel {
    public:
        FrameDefinitionTabNavigatorPanel(std::string_view panelName_) :
            StandardPanel{panelName_}
        {
        }
    private:
        void implDrawContent() final
        {
            ImGui::Text("TODO: draw navigator content");
        }
    };

    class FrameDefinitionTabMainMenu final {
    public:
        explicit FrameDefinitionTabMainMenu(
            std::shared_ptr<osc::UndoableModelStatePair> model_,
            std::shared_ptr<osc::PanelManager> panelManager_) :
            m_Model{std::move(model_)},
            m_WindowMenu{std::move(panelManager_)}
        {
        }

        void draw()
        {
            drawEditMenu();
            m_WindowMenu.draw();
            m_AboutMenu.draw();
        }

    private:
        void drawEditMenu()
        {
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem(ICON_FA_UNDO " Undo", nullptr, false, m_Model->canUndo()))
                {
                    osc::ActionUndoCurrentlyEditedModel(*m_Model);
                }

                if (ImGui::MenuItem(ICON_FA_REDO " Redo", nullptr, false, m_Model->canRedo()))
                {
                    osc::ActionRedoCurrentlyEditedModel(*m_Model);
                }
                ImGui::EndMenu();
            }
        }

        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        osc::WindowMenu m_WindowMenu;
        osc::MainMenuAboutTab m_AboutMenu;
    };
}

class osc::FrameDefinitionTab::Impl final : public EditorAPI {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        // register user-visible panels that this tab can host

        m_PanelManager->registerToggleablePanel(
            "Navigator",
            [this](std::string_view panelName)
            {
                return std::make_shared<FrameDefinitionTabNavigatorPanel>(
                    panelName
                );
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Navigator (legacy)",
            [this](std::string_view panelName)
            {
                return std::make_shared<NavigatorPanel>(
                    panelName,
                    m_Model
                );
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Properties",
            [this](std::string_view panelName)
            {
                return std::make_shared<PropertiesPanel>(panelName, this, m_Model);
            }
        );
        m_PanelManager->registerToggleablePanel(
            "Log",
            [this](std::string_view panelName)
            {
                return std::make_shared<LogViewerPanel>(panelName);
            }
        );
        m_PanelManager->registerSpawnablePanel(
            "viewer",
            [this](std::string_view panelName)
            {
                ModelEditorViewerPanelParameters panelParams
                {
                    m_Model,
                    [this](ModelEditorViewerPanelRightClickEvent const& e)
                    {
                        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
                            "##ContextMenu",
                            this,
                            m_Model,
                            e.componentAbsPathOrEmpty,
                            e
                        ));
                    }
                };
                SetupDefault3DViewportRenderingParams(panelParams.updRenderParams());

                return std::make_shared<ModelEditorViewerPanel>(panelName, panelParams);
            },
            1
        );
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_PanelManager->onMount();
        m_PopupManager.onMount();
    }

    void onUnmount()
    {
        m_PanelManager->onUnmount();
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_KEYDOWN)
        {
            return onKeydownEvent(e.key);
        }
        else
        {
            return false;
        }
    }

    void onTick()
    {
        m_PanelManager->onTick();
    }

    void onDrawMainMenu()
    {
        m_MainMenu.draw();
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
        m_PanelManager->onDraw();
        m_PopupManager.draw();
    }

private:
    bool onKeydownEvent(SDL_KeyboardEvent const& e)
    {
        bool const ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && e.keysym.mod & KMOD_SHIFT && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Shift+Z: redo
            osc::ActionRedoCurrentlyEditedModel(*m_Model);
            return true;
        }
        else if (ctrlOrSuperDown && e.keysym.sym == SDLK_z)
        {
            // Ctrl+Z: undo
            osc::ActionUndoCurrentlyEditedModel(*m_Model);
            return true;
        }
        else
        {
            return false;
        }
    }

    void implPushComponentContextMenuPopup(OpenSim::ComponentPath const& componentPath) final
    {
        pushPopup(std::make_unique<FrameDefinitionContextMenu>(
            "##ContextMenu",
            this,
            m_Model,
            componentPath
        ));
    }

    void implPushPopup(std::unique_ptr<Popup> popup) final
    {
        popup->open();
        m_PopupManager.push_back(std::move(popup));
    }

    void implAddMusclePlot(OpenSim::Coordinate const&, OpenSim::Muscle const&)
    {
        // ignore: not applicable in this tab
    }

    std::shared_ptr<PanelManager> implGetPanelManager()
    {
        return m_PanelManager;
    }

    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;

    std::shared_ptr<UndoableModelStatePair> m_Model = MakeSharedUndoableFrameDefinitionModel();
    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>();
    PopupManager m_PopupManager;

    FrameDefinitionTabMainMenu m_MainMenu{m_Model, m_PanelManager};
};


// public API (PIMPL)

osc::CStringView osc::FrameDefinitionTab::id() noexcept
{
    return c_TabStringID;
}

osc::FrameDefinitionTab::FrameDefinitionTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::FrameDefinitionTab::FrameDefinitionTab(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab& osc::FrameDefinitionTab::operator=(FrameDefinitionTab&&) noexcept = default;
osc::FrameDefinitionTab::~FrameDefinitionTab() noexcept = default;

osc::UID osc::FrameDefinitionTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::FrameDefinitionTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::FrameDefinitionTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::FrameDefinitionTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::FrameDefinitionTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::FrameDefinitionTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::FrameDefinitionTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::FrameDefinitionTab::implOnDraw()
{
    m_Impl->onDraw();
}
