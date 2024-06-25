#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSWarpResultCache.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocument.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabHover.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabUserSelection.h>

#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Widgets/PopupManager.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/UID.h>

#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace osc
{
    // top-level UI state that is shared by all UI panels
    class MeshWarpingTabSharedState final {
    public:
        MeshWarpingTabSharedState(
            UID tabID_,
            ParentPtr<ITabHost> parent_,
            std::shared_ptr<SceneCache> sceneCache_) :

            m_TabID{tabID_},
            m_TabHost{std::move(parent_)},
            m_SceneCache{std::move(sceneCache_)}
        {
            OSC_ASSERT(m_SceneCache != nullptr);
        }

        void on_mount()
        {
            m_PopupManager.on_mount();
        }

        void on_unmount()
        {}

        void on_draw()
        {
            // draw active popups over the UI
            m_PopupManager.on_draw();
        }

        const TPSDocument& getScratch() const
        {
            return m_UndoableTPSDocument->scratch();
        }

        const UndoableTPSDocument& getUndoable() const
        {
            return *m_UndoableTPSDocument;
        }

        UndoableTPSDocument& updUndoable()
        {
            return *m_UndoableTPSDocument;
        }

        std::shared_ptr<UndoableTPSDocument> getUndoableSharedPtr()
        {
            return m_UndoableTPSDocument;
        }

        const Mesh& getScratchMesh(TPSDocumentInputIdentifier which) const
        {
            return GetMesh(getScratch(), which);
        }

        const BVH& getScratchMeshBVH(TPSDocumentInputIdentifier which)
        {
            const Mesh& mesh = getScratchMesh(which);
            return updSceneCache().get_bvh(mesh);
        }

        TPSResultCache& updResultCache()
        {
            return m_WarpingCache;
        }

        // returns a (potentially cached) post-TPS-warp mesh
        const Mesh& getResultMesh()
        {
            return m_WarpingCache.getWarpedMesh(m_UndoableTPSDocument->scratch());
        }

        std::span<const Vec3> getResultNonParticipatingLandmarkLocations()
        {
            return m_WarpingCache.getWarpedNonParticipatingLandmarkLocations(m_UndoableTPSDocument->scratch());
        }

        bool isHoveringSomething() const
        {
            return m_CurrentHover.has_value();
        }

        const MeshWarpingTabHover& getCurrentHover() const
        {
            return *m_CurrentHover;
        }

        bool isHovered(const TPSDocumentElementID& id) const
        {
            return m_CurrentHover && m_CurrentHover->isHovering(id);
        }

        void setHover(const std::optional<MeshWarpingTabHover>& newHover)
        {
            m_CurrentHover = newHover;
        }

        void setHover(TPSDocumentInputIdentifier id, const Vec3& position)
        {
            m_CurrentHover.emplace(id, position);
        }

        void setHover(std::nullopt_t)
        {
            m_CurrentHover.reset();
        }

        bool hasSelection() const
        {
            // TODO: should probably gc the selection
            for (const auto& el : m_UserSelection.getUnderlyingSet()) {
                if (FindElement(getScratch(), el)) {
                    return true;
                }
            }
            return false;
        }

        bool isSelected(const TPSDocumentElementID& id) const
        {
            return m_UserSelection.contains(id);
        }

        void select(const TPSDocumentElementID& id)
        {
            m_UserSelection.select(id);
        }

        void clearSelection()
        {
            m_UserSelection.clear();
        }

        void selectAll()
        {
            for (const auto& el : GetAllElementIDs(m_UndoableTPSDocument->scratch())) {
                m_UserSelection.select(el);
            }
        }

        const std::unordered_set<TPSDocumentElementID>& getUnderlyingSelectionSet() const
        {
            return m_UserSelection.getUnderlyingSet();
        }

        void closeTab()
        {
            m_TabHost->close_tab(m_TabID);
        }

        bool canUndo() const
        {
            return m_UndoableTPSDocument->can_undo();
        }

        void undo()
        {
            m_UndoableTPSDocument->undo();
        }

        bool canRedo() const
        {
            return m_UndoableTPSDocument->can_redo();
        }

        void redo()
        {
            m_UndoableTPSDocument->redo();
        }

        template<std::derived_from<IPopup> TPopup, class... Args>
        requires std::constructible_from<TPopup, Args&&...>
        void emplacePopup(Args&&... args)
        {
            auto p = std::make_shared<TPopup>(std::forward<Args>(args)...);
            p->open();
            m_PopupManager.push_back(std::move(p));
        }

        const Material& wireframe_material() const { return m_WireframeMaterial; }
        const Mesh& getLandmarkSphereMesh() const { return m_LandmarkSphere; }
        SceneCache& updSceneCache() { return *m_SceneCache; }

        Vec2 getOverlayPadding() const { return {10.0f, 10.0f}; }
        Color getPairedLandmarkColor() const { return Color::green(); }
        Color getUnpairedLandmarkColor() const { return Color::red(); }
        Color getNonParticipatingLandmarkColor() const { return Color::purple(); }

        const PolarPerspectiveCamera& getLinkedBaseCamera() const { return m_LinkedCameraBase; }
        bool isCamerasLinked() const { return m_LinkCameras; }
        void setCamerasLinked(bool v) { m_LinkCameras = v; }
        bool isOnlyCameraRotationLinked() const { return m_OnlyLinkRotation; }
        void setOnlyCameraRotationLinked(bool v) { m_OnlyLinkRotation = v; }
        bool updateOneCameraFromLinkedBase(PolarPerspectiveCamera& camera)
        {
            // if the cameras are linked together, ensure this camera is updated from the linked camera
            if (isCamerasLinked() and camera != getLinkedBaseCamera()) {
                if (isOnlyCameraRotationLinked()) {
                    camera.phi = m_LinkedCameraBase.phi;
                    camera.theta = m_LinkedCameraBase.theta;
                } else {
                    camera = m_LinkedCameraBase;
                }
                return true;
            }
            return false;
        }
        void setLinkedBaseCamera(const PolarPerspectiveCamera& newCamera)
        {
            m_LinkedCameraBase = newCamera;
        }

    private:
        // ID of the top-level TPS3D tab
        UID m_TabID;

        // handle to the screen that owns the TPS3D tab
        ParentPtr<ITabHost> m_TabHost;

        // cached TPS3D algorithm result (to prevent recomputing it over and over)
        TPSResultCache m_WarpingCache;

        // the document that the user is editing
        std::shared_ptr<UndoableTPSDocument> m_UndoableTPSDocument = std::make_shared<UndoableTPSDocument>();

        // `true` if the user wants the cameras to be linked
        bool m_LinkCameras = true;

        // `true` if `LinkCameras` should only link the rotational parts of the cameras
        bool m_OnlyLinkRotation = false;

        // shared linked camera
        PolarPerspectiveCamera m_LinkedCameraBase = create_camera_focused_on(m_UndoableTPSDocument->scratch().sourceMesh.bounds());

        // shared scene cache, to minimize rendering effort when redrawing
        std::shared_ptr<SceneCache> m_SceneCache;

        // wireframe material, used to draw scene elements in a wireframe style
        MeshBasicMaterial m_WireframeMaterial = m_SceneCache->wireframe_material();

        // cached sphere mesh (to prevent re-generating a sphere over and over)
        Mesh m_LandmarkSphere = m_SceneCache->sphere_mesh();

        // current user selection
        MeshWaringTabUserSelection m_UserSelection;

        // current user hover: reset per-frame
        std::optional<MeshWarpingTabHover> m_CurrentHover;

        // currently active tab-wide popups
        PopupManager m_PopupManager;
    };
}
