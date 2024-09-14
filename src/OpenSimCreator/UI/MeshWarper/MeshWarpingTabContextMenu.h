#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/StringHelpers.h>

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
            m_State{std::move(shared_)},
            m_ElementID{std::move(rightClickedID_)}
        {
            set_modal(false);
        }
    private:
        void impl_draw_content() final
        {
            const TPSDocumentElement* el = FindElement(m_State->getScratch(), m_ElementID);
            if (!el)
            {
                request_close();  // element cannot be found in document (deleted? renamed?)
            }
            else if (const auto* landmarkPair = dynamic_cast<const TPSDocumentLandmarkPair*>(el))
            {
                drawContextMenu(*landmarkPair);
            }
            else if (const auto* npl = dynamic_cast<const TPSDocumentNonParticipatingLandmark*>(el))
            {
                drawContextMenu(*npl);
            }
            else
            {
                request_close();  // (defensive programming)
            }
        }

        void drawContextMenu(const TPSDocumentLandmarkPair& lm)
        {
            // header
            DrawContextMenuHeader(truncate_with_ellipsis(lm.name, 15), "Landmark");
            DrawContextMenuSeparator();

            // name editor
            if (!m_ActiveNameEdit)
            {
                m_ActiveNameEdit = lm.name;
            }
            ui::draw_string_input("name", *m_ActiveNameEdit);
            if (ui::should_save_last_drawn_item_value())
            {
                ActionRenameLandmark(m_State->updUndoable(), lm.uid, *m_ActiveNameEdit);
                m_ActiveNameEdit.reset();
            }

            if (lm.maybeSourceLocation)
            {
                if (!m_ActivePositionEdit)
                {
                    m_ActivePositionEdit = lm.maybeSourceLocation;
                }
                ui::draw_float3_meters_input("source           ", *m_ActivePositionEdit);  // (padded to align with `destination`)
                if (ui::should_save_last_drawn_item_value())
                {
                    ActionSetLandmarkPosition(m_State->updUndoable(), lm.uid, TPSDocumentInputIdentifier::Source, *m_ActivePositionEdit);
                    m_ActivePositionEdit.reset();
                }
            }
            else
            {
                if (ui::draw_button("add source"))
                {
                    ActionSetLandmarkPosition(m_State->updUndoable(), lm.uid, TPSDocumentInputIdentifier::Source, Vec3{});
                }
            }

            if (lm.maybeDestinationLocation)
            {
                if (!m_ActiveDestinationPositionEdit)
                {
                    m_ActiveDestinationPositionEdit = lm.maybeDestinationLocation;
                }
                ui::draw_float3_meters_input("destination", *m_ActiveDestinationPositionEdit);
                if (ui::should_save_last_drawn_item_value())
                {
                    ActionSetLandmarkPosition(m_State->updUndoable(), lm.uid, TPSDocumentInputIdentifier::Destination, *m_ActiveDestinationPositionEdit);
                    m_ActivePositionEdit.reset();
                }
            }
            else
            {
                if (ui::draw_button_centered("add destination"))
                {
                    ActionSetLandmarkPosition(m_State->updUndoable(), lm.uid, TPSDocumentInputIdentifier::Destination, Vec3{});
                }
            }

            DrawContextMenuSeparator();

            if (ui::draw_menu_item(OSC_ICON_TRASH " Delete", "Delete"))
            {
                ActionDeleteElementByID(m_State->updUndoable(), lm.uid);
                return;  // CARE: `lm` is now dead
            }
        }

        void drawContextMenu(const TPSDocumentNonParticipatingLandmark& npl)
        {
            // header
            DrawContextMenuHeader(truncate_with_ellipsis(npl.name, 15), "Non-Participating Landmark");
            DrawContextMenuSeparator();

            // name editor
            if (!m_ActiveNameEdit)
            {
                m_ActiveNameEdit = npl.name;
            }
            ui::draw_string_input("name", *m_ActiveNameEdit);
            if (ui::should_save_last_drawn_item_value())
            {
                ActionRenameNonParticipatingLandmark(m_State->updUndoable(), npl.uid, *m_ActiveNameEdit);
                m_ActiveNameEdit.reset();
            }

            if (!m_ActivePositionEdit)
            {
                m_ActivePositionEdit = npl.location;
            }
            ui::draw_float3_meters_input("location", *m_ActivePositionEdit);
            if (ui::should_save_last_drawn_item_value())
            {
                ActionSetNonParticipatingLandmarkPosition(m_State->updUndoable(), npl.uid, *m_ActivePositionEdit);
                m_ActivePositionEdit.reset();
            }

            DrawContextMenuSeparator();

            if (ui::draw_menu_item(OSC_ICON_TRASH " Delete", "Delete"))
            {
                ActionDeleteElementByID(m_State->updUndoable(), npl.uid);
                return;  // CARE: `npl` is now dead
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        TPSDocumentElementID m_ElementID;
        std::optional<std::string> m_ActiveNameEdit;
        std::optional<Vec3> m_ActivePositionEdit;
        std::optional<Vec3> m_ActiveDestinationPositionEdit;
    };
}
