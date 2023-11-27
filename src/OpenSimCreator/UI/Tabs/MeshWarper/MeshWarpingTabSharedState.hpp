#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSResultCache.hpp>
#include <OpenSimCreator/Documents/MeshWarp/UndoableTPSDocument.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabHover.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabUserSelection.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneHelpers.hpp>
#include <oscar/UI/Tabs/TabHost.hpp>
#include <oscar/UI/Widgets/PopupManager.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/UID.hpp>

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
            ParentPtr<TabHost> parent_) :

            tabID{tabID_},
            tabHost{std::move(parent_)}
        {
        }

        TPSDocument const& getScratch() const
        {
            return editedDocument->getScratch();
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

        std::span<Vec3 const> getResultNonParticipatingLandmarks()
        {
            return meshResultCache.getWarpedNonParticipatingLandmarks(editedDocument->getScratch());
        }

        // ID of the top-level TPS3D tab
        UID tabID;

        // handle to the screen that owns the TPS3D tab
        ParentPtr<TabHost> tabHost;

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
        Material wireframeMaterial = CreateWireframeOverlayMaterial(
            App::config(),
            *App::singleton<ShaderCache>()
        );

        // shared sphere mesh (used by rendering code)
        Mesh landmarkSphere = App::singleton<SceneCache>()->getSphereMesh();

        // current user selection
        MeshWaringTabUserSelection userSelection;

        // current user hover: reset per-frame
        std::optional<MeshWarpingTabHover> currentHover;

        // currently active tab-wide popups
        PopupManager popupManager;

        // shared mesh cache
        std::shared_ptr<SceneCache> meshCache = App::singleton<SceneCache>();

        Vec2 overlayPadding = {10.0f, 10.0f};
        Color pairedLandmarkColor = Color::green();
        Color unpairedLandmarkColor = Color::red();
        Color nonParticipatingLandmarkColor = Color::purple();
    };
}
