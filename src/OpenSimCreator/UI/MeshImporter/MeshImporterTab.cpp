#include "MeshImporterTab.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/MeshImporter/Body.h>
#include <OpenSimCreator/Documents/MeshImporter/Document.h>
#include <OpenSimCreator/Documents/MeshImporter/Ground.h>
#include <OpenSimCreator/Documents/MeshImporter/Joint.h>
#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObjectHelpers.h>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.h>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.h>
#include <OpenSimCreator/Documents/MeshImporter/OpenSimExportFlags.h>
#include <OpenSimCreator/Documents/MeshImporter/Station.h>
#include <OpenSimCreator/Documents/MeshImporter/UndoableActions.h>
#include <OpenSimCreator/Documents/MeshImporter/UndoableDocument.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/MeshImporter/ChooseElLayer.h>
#include <OpenSimCreator/UI/MeshImporter/DrawableThing.h>
#include <OpenSimCreator/UI/MeshImporter/IMeshImporterUILayerHost.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterHover.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterSharedState.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterUILayer.h>
#include <OpenSimCreator/UI/MeshImporter/Select2MeshPointsLayer.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/UI/Shared/ImportStationsFromCSVPopup.h>
#include <OpenSimCreator/UI/Shared/MainMenu.h>

#include <IconsFontAwesome5.h>
#include <oscar/Formats/OBJ.h>
#include <oscar/Formats/STL.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/TransformFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/ImGuizmoHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Panels/UndoRedoPanel.h>
#include <oscar/UI/Widgets/CameraViewAxes.h>
#include <oscar/UI/Widgets/LogViewer.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/UID.h>
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
class osc::mi::MeshImporterTab::Impl final : public IMeshImporterUILayerHost {
public:
    explicit Impl(ParentPtr<IMainUIStateAPI> const& parent_) :
        m_Parent{parent_},
        m_Shared{std::make_shared<MeshImporterSharedState>()}
    {
    }

    Impl(
        ParentPtr<IMainUIStateAPI> const& parent_,
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
            PerfPanel& pp = m_Shared->updPerfPanel();

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

        Document const& mg = m_Shared->getModelGraph();

        MIObject const* hoveredMIObject = mg.tryGetByID(m_MaybeHover.ID);

        if (!hoveredMIObject)
        {
            return;  // current hover isn't in the current model graph
        }

        UID maybeID = GetStationAttachmentParent(mg, *hoveredMIObject);

        if (maybeID == MIIDs::Ground() || maybeID == MIIDs::Empty())
        {
            return;  // can't attach to it as-if it were a body
        }

        auto const* bodyEl = mg.tryGetByID<Body>(maybeID);
        if (!bodyEl)
        {
            return;  // suggested attachment parent isn't in the current model graph?
        }

        transitionToChoosingJointParent(*bodyEl);
    }

    // try transitioning the shown UI layer to one where the user is assigning a mesh
    void tryTransitionToAssigningHoverAndSelectionNextFrame()
    {
        Document const& mg = m_Shared->getModelGraph();

        std::unordered_set<UID> meshes;
        meshes.insert(mg.getSelected().begin(), mg.getSelected().end());
        if (m_MaybeHover)
        {
            meshes.insert(m_MaybeHover.ID);
        }

        std::erase_if(meshes, [&mg](UID meshID) { return !mg.contains<Mesh>(meshID); });

        if (meshes.empty())
        {
            return;  // nothing to assign
        }

        std::unordered_set<UID> attachments;
        for (UID meshID : meshes)
        {
            attachments.insert(mg.getByID<Mesh>(meshID).getParentID());
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
    void transitionToChoosingJointParent(Body const& child)
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
    void transitionToChoosingWhichElementToPointAxisTowards(MIObject& el, int axis)
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

            return point_axis_towards(shared->updCommittableModelGraph(), id, axis, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two elements that the given axis
    // should be aligned along (i.e. the direction vector from the first element to the second element
    // becomes the direction vector of the given axis)
    void transitionToChoosingTwoElementsToAlignAxisAlong(MIObject& el, int axis)
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

            return TryOrientObjectAxisAlongTwoObjects(
                shared->updCommittableModelGraph(),
                id,
                axis,
                choices[0],
                choices[1]
            );
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToChoosingWhichElementToTranslateTo(MIObject& el)
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

            return TryTranslateObjectToAnotherObject(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToChoosingElementsToTranslateBetween(MIObject& el)
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

            return TryTranslateBetweenTwoObjects(
                shared->updCommittableModelGraph(),
                id,
                choices[0],
                choices[1]
            );
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToCopyingSomethingElsesOrientation(MIObject& el)
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
    void transitionToOrientingElementAlongTwoMeshPoints(MIObject& el, int axis)
    {
        Select2MeshPointsOptions opts;
        opts.onTwoPointsChosen = [shared = m_Shared, id = el.getID(), axis](Vec3 a, Vec3 b)
        {
            return TryOrientObjectAxisAlongTwoPoints(shared->updCommittableModelGraph(), id, axis, a, b);
        };
        m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
    }

    // transition the shown UI layer to one where the user is choosing two mesh points that
    // the element sould be translated to the midpoint of
    void transitionToTranslatingElementAlongTwoMeshPoints(MIObject& el)
    {
        Select2MeshPointsOptions opts;
        opts.onTwoPointsChosen = [shared = m_Shared, id = el.getID()](Vec3 a, Vec3 b)
        {
            return TryTranslateObjectBetweenTwoPoints(shared->updCommittableModelGraph(), id, a, b);
        };
        m_Maybe3DViewerModal = std::make_shared<Select2MeshPointsLayer>(*this, m_Shared, opts);
    }

    void transitionToTranslatingElementToMeshAverageCenter(MIObject& el)
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

    void transitionToTranslatingElementToMeshBoundsCenter(MIObject& el)
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

    void transitionToTranslatingElementToMeshMassCenter(MIObject& el)
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
    void transitionToTranslatingElementToAnotherElementsCenter(MIObject& el)
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

            return TryTranslateObjectToAnotherObject(shared->updCommittableModelGraph(), id, choices.front());
        };
        m_Maybe3DViewerModal = std::make_shared<ChooseElLayer>(*this, m_Shared, opts);
    }

    void transitionToReassigningCrossRef(MIObject& el, int crossrefIdx)
    {
        int nRefs = el.getNumCrossReferences();

        if (crossrefIdx < 0 || crossrefIdx >= nRefs)
        {
            return;  // invalid index?
        }

        MIObject const* old = m_Shared->getModelGraph().tryGetByID(el.getCrossReferenceConnecteeID(crossrefIdx));

        if (!old)
        {
            return;  // old el doesn't exist?
        }

        ChooseElLayerOptions opts;
        opts.canChooseBodies = (dynamic_cast<Body const*>(old) != nullptr) || (dynamic_cast<Ground const*>(old) != nullptr);
        opts.canChooseGround = (dynamic_cast<Body const*>(old) != nullptr) || (dynamic_cast<Ground const*>(old) != nullptr);
        opts.canChooseJoints = dynamic_cast<Joint const*>(old) != nullptr;
        opts.canChooseMeshes = dynamic_cast<Mesh const*>(old) != nullptr;
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
        Document const& mg = m_Shared->getModelGraph();

        if (m_MaybeHover && !mg.contains(m_MaybeHover.ID))
        {
            m_MaybeHover.reset();
        }

        if (m_MaybeOpenedContextMenu && !mg.contains(m_MaybeOpenedContextMenu.ID))
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
        DeleteObject(m_Shared->updCommittableModelGraph(), elID);
        garbageCollectStaleRefs();
    }

    // update this scene from the current keyboard state, as saved by ImGui
    bool updateFromImGuiKeyboardState()
    {
        if (ImGui::GetIO().WantCaptureKeyboard)
        {
            return false;
        }

        bool shiftDown = IsShiftDown();
        bool ctrlOrSuperDown = IsCtrlOrSuperDown();

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
        else if (IsAnyKeyDown({ImGuiKey_Delete, ImGuiKey_Backspace}))
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
        ui::Text(ICON_FA_BOLT " Actions");
        ImGui::SameLine();
        ui::TextDisabled("(nothing clicked)");
        ImGui::Separator();
    }

    void drawMIObjectContextMenuContentHeader(MIObject const& e)
    {
        ui::Text("%s %s", e.getClass().getIconUTF8().c_str(), e.getLabel().c_str());
        ImGui::SameLine();
        ui::TextDisabled(GetContextMenuSubHeaderText(m_Shared->getModelGraph(), e));
        ImGui::SameLine();
        DrawHelpMarker(e.getClass().getName(), e.getClass().getDescription());
        ImGui::Separator();
    }

    void drawMIObjectPropEditors(MIObject const& e)
    {
        Document& mg = m_Shared->updModelGraph();

        // label/name editor
        if (e.canChangeLabel())
        {
            std::string buf{static_cast<std::string_view>(e.getLabel())};
            if (InputString("Name", buf))
            {
                mg.updByID(e.getID()).setLabel(buf);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getClass().getName() << " name";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            DrawHelpMarker("Component Name", "This is the name that the component will have in the exported OpenSim model.");
        }

        // position editor
        if (e.canChangePosition())
        {
            Vec3 translation = e.getPos(mg);
            if (ImGui::InputFloat3("Translation", value_ptr(translation), "%.6f"))
            {
                mg.updByID(e.getID()).setPos(mg, translation);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s translation";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            DrawHelpMarker("Translation", MIStrings::c_TranslationDescription);
        }

        // rotation editor
        if (e.canChangeRotation())
        {
            Eulers eulers = euler_angles(normalize(e.getRotation(m_Shared->getModelGraph())));

            if (InputAngle3("Rotation", eulers, "%.6f"))
            {
                Quat quatRads = osc::WorldspaceRotation(eulers);
                mg.updByID(e.getID()).setRotation(mg, quatRads);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s rotation";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            DrawHelpMarker("Rotation", "These are the rotation Euler angles for the component in ground. Positive rotations are anti-clockwise along that axis.\n\nNote: the numbers may contain slight rounding error, due to backend constraints. Your values *should* be accurate to a few decimal places.");
        }

        // scale factor editor
        if (e.canChangeScale())
        {
            Vec3 scaleFactors = e.getScale(mg);
            if (ImGui::InputFloat3("Scale", value_ptr(scaleFactors), "%.6f"))
            {
                mg.updByID(e.getID()).setScale(mg, scaleFactors);
            }
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s scale";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ImGui::SameLine();
            DrawHelpMarker("Scale", "These are the scale factors of the component in ground. These scale-factors are applied to the element before any other transform (it scales first, then rotates, then translates).");
        }
    }

    // draw content of "Add" menu for some scene element
    void drawAddOtherToMIObjectActions(MIObject& el, Vec3 const& clickPos)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});
        ScopeGuard const g1{[]() { ImGui::PopStyleVar(); }};

        int imguiID = 0;
        ImGui::PushID(imguiID++);
        ScopeGuard const g2{[]() { ImGui::PopID(); }};

        if (CanAttachMeshTo(el))
        {
            if (ui::MenuItem(ICON_FA_CUBE " Meshes"))
            {
                m_Shared->pushMeshLoadRequests(el.getID(), m_Shared->promptUserForMeshFiles());
            }
            DrawTooltipIfItemHovered("Add Meshes", MIStrings::c_MeshDescription);
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (el.hasPhysicalSize())
        {
            if (ui::BeginMenu(ICON_FA_CIRCLE " Body"))
            {
                if (ui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                {
                    AddBody(m_Shared->updCommittableModelGraph(), el.getPos(m_Shared->getModelGraph()), el.getID());
                }
                DrawTooltipIfItemHovered("Add Body", MIStrings::c_BodyDescription.c_str());

                if (ui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                {
                    AddBody(m_Shared->updCommittableModelGraph(), clickPos, el.getID());
                }
                DrawTooltipIfItemHovered("Add Body", MIStrings::c_BodyDescription.c_str());

                if (ui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                {
                    AddBody(m_Shared->updCommittableModelGraph());
                }
                DrawTooltipIfItemHovered("Add body", MIStrings::c_BodyDescription.c_str());

                if (auto const* mesh = dynamic_cast<Mesh const*>(&el))
                {
                    if (ui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                    {
                        Vec3 const location = centroid(mesh->calcBounds());
                        AddBody(m_Shared->updCommittableModelGraph(), location, mesh->getID());
                    }
                    DrawTooltipIfItemHovered("Add Body", MIStrings::c_BodyDescription.c_str());

                    if (ui::MenuItem(ICON_FA_DIVIDE " at mesh average center"))
                    {
                        Vec3 const location = AverageCenter(*mesh);
                        AddBody(m_Shared->updCommittableModelGraph(), location, mesh->getID());
                    }
                    DrawTooltipIfItemHovered("Add Body", MIStrings::c_BodyDescription.c_str());

                    if (ui::MenuItem(ICON_FA_WEIGHT " at mesh mass center"))
                    {
                        Vec3 const location = MassCenter(*mesh);
                        AddBody(m_Shared->updCommittableModelGraph(), location, mesh->getID());
                    }
                    DrawTooltipIfItemHovered("Add body", MIStrings::c_BodyDescription.c_str());
                }

                ui::EndMenu();
            }
        }
        else
        {
            if (ui::MenuItem(ICON_FA_CIRCLE " Body"))
            {
                AddBody(m_Shared->updCommittableModelGraph(), el.getPos(m_Shared->getModelGraph()), el.getID());
            }
            DrawTooltipIfItemHovered("Add Body", MIStrings::c_BodyDescription.c_str());
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (auto const* body = dynamic_cast<Body const*>(&el))
        {
            if (ui::MenuItem(ICON_FA_LINK " Joint"))
            {
                transitionToChoosingJointParent(*body);
            }
            DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }
        ImGui::PopID();

        ImGui::PushID(imguiID++);
        if (CanAttachStationTo(el))
        {
            if (el.hasPhysicalSize())
            {
                if (ui::BeginMenu(ICON_FA_MAP_PIN " Station"))
                {
                    if (ui::MenuItem(ICON_FA_COMPRESS_ARROWS_ALT " at center"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, el.getPos(m_Shared->getModelGraph()));
                    }
                    DrawTooltipIfItemHovered("Add Station", MIStrings::c_StationDescription);

                    if (ui::MenuItem(ICON_FA_MOUSE_POINTER " at click position"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, clickPos);
                    }
                    DrawTooltipIfItemHovered("Add Station", MIStrings::c_StationDescription);

                    if (ui::MenuItem(ICON_FA_DOT_CIRCLE " at ground"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, Vec3{});
                    }
                    DrawTooltipIfItemHovered("Add Station", MIStrings::c_StationDescription);

                    if (dynamic_cast<Mesh const*>(&el))
                    {
                        if (ui::MenuItem(ICON_FA_BORDER_ALL " at bounds center"))
                        {
                            AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, centroid(el.calcBounds(m_Shared->getModelGraph())));
                        }
                        DrawTooltipIfItemHovered("Add Station", MIStrings::c_StationDescription);
                    }

                    ui::EndMenu();
                }
            }
            else
            {
                if (ui::MenuItem(ICON_FA_MAP_PIN " Station"))
                {
                    AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, el.getPos(m_Shared->getModelGraph()));
                }
                DrawTooltipIfItemHovered("Add Station", MIStrings::c_StationDescription);
            }
        }
        // ~ScopeGuard: implicitly calls ImGui::PopID()
    }

    void drawNothingActions()
    {
        if (ui::MenuItem(ICON_FA_CUBE " Add Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        DrawTooltipIfItemHovered("Add Meshes to the model", MIStrings::c_MeshDescription);

        if (ui::BeginMenu(ICON_FA_PLUS " Add Other"))
        {
            drawAddOtherMenuItems();

            ui::EndMenu();
        }
    }

    void drawMIObjectActions(MIObject& el, Vec3 const& clickPos)
    {
        if (ui::MenuItem(ICON_FA_CAMERA " Focus camera on this"))
        {
            m_Shared->focusCameraOn(centroid(el.calcBounds(m_Shared->getModelGraph())));
        }
        DrawTooltipIfItemHovered("Focus camera on this scene element", "Focuses the scene camera on this element. This is useful for tracking the camera around that particular object in the scene");

        if (ui::BeginMenu(ICON_FA_PLUS " Add"))
        {
            drawAddOtherToMIObjectActions(el, clickPos);
            ui::EndMenu();
        }

        if (auto const* body = dynamic_cast<Body const*>(&el))
        {
            if (ui::MenuItem(ICON_FA_LINK " Join to"))
            {
                transitionToChoosingJointParent(*body);
            }
            DrawTooltipIfItemHovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }

        if (el.canDelete())
        {
            if (ui::MenuItem(ICON_FA_TRASH " Delete"))
            {
                DeleteObject(m_Shared->updCommittableModelGraph(), el.getID());
                garbageCollectStaleRefs();
                ImGui::CloseCurrentPopup();
            }
            DrawTooltipIfItemHovered("Delete", "Deletes the component from the model. Deletion is undo-able (use the undo/redo feature). Anything attached to this element (e.g. joints, meshes) will also be deleted.");
        }
    }

    // draw the "Translate" menu for any generic `MIObject`
    void drawTranslateMenu(MIObject& el)
    {
        if (!el.canChangePosition())
        {
            return;  // can't change its position
        }

        if (!ui::BeginMenu(ICON_FA_ARROWS_ALT " Translate"))
        {
            return;  // top-level menu isn't open
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
        {
            std::string label = "To " + el.getCrossReferenceLabel(i);
            if (ui::MenuItem(label.c_str()))
            {
                TryTranslateObjectToAnotherObject(m_Shared->updCommittableModelGraph(), el.getID(), el.getCrossReferenceConnecteeID(i));
            }
        }

        if (ui::MenuItem("To (select something)"))
        {
            transitionToChoosingWhichElementToTranslateTo(el);
        }

        if (el.getNumCrossReferences() == 2)
        {
            std::string label = "Between " + el.getCrossReferenceLabel(0) + " and " + el.getCrossReferenceLabel(1);
            if (ui::MenuItem(label.c_str()))
            {
                UID a = el.getCrossReferenceConnecteeID(0);
                UID b = el.getCrossReferenceConnecteeID(1);
                TryTranslateBetweenTwoObjects(m_Shared->updCommittableModelGraph(), el.getID(), a, b);
            }
        }

        if (ui::MenuItem("Between two scene elements"))
        {
            transitionToChoosingElementsToTranslateBetween(el);
        }

        if (ui::MenuItem("Between two mesh points"))
        {
            transitionToTranslatingElementAlongTwoMeshPoints(el);
        }

        if (ui::MenuItem("To mesh bounds center"))
        {
            transitionToTranslatingElementToMeshBoundsCenter(el);
        }
        DrawTooltipIfItemHovered("Translate to mesh bounds center", "Translates the given element to the center of the selected mesh's bounding box. The bounding box is the smallest box that contains all mesh vertices");

        if (ui::MenuItem("To mesh average center"))
        {
            transitionToTranslatingElementToMeshAverageCenter(el);
        }
        DrawTooltipIfItemHovered("Translate to mesh average center", "Translates the given element to the average center point of vertices in the selected mesh.\n\nEffectively, this adds each vertex location in the mesh, divides the sum by the number of vertices in the mesh, and sets the translation of the given object to that location.");

        if (ui::MenuItem("To mesh mass center"))
        {
            transitionToTranslatingElementToMeshMassCenter(el);
        }
        DrawTooltipIfItemHovered("Translate to mesh mess center", "Translates the given element to the mass center of the selected mesh.\n\nCAREFUL: the algorithm used to do this heavily relies on your triangle winding (i.e. normals) being correct and your mesh being a closed surface. If your mesh doesn't meet these requirements, you might get strange results (apologies: the only way to get around that problems involves complicated voxelization and leak-detection algorithms :( )");

        ImGui::PopStyleVar();
        ui::EndMenu();
    }

    // draw the "Reorient" menu for any generic `MIObject`
    void drawReorientMenu(MIObject& el)
    {
        if (!el.canChangeRotation())
        {
            return;  // can't change its rotation
        }

        if (!ui::BeginMenu(ICON_FA_REDO " Reorient"))
        {
            return;  // top-level menu isn't open
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

        {
            auto DrawMenuContent = [&](int axis)
            {
                for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
                {
                    std::string label = "Towards " + el.getCrossReferenceLabel(i);

                    if (ui::MenuItem(label.c_str()))
                    {
                        point_axis_towards(m_Shared->updCommittableModelGraph(), el.getID(), axis, el.getCrossReferenceConnecteeID(i));
                    }
                }

                if (ui::MenuItem("Towards (select something)"))
                {
                    transitionToChoosingWhichElementToPointAxisTowards(el, axis);
                }

                if (ui::MenuItem("Along line between (select two elements)"))
                {
                    transitionToChoosingTwoElementsToAlignAxisAlong(el, axis);
                }

                if (ui::MenuItem("90 degress"))
                {
                    RotateAxis(m_Shared->updCommittableModelGraph(), el, axis, 90_deg);
                }

                if (ui::MenuItem("180 degrees"))
                {
                    RotateAxis(m_Shared->updCommittableModelGraph(), el, axis, 180_deg);
                }

                if (ui::MenuItem("Along two mesh points"))
                {
                    transitionToOrientingElementAlongTwoMeshPoints(el, axis);
                }
            };

            if (ui::BeginMenu("x"))
            {
                DrawMenuContent(0);
                ui::EndMenu();
            }

            if (ui::BeginMenu("y"))
            {
                DrawMenuContent(1);
                ui::EndMenu();
            }

            if (ui::BeginMenu("z"))
            {
                DrawMenuContent(2);
                ui::EndMenu();
            }
        }

        if (ui::MenuItem("copy"))
        {
            transitionToCopyingSomethingElsesOrientation(el);
        }

        if (ui::MenuItem("reset"))
        {
            el.setXform(m_Shared->getModelGraph(), Transform{.position = el.getPos(m_Shared->getModelGraph())});
            m_Shared->commitCurrentModelGraph("reset " + el.getLabel() + " orientation");
        }

        ImGui::PopStyleVar();
        ui::EndMenu();
    }

    // draw the "Mass" editor for a `BodyEl`
    void drawMassEditor(Body const& bodyEl)
    {
        auto curMass = static_cast<float>(bodyEl.getMass());
        if (ImGui::InputFloat("Mass", &curMass, 0.0f, 0.0f, "%.6f"))
        {
            m_Shared->updModelGraph().updByID<Body>(bodyEl.getID()).setMass(static_cast<double>(curMass));
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            m_Shared->commitCurrentModelGraph("changed body mass");
        }
        ImGui::SameLine();
        DrawHelpMarker("Mass", "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
    }

    // draw the "Joint Type" editor for a `JointEl`
    void drawJointTypeEditor(Joint const& jointEl)
    {
        size_t currentIdx = jointEl.getJointTypeIndex();
        auto const& registry = GetComponentRegistry<OpenSim::Joint>();
        auto const nameAccessor = [&registry](size_t i) { return registry[i].name(); };

        if (Combo("Joint Type", &currentIdx, registry.size(), nameAccessor))
        {
            m_Shared->updModelGraph().updByID<Joint>(jointEl.getID()).setJointTypeIndex(currentIdx);
            m_Shared->commitCurrentModelGraph("changed joint type");
        }
        ImGui::SameLine();
        DrawHelpMarker("Joint Type", "This is the type of joint that should be added into the OpenSim model. The joint's type dictates what types of motion are permitted around the joint center. See the official OpenSim documentation for an explanation of each joint type.");
    }

    // draw the "Reassign Connection" menu, which lets users change an element's cross reference
    void drawReassignCrossrefMenu(MIObject& el)
    {
        int nRefs = el.getNumCrossReferences();

        if (nRefs == 0)
        {
            return;
        }

        if (ui::BeginMenu(ICON_FA_EXTERNAL_LINK_ALT " Reassign Connection"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{10.0f, 10.0f});

            for (int i = 0; i < nRefs; ++i)
            {
                CStringView label = el.getCrossReferenceLabel(i);
                if (ui::MenuItem(label.c_str()))
                {
                    transitionToReassigningCrossRef(el, i);
                }
            }

            ImGui::PopStyleVar();
            ui::EndMenu();
        }
    }

    void actionPromptUserToSaveMeshAsOBJ(
        osc::Mesh const& mesh)
    {
        // prompt user for a save location
        std::optional<std::filesystem::path> const maybeUserSaveLocation =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("obj");
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
            std::string const error = CurrentErrnoAsString();
            log_error("%s: could not save obj output: %s", userSaveLocation.string().c_str(), error.c_str());
            return;
        }

        AppMetadata const& appMetadata = App::get().getMetadata();
        ObjMetadata const objMetadata
        {
            CalcFullApplicationNameWithVersionAndBuild(appMetadata),
        };

        WriteMeshAsObj(
            outputFileStream,
            mesh,
            objMetadata,
            ObjWriterFlags::NoWriteNormals
        );
    }

    void actionPromptUserToSaveMeshAsSTL(
        osc::Mesh const& mesh)
    {
        // prompt user for a save location
        std::optional<std::filesystem::path> const maybeUserSaveLocation =
            PromptUserForFileSaveLocationAndAddExtensionIfNecessary("stl");
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
            std::string const error = CurrentErrnoAsString();
            log_error("%s: could not save obj output: %s", userSaveLocation.string().c_str(), error.c_str());
            return;
        }

        AppMetadata const& appMetadata = App::get().getMetadata();
        StlMetadata const stlMetadata
        {
            CalcFullApplicationNameWithVersionAndBuild(appMetadata),
        };

        WriteMeshAsStl(outputFileStream, mesh, stlMetadata);
    }

    void drawSaveMeshMenu(Mesh const& el)
    {
        if (ui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
        {
            ui::TextDisabled("With Respect to:");
            ImGui::Separator();
            for (MIObject const& MIObject : m_Shared->getModelGraph().iter())
            {
                if (ui::BeginMenu(MIObject.getLabel().c_str()))
                {
                    ui::TextDisabled("Format:");
                    ImGui::Separator();

                    if (ui::MenuItem(".obj"))
                    {
                        Transform const MIObjectToGround = MIObject.getXForm(m_Shared->getModelGraph());
                        Transform const meshVertToGround = el.getXForm();
                        Mat4 const meshVertToMIObjectVert = inverse_mat4_cast(MIObjectToGround) * mat4_cast(meshVertToGround);

                        osc::Mesh mesh = el.getMeshData();
                        mesh.transformVerts(meshVertToMIObjectVert);
                        actionPromptUserToSaveMeshAsOBJ(mesh);
                    }

                    if (ui::MenuItem(".stl"))
                    {
                        Transform const MIObjectToGround = MIObject.getXForm(m_Shared->getModelGraph());
                        Transform const meshVertToGround = el.getXForm();
                        Mat4 const meshVertToMIObjectVert = inverse_mat4_cast(MIObjectToGround) * mat4_cast(meshVertToGround);

                        osc::Mesh mesh = el.getMeshData();
                        mesh.transformVerts(meshVertToMIObjectVert);
                        actionPromptUserToSaveMeshAsSTL(mesh);
                    }

                    ui::EndMenu();
                }
            }
            ui::EndMenu();
        }
    }

    void drawContextMenuSpacer()
    {
        ImGui::Dummy({0.0f, 0.0f});
    }

    // draw context menu content for when user right-clicks nothing
    void drawNothingContextMenuContent()
    {
        drawNothingContextMenuContentHeader();
        drawContextMenuSpacer();
        drawNothingActions();
    }

    // draw context menu content for a `GroundEl`
    void drawContextMenuContent(Ground& el, Vec3 const& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);
        drawContextMenuSpacer();
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `BodyEl`
    void drawContextMenuContent(Body& el, Vec3 const& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);
        drawMassEditor(el);

        drawContextMenuSpacer();
        ImGui::Separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `Mesh`
    void drawContextMenuContent(Mesh& el, Vec3 const& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);

        drawContextMenuSpacer();
        ImGui::Separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawSaveMeshMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `JointEl`
    void drawContextMenuContent(Joint& el, Vec3 const& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);
        drawJointTypeEditor(el);

        drawContextMenuSpacer();
        ImGui::Separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `StationEl`
    void drawContextMenuContent(StationEl& el, Vec3 const& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);

        drawContextMenuSpacer();
        ImGui::Separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for some scene element
    void drawContextMenuContent(MIObject& el, Vec3 const& clickPos)
    {
        std::visit(Overload
        {
            [this, &clickPos](Ground& el)  { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](Mesh& el)    { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](Body& el)    { this->drawContextMenuContent(el, clickPos); },
            [this, &clickPos](Joint& el)   { this->drawContextMenuContent(el, clickPos); },
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
        else if (m_MaybeOpenedContextMenu.ID == MIIDs::RightClickedNothing())
        {
            // context menu was opened on "nothing" specifically
            PushID(UID::empty());
            ScopeGuard const g{[]() { ImGui::PopID(); }};
            drawNothingContextMenuContent();
        }
        else if (MIObject* el = m_Shared->updModelGraph().tryUpdByID(m_MaybeOpenedContextMenu.ID))
        {
            // context menu was opened on a scene element that exists in the modelgraph
            PushID(el->getID());
            ScopeGuard const g{[]() { ImGui::PopID(); }};
            drawContextMenuContent(*el, m_MaybeOpenedContextMenu.Pos);
        }


        // context menu should be closed under these conditions
        if (IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Escape}))
        {
            m_MaybeOpenedContextMenu.reset();
            ImGui::CloseCurrentPopup();
        }
    }

    // draw the content of the (undo/redo) "History" panel
    void drawHistoryPanelContent()
    {
        UndoRedoPanel::DrawContent(m_Shared->updCommittableModelGraph());
    }

    void drawNavigatorElement(MIClass const& c)
    {
        Document& mg = m_Shared->updModelGraph();

        ui::Text("%s %s", c.getIconUTF8().c_str(), c.getNamePluralized().c_str());
        ImGui::SameLine();
        DrawHelpMarker(c.getNamePluralized(), c.getDescription());
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::Indent();

        bool empty = true;
        for (MIObject const& el : mg.iter())
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
                PushStyleColor(ImGuiCol_Text, Color::yellow());
                ++styles;
            }
            else if (m_Shared->isSelected(id))
            {
                PushStyleColor(ImGuiCol_Text, Color::yellow());
                ++styles;
            }

            ui::Text(el.getLabel());

            ImGui::PopStyleColor(styles);

            if (ImGui::IsItemHovered())
            {
                m_MaybeHover = {id, {}};
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                if (!IsShiftDown())
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
            ui::TextDisabled("(no %s)", c.getNamePluralized().c_str());
        }
        ImGui::Unindent();
    }

    void drawNavigatorPanelContent()
    {
        for (MIClass const& c : GetSceneElClasses())
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

        if (ui::MenuItem(ICON_FA_CUBE " Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        DrawTooltipIfItemHovered("Add Meshes", MIStrings::c_MeshDescription);

        if (ui::MenuItem(ICON_FA_CIRCLE " Body"))
        {
            AddBody(m_Shared->updCommittableModelGraph());
        }
        DrawTooltipIfItemHovered("Add Body", MIStrings::c_BodyDescription);

        if (ui::MenuItem(ICON_FA_MAP_PIN " Station"))
        {
            Document& mg = m_Shared->updModelGraph();
            auto& e = mg.emplace<StationEl>(UID{}, MIIDs::Ground(), Vec3{}, StationEl::Class().generateName());
            mg.selectOnly(e);
        }
        DrawTooltipIfItemHovered("Add Station", StationEl::Class().getDescription());

        ImGui::PopStyleVar();
    }

    void draw3DViewerOverlayTopBar()
    {
        int imguiID = 0;

        if (ImGui::Button(ICON_FA_CUBE " Add Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        DrawTooltipIfItemHovered("Add Meshes to the model", MIStrings::c_MeshDescription);

        ImGui::SameLine();

        ImGui::Button(ICON_FA_PLUS " Add Other");
        DrawTooltipIfItemHovered("Add components to the model");

        if (ImGui::BeginPopupContextItem("##additemtoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            drawAddOtherMenuItems();
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_PAINT_ROLLER " Colors");
        DrawTooltipIfItemHovered("Change scene display colors", "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

        if (ImGui::BeginPopupContextItem("##addpainttoscenepopup", ImGuiPopupFlags_MouseButtonLeft))
        {
            std::span<Color const> colors = m_Shared->getColors();
            std::span<char const* const> labels = m_Shared->getColorLabels();
            OSC_ASSERT(colors.size() == labels.size() && "every color should have a label");

            for (size_t i = 0; i < colors.size(); ++i)
            {
                Color colorVal = colors[i];
                ImGui::PushID(imguiID++);
                if (ImGui::ColorEdit4(labels[i], value_ptr(colorVal)))
                {
                    m_Shared->setColor(i, colorVal);
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::Button(ICON_FA_EYE " Visibility");
        DrawTooltipIfItemHovered("Change what's visible in the 3D scene", "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

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
        DrawTooltipIfItemHovered("Change what your mouse can interact with in the 3D scene", "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

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
            DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);
        }
    }

    std::optional<AABB> calcSceneAABB() const
    {
        return maybe_aabb_of(m_DrawablesBuffer, [](DrawableThing const& drawable) -> std::optional<AABB>
        {
            if (drawable.id != MIIDs::Empty()) {
                return calcBounds(drawable);
            }
            return std::nullopt;
        });
    }

    void draw3DViewerOverlayBottomBar()
    {
        ImGui::PushID("##3DViewerOverlay");

        // bottom-left axes overlay
        {
            CameraViewAxes axes;

            ImGuiStyle const& style = ImGui::GetStyle();
            Rect const& r = m_Shared->get3DSceneRect();
            Vec2 const topLeft =
            {
                r.p1.x + style.WindowPadding.x,
                r.p2.y - style.WindowPadding.y - axes.dimensions().y,
            };
            ImGui::SetCursorScreenPos(topLeft);
            axes.draw(m_Shared->updCamera());
        }

        Rect sceneRect = m_Shared->get3DSceneRect();
        Vec2 trPos = {sceneRect.p1.x + 100.0f, sceneRect.p2.y - 55.0f};
        ImGui::SetCursorScreenPos(trPos);

        if (ImGui::Button(ICON_FA_SEARCH_MINUS))
        {
            m_Shared->updCamera().radius *= 1.2f;
        }
        DrawTooltipIfItemHovered("Zoom Out");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_SEARCH_PLUS))
        {
            m_Shared->updCamera().radius *= 0.8f;
        }
        DrawTooltipIfItemHovered("Zoom In");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
        {
            if (std::optional<AABB> const sceneAABB = calcSceneAABB())
            {
                AutoFocus(m_Shared->updCamera(), *sceneAABB, AspectRatio(m_Shared->get3DSceneDims()));
            }
        }
        DrawTooltipIfItemHovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");

        ImGui::SameLine();

        if (ImGui::Button("X"))
        {
            m_Shared->updCamera().theta = 90_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->updCamera().theta = -90_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        DrawTooltipIfItemHovered("Face camera facing along X", "Right-clicking faces it along X, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button("Y"))
        {
            m_Shared->updCamera().theta = 0_deg;
            m_Shared->updCamera().phi = 90_deg;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->updCamera().theta = 0_deg;
            m_Shared->updCamera().phi = -90_deg;
        }
        DrawTooltipIfItemHovered("Face camera facing along Y", "Right-clicking faces it along Y, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button("Z"))
        {
            m_Shared->updCamera().theta = 0_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_Shared->updCamera().theta = 180_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        DrawTooltipIfItemHovered("Face camera facing along Z", "Right-clicking faces it along Z, but in the opposite direction");

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_CAMERA))
        {
            m_Shared->resetCamera();
        }
        DrawTooltipIfItemHovered("Reset camera", "Resets the camera to its default position (the position it's in when the wizard is first loaded)");

        ImGui::PopID();
    }

    void draw3DViewerOverlayConvertToOpenSimModelButton()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});

        constexpr CStringView mainButtonText = "Convert to OpenSim Model " ICON_FA_ARROW_RIGHT;
        constexpr CStringView settingButtonText = ICON_FA_COG;
        constexpr Vec2 spacingBetweenMainAndSettingsButtons = {1.0f, 0.0f};
        constexpr Vec2 margin = {25.0f, 35.0f};

        Vec2 const mainButtonDims = CalcButtonSize(mainButtonText);
        Vec2 const settingButtonDims = CalcButtonSize(settingButtonText);
        Vec2 const viewportBottomRight = m_Shared->get3DSceneRect().p2;

        Vec2 const buttonTopLeft =
        {
            viewportBottomRight.x - (margin.x + spacingBetweenMainAndSettingsButtons.x + settingButtonDims.x + mainButtonDims.x),
            viewportBottomRight.y - (margin.y + mainButtonDims.y),
        };

        ImGui::SetCursorScreenPos(buttonTopLeft);
        PushStyleColor(ImGuiCol_Button, Color::darkGreen());
        if (ImGui::Button(mainButtonText.c_str()))
        {
            m_Shared->tryCreateOutputModel();
        }
        PopStyleColor();

        ImGui::PopStyleVar();
        DrawTooltipIfItemHovered("Convert current scene to an OpenSim Model", "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nYour progress in this tab will remain untouched.");

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

    void drawMIObjectTooltip(MIObject const& e) const
    {
        ImGui::BeginTooltip();
        ui::Text("%s %s", e.getClass().getIconUTF8().c_str(), e.getLabel().c_str());
        ImGui::SameLine();
        ui::TextDisabled(GetContextMenuSubHeaderText(m_Shared->getModelGraph(), e));
        ImGui::EndTooltip();
    }

    void drawHoverTooltip()
    {
        if (!m_MaybeHover)
        {
            return;  // nothing is hovered
        }

        if (MIObject const* e = m_Shared->getModelGraph().tryGetByID(m_MaybeHover.ID))
        {
            drawMIObjectTooltip(*e);
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

            Document const& mg = m_Shared->getModelGraph();

            int n = 0;

            Transform ras = mg.getXFormByID(*it);
            ++it;
            ++n;


            while (it != end) {
                Transform const t = mg.getXFormByID(*it);
                ras.position += t.position;
                ras.rotation += t.rotation;
                ras.scale += t.scale;
                ++it;
                ++n;
            }

            ras.position /= static_cast<float>(n);
            ras.rotation /= static_cast<float>(n);
            ras.scale /= static_cast<float>(n);
            ras.rotation = normalize(ras.rotation);

            m_ImGuizmoState.mtx = mat4_cast(ras);
        }

        // else: is using OR nselected > 0 (so draw it)

        Rect sceneRect = m_Shared->get3DSceneRect();

        ImGuizmo::SetRect(
            sceneRect.p1.x,
            sceneRect.p1.y,
            dimensions(sceneRect).x,
            dimensions(sceneRect).y
        );
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::AllowAxisFlip(false);  // user's didn't like this feature in UX sessions

        Mat4 delta;
        SetImguizmoStyleToOSCStandard();
        bool manipulated = ImGuizmo::Manipulate(
            value_ptr(m_Shared->getCamera().view_matrix()),
            value_ptr(m_Shared->getCamera().projection_matrix(AspectRatio(sceneRect))),
            m_ImGuizmoState.op,
            m_ImGuizmoState.mode,
            value_ptr(m_ImGuizmoState.mtx),
            value_ptr(delta),
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
        Vec3 rotationDegrees;
        Vec3 scale;
        ImGuizmo::DecomposeMatrixToComponents(
            value_ptr(delta),
            value_ptr(translation),
            value_ptr(rotationDegrees),
            value_ptr(scale)
        );
        Eulers rotation = Vec<3, Degrees>{rotationDegrees};

        for (UID id : m_Shared->getCurrentSelection())
        {
            MIObject& el = m_Shared->updModelGraph().updByID(id);
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

        bool const lcClicked = IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
        bool const shiftDown = IsShiftDown();
        bool const altDown = IsAltDown();
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

        for (MIObject const& e : m_Shared->getModelGraph().iter())
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

        std::vector<DrawableThing>& MIObjects = generateDrawables();

        // hovertest the generated geometry
        m_MaybeHover = hovertestScene(MIObjects);
        handleCurrentHover();

        // assign rim highlights based on hover
        for (DrawableThing& dt : MIObjects)
        {
            dt.flags = computeFlags(m_Shared->getModelGraph(), dt.id, m_MaybeHover.ID);
        }

        // draw 3D scene (effectively, as an ImGui::Image)
        m_Shared->drawScene(MIObjects);
        if (m_Shared->isRenderHovered() && IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
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
        if (ui::BeginMenu("File"))
        {
            if (ui::MenuItem(ICON_FA_FILE " New", "Ctrl+N"))
            {
                m_Shared->requestNewMeshImporterTab();
            }

            ImGui::Separator();

            if (ui::MenuItem(ICON_FA_FOLDER_OPEN " Import", "Ctrl+O"))
            {
                m_Shared->openOsimFileAsModelGraph();
            }
            DrawTooltipIfItemHovered("Import osim into mesh importer", "Try to import an existing osim file into the mesh importer.\n\nBEWARE: the mesh importer is *not* an OpenSim model editor. The import process will delete information from your osim in order to 'jam' it into this screen. The main purpose of this button is to export/import mesh editor scenes, not to edit existing OpenSim models.");

            if (ui::MenuItem(ICON_FA_SAVE " Export", "Ctrl+S"))
            {
                m_Shared->exportModelGraphAsOsimFile();
            }
            DrawTooltipIfItemHovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            if (ui::MenuItem(ICON_FA_SAVE " Export As", "Shift+Ctrl+S"))
            {
                m_Shared->exportAsModelGraphAsOsimFile();
            }
            DrawTooltipIfItemHovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            ImGui::Separator();

            if (ui::MenuItem(ICON_FA_FOLDER_OPEN " Import Stations from CSV"))
            {
                auto popup = std::make_shared<ImportStationsFromCSVPopup>(
                    "Import Stations from CSV",
                    [state = m_Shared](auto data)
                    {
                        ActionImportLandmarks(
                            state->updCommittableModelGraph(),
                            data.landmarks,
                            data.maybeLabel
                        );
                    }
                );
                popup->open();
                m_PopupManager.push_back(std::move(popup));
            }

            ImGui::Separator();

            if (ui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W"))
            {
                m_Shared->requestClose();
            }

            if (ui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
            {
                App::upd().requestQuit();
            }

            ui::EndMenu();
        }
    }

    void drawMainMenuEditMenu()
    {
        if (ui::BeginMenu("Edit"))
        {
            if (ui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Shared->canUndoCurrentModelGraph()))
            {
                m_Shared->undoCurrentModelGraph();
            }
            if (ui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Shared->canRedoCurrentModelGraph()))
            {
                m_Shared->redoCurrentModelGraph();
            }
            ui::EndMenu();
        }
    }

    void drawMainMenuWindowMenu()
    {

        if (ui::BeginMenu("Window"))
        {
            for (size_t i = 0; i < m_Shared->getNumToggleablePanels(); ++i)
            {
                bool isEnabled = m_Shared->isNthPanelEnabled(i);
                if (ui::MenuItem(m_Shared->getNthPanelName(i).c_str(), nullptr, isEnabled))
                {
                    m_Shared->setNthPanelEnabled(i, !isEnabled);
                }
            }
            ui::EndMenu();
        }
    }

    void drawMainMenuAboutMenu()
    {
        MainMenuAboutTab{}.onDraw();
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
    ParentPtr<IMainUIStateAPI> m_Parent;
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
        Mat4 mtx = identity<Mat4>();
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mode = ImGuizmo::WORLD;
    } m_ImGuizmoState;

    // manager for active modal popups (importer popups, etc.)
    PopupManager m_PopupManager;
};


// public API (PIMPL)

osc::mi::MeshImporterTab::MeshImporterTab(
    ParentPtr<IMainUIStateAPI> const& parent_) :

    m_Impl{std::make_unique<Impl>(parent_)}
{
}

osc::mi::MeshImporterTab::MeshImporterTab(
    ParentPtr<IMainUIStateAPI> const& parent_,
    std::vector<std::filesystem::path> files_) :

    m_Impl{std::make_unique<Impl>(parent_, std::move(files_))}
{
}

osc::mi::MeshImporterTab::MeshImporterTab(MeshImporterTab&&) noexcept = default;
osc::mi::MeshImporterTab& osc::mi::MeshImporterTab::operator=(MeshImporterTab&&) noexcept = default;
osc::mi::MeshImporterTab::~MeshImporterTab() noexcept = default;

osc::UID osc::mi::MeshImporterTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::mi::MeshImporterTab::implGetName() const
{
    return m_Impl->getName();
}

bool osc::mi::MeshImporterTab::implIsUnsaved() const
{
    return m_Impl->isUnsaved();
}

bool osc::mi::MeshImporterTab::implTrySave()
{
    return m_Impl->trySave();
}

void osc::mi::MeshImporterTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::mi::MeshImporterTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::mi::MeshImporterTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::mi::MeshImporterTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::mi::MeshImporterTab::implOnDrawMainMenu()
{
    m_Impl->drawMainMenu();
}

void osc::mi::MeshImporterTab::implOnDraw()
{
    m_Impl->onDraw();
}
