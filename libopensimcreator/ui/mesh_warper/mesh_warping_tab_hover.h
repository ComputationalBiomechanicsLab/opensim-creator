#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>

#include <liboscar/maths/vector.h>

#include <optional>

namespace osc
{
    // a mouse hovertest result
    class MeshWarpingTabHover final {
    public:
        explicit MeshWarpingTabHover(
            MiDocumentInputIdentifier input_,
            const Vector3& worldSpaceLocation_) :
            m_Input{input_},
            m_WorldSpaceLocation{worldSpaceLocation_}
        {}

        explicit MeshWarpingTabHover(
            MwDocumentElementID sceneElementID_,
            const Vector3& worldSpaceLocation_) :

            m_MaybeSceneElementID{std::move(sceneElementID_)},
            m_Input{m_MaybeSceneElementID->input},
            m_WorldSpaceLocation{worldSpaceLocation_}
        {
        }

        MiDocumentInputIdentifier getInput() const
        {
            return m_Input;
        }

        std::optional<MwDocumentElementID> getSceneElementID() const
        {
            return m_MaybeSceneElementID;
        }

        bool isHoveringASceneElement() const
        {
            return m_MaybeSceneElementID.has_value();
        }

        bool isHovering(const MwDocumentElementID& el) const
        {
            return m_MaybeSceneElementID == el;
        }

        const Vector3& getWorldSpaceLocation() const
        {
            return m_WorldSpaceLocation;
        }

    private:
        std::optional<MwDocumentElementID> m_MaybeSceneElementID;
        MiDocumentInputIdentifier m_Input;
        Vector3 m_WorldSpaceLocation;
    };
}
