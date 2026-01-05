#pragma once

#include <libopensimcreator/Documents/FileFilters.h>
#include <libopensimcreator/Documents/MeshImporter/Body.h>
#include <libopensimcreator/Documents/MeshImporter/CrossrefDirection.h>
#include <libopensimcreator/Documents/MeshImporter/Document.h>
#include <libopensimcreator/Documents/MeshImporter/DocumentHelpers.h>
#include <libopensimcreator/Documents/MeshImporter/Ground.h>
#include <libopensimcreator/Documents/MeshImporter/Joint.h>
#include <libopensimcreator/Documents/MeshImporter/Mesh.h>
#include <libopensimcreator/Documents/MeshImporter/MIObject.h>
#include <libopensimcreator/Documents/MeshImporter/OpenSimBridge.h>
#include <libopensimcreator/Documents/MeshImporter/Station.h>
#include <libopensimcreator/Documents/MeshImporter/UndoableDocument.h>
#include <libopensimcreator/Graphics/SimTKMeshLoader.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/MeshImporter/DrawableThing.h>
#include <libopensimcreator/UI/MeshImporter/MeshImporterHover.h>
#include <libopensimcreator/UI/MeshImporter/MeshLoader.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Geometries/ConeGeometry.h>
#include <liboscar/Graphics/Geometries/CylinderGeometry.h>
#include <liboscar/Graphics/Geometries/SphereGeometry.h>
#include <liboscar/Graphics/Materials/MeshBasicMaterial.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneDecorationFlags.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Graphics/Scene/SceneRenderer.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/CollisionTests.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/QuaternionFunctions.h>
#include <liboscar/Maths/Ray.h>
#include <liboscar/Maths/RayCollision.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Sphere.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/PerfPanel.h>
#include <liboscar/UI/Tabs/TabSaveResult.h>
#include <liboscar/UI/Widgets/LogViewer.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/UID.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { class Event; }

using namespace osc::literals;

namespace osc::mi
{
    // shared data support
    //
    // data that's shared between multiple UI states.
    class MeshImporterSharedState final : public std::enable_shared_from_this<MeshImporterSharedState> {
    public:
        MeshImporterSharedState(Widget* parent) :
            MeshImporterSharedState{parent, std::vector<std::filesystem::path>{}}
        {}

        explicit MeshImporterSharedState(Widget* parent, std::vector<std::filesystem::path> meshFiles) :
            m_Logviewer{parent},
            m_PerfPanel{parent}
        {
            m_FloorMaterial.set_transparent(true);
            pushMeshLoadRequests(std::move(meshFiles));
        }

        //
        // OpenSim OUTPUT MODEL STUFF
        //

        bool hasOutputModel() const
        {
            return m_MaybeOutputModel != nullptr;
        }

        std::unique_ptr<OpenSim::Model>& updOutputModel()
        {
            return m_MaybeOutputModel;
        }

        void tryCreateOutputModel()
        {
            try
            {
                m_MaybeOutputModel = CreateOpenSimModelFromMeshImporterDocument(getModelGraph(), m_ModelCreationFlags, m_IssuesBuffer);
            }
            catch (const std::exception& ex)
            {
                log_error("error occurred while trying to create an OpenSim model from the mesh editor scene: %s", ex.what());
            }
        }

        //
        // MODEL GRAPH STUFF
        //

        void openOsimFileAsModelGraph()
        {
            if (not shared_from_this()) {
                log_critical("cannot open import dialog because the mesh importer's state isn't reference-counted");
                return;
            }

            App::upd().prompt_user_to_select_file_async(
                [state = shared_from_this()](FileDialogResponse response)
                {
                    if (not state) {
                        return;  // Something went horribly wrong (should've been checked earlier).
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    // Wrap model import in a `try..catch` because the user may provide malformed
                    // data and we don't want the exception to propagate all the way up to the
                    // event loop (#1050).
                    try {
                        state->m_ModelGraphSnapshots = UndoableDocument{CreateModelFromOsimFile(response.front())};
                        state->m_MaybeModelGraphExportLocation = response.front();
                        state->m_MaybeModelGraphExportedUID = state->m_ModelGraphSnapshots.head_id();
                    }
                    catch (const std::exception& ex) {
                        log_error("Error importing %s as a model: %s", response.front().c_str(), ex.what());
                        return;  // Error importing the model
                    }
                },
                GetModelFileFilters()
            );
        }

        std::future<TabSaveResult> exportAsModelGraphAsOsimFile()
        {
            auto promise = std::make_shared<std::promise<TabSaveResult>>();
            App::upd().prompt_user_to_save_file_with_extension_async([promise, ptr = shared_from_this()](std::optional<std::filesystem::path> p)
            {
                if (not p) {
                    promise->set_value(TabSaveResult::Cancelled);
                    return;  // user cancelled out of the prompt
                }
                try {
                    ptr->exportModelGraphTo(*p);
                    promise->set_value(TabSaveResult::Done);
                }
                catch (const std::exception&) {
                    promise->set_value(TabSaveResult::Cancelled);
                }
            }, "osim");
            return promise->get_future();
        }

        std::future<TabSaveResult> exportModelGraphAsOsimFile()
        {
            if (m_MaybeModelGraphExportLocation.empty()) {
                return exportAsModelGraphAsOsimFile();
            }
            else {
                std::promise<TabSaveResult> promise;
                promise.set_value(exportModelGraphTo(m_MaybeModelGraphExportLocation) ? TabSaveResult::Done : TabSaveResult::Cancelled);
                return promise.get_future();
            }
        }

        bool isModelGraphUpToDateWithDisk() const
        {
            return m_MaybeModelGraphExportedUID == m_ModelGraphSnapshots.head_id();
        }

        bool isCloseRequested() const
        {
            return m_CloseRequested;
        }

        void request_close()
        {
            m_CloseRequested = true;
        }

        void resetRequestClose()
        {
            m_CloseRequested = false;
        }

        bool isNewMeshImpoterTabRequested() const
        {
            return m_NewTabRequested;
        }

        void requestNewMeshImporterTab()
        {
            m_NewTabRequested = true;
        }

        void resetRequestNewMeshImporter()
        {
            m_NewTabRequested = false;
        }

        std::string getRecommendedTitle() const
        {
            std::stringstream ss;
            ss << MSMICONS_CUBE << ' ' << getDocumentName();
            return std::move(ss).str();
        }

        const Document& getModelGraph() const
        {
            return m_ModelGraphSnapshots.scratch();
        }

        Document& updModelGraph()
        {
            return m_ModelGraphSnapshots.upd_scratch();
        }

        UndoableDocument& updCommittableModelGraph()
        {
            return m_ModelGraphSnapshots;
        }

        void commitCurrentModelGraph(std::string_view commitMsg)
        {
            m_ModelGraphSnapshots.commit_scratch(commitMsg);
        }

        bool canUndoCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.can_undo();
        }

        void undoCurrentModelGraph()
        {
            m_ModelGraphSnapshots.undo();
        }

        bool canRedoCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.can_redo();
        }

        void redoCurrentModelGraph()
        {
            m_ModelGraphSnapshots.redo();
        }

        const std::unordered_set<UID>& getCurrentSelection() const
        {
            return getModelGraph().getSelected();
        }

        void selectAll()
        {
            updModelGraph().selectAll();
        }

        void deSelectAll()
        {
            updModelGraph().deSelectAll();
        }

        bool hasSelection() const
        {
            return getModelGraph().hasSelection();
        }

        bool isSelected(UID id) const
        {
            return getModelGraph().isSelected(id);
        }

        //
        // MESH LOADING STUFF
        //

        void pushMeshLoadRequests(std::vector<std::filesystem::path> paths, UID attachmentPoint = MIIDs::Ground())
        {
            m_MeshLoader.send(MeshLoadRequest{attachmentPoint, std::move(paths)});
        }

        void promptUserForMeshFilesAndPushThemOntoMeshLoader(UID attachmentPoint = MIIDs::Ground())
        {
            if (not shared_from_this()) {
                log_critical("cannot open mesh import dialog because the mesh importer's state isn't reference-counted");
                return;
            }
            App::upd().prompt_user_to_select_file_async(
                [state = shared_from_this(), attachmentPoint](FileDialogResponse response)
                {
                    if (not state) {
                        return;  // Something went wrong
                    }
                    std::vector<std::filesystem::path> paths(response.begin(), response.end());
                    state->pushMeshLoadRequests(paths, attachmentPoint);
                },
                GetSupportedSimTKMeshFormatsAsFilters(),
                std::nullopt,
                true
            );
        }

        void reloadMeshes()
        {
            for (auto& mesh : m_ModelGraphSnapshots.upd_scratch().iter<mi::Mesh>()) {
                try {
                    mesh.reloadMeshDataFromDisk();
                }
                catch (const std::exception& ex) {
                    log_info("%s: error reloading: %s", mesh.getLabel().c_str(), ex.what());
                }
            }
        }

        //
        // UI OVERLAY STUFF
        //

        Vector2 worldPosToScreenPos(const Vector3& worldPos) const
        {
            return getCamera().project_onto_viewport(worldPos, get3DSceneRect());
        }

        void drawConnectionLine(
            const Color& color,
            const Vector3& parent,
            const Vector3& child) const
        {
            // the line
            ui::get_panel_draw_list().add_line(worldPosToScreenPos(parent), worldPosToScreenPos(child), color, c_ConnectionLineWidth);

            // the triangle
            drawConnectionLineTriangleAtMidpoint(color, parent, child);
        }

        void drawConnectionLines(
            const Color& color,
            const std::unordered_set<UID>& excludedIDs) const
        {
            const Document& mg = getModelGraph();

            for (const MIObject& el : mg.iter())
            {
                UID id = el.getID();

                if (excludedIDs.contains(id))
                {
                    continue;
                }

                if (!shouldShowConnectionLines(el))
                {
                    continue;
                }

                if (el.getNumCrossReferences() > 0)
                {
                    drawConnectionLines(el, color, excludedIDs);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    drawConnectionLineToGround(el, color);
                }
            }
        }

        void drawConnectionLines(const Color& color) const
        {
            drawConnectionLines(color, {});
        }

        void drawConnectionLines(const MeshImporterHover& currentHover) const
        {
            const Document& mg = getModelGraph();

            for (const MIObject& el : mg.iter())
            {
                UID id = el.getID();

                if (id != currentHover.ID && !el.isCrossReferencing(currentHover.ID))
                {
                    continue;
                }

                if (!shouldShowConnectionLines(el))
                {
                    continue;
                }

                if (el.getNumCrossReferences() > 0)
                {
                    drawConnectionLines(el, m_Colors.connectionLines);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    drawConnectionLineToGround(el, m_Colors.connectionLines);
                }
            }
            //drawConnectionLines(m_Colors.connectionLines);
        }

        //
        // RENDERING STUFF
        //

        void setContentRegionAvailAsSceneRect()
        {
            set3DSceneRect(ui::get_content_region_available_ui_rect());
        }

        void drawScene(std::span<const DrawableThing> drawables)
        {
            const App& app = App::get();

            // setup rendering params
            SceneRendererParams p;
            p.dimensions = get3DSceneDims();
            p.device_pixel_ratio = app.settings().get_value<float>("graphics/render_scale", 1.0f) * app.main_window_device_pixel_ratio();
            p.anti_aliasing_level = app.anti_aliasing_level();
            p.draw_rims = true;
            p.draw_floor = false;
            p.near_clipping_plane = m_3DSceneCamera.znear;
            p.far_clipping_plane = m_3DSceneCamera.zfar;
            p.view_matrix = m_3DSceneCamera.view_matrix();
            p.projection_matrix = m_3DSceneCamera.projection_matrix(aspect_ratio_of(p.dimensions));
            p.viewer_position = m_3DSceneCamera.position();
            p.light_direction = recommended_light_direction(m_3DSceneCamera);
            p.light_color = Color::white();
            p.ambient_strength *= 1.5f;
            p.background_color = getColorSceneBackground();

            std::vector<SceneDecoration> decs;
            decs.reserve(drawables.size());
            for (const DrawableThing& dt : drawables)
            {
                decs.push_back(SceneDecoration{
                    .mesh = dt.mesh,
                    .transform = dt.transform,
                    .shading = dt.shading,
                    .flags = dt.flags,
                });
            }

            // render
            scene_renderer_.render(decs, p);

            // draw texture in ui
            ui::draw_image(scene_renderer_.upd_render_texture());

            // handle hittesting, etc.
            setIsRenderHovered(ui::is_item_hovered(ui::HoveredFlag::AllowWhenBlockedByPopup));
        }

        bool isRenderHovered() const
        {
            return m_IsRenderHovered;
        }

        const Rect& get3DSceneRect() const
        {
            return m_3DSceneRect;
        }

        Vector2 get3DSceneDims() const
        {
            return m_3DSceneRect.dimensions();
        }

        const PolarPerspectiveCamera& getCamera() const
        {
            return m_3DSceneCamera;
        }

        PolarPerspectiveCamera& updCamera()
        {
            return m_3DSceneCamera;
        }

        void resetCamera()
        {
            m_3DSceneCamera = CreateDefaultCamera();
        }

        void focusCameraOn(const Vector3& focusPoint)
        {
            m_3DSceneCamera.focus_point = -focusPoint;
        }

        std::span<const Color> colors() const
        {
            static_assert(offsetof(Colors, ground) == 0);
            static_assert(sizeof(Colors) % sizeof(Color) == 0);
            return {&m_Colors.ground, sizeof(m_Colors)/sizeof(Color)};
        }

        void setColor(size_t i, const Color& newColorValue)
        {
            updColors()[i] = newColorValue;
        }

        std::span<const char* const> getColorLabels() const
        {
            return c_ColorNames;
        }

        const Color& getColorConnectionLine() const
        {
            return m_Colors.connectionLines;
        }

        std::span<const bool> getVisibilityFlags() const
        {
            static_assert(offsetof(VisibilityFlags, ground) == 0);
            static_assert(sizeof(VisibilityFlags) % sizeof(bool) == 0);
            return {&m_VisibilityFlags.ground, sizeof(m_VisibilityFlags)/sizeof(bool)};
        }

        void setVisibilityFlag(size_t i, bool newVisibilityValue)
        {
            updVisibilityFlags()[i] = newVisibilityValue;
        }

        std::span<const char* const> getVisibilityFlagLabels() const
        {
            return c_VisibilityFlagNames;
        }

        bool isShowingFloor() const
        {
            return m_VisibilityFlags.floor;
        }

        DrawableThing generateFloorDrawable() const
        {
            auto props = MeshBasicMaterial::PropertyBlock{};
            props.set_color(m_Colors.gridLines);

            return DrawableThing{
                .id = MIIDs::Empty(),
                .groupId = MIIDs::Empty(),
                .mesh = App::singleton<SceneCache>(App::resource_loader())->grid_mesh(),
                .transform = {
                    .scale = 0.5f * Vector3{m_SceneScaleFactor * 100.0f, m_SceneScaleFactor * 100.0f, 1.0f},
                    .rotation = angle_axis(90_deg, Vector3{-1.0f, 0.0f, 0.0f}),
                },
                .shading = std::pair<Material, MaterialPropertyBlock>{m_FloorMaterial, props},
                .flags = SceneDecorationFlag::AnnotationElement,
            };
        }

        //
        // HOVERTEST/INTERACTIVITY
        //

        std::span<const bool> getIneractivityFlags() const
        {
            static_assert(offsetof(InteractivityFlags, ground) == 0);
            static_assert(sizeof(InteractivityFlags) % sizeof(bool) == 0);
            return {&m_InteractivityFlags.ground, sizeof(m_InteractivityFlags)/sizeof(bool)};
        }

        void setInteractivityFlag(size_t i, bool newInteractivityValue)
        {
            updInteractivityFlags()[i] = newInteractivityValue;
        }

        std::span<const char* const> getInteractivityFlagLabels() const
        {
            return c_InteractivityFlagNames;
        }

        float getSceneScaleFactor() const
        {
            return m_SceneScaleFactor;
        }

        void setSceneScaleFactor(float newScaleFactor)
        {
            m_SceneScaleFactor = newScaleFactor;
        }

        MeshImporterHover doHovertest(const std::vector<DrawableThing>& drawables) const
        {
            auto cache = App::singleton<SceneCache>(App::resource_loader());

            const Rect sceneRect = get3DSceneRect();
            const Vector2 mouseUiPosition = ui::get_mouse_ui_position();

            if (!is_intersecting(sceneRect, mouseUiPosition))
            {
                // mouse isn't over the scene render
                return MeshImporterHover{};
            }

            const Vector2 sceneDims = sceneRect.dimensions();
            const Vector2 relMousePos = mouseUiPosition - sceneRect.ypd_top_left();

            const Ray ray = getCamera().unproject_topleft_position_to_world_ray(relMousePos, sceneDims);
            const bool hittestMeshes = isMeshesInteractable();
            const bool hittestBodies = isBodiesInteractable();
            const bool hittestJointCenters = isJointCentersInteractable();
            const bool hittestGround = isGroundInteractable();
            const bool hittestStations = isStationsInteractable();

            UID closestID = MIIDs::Empty();
            float closestDist = std::numeric_limits<float>::max();
            for (const DrawableThing& drawable : drawables)
            {
                if (drawable.id == MIIDs::Empty())
                {
                    continue;  // no hittest data
                }

                if (drawable.groupId == MIIDs::BodyGroup() && !hittestBodies)
                {
                    continue;
                }

                if (drawable.groupId == MIIDs::MeshGroup() && !hittestMeshes)
                {
                    continue;
                }

                if (drawable.groupId == MIIDs::JointGroup() && !hittestJointCenters)
                {
                    continue;
                }

                if (drawable.groupId == MIIDs::GroundGroup() && !hittestGround)
                {
                    continue;
                }

                if (drawable.groupId == MIIDs::StationGroup() && !hittestStations)
                {
                    continue;
                }

                const std::optional<RayCollision> rc = get_closest_world_space_ray_triangle_collision(
                    drawable.mesh,
                    cache->get_bvh(drawable.mesh),
                    drawable.transform,
                    ray
                );

                if (rc && rc->distance < closestDist)
                {
                    closestID = drawable.id;
                    closestDist = rc->distance;
                }
            }

            const Vector3 hitPos = closestID != MIIDs::Empty() ? ray.origin + closestDist*ray.direction : Vector3{};

            return MeshImporterHover{closestID, hitPos};
        }

        //
        // MODEL CREATION FLAGS
        //

        ModelCreationFlags getModelCreationFlags() const
        {
            return m_ModelCreationFlags;
        }

        void setModelCreationFlags(ModelCreationFlags newFlags)
        {
            m_ModelCreationFlags = newFlags;
        }

        //
        // SCENE ELEMENT STUFF (specific methods for specific scene element types)
        //

        DrawableThing generateMeshDrawable(const osc::mi::Mesh& el) const
        {
            return DrawableThing{
                .id = el.getID(),
                .groupId = MIIDs::MeshGroup(),
                .mesh = el.getMeshData(),
                .transform = el.getXForm(),
                .shading = el.getParentID() == MIIDs::Ground() || el.getParentID() == MIIDs::Empty() ? RedifyColor(getColorMesh()) : getColorMesh(),
            };
        }

        void appendDrawables(
            const MIObject& e,
            std::vector<DrawableThing>& appendOut) const
        {
            std::visit(Overload
            {
                [this, &appendOut](const Ground&)
                {
                    if (!isShowingGround())
                    {
                        return;
                    }

                    appendOut.push_back(generateGroundSphere(getColorGround()));
                },
                [this, &appendOut](const Mesh& el)
                {
                    if (!isShowingMeshes())
                    {
                        return;
                    }

                    appendOut.push_back(generateMeshDrawable(el));
                },
                [this, &appendOut](const Body& el)
                {
                    if (!isShowingBodies())
                    {
                        return;
                    }

                    appendBodyElAsCubeThing(el, appendOut);
                },
                [this, &appendOut](const Joint& el)
                {
                    if (!isShowingJointCenters())
                    {
                        return;
                    }

                    appendAsFrame(
                        el.getID(),
                        MIIDs::JointGroup(),
                        el.getXForm(),
                        appendOut,
                        1.0f,
                        SceneDecorationFlag::Default,
                        GetJointAxisLengths(el)
                    );
                },
                [this, &appendOut](const StationEl& el)
                {
                    if (!isShowingStations())
                    {
                        return;
                    }

                    appendOut.push_back(generateStationSphere(el, getColorStation()));
                },
            }, e.toVariant());
        }

        //
        // WINDOWS
        //
        enum class PanelIndex {
            History = 0,
            Navigator,
            Log,
            Performance,
            NUM_OPTIONS,
        };

        size_t num_toggleable_panels() const
        {
            return num_options<PanelIndex>();
        }

        CStringView getNthPanelName(size_t n) const
        {
            return c_OpenedPanelNames[n];
        }

        bool isNthPanelEnabled(size_t n) const
        {
            return m_PanelStates[n];
        }

        void setNthPanelEnabled(size_t n, bool v)
        {
            m_PanelStates[n] = v;
        }

        bool isPanelEnabled(PanelIndex idx) const
        {
            return m_PanelStates[to_index(idx)];
        }

        void setPanelEnabled(PanelIndex idx, bool v)
        {
            m_PanelStates[to_index(idx)] = v;
        }

        LogViewer& updLogViewer()
        {
            return m_Logviewer;
        }

        PerfPanel& updPerfPanel()
        {
            return m_PerfPanel;
        }


        //
        // TOP-LEVEL STUFF
        //

        bool onEvent(Event&);

        void tick(float)
        {
            // push any user-drag-dropped files as one batch
            if (!m_DroppedFiles.empty())
            {
                std::vector<std::filesystem::path> buf;
                std::swap(buf, m_DroppedFiles);
                pushMeshLoadRequests(std::move(buf));
            }

            // pop any background-loaded meshes
            popMeshLoader();

            m_ModelGraphSnapshots.upd_scratch().garbageCollect();
        }

    private:
        bool exportModelGraphTo(const std::filesystem::path& exportPath)
        {
            std::vector<std::string> issues;
            std::unique_ptr<OpenSim::Model> m;

            try {
                m = CreateOpenSimModelFromMeshImporterDocument(getModelGraph(), m_ModelCreationFlags, issues);
            }
            catch (const std::exception& ex) {
                log_error("error occurred while trying to create an OpenSim model from the mesh editor scene: %s", ex.what());
            }

            if (m) {
                m->print(exportPath.string());
                m_MaybeModelGraphExportLocation = exportPath;
                m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.head_id();
                return true;
            }
            else {
                for (const std::string& issue : issues) {
                    log_error("%s", issue.c_str());
                }
                return false;
            }
        }

        std::string getDocumentName() const
        {
            if (m_MaybeModelGraphExportLocation.empty())
            {
                return "untitled.osim";
            }
            else
            {
                return m_MaybeModelGraphExportLocation.filename().string();
            }
        }

        // called when the mesh loader responds with a fully-loaded mesh
        void popMeshLoaderHandleOKResponse(MeshLoadOKResponse& ok)
        {
            if (ok.meshes.empty())
            {
                return;
            }

            // add each loaded mesh into the model graph
            Document& mg = updModelGraph();
            mg.deSelectAll();

            for (const LoadedMesh& lm : ok.meshes)
            {
                MIObject* el = mg.tryUpdByID(ok.preferredAttachmentPoint);

                if (el)
                {
                    auto& mesh = mg.emplace<Mesh>(UID{}, ok.preferredAttachmentPoint, lm.meshData, lm.path);
                    mesh.setXform(el->getXForm(mg));
                    mg.select(mesh);
                    mg.select(*el);
                }
            }

            // commit
            {
                std::stringstream commitMsgSS;
                if (ok.meshes.empty())
                {
                    commitMsgSS << "loaded 0 meshes";
                }
                else if (ok.meshes.size() == 1)
                {
                    commitMsgSS << "loaded " << ok.meshes.front().path.filename();
                }
                else
                {
                    commitMsgSS << "loaded " << ok.meshes.size() << " meshes";
                }

                commitCurrentModelGraph(std::move(commitMsgSS).str());
            }
        }

        // called when the mesh loader responds with a mesh loading error
        void popMeshLoaderHandleErrorResponse(MeshLoadErrorResponse& err)
        {
            log_error("%s: error loading mesh file: %s", err.path.string().c_str(), err.error.c_str());
        }

        void popMeshLoader()
        {
            for (auto maybeResponse = m_MeshLoader.poll(); maybeResponse.has_value(); maybeResponse = m_MeshLoader.poll())
            {
                MeshLoadResponse& meshLoaderResp = *maybeResponse;

                if (std::holds_alternative<MeshLoadOKResponse>(meshLoaderResp))
                {
                    popMeshLoaderHandleOKResponse(std::get<MeshLoadOKResponse>(meshLoaderResp));
                }
                else
                {
                    popMeshLoaderHandleErrorResponse(std::get<MeshLoadErrorResponse>(meshLoaderResp));
                }
            }
        }

        void drawConnectionLineTriangleAtMidpoint(
            const Color& color,
            const Vector3& parent,
            const Vector3& child) const
        {
            constexpr float triangleWidth = 6.0f * c_ConnectionLineWidth;
            constexpr float triangleWidthSquared = triangleWidth*triangleWidth;

            const Vector2 parentScr = worldPosToScreenPos(parent);
            const Vector2 childScr = worldPosToScreenPos(child);
            const Vector2 child2ParentScr = parentScr - childScr;

            if (dot(child2ParentScr, child2ParentScr) < triangleWidthSquared) {
                return;
            }

            const Vector3 mp = midpoint(parent, child);
            const Vector2 midpointScr = worldPosToScreenPos(mp);
            const Vector2 directionScr = normalize(child2ParentScr);
            const Vector2 directionNormalScr = {-directionScr.y, directionScr.x};

            const Vector2 p1 = midpointScr + (triangleWidth/2.0f)*directionNormalScr;
            const Vector2 p2 = midpointScr - (triangleWidth/2.0f)*directionNormalScr;
            const Vector2 p3 = midpointScr + triangleWidth*directionScr;

            ui::get_panel_draw_list().add_triangle_filled(p1, p2, p3, color);
        }

        void drawConnectionLines(
            const MIObject& el,
            const Color& color,
            const std::unordered_set<UID>& excludedIDs) const
        {
            const Document& mg = getModelGraph();
            for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
            {
                UID refID = el.getCrossReferenceConnecteeID(i);

                if (excludedIDs.contains(refID))
                {
                    continue;
                }

                const MIObject* other = mg.tryGetByID(refID);

                if (!other)
                {
                    continue;
                }

                Vector3 child = el.getPos(mg);
                Vector3 parent = other->getPos(mg);

                if (el.getCrossReferenceDirection(i) == CrossrefDirection::ToChild)
                {
                    std::swap(parent, child);
                }

                drawConnectionLine(color, parent, child);
            }
        }

        void drawConnectionLines(const MIObject& el, const Color& color) const
        {
            drawConnectionLines(el, color, std::unordered_set<UID>{});
        }

        void drawConnectionLineToGround(const MIObject& el, const Color& color) const
        {
            if (el.getID() == MIIDs::Ground())
            {
                return;
            }

            drawConnectionLine(color, Vector3{}, el.getPos(getModelGraph()));
        }

        bool shouldShowConnectionLines(const MIObject& el) const
        {
            return std::visit(Overload
            {
                []    (const Ground&)  { return false; },
                [this](const Mesh&)    { return this->isShowingMeshConnectionLines(); },
                [this](const Body&)    { return this->isShowingBodyConnectionLines(); },
                [this](const Joint&)   { return this->isShowingJointConnectionLines(); },
                [this](const StationEl&) { return this->isShowingMeshConnectionLines(); },
            }, el.toVariant());
        }

        void setIsRenderHovered(bool newIsHovered)
        {
            m_IsRenderHovered = newIsHovered;
        }

        void set3DSceneRect(const Rect& newRect)
        {
            m_3DSceneRect = newRect;
        }

        std::span<Color> updColors()
        {
            static_assert(offsetof(Colors, ground) == 0);
            static_assert(sizeof(Colors) % sizeof(Color) == 0);
            return {&m_Colors.ground, sizeof(m_Colors)/sizeof(Color)};
        }

        const Color& getColorSceneBackground() const
        {
            return m_Colors.sceneBackground;
        }

        const Color& getColorGround() const
        {
            return m_Colors.ground;
        }

        const Color& getColorMesh() const
        {
            return m_Colors.meshes;
        }

        const Color& getColorStation() const
        {
            return m_Colors.stations;
        }

        std::span<bool> updVisibilityFlags()
        {
            static_assert(offsetof(VisibilityFlags, ground) == 0);
            static_assert(sizeof(VisibilityFlags) % sizeof(bool) == 0);
            return {&m_VisibilityFlags.ground, sizeof(m_VisibilityFlags)/sizeof(bool)};
        }

        bool isShowingMeshes() const
        {
            return m_VisibilityFlags.meshes;
        }

        void setIsShowingMeshes(bool newIsShowing)
        {
            m_VisibilityFlags.meshes = newIsShowing;
        }

        bool isShowingBodies() const
        {
            return m_VisibilityFlags.bodies;
        }

        void setIsShowingBodies(bool newIsShowing)
        {
            m_VisibilityFlags.bodies = newIsShowing;
        }

        bool isShowingJointCenters() const
        {
            return m_VisibilityFlags.joints;
        }

        void setIsShowingJointCenters(bool newIsShowing)
        {
            m_VisibilityFlags.joints = newIsShowing;
        }

        bool isShowingGround() const
        {
            return m_VisibilityFlags.ground;
        }

        void setIsShowingGround(bool newIsShowing)
        {
            m_VisibilityFlags.ground = newIsShowing;
        }

        void setIsShowingFloor(bool newIsShowing)
        {
            m_VisibilityFlags.floor = newIsShowing;
        }

        bool isShowingStations() const
        {
            return m_VisibilityFlags.stations;
        }

        void setIsShowingStations(bool v)
        {
            m_VisibilityFlags.stations = v;
        }

        bool isShowingJointConnectionLines() const
        {
            return m_VisibilityFlags.jointConnectionLines;
        }

        void setIsShowingJointConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.jointConnectionLines = newIsShowing;
        }

        bool isShowingMeshConnectionLines() const
        {
            return m_VisibilityFlags.meshConnectionLines;
        }

        void setIsShowingMeshConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.meshConnectionLines = newIsShowing;
        }

        bool isShowingBodyConnectionLines() const
        {
            return m_VisibilityFlags.bodyToGroundConnectionLines;
        }

        void setIsShowingBodyConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.bodyToGroundConnectionLines = newIsShowing;
        }

        bool isShowingStationConnectionLines() const
        {
            return m_VisibilityFlags.stationConnectionLines;
        }

        void setIsShowingStationConnectionLines(bool newIsShowing)
        {
            m_VisibilityFlags.stationConnectionLines = newIsShowing;
        }

        float getSphereRadius() const
        {
            return 0.02f * m_SceneScaleFactor;
        }

        Sphere sphereAtTranslation(const Vector3& translation) const
        {
            return Sphere{translation, getSphereRadius()};
        }

        void appendAsFrame(
            UID logicalID,
            UID groupID,
            const Transform& xform,
            std::vector<DrawableThing>& appendOut,
            float alpha = 1.0f,
            SceneDecorationFlags flags = SceneDecorationFlag::Default,
            Vector3 legLen = {1.0f, 1.0f, 1.0f},
            Color coreColor = Color::white()) const
        {
            const float coreRadius = getSphereRadius();
            const float legThickness = 0.5f * coreRadius;

            // this is how much the cylinder has to be "pulled in" to the core to hide the edges
            const float cylinderPullback = coreRadius * sin((180_deg * legThickness) / coreRadius);

            // emit origin sphere
            appendOut.push_back(DrawableThing{
                .id = logicalID,
                .groupId = groupID,
                .mesh = m_SphereMesh,
                .transform = {
                    .scale = Vector3{coreRadius},
                    .rotation = xform.rotation,
                    .translation = xform.translation,
                },
                .shading = coreColor.with_alpha(coreColor.a * alpha),
                .flags = flags,
            });

            // emit "legs"
            for (int i = 0; i < 3; ++i)
            {
                // cylinder meshes are -1.0f to 1.0f in Y, so create a transform that maps the
                // mesh onto the legs, which are:
                //
                // - 4.0f * leglen[leg] * radius long
                // - 0.5f * radius thick

                const Vector3 meshDirection = {0.0f, 1.0f, 0.0f};
                Vector3 cylinderDirection = {};
                cylinderDirection[i] = 1.0f;

                const float actualLegLen = 4.0f * legLen[i] * coreRadius;

                const Quaternion rot = normalize(xform.rotation * rotation(meshDirection, cylinderDirection));
                appendOut.push_back(DrawableThing{
                    .id = logicalID,
                    .groupId = groupID,
                    .mesh = m_CylinderMesh,
                    .transform = {
                        .scale = {legThickness, 0.5f * actualLegLen, legThickness}, // note: cylinder is 2 units high
                        .rotation = rot,
                        .translation = xform.translation + (rot * (((getSphereRadius() + (0.5f * actualLegLen)) - cylinderPullback) * meshDirection)),
                    },
                    .shading = Color{0.0f, alpha}.with_element(i, 1.0f),
                    .flags = flags,
                });
            }
        }

        void appendAsCubeThing(
            UID logicalID,
            UID groupID,
            const Transform& xform,
            std::vector<DrawableThing>& appendOut) const
        {
            const float halfWidth = 1.5f * getSphereRadius();

            // core
            {
                Transform scaled{xform};
                scaled.scale *= halfWidth;

                appendOut.push_back(DrawableThing{
                    .id = logicalID,
                    .groupId = groupID,
                    .mesh = App::singleton<SceneCache>(App::resource_loader())->brick_mesh(),
                    .transform = scaled,
                    .shading = Color::white(),
                });
            }

            // legs
            for (int i = 0; i < 3; ++i) {
                // cone mesh has a source height of 2, stretches from -1 to +1 in Y
                const float coneHeight = 0.75f * halfWidth;

                const Vector3 meshDirection = {0.0f, 1.0f, 0.0f};
                const Vector3 coneDirection = Vector3{}.with_element(i, 1.0f);

                const Quaternion rot = xform.rotation * rotation(meshDirection, coneDirection);

                appendOut.push_back(DrawableThing{
                    .id = logicalID,
                    .groupId = groupID,
                    .mesh = App::singleton<SceneCache>(App::resource_loader())->cone_mesh(),
                    .transform = {
                        .scale = 0.5f * Vector3{halfWidth, coneHeight, halfWidth},
                        .rotation = rot,
                        .translation = xform.translation + (rot * ((halfWidth + (0.5f * coneHeight)) * meshDirection)),
                    },
                    .shading = Color::black().with_element(i, 1.0f),
                });

            }
        }

        std::span<bool> updInteractivityFlags()
        {
            static_assert(offsetof(InteractivityFlags, ground) == 0);
            static_assert(sizeof(InteractivityFlags) % sizeof(bool) == 0);
            return {&m_InteractivityFlags.ground, sizeof(m_InteractivityFlags)/sizeof(bool)};
        }

        bool isMeshesInteractable() const
        {
            return m_InteractivityFlags.meshes;
        }

        void setIsMeshesInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.meshes = newIsInteractable;
        }

        bool isBodiesInteractable() const
        {
            return m_InteractivityFlags.bodies;
        }

        void setIsBodiesInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.bodies = newIsInteractable;
        }

        bool isJointCentersInteractable() const
        {
            return m_InteractivityFlags.joints;
        }

        void setIsJointCentersInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.joints = newIsInteractable;
        }

        bool isGroundInteractable() const
        {
            return m_InteractivityFlags.ground;
        }

        void setIsGroundInteractable(bool newIsInteractable)
        {
            m_InteractivityFlags.ground = newIsInteractable;
        }

        bool isStationsInteractable() const
        {
            return m_InteractivityFlags.stations;
        }

        void setIsStationsInteractable(bool v)
        {
            m_InteractivityFlags.stations = v;
        }

        void appendBodyElAsCubeThing(const Body& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            appendAsCubeThing(bodyEl.getID(), MIIDs::BodyGroup(), bodyEl.getXForm(), appendOut);
        }

        DrawableThing generateBodyElSphere(const Body& bodyEl, const Color& color) const
        {
            return {
                .id = bodyEl.getID(),
                .groupId = MIIDs::BodyGroup(),
                .mesh = m_SphereMesh,
                .transform = SphereMeshToSceneSphereTransform(sphereAtTranslation(bodyEl.getXForm().translation)),
                .shading = color,
            };
        }

        DrawableThing generateGroundSphere(const Color& color) const
        {
            return {
                .id = MIIDs::Ground(),
                .groupId = MIIDs::GroundGroup(),
                .mesh = m_SphereMesh,
                .transform = SphereMeshToSceneSphereTransform(sphereAtTranslation({0.0f, 0.0f, 0.0f})),
                .shading = color,
            };
        }

        DrawableThing generateStationSphere(const StationEl& el, const Color& color) const
        {
            return {
                .id = el.getID(),
                .groupId = MIIDs::StationGroup(),
                .mesh = m_SphereMesh,
                .transform = SphereMeshToSceneSphereTransform(sphereAtTranslation(el.getPos(getModelGraph()))),
                .shading = color,
            };
        }

        Color RedifyColor(const Color& srcColor) const
        {
            constexpr float factor = 0.8f;
            return {srcColor.r, factor * srcColor.g, factor * srcColor.b, factor * srcColor.a};
        }

        // returns a transform that maps a sphere mesh (defined to be @ 0,0,0 with radius 1)
        // to some sphere in the scene (e.g. a body/ground)
        Transform SphereMeshToSceneSphereTransform(const Sphere& sceneSphere) const
        {
            return {
                .scale = Vector3{sceneSphere.radius},
                .translation = sceneSphere.origin,
            };
        }

        // returns a camera that is in the initial position the camera should be in for this screen
        PolarPerspectiveCamera CreateDefaultCamera() const
        {
            PolarPerspectiveCamera rv;
            rv.phi = 45_deg;
            rv.theta = 45_deg;
            rv.radius = 2.5f;
            return rv;
        }

        // in-memory model graph (snapshots) that the user is manipulating
        UndoableDocument m_ModelGraphSnapshots;

        // (maybe) the filesystem location where the model graph should be saved
        std::filesystem::path m_MaybeModelGraphExportLocation;

        // (maybe) the UID of the model graph when it was last successfully saved to disk (used for dirty checking)
        UID m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.head_id();

        // a batch of files that the user drag-dropped into the UI in the last frame
        std::vector<std::filesystem::path> m_DroppedFiles;

        // loads meshes in a background thread
        MeshLoader m_MeshLoader;

        // sphere mesh used by various scene elements
        osc::Mesh m_SphereMesh = SphereGeometry{{.num_width_segments = 12, .num_height_segments = 12}};

        // cylinder mesh used by various scene elements
        osc::Mesh m_CylinderMesh = CylinderGeometry{{.height = 2.0f, .num_radial_segments = 32}};

        // cone mesh used to render scene elements
        osc::Mesh m_ConeMesh = ConeGeometry{{.radius = 1.0f, .height = 2.0f, .num_radial_segments = 16}};

        // material used to draw the floor grid
        MeshBasicMaterial m_FloorMaterial;

        // main 3D scene camera
        PolarPerspectiveCamera m_3DSceneCamera = CreateDefaultCamera();

        // screen space rect where the 3D scene is currently being drawn to
        Rect m_3DSceneRect = {};

        // renderer that draws the scene
        SceneRenderer scene_renderer_{
            *App::singleton<SceneCache>(App::resource_loader()),
        };

        // COLORS
        //
        // these are runtime-editable color values for things in the scene
        struct Colors {
            Color ground{196.0f/255.0f, 196.0f/255.0f, 196.0f/255.0f, 1.0f};
            Color meshes{1.0f, 1.0f, 1.0f, 1.0f};
            Color stations{196.0f/255.0f, 0.0f, 0.0f, 1.0f};
            Color connectionLines{0.6f, 0.6f, 0.6f, 1.0f};
            Color sceneBackground{48.0f/255.0f, 48.0f/255.0f, 48.0f/255.0f, 1.0f};
            Color gridLines{0.7f, 0.7f, 0.7f, 0.15f};
        } m_Colors;
        static constexpr auto c_ColorNames = std::to_array<const char*>(
        {
            "ground",
            "meshes",
            "stations",
            "connection lines",
            "scene background",
            "grid lines",
        });
        static_assert(c_ColorNames.size() == sizeof(decltype(m_Colors))/sizeof(Color));

        // VISIBILITY
        //
        // these are runtime-editable visibility flags for things in the scene
        struct VisibilityFlags {
            bool ground = true;
            bool meshes = true;
            bool bodies = true;
            bool joints = true;
            bool stations = true;
            bool jointConnectionLines = true;
            bool meshConnectionLines = true;
            bool bodyToGroundConnectionLines = true;
            bool stationConnectionLines = true;
            bool floor = true;
        } m_VisibilityFlags;
        static constexpr auto c_VisibilityFlagNames = std::to_array<const char*>(
        {
            "ground",
            "meshes",
            "bodies",
            "joints",
            "stations",
            "joint connection lines",
            "mesh connection lines",
            "body-to-ground connection lines",
            "station connection lines",
            "grid lines",
        });
        static_assert(c_VisibilityFlagNames.size() == sizeof(decltype(m_VisibilityFlags))/sizeof(bool));

        // LOCKING
        //
        // these are runtime-editable flags that dictate what gets hit-tested
        struct InteractivityFlags {
            bool ground = true;
            bool meshes = true;
            bool bodies = true;
            bool joints = true;
            bool stations = true;
        } m_InteractivityFlags;
        static constexpr auto c_InteractivityFlagNames = std::to_array<const char*>(
        {
            "ground",
            "meshes",
            "bodies",
            "joints",
            "stations",
        });
        static_assert(c_InteractivityFlagNames.size() == sizeof(decltype(m_InteractivityFlags))/sizeof(bool));

        // WINDOWS
        //
        // these are runtime-editable flags that dictate which panels are open
        static inline constexpr size_t c_NumPanelStates = 4;
        std::array<bool, c_NumPanelStates> m_PanelStates{false, true, false, false};
        static constexpr auto c_OpenedPanelNames = std::to_array<const char*>(
        {
            "History",
            "Navigator",
            "Log",
            "Performance",
        });
        static_assert(c_OpenedPanelNames.size() == c_NumPanelStates);
        static_assert(num_options<PanelIndex>() == c_NumPanelStates);
        LogViewer m_Logviewer;
        PerfPanel m_PerfPanel;

        // scale factor for all non-mesh, non-overlay scene elements (e.g.
        // the floor, bodies)
        //
        // this is necessary because some meshes can be extremely small/large and
        // scene elements need to be scaled accordingly (e.g. without this, a body
        // sphere end up being much larger than a mesh instance). Imagine if the
        // mesh was the leg of a fly
        float m_SceneScaleFactor = 1.0f;

        // buffer containing issues found in the modelgraph
        std::vector<std::string> m_IssuesBuffer;

        // model created by this wizard
        //
        // `nullptr` until the model is successfully created
        std::unique_ptr<OpenSim::Model> m_MaybeOutputModel = nullptr;

        // set to true after drawing the ui::Image
        bool m_IsRenderHovered = false;

        // true if the implementation wants the host to close the mesh importer UI
        bool m_CloseRequested = false;

        // true if the implementation wants the host to open a new mesh importer
        bool m_NewTabRequested = false;

        // changes how a model is created
        ModelCreationFlags m_ModelCreationFlags = ModelCreationFlags::None;

        static constexpr float c_ConnectionLineWidth = 1.0f;
    };
}
