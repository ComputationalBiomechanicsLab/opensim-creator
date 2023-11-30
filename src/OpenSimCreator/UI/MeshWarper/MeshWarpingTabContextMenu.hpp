#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>

#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/UI/Widgets/StandardPopup.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <memory>
#include <optional>
#include <string>
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

            StandardPopup{label_},
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
                requestClose();  // element cannot be found in document (deleted? renamed?)
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
                requestClose();  // (defensive programming)
            }
        }

        void drawContextMenu(TPSDocumentLandmarkPair const& lm)
        {
            // header
            DrawContextMenuHeader(Ellipsis(lm.name, 15), "Landmark");
            DrawContextMenuSeparator();

            // name editor
            if (!m_ActiveNameEdit)
            {
                m_ActiveNameEdit = lm.name;
            }
            InputString("name", *m_ActiveNameEdit);
            if (ItemValueShouldBeSaved())
            {
                ActionRenameLandmark(*m_Shared->editedDocument, lm.uid, *m_ActiveNameEdit);
                m_ActiveNameEdit.reset();
            }

            if (lm.maybeSourceLocation)
            {
                if (!m_ActivePositionEdit)
                {
                    m_ActivePositionEdit = lm.maybeSourceLocation;
                }
                InputMetersFloat3("source", *m_ActivePositionEdit);
                if (ItemValueShouldBeSaved())
                {
                    ActionSetLandmarkPosition(*m_Shared->editedDocument, lm.uid, TPSDocumentInputIdentifier::Source, *m_ActivePositionEdit);
                    m_ActivePositionEdit.reset();
                }
            }
            else
            {
                if (ImGui::Button("add source"))
                {
                    ActionSetLandmarkPosition(*m_Shared->editedDocument, lm.uid, TPSDocumentInputIdentifier::Source, Vec3{});
                }
            }

            if (lm.maybeDestinationLocation)
            {
                if (!m_ActiveDestinationPositionEdit)
                {
                    m_ActiveDestinationPositionEdit = lm.maybeDestinationLocation;
                }
                InputMetersFloat3("destination", *m_ActiveDestinationPositionEdit);
                if (ItemValueShouldBeSaved())
                {
                    ActionSetLandmarkPosition(*m_Shared->editedDocument, lm.uid, TPSDocumentInputIdentifier::Destination, *m_ActiveDestinationPositionEdit);
                    m_ActivePositionEdit.reset();
                }
            }
            else
            {
                if (ImGui::Button("add destination"))
                {
                    ActionSetLandmarkPosition(*m_Shared->editedDocument, lm.uid, TPSDocumentInputIdentifier::Destination, Vec3{});
                }
            }
        }

        void drawContextMenu(TPSDocumentNonParticipatingLandmark const& npl)
        {
            // header
            DrawContextMenuHeader(Ellipsis(npl.name, 15), "Non-Participating Landmark");
            DrawContextMenuSeparator();

            // name editor
            if (!m_ActiveNameEdit)
            {
                m_ActiveNameEdit = npl.name;
            }
            InputString("name", *m_ActiveNameEdit);
            if (ItemValueShouldBeSaved())
            {
                ActionRenameNonParticipatingLandmark(*m_Shared->editedDocument, npl.uid, *m_ActiveNameEdit);
                m_ActiveNameEdit.reset();
            }

            if (!m_ActivePositionEdit)
            {
                m_ActivePositionEdit = npl.location;
            }
            InputMetersFloat3("location", *m_ActivePositionEdit);
            if (ItemValueShouldBeSaved())
            {
                ActionSetNonParticipatingLandmarkPosition(*m_Shared->editedDocument, npl.uid, *m_ActivePositionEdit);
                m_ActivePositionEdit.reset();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_Shared;
        TPSDocumentElementID m_ElementID;
        std::optional<std::string> m_ActiveNameEdit;
        std::optional<Vec3> m_ActivePositionEdit;
        std::optional<Vec3> m_ActiveDestinationPositionEdit;
    };
}
