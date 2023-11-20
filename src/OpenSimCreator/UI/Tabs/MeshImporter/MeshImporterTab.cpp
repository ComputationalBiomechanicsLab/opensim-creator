#include "MeshImporterTab.hpp"

#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/ModelGraph/BodyEl.hpp>
#include <OpenSimCreator/ModelGraph/CommittableModelGraph.hpp>
#include <OpenSimCreator/ModelGraph/CommittableModelGraphActions.hpp>
#include <OpenSimCreator/ModelGraph/GroundEl.hpp>
#include <OpenSimCreator/ModelGraph/JointEl.hpp>
#include <OpenSimCreator/ModelGraph/MeshEl.hpp>
#include <OpenSimCreator/ModelGraph/ModelCreationFlags.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraph.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphStrings.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>
#include <OpenSimCreator/ModelGraph/SceneElClass.hpp>
#include <OpenSimCreator/ModelGraph/SceneElHelpers.hpp>
#include <OpenSimCreator/ModelGraph/StationEl.hpp>
#include <OpenSimCreator/Registry/ComponentRegistry.hpp>
#include <OpenSimCreator/Registry/StaticComponentRegistries.hpp>
#include <OpenSimCreator/UI/Middleware/MainUIStateAPI.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/ChooseElLayer.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/DrawableThing.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/ImportStationsFromCSVPopup.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterHover.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterSharedState.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterUILayer.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterUILayerHost.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/Select2MeshPointsLayer.hpp>
#include <OpenSimCreator/UI/Tabs/ModelEditorTab.hpp>
#include <OpenSimCreator/UI/Widgets/MainMenu.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Bindings/ImGuizmoHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Formats/OBJ.hpp>
#include <oscar/Formats/STL.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/UI/Panels/PerfPanel.hpp>
#include <oscar/UI/Panels/UndoRedoPanel.hpp>
#include <oscar/UI/Widgets/LogViewer.hpp>
#include <oscar/UI/Widgets/PopupManager.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/ScopeGuard.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

// mesh importer tab implementation
class osc::MeshImporterTab::Impl final : public MeshImporterUILayerHost {
public:
    explicit Impl(ParentPtr<MainUIStateAPI> const& parent_) :
        m_Parent{parent_},
        m_Shared{std::make_shared<MeshImporterSharedState>()}
    {
    }

    Impl(
        ParentPtr<MainUIStateAPI> const& parent_,
        std::vector<std::filesystem::path> meshPaths_) :

        m_Parent{parent_},
        m_Shared{std::make_shared<MeshImporterSharedState>(std::move(meshPaths_))}
    {
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    bool isUnsaved() const
    {
        return !m_Shared->isModelGraphUpToDateWithDisk();
    }

    bool trySave()
    {
        if (m_Shared->isModelGraphUpToDateWithDisk())
        {
            // nothing to save
            return true;
        }
        else
        {
            // try to save the changes
            return m_Shared->exportAsModelGraphAsOsimFile();
        }
    }

    void onMount()
    {
        App::upd().makeMainEventLoopWaiting();
        m_PopupManager.onMount();
    }

    void onUnmount()
    {
        App::upd().makeMainEventLoopPolling();
    }

    bool onEvent(SDL_Event const& e)
    {
        if (m_Shared->onEvent(e))
        {
            return true;
        }

        if (m_Maybe3DViewerModal)
        {
            std::shared_ptr<MeshImporterUILayer> ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
            if (ptr->onEvent(e))
            {
                return true;
            }
        }

        return false;
    }

    void onTick()
    {
        auto const dt = static_cast<float>(App::get().getFrameDeltaSinceLastFrame().count());

        m_Shared->tick(dt);

        if (m_Maybe3DViewerModal)
        {
            std::shared_ptr<MeshImporterUILayer> ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
            ptr->tick(dt);
        }

        // if some screen generated an OpenSim::Model, transition to the main editor
        if (m_Shared->hasOutputModel())
        {
            auto ptr = std::make_unique<UndoableModelStatePair>(std::move(m_Shared->updOutputModel()));
            ptr->setFixupScaleFactor(m_Shared->getSceneScaleFactor());
            m_Parent->addAndSelectTab<ModelEditorTab>(m_Parent, std::move(ptr));
        }

        m_Name = m_Shared->getRecommendedTitle();

        if (m_Shared->isCloseRequested())
        {
            m_Parent->closeTab(m_TabID);
            m_Shared->resetRequestClose();
        }

        if (m_Shared->isNewMeshImpoterTabRequested())
        {
            m_Parent->addAndSelectTab<MeshImporterTab>(m_Parent);
            m_Shared->resetRequestNewMeshImporter();
        }
    }

    void drawMainMenu()
    {
        drawMainMenuFileMenu();
        drawMainMenuEditMenu();
        drawMainMenuWindowMenu();
        drawMainMenuAboutMenu();
    }

    void onDraw()
    {
        // enable panel docking
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // handle keyboards using ImGui's input poller
        if (!m_Maybe3DViewerModal)
        {
            updateFromImGuiKeyboardState();
        }

        if (!m_Maybe3DViewerModal && m_Shared->isRenderHovered() && !ImGuizmo::IsUsing())
        {
            UpdatePolarCameraFromImGuiMouseInputs(m_Shared->updCamera(), m_Shared->get3DSceneDims());
        }

        // draw history panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex_History))
        {
            bool v = true;
            if (ImGui::Begin("history", &v))
            {
                drawHistoryPanelContent();
            }
            ImGui::End();

            m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex_History, v);
        }

        // draw navigator panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex_Navigator))
        {
            bool v = true;
            if (ImGui::Begin("navigator", &v))
            {
                drawNavigatorPanelContent();
            }
            ImGui::End();

            m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex_Navigator, v);
        }

        // draw log panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex_Log))
        {
            bool v = true;
            if (ImGui::Begin("Log", &v, ImGuiWindowFlags_MenuBar))
            {
                m_Shared->updLogViewer().onDraw();
            }
            ImGui::End();

            m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex_Log, v);
        }

        // draw performance panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex_Performance))
        {
            osc::PerfPanel& pp = m_Shared->updPerfPanel();

            pp.open();
            pp.onDraw();
            if (!pp.isOpen())
            {
                m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex_Performance, false);
            }
        }

        // draw contextual 3D modal (if there is one), else: draw standard 3D viewer
        drawMainViewerPanelOrModal();

        // draw any active popups over the scene
        m_PopupManager.onDraw();
    }

private:

    //
    // ACTIONS
    //

    // pop the current UI layer
    void implRequestPop(MeshImporterUILayer&) final
    {
        m_Maybe3DViewerModal.reset();
        App::upd().requestRedraw();
    }

    // try to select *only* what is currently hovered
    void selectJustHover()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        m_Shared->updModelGraph().select(m_MaybeHover.ID);
    }

    // try to select what is currently hovered *and* anything that is "grouped"
    // with the hovered item
    //
    // "grouped" here specifically means other meshes connected to the same body
    void selectAnythingGroupedWithHover()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        SelectAnythingGroupedWith(m_Shared->updModelGraph(), m_MaybeHover.ID);
    }

    // add a body element to whatever's currently hovered at the hover (raycast) position
    void tryAddBodyToHoveredElement()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        AddBody(m_Shared->updCommittableModelGraph(), m_MaybeHover.Pos, {m_MaybeHover.ID});
    }

    void tryCreatingJointFromHoveredElement()
    {
        if (!m_MaybeHover)
        {
            return;  // nothing hovered
        }

        ModelGraph const& mg = m_Shared->getModelGraph();

        SceneEl const* hoveredSceneEl = mg.tryGetElByID(m_MaybeHover.ID);

        if (!hoveredSceneEl)
        {
            return;  // current hover isn't in the current model graph
        }

        UID maybeID = GetStationAttachmentParent(mg, *hoveredSceneEl);

        if (maybeID == ModelGraphIDs::Ground() || maybeID == ModelGraphIDs::Empty())
        {
            return;  // can't attach to it as-if it were a body
        }

        auto const* bodyEl = mg.tryGetElByID<BodyEl>(maybeID);
        if (!bodyEl)
        {
            return;  // suggested attachment parent isn't in the current model graph?
        }

        transitionToChoosingJointParent(*bodyEl);
    }

    // try transitioning the shown UI layer to one where the user is assigning a mesh
    void tryTransitionToAssigningHoverAndSelectionNextFrame()
    {
        ModelGraph const& mg = m_Shared->getModelGraph();

        std::unordered_set<UID> meshes;
        meshes.insert(mg.getSelected().begin(), mg.getSelected().end());
        if (m_MaybeHover)
        {
            meshes.insert(m_MaybeHover.ID);
        }

        std::erase_if(meshes, [&mg](UID meshID) { return !mg.containsEl<MeshEl>(meshID); });

        if (meshes.empty())
        {
            return;  // nothing to assign
        }

        std::unordered_set<UID> attachments;
        for (UID meshID : meshes)
        {
            attachments.insert(mg.getElByID<MeshEl>(meshID).getParentID());
        }

        transitionToAssigningMeshesNextFrame(meshes, attachments);
    }

    void tryAddingStationAtMousePosToHoveredElement()
    {
        if (!m_MaybeHover)
        {
            return;
        }

        AddStationAtLocation(m_Shared->updCommittableModelGraph(), m_MaybeHover.ID, m_MaybeHover.Pos);
    }

    //
    // TRANSITIONS
    //
    // methods for transitioning the main 3D UI to some other state
    //

    // transition the shown UI layer to one where the user is assigning a mesh
    void transitionToAssigningMeshesNextFrame(std::unordered_set<UID> const& meshes, std::unordered_set<UID> const& existingAttachments)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = false;
        opts.maybeElsAttachingTo = meshes;
        opts.isAttachingTowardEl = false;
        opts.maybeElsBeingReplacedByChoice = existingAttachments;
        opts.header = "choose mesh attachment (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, meshes](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryAssignMeshAttachments(shared->updCommittableModelGraph(), meshes, choices.front());
        };

        // request a state transition
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing a joint parent
    void transitionToChoosingJointParent(BodyEl const& child)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = false;
        opts.header = "choose joint parent (ESC to cancel)";
        opts.maybeElsAttachingTo = {child.getID()};
        opts.isAttachingTowardEl = false;  // away from the body
        opts.onUserChoice = [shared = m_Shared, childID = child.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryCreateJoint(shared->updCommittableModelGraph(), childID, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing which element in the scene to point
    // an element's axis towards
    void transitionToChoosingWhichElementToPointAxisTowards(SceneEl& el, int axis)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.canChooseStations = true;
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose what to point towards (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID(), axis](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return PointAxisTowards(shared->updCommittableModelGraph(), id, axis, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two elements that the given axis
    // should be aligned along (i.e. the direction vector from the first element to the second element
    // becomes the direction vector of the given axis)
    void transitionToChoosingTwoElementsToAlignAxisAlong(SceneEl& el, int axis)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.canChooseStations = true;
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose two elements to align the axis along (ESC to cancel)";
        opts.numElementsUserMustChoose = 2;
        opts.onUserChoice = [shared = m_Shared, id = el.getID(), axis](std::span<UID> choices)
        {
            if (choices.size() < 2)
            {
                return false;
            }

            return TryOrientElementAxisAlongTwoElements(
                shared->updCommittableModelGraph(),
                id,
                axis,
                choices[0],
                choices[1]
            );
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToChoosingWhichElementToTranslateTo(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.canChooseStations = true;
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose what to translate to (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateElementToAnotherElement(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToChoosingElementsToTranslateBetween(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = false;
        opts.canChooseStations = true;
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose two elements to translate between (ESC to cancel)";
        opts.numElementsUserMustChoose = 2;
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.size() < 2)
            {
                return false;
            }

            return TryTranslateBetweenTwoElements(
                shared->updCommittableModelGraph(),
                id,
                choices[0],
                choices[1]
            );
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToCopyingSomethingElsesOrientation(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = true;
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose which orientation to copy (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryCopyOrientation(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element should be oriented along
    void transitionToOrientingElementAlongTwoMeshPoints(SceneEl& el, int axis)
    {
        Select2MeshPointsOptions opts;
        opts.onTwoPointsChosen = [shared = m_Shared, id = el.getID(), axis](Vec3 a, Vec3 b)
        {
            return TryOrientElementAxisAlongTwoPoints(shared->updCommittableModelGraph(), id, axis, a, b);
        };
        m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element sould be translated to the midpoint of
    void transitionToTranslatingElementAlongTwoMeshPoints(SceneEl& el)
    {
        Select2MeshPointsOptions opts;
        opts.onTwoPointsChosen = [shared = m_Shared, id = el.getID()](Vec3 a, Vec3 b)
        {
            return TryTranslateElementBetweenTwoPoints(shared->updCommittableModelGraph(), id, a, b);
        };
        m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
    }

    void transitionToTranslatingElementToMeshAverageCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = false;
        opts.canChooseGround = false;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateToMeshAverageCenter(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToTranslatingElementToMeshBoundsCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = false;
        opts.canChooseGround = false;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateToMeshBoundsCenter(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToTranslatingElementToMeshMassCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = false;
        opts.canChooseGround = false;
        opts.canChooseJoints = false;
        opts.canChooseMeshes = true;
        opts.header = "choose a mesh (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateToMeshMassCenter(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing another element that
    // the element should be translated to the midpoint of
    void transitionToTranslatingElementToAnotherElementsCenter(SceneEl& el)
    {
        ChooseElLayerOptions opts;
        opts.canChooseBodies = true;
        opts.canChooseGround = true;
        opts.canChooseJoints = true;
        opts.canChooseMeshes = true;
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose where to place it (ESC to cancel)";
        opts.onUserChoice = [shared = m_Shared, id = el.getID()](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }

            return TryTranslateElementToAnotherElement(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToReassigningCrossRef(SceneEl& el, int crossrefIdx)
    {
        int nRefs = el.getNumCrossReferences();

        if (crossrefIdx < 0 || crossrefIdx >= nRefs)
        {
            return;  // invalid index?
        }

        SceneEl const* old = m_Shared->getModelGraph().tryGetElByID(el.getCrossReferenceConnecteeID(crossrefIdx));

        if (!old)
        {
            return;  // old el doesn't exist?
        }

        ChooseElLayerOptions opts;
        opts.canChooseBodies = dynamic_cast<BodyEl const*>(old) || dynamic_cast<GroundEl const*>(old);
        opts.canChooseGround = dynamic_cast<BodyEl const*>(old) || dynamic_cast<GroundEl const*>(old);
        opts.canChooseJoints = dynamic_cast<JointEl const*>(old);
        opts.canChooseMeshes = dynamic_cast<MeshEl const*>(old);
        opts.maybeElsAttachingTo = {el.getID()};
        opts.header = "choose what to attach to";
        opts.onUserChoice = [shared = m_Shared, id = el.getID(), crossrefIdx](std::span<UID> choices)
        {
            if (choices.empty())
            {
                return false;
            }
            return TryReassignCrossref(shared->updCommittableModelGraph(), id, crossrefIdx, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // ensure any stale references into the modelgrah are cleaned up
    void garbageCollectStaleRefs()
    {
        ModelGraph const& mg = m_Shared->getModelGraph();

        if (m_MaybeHover && !mg.containsEl(m_MaybeHover.ID))
        {
            m_MaybeHover.reset();
        }

        if (m_MaybeOpenedContextMenu && !mg.containsEl(m_MaybeOpenedContextMenu.ID))
        {
            m_MaybeOpenedContextMenu.reset();
        }
    }

    // delete currently-selected scene elements
    void deleteSelected()
    {
        DeleteSelected(m_Shared->updCommittableModelGraph());
        garbageCollectStaleRefs();
    }

    // delete a particular scene element
    void deleteEl(UID elID)
    {
        DeleteEl(m_Shared->updCommittableModelGraph(), elID);
        garbageCollectStaleRefs();
    }

    // update this scene from the current keyboard state, as saved by ImGui
    bool updateFromImGuiKeyboardState()
    {
        if (ImGui::GetIO().WantCaptureKeyboard)
        {
            return false;
        }

        bool shiftDown = osc::IsShiftDown();
        bool ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

        if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_N))
        {
            // Ctrl+N: new scene
            m_Shared->requestNewMeshImporterTab();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_O))
        {
            // Ctrl+O: open osim
            m_Shared->openOsimFileAsModelGraph();
            return true;
        }
        else if (ctrlOrSuperDown && shiftDown && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            // Ctrl+Shift+S: export as: export scene as osim to user-specified location
            m_Shared->exportAsModelGraphAsOsimFile();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            // Ctrl+S: export: export scene as osim according to typical export heuristic
            m_Shared->exportModelGraphAsOsimFile();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_W))
        {
            // Ctrl+W: close
            m_Shared->requestClose();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_Q))
        {
            // Ctrl+Q: quit application
            App::upd().requestQuit();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_A))
        {
            // Ctrl+A: select all
            m_Shared->selectAll();
            return true;
        }
        else if (ctrlOrSuperDown && shiftDown && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            // Ctrl+Shift+Z: redo
            m_Shared->redoCurrentModelGraph();
            return true;
        }
        else if (ctrlOrSuperDown && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            // Ctrl+Z: undo
            m_Shared->undoCurrentModelGraph();
            return true;
        }
        else if (osc::IsAnyKeyDown({ImGuiKey_Delete, ImGuiKey_Backspace}))
        {
            // Delete/Backspace: delete any selected elements
            deleteSelected();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_B))
        {
            // B: add body to hovered element
            tryAddBodyToHoveredElement();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_A))
        {
            // A: assign a parent for the hovered element
            tryTransitionToAssigningHoverAndSelectionNextFrame();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_J))
        {
            // J: try to create a joint
            tryCreatingJointFromHoveredElement();
            return true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_T))
        {
            // T: try to add a station to the current hover
            tryAddingStationAtMousePosToHoveredElement();
            return true;
        }
        else if (UpdateImguizmoStateFromKeyboard(m_ImGuizmoState.op, m_ImGuizmoState.mode))
        {
            return true;
        }
        else if (UpdatePolarCameraFromImGuiKeyboardInputs(m_Shared->updCamera(), m_Shared->get3DSceneRect(), calcSceneAABB()))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void drawNothingContextMenuContentHeader()
    {
        ImGui::Text(ICON_FA_BOLT " Actions");
        ImGui::SameLine();
        ImGui::TextDisabled("(nothing clicked)");
        ImGui::Separator();
    }

    void drawSceneElContextMenuContentHeader(SceneEl const& e)
    {
        ImGui::Text("%s %s", e.getClass().getIconUTF8().c_str(), e.getLabel().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", GetContextMenuSubHeaderText(m_Shared->getModelGraph(), e).c_str());
        ImGui::SameLine();
        osc::DrawHelpMarker(e.getClass().getName(), e.getClass().getDescription());
        ImGui::Separator();
    }

    void drawSceneElPropEditors(SceneEl const& e)
    {
        ModelGraph& mg = m_Shared->updModelGraph();

        // label/name editor
        if (e.canChangeLabel())
        {
            std::string buf{static_cast<std::string_view>(e.getLabel())};
            if (osc::InputString("Name", buf))
            {
                mg.updElByID(e.getID()).setLabel(buf);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getClass().getName() << " name";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Component Name", "This is the name that the component will have in the exported OpenSim model.");
        }

        // position editor
        if (e.canChangePosition())
        {
            Vec3 translation = e.getPos(mg);
            if (ImGui::InputFloat3("Translation", osc::ValuePtr(translation), "%.6f"))
            {
                mg.updElByID(e.getID()).setPos(mg, translation);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s translation";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Translation", ModelGraphStrings::c_TranslationDescription);
        }

        // rotation editor
        if (e.canChangeRotation())
        {
            Vec3 eulerDegs = osc::Rad2Deg(osc::EulerAngles(e.getRotation(m_Shared->getModelGraph())));

            if (ImGui::InputFloat3("Rotation (deg)", osc::ValuePtr(eulerDegs), "%.6f"))
            {
                Quat quatRads = Quat{osc::Deg2Rad(eulerDegs)};
                mg.updElByID(e.getID()).setRotation(mg, quatRads);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s rotation";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Rotation", "These are the rotation Euler angles for the component in ground. Positive rotations are anti-clockwise along that axis.\n\nNote: the numbers may contain slight rounding error, due to backend constraints. Your values *should* be accurate to a few decimal places.");
        }

        // scale factor editor
        if (e.canChangeScale())
        {
            Vec3 scaleFactors = e.getScale(mg);
            if (ImGui::InputFloat3("Scale", osc::ValuePtr(scaleFactors), "%.6f"))
            {
                mg.updElByID(e.getID()).setScale(mg, scaleFactors);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s scale";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            osc::DrawHelpMarker("Scale", "These are the scale factors of the component in ground. These scale-factors are applied to the element before any other transform (it scales first, then rotates, then translates).");
        }
    }

    // draw content of "Add" menu for some scene element
    void drawAddOtherToSceneElActions(SceneEl& el, Vec3 const& clickPos)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});
        ScopeGuard const g1{[]() { ImGui::PopStyleVar(); }};

        int imguiID = 0;
        ImGui::PushID(imguiID++);
        ScopeGuard const g2{[]() { ImGui::PopID(); }};

        if (CanAttachMeshTo(el))
        {
            if (ImGui::MenuItem(ICON_FA_CUBE " Meshes"))
            {
                m_Shared->pushMeshLoadRequests(el.getID(), m_Shared->promptUserForMeshFiles());
            }
            osc::DrawTooltipIfItemHovered("Add Meshes", ModelGraphStrings::c_MeshDescription);
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (el.hasPhysicalSize())
        {
            if (ImGui::BeginMenu(ICON_FA_CIRCLE " Body"))
            {
                if (ImGui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                {
                    AddBody(m_Shared->updCommittableModelGraph(), el.getPos(m_Shared->getModelGraph()), el.getID());
                }
                osc::DrawTooltipIfItemHovered("Add Body", ModelGraphStrings::c_BodyDescription.c_str());

                if (ImGui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                {
                    AddBody(m_Shared->updCommittableModelGraph(), clickPos, el.getID());
                }
                osc::DrawTooltipIfItemHovered("Add Body", ModelGraphStrings::c_BodyDescription.c_str());

                if (ImGui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                {
                    AddBody(m_Shared->updCommittableModelGraph());
                }
                osc::DrawTooltipIfItemHovered("Add body", ModelGraphStrings::c_BodyDescription.c_str());

                if (auto const* meshEl = dynamic_cast<MeshEl const*>(&el))
                {
                    if (ImGui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                    {
                        Vec3 const location = Midpoint(meshEl->calcBounds());
                        AddBody(m_Shared->updCommittableModelGraph(), location, meshEl->getID());
                    }
                    osc::DrawTooltipIfItemHovered("Add Body", ModelGraphStrings::c_BodyDescription.c_str());

                    if (ImGui::MenuItem(ICON_FA_DIVIDE " at mesh average center"))
                    {
                        Vec3 const location = AverageCenter(*meshEl);
                        AddBody(m_Shared->updCommittableModelGraph(), location, meshEl->getID());
                    }
                    osc::DrawTooltipIfItemHovered("Add Body", ModelGraphStrings::c_BodyDescription.c_str());

                    if (ImGui::MenuItem(ICON_FA_WEIGHT " at mesh mass center"))
                    {
                        Vec3 const location = MassCenter(*meshEl);
                        AddBody(m_Shared->updCommittableModelGraph(), location, meshEl->getID());
                    }
                    osc::DrawTooltipIfItemHovered("Add body", ModelGraphStrings::c_BodyDescription.c_str());
                }

                ImGui::EndMenu();
            }
        }
        else
        {
            if (ImGui::MenuItem(ICON_FA_CIRCLE " Body"))
            {
                AddBody(m_Shared->updCommittableModelGraph(), el.getPos(m_Shared->getModelGraph()), el.getID());
            }
            osc::DrawTooltipIfItemHovered("Add Body", ModelGraphStrings::c_BodyDescription.c_str());
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (auto const* body = dynamic_cast<BodyEl const*>(&el))
        {
            if (ImGui::MenuItem(ICON_FA_LINK " Joint"))
            {
                transitionToChoosingJointParent(*body);
            }
            osc::DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (CanAttachStationTo(el))
        {
            if (el.hasPhysicalSize())
            {
                if (ImGui::BeginMenu(ICON_FA_MAP_PIN " Station"))
                {
                    if (ImGui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, el.getPos(m_Shared->getModelGraph()));
                    }
                    osc::DrawTooltipIfItemHovered("Add Station", ModelGraphStrings::c_StationDescription);

                    if (ImGui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, clickPos);
                    }
                    osc::DrawTooltipIfItemHovered("Add Station", ModelGraphStrings::c_StationDescription);

                    if (ImGui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, Vec3{});
                    }
                    osc::DrawTooltipIfItemHovered("Add Station", ModelGraphStrings::c_StationDescription);

                    if (dynamic_cast<MeshEl const*>(&el))
                    {
                        if (ImGui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                        {
                            AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, Midpoint(el.calcBounds(m_Shared->getModelGraph())));
                        }
                        osc::DrawTooltipIfItemHovered("Add Station", ModelGraphStrings::c_StationDescription);
                    }

                    ImGui::EndMenu();
                }
            }
            else
            {
                if (ImGui::MenuItem(ICON_FA_MAP_PIN " Station"))
                {
                    AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, el.getPos(m_Shared->getModelGraph()));
                }
                osc::DrawTooltipIfItemHovered("Add Station", ModelGraphStrings::c_StationDescription);
            }
        }
        // ~ScopeGuard: implicitly calls ImGui::PopID()
    }

    void drawNothingActions()
    {
        if (ImGui::MenuItem(ICON_FA_CUBE " Add Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        osc::DrawTooltipIfItemHovered("Add Meshes to the model", ModelGraphStrings::c_MeshDescription);

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add Other"))
        {
            drawAddOtherMenuItems();

            ImGui::EndMenu();
        }
    }

    void drawSceneElActions(SceneEl& el, Vec3 const& clickPos)
    {
        if (ImGui::MenuItem(ICON_FA_CAMERA " Focus camera on this"))
        {
            m_Shared->focusCameraOn(Midpoint(el.calcBounds(m_Shared->getModelGraph())));
        }
        osc::DrawTooltipIfItemHovered("Focus camera on this scene element", "Focuses the scene camera on this element. This is useful for tracking the camera around that particular object in the scene");

        if (ImGui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            drawAddOtherToSceneElActions(el, clickPos);
            ImGui::EndMenu();
        }

        if (auto const* body = dynamic_cast<BodyEl const*>(&el))
        {
            if (ImGui::MenuItem(ICON_FA_LINK " Join to"))
            {
                transitionToChoosingJointParent(*body);
            }
            osc::DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }

        if (el.canDelete())
        {
            if (ImGui::MenuItem(ICON_FA_TRASH " Delete"))
            {
                DeleteEl(m_Shared->updCommittableModelGraph(), el.getID());
                garbageCollectStaleRefs();
                ImGui::CloseCurrentPopup();
            }
            osc::DrawTooltipIfItemHovered("Delete", "Deletes the component from the model. Deletion is undo-able (use the undo/redo feature). Anything attached to this element (e.g. joints, meshes) will also be deleted.");
        }
    }

    // draw the "Translate" menu for any generic `SceneEl`
    void drawTranslateMenu(SceneEl& el)
    {
        if (!el.canChangePosition())
        {
            return;  // can't change its position
        }

        if (!ImGui::BeginMenu(ICON_FA_ARROWS_ALT " Translate"))
        {
            return;  // top-level menu isn't open
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
        {
            std::string label = "To " + el.getCrossReferenceLabel(i);
            if (ImGui::MenuItem(label.c_str()))
            {
                TryTranslateElementToAnotherElement(m_Shared->updCommittableModelGraph(), el.getID(), el.getCrossReferenceConnecteeID(i));
            }
        }

        if (ImGui::MenuItem("To (select something)"))
        {
            transitionToChoosingWhichElementToTranslateTo(el);
        }

        if (el.getNumCrossReferences() == 2)
        {
            std::string label = "Between " + el.getCrossReferenceLabel(0) + " and " + el.getCrossReferenceLabel(1);
            if (ImGui::MenuItem(label.c_str()))
            {
                UID a = el.getCrossReferenceConnecteeID(0);
                UID b = el.getCrossReferenceConnecteeID(1);
                TryTranslateBetweenTwoElements(m_Shared->updCommittableModelGraph(), el.getID(), a, b);
            }
        }

        if (ImGui::MenuItem("Between two scene elements"))
        {
            transitionToChoosingElementsToTranslateBetween(el);
        }

        if (ImGui::MenuItem("Between two mesh points"))
        {
            transitionToTranslatingElementAlongTwoMeshPoints(el);
        }

        if (ImGui::MenuItem("To mesh bounds center"))
        {
            transitionToTranslatingElementToMeshBoundsCenter(el);
        }
        osc::DrawTooltipIfItemHovered("Translate to mesh bounds center", "Translates the given element to the center of the selected mesh's bounding box. The bounding box is the smallest box that contains all mesh vertices");

        if (ImGui::MenuItem("To mesh average center"))
        {
            transitionToTranslatingElementToMeshAverageCenter(el);
        }
        osc::DrawTooltipIfItemHovered("Translate to mesh average center", "Translates the given element to the average center point of vertices in the selected mesh.\n\nEffectively, this adds each vertex location in the mesh, divides the sum by the number of vertices in the mesh, and sets the translation of the given object to that location.");

        if (ImGui::MenuItem("To mesh mass center"))
        {
            transitionToTranslatingElementToMeshMassCenter(el);
        }
        osc::DrawTooltipIfItemHovered("Translate to mesh mess center", "Translates the given element to the mass center of the selected mesh.\n\nCAREFUL: the algorithm used to do this heavily relies on your triangle winding (i.e. normals) being correct and your mesh being a closed surface. If your mesh doesn't meet these requirements, you might get strange results (apologies: the only way to get around that problems involves complicated voxelization and leak-detection algorithms :( )");

        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    // draw the "Reorient" menu for any generic `SceneEl`
    void drawReorientMenu(SceneEl& el)
    {
        if (!el.canChangeRotation())
        {
            return;  // can't change its rotation
        }

        if (!ImGui::BeginMenu(ICON_FA_REDO " Reorient"))
        {
            return;  // top-level menu isn't open
        }
        osc::DrawTooltipIfItemHovered("Reorient the scene element", "Rotates the scene element in without changing its position");

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        {
            auto DrawMenuContent = [&](int axis)
            {
                for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
                {
                    std::string label = "Towards " + el.getCrossReferenceLabel(i);

                    if (ImGui::MenuItem(label.c_str()))
                    {
                        PointAxisTowards(m_Shared->updCommittableModelGraph(), el.getID(), axis, el.getCrossReferenceConnecteeID(i));
                    }
                }

                if (ImGui::MenuItem("Towards (select something)"))
                {
                    transitionToChoosingWhichElementToPointAxisTowards(el, axis);
                }

                if (ImGui::MenuItem("Along line between (select two elements)"))
                {
                    transitionToChoosingTwoElementsToAlignAxisAlong(el, axis);
                }

                if (ImGui::MenuItem("90 degress"))
                {
                    RotateAxisXRadians(m_Shared->updCommittableModelGraph(), el, axis, std::numbers::pi_v<float>/2.0f);
                }

                if (ImGui::MenuItem("180 degrees"))
                {
                    RotateAxisXRadians(m_Shared->updCommittableModelGraph(), el, axis, std::numbers::pi_v<float>);
                }

                if (ImGui::MenuItem("Along two mesh points"))
                {
                    transitionToOrientingElementAlongTwoMeshPoints(el, axis);
                }
            };

            if (ImGui::BeginMenu("x"))
            {
                DrawMenuContent(0);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("y"))
            {
                DrawMenuContent(1);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("z"))
            {
                DrawMenuContent(2);
                ImGui::EndMenu();
            }
        }

        if (ImGui::MenuItem("copy"))
        {
            transitionToCopyingSomethingElsesOrientation(el);
        }

        if (ImGui::MenuItem("reset"))
        {
            el.setXform(m_Shared->getModelGraph(), Transform{.position = el.getPos(m_Shared->getModelGraph())});
            m_Shared->commitCurrentModelGraph("reset " + el.getLabel() + " orientation");
        }

        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    // draw the "Mass" editor for a `BodyEl`
    void drawMassEditor(BodyEl const& bodyEl)
    {
        auto curMass = static_cast<float>(bodyEl.getMass());
        if (ImGui::InputFloat("Mass", &curMass, 0.0f, 0.0f, "%.6f"))
        {
            m_Shared->updModelGraph().updElByID<BodyEl>(bodyEl.getID()).setMass(static_cast<double>(curMass));
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            m_Shared->commitCurrentModelGraph("changed body mass");
        }
        ImGui::SameLine();
        osc::DrawHelpMarker("Mass", "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
    }

    // draw the "Joint Type" editor for a `JointEl`
    void drawJointTypeEditor(JointEl const& jointEl)
    {
        size_t currentIdx = jointEl.getJointTypeIndex();
        auto const& registry = osc::GetComponentRegistry<OpenSim::Joint>();
        auto const nameAccessor = [&registry](size_t i) { return registry[i].name(); };

        if (osc::Combo("Joint Type", &currentIdx, registry.size(), nameAccessor))
        {
            m_Shared->updModelGraph().updElByID<JointEl>(jointEl.getID()).setJointTypeIndex(currentIdx);
            m_Shared->commitCurrentModelGraph("changed joint type");
        }
        ImGui::SameLine();
        osc::DrawHelpMarker("Joint Type", "This is the type of joint that should be added into the OpenSim model. The joint's type dictates what types of motion are permitted around the joint center. See the official OpenSim documentation for an explanation of each joint type.");
    }

    // draw the "Reassign Connection" menu, which lets users change an element's cross reference
    void drawReassignCrossrefMenu(SceneEl& el)
    {
        int nRefs = el.getNumCrossReferences();

        if (nRefs == 0)
        {
            return;
        }

        if (ImGui::BeginMenu(ICON_FA_EXTERNAL_LINK_ALT " Reassign Connection"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

            for (int i = 0; i < nRefs; ++i)
            {
                CStringView label = el.getCrossReferenceLabel(i);
                if (ImGui::MenuItem(label.c_str()))
                {
                    transitionToReassigningCrossRef(el, i);
                }
            }

            ImGui::PopStyleVar();
            ImGui::EndMenu();
        }
    }

    void actionPromptUserToSaveMeshAsOBJ(
        Mesh const& mesh)
    {
        // prompt user for a save location
        std::optional<std::filesystem::path> const maybeUserSaveLocation =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("obj");
        if (!maybeUserSaveLocation)
        {
            return;  // user didn't select a save location
        }
        std::filesystem::path const& userSaveLocation = *maybeUserSaveLocation;

        // write transformed mesh to output
        std::ofstream outputFileStream
        {
            userSaveLocation,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary,
        };
        if (!outputFileStream)
        {
            std::string const error = osc::CurrentErrnoAsString();
            osc::log::error("%s: could not save obj output: %s", userSaveLocation.string().c_str(), error.c_str());
            return;
        }

        AppMetadata const& appMetadata = App::get().getMetadata();
        ObjMetadata const objMetadata
        {
            osc::CalcFullApplicationNameWithVersionAndBuild(appMetadata),
        };

        osc::WriteMeshAsObj(
            outputFileStream,
            mesh,
            objMetadata,
            ObjWriterFlags::NoWriteNormals
        );
    }

    void actionPromptUserToSaveMeshAsSTL(
        Mesh const& mesh)
    {
        // prompt user for a save location
        std::optional<std::filesystem::path> const maybeUserSaveLocation =
            osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("stl");
        if (!maybeUserSaveLocation)
        {
            return;  // user didn't select a save location
        }
        std::filesystem::path const& userSaveLocation = *maybeUserSaveLocation;

        // write transformed mesh to output
        std::ofstream outputFileStream
        {
            userSaveLocation,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary,
        };
        if (!outputFileStream)
        {
            std::string const error = osc::CurrentErrnoAsString();
            osc::log::error("%s: could not save obj output: %s", userSaveLocation.string().c_str(), error.c_str());
            return;
        }

        AppMetadata const& appMetadata = App::get().getMetadata();
        StlMetadata const stlMetadata
        {
            osc::CalcFullApplicationNameWithVersionAndBuild(appMetadata),
        };

        osc::WriteMeshAsStl(outputFileStream, mesh, stlMetadata);
    }

    void drawSaveMeshMenu(MeshEl const& el)
    {
        if (ImGui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
        {
            ImGui::TextDisabled("With Respect to:");
            ImGui::Separator();
            for (SceneEl const& sceneEl : m_Shared->getModelGraph().iter())
            {
                if (ImGui::BeginMenu(sceneEl.getLabel().c_str()))
                {
                    ImGui::TextDisabled("Format:");
                    ImGui::Separator();

                    if (ImGui::MenuItem(".obj"))
                    {
                        Transform const sceneElToGround = sceneEl.getXForm(m_Shared->getModelGraph());
                        Transform const meshVertToGround = el.getXForm();
                        Mat4 const meshVertToSceneElVert = osc::ToInverseMat4(sceneElToGround) * osc::ToMat4(meshVertToGround);

                        Mesh mesh = el.getMeshData();
                        mesh.transformVerts(meshVertToSceneElVert);
                        actionPromptUserToSaveMeshAsOBJ(mesh);
                    }

                    if (ImGui::MenuItem(".stl"))
                    {
                        Transform const sceneElToGround = sceneEl.getXForm(m_Shared->getModelGraph());
                        Transform const meshVertToGround = el.getXForm();
                        Mat4 const meshVertToSceneElVert = osc::ToInverseMat4(sceneElToGround) * osc::ToMat4(meshVertToGround);

                        Mesh mesh = el.getMeshData();
                        mesh.transformVerts(meshVertToSceneElVert);
                        actionPromptUserToSaveMeshAsSTL(mesh);
                    }

                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }
    }

    // draw context menu content for when user right-clicks nothing
    void drawNothingContextMenuContent()
    {
        drawNothingContextMenuContentHeader();
        ImGui::Dummy({0.0f, 5.0f});
        drawNothingActions();
    }

    // draw context menu content for a `GroundEl`
    void drawContextMenuContent(GroundEl& el, Vec3 const& clickPos)
    {
        drawSceneElContextMenuContentHeader(el);
        ImGui::Dummy({0.0f, 5.0f});
        drawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `BodyEl`
    void drawContextMenuContent(BodyEl& el, Vec3 const& clickPos)
    {
        drawSceneElContextMenuContentHeader(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawSceneElPropEditors(el);
        drawMassEditor(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `MeshEl`
    void drawContextMenuContent(MeshEl& el, Vec3 const& clickPos)
    {
        drawSceneElContextMenuContentHeader(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawSceneElPropEditors(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawSaveMeshMenu(el);
        drawReassignCrossrefMenu(el);
        drawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `JointEl`
    void drawContextMenuContent(JointEl& el, Vec3 const& clickPos)
    {
        drawSceneElContextMenuContentHeader(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawSceneElPropEditors(el);
        drawJointTypeEditor(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawSceneElActions(el, clickPos);
    }

    // draw context menu content for a `StationEl`
    void drawContextMenuContent(StationEl& el, Vec3 const& clickPos)
    {
        drawSceneElContextMenuContentHeader(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawSceneElPropEditors(el);

        ImGui::Dummy({0.0f, 5.0f});

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawSceneElActions(el, clickPos);
    }

    // draw context menu content for some scene element
    void drawContextMenuContent(SceneEl& el, Vec3 const& clickPos)
    {
        std::visit(Overload
        {
            [this, &clickPos](GroundEl& el)  { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](MeshEl& el)    { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](BodyEl& el)    { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](JointEl& el)   { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](StationEl& el) { this->drawContextMenuContent(el, clickPos); },
        }, el.toVariant());
    }

    // draw a context menu for the current state (if applicable)
    void drawContextMenuContent()
    {
        if (!m_MaybeOpenedContextMenu)
        {
            // context menu not open, but just draw the "nothing" menu
            PushID(UID::empty());
            ScopeGuard const g{[]() { ImGui::PopID(); }};
            drawNothingContextMenuContent();
        }
        else if (m_MaybeOpenedContextMenu.ID == ModelGraphIDs::RightClickedNothing())
        {
            // context menu was opened on "nothing" specifically
            PushID(UID::empty());
            ScopeGuard const g{[]() { ImGui::PopID(); }};
            drawNothingContextMenuContent();
        }
        else if (SceneEl* el = m_Shared->updModelGraph().tryUpdElByID(m_MaybeOpenedContextMenu.ID))
        {
            // context menu was opened on a scene element that exists in the modelgraph
            PushID(el->getID());
            ScopeGuard const g{[]() { ImGui::PopID(); }};
            drawContextMenuContent(*el, m_MaybeOpenedContextMenu.Pos);
        }


        // context menu should be closed under these conditions
        if (osc::IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Escape}))
        {
            m_MaybeOpenedContextMenu.reset();
            ImGui::CloseCurrentPopup();
        }
    }

    // draw the content of the (undo/redo) "History" panel
    void drawHistoryPanelContent()
    {
        osc::UndoRedoPanel::DrawContent(m_Shared->updCommittableModelGraph());
    }

    void drawNavigatorElement(SceneElClass const& c)
    {
        ModelGraph& mg = m_Shared->updModelGraph();

        ImGui::Text("%s %s", c.getIconUTF8().c_str(), c.getNamePluralized().c_str());
        ImGui::SameLine();
        osc::DrawHelpMarker(c.getNamePluralized(), c.getDescription());
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::Indent();

        bool empty = true;
        for (SceneEl const& el : mg.iter())
        {
            if (el.getClass() != c)
            {
                continue;
            }

            empty = false;

            UID id = el.getID();
            int styles = 0;

            if (id == m_MaybeHover.ID)
            {
                osc::PushStyleColor(ImGuiCol_Text, Color::yellow());
                ++styles;
            }
            else if (m_Shared->isSelected(id))
            {
                osc::PushStyleColor(ImGuiCol_Text, Color::yellow());
                ++styles;
            }

            ImGui::Text("%s", el.getLabel().c_str());

            ImGui::PopStyleColor(styles);

            if (ImGui::IsItemHovered())
            {
                m_MaybeHover = {id, {}};
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                if (!osc::IsShiftDown())
                {
                    m_Shared->updModelGraph().deSelectAll();
                }
                m_Shared->updModelGraph().select(id);
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                m_MaybeOpenedContextMenu = MeshImporterHover{id, {}};
                ImGui::OpenPopup("##maincontextmenu");
                App::upd().requestRedraw();
            }
        }

        if (empty)
        {
            ImGui::TextDisabled("(no %s)", c.getNamePluralized().c_str());
        }
        ImGui::Unindent();
    }

    void drawNavigatorPanelContent()
    {
        for (SceneElClass const& c : GetSceneElClasses())
        {
            drawNavigatorElement(c);
            ImGui::Dummy({0.0f, 5.0f});
        }

        // a navigator element might have opened the context menu in the navigator panel
        //
        // this can happen when the user right-clicks something in the navigator
        if (ImGui::BeginPopup("##maincontextmenu"))
        {
            drawContextMenuContent();
            ImGui::EndPopup();
        }
    }

    void drawAddOtherMenuItems()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        if (ImGui::MenuItem(ICON_FA_CUBE " Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        osc::DrawTooltipIfItemHovered("Add Meshes", ModelGraphStrings::c_MeshDescription);

        if (ImGui::MenuItem(ICON_FA_CIRCLE " Body"))
        {
            AddBody(m_Shared->updCommittableModelGraph());
        }
        osc::DrawTooltipIfItemHovered("Add Body", ModelGraphStrings::c_BodyDescription);

        if (ImGui::MenuItem(ICON_FA_MAP_PIN " Station"))
        {
            ModelGraph& mg = m_Shared->updModelGraph();
            auto& e = mg.emplaceEl<StationEl>(UID{}, ModelGraphIDs::Ground(), Vec3{}, StationEl::Class().generateName());
            SelectOnly(mg, e);
        }
        osc::DrawTooltipIfItemHovered("Add Station", StationEl::Class().getDescription());

        ImGui::PopStyleVar();
    }

    void draw3DViewerOverlayTopBar()
    {
        int imguiID = 0;

        if (ImGui::Button(ICON_FA_CUBE " Add Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        osc::DrawTooltipIfItemHovered("Add Meshes to the model", ModelGraphStrings::c_MeshDescription);

        ImGui::SameLine();

        ImGui::Button(ICON_FA_PLUS " Add Other");
        osc::DrawTooltipIfItemHovered("Add components to the model");

        if (ImGui::BeginPopupContextItem("##additemtoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            drawAddOtherMenuItems();
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_PAINT_ROLLER " Colors");
        osc::DrawTooltipIfItemHovered("Change scene display colors", "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

        if (ImGui::BeginPopupContextItem("##addpainttoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            std::span<Color const> colors = m_Shared->getColors();
            std::span<char const* const> labels = m_Shared->getColorLabels();
            OSC_ASSERT(colors.size() == labels.size() && "every color should have a label");

            for (size_t i = 0; i < colors.size(); ++i)
            {
                Color colorVal = colors[i];
                ImGui::PushID(imguiID++);
                if (ImGui::ColorEdit4(labels[i], osc::ValuePtr(colorVal)))
                {
                    m_Shared->setColor(i, colorVal);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_EYE " Visibility");
        osc::DrawTooltipIfItemHovered("Change what's visible in the 3D scene", "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

        if (ImGui::BeginPopupContextItem("##changevisibilitypopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            std::span<bool const> visibilities = m_Shared->getVisibilityFlags();
            std::span<char const* const> labels = m_Shared->getVisibilityFlagLabels();
            OSC_ASSERT(visibilities.size() == labels.size() && "every visibility flag should have a label");

            for (size_t i = 0; i < visibilities.size(); ++i)
            {
                bool v = visibilities[i];
                ImGui::PushID(imguiID++);
                if (ImGui::Checkbox(labels[i], &v))
                {
                    m_Shared->setVisibilityFlag(i, v);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_LOCK " Interactivity");
        osc::DrawTooltipIfItemHovered("Change what your mouse can interact with in the 3D scene", "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

        if (ImGui::BeginPopupContextItem("##changeinteractionlockspopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            std::span<bool const> interactables = m_Shared->getIneractivityFlags();
            std::span<char const* const> labels =  m_Shared->getInteractivityFlagLabels();
            OSC_ASSERT(interactables.size() == labels.size());

            for (size_t i = 0; i < interactables.size(); ++i)
            {
                bool v = interactables[i];
                ImGui::PushID(imguiID++);
                if (ImGui::Checkbox(labels[i], &v))
                {
                    m_Shared->setInteractivityFlag(i, v);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        DrawGizmoOpSelector(m_ImGuizmoState.op);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
        ImGui::SameLine();
        ImGui::PopStyleVar();

        // local/global dropdown
        DrawGizmoModeSelector(m_ImGuizmoState.mode);
        ImGui::SameLine();

        // scale factor
        {
            CStringView const tooltipTitle = "Change scene scale factor";
            CStringView const tooltipDesc = "This rescales *some* elements in the scene. Specifically, the ones that have no 'size', such as body frames, joint frames, and the chequered floor texture.\n\nChanging this is handy if you are working on smaller or larger models, where the size of the (decorative) frames and floor are too large/small compared to the model you are working on.\n\nThis is purely decorative and does not affect the exported OpenSim model in any way.";

            float sf = m_Shared->getSceneScaleFactor();
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("1000.00").x);
            if (ImGui::InputFloat("scene scale factor", &sf))
            {
                m_Shared->setSceneScaleFactor(sf);
            }
            osc::DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
        }
    }

    std::optional<AABB> calcSceneAABB() const
    {
        std::optional<AABB> rv;
        for (DrawableThing const& drawable : m_DrawablesBuffer)
        {
            if (drawable.id != ModelGraphIDs::Empty())
            {
                AABB const bounds = calcBounds(drawable);
                rv = rv ? Union(*rv, bounds) : bounds;
            }
        }
        return rv;
    }

    void draw3DViewerOverlayBottomBar()
    {
        ImGui::PushID("##3DViewerOverlay");

        // bottom-left axes overlay
        {
            ImGuiStyle const& style = ImGui::GetStyle();
            Rect const& r = m_Shared->get3DSceneRect();
            Vec2 const topLeft =
            {
                r.p1.x + style.WindowPadding.x,
                r.p2.y - style.WindowPadding.y - CalcAlignmentAxesDimensions().y,
            };
            ImGui::SetCursorScreenPos(topLeft);
            DrawAlignmentAxes(m_Shared->getCamera().getViewMtx());
        }

        Rect sceneRect = m_Shared->get3DSceneRect();
        Vec2 trPos = {sceneRect.p1.x + 100.0f, sceneRect.p2.y - 55.0f};
        ImGui::SetCursorScreenPos(trPos);

        if (ImGui::Button(ICON_FA_SEARCH_MINUS))
        {
            m_Shared->updCamera().radius *= 1.2f;
        }
        osc::DrawTooltipIfItemHovered("Zoom Out");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_SEARCH_PLUS))
        {
            m_Shared->updCamera().radius *= 0.8f;
        }
        osc::DrawTooltipIfItemHovered("Zoom In");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
        {
            if (std::optional<AABB> const sceneAABB = calcSceneAABB())
            {
                osc::AutoFocus(m_Shared->updCamera(), *sceneAABB, osc::AspectRatio(m_Shared->get3DSceneDims()));
            }
        }
        osc::DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");

        ImGui::SameLine();

        if (ImGui::Button("X"))
        {
            m_Shared->updCamera().theta = std::numbers::pi_v<float>/2.0f;
            m_Shared->updCamera().phi = 0.0f;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->updCamera().theta = -std::numbers::pi_v<float>/2.0f;
            m_Shared->updCamera().phi = 0.0f;
        }
        osc::DrawTooltipIfItemHovered("Face camera facing along X", "Right-clicking faces it along X, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button("Y"))
        {
            m_Shared->updCamera().theta = 0.0f;
            m_Shared->updCamera().phi = std::numbers::pi_v<float>/2.0f;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->updCamera().theta = 0.0f;
            m_Shared->updCamera().phi = -std::numbers::pi_v<float>/2.0f;
        }
        osc::DrawTooltipIfItemHovered("Face camera facing along Y", "Right-clicking faces it along Y, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button("Z"))
        {
            m_Shared->updCamera().theta = 0.0f;
            m_Shared->updCamera().phi = 0.0f;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->updCamera().theta = std::numbers::pi_v<float>;
            m_Shared->updCamera().phi = 0.0f;
        }
        osc::DrawTooltipIfItemHovered("Face camera facing along Z", "Right-clicking faces it along Z, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_CAMERA))
        {
            m_Shared->resetCamera();
        }
        osc::DrawTooltipIfItemHovered("Reset camera", "Resets the camera to its default position (the position it's in when the wizard is first loaded)");

        ImGui::PopID();
    }

    void draw3DViewerOverlayConvertToOpenSimModelButton()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});

        constexpr CStringView mainButtonText = "Convert to OpenSim Model " ICON_FA_ARROW_RIGHT;
        constexpr CStringView settingButtonText = ICON_FA_COG;
        constexpr Vec2 spacingBetweenMainAndSettingsButtons = {1.0f, 0.0f};
        constexpr Vec2 margin = {25.0f, 35.0f};

        Vec2 const mainButtonDims = osc::CalcButtonSize(mainButtonText);
        Vec2 const settingButtonDims = osc::CalcButtonSize(settingButtonText);
        Vec2 const viewportBottomRight = m_Shared->get3DSceneRect().p2;

        Vec2 const buttonTopLeft =
        {
            viewportBottomRight.x - (margin.x + spacingBetweenMainAndSettingsButtons.x + settingButtonDims.x + mainButtonDims.x),
            viewportBottomRight.y - (margin.y + mainButtonDims.y),
        };

        ImGui::SetCursorScreenPos(buttonTopLeft);
        osc::PushStyleColor(ImGuiCol_Button, Color::darkGreen());
        if (ImGui::Button(mainButtonText.c_str()))
        {
            m_Shared->tryCreateOutputModel();
        }
        osc::PopStyleColor();

        ImGui::PopStyleVar();
        osc::DrawTooltipIfItemHovered("Convert current scene to an OpenSim Model", "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nYour progress in this tab will remain untouched.");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});
        ImGui::SameLine(0.0f, spacingBetweenMainAndSettingsButtons.x);
        ImGui::Button(settingButtonText.c_str());
        ImGui::PopStyleVar();

        if (ImGui::BeginPopupContextItem("##settingspopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            ModelCreationFlags const flags = m_Shared->getModelCreationFlags();

            {
                bool v = flags & ModelCreationFlags::ExportStationsAsMarkers;
                if (ImGui::Checkbox("Export Stations as Markers", &v))
                {
                    ModelCreationFlags const newFlags = v ?
                        flags + ModelCreationFlags::ExportStationsAsMarkers :
                        flags - ModelCreationFlags::ExportStationsAsMarkers;
                    m_Shared->setModelCreationFlags(newFlags);
                }
            }

            ImGui::EndPopup();
        }
    }

    void draw3DViewerOverlay()
    {
        draw3DViewerOverlayTopBar();
        draw3DViewerOverlayBottomBar();
        draw3DViewerOverlayConvertToOpenSimModelButton();
    }

    void drawSceneElTooltip(SceneEl const& e) const
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s %s", e.getClass().getIconUTF8().c_str(), e.getLabel().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", GetContextMenuSubHeaderText(m_Shared->getModelGraph(), e).c_str());
        ImGui::EndTooltip();
    }

    void drawHoverTooltip()
    {
        if (!m_MaybeHover)
        {
            return;  // nothing is hovered
        }

        if (SceneEl const* e = m_Shared->getModelGraph().tryGetElByID(m_MaybeHover.ID))
        {
            drawSceneElTooltip(*e);
        }
    }

    // draws 3D manipulator overlays (drag handles, etc.)
    void drawSelection3DManipulatorGizmos()
    {
        if (!m_Shared->hasSelection())
        {
            return;  // can only manipulate if selecting something
        }

        // if the user isn't *currently* manipulating anything, create an
        // up-to-date manipulation matrix
        //
        // this is so that ImGuizmo can *show* the manipulation axes, and
        // because the user might start manipulating during this frame
        if (!ImGuizmo::IsUsing())
        {
            auto it = m_Shared->getCurrentSelection().begin();
            auto end = m_Shared->getCurrentSelection().end();

            if (it == end)
            {
                return;  // sanity exit
            }

            ModelGraph const& mg = m_Shared->getModelGraph();

            int n = 0;

            Transform ras = GetTransform(mg, *it);
            ++it;
            ++n;

            while (it != end)
            {
                ras += GetTransform(mg, *it);
                ++it;
                ++n;
            }

            ras /= static_cast<float>(n);
            ras.rotation = osc::Normalize(ras.rotation);

            m_ImGuizmoState.mtx = ToMat4(ras);
        }

        // else: is using OR nselected > 0 (so draw it)

        Rect sceneRect = m_Shared->get3DSceneRect();

        ImGuizmo::SetRect(
            sceneRect.p1.x,
            sceneRect.p1.y,
            Dimensions(sceneRect).x,
            Dimensions(sceneRect).y
        );
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::AllowAxisFlip(false);  // user's didn't like this feature in UX sessions

        Mat4 delta;
        SetImguizmoStyleToOSCStandard();
        bool manipulated = ImGuizmo::Manipulate(
            osc::ValuePtr(m_Shared->getCamera().getViewMtx()),
            osc::ValuePtr(m_Shared->getCamera().getProjMtx(AspectRatio(sceneRect))),
            m_ImGuizmoState.op,
            m_ImGuizmoState.mode,
            osc::ValuePtr(m_ImGuizmoState.mtx),
            osc::ValuePtr(delta),
            nullptr,
            nullptr,
            nullptr
        );

        bool isUsingThisFrame = ImGuizmo::IsUsing();
        bool wasUsingLastFrame = m_ImGuizmoState.wasUsingLastFrame;
        m_ImGuizmoState.wasUsingLastFrame = isUsingThisFrame;  // so next frame can know

                                                               // if the user was using the gizmo last frame, and isn't using it this frame,
                                                               // then they probably just finished a manipulation, which should be snapshotted
                                                               // for undo/redo support
        if (wasUsingLastFrame && !isUsingThisFrame)
        {
            m_Shared->commitCurrentModelGraph("manipulated selection");
            App::upd().requestRedraw();
        }

        // if no manipulation happened this frame, exit early
        if (!manipulated)
        {
            return;
        }

        Vec3 translation;
        Vec3 rotation;
        Vec3 scale;
        ImGuizmo::DecomposeMatrixToComponents(
            osc::ValuePtr(delta),
            osc::ValuePtr(translation),
            osc::ValuePtr(rotation),
            osc::ValuePtr(scale)
        );
        rotation = osc::Deg2Rad(rotation);

        for (UID id : m_Shared->getCurrentSelection())
        {
            SceneEl& el = m_Shared->updModelGraph().updElByID(id);
            switch (m_ImGuizmoState.op) {
            case ImGuizmo::ROTATE:
                el.applyRotation(m_Shared->getModelGraph(), rotation, m_ImGuizmoState.mtx[3]);
                break;
            case ImGuizmo::TRANSLATE:
                el.applyTranslation(m_Shared->getModelGraph(), translation);
                break;
            case ImGuizmo::SCALE:
                el.applyScale(m_Shared->getModelGraph(), scale);
                break;
            default:
                break;
            }
        }
    }

    // perform a hovertest on the current 3D scene to determine what the user's mouse is over
    MeshImporterHover hovertestScene(std::vector<DrawableThing> const& drawables)
    {
        if (!m_Shared->isRenderHovered())
        {
            return m_MaybeHover;
        }

        if (ImGuizmo::IsUsing())
        {
            return MeshImporterHover{};
        }

        return m_Shared->doHovertest(drawables);
    }

    // handle any side effects for current user mouse hover
    void handleCurrentHover()
    {
        if (!m_Shared->isRenderHovered())
        {
            return;  // nothing hovered
        }

        bool const lcClicked = osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
        bool const shiftDown = osc::IsShiftDown();
        bool const altDown = osc::IsAltDown();
        bool const isUsingGizmo = ImGuizmo::IsUsing();

        if (!m_MaybeHover && lcClicked && !isUsingGizmo && !shiftDown)
        {
            // user clicked in some empty part of the screen: clear selection
            m_Shared->deSelectAll();
        }
        else if (m_MaybeHover && lcClicked && !isUsingGizmo)
        {
            // user clicked hovered thing: select hovered thing
            if (!shiftDown)
            {
                // user wasn't holding SHIFT, so clear selection
                m_Shared->deSelectAll();
            }

            if (altDown)
            {
                // ALT: only select the thing the mouse is over
                selectJustHover();
            }
            else
            {
                // NO ALT: select the "grouped items"
                selectAnythingGroupedWithHover();
            }
        }
    }

    // generate 3D scene drawables for current state
    std::vector<DrawableThing>& generateDrawables()
    {
        m_DrawablesBuffer.clear();

        for (SceneEl const& e : m_Shared->getModelGraph().iter())
        {
            m_Shared->appendDrawables(e, m_DrawablesBuffer);
        }

        if (m_Shared->isShowingFloor())
        {
            m_DrawablesBuffer.push_back(m_Shared->generateFloorDrawable());
        }

        return m_DrawablesBuffer;
    }

    // draws main 3D viewer panel
    void draw3DViewer()
    {
        m_Shared->setContentRegionAvailAsSceneRect();

        std::vector<DrawableThing>& sceneEls = generateDrawables();

        // hovertest the generated geometry
        m_MaybeHover = hovertestScene(sceneEls);
        handleCurrentHover();

        // assign rim highlights based on hover
        for (DrawableThing& dt : sceneEls)
        {
            dt.flags = computeFlags(m_Shared->getModelGraph(), dt.id, m_MaybeHover.ID);
        }

        // draw 3D scene (effectively, as an ImGui::Image)
        m_Shared->drawScene(sceneEls);
        if (m_Shared->isRenderHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
        {
            m_MaybeOpenedContextMenu = m_MaybeHover;
            ImGui::OpenPopup("##maincontextmenu");
        }

        bool ctxMenuShowing = false;
        if (ImGui::BeginPopup("##maincontextmenu"))
        {
            ctxMenuShowing = true;
            drawContextMenuContent();
            ImGui::EndPopup();
        }

        if (m_Shared->isRenderHovered() && m_MaybeHover && (ctxMenuShowing ? m_MaybeHover.ID != m_MaybeOpenedContextMenu.ID : true))
        {
            drawHoverTooltip();
        }

        // draw overlays/gizmos
        drawSelection3DManipulatorGizmos();
        m_Shared->drawConnectionLines(m_MaybeHover);
    }

    void drawMainMenuFileMenu()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N"))
            {
                m_Shared->requestNewMeshImporterTab();
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Import", "Ctrl+O"))
            {
                m_Shared->openOsimFileAsModelGraph();
            }
            osc::DrawTooltipIfItemHovered("Import osim into mesh importer", "Try to import an existing osim file into the mesh importer.\n\nBEWARE: the mesh importer is *not* an OpenSim model editor. The import process will delete information from your osim in order to 'jam' it into this screen. The main purpose of this button is to export/import mesh editor scenes, not to edit existing OpenSim models.");

            if (ImGui::MenuItem(ICON_FA_SAVE " Export", "Ctrl+S"))
            {
                m_Shared->exportModelGraphAsOsimFile();
            }
            osc::DrawTooltipIfItemHovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            if (ImGui::MenuItem(ICON_FA_SAVE " Export As", "Shift+Ctrl+S"))
            {
                m_Shared->exportAsModelGraphAsOsimFile();
            }
            osc::DrawTooltipIfItemHovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Import Stations from CSV"))
            {
                auto popup = std::make_shared<ImportStationsFromCSVPopup>(
                    "Import Stations from CSV",
                    m_Shared
                );
                popup->open();
                m_PopupManager.push_back(std::move(popup));
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W"))
            {
                m_Shared->requestClose();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
            {
                App::upd().requestQuit();
            }

            ImGui::EndMenu();
        }
    }

    void drawMainMenuEditMenu()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Shared->canUndoCurrentModelGraph()))
            {
                m_Shared->undoCurrentModelGraph();
            }
            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Shared->canRedoCurrentModelGraph()))
            {
                m_Shared->redoCurrentModelGraph();
            }
            ImGui::EndMenu();
        }
    }

    void drawMainMenuWindowMenu()
    {

        if (ImGui::BeginMenu("Window"))
        {
            for (size_t i = 0; i < m_Shared->getNumToggleablePanels(); ++i)
            {
                bool isEnabled = m_Shared->isNthPanelEnabled(i);
                if (ImGui::MenuItem(m_Shared->getNthPanelName(i).c_str(), nullptr, isEnabled))
                {
                    m_Shared->setNthPanelEnabled(i, !isEnabled);
                }
            }
            ImGui::EndMenu();
        }
    }

    void drawMainMenuAboutMenu()
    {
        osc::MainMenuAboutTab{}.onDraw();
    }

    // draws main 3D viewer, or a modal (if one is active)
    void drawMainViewerPanelOrModal()
    {
        if (m_Maybe3DViewerModal)
        {
            // ensure it stays alive - even if it pops itself during the drawcall
            std::shared_ptr<MeshImporterUILayer> const ptr = m_Maybe3DViewerModal;

            // open it "over" the whole UI as a "modal" - so that the user can't click things
            // outside of the panel
            ImGui::OpenPopup("##visualizermodalpopup");
            ImGui::SetNextWindowSize(m_Shared->get3DSceneDims());
            ImGui::SetNextWindowPos(m_Shared->get3DSceneRect().p1);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});

            ImGuiWindowFlags const modalFlags =
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize;

            if (ImGui::BeginPopupModal("##visualizermodalpopup", nullptr, modalFlags))
            {
                ImGui::PopStyleVar();
                ptr->onDraw();
                ImGui::EndPopup();
            }
            else
            {
                ImGui::PopStyleVar();
            }
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
            if (ImGui::Begin("wizard_3dViewer"))
            {
                ImGui::PopStyleVar();
                draw3DViewer();
                ImGui::SetCursorPos(Vec2{ImGui::GetCursorStartPos()} + Vec2{10.0f, 10.0f});
                draw3DViewerOverlay();
            }
            else
            {
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
    }

    // tab data
    UID m_TabID;
    ParentPtr<MainUIStateAPI> m_Parent;
    std::string m_Name = "MeshImporterTab";

    // data shared between states
    std::shared_ptr<MeshImporterSharedState> m_Shared;

    // buffer that's filled with drawable geometry during a drawcall
    std::vector<DrawableThing> m_DrawablesBuffer;

    // (maybe) hover + worldspace location of the hover
    MeshImporterHover m_MaybeHover;

    // (maybe) the scene element that the user opened a context menu for
    MeshImporterHover m_MaybeOpenedContextMenu;

    // (maybe) the next state the host screen should transition to
    std::shared_ptr<MeshImporterUILayer> m_Maybe3DViewerModal;

    // ImGuizmo state
    struct {
        bool wasUsingLastFrame = false;
        Mat4 mtx = Identity<Mat4>();
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mode = ImGuizmo::WORLD;
    } m_ImGuizmoState;

    // manager for active modal popups (importer popups, etc.)
    PopupManager m_PopupManager;
};


// public API (PIMPL)

osc::MeshImporterTab::MeshImporterTab(
    ParentPtr<MainUIStateAPI> const& parent_) :

    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::MeshImporterTab::MeshImporterTab(
    ParentPtr<MainUIStateAPI> const& parent_,
    std::vector<std::filesystem::path> files_) :

    m_Impl{std::make_unique<Impl>(parent_, std::move(files_))}
{
}

osc::MeshImporterTab::MeshImporterTab(MeshImporterTab&&) noexcept = default;
osc::MeshImporterTab& osc::MeshImporterTab::operator=(MeshImporterTab&&) noexcept = default;
osc::MeshImporterTab::~MeshImporterTab() noexcept = default;

osc::UID osc::MeshImporterTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MeshImporterTab::implGetName() const
{
    return m_Impl->getName();
}

bool osc::MeshImporterTab::implIsUnsaved() const
{
    return m_Impl->isUnsaved();
}

bool osc::MeshImporterTab::implTrySave()
{
    return m_Impl->trySave();
}

void osc::MeshImporterTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::MeshImporterTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::MeshImporterTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::MeshImporterTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::MeshImporterTab::implOnDrawMainMenu()
{
    m_Impl->drawMainMenu();
}

void osc::MeshImporterTab::implOnDraw()
{
    m_Impl->onDraw();
}
