#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <imgui.h>
#include <oscar/Platform/Log.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace osc
{
    class MeshWarpingTabContextMenu final : public StandardPopup {
    public:
        MeshWarpingTabContextMenu(
            std::string_view label_,
            std::shared_ptr<MeshWarpingTabSharedState> shared_,
            TPSDocumentElementID rightClickedID_) :

            StandardPopup{label_, {10.0f, 10.0f}, ImGuiWindowFlags_NoMouseInputs},
            m_Shared{std::move(shared_)},
            m_ElementID{std::move(rightClickedID_)}
        {
            setModal(false);
        }
    private:
        void implDrawContent() final
        {
            TPSDocumentElement const* el = FindElement(m_Shared->getScratch(), m_ElementID);
            if (!el)
            {
                requestClose();
            }
            else if (auto const* landmarkPair = dynamic_cast<TPSDocumentLandmarkPair const*>(el))
            {
                drawContextMenu(*landmarkPair);
            }
            else if (auto const* npl = dynamic_cast<TPSDocumentNonParticipatingLandmark const*>(el))
            {
                drawContextMenu(*npl);
            }
            else
            {
                requestClose();  // defensive programming
            }
        }

        void drawContextMenu(TPSDocumentLandmarkPair const&)
        {
            ImGui::Text("right-clicked landmark");
        }

        void drawContextMenu(TPSDocumentNonParticipatingLandmark const&)
        {
            ImGui::Text("right-clicked non-participating");
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_Shared;
        TPSDocumentElementID m_ElementID;
    };
}
