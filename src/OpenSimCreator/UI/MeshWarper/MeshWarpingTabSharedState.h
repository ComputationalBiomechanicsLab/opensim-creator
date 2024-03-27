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
#include <oscar/Platform/App.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Widgets/PopupManager.h>
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
    struct MeshWarpingTabSharedState final {

        MeshWarpingTabSharedState(
            UID tabID_,
            ParentPtr<ITabHost> parent_) :

            tabID{tabID_},
            tabHost{std::move(parent_)}
        {
        }

        TPSDocument const& getScratch() const
        {
            return editedDocument->getScratch();
        }

        UndoableTPSDocument const& getUndoable() const
        {
            return *editedDocument;
        }

        UndoableTPSDocument& updUndoable()
        {
            return *editedDocument;
        }

        Mesh const& getScratchMesh(TPSDocumentInputIdentifier which) const
        {
            return GetMesh(getScratch(), which);
        }

        BVH const& getScratchMeshBVH(TPSDocumentInputIdentifier which)
        {
            Mesh const& mesh = getScratchMesh(which);
            return meshCache->getBVH(mesh);
        }

        // returns a (potentially cached) post-TPS-warp mesh
        Mesh const& getResultMesh()
        {
            return meshResultCache.getWarpedMesh(editedDocument->getScratch());
        }

        std::span<Vec3 const> getResultNonParticipatingLandmarkLocations()
        {
            return meshResultCache.getWarpedNonParticipatingLandmarkLocations(editedDocument->getScratch());
        }

        bool isHovered(TPSDocumentElementID const& id) const
        {
            return currentHover && currentHover->isHovering(id);
        }

        bool hasSelection() const
        {
            // TODO: should probably gc the selection
            for (auto const& el : userSelection.getUnderlyingSet())
            {
                if (FindElement(getScratch(), el))
                {
                    return true;
                }
            }
            return false;
        }

        bool isSelected(TPSDocumentElementID const& id) const
        {
            return userSelection.contains(id);
        }

        void select(TPSDocumentElementID const& id)
        {
            userSelection.select(id);
        }

        void clearSelection()
        {
            userSelection.clear();
        }

        void selectAll()
        {
            for (auto const& el : GetAllElementIDs(editedDocument->getScratch()))
            {
                userSelection.select(el);
            }
        }

        void closeTab()
        {
            tabHost->closeTab(tabID);
        }

        bool canUndo() const
        {
            return editedDocument->canUndo();
        }

        void undo()
        {
            editedDocument->undo();
        }

        bool canRedo() const
        {
            return editedDocument->canRedo();
        }

        void redo()
        {
            editedDocument->redo();
        }

        template<std::derived_from<IPopup> TPopup, class... Args>
        requires std::constructible_from<TPopup, Args&&...>
        void emplacePopup(Args&&... args)
        {
            auto p = std::make_shared<TPopup>(std::forward<Args>(args)...);
            p->open();
            popupManager.push_back(std::move(p));
        }

        // ID of the top-level TPS3D tab
        UID tabID;

        // handle to the screen that owns the TPS3D tab
        ParentPtr<ITabHost> tabHost;

        // cached TPS3D algorithm result (to prevent recomputing it each frame)
        TPSResultCache meshResultCache;

        // the document that the user is editing
        std::shared_ptr<UndoableTPSDocument> editedDocument = std::make_shared<UndoableTPSDocument>();

        // `true` if the user wants the cameras to be linked
        bool linkCameras = true;

        // `true` if `LinkCameras` should only link the rotational parts of the cameras
        bool onlyLinkRotation = false;

        // shared linked camera
        PolarPerspectiveCamera linkedCameraBase = CreateCameraFocusedOn(editedDocument->getScratch().sourceMesh.getBounds());

        // wireframe material, used to draw scene elements in a wireframe style
        MeshBasicMaterial wireframeMaterial = App::singleton<SceneCache>(App::resource_loader())->wireframeMaterial();

        // shared sphere mesh (used by rendering code)
        Mesh landmarkSphere = App::singleton<SceneCache>(App::resource_loader())->getSphereMesh();

        // current user selection
        MeshWaringTabUserSelection userSelection;

        // current user hover: reset per-frame
        std::optional<MeshWarpingTabHover> currentHover;

        // currently active tab-wide popups
        PopupManager popupManager;

        // shared mesh cache
        std::shared_ptr<SceneCache> meshCache = App::singleton<SceneCache>(App::resource_loader());

        Vec2 overlayPadding = {10.0f, 10.0f};
        Color pairedLandmarkColor = Color::green();
        Color unpairedLandmarkColor = Color::red();
        Color nonParticipatingLandmarkColor = Color::purple();
    };
}
