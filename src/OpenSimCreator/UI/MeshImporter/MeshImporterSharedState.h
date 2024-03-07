#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Body.h>
#include <OpenSimCreator/Documents/MeshImporter/CrossrefDirection.h>
#include <OpenSimCreator/Documents/MeshImporter/Document.h>
#include <OpenSimCreator/Documents/MeshImporter/DocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshImporter/Ground.h>
#include <OpenSimCreator/Documents/MeshImporter/Joint.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.h>
#include <OpenSimCreator/Documents/MeshImporter/OpenSimBridge.h>
#include <OpenSimCreator/Documents/MeshImporter/Station.h>
#include <OpenSimCreator/Documents/MeshImporter/UndoableDocument.h>
#include <OpenSimCreator/Graphics/SimTKMeshLoader.h>
#include <OpenSimCreator/UI/MeshImporter/DrawableThing.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterHover.h>
#include <OpenSimCreator/UI/MeshImporter/MeshLoader.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Geometries.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Graphics/Scene/ShaderCache.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Widgets/LogViewer.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/UID.h>
#include <SDL_events.h>

#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace OpenSim { class Model; }

using namespace osc::literals;

namespace osc::mi
{
    // shared data support
    //
    // data that's shared between multiple UI states.
    class MeshImporterSharedState final {
    public:
        MeshImporterSharedState() = default;

        explicit MeshImporterSharedState(std::vector<std::filesystem::path> meshFiles)
        {
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
            catch (std::exception const& ex)
            {
                log_error("error occurred while trying to create an OpenSim model from the mesh editor scene: %s", ex.what());
            }
        }

        //
        // MODEL GRAPH STUFF
        //

        bool openOsimFileAsModelGraph()
        {
            std::optional<std::filesystem::path> const maybeOsimPath = PromptUserForFile("osim");

            if (maybeOsimPath)
            {
                m_ModelGraphSnapshots = UndoableDocument{CreateModelFromOsimFile(*maybeOsimPath)};
                m_MaybeModelGraphExportLocation = *maybeOsimPath;
                m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.getHeadID();
                return true;
            }
            else
            {
                return false;
            }
        }

        bool exportAsModelGraphAsOsimFile()
        {
            std::optional<std::filesystem::path> const maybeExportPath =
                PromptUserForFileSaveLocationAndAddExtensionIfNecessary("osim");

            if (!maybeExportPath)
            {
                return false;  // user probably cancelled out
            }

            return exportModelGraphTo(*maybeExportPath);
        }

        bool exportModelGraphAsOsimFile()
        {
            if (m_MaybeModelGraphExportLocation.empty())
            {
                return exportAsModelGraphAsOsimFile();
            }

            return exportModelGraphTo(m_MaybeModelGraphExportLocation);
        }

        bool isModelGraphUpToDateWithDisk() const
        {
            return m_MaybeModelGraphExportedUID == m_ModelGraphSnapshots.getHeadID();
        }

        bool isCloseRequested() const
        {
            return m_CloseRequested;
        }

        void requestClose()
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
            ss << ICON_FA_CUBE << ' ' << getDocumentName();
            return std::move(ss).str();
        }

        Document const& getModelGraph() const
        {
            return m_ModelGraphSnapshots.getScratch();
        }

        Document& updModelGraph()
        {
            return m_ModelGraphSnapshots.updScratch();
        }

        UndoableDocument& updCommittableModelGraph()
        {
            return m_ModelGraphSnapshots;
        }

        void commitCurrentModelGraph(std::string_view commitMsg)
        {
            m_ModelGraphSnapshots.commitScratch(commitMsg);
        }

        bool canUndoCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.canUndo();
        }

        void undoCurrentModelGraph()
        {
            m_ModelGraphSnapshots.undo();
        }

        bool canRedoCurrentModelGraph() const
        {
            return m_ModelGraphSnapshots.canRedo();
        }

        void redoCurrentModelGraph()
        {
            m_ModelGraphSnapshots.redo();
        }

        std::unordered_set<UID> const& getCurrentSelection() const
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

        std::vector<std::filesystem::path> promptUserForMeshFiles() const
        {
            return PromptUserForFiles(GetCommaDelimitedListOfSupportedSimTKMeshFormats());
        }

        void pushMeshLoadRequests(UID attachmentPoint, std::vector<std::filesystem::path> paths)
        {
            m_MeshLoader.send(MeshLoadRequest{attachmentPoint, std::move(paths)});
        }

        void promptUserForMeshFilesAndPushThemOntoMeshLoader()
        {
            pushMeshLoadRequests(promptUserForMeshFiles());
        }

        //
        // UI OVERLAY STUFF
        //

        Vec2 worldPosToScreenPos(Vec3 const& worldPos) const
        {
            return getCamera().projectOntoScreenRect(worldPos, get3DSceneRect());
        }

        void drawConnectionLine(
            ImU32 color,
            Vec3 const& parent,
            Vec3 const& child) const
        {
            // the line
            ui::GetWindowDrawList()->AddLine(worldPosToScreenPos(parent), worldPosToScreenPos(child), color, c_ConnectionLineWidth);

            // the triangle
            drawConnectionLineTriangleAtMidpoint(color, parent, child);
        }

        void drawConnectionLines(
            Color const& color,
            std::unordered_set<UID> const& excludedIDs) const
        {
            Document const& mg = getModelGraph();
            ImU32 colorU32 = ui::ToImU32(color);

            for (MIObject const& el : mg.iter())
            {
                UID id = el.getID();

                if (excludedIDs.find(id) != excludedIDs.end())
                {
                    continue;
                }

                if (!shouldShowConnectionLines(el))
                {
                    continue;
                }

                if (el.getNumCrossReferences() > 0)
                {
                    drawConnectionLines(el, colorU32, excludedIDs);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    drawConnectionLineToGround(el, colorU32);
                }
            }
        }

        void drawConnectionLines(Color const& color) const
        {
            drawConnectionLines(color, {});
        }

        void drawConnectionLines(MeshImporterHover const& currentHover) const
        {
            Document const& mg = getModelGraph();
            ImU32 color = ui::ToImU32(m_Colors.connectionLines);

            for (MIObject const& el : mg.iter())
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
                    drawConnectionLines(el, color);
                }
                else if (!IsAChildAttachmentInAnyJoint(mg, el))
                {
                    drawConnectionLineToGround(el, color);
                }
            }
            //drawConnectionLines(m_Colors.connectionLines);
        }

        //
        // RENDERING STUFF
        //

        void setContentRegionAvailAsSceneRect()
        {
            set3DSceneRect(ui::ContentRegionAvailScreenRect());
        }

        void drawScene(std::span<DrawableThing const> drawables)
        {
            // setup rendering params
            SceneRendererParams p;
            p.dimensions = dimensions(get3DSceneRect());
            p.antiAliasingLevel = App::get().getCurrentAntiAliasingLevel();
            p.drawRims = true;
            p.drawFloor = false;
            p.nearClippingPlane = m_3DSceneCamera.znear;
            p.farClippingPlane = m_3DSceneCamera.zfar;
            p.viewMatrix = m_3DSceneCamera.view_matrix();
            p.projectionMatrix = m_3DSceneCamera.projection_matrix(AspectRatio(p.dimensions));
            p.viewPos = m_3DSceneCamera.getPos();
            p.lightDirection = RecommendedLightDirection(m_3DSceneCamera);
            p.lightColor = Color::white();
            p.ambientStrength *= 1.5f;
            p.backgroundColor = getColorSceneBackground();

            std::vector<SceneDecoration> decs;
            decs.reserve(drawables.size());
            for (DrawableThing const& dt : drawables)
            {
                decs.push_back({
                    dt.mesh,
                    dt.transform,
                    dt.color,
                    std::string{},
                    dt.flags,
                    dt.maybeMaterial,
                    dt.maybePropertyBlock
                });
            }

            // render
            m_SceneRenderer.render(decs, p);

            // send texture to ImGui
            ui::DrawTextureAsImGuiImage(m_SceneRenderer.updRenderTexture(), m_SceneRenderer.getDimensions());

            // handle hittesting, etc.
            setIsRenderHovered(ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
        }

        bool isRenderHovered() const
        {
            return m_IsRenderHovered;
        }

        Rect const& get3DSceneRect() const
        {
            return m_3DSceneRect;
        }

        Vec2 get3DSceneDims() const
        {
            return dimensions(m_3DSceneRect);
        }

        PolarPerspectiveCamera const& getCamera() const
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

        void focusCameraOn(Vec3 const& focusPoint)
        {
            m_3DSceneCamera.focusPoint = -focusPoint;
        }

        std::span<Color const> getColors() const
        {
            static_assert(offsetof(Colors, ground) == 0);
            static_assert(sizeof(Colors) % sizeof(Color) == 0);
            return {&m_Colors.ground, sizeof(m_Colors)/sizeof(Color)};
        }

        void setColor(size_t i, Color const& newColorValue)
        {
            updColors()[i] = newColorValue;
        }

        std::span<char const* const> getColorLabels() const
        {
            return c_ColorNames;
        }

        Color const& getColorConnectionLine() const
        {
            return m_Colors.connectionLines;
        }

        std::span<bool const> getVisibilityFlags() const
        {
            static_assert(offsetof(VisibilityFlags, ground) == 0);
            static_assert(sizeof(VisibilityFlags) % sizeof(bool) == 0);
            return {&m_VisibilityFlags.ground, sizeof(m_VisibilityFlags)/sizeof(bool)};
        }

        void setVisibilityFlag(size_t i, bool newVisibilityValue)
        {
            updVisibilityFlags()[i] = newVisibilityValue;
        }

        std::span<char const* const> getVisibilityFlagLabels() const
        {
            return c_VisibilityFlagNames;
        }

        bool isShowingFloor() const
        {
            return m_VisibilityFlags.floor;
        }

        DrawableThing generateFloorDrawable() const
        {
            Transform t = getFloorTransform();
            t.scale *= 0.5f;

            Material material
            {
                App::singleton<ShaderCache>(App::resource_loader())->load(
                    "shaders/SolidColor.vert",
                    "shaders/SolidColor.frag"
                )
            };
            material.setColor("uColor", m_Colors.gridLines);
            material.setTransparent(true);

            DrawableThing dt;
            dt.id = MIIDs::Empty();
            dt.groupId = MIIDs::Empty();
            dt.mesh = App::singleton<SceneCache>()->get100x100GridMesh();
            dt.transform = t;
            dt.color = m_Colors.gridLines;
            dt.flags = SceneDecorationFlags::None;
            dt.maybeMaterial = std::move(material);
            return dt;
        }

        //
        // HOVERTEST/INTERACTIVITY
        //

        std::span<bool const> getIneractivityFlags() const
        {
            static_assert(offsetof(InteractivityFlags, ground) == 0);
            static_assert(sizeof(InteractivityFlags) % sizeof(bool) == 0);
            return {&m_InteractivityFlags.ground, sizeof(m_InteractivityFlags)/sizeof(bool)};
        }

        void setInteractivityFlag(size_t i, bool newInteractivityValue)
        {
            updInteractivityFlags()[i] = newInteractivityValue;
        }

        std::span<char const* const> getInteractivityFlagLabels() const
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

        MeshImporterHover doHovertest(std::vector<DrawableThing> const& drawables) const
        {
            auto cache = App::singleton<SceneCache>();

            Rect const sceneRect = get3DSceneRect();
            Vec2 const mousePos = ui::GetMousePos();

            if (!is_point_in_rect(sceneRect, mousePos))
            {
                // mouse isn't over the scene render
                return MeshImporterHover{};
            }

            Vec2 const sceneDims = dimensions(sceneRect);
            Vec2 const relMousePos = mousePos - sceneRect.p1;

            Line const ray = getCamera().unprojectTopLeftPosToWorldRay(relMousePos, sceneDims);
            bool const hittestMeshes = isMeshesInteractable();
            bool const hittestBodies = isBodiesInteractable();
            bool const hittestJointCenters = isJointCentersInteractable();
            bool const hittestGround = isGroundInteractable();
            bool const hittestStations = isStationsInteractable();

            UID closestID = MIIDs::Empty();
            float closestDist = std::numeric_limits<float>::max();
            for (DrawableThing const& drawable : drawables)
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

                std::optional<RayCollision> const rc = GetClosestWorldspaceRayCollision(
                    drawable.mesh,
                    cache->getBVH(drawable.mesh),
                    drawable.transform,
                    ray
                );

                if (rc && rc->distance < closestDist)
                {
                    closestID = drawable.id;
                    closestDist = rc->distance;
                }
            }

            Vec3 const hitPos = closestID != MIIDs::Empty() ? ray.origin + closestDist*ray.direction : Vec3{};

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

        DrawableThing generateMeshDrawable(osc::mi::Mesh const& el) const
        {
            DrawableThing rv;
            rv.id = el.getID();
            rv.groupId = MIIDs::MeshGroup();
            rv.mesh = el.getMeshData();
            rv.transform = el.getXForm();
            rv.color = el.getParentID() == MIIDs::Ground() || el.getParentID() == MIIDs::Empty() ? RedifyColor(getColorMesh()) : getColorMesh();
            rv.flags = SceneDecorationFlags::None;
            return rv;
        }

        void appendDrawables(
            MIObject const& e,
            std::vector<DrawableThing>& appendOut) const
        {
            std::visit(Overload
            {
                [this, &appendOut](Ground const&)
                {
                    if (!isShowingGround())
                    {
                        return;
                    }

                    appendOut.push_back(generateGroundSphere(getColorGround()));
                },
                [this, &appendOut](Mesh const& el)
                {
                    if (!isShowingMeshes())
                    {
                        return;
                    }

                    appendOut.push_back(generateMeshDrawable(el));
                },
                [this, &appendOut](Body const& el)
                {
                    if (!isShowingBodies())
                    {
                        return;
                    }

                    appendBodyElAsCubeThing(el, appendOut);
                },
                [this, &appendOut](Joint const& el)
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
                        SceneDecorationFlags::None,
                        GetJointAxisLengths(el)
                    );
                },
                [this, &appendOut](StationEl const& el)
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
        enum PanelIndex_ {
            PanelIndex_History = 0,
            PanelIndex_Navigator,
            PanelIndex_Log,
            PanelIndex_Performance,
            PanelIndex_COUNT,
        };
        size_t getNumToggleablePanels() const
        {
            return static_cast<size_t>(PanelIndex_COUNT);
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

        bool isPanelEnabled(PanelIndex_ idx) const
        {
            return m_PanelStates[idx];
        }

        void setPanelEnabled(PanelIndex_ idx, bool v)
        {
            m_PanelStates[idx] = v;
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

        bool onEvent(SDL_Event const& e)
        {
            // if the user drags + drops a file into the window, assume it's a meshfile
            // and start loading it
            if (e.type == SDL_DROPFILE && e.drop.file != nullptr)
            {
                m_DroppedFiles.emplace_back(e.drop.file);
                return true;
            }

            return false;
        }

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

            m_ModelGraphSnapshots.updScratch().garbageCollect();
        }

    private:
        bool exportModelGraphTo(std::filesystem::path const& exportPath)
        {
            std::vector<std::string> issues;
            std::unique_ptr<OpenSim::Model> m;

            try
            {
                m = CreateOpenSimModelFromMeshImporterDocument(getModelGraph(), m_ModelCreationFlags, issues);
            }
            catch (std::exception const& ex)
            {
                log_error("error occurred while trying to create an OpenSim model from the mesh editor scene: %s", ex.what());
            }

            if (m)
            {
                m->print(exportPath.string());
                m_MaybeModelGraphExportLocation = exportPath;
                m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.getHeadID();
                return true;
            }
            else
            {
                for (std::string const& issue : issues)
                {
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

        void pushMeshLoadRequests(std::vector<std::filesystem::path> paths)
        {
            pushMeshLoadRequests(MIIDs::Ground(), std::move(paths));
        }

        void pushMeshLoadRequest(UID attachmentPoint, std::filesystem::path const& path)
        {
            pushMeshLoadRequests(attachmentPoint, std::vector<std::filesystem::path>{path});
        }

        void pushMeshLoadRequest(std::filesystem::path const& meshFilePath)
        {
            pushMeshLoadRequest(MIIDs::Ground(), meshFilePath);
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

            for (LoadedMesh const& lm : ok.meshes)
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
                    commitMsgSS << "loaded " << ok.meshes[0].path.filename();
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
            ImU32 color,
            Vec3 parent,
            Vec3 child) const
        {
            constexpr float triangleWidth = 6.0f * c_ConnectionLineWidth;
            constexpr float triangleWidthSquared = triangleWidth*triangleWidth;

            Vec2 const parentScr = worldPosToScreenPos(parent);
            Vec2 const childScr = worldPosToScreenPos(child);
            Vec2 const child2ParentScr = parentScr - childScr;

            if (dot(child2ParentScr, child2ParentScr) < triangleWidthSquared)
            {
                return;
            }

            Vec3 const mp = midpoint(parent, child);
            Vec2 const midpointScr = worldPosToScreenPos(mp);
            Vec2 const directionScr = normalize(child2ParentScr);
            Vec2 const directionNormalScr = {-directionScr.y, directionScr.x};

            Vec2 const p1 = midpointScr + (triangleWidth/2.0f)*directionNormalScr;
            Vec2 const p2 = midpointScr - (triangleWidth/2.0f)*directionNormalScr;
            Vec2 const p3 = midpointScr + triangleWidth*directionScr;

            ui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, color);
        }

        void drawConnectionLines(
            MIObject const& el,
            ImU32 color,
            std::unordered_set<UID> const& excludedIDs) const
        {
            Document const& mg = getModelGraph();
            for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
            {
                UID refID = el.getCrossReferenceConnecteeID(i);

                if (excludedIDs.find(refID) != excludedIDs.end())
                {
                    continue;
                }

                MIObject const* other = mg.tryGetByID(refID);

                if (!other)
                {
                    continue;
                }

                Vec3 child = el.getPos(mg);
                Vec3 parent = other->getPos(mg);

                if (el.getCrossReferenceDirection(i) == CrossrefDirection::ToChild)
                {
                    std::swap(parent, child);
                }

                drawConnectionLine(color, parent, child);
            }
        }

        void drawConnectionLines(MIObject const& el, ImU32 color) const
        {
            drawConnectionLines(el, color, std::unordered_set<UID>{});
        }

        void drawConnectionLineToGround(MIObject const& el, ImU32 color) const
        {
            if (el.getID() == MIIDs::Ground())
            {
                return;
            }

            drawConnectionLine(color, Vec3{}, el.getPos(getModelGraph()));
        }

        bool shouldShowConnectionLines(MIObject const& el) const
        {
            return std::visit(Overload
            {
                []    (Ground const&)  { return false; },
                [this](Mesh const&)    { return this->isShowingMeshConnectionLines(); },
                [this](Body const&)    { return this->isShowingBodyConnectionLines(); },
                [this](Joint const&)   { return this->isShowingJointConnectionLines(); },
                [this](StationEl const&) { return this->isShowingMeshConnectionLines(); },
            }, el.toVariant());
        }

        void setIsRenderHovered(bool newIsHovered)
        {
            m_IsRenderHovered = newIsHovered;
        }

        void set3DSceneRect(Rect const& newRect)
        {
            m_3DSceneRect = newRect;
        }

        std::span<Color> updColors()
        {
            static_assert(offsetof(Colors, ground) == 0);
            static_assert(sizeof(Colors) % sizeof(Color) == 0);
            return {&m_Colors.ground, sizeof(m_Colors)/sizeof(Color)};
        }

        Color const& getColorSceneBackground() const
        {
            return m_Colors.sceneBackground;
        }

        Color const& getColorGround() const
        {
            return m_Colors.ground;
        }

        Color const& getColorMesh() const
        {
            return m_Colors.meshes;
        }

        Color const& getColorStation() const
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

        Transform getFloorTransform() const
        {
            return {
                .scale = {m_SceneScaleFactor * 100.0f, m_SceneScaleFactor * 100.0f, 1.0f},
                .rotation = angle_axis(90_deg, Vec3{-1.0f, 0.0f, 0.0f}),
            };
        }

        float getSphereRadius() const
        {
            return 0.02f * m_SceneScaleFactor;
        }

        Sphere sphereAtTranslation(Vec3 const& translation) const
        {
            return Sphere{translation, getSphereRadius()};
        }

        void appendAsFrame(
            UID logicalID,
            UID groupID,
            Transform const& xform,
            std::vector<DrawableThing>& appendOut,
            float alpha = 1.0f,
            SceneDecorationFlags flags = SceneDecorationFlags::None,
            Vec3 legLen = {1.0f, 1.0f, 1.0f},
            Color coreColor = Color::white()) const
        {
            float const coreRadius = getSphereRadius();
            float const legThickness = 0.5f * coreRadius;

            // this is how much the cylinder has to be "pulled in" to the core to hide the edges
            float const cylinderPullback = coreRadius * sin((180_deg * legThickness) / coreRadius);

            // emit origin sphere
            appendOut.push_back({
                .id = logicalID,
                .groupId = groupID,
                .mesh = m_SphereMesh,
                .transform = {
                    .scale = Vec3{coreRadius},
                    .rotation = xform.rotation,
                    .position = xform.position,
                },
                .color = coreColor.withAlpha(coreColor.a * alpha),
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

                Vec3 const meshDirection = {0.0f, 1.0f, 0.0f};
                Vec3 cylinderDirection = {};
                cylinderDirection[i] = 1.0f;

                float const actualLegLen = 4.0f * legLen[i] * coreRadius;

                Transform t;
                t.scale.x = legThickness;
                t.scale.y = 0.5f * actualLegLen;  // cylinder is 2 units high
                t.scale.z = legThickness;
                t.rotation = normalize(xform.rotation * rotation(meshDirection, cylinderDirection));
                t.position = xform.position + (t.rotation * (((getSphereRadius() + (0.5f * actualLegLen)) - cylinderPullback) * meshDirection));

                Color color = {0.0f, 0.0f, 0.0f, alpha};
                color[i] = 1.0f;

                DrawableThing& se = appendOut.emplace_back();
                se.id = logicalID;
                se.groupId = groupID;
                se.mesh = m_CylinderMesh;
                se.transform = t;
                se.color = color;
                se.flags = flags;
            }
        }

        void appendAsCubeThing(
            UID logicalID,
            UID groupID,
            Transform const& xform,
            std::vector<DrawableThing>& appendOut) const
        {
            float const halfWidth = 1.5f * getSphereRadius();

            // core
            {
                Transform scaled{xform};
                scaled.scale *= halfWidth;

                DrawableThing& originCube = appendOut.emplace_back();
                originCube.id = logicalID;
                originCube.groupId = groupID;
                originCube.mesh = App::singleton<SceneCache>()->getBrickMesh();
                originCube.transform = scaled;
                originCube.color = Color::white();
                originCube.flags = SceneDecorationFlags::None;
            }

            // legs
            for (int i = 0; i < 3; ++i)
            {
                // cone mesh has a source height of 2, stretches from -1 to +1 in Y
                float const coneHeight = 0.75f * halfWidth;

                Vec3 const meshDirection = {0.0f, 1.0f, 0.0f};
                Vec3 coneDirection = {};
                coneDirection[i] = 1.0f;

                Transform t;
                t.scale.x = 0.5f * halfWidth;
                t.scale.y = 0.5f * coneHeight;
                t.scale.z = 0.5f * halfWidth;
                t.rotation = xform.rotation * rotation(meshDirection, coneDirection);
                t.position = xform.position + (t.rotation * ((halfWidth + (0.5f * coneHeight)) * meshDirection));

                Color color = {0.0f, 0.0f, 0.0f, 1.0f};
                color[i] = 1.0f;

                DrawableThing& legCube = appendOut.emplace_back();
                legCube.id = logicalID;
                legCube.groupId = groupID;
                legCube.mesh = App::singleton<SceneCache>()->getConeMesh();
                legCube.transform = t;
                legCube.color = color;
                legCube.flags = SceneDecorationFlags::None;
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

        void appendBodyElAsCubeThing(Body const& bodyEl, std::vector<DrawableThing>& appendOut) const
        {
            appendAsCubeThing(bodyEl.getID(), MIIDs::BodyGroup(), bodyEl.getXForm(), appendOut);
        }

        DrawableThing generateBodyElSphere(Body const& bodyEl, Color const& color) const
        {
            DrawableThing rv;
            rv.id = bodyEl.getID();
            rv.groupId = MIIDs::BodyGroup();
            rv.mesh = m_SphereMesh;
            rv.transform = SphereMeshToSceneSphereTransform(sphereAtTranslation(bodyEl.getXForm().position));
            rv.color = color;
            rv.flags = SceneDecorationFlags::None;
            return rv;
        }

        DrawableThing generateGroundSphere(Color const& color) const
        {
            DrawableThing rv;
            rv.id = MIIDs::Ground();
            rv.groupId = MIIDs::GroundGroup();
            rv.mesh = m_SphereMesh;
            rv.transform = SphereMeshToSceneSphereTransform(sphereAtTranslation({0.0f, 0.0f, 0.0f}));
            rv.color = color;
            rv.flags = SceneDecorationFlags::None;
            return rv;
        }

        DrawableThing generateStationSphere(StationEl const& el, Color const& color) const
        {
            DrawableThing rv;
            rv.id = el.getID();
            rv.groupId = MIIDs::StationGroup();
            rv.mesh = m_SphereMesh;
            rv.transform = SphereMeshToSceneSphereTransform(sphereAtTranslation(el.getPos(getModelGraph())));
            rv.color = color;
            rv.flags = SceneDecorationFlags::None;
            return rv;
        }

        Color RedifyColor(Color const& srcColor) const
        {
            constexpr float factor = 0.8f;
            return {srcColor[0], factor * srcColor[1], factor * srcColor[2], factor * srcColor[3]};
        }

        // returns a transform that maps a sphere mesh (defined to be @ 0,0,0 with radius 1)
        // to some sphere in the scene (e.g. a body/ground)
        Transform SphereMeshToSceneSphereTransform(Sphere const& sceneSphere) const
        {
            Transform t;
            t.scale *= sceneSphere.radius;
            t.position = sceneSphere.origin;
            return t;
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
        UID m_MaybeModelGraphExportedUID = m_ModelGraphSnapshots.getHeadID();

        // a batch of files that the user drag-dropped into the UI in the last frame
        std::vector<std::filesystem::path> m_DroppedFiles;

        // loads meshes in a background thread
        MeshLoader m_MeshLoader;

        // sphere mesh used by various scene elements
        osc::Mesh m_SphereMesh = SphereGeometry::generate_mesh(1.0f, 12, 12);

        // cylinder mesh used by various scene elements
        osc::Mesh m_CylinderMesh = CylinderGeometry::generate_mesh(1.0f, 1.0f, 2.0f, 16);

        // cone mesh used to render scene elements
        osc::Mesh m_ConeMesh = ConeGeometry::generate_mesh(1.0f, 2.0f, 16);

        // main 3D scene camera
        PolarPerspectiveCamera m_3DSceneCamera = CreateDefaultCamera();

        // screenspace rect where the 3D scene is currently being drawn to
        Rect m_3DSceneRect = {};

        // renderer that draws the scene
        SceneRenderer m_SceneRenderer{
            *App::singleton<SceneCache>(),
            *App::singleton<ShaderCache>(App::resource_loader())
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
        static constexpr auto c_ColorNames = std::to_array<char const*>(
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
        static constexpr auto c_VisibilityFlagNames = std::to_array<char const*>(
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
        static constexpr auto c_InteractivityFlagNames = std::to_array<char const*>(
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
        static constexpr auto c_OpenedPanelNames = std::to_array<char const*>(
        {
            "History",
            "Navigator",
            "Log",
            "Performance",
        });
        static_assert(c_OpenedPanelNames.size() == c_NumPanelStates);
        static_assert(PanelIndex_COUNT == c_NumPanelStates);
        LogViewer m_Logviewer;
        PerfPanel m_PerfPanel{"Performance"};

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
