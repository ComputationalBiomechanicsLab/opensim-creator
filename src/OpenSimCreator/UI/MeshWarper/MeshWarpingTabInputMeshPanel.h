#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabContextMenu.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabDecorationGenerators.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabHover.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabPanel.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Scene/CachedSceneRenderer.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using osc::lm::LandmarkCSVFlags;

namespace osc
{
    // an "input" panel (i.e. source or destination mesh, before warping)
    class MeshWarpingTabInputMeshPanel final : public MeshWarpingTabPanel {
    public:
        MeshWarpingTabInputMeshPanel(
            std::string_view panelName_,
            std::shared_ptr<MeshWarpingTabSharedState> state_,
            TPSDocumentInputIdentifier documentIdentifier_) :

            MeshWarpingTabPanel{panelName_, ImGuiDockNodeFlags_PassthruCentralNode},
            m_State{std::move(state_)},
            m_DocumentIdentifier{documentIdentifier_}
        {
        }
    private:
        // draws all of the panel's content
        void implDrawContent() final
        {
            // compute top-level UI variables (render rect, mouse pos, etc.)
            Rect const contentRect = ui::ContentRegionAvailScreenRect();
            Vec2 const contentRectDims = dimensions(contentRect);
            Vec2 const mousePos = ui::GetMousePos();

            // un-project mouse's (2D) location into the 3D scene as a ray
            Line const cameraRay = m_Camera.unprojectTopLeftPosToWorldRay(mousePos - contentRect.p1, contentRectDims);

            // mesh hittest: compute whether the user is hovering over the mesh (affects rendering)
            Mesh const& inputMesh = m_State->getScratchMesh(m_DocumentIdentifier);
            BVH const& inputMeshBVH = m_State->getScratchMeshBVH(m_DocumentIdentifier);
            std::optional<RayCollision> const meshCollision = m_LastTextureHittestResult.isHovered ?
                GetClosestWorldspaceRayCollision(inputMesh, inputMeshBVH, Transform{}, cameraRay) :
                std::nullopt;

            // landmark hittest: compute whether the user is hovering over a landmark
            std::optional<MeshWarpingTabHover> landmarkCollision = m_LastTextureHittestResult.isHovered ?
                getMouseLandmarkCollisions(cameraRay) :
                std::nullopt;

            // state update: tell central state if something's being hovered in this panel
            if (landmarkCollision)
            {
                m_State->currentHover = landmarkCollision;
            }
            else if (meshCollision)
            {
                m_State->currentHover.emplace(m_DocumentIdentifier, meshCollision->position);
            }

            // update camera: NOTE: make sure it's updated *before* rendering; otherwise, it'll be one frame late
            updateCamera();

            // render 3D: draw the scene into the content rect and 2D-hittest it
            RenderTexture& renderTexture = renderScene(contentRectDims, meshCollision, landmarkCollision);
            ui::DrawTextureAsImGuiImage(renderTexture);
            m_LastTextureHittestResult = ui::HittestLastImguiItem();

            // handle any events due to hovering over, clicking, etc.
            handleInputAndHoverEvents(m_LastTextureHittestResult, meshCollision, landmarkCollision);

            // render 2D: draw any 2D overlays over the 3D render
            draw2DOverlayUI(m_LastTextureHittestResult.rect);
        }

        // update the 3D camera from user inputs/external data
        void updateCamera()
        {
            // if the cameras are linked together, ensure this camera is updated from the linked camera
            if (m_State->linkCameras && m_Camera != m_State->linkedCameraBase)
            {
                if (m_State->onlyLinkRotation)
                {
                    m_Camera.phi = m_State->linkedCameraBase.phi;
                    m_Camera.theta = m_State->linkedCameraBase.theta;
                }
                else
                {
                    m_Camera = m_State->linkedCameraBase;
                }
            }

            // if the user interacts with the render, update the camera as necessary
            if (m_LastTextureHittestResult.isHovered)
            {
                if (ui::UpdatePolarCameraFromImGuiMouseInputs(m_Camera, dimensions(m_LastTextureHittestResult.rect)))
                {
                    m_State->linkedCameraBase = m_Camera;  // reflects latest modification
                }
            }
        }

        // returns the closest collision, if any, between the provided camera ray and a landmark
        std::optional<MeshWarpingTabHover> getMouseLandmarkCollisions(Line const& cameraRay) const
        {
            std::optional<MeshWarpingTabHover> rv;
            hittestLandmarks(cameraRay, rv);
            hittestNonParticipatingLandmarks(cameraRay, rv);
            return rv;
        }

        // 3D hittests landmarks and updates `closest` if a closer collision is found
        void hittestLandmarks(
            Line const& cameraRay,
            std::optional<MeshWarpingTabHover>& closest) const
        {
            for (TPSDocumentLandmarkPair const& landmark : m_State->getScratch().landmarkPairs)
            {
                hittestLandmark(cameraRay, closest, landmark);
            }
        }

        // 3D hittests one landmark and updates `closest` if a closer collision is found
        void hittestLandmark(
            Line const& cameraRay,
            std::optional<MeshWarpingTabHover>& closest,
            TPSDocumentLandmarkPair const& landmark) const
        {
            std::optional<Vec3> const maybePos = GetLocation(landmark, m_DocumentIdentifier);
            if (!maybePos)
            {
                return;  // landmark doesn't have a source/destination
            }

            // hittest the landmark as an analytic sphere
            Sphere const landmarkSphere = {.origin = *maybePos, .radius = m_LandmarkRadius};
            if (auto const collision = find_collision(cameraRay, landmarkSphere))
            {
                if (!closest || length(closest->getWorldspaceLocation() - cameraRay.origin) > collision->distance)
                {
                    TPSDocumentElementID fullID{landmark.uid, TPSDocumentElementType::Landmark, m_DocumentIdentifier};
                    closest.emplace(std::move(fullID), *maybePos);
                }
            }
        }

        // 3D hittests non-participating landmarks and updates `closest` if a closer collision is found
        void hittestNonParticipatingLandmarks(
            Line const& cameraRay,
            std::optional<MeshWarpingTabHover>& closest) const
        {
            for (auto const& nonPariticpatingLandmark : m_State->getScratch().nonParticipatingLandmarks)
            {
                hittestNonParticipatingLandmark(cameraRay, closest, nonPariticpatingLandmark);
            }
        }

        // 3D hittests one non-participating landmark and updates `closest` if a closer collision is found
        void hittestNonParticipatingLandmark(
            Line const& cameraRay,
            std::optional<MeshWarpingTabHover>& closest,
            TPSDocumentNonParticipatingLandmark const& nonPariticpatingLandmark) const
        {
            // hittest non-participating landmark as an analytic sphere

            Sphere const decorationSphere = {
                .origin = nonPariticpatingLandmark.location,
                .radius = GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius
            };

            if (auto const collision = find_collision(cameraRay, decorationSphere))
            {
                if (!closest || length(closest->getWorldspaceLocation() - cameraRay.origin) > collision->distance)
                {
                    TPSDocumentElementID fullID{nonPariticpatingLandmark.uid, TPSDocumentElementType::NonParticipatingLandmark, m_DocumentIdentifier};
                    closest.emplace(std::move(fullID), nonPariticpatingLandmark.location);
                }
            }
        }

        // renders this panel's 3D scene to a texture
        RenderTexture& renderScene(
            Vec2 dims,
            std::optional<RayCollision> const& maybeMeshCollision,
            std::optional<MeshWarpingTabHover> const& maybeLandmarkCollision)
        {
            SceneRendererParams const params = CalcStandardDarkSceneRenderParams(
                m_Camera,
                App::get().getCurrentAntiAliasingLevel(),
                dims
            );
            std::vector<SceneDecoration> const decorations = generateDecorations(maybeMeshCollision, maybeLandmarkCollision);
            return m_CachedRenderer.render(decorations, params);
        }

        // returns a fresh list of 3D decorations for this panel's 3D render
        std::vector<SceneDecoration> generateDecorations(
            std::optional<RayCollision> const& maybeMeshCollision,
            std::optional<MeshWarpingTabHover> const& maybeLandmarkCollision) const
        {
            std::vector<SceneDecoration> decorations;
            decorations.reserve(
                6 +
                CountNumLandmarksForInput(m_State->getScratch(), m_DocumentIdentifier) +
                m_State->getScratch().nonParticipatingLandmarks.size()
            );

            std::function<void(SceneDecoration&&)> const decorationConsumer =
                [&decorations](SceneDecoration&& dec) { decorations.push_back(std::move(dec)); };

            // generate common decorations (mesh, wireframe, grid, etc.)
            AppendCommonDecorations(
                *m_State,
                m_State->getScratchMesh(m_DocumentIdentifier),
                m_WireframeMode,
                decorationConsumer
            );

            // generate decorations for all of the landmarks
            generateDecorationsForLandmarks(decorationConsumer);

            // if applicable, generate decorations for the non-participating landmarks
            generateDecorationsForNonParticipatingLandmarks(decorationConsumer);

            // if applicable, show a mouse-to-mesh collision as faded landmark as a placement hint for user
            if (maybeMeshCollision && !maybeLandmarkCollision)
            {
                generateDecorationsForMouseOverMeshHover(maybeMeshCollision->position, decorationConsumer);
            }

            return decorations;
        }

        void generateDecorationsForLandmarks(
            std::function<void(SceneDecoration&&)> const& decorationConsumer) const
        {
            for (TPSDocumentLandmarkPair const& landmarkPair : m_State->getScratch().landmarkPairs)
            {
                generateDecorationsForLandmark(landmarkPair, decorationConsumer);
            }
        }

        void generateDecorationsForLandmark(
            TPSDocumentLandmarkPair const& landmarkPair,
            std::function<void(SceneDecoration&&)> const& decorationConsumer) const
        {
            std::optional<Vec3> const maybeLocation = GetLocation(landmarkPair, m_DocumentIdentifier);
            if (!maybeLocation)
            {
                return;  // no source/destination location for the landmark
            }

            SceneDecoration decoration
            {
                .mesh = m_State->landmarkSphere,
                .transform = {.scale = Vec3{m_LandmarkRadius}, .position = *maybeLocation},
                .color = IsFullyPaired(landmarkPair) ? m_State->pairedLandmarkColor : m_State->unpairedLandmarkColor,
            };

            TPSDocumentElementID const landmarkID{landmarkPair.uid, TPSDocumentElementType::Landmark, m_DocumentIdentifier};
            if (m_State->isSelected(landmarkID))
            {
                decoration.flags |= SceneDecorationFlags::IsSelected;
            }
            if (m_State->isHovered(landmarkID))
            {
                decoration.color = ToSRGB(ClampToLDR(MultiplyLuminance(ToLinear(decoration.color), 1.2f)));
                decoration.flags |= SceneDecorationFlags::IsHovered;
            }

            decorationConsumer(std::move(decoration));
        }

        void generateDecorationsForNonParticipatingLandmarks(
            std::function<void(SceneDecoration&&)> const& decorationConsumer) const
        {
            if (m_DocumentIdentifier != TPSDocumentInputIdentifier::Source)
            {
                return;  // only show them on the source (to-be-warped) mesh
            }

            for (auto const& nonParticipatingLandmark : m_State->getScratch().nonParticipatingLandmarks)
            {
                generateDecorationsForNonParticipatingLandmark(nonParticipatingLandmark, decorationConsumer);
            }
        }

        void generateDecorationsForNonParticipatingLandmark(
            TPSDocumentNonParticipatingLandmark const& npl,
            std::function<void(SceneDecoration&&)> const& decorationConsumer) const
        {
            SceneDecoration decoration
            {
                .mesh = m_State->landmarkSphere,
                .transform =
                {
                    .scale = Vec3{GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius},
                    .position = npl.location,
                },
                .color = m_State->nonParticipatingLandmarkColor,
            };
            TPSDocumentElementID const id{npl.uid, TPSDocumentElementType::NonParticipatingLandmark, m_DocumentIdentifier};
            if (m_State->isSelected(id))
            {
                decoration.flags |= SceneDecorationFlags::IsSelected;
            }
            if (m_State->isHovered(id))
            {
                decoration.color = ToSRGB(ClampToLDR(MultiplyLuminance(ToLinear(decoration.color), 1.2f)));
                decoration.flags |= SceneDecorationFlags::IsHovered;
            }

            decorationConsumer(std::move(decoration));
        }

        void generateDecorationsForMouseOverMeshHover(
            Vec3 const& meshCollisionPosition,
            std::function<void(SceneDecoration&&)> const& decorationConsumer) const
        {
            bool const nonParticipating = isUserPlacingNonParticipatingLandmark();

            Color const color = nonParticipating ?
                m_State->nonParticipatingLandmarkColor :
                m_State->unpairedLandmarkColor;

            float const radius = nonParticipating ?
                GetNonParticipatingLandmarkScaleFactor()*m_LandmarkRadius :
                m_LandmarkRadius;

            decorationConsumer({
                .mesh = m_State->landmarkSphere,
                .transform = {.scale = Vec3{radius}, .position = meshCollisionPosition},
                .color = color.with_alpha(0.8f),  // faded
            });
        }

        // handle any input-related side-effects
        void handleInputAndHoverEvents(
            ui::ImGuiItemHittestResult const& htResult,
            std::optional<RayCollision> const& meshCollision,
            std::optional<MeshWarpingTabHover> const& landmarkCollision)
        {
            // event: if the user left-clicks and something is hovered, select it; otherwise, add a landmark
            if (htResult.isLeftClickReleasedWithoutDragging)
            {
                if (landmarkCollision && landmarkCollision->isHoveringASceneElement())
                {
                    if (!ui::IsShiftDown())
                    {
                        m_State->clearSelection();
                    }
                    m_State->select(*landmarkCollision->getSceneElementID());
                }
                else if (meshCollision)
                {
                    auto const pos = meshCollision->position;
                    if (isUserPlacingNonParticipatingLandmark())
                    {
                        ActionAddNonParticipatingLandmark(m_State->updUndoable(), pos);
                    }
                    else
                    {
                        ActionAddLandmark(m_State->updUndoable(), m_DocumentIdentifier, pos);
                    }
                }
            }

            // event: if the user right-clicks on a landmark then open the context menu for that landmark
            if (htResult.isRightClickReleasedWithoutDragging)
            {
                if (landmarkCollision && landmarkCollision->isHoveringASceneElement())
                {
                    m_State->emplacePopup<MeshWarpingTabContextMenu>(
                        "##MeshInputContextMenu",
                        m_State,
                        *landmarkCollision->getSceneElementID()
                    );
                }
            }

            // event: if the user is hovering the render while something is selected and the user
            // presses delete then the landmarks should be deleted
            if (htResult.isHovered && ui::IsAnyKeyPressed({ImGuiKey_Delete, ImGuiKey_Backspace}))
            {
                ActionDeleteSceneElementsByID(
                    m_State->updUndoable(),
                    m_State->userSelection.getUnderlyingSet()
                );
                m_State->clearSelection();
            }
        }

        // 2D UI stuff (buttons, sliders, tables, etc.):

        // draws 2D ImGui overlays over the scene render
        void draw2DOverlayUI(Rect const& renderRect)
        {
            ui::SetCursorScreenPos(renderRect.p1 + m_State->overlayPadding);

            drawInformationIcon();
            ui::SameLine();
            drawImportButton();
            ui::SameLine();
            drawExportButton();
            ui::SameLine();
            drawAutoFitCameraButton();
            ui::SameLine();
            drawLandmarkRadiusSlider();
        }

        // draws a information icon that shows basic mesh info when hovered
        void drawInformationIcon()
        {
            ui::ButtonNoBg(ICON_FA_INFO_CIRCLE);
            if (ui::IsItemHovered())
            {
                ui::BeginTooltip();

                ui::TextDisabled("Input Information:");
                drawInputInformationTable();

                ui::EndTooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void drawInputInformationTable()
        {
            if (ui::BeginTable("##inputinfo", 2))
            {
                ui::TableSetupColumn("Name");
                ui::TableSetupColumn("Value");

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("# landmarks");
                ui::TableSetColumnIndex(1);
                ui::Text("%zu", CountNumLandmarksForInput(m_State->getScratch(), m_DocumentIdentifier));

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("# verts");
                ui::TableSetColumnIndex(1);
                ui::Text("%zu", m_State->getScratchMesh(m_DocumentIdentifier).getNumVerts());

                ui::TableNextRow();
                ui::TableSetColumnIndex(0);
                ui::Text("# triangles");
                ui::TableSetColumnIndex(1);
                ui::Text("%zu", m_State->getScratchMesh(m_DocumentIdentifier).getNumIndices()/3);

                ui::EndTable();
            }
        }

        // draws an import button that enables the user to import things for this input
        void drawImportButton()
        {
            ui::Button(ICON_FA_FILE_IMPORT " import" ICON_FA_CARET_DOWN);
            if (ui::BeginPopupContextItem("##importcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ui::MenuItem("Mesh"))
                {
                    ActionLoadMeshFile(m_State->updUndoable(), m_DocumentIdentifier);
                }
                if (ui::MenuItem("Landmarks from CSV"))
                {
                    ActionLoadLandmarksFromCSV(m_State->updUndoable(), m_DocumentIdentifier);
                }
                if (m_DocumentIdentifier == TPSDocumentInputIdentifier::Source &&
                    ui::MenuItem("Non-Participating Landmarks from CSV"))
                {
                    ActionLoadNonParticipatingLandmarksFromCSV(m_State->updUndoable());
                }
                ui::EndPopup();
            }
        }

        // draws an export button that enables the user to export things from this input
        void drawExportButton()
        {
            ui::Button(ICON_FA_FILE_EXPORT " export" ICON_FA_CARET_DOWN);
            if (ui::BeginPopupContextItem("##exportcontextmenu", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ui::MenuItem("Mesh to OBJ"))
                {
                    ActionTrySaveMeshToObjFile(m_State->getScratchMesh(m_DocumentIdentifier));
                }
                if (ui::MenuItem("Mesh to STL"))
                {
                    ActionTrySaveMeshToStlFile(m_State->getScratchMesh(m_DocumentIdentifier));
                }
                if (ui::MenuItem("Landmarks to CSV"))
                {
                    ActionSaveLandmarksToCSV(m_State->getScratch(), m_DocumentIdentifier);
                }
                if (ui::MenuItem("Landmark Positions to CSV"))
                {
                    ActionSaveLandmarksToCSV(m_State->getScratch(), m_DocumentIdentifier, LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                }
                if (ui::MenuItem("Non-Participating Landmarks to CSV"))
                {
                    ActionSaveNonParticipatingLandmarksToCSV(m_State->getScratch());
                }
                if (ui::MenuItem("Non-Participating Landmark Positions to CSV"))
                {
                    ActionSaveNonParticipatingLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoHeader | LandmarkCSVFlags::NoNames);
                }
                ui::EndPopup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void drawAutoFitCameraButton()
        {
            if (ui::Button(ICON_FA_EXPAND_ARROWS_ALT))
            {
                AutoFocus(m_Camera, m_State->getScratchMesh(m_DocumentIdentifier).getBounds(), AspectRatio(m_LastTextureHittestResult.rect));
                m_State->linkedCameraBase = m_Camera;
            }
            ui::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");
        }

        // draws a slider that lets the user edit how large the landmarks are
        void drawLandmarkRadiusSlider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            ImGuiSliderFlags const flags = ImGuiSliderFlags_Logarithmic;

            CStringView const label = "landmark radius";
            ui::SetNextItemWidth(ui::GetContentRegionAvail().x - ui::CalcTextSize(label).x - ui::GetStyle().ItemInnerSpacing.x - m_State->overlayPadding.x);
            ui::SliderFloat(label, &m_LandmarkRadius, 0.0001f, 100.0f, "%.4f", flags);
        }

        bool isUserPlacingNonParticipatingLandmark() const
        {
            static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
            bool const isSourceMesh = m_DocumentIdentifier == TPSDocumentInputIdentifier::Source;
            bool const isCtrlPressed = ui::IsAnyKeyDown({ImGuiKey_LeftCtrl, ImGuiKey_RightCtrl});
            return isSourceMesh && isCtrlPressed;
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        TPSDocumentInputIdentifier m_DocumentIdentifier;
        PolarPerspectiveCamera m_Camera = CreateCameraFocusedOn(m_State->getScratchMesh(m_DocumentIdentifier).getBounds());
        CachedSceneRenderer m_CachedRenderer{
            *App::singleton<SceneCache>(App::resource_loader()),
        };
        ui::ImGuiItemHittestResult m_LastTextureHittestResult;
        bool m_WireframeMode = true;
        float m_LandmarkRadius = 0.05f;
    };
}
