#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>

#include <liboscar/maths/vector.h>

#include <optional>

namespace osc
{
    // a mouse hovertest result
    class MwUiHovertestResult final {
    public:
        explicit MwUiHovertestResult(
            MiDocumentInputIdentifier input, const Vector3& world_space_location) :

            input_{input},
            world_space_location_{world_space_location}
        {}

        explicit MwUiHovertestResult(
            MwDocumentElementID scene_element_id, const Vector3& world_space_location) :

            maybe_scene_element_id_{scene_element_id},
            input_{maybe_scene_element_id_->input},
            world_space_location_{world_space_location}
        {}

        MiDocumentInputIdentifier get_input() const { return input_; }

        std::optional<MwDocumentElementID> get_scene_element_id() const
        {
            return maybe_scene_element_id_;
        }

        bool is_hovering_a_scene_element() const { return maybe_scene_element_id_.has_value(); }

        bool is_hovering(const MwDocumentElementID& el) const
        {
            return maybe_scene_element_id_ == el;
        }

        const Vector3& get_world_space_location() const { return world_space_location_; }

    private:
        std::optional<MwDocumentElementID> maybe_scene_element_id_;
        MiDocumentInputIdentifier input_;
        Vector3 world_space_location_;
    };
}
