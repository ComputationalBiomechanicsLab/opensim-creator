#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_named_landmark_pair_3d.h>
#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>

#include <libopynsim/utilities/landmark_pair_3d.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/string_name.h>

#include <cstddef>
#include <optional>
#include <vector>

// TPS document helper functions
namespace osc
{
    // if it exists in the document, returns the position of the given landmark in its mesh
    // coordinate system.
    std::optional<Vector3> find_landmark_location(
        const MwDocument&, UID, MiDocumentInputIdentifier, MwDocumentElementType);

    // if it exists in the document, translates the given landmark by the given translation vector
    // in the mesh's coordinate system.
    bool translate_landmark_by_id(
        MwDocument&,
        UID,
        MiDocumentInputIdentifier,
        MwDocumentElementType,
        const Vector3& translation);

    // if it exists in the document, sets the location of the given landmark to `location` in the
    // mesh's coordinate system.
    bool set_landmark_location_by_id(
        MwDocument&,
        UID,
        MiDocumentInputIdentifier,
        MwDocumentElementType,
        const Vector3& location);

    // if it exists in the document, returns a pointer to the identified landmark pair; otherwise,
    // returns `nullptr`
    const MwDocumentLandmarkPair* find_landmark_pair(const MwDocument&, UID);
    MwDocumentLandmarkPair* find_landmark_pair(MwDocument&, UID);

    // if it exists in the document, returns a pointer to the identified non-participating landmark;
    // otherwise, returns `nullptr`
    const MwDocumentNonParticipatingLandmark* find_non_participating_landmark(
        const MwDocument&, UID);
    MwDocumentNonParticipatingLandmark* find_non_participating_landmark(MwDocument&, UID);

    // if it exists in the document, returns a pointer to the identified element; otherwise, returns
    // `nullptr`
    const MwDocumentElement* find_element(const MwDocument& doc, const MwDocumentElementID&);

    // if it exists in the document, returns a pointer to the landmark that has the given name;
    // otherwise, returns `nullptr`
    const MwDocumentLandmarkPair* find_landmark_pair_by_name(const MwDocument&, const StringName&);
    MwDocumentLandmarkPair* find_landmark_pair_by_name(MwDocument&, const StringName&);
    const MwDocumentNonParticipatingLandmark* find_non_participating_landmark_by_name(
        const MwDocument&, const StringName&);
    MwDocumentNonParticipatingLandmark* find_non_participating_landmark_by_name(
        MwDocument&, const StringName&);

    // returns `true` if the document contains an element (landmark, non-participating landmark,
    // etc.) with the given name
    bool contains_element_with_name(const MwDocument& doc, const StringName&);

    // returns the (mutable) source/destination of the given landmark pair, if available
    inline std::optional<Vector3>& upd_location(
        MwDocumentLandmarkPair& landmark_pair, MiDocumentInputIdentifier which)
    {
        static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
        return which == MiDocumentInputIdentifier::Source
                   ? landmark_pair.maybe_source_location
                   : landmark_pair.maybe_destination_location;
    }

    // returns the source/destination of the given landmark pair, if available
    inline std::optional<Vector3> const& get_location(
        const MwDocumentLandmarkPair& landmark_pair, MiDocumentInputIdentifier which)
    {
        static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
        return which == MiDocumentInputIdentifier::Source
                   ? landmark_pair.maybe_source_location
                   : landmark_pair.maybe_destination_location;
    }

    // returns `true` if the given landmark pair has a location assigned for `which`
    inline bool has_location(
        const MwDocumentLandmarkPair& landmark_pair, MiDocumentInputIdentifier which)
    {
        return get_location(landmark_pair, which).has_value();
    }

    // returns the (mutable) source/destination mesh in the given document
    inline Mesh& upd_mesh(MwDocument& doc, MiDocumentInputIdentifier which)
    {
        static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
        return which == MiDocumentInputIdentifier::Source ? doc.source_mesh : doc.destination_mesh;
    }

    // returns the source/destination mesh in the given document
    inline const Mesh& get_mesh(const MwDocument& doc, MiDocumentInputIdentifier which)
    {
        static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
        return which == MiDocumentInputIdentifier::Source ? doc.source_mesh : doc.destination_mesh;
    }

    // returns `true` if both the source and destination are defined for the given UI landmark
    inline bool is_fully_paired(const MwDocumentLandmarkPair& p)
    {
        return p.maybe_source_location && p.maybe_destination_location;
    }

    // returns `true` if the given UI landmark has either a source or a destination defined
    inline bool has_source_or_destination_location(const MwDocumentLandmarkPair& p)
    {
        return p.maybe_source_location || p.maybe_destination_location;
    }

    // returns `true` if the document contains at least one "morphing" landmarks (i.e. ignores
    // non-participating landmarks)
    inline bool contains_landmarks(const MwDocument& doc)
    {
        return !doc.landmark_pairs.empty();
    }

    // returns `true` if the document contains at least one non-participating landmark
    inline bool contains_non_participating_landmarks(const MwDocument& doc)
    {
        return !doc.non_participating_landmarks.empty();
    }

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns
    // std::nullopt
    std::optional<opyn::LandmarkPair3D<float>> try_extract_landmark_pair(
        const MwDocumentLandmarkPair&);

    // returns all fully paired landmarks in `doc`
    std::vector<opyn::LandmarkPair3D<float>> get_landmark_pairs(const MwDocument&);

    // returns all fully paired landmarks, incl. their names, in `doc`
    std::vector<MwNamedLandmarkPair3D> get_named_landmark_pairs(const MwDocument&);

    // returns the count of landmarks in the document for which `which` is defined
    size_t count_num_landmarks_for_input(const MwDocument&, MiDocumentInputIdentifier);

    // returns the next available unique landmark name
    StringName next_landmark_name(const MwDocument&);

    // returns the next available unique non-participating landmark name
    StringName next_non_participating_landmark_name(const MwDocument&);

    // adds a source/destination landmark at the given location
    void add_landmark_to_input(
        MwDocument&,
        MiDocumentInputIdentifier,
        const Vector3&,
        std::optional<std::string_view> suggested_name = std::nullopt);

    // adds a non-participating landmark to the document
    void add_non_participating_landmark(
        MwDocument&, const Vector3&, std::optional<std::string_view> suggested_name = std::nullopt);

    // returns `true` if the given element was deleted from the document
    bool delete_element_by_id(MwDocument&, const MwDocumentElementID&);
    bool delete_element_by_id(MwDocument&, UID);

    // returns the name of the element, or an alternative if the element cannot be found
    CStringView find_element_name_or(
        const MwDocument&, const MwDocumentElementID&, CStringView alternative = "unknown");

    // returns all element IDs for all elements in the document
    std::vector<MwDocumentElementID> get_all_element_ids(const MwDocument&);
}
