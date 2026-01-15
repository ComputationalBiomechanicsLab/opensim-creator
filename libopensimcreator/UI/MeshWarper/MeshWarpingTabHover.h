#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>

#include <liboscar/maths/vector3.h>

#include <optional>

namespace osc
{
    // a mouse hovertest result
    class MeshWarpingTabHover final {
    public:
        explicit MeshWarpingTabHover(
            TPSDocumentInputIdentifier input_,
            const Vector3& worldSpaceLocation_) :
            m_Input{input_},
            m_WorldSpaceLocation{worldSpaceLocation_}
        {}

        explicit MeshWarpingTabHover(
            TPSDocumentElementID sceneElementID_,
            const Vector3& worldSpaceLocation_) :

            m_MaybeSceneElementID{std::move(sceneElementID_)},
            m_Input{m_MaybeSceneElementID->input},
            m_WorldSpaceLocation{worldSpaceLocation_}
        {
        }

        TPSDocumentInputIdentifier getInput() const
        {
            return m_Input;
        }

        std::optional<TPSDocumentElementID> getSceneElementID() const
        {
            return m_MaybeSceneElementID;
        }

        bool isHoveringASceneElement() const
        {
            return m_MaybeSceneElementID.has_value();
        }

        bool isHovering(const TPSDocumentElementID& el) const
        {
            return m_MaybeSceneElementID == el;
        }

        const Vector3& getWorldSpaceLocation() const
        {
            return m_WorldSpaceLocation;
        }

    private:
        std::optional<TPSDocumentElementID> m_MaybeSceneElementID;
        TPSDocumentInputIdentifier m_Input;
        Vector3 m_WorldSpaceLocation;
    };
}
