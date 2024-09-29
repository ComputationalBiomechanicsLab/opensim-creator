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
#include <OpenSimCreator/Platform/OSCColors.h>
#include <OpenSimCreator/UI/MainUIScreen.h>
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
#include <oscar/Platform/Event.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/UI/Events/CloseTabEvent.h>
#include <oscar/UI/Events/OpenTabEvent.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/PerfPanel.h>
#include <oscar/UI/Panels/UndoRedoPanel.h>
#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/UI/Widgets/CameraViewAxes.h>
#include <oscar/UI/Widgets/LogViewer.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/LifetimedPtr.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/UID.h>

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
class osc::mi::MeshImporterTab::Impl final :
    public TabPrivate,
    public IMeshImporterUILayerHost {
public:
    explicit Impl(
        MeshImporterTab& owner,
        MainUIScreen& parent_) :

        TabPrivate{owner, &parent_, "MeshImporterTab"},
        m_Parent{parent_.weak_ref()},
        m_Shared{std::make_shared<MeshImporterSharedState>()}
    {}

    explicit Impl(
        MeshImporterTab& owner,
        MainUIScreen& parent_,
        std::vector<std::filesystem::path> meshPaths_) :

        TabPrivate{owner, &parent_, "MeshImporterTab"},
        m_Parent{parent_.weak_ref()},
        m_Shared{std::make_shared<MeshImporterSharedState>(std::move(meshPaths_))}
    {}

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

    void on_mount()
    {
        App::upd().make_main_loop_waiting();
        m_PopupManager.on_mount();
    }

    void on_unmount()
    {
        App::upd().make_main_loop_polling();
    }

    bool onEvent(Event& e)
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

    void on_tick()
    {
        const auto dt = static_cast<float>(App::get().frame_delta_since_last_frame().count());

        m_Shared->tick(dt);

        if (m_Maybe3DViewerModal)
        {
            std::shared_ptr<MeshImporterUILayer> ptr = m_Maybe3DViewerModal;  // ensure it stays alive - even if it pops itself during the drawcall
            ptr->tick(dt);
        }

        // if some screen generated an OpenSim::Model, transition to the main editor
        if (m_Shared->hasOutputModel())
        {
            m_Parent->add_and_select_tab<ModelEditorTab>(
                *m_Parent,
                std::move(m_Shared->updOutputModel()),
                m_Shared->getSceneScaleFactor()
            );
        }

        set_name(m_Shared->getRecommendedTitle());

        if (m_Shared->isCloseRequested())
        {
            App::post_event<CloseTabEvent>(*m_Parent, id());
            m_Shared->resetRequestClose();
        }

        if (m_Shared->isNewMeshImpoterTabRequested())
        {
            App::post_event<OpenTabEvent>(*m_Parent, std::make_unique<MeshImporterTab>(*m_Parent));
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
        ui::enable_dockspace_over_main_viewport();

        // handle keyboards using ImGui's input poller
        if (!m_Maybe3DViewerModal)
        {
            updateFromImGuiKeyboardState();
        }

        if (!m_Maybe3DViewerModal && m_Shared->isRenderHovered() && !m_Gizmo.is_using())
        {
            ui::update_polar_camera_from_mouse_inputs(m_Shared->updCamera(), m_Shared->get3DSceneDims());
        }

        // draw history panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex::History))
        {
            bool v = true;
            if (ui::begin_panel("history", &v))
            {
                drawHistoryPanelContent();
            }
            ui::end_panel();

            m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex::History, v);
        }

        // draw navigator panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex::Navigator))
        {
            bool v = true;
            if (ui::begin_panel("navigator", &v))
            {
                drawNavigatorPanelContent();
            }
            ui::end_panel();

            m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex::Navigator, v);
        }

        // draw log panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex::Log))
        {
            bool v = true;
            if (ui::begin_panel("Log", &v, ui::WindowFlag::MenuBar))
            {
                m_Shared->updLogViewer().on_draw();
            }
            ui::end_panel();

            m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex::Log, v);
        }

        // draw performance panel (if enabled)
        if (m_Shared->isPanelEnabled(MeshImporterSharedState::PanelIndex::Performance))
        {
            PerfPanel& pp = m_Shared->updPerfPanel();

            pp.open();
            pp.on_draw();
            if (!pp.is_open())
            {
                m_Shared->setPanelEnabled(MeshImporterSharedState::PanelIndex::Performance, false);
            }
        }

        // draw contextual 3D modal (if there is one), else: draw standard 3D viewer
        drawMainViewerPanelOrModal();

        // draw any active popups over the scene
        m_PopupManager.on_draw();
    }

private:

    //
    // ACTIONS
    //

    // pop the current UI layer
    void implRequestPop(MeshImporterUILayer&) final
    {
        m_Maybe3DViewerModal.reset();
        App::upd().request_redraw();
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

        const Document& mg = m_Shared->getModelGraph();

        const MIObject* hoveredMIObject = mg.tryGetByID(m_MaybeHover.ID);

        if (!hoveredMIObject)
        {
            return;  // current hover isn't in the current model graph
        }

        UID maybeID = GetStationAttachmentParent(mg, *hoveredMIObject);

        if (maybeID == MIIDs::Ground() || maybeID == MIIDs::Empty())
        {
            return;  // can't attach to it as-if it were a body
        }

        const auto* bodyEl = mg.tryGetByID<Body>(maybeID);
        if (!bodyEl)
        {
            return;  // suggested attachment parent isn't in the current model graph?
        }

        transitionToChoosingJointParent(*bodyEl);
    }

    // try transitioning the shown UI layer to one where the user is assigning a mesh
    void tryTransitionToAssigningHoverAndSelectionNextFrame()
    {
        const Document& mg = m_Shared->getModelGraph();

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
    void transitionToAssigningMeshesNextFrame(const std::unordered_set<UID>& meshes, const std::unordered_set<UID>& existingAttachments)
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
    void transitionToChoosingJointParent(const Body& child)
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

        const MIObject* old = m_Shared->getModelGraph().tryGetByID(el.getCrossReferenceConnecteeID(crossrefIdx));

        if (!old)
        {
            return;  // old el doesn't exist?
        }

        ChooseElLayerOptions opts;
        opts.canChooseBodies = (dynamic_cast<const Body*>(old) != nullptr) || (dynamic_cast<const Ground*>(old) != nullptr);
        opts.canChooseGround = (dynamic_cast<const Body*>(old) != nullptr) || (dynamic_cast<const Ground*>(old) != nullptr);
        opts.canChooseJoints = dynamic_cast<const Joint*>(old) != nullptr;
        opts.canChooseMeshes = dynamic_cast<const Mesh*>(old) != nullptr;
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
        const Document& mg = m_Shared->getModelGraph();

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
        if (ui::wants_keyboard())
        {
            return false;
        }

        bool shiftDown = ui::is_shift_down();
        bool ctrlOrSuperDown = ui::is_ctrl_or_super_down();

        if (ctrlOrSuperDown && ui::is_key_pressed(Key::N))
        {
            // Ctrl+N: new scene
            m_Shared->requestNewMeshImporterTab();
            return true;
        }
        else if (ctrlOrSuperDown && ui::is_key_pressed(Key::O))
        {
            // Ctrl+O: open osim
            m_Shared->openOsimFileAsModelGraph();
            return true;
        }
        else if (ctrlOrSuperDown && shiftDown && ui::is_key_pressed(Key::S))
        {
            // Ctrl+Shift+S: export as: export scene as osim to user-specified location
            m_Shared->exportAsModelGraphAsOsimFile();
            return true;
        }
        else if (ctrlOrSuperDown && ui::is_key_pressed(Key::S))
        {
            // Ctrl+S: export: export scene as osim according to typical export heuristic
            m_Shared->exportModelGraphAsOsimFile();
            return true;
        }
        else if (ctrlOrSuperDown && ui::is_key_pressed(Key::Q))
        {
            // Ctrl+Q: quit application
            App::upd().request_quit();
            return true;
        }
        else if (ctrlOrSuperDown && ui::is_key_pressed(Key::A))
        {
            // Ctrl+A: select all
            m_Shared->selectAll();
            return true;
        }
        else if (ctrlOrSuperDown && shiftDown && ui::is_key_pressed(Key::Z))
        {
            // Ctrl+Shift+Z: redo
            m_Shared->redoCurrentModelGraph();
            return true;
        }
        else if (ctrlOrSuperDown && ui::is_key_pressed(Key::Z))
        {
            // Ctrl+Z: undo
            m_Shared->undoCurrentModelGraph();
            return true;
        }
        else if (ui::any_of_keys_down({Key::Delete, Key::Backspace}))
        {
            // Delete/Backspace: delete any selected elements
            deleteSelected();
            return true;
        }
        else if (ui::is_key_pressed(Key::B))
        {
            // B: add body to hovered element
            tryAddBodyToHoveredElement();
            return true;
        }
        else if (ui::is_key_pressed(Key::A))
        {
            // A: assign a parent for the hovered element
            tryTransitionToAssigningHoverAndSelectionNextFrame();
            return true;
        }
        else if (ui::is_key_pressed(Key::J))
        {
            // J: try to create a joint
            tryCreatingJointFromHoveredElement();
            return true;
        }
        else if (ui::is_key_pressed(Key::T))
        {
            // T: try to add a station to the current hover
            tryAddingStationAtMousePosToHoveredElement();
            return true;
        }
        else if (m_Gizmo.handle_keyboard_inputs())
        {
            return true;
        }
        else if (ui::update_polar_camera_from_keyboard_inputs(m_Shared->updCamera(), m_Shared->get3DSceneRect(), calcSceneAABB()))
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
        ui::draw_text(OSC_ICON_BOLT " Actions");
        ui::same_line();
        ui::draw_text_disabled("(nothing clicked)");
        ui::draw_separator();
    }

    void drawMIObjectContextMenuContentHeader(const MIObject& e)
    {
        ui::draw_text("%s %s", e.getClass().getIconUTF8().c_str(), e.getLabel().c_str());
        ui::same_line();
        ui::draw_text_disabled(GetContextMenuSubHeaderText(m_Shared->getModelGraph(), e));
        ui::same_line();
        ui::draw_help_marker(e.getClass().getName(), e.getClass().getDescription());
        ui::draw_separator();
    }

    void drawMIObjectPropEditors(const MIObject& e)
    {
        Document& mg = m_Shared->updModelGraph();

        // label/name editor
        if (e.canChangeLabel())
        {
            std::string buf{static_cast<std::string_view>(e.getLabel())};
            if (ui::draw_string_input("Name", buf))
            {
                mg.updByID(e.getID()).setLabel(buf);
            }
            if (ui::is_item_deactivated_after_edit())
            {
                std::stringstream ss;
                ss << "changed " << e.getClass().getName() << " name";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker("Component Name", "This is the name that the component will have in the exported OpenSim model.");
        }

        // position editor
        if (e.canChangePosition())
        {
            Vec3 translation = e.getPos(mg);
            if (ui::draw_vec3_input("Translation", translation, "%.6f"))
            {
                mg.updByID(e.getID()).setPos(mg, translation);
            }
            if (ui::is_item_deactivated_after_edit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s translation";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker("Translation", MIStrings::c_TranslationDescription);
        }

        // rotation editor
        if (e.canChangeRotation())
        {
            EulerAngles eulers = to_euler_angles(normalize(e.rotation(m_Shared->getModelGraph())));

            if (ui::draw_angle3_input("Rotation", eulers, "%.6f"))
            {
                Quat quatRads = osc::to_worldspace_rotation_quat(eulers);
                mg.updByID(e.getID()).set_rotation(mg, quatRads);
            }
            if (ui::is_item_deactivated_after_edit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s rotation";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker("Rotation", "These are the rotation Euler angles for the component in ground. Positive rotations are anti-clockwise along that axis.\n\nNote: the numbers may contain slight rounding error, due to backend constraints. Your values *should* be accurate to a few decimal places.");
        }

        // scale factor editor
        if (e.canChangeScale())
        {
            Vec3 scaleFactors = e.getScale(mg);
            if (ui::draw_vec3_input("Scale", scaleFactors, "%.6f"))
            {
                mg.updByID(e.getID()).setScale(mg, scaleFactors);
            }
            if (ui::is_item_deactivated_after_edit())
            {
                std::stringstream ss;
                ss << "changed " << e.getLabel() << "'s scale";
                m_Shared->commitCurrentModelGraph(std::move(ss).str());
            }
            ui::same_line();
            ui::draw_help_marker("Scale", "These are the scale factors of the component in ground. These scale-factors are applied to the element before any other transform (it scales first, then rotates, then translates).");
        }
    }

    // draw content of "Add" menu for some scene element
    void drawAddOtherToMIObjectActions(MIObject& el, const Vec3& clickPos)
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});
        const ScopeGuard g1{[]() { ui::pop_style_var(); }};

        int imguiID = 0;
        ui::push_id(imguiID++);
        const ScopeGuard g2{[]() { ui::pop_id(); }};

        if (CanAttachMeshTo(el))
        {
            if (ui::draw_menu_item(OSC_ICON_CUBE " Meshes"))
            {
                m_Shared->pushMeshLoadRequests(el.getID(), m_Shared->promptUserForMeshFiles());
            }
            ui::draw_tooltip_if_item_hovered("Add Meshes", MIStrings::c_MeshDescription);
        }
        ui::pop_id();

        ui::push_id(imguiID++);
        if (el.hasPhysicalSize())
        {
            if (ui::begin_menu(OSC_ICON_CIRCLE " Body"))
            {
                if (ui::draw_menu_item(OSC_ICON_COMPRESS_ARROWS_ALT " at center"))
                {
                    AddBody(m_Shared->updCommittableModelGraph(), el.getPos(m_Shared->getModelGraph()), el.getID());
                }
                ui::draw_tooltip_if_item_hovered("Add Body", MIStrings::c_BodyDescription);

                if (ui::draw_menu_item(OSC_ICON_MOUSE_POINTER " at click position"))
                {
                    AddBody(m_Shared->updCommittableModelGraph(), clickPos, el.getID());
                }
                ui::draw_tooltip_if_item_hovered("Add Body", MIStrings::c_BodyDescription);

                if (ui::draw_menu_item(OSC_ICON_DOT_CIRCLE " at ground"))
                {
                    AddBody(m_Shared->updCommittableModelGraph());
                }
                ui::draw_tooltip_if_item_hovered("Add body", MIStrings::c_BodyDescription);

                if (const auto* mesh = dynamic_cast<const Mesh*>(&el))
                {
                    if (ui::draw_menu_item(OSC_ICON_BORDER_ALL " at bounds center"))
                    {
                        const Vec3 location = centroid_of(mesh->calcBounds());
                        AddBody(m_Shared->updCommittableModelGraph(), location, mesh->getID());
                    }
                    ui::draw_tooltip_if_item_hovered("Add Body", MIStrings::c_BodyDescription);

                    if (ui::draw_menu_item(OSC_ICON_DIVIDE " at mesh average center"))
                    {
                        const Vec3 location = AverageCenter(*mesh);
                        AddBody(m_Shared->updCommittableModelGraph(), location, mesh->getID());
                    }
                    ui::draw_tooltip_if_item_hovered("Add Body", MIStrings::c_BodyDescription);

                    if (ui::draw_menu_item(OSC_ICON_WEIGHT " at mesh mass center"))
                    {
                        const Vec3 location = mass_center_of(*mesh);
                        AddBody(m_Shared->updCommittableModelGraph(), location, mesh->getID());
                    }
                    ui::draw_tooltip_if_item_hovered("Add body", MIStrings::c_BodyDescription);
                }

                ui::end_menu();
            }
        }
        else
        {
            if (ui::draw_menu_item(OSC_ICON_CIRCLE " Body"))
            {
                AddBody(m_Shared->updCommittableModelGraph(), el.getPos(m_Shared->getModelGraph()), el.getID());
            }
            ui::draw_tooltip_if_item_hovered("Add Body", MIStrings::c_BodyDescription);
        }
        ui::pop_id();

        ui::push_id(imguiID++);
        if (const auto* body = dynamic_cast<const Body*>(&el))
        {
            if (ui::draw_menu_item(OSC_ICON_LINK " Joint"))
            {
                transitionToChoosingJointParent(*body);
            }
            ui::draw_tooltip_if_item_hovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }
        ui::pop_id();

        ui::push_id(imguiID++);
        if (CanAttachStationTo(el))
        {
            if (el.hasPhysicalSize())
            {
                if (ui::begin_menu(OSC_ICON_MAP_PIN " Station"))
                {
                    if (ui::draw_menu_item(OSC_ICON_COMPRESS_ARROWS_ALT " at center"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, el.getPos(m_Shared->getModelGraph()));
                    }
                    ui::draw_tooltip_if_item_hovered("Add Station", MIStrings::c_StationDescription);

                    if (ui::draw_menu_item(OSC_ICON_MOUSE_POINTER " at click position"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, clickPos);
                    }
                    ui::draw_tooltip_if_item_hovered("Add Station", MIStrings::c_StationDescription);

                    if (ui::draw_menu_item(OSC_ICON_DOT_CIRCLE " at ground"))
                    {
                        AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, Vec3{});
                    }
                    ui::draw_tooltip_if_item_hovered("Add Station", MIStrings::c_StationDescription);

                    if (dynamic_cast<const Mesh*>(&el))
                    {
                        if (ui::draw_menu_item(OSC_ICON_BORDER_ALL " at bounds center"))
                        {
                            AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, centroid_of(el.calcBounds(m_Shared->getModelGraph())));
                        }
                        ui::draw_tooltip_if_item_hovered("Add Station", MIStrings::c_StationDescription);
                    }

                    ui::end_menu();
                }
            }
            else
            {
                if (ui::draw_menu_item(OSC_ICON_MAP_PIN " Station"))
                {
                    AddStationAtLocation(m_Shared->updCommittableModelGraph(), el, el.getPos(m_Shared->getModelGraph()));
                }
                ui::draw_tooltip_if_item_hovered("Add Station", MIStrings::c_StationDescription);
            }
        }
        // ~ScopeGuard: implicitly calls ui::pop_id()
    }

    void drawNothingActions()
    {
        if (ui::draw_menu_item(OSC_ICON_CUBE " Add Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        ui::draw_tooltip_if_item_hovered("Add Meshes to the model", MIStrings::c_MeshDescription);

        if (ui::begin_menu(OSC_ICON_PLUS " Add Other"))
        {
            drawAddOtherMenuItems();

            ui::end_menu();
        }
    }

    void drawMIObjectActions(MIObject& el, const Vec3& clickPos)
    {
        if (ui::draw_menu_item(OSC_ICON_CAMERA " Focus camera on this"))
        {
            m_Shared->focusCameraOn(centroid_of(el.calcBounds(m_Shared->getModelGraph())));
        }
        ui::draw_tooltip_if_item_hovered("Focus camera on this scene element", "Focuses the scene camera on this element. This is useful for tracking the camera around that particular object in the scene");

        if (ui::begin_menu(OSC_ICON_PLUS " Add"))
        {
            drawAddOtherToMIObjectActions(el, clickPos);
            ui::end_menu();
        }

        if (const auto* body = dynamic_cast<const Body*>(&el))
        {
            if (ui::draw_menu_item(OSC_ICON_LINK " Join to"))
            {
                transitionToChoosingJointParent(*body);
            }
            ui::draw_tooltip_if_item_hovered("Creating Joints", "Create a joint from this body (the \"child\") to some other body in the model (the \"parent\").\n\nAll bodies in an OpenSim model must eventually connect to ground via joints. If no joint is added to the body then OpenSim Creator will automatically add a WeldJoint between the body and ground.");
        }

        if (el.canDelete())
        {
            if (ui::draw_menu_item(OSC_ICON_TRASH " Delete"))
            {
                DeleteObject(m_Shared->updCommittableModelGraph(), el.getID());
                garbageCollectStaleRefs();
                ui::close_current_popup();
            }
            ui::draw_tooltip_if_item_hovered("Delete", "Deletes the component from the model. Deletion is undo-able (use the undo/redo feature). Anything attached to this element (e.g. joints, meshes) will also be deleted.");
        }
    }

    // draw the "Translate" menu for any generic `MIObject`
    void drawTranslateMenu(MIObject& el)
    {
        if (!el.canChangePosition())
        {
            return;  // can't change its position
        }

        if (!ui::begin_menu(OSC_ICON_ARROWS_ALT " Translate"))
        {
            return;  // top-level menu isn't open
        }

        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

        for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
        {
            std::string label = "To " + el.getCrossReferenceLabel(i);
            if (ui::draw_menu_item(label))
            {
                TryTranslateObjectToAnotherObject(m_Shared->updCommittableModelGraph(), el.getID(), el.getCrossReferenceConnecteeID(i));
            }
        }

        if (ui::draw_menu_item("To (select something)"))
        {
            transitionToChoosingWhichElementToTranslateTo(el);
        }

        if (el.getNumCrossReferences() == 2)
        {
            std::string label = "Between " + el.getCrossReferenceLabel(0) + " and " + el.getCrossReferenceLabel(1);
            if (ui::draw_menu_item(label))
            {
                UID a = el.getCrossReferenceConnecteeID(0);
                UID b = el.getCrossReferenceConnecteeID(1);
                TryTranslateBetweenTwoObjects(m_Shared->updCommittableModelGraph(), el.getID(), a, b);
            }
        }

        if (ui::draw_menu_item("Between two scene elements"))
        {
            transitionToChoosingElementsToTranslateBetween(el);
        }

        if (ui::draw_menu_item("Between two mesh points"))
        {
            transitionToTranslatingElementAlongTwoMeshPoints(el);
        }

        if (ui::draw_menu_item("To mesh bounds center"))
        {
            transitionToTranslatingElementToMeshBoundsCenter(el);
        }
        ui::draw_tooltip_if_item_hovered("Translate to mesh bounds center", "Translates the given element to the center of the selected mesh's bounding box. The bounding box is the smallest box that contains all mesh vertices");

        if (ui::draw_menu_item("To mesh average center"))
        {
            transitionToTranslatingElementToMeshAverageCenter(el);
        }
        ui::draw_tooltip_if_item_hovered("Translate to mesh average center", "Translates the given element to the average center point of vertices in the selected mesh.\n\nEffectively, this adds each vertex location in the mesh, divides the sum by the number of vertices in the mesh, and sets the translation of the given object to that location.");

        if (ui::draw_menu_item("To mesh mass center"))
        {
            transitionToTranslatingElementToMeshMassCenter(el);
        }
        ui::draw_tooltip_if_item_hovered("Translate to mesh mess center", "Translates the given element to the mass center of the selected mesh.\n\nCAREFUL: the algorithm used to do this heavily relies on your triangle winding (i.e. normals) being correct and your mesh being a closed surface. If your mesh doesn't meet these requirements, you might get strange results (apologies: the only way to get around that problems involves complicated voxelization and leak-detection algorithms :( )");

        ui::pop_style_var();
        ui::end_menu();
    }

    // draw the "Reorient" menu for any generic `MIObject`
    void drawReorientMenu(MIObject& el)
    {
        if (!el.canChangeRotation())
        {
            return;  // can't change its rotation
        }

        if (!ui::begin_menu(OSC_ICON_REDO " Reorient"))
        {
            return;  // top-level menu isn't open
        }

        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

        {
            auto DrawMenuContent = [&](int axis)
            {
                for (int i = 0, len = el.getNumCrossReferences(); i < len; ++i)
                {
                    std::string label = "Towards " + el.getCrossReferenceLabel(i);

                    if (ui::draw_menu_item(label))
                    {
                        point_axis_towards(m_Shared->updCommittableModelGraph(), el.getID(), axis, el.getCrossReferenceConnecteeID(i));
                    }
                }

                if (ui::draw_menu_item("Towards (select something)"))
                {
                    transitionToChoosingWhichElementToPointAxisTowards(el, axis);
                }

                if (ui::draw_menu_item("Along line between (select two elements)"))
                {
                    transitionToChoosingTwoElementsToAlignAxisAlong(el, axis);
                }

                if (ui::draw_menu_item("90 degress"))
                {
                    RotateAxis(m_Shared->updCommittableModelGraph(), el, axis, 90_deg);
                }

                if (ui::draw_menu_item("180 degrees"))
                {
                    RotateAxis(m_Shared->updCommittableModelGraph(), el, axis, 180_deg);
                }

                if (ui::draw_menu_item("Along two mesh points"))
                {
                    transitionToOrientingElementAlongTwoMeshPoints(el, axis);
                }
            };

            if (ui::begin_menu("x"))
            {
                DrawMenuContent(0);
                ui::end_menu();
            }

            if (ui::begin_menu("y"))
            {
                DrawMenuContent(1);
                ui::end_menu();
            }

            if (ui::begin_menu("z"))
            {
                DrawMenuContent(2);
                ui::end_menu();
            }
        }

        if (ui::draw_menu_item("copy"))
        {
            transitionToCopyingSomethingElsesOrientation(el);
        }

        if (ui::draw_menu_item("reset"))
        {
            el.setXform(m_Shared->getModelGraph(), Transform{.position = el.getPos(m_Shared->getModelGraph())});
            m_Shared->commitCurrentModelGraph("reset " + el.getLabel() + " orientation");
        }

        ui::pop_style_var();
        ui::end_menu();
    }

    // draw the "Mass" editor for a `BodyEl`
    void drawMassEditor(const Body& bodyEl)
    {
        auto curMass = static_cast<float>(bodyEl.getMass());
        if (ui::draw_float_input("Mass", &curMass, 0.0f, 0.0f, "%.6f"))
        {
            m_Shared->updModelGraph().updByID<Body>(bodyEl.getID()).setMass(static_cast<double>(curMass));
        }
        if (ui::is_item_deactivated_after_edit())
        {
            m_Shared->commitCurrentModelGraph("changed body mass");
        }
        ui::same_line();
        ui::draw_help_marker("Mass", "The mass of the body. OpenSim defines this as 'unitless'; however, models conventionally use kilograms.");
    }

    // draw the "Joint Type" editor for a `JointEl`
    void drawJointTypeEditor(const Joint& jointEl)
    {
        if (ui::begin_combobox("Joint Type", jointEl.getSpecificTypeName())) {
            for (const auto& joint : GetComponentRegistry<OpenSim::Joint>()) {
                if (ui::draw_selectable(joint.name(), joint.name() == jointEl.getSpecificTypeName())) {
                    m_Shared->updModelGraph().updByID<Joint>(jointEl.getID()).setSpecificTypeName(joint.name());
                    m_Shared->commitCurrentModelGraph("changed joint type");
                }
            }
            ui::end_combobox();
        }
        ui::same_line();
        ui::draw_help_marker("Joint Type", "This is the type of joint that should be added into the OpenSim model. The joint's type dictates what types of motion are permitted around the joint center. See the official OpenSim documentation for an explanation of each joint type.");
    }

    // draw the "Reassign Connection" menu, which lets users change an element's cross reference
    void drawReassignCrossrefMenu(MIObject& el)
    {
        int nRefs = el.getNumCrossReferences();

        if (nRefs == 0)
        {
            return;
        }

        if (ui::begin_menu(OSC_ICON_EXTERNAL_LINK_ALT " Reassign Connection"))
        {
            ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

            for (int i = 0; i < nRefs; ++i)
            {
                CStringView label = el.getCrossReferenceLabel(i);
                if (ui::draw_menu_item(label))
                {
                    transitionToReassigningCrossRef(el, i);
                }
            }

            ui::pop_style_var();
            ui::end_menu();
        }
    }

    void actionPromptUserToSaveMeshAsOBJ(
        const osc::Mesh& mesh)
    {
        // prompt user for a save location
        const std::optional<std::filesystem::path> maybeUserSaveLocation =
            prompt_user_for_file_save_location_add_extension_if_necessary("obj");
        if (!maybeUserSaveLocation)
        {
            return;  // user didn't select a save location
        }
        const std::filesystem::path& userSaveLocation = *maybeUserSaveLocation;

        // write transformed mesh to output
        std::ofstream outputFileStream
        {
            userSaveLocation,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary,
        };
        if (!outputFileStream)
        {
            const std::string error = errno_to_string_threadsafe();
            log_error("%s: could not save obj output: %s", userSaveLocation.string().c_str(), error.c_str());
            return;
        }

        const AppMetadata& appMetadata = App::get().metadata();
        const ObjMetadata objMetadata
        {
            calc_full_application_name_with_version_and_build_id(appMetadata),
        };

        write_as_obj(
            outputFileStream,
            mesh,
            objMetadata,
            ObjWriterFlag::NoWriteNormals
        );
    }

    void actionPromptUserToSaveMeshAsSTL(
        const osc::Mesh& mesh)
    {
        // prompt user for a save location
        const std::optional<std::filesystem::path> maybeUserSaveLocation =
            prompt_user_for_file_save_location_add_extension_if_necessary("stl");
        if (!maybeUserSaveLocation)
        {
            return;  // user didn't select a save location
        }
        const std::filesystem::path& userSaveLocation = *maybeUserSaveLocation;

        // write transformed mesh to output
        std::ofstream outputFileStream
        {
            userSaveLocation,
            std::ios_base::out | std::ios_base::trunc | std::ios_base::binary,
        };
        if (!outputFileStream)
        {
            const std::string error = errno_to_string_threadsafe();
            log_error("%s: could not save obj output: %s", userSaveLocation.string().c_str(), error.c_str());
            return;
        }

        const AppMetadata& appMetadata = App::get().metadata();
        const StlMetadata stlMetadata
        {
            calc_full_application_name_with_version_and_build_id(appMetadata),
        };

        write_as_stl(outputFileStream, mesh, stlMetadata);
    }

    void drawSaveMeshMenu(const Mesh& el)
    {
        if (ui::begin_menu(OSC_ICON_FILE_EXPORT " Export"))
        {
            ui::draw_text_disabled("With Respect to:");
            ui::draw_separator();
            for (const MIObject& MIObject : m_Shared->getModelGraph().iter())
            {
                if (ui::begin_menu(MIObject.getLabel()))
                {
                    ui::draw_text_disabled("Format:");
                    ui::draw_separator();

                    if (ui::draw_menu_item(".obj"))
                    {
                        const Transform MIObjectToGround = MIObject.getXForm(m_Shared->getModelGraph());
                        const Transform meshVertToGround = el.getXForm();
                        const Mat4 meshVertToMIObjectVert = inverse_mat4_cast(MIObjectToGround) * mat4_cast(meshVertToGround);

                        osc::Mesh mesh = el.getMeshData();
                        mesh.transform_vertices(meshVertToMIObjectVert);
                        actionPromptUserToSaveMeshAsOBJ(mesh);
                    }

                    if (ui::draw_menu_item(".stl"))
                    {
                        const Transform MIObjectToGround = MIObject.getXForm(m_Shared->getModelGraph());
                        const Transform meshVertToGround = el.getXForm();
                        const Mat4 meshVertToMIObjectVert = inverse_mat4_cast(MIObjectToGround) * mat4_cast(meshVertToGround);

                        osc::Mesh mesh = el.getMeshData();
                        mesh.transform_vertices(meshVertToMIObjectVert);
                        actionPromptUserToSaveMeshAsSTL(mesh);
                    }

                    ui::end_menu();
                }
            }
            ui::end_menu();
        }
    }

    void drawContextMenuSpacer()
    {
        ui::draw_dummy({0.0f, 0.0f});
    }

    // draw context menu content for when user right-clicks nothing
    void drawNothingContextMenuContent()
    {
        drawNothingContextMenuContentHeader();
        drawContextMenuSpacer();
        drawNothingActions();
    }

    // draw context menu content for a `GroundEl`
    void drawContextMenuContent(Ground& el, const Vec3& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);
        drawContextMenuSpacer();
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `BodyEl`
    void drawContextMenuContent(Body& el, const Vec3& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);
        drawMassEditor(el);

        drawContextMenuSpacer();
        ui::draw_separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `Mesh`
    void drawContextMenuContent(Mesh& el, const Vec3& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);

        drawContextMenuSpacer();
        ui::draw_separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawSaveMeshMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `JointEl`
    void drawContextMenuContent(Joint& el, const Vec3& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);
        drawJointTypeEditor(el);

        drawContextMenuSpacer();
        ui::draw_separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for a `StationEl`
    void drawContextMenuContent(StationEl& el, const Vec3& clickPos)
    {
        drawMIObjectContextMenuContentHeader(el);

        drawContextMenuSpacer();

        drawMIObjectPropEditors(el);

        drawContextMenuSpacer();
        ui::draw_separator();
        drawContextMenuSpacer();

        drawTranslateMenu(el);
        drawReorientMenu(el);
        drawReassignCrossrefMenu(el);
        drawMIObjectActions(el, clickPos);
    }

    // draw context menu content for some scene element
    void drawContextMenuContent(MIObject& el, const Vec3& clickPos)
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
            ui::push_id(UID::empty());
            const ScopeGuard g{[]() { ui::pop_id(); }};
            drawNothingContextMenuContent();
        }
        else if (m_MaybeOpenedContextMenu.ID == MIIDs::RightClickedNothing())
        {
            // context menu was opened on "nothing" specifically
            ui::push_id(UID::empty());
            const ScopeGuard g{[]() { ui::pop_id(); }};
            drawNothingContextMenuContent();
        }
        else if (MIObject* el = m_Shared->updModelGraph().tryUpdByID(m_MaybeOpenedContextMenu.ID))
        {
            // context menu was opened on a scene element that exists in the modelgraph
            ui::push_id(el->getID());
            const ScopeGuard g{[]() { ui::pop_id(); }};
            drawContextMenuContent(*el, m_MaybeOpenedContextMenu.Pos);
        }


        // context menu should be closed under these conditions
        if (ui::any_of_keys_pressed({Key::Return, Key::Escape}))
        {
            m_MaybeOpenedContextMenu.reset();
            ui::close_current_popup();
        }
    }

    // draw the content of the (undo/redo) "History" panel
    void drawHistoryPanelContent()
    {
        UndoRedoPanel::draw_content(m_Shared->updCommittableModelGraph());
    }

    void drawNavigatorElement(const MIClass& c)
    {
        Document& mg = m_Shared->updModelGraph();

        ui::draw_text("%s %s", c.getIconUTF8().c_str(), c.getNamePluralized().c_str());
        ui::same_line();
        ui::draw_help_marker(c.getNamePluralized(), c.getDescription());
        ui::draw_dummy({0.0f, 5.0f});
        ui::indent();

        bool empty = true;
        for (const MIObject& el : mg.iter())
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
                ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
                ++styles;
            }
            else if (m_Shared->isSelected(id))
            {
                ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
                ++styles;
            }

            ui::draw_text(el.getLabel());

            ui::pop_style_color(styles);

            if (ui::is_item_hovered())
            {
                m_MaybeHover = {id, {}};
            }

            if (ui::is_item_clicked(ui::MouseButton::Left))
            {
                if (!ui::is_shift_down())
                {
                    m_Shared->updModelGraph().deSelectAll();
                }
                m_Shared->updModelGraph().select(id);
            }

            if (ui::is_item_clicked(ui::MouseButton::Right))
            {
                m_MaybeOpenedContextMenu = MeshImporterHover{id, {}};
                ui::open_popup("##maincontextmenu");
                App::upd().request_redraw();
            }
        }

        if (empty)
        {
            ui::draw_text_disabled("(no %s)", c.getNamePluralized().c_str());
        }
        ui::unindent();
    }

    void drawNavigatorPanelContent()
    {
        for (const MIClass& c : GetSceneElClasses())
        {
            drawNavigatorElement(c);
            ui::draw_dummy({0.0f, 5.0f});
        }

        // a navigator element might have opened the context menu in the navigator panel
        //
        // this can happen when the user right-clicks something in the navigator
        if (ui::begin_popup("##maincontextmenu"))
        {
            drawContextMenuContent();
            ui::end_popup();
        }
    }

    void drawAddOtherMenuItems()
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {10.0f, 10.0f});

        if (ui::draw_menu_item(OSC_ICON_CUBE " Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        ui::draw_tooltip_if_item_hovered("Add Meshes", MIStrings::c_MeshDescription);

        if (ui::draw_menu_item(OSC_ICON_CIRCLE " Body"))
        {
            AddBody(m_Shared->updCommittableModelGraph());
        }
        ui::draw_tooltip_if_item_hovered("Add Body", MIStrings::c_BodyDescription);

        if (ui::draw_menu_item(OSC_ICON_MAP_PIN " Station"))
        {
            Document& mg = m_Shared->updModelGraph();
            auto& e = mg.emplace<StationEl>(UID{}, MIIDs::Ground(), Vec3{}, StationEl::Class().generateName());
            mg.selectOnly(e);
        }
        ui::draw_tooltip_if_item_hovered("Add Station", StationEl::Class().getDescription());

        ui::pop_style_var();
    }

    void draw3DViewerOverlayTopBar()
    {
        int imguiID = 0;

        if (ui::draw_button(OSC_ICON_CUBE " Add Meshes"))
        {
            m_Shared->promptUserForMeshFilesAndPushThemOntoMeshLoader();
        }
        ui::draw_tooltip_if_item_hovered("Add Meshes to the model", MIStrings::c_MeshDescription);

        ui::same_line();

        ui::draw_button(OSC_ICON_PLUS " Add Other");
        ui::draw_tooltip_if_item_hovered("Add components to the model");

        if (ui::begin_popup_context_menu("##additemtoscenepopup", ui::PopupFlag::MouseButtonLeft))
        {
            drawAddOtherMenuItems();
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button(OSC_ICON_PAINT_ROLLER " Colors");
        ui::draw_tooltip_if_item_hovered("Change scene display colors", "This only changes the decroative display colors of model elements in this screen. Color changes are not saved to the exported OpenSim model. Changing these colors can be handy for spotting things, or constrasting scene elements more strongly");

        if (ui::begin_popup_context_menu("##addpainttoscenepopup", ui::PopupFlag::MouseButtonLeft))
        {
            std::span<const Color> colors = m_Shared->colors();
            std::span<const char* const> labels = m_Shared->getColorLabels();
            OSC_ASSERT(colors.size() == labels.size() && "every color should have a label");

            for (size_t i = 0; i < colors.size(); ++i)
            {
                Color colorVal = colors[i];
                ui::push_id(imguiID++);
                if (ui::draw_rgba_color_editor(labels[i], colorVal))
                {
                    m_Shared->setColor(i, colorVal);
                }
                ui::pop_id();
            }
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button(OSC_ICON_EYE " Visibility");
        ui::draw_tooltip_if_item_hovered("Change what's visible in the 3D scene", "This only changes what's visible in this screen. Visibility options are not saved to the exported OpenSim model. Changing these visibility options can be handy if you have a lot of overlapping/intercalated scene elements");

        if (ui::begin_popup_context_menu("##changevisibilitypopup", ui::PopupFlag::MouseButtonLeft))
        {
            std::span<const bool> visibilities = m_Shared->getVisibilityFlags();
            std::span<const char* const> labels = m_Shared->getVisibilityFlagLabels();
            OSC_ASSERT(visibilities.size() == labels.size() && "every visibility flag should have a label");

            for (size_t i = 0; i < visibilities.size(); ++i)
            {
                bool v = visibilities[i];
                ui::push_id(imguiID++);
                if (ui::draw_checkbox(labels[i], &v))
                {
                    m_Shared->setVisibilityFlag(i, v);
                }
                ui::pop_id();
            }
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_button(OSC_ICON_LOCK " Interactivity");
        ui::draw_tooltip_if_item_hovered("Change what your mouse can interact with in the 3D scene", "This does not prevent being able to edit the model - it only affects whether you can click that type of element in the 3D scene. Combining these flags with visibility and custom colors can be handy if you have heavily overlapping/intercalated scene elements.");

        if (ui::begin_popup_context_menu("##changeinteractionlockspopup", ui::PopupFlag::MouseButtonLeft))
        {
            std::span<const bool> interactables = m_Shared->getIneractivityFlags();
            std::span<const char* const> labels =  m_Shared->getInteractivityFlagLabels();
            OSC_ASSERT(interactables.size() == labels.size());

            for (size_t i = 0; i < interactables.size(); ++i)
            {
                bool v = interactables[i];
                ui::push_id(imguiID++);
                if (ui::draw_checkbox(labels[i], &v))
                {
                    m_Shared->setInteractivityFlag(i, v);
                }
                ui::pop_id();
            }
            ui::end_popup();
        }

        ui::same_line();

        ui::draw_gizmo_op_selector(m_Gizmo);

        ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});
        ui::same_line();
        ui::pop_style_var();

        // local/global dropdown
        ui::draw_gizmo_mode_selector(m_Gizmo);
        ui::same_line();

        // scale factor
        {
            const CStringView tooltipTitle = "Change scene scale factor";
            const CStringView tooltipDesc = "This rescales *some* elements in the scene. Specifically, the ones that have no 'size', such as body frames, joint frames, and the chequered floor texture.\n\nChanging this is handy if you are working on smaller or larger models, where the size of the (decorative) frames and floor are too large/small compared to the model you are working on.\n\nThis is purely decorative and does not affect the exported OpenSim model in any way.";

            float sf = m_Shared->getSceneScaleFactor();
            ui::set_next_item_width(ui::calc_text_size("1000.00").x);
            if (ui::draw_float_input("scene scale factor", &sf))
            {
                m_Shared->setSceneScaleFactor(sf);
            }
            ui::draw_tooltip_if_item_hovered(tooltipTitle, tooltipDesc);
        }
    }

    std::optional<AABB> calcSceneAABB() const
    {
        return maybe_bounding_aabb_of(m_DrawablesBuffer, [](const DrawableThing& drawable) -> std::optional<AABB>
        {
            if (drawable.id != MIIDs::Empty()) {
                return calcBounds(drawable);
            }
            return std::nullopt;
        });
    }

    void draw3DViewerOverlayBottomBar()
    {
        ui::push_id("##3DViewerOverlay");

        // bottom-left axes overlay
        {
            CameraViewAxes axes;

            const Vec2 windowPadding = ui::get_style_panel_padding();
            const Rect& r = m_Shared->get3DSceneRect();
            const Vec2 topLeft =
            {
                r.p1.x + windowPadding.x,
                r.p2.y - windowPadding.y - axes.dimensions().y,
            };
            ui::set_cursor_screen_pos(topLeft);
            axes.draw(m_Shared->updCamera());
        }

        Rect sceneRect = m_Shared->get3DSceneRect();
        Vec2 trPos = {sceneRect.p1.x + 100.0f, sceneRect.p2.y - 55.0f};
        ui::set_cursor_screen_pos(trPos);

        if (ui::draw_button(OSC_ICON_SEARCH_MINUS))
        {
            m_Shared->updCamera().radius *= 1.2f;
        }
        ui::draw_tooltip_if_item_hovered("Zoom Out");

        ui::same_line();

        if (ui::draw_button(OSC_ICON_SEARCH_PLUS))
        {
            m_Shared->updCamera().radius *= 0.8f;
        }
        ui::draw_tooltip_if_item_hovered("Zoom In");

        ui::same_line();

        if (ui::draw_button(OSC_ICON_EXPAND_ARROWS_ALT))
        {
            if (const std::optional<AABB> sceneAABB = calcSceneAABB())
            {
                auto_focus(m_Shared->updCamera(), *sceneAABB, aspect_ratio_of(m_Shared->get3DSceneDims()));
            }
        }
        ui::draw_tooltip_if_item_hovered("Autoscale Scene", "Zooms camera to try and fit everything in the scene into the viewer");

        ui::same_line();

        if (ui::draw_button("X"))
        {
            m_Shared->updCamera().theta = 90_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        if (ui::is_item_clicked(ui::MouseButton::Right))
        {
            m_Shared->updCamera().theta = -90_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        ui::draw_tooltip_if_item_hovered("Face camera facing along X", "Right-clicking faces it along X, but in the opposite direction");

        ui::same_line();

        if (ui::draw_button("Y"))
        {
            m_Shared->updCamera().theta = 0_deg;
            m_Shared->updCamera().phi = 90_deg;
        }
        if (ui::is_item_clicked(ui::MouseButton::Right))
        {
            m_Shared->updCamera().theta = 0_deg;
            m_Shared->updCamera().phi = -90_deg;
        }
        ui::draw_tooltip_if_item_hovered("Face camera facing along Y", "Right-clicking faces it along Y, but in the opposite direction");

        ui::same_line();

        if (ui::draw_button("Z"))
        {
            m_Shared->updCamera().theta = 0_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        if (ui::is_item_clicked(ui::MouseButton::Right))
        {
            m_Shared->updCamera().theta = 180_deg;
            m_Shared->updCamera().phi = 0_deg;
        }
        ui::draw_tooltip_if_item_hovered("Face camera facing along Z", "Right-clicking faces it along Z, but in the opposite direction");

        ui::same_line();

        if (ui::draw_button(OSC_ICON_CAMERA))
        {
            m_Shared->resetCamera();
        }
        ui::draw_tooltip_if_item_hovered("Reset camera", "Resets the camera to its default position (the position it's in when the wizard is first loaded)");

        ui::pop_id();
    }

    void draw3DViewerOverlayConvertToOpenSimModelButton()
    {
        ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});

        constexpr CStringView mainButtonText = "Convert to OpenSim Model " OSC_ICON_ARROW_RIGHT;
        constexpr CStringView settingButtonText = OSC_ICON_COG;
        constexpr Vec2 spacingBetweenMainAndSettingsButtons = {1.0f, 0.0f};
        constexpr Vec2 margin = {25.0f, 35.0f};

        const Vec2 mainButtonDims = ui::calc_button_size(mainButtonText);
        const Vec2 settingButtonDims = ui::calc_button_size(settingButtonText);
        const Vec2 viewportBottomRight = m_Shared->get3DSceneRect().p2;

        const Vec2 buttonTopLeft =
        {
            viewportBottomRight.x - (margin.x + spacingBetweenMainAndSettingsButtons.x + settingButtonDims.x + mainButtonDims.x),
            viewportBottomRight.y - (margin.y + mainButtonDims.y),
        };

        ui::set_cursor_screen_pos(buttonTopLeft);
        ui::push_style_color(ui::ColorVar::Button, Color::dark_green());
        if (ui::draw_button(mainButtonText))
        {
            m_Shared->tryCreateOutputModel();
        }
        ui::pop_style_color();

        ui::pop_style_var();
        ui::draw_tooltip_if_item_hovered("Convert current scene to an OpenSim Model", "This will attempt to convert the current scene into an OpenSim model, followed by showing the model in OpenSim Creator's OpenSim model editor screen.\n\nYour progress in this tab will remain untouched.");

        ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});
        ui::same_line(0.0f, spacingBetweenMainAndSettingsButtons.x);
        ui::draw_button(settingButtonText);
        ui::pop_style_var();

        if (ui::begin_popup_context_menu("##settingspopup", ui::PopupFlag::MouseButtonLeft))
        {
            const ModelCreationFlags flags = m_Shared->getModelCreationFlags();

            {
                bool v = flags & ModelCreationFlags::ExportStationsAsMarkers;
                if (ui::draw_checkbox("Export Stations as Markers", &v))
                {
                    const ModelCreationFlags newFlags = v ?
                        flags + ModelCreationFlags::ExportStationsAsMarkers :
                        flags - ModelCreationFlags::ExportStationsAsMarkers;
                    m_Shared->setModelCreationFlags(newFlags);
                }
            }

            ui::end_popup();
        }
    }

    void draw3DViewerOverlay()
    {
        draw3DViewerOverlayTopBar();
        draw3DViewerOverlayBottomBar();
        draw3DViewerOverlayConvertToOpenSimModelButton();
    }

    void drawMIObjectTooltip(const MIObject& e) const
    {
        ui::begin_tooltip_nowrap();
        ui::draw_text("%s %s", e.getClass().getIconUTF8().c_str(), e.getLabel().c_str());
        ui::same_line();
        ui::draw_text_disabled(GetContextMenuSubHeaderText(m_Shared->getModelGraph(), e));
        ui::end_tooltip_nowrap();
    }

    void drawHoverTooltip()
    {
        if (!m_MaybeHover)
        {
            return;  // nothing is hovered
        }

        if (const MIObject* e = m_Shared->getModelGraph().tryGetByID(m_MaybeHover.ID))
        {
            drawMIObjectTooltip(*e);
        }
    }

    // draws 3D manipulator overlays (drag handles, etc.)
    void drawSelection3DManipulatorGizmos()
    {
        if (not m_Shared->hasSelection()) {
            return;  // can only manipulate if selecting something
        }

        // if the user isn't *currently* manipulating anything, create an
        // up-to-date manipulation matrix
        //
        // this is so that we can at least *show* the manipulation axes, and
        // because the user might start manipulating during this frame
        if (not m_Gizmo.is_using()) {
            auto it = m_Shared->getCurrentSelection().begin();
            auto end = m_Shared->getCurrentSelection().end();

            if (it == end)
            {
                return;  // sanity exit
            }

            const Document& mg = m_Shared->getModelGraph();

            int n = 0;

            Transform ras = mg.getXFormByID(*it);
            ++it;
            ++n;


            while (it != end) {
                const Transform t = mg.getXFormByID(*it);
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

            m_GizmoModelMatrix = mat4_cast(ras);
        }

        // else: is using OR nselected > 0 (so draw it)

        Rect sceneRect = m_Shared->get3DSceneRect();

        const auto userManipulation = m_Gizmo.draw(
            m_GizmoModelMatrix,
            m_Shared->getCamera().view_matrix(),
            m_Shared->getCamera().projection_matrix(aspect_ratio_of(sceneRect)),
            sceneRect
        );

        if (m_Gizmo.was_using() and not m_Gizmo.is_using()) {
            // the user stopped editing, so save it and re-render
            m_Shared->commitCurrentModelGraph("manipulated selection");
            App::upd().request_redraw();
        }

        if (not userManipulation) {
            return;  // no user manipulation this frame, exit early
        }

        for (UID id : m_Shared->getCurrentSelection()) {
            MIObject& el = m_Shared->updModelGraph().updByID(id);
            switch (m_Gizmo.operation()) {
            case ui::GizmoOperation::Rotate:
                el.applyRotation(m_Shared->getModelGraph(), extract_eulers_xyz(userManipulation->rotation), Vec3{m_GizmoModelMatrix[3]});
                break;
            case ui::GizmoOperation::Translate: {
                el.applyTranslation(m_Shared->getModelGraph(), userManipulation->position);
                break;
            }
            case ui::GizmoOperation::Scale:
                el.applyScale(m_Shared->getModelGraph(), userManipulation->scale);
                break;
            default:
                break;
            }
        }
    }

    // perform a hovertest on the current 3D scene to determine what the user's mouse is over
    MeshImporterHover hovertestScene(const std::vector<DrawableThing>& drawables)
    {
        if (!m_Shared->isRenderHovered())
        {
            return m_MaybeHover;
        }

        if (m_Gizmo.is_using())
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

        const bool lcClicked = ui::is_mouse_released_without_dragging(ui::MouseButton::Left);
        const bool shiftDown = ui::is_shift_down();
        const bool altDown = ui::is_alt_down();
        const bool isUsingGizmo = m_Gizmo.is_using();

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

        for (const MIObject& e : m_Shared->getModelGraph().iter())
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

        // draw 3D scene (effectively, as an ui::Image)
        m_Shared->drawScene(MIObjects);
        if (m_Shared->isRenderHovered() && ui::is_mouse_released_without_dragging(ui::MouseButton::Right) && !m_Gizmo.is_using())
        {
            m_MaybeOpenedContextMenu = m_MaybeHover;
            ui::open_popup("##maincontextmenu");
        }

        bool ctxMenuShowing = false;
        if (ui::begin_popup("##maincontextmenu"))
        {
            ctxMenuShowing = true;
            drawContextMenuContent();
            ui::end_popup();
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
        if (ui::begin_menu("File"))
        {
            if (ui::draw_menu_item(OSC_ICON_FILE " New", "Ctrl+N"))
            {
                m_Shared->requestNewMeshImporterTab();
            }

            ui::draw_separator();

            if (ui::draw_menu_item(OSC_ICON_FOLDER_OPEN " Import", "Ctrl+O"))
            {
                m_Shared->openOsimFileAsModelGraph();
            }
            ui::draw_tooltip_if_item_hovered("Import osim into mesh importer", "Try to import an existing osim file into the mesh importer.\n\nBEWARE: the mesh importer is *not* an OpenSim model editor. The import process will delete information from your osim in order to 'jam' it into this screen. The main purpose of this button is to export/import mesh editor scenes, not to edit existing OpenSim models.");

            if (ui::draw_menu_item(OSC_ICON_SAVE " Export", "Ctrl+S"))
            {
                m_Shared->exportModelGraphAsOsimFile();
            }
            ui::draw_tooltip_if_item_hovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            if (ui::draw_menu_item(OSC_ICON_SAVE " Export As", "Shift+Ctrl+S"))
            {
                m_Shared->exportAsModelGraphAsOsimFile();
            }
            ui::draw_tooltip_if_item_hovered("Export mesh impoter scene to osim", "Try to export the current mesh importer scene to an osim.\n\nBEWARE: the mesh importer scene may not map 1:1 onto an OpenSim model, so re-importing the scene *may* change a few things slightly. The main utility of this button is to try and save some progress in the mesh importer.");

            ui::draw_separator();

            if (ui::draw_menu_item(OSC_ICON_FOLDER_OPEN " Import Stations from CSV"))
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

            ui::draw_separator();

            if (ui::draw_menu_item(OSC_ICON_TIMES " Close", "Ctrl+W"))
            {
                m_Shared->request_close();
            }

            if (ui::draw_menu_item(OSC_ICON_TIMES_CIRCLE " Quit", "Ctrl+Q"))
            {
                App::upd().request_quit();
            }

            ui::end_menu();
        }
    }

    void drawMainMenuEditMenu()
    {
        if (ui::begin_menu("Edit"))
        {
            if (ui::draw_menu_item(OSC_ICON_UNDO " Undo", "Ctrl+Z", false, m_Shared->canUndoCurrentModelGraph()))
            {
                m_Shared->undoCurrentModelGraph();
            }
            if (ui::draw_menu_item(OSC_ICON_REDO " Redo", "Ctrl+Shift+Z", false, m_Shared->canRedoCurrentModelGraph()))
            {
                m_Shared->redoCurrentModelGraph();
            }
            ui::end_menu();
        }
    }

    void drawMainMenuWindowMenu()
    {

        if (ui::begin_menu("Window"))
        {
            for (size_t i = 0; i < m_Shared->num_toggleable_panels(); ++i)
            {
                bool isEnabled = m_Shared->isNthPanelEnabled(i);
                if (ui::draw_menu_item(m_Shared->getNthPanelName(i), {}, isEnabled))
                {
                    m_Shared->setNthPanelEnabled(i, !isEnabled);
                }
            }
            ui::end_menu();
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
            const std::shared_ptr<MeshImporterUILayer> ptr = m_Maybe3DViewerModal;

            // open it "over" the whole UI as a "modal" - so that the user can't click things
            // outside of the panel
            ui::open_popup("##visualizermodalpopup");
            ui::set_next_panel_size(m_Shared->get3DSceneDims());
            ui::set_next_panel_pos(m_Shared->get3DSceneRect().p1);
            ui::push_style_var(ui::StyleVar::WindowPadding, {0.0f, 0.0f});

            const ui::WindowFlags modalFlags = {
                ui::WindowFlag::AlwaysAutoResize,
                ui::WindowFlag::NoTitleBar,
                ui::WindowFlag::NoMove,
                ui::WindowFlag::NoResize,
            };

            if (ui::begin_popup_modal("##visualizermodalpopup", nullptr, modalFlags))
            {
                ui::pop_style_var();
                ptr->onDraw();
                ui::end_popup();
            }
            else
            {
                ui::pop_style_var();
            }
        }
        else
        {
            ui::push_style_var(ui::StyleVar::WindowPadding, {0.0f, 0.0f});
            if (ui::begin_panel("wizard_3dViewer"))
            {
                ui::pop_style_var();
                draw3DViewer();
                ui::set_cursor_pos(Vec2{ui::get_cursor_start_pos()} + Vec2{10.0f, 10.0f});
                draw3DViewerOverlay();
            }
            else
            {
                ui::pop_style_var();
            }
            ui::end_panel();
        }
    }

    // tab data
    LifetimedPtr<MainUIScreen> m_Parent;

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

    // Gizmo state
    ui::Gizmo m_Gizmo;
    Mat4 m_GizmoModelMatrix = identity<Mat4>();

    // manager for active modal popups (importer popups, etc.)
    PopupManager m_PopupManager;
};


osc::mi::MeshImporterTab::MeshImporterTab(
    MainUIScreen& parent_) :

    Tab{std::make_unique<Impl>(*this, parent_)}
{}
osc::mi::MeshImporterTab::MeshImporterTab(
    MainUIScreen& parent_,
    std::vector<std::filesystem::path> files_) :

    Tab{std::make_unique<Impl>(*this, parent_, std::move(files_))}
{}
bool osc::mi::MeshImporterTab::impl_is_unsaved() const { return private_data().isUnsaved(); }
bool osc::mi::MeshImporterTab::impl_try_save() { return private_data().trySave(); }
void osc::mi::MeshImporterTab::impl_on_mount() { private_data().on_mount(); }
void osc::mi::MeshImporterTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::mi::MeshImporterTab::impl_on_event(Event& e) { return private_data().onEvent(e); }
void osc::mi::MeshImporterTab::impl_on_tick() { private_data().on_tick(); }
void osc::mi::MeshImporterTab::impl_on_draw_main_menu() { private_data().drawMainMenu(); }
void osc::mi::MeshImporterTab::impl_on_draw() { private_data().onDraw(); }
