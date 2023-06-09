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
        SimTK::Transform t;
        t.setP(position);
        sphere.setTransform(t);
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

    // virtual base class for "a point in the frame definition scene"
    class FDVirtualPoint : public ModelComponent {
        OpenSim_DECLARE_ABSTRACT_OBJECT(FDVirtualPoint, ModelComponent)
    public:
        SimTK::Vec3 getPositionInGround(SimTK::State const& state) const
        {
            return implGetPositionInGround(state);
        }
    private:
        virtual SimTK::Vec3 implGetPositionInGround(SimTK::State const&) const = 0;
    };

    // "the center of a sphere located on a frame"
    class FDSphere final : public FDVirtualPoint {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDSphere, FDVirtualPoint)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the sphere (decorative)");
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the sphere (decorative)");
        OpenSim_DECLARE_SOCKET(frame, Frame, "The frame to which the sphere is attached");

        FDSphere()
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
                getPositionInGround(state),
                get_Appearance()
            ));
        }

    private:
        SimTK::Vec3 implGetPositionInGround(SimTK::State const& state) const final
        {
            return getConnectee<Frame>("frame").getPositionInGround(state);
        }
    };

    // "a point between two other points"
    class FDMidpoint final : public FDVirtualPoint {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDMidpoint, FDVirtualPoint)
    public:
        OpenSim_DECLARE_PROPERTY(radius, double, "The radius of the midpoint (decorative)");
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the midpoint (decorative)");
        OpenSim_DECLARE_SOCKET(pointA, FDVirtualPoint, "The first point that the midpoint is between");
        OpenSim_DECLARE_SOCKET(pointB, FDVirtualPoint, "The second point that the midpoint is between");

        FDMidpoint()
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
                getPositionInGround(state),
                get_Appearance()
            ));
        }

    private:
        SimTK::Vec3 implGetPositionInGround(SimTK::State const& state) const final
        {
            SimTK::Vec3 const pointAPos = getConnectee<FDVirtualPoint>("pointA").getPositionInGround(state);
            SimTK::Vec3 const pointBPos = getConnectee<FDVirtualPoint>("pointB").getPositionInGround(state);
            return 0.5*(pointAPos + pointBPos);
        }
    };

    // the start and end locations of an edge in 3D space
    struct EdgePoints final {
        SimTK::Vec3 start;
        SimTK::Vec3 end;
    };

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

    // virtual base class for "an edge that starts at one location and ends at another"
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

    // "an edge derived from two virtual points"
    class FDPointToPointEdge final : public FDVirtualEdge {
        OpenSim_DECLARE_CONCRETE_OBJECT(FDPointToPointEdge, FDVirtualEdge)
    public:
        OpenSim_DECLARE_UNNAMED_PROPERTY(Appearance, "The appearance of the edge (decorative)");
        OpenSim_DECLARE_SOCKET(pointA, FDVirtualPoint, "The first point that the edge is connected to");
        OpenSim_DECLARE_SOCKET(pointB, FDVirtualPoint, "The second point that the edge is connected to");

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
            FDVirtualPoint const& pointA = getConnectee<FDVirtualPoint>("pointA");
            SimTK::Vec3 const pointAGroundLoc = pointA.getPositionInGround(state);

            FDVirtualPoint const& pointB = getConnectee<FDVirtualPoint>("pointB");
            SimTK::Vec3 const pointBGroundLoc = pointB.getPositionInGround(state);

            return {pointAGroundLoc, pointBGroundLoc};
        }
    };

    // "an edge calculated from the cross product between two other edges"
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
        return dynamic_cast<OpenSim::FDVirtualPoint const*>(&component);
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

        // add an offset frame to the mesh: this is how the sphere can be
        // freely moved in the scene
        auto meshPhysicalOffsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        meshPhysicalOffsetFrame->setParentFrame(dynamic_cast<OpenSim::PhysicalFrame const&>(mesh.getFrame()));
        meshPhysicalOffsetFrame->setName(sphereName + "_offset");
        meshPhysicalOffsetFrame->set_translation(translationInMeshFrame);

        // attach the sphere to the frame
        OpenSim::FDSphere const* const spherePtr = [&sphereName, &meshPhysicalOffsetFrame]()
        {
            auto sphere = std::make_unique<OpenSim::FDSphere>();
            sphere->setName(sphereName);
            sphere->connectSocket_frame(*meshPhysicalOffsetFrame);
            OpenSim::FDSphere const* ptr = sphere.get();
            meshPhysicalOffsetFrame->addComponent(sphere.release());
            return ptr;
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
            mutableModel.addComponent(meshPhysicalOffsetFrame.release());
            mutableModel.finalizeConnections();
            osc::InitializeModel(mutableModel);
            osc::InitializeState(mutableModel);

            model.setSelected(spherePtr);
            model.commit(commitMessage);
        }
    }

    void ActionAddPointToPointEdge(
        osc::UndoableModelStatePair& model,
        OpenSim::FDVirtualPoint const& pointA,
        OpenSim::FDVirtualPoint const& pointB)
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
        OpenSim::FDVirtualPoint const& pointA,
        OpenSim::FDVirtualPoint const& pointB)
    {
        // generate name
        std::string const midpointName = []()
        {
            std::stringstream ss;
            ss << "midpoint_" << GetNextGlobalGeometrySuffix();
            return std::move(ss).str();
        }();

        // construct midpoint
        auto midpoint = std::make_unique<OpenSim::FDMidpoint>();
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
            OpenSim::FDMidpoint const* midpointPtr = midpoint.get();

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
        OpenSim::FDVirtualPoint const& point,
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

            OpenSim::FDVirtualPoint const* pointA = osc::FindComponent<OpenSim::FDVirtualPoint>(model->getModel(), pointAPath);
            if (!pointA)
            {
                osc::log::error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            OpenSim::FDVirtualPoint const* pointB = osc::FindComponent<OpenSim::FDVirtualPoint>(model->getModel(), pointBPath);
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
        OpenSim::FDVirtualPoint const& point,
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

            OpenSim::FDVirtualPoint const* pointA = osc::FindComponent<OpenSim::FDVirtualPoint>(model->getModel(), pointAPath);
            if (!pointA)
            {
                osc::log::error("point A's component path (%s) does not exist in the model", pointAPath.c_str());
                return false;
            }

            OpenSim::FDVirtualPoint const* pointB = osc::FindComponent<OpenSim::FDVirtualPoint>(model->getModel(), pointBPath);
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
        OpenSim::FDVirtualPoint const& point)
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
            else if (OpenSim::FDVirtualPoint const* maybePoint = dynamic_cast<OpenSim::FDVirtualPoint const*>(maybeComponent))
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
