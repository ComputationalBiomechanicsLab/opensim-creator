#include "mw_document_helpers.h"

#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_type.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_non_participating_landmark.h>

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/string_name.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace rgs = std::ranges;
using namespace osc;

namespace
{
    template<typename T>
    concept UIDed = requires(T v) {
        { v.uid } -> std::convertible_to<UID>;
    };

    template<UIDed T>
    UID id_of(const T& v)
    {
        return v.uid;
    }

    template<typename T>
    concept Named = requires(T v) {
        { v.name } -> std::convertible_to<std::string_view>;
    };

    template<Named T>
    std::string_view name_of(const T& v)
    {
        return v.name;
    }

    // returns the next available unique ID with the given prefix
    template<rgs::range Range>
    requires Named<typename Range::value_type>
    StringName next_unique_id(const Range& range, std::string_view prefix)
    {
        std::string name;
        for (size_t i = 0; i < std::numeric_limits<decltype(i)>::max() - 1; ++i) {
            name += prefix;
            name += std::to_string(i);

            if (not cpp23::contains(range, name, name_of<typename Range::value_type>)) {
                return StringName{std::move(name)};
            }
            name.clear();
        }
        throw std::runtime_error{"unable to generate a unique name for a scene element"};
    }

    // this template exists because there's a const and non-const version of the function
    template<std::convertible_to<MwDocument> TDocument>
    auto find_landmark_pair_impl(TDocument& doc, UID id) -> decltype(doc.landmark_pairs.data())
    {
        return find_or_nullptr(doc.landmark_pairs, id, id_of<MwDocumentLandmarkPair>);
    }

    template<std::convertible_to<MwDocument> TDocument>
    auto find_non_participating_landmark_impl(TDocument& doc, UID id)
        -> decltype(doc.non_participating_landmarks.data())
    {
        return find_or_nullptr(
            doc.non_participating_landmarks, id, id_of<MwDocumentNonParticipatingLandmark>);
    }

    size_t count_fully_paired(const MwDocument& doc)
    {
        return rgs::count_if(doc.landmark_pairs, is_fully_paired);
    }
}

std::optional<Vector3> osc::find_landmark_location(
    const MwDocument& doc,
    UID uid,
    MiDocumentInputIdentifier input,
    MwDocumentElementType element_type)
{
    static_assert(num_options_v<MwDocumentElementType> == 2);
    switch (element_type) {
    case MwDocumentElementType::Landmark: {
        if (const MwDocumentLandmarkPair* p = find_landmark_pair(doc, uid)) {
            static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
            return input == MiDocumentInputIdentifier::Source ? p->maybe_source_location
                                                              : p->maybe_destination_location;
        }
        break;
    }
    case MwDocumentElementType::NonParticipatingLandmark: {
        if (const auto* npl = find_non_participating_landmark(doc, uid)) {
            return npl->location;
        }
        break;
    default: return std::nullopt;
    }
    }
    return std::nullopt;
}

bool osc::translate_landmark_by_id(
    MwDocument& doc,
    UID uid,
    MiDocumentInputIdentifier input,
    MwDocumentElementType element_type,
    const Vector3& translation)
{
    static_assert(num_options_v<MwDocumentElementType> == 2);
    switch (element_type) {
    case MwDocumentElementType::Landmark: {
        if (MwDocumentLandmarkPair* p = find_landmark_pair(doc, uid)) {
            static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
            if (auto& pos = input == MiDocumentInputIdentifier::Source
                                ? p->maybe_source_location
                                : p->maybe_destination_location) {
                *pos += translation;
                return true;
            }
        }
        break;
    }
    case MwDocumentElementType::NonParticipatingLandmark: {
        if (auto* npl = find_non_participating_landmark(doc, uid)) {
            npl->location += translation;
            return true;
        }
        break;
    }
    default: return false;
    }
    return false;
}

bool osc::set_landmark_location_by_id(
    MwDocument& doc,
    UID uid,
    MiDocumentInputIdentifier input,
    MwDocumentElementType element_type,
    const Vector3& location)
{
    static_assert(num_options_v<MwDocumentElementType> == 2);
    switch (element_type) {
    case MwDocumentElementType::Landmark: {
        if (MwDocumentLandmarkPair* p = find_landmark_pair(doc, uid)) {
            static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
            if (auto& pos = input == MiDocumentInputIdentifier::Source
                                ? p->maybe_source_location
                                : p->maybe_destination_location) {
                *pos = location;
                return true;
            }
        }
        break;
    }
    case MwDocumentElementType::NonParticipatingLandmark: {
        if (auto* npl = find_non_participating_landmark(doc, uid)) {
            npl->location = location;
            return true;
        }
        break;
    }
    default: return false;
    }
    return false;
}

const MwDocumentLandmarkPair* osc::find_landmark_pair(const MwDocument& doc, UID uid)
{
    return find_landmark_pair_impl(doc, uid);
}

MwDocumentLandmarkPair* osc::find_landmark_pair(MwDocument& doc, UID uid)
{
    return find_landmark_pair_impl(doc, uid);
}

const MwDocumentNonParticipatingLandmark* osc::find_non_participating_landmark(
    const MwDocument& doc, UID id)
{
    return find_non_participating_landmark_impl(doc, id);
}

MwDocumentNonParticipatingLandmark* osc::find_non_participating_landmark(MwDocument& doc, UID id)
{
    return find_non_participating_landmark_impl(doc, id);
}

const MwDocumentElement* osc::find_element(const MwDocument& doc, const MwDocumentElementID& id)
{
    static_assert(num_options_v<MwDocumentElementType> == 2);

    switch (id.type) {
    case MwDocumentElementType::Landmark:
        if (const auto* p = find_landmark_pair(doc, id.uid); p && get_location(*p, id.input)) {
            return p;
        } else {
            return nullptr;
        }
    case MwDocumentElementType::NonParticipatingLandmark:
        return find_non_participating_landmark(doc, id.uid);
    default: return nullptr;
    }
}

const MwDocumentLandmarkPair* osc::find_landmark_pair_by_name(
    const MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmark_pairs, name, name_of<MwDocumentLandmarkPair>);
}

MwDocumentLandmarkPair* osc::find_landmark_pair_by_name(MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmark_pairs, name, name_of<MwDocumentLandmarkPair>);
}

const MwDocumentNonParticipatingLandmark* osc::find_non_participating_landmark_by_name(
    const MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(
        doc.non_participating_landmarks, name, name_of<MwDocumentNonParticipatingLandmark>);
}

MwDocumentNonParticipatingLandmark* osc::find_non_participating_landmark_by_name(
    MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(
        doc.non_participating_landmarks, name, name_of<MwDocumentNonParticipatingLandmark>);
}

bool osc::contains_element_with_name(const MwDocument& doc, const StringName& name)
{
    return find_landmark_pair_by_name(doc, name) != nullptr ||
           find_non_participating_landmark_by_name(doc, name) != nullptr;
}

std::optional<opyn::LandmarkPair3D<float>> osc::try_extract_landmark_pair(
    const MwDocumentLandmarkPair& p)
{
    if (is_fully_paired(p)) {
        return opyn::LandmarkPair3D<float>{
            to<SimTK::fVec3>(*p.maybe_source_location),
            to<SimTK::fVec3>(*p.maybe_destination_location)};
    } else {
        return std::nullopt;
    }
}

std::vector<opyn::LandmarkPair3D<float>> osc::get_landmark_pairs(const MwDocument& doc)
{
    std::vector<opyn::LandmarkPair3D<float>> rv;
    rv.reserve(count_fully_paired(doc));
    for (const auto& p : doc.landmark_pairs) {
        if (is_fully_paired(p)) {
            rv.push_back(
                opyn::LandmarkPair3D<float>{
                    to<SimTK::fVec3>(*p.maybe_source_location),
                    to<SimTK::fVec3>(*p.maybe_destination_location)});
        }
    }
    return rv;
}

std::vector<MwNamedLandmarkPair3D> osc::get_named_landmark_pairs(const MwDocument& doc)
{
    std::vector<MwNamedLandmarkPair3D> rv;
    rv.reserve(count_fully_paired(doc));
    for (const auto& p : doc.landmark_pairs) {
        if (is_fully_paired(p)) {
            rv.push_back(
                MwNamedLandmarkPair3D{
                    {to<SimTK::fVec3>(*p.maybe_source_location),
                     to<SimTK::fVec3>(*p.maybe_destination_location)},
                    p.name});
        }
    }
    return rv;
}

size_t osc::count_num_landmarks_for_input(const MwDocument& doc, MiDocumentInputIdentifier which)
{
    const auto has_location = [which](const MwDocumentLandmarkPair& p) {
        return osc::has_location(p, which);
    };
    return rgs::count_if(doc.landmark_pairs, has_location);
}

// returns the next available unique landmark ID
StringName osc::next_landmark_name(const MwDocument& doc)
{
    return next_unique_id(doc.landmark_pairs, "landmark_");
}

// returns the next available unique non-participating landmark ID
StringName osc::next_non_participating_landmark_name(const MwDocument& doc)
{
    return next_unique_id(doc.non_participating_landmarks, "datapoint_");
}

void osc::add_landmark_to_input(
    MwDocument& doc,
    MiDocumentInputIdentifier which,
    const Vector3& position,
    std::optional<std::string_view> suggested_name)
{
    if (suggested_name) {
        // if a name is suggested, then lookup the landmark by name and
        // overwrite the location; otherwise, create a new landmark with
        // the name (this is _probably_ what the user intended)

        const StringName name{*suggested_name};
        auto* p = find_landmark_pair_by_name(doc, name);
        if (!p) {
            p = &doc.landmark_pairs.emplace_back(name);
        }
        upd_location(*p, which) = position;
    } else {
        // no name suggested: so assume that the user wants to pair the
        // landmarks in-order with landmarks that have no corresponding
        // location yet; otherwise, create a new (half) landmark with
        // a generated name

        bool was_assigned_to_existing_empty_slot = false;
        for (MwDocumentLandmarkPair& p : doc.landmark_pairs) {
            std::optional<Vector3>& maybe_loc = upd_location(p, which);
            if (!maybe_loc) {
                maybe_loc = position;
                was_assigned_to_existing_empty_slot = true;
                break;
            }
        }

        // if there wasn't an empty slot, then create a new landmark pair and
        // assign the location to the relevant part of the pair
        if (!was_assigned_to_existing_empty_slot) {
            MwDocumentLandmarkPair& p = doc.landmark_pairs.emplace_back(next_landmark_name(doc));
            upd_location(p, which) = position;
        }
    }
}

void osc::add_non_participating_landmark(
    MwDocument& doc, const Vector3& location, std::optional<std::string_view> suggested_name)
{
    if (suggested_name) {
        // if a name is suggested, then lookup the non-participating landmark
        // by name and overwrite the location; otherwise, create a new
        // landmark with the name (this is _probably_ what the user intended)

        const StringName name{*suggested_name};
        auto* p = find_non_participating_landmark_by_name(doc, name);
        if (p) {
            p->location = location;
        } else {
            doc.non_participating_landmarks.emplace_back(name, location);
        }
    } else {
        // if no name is suggested, generate one

        doc.non_participating_landmarks.emplace_back(
            next_non_participating_landmark_name(doc), location);
    }
}

bool osc::delete_element_by_id(MwDocument& doc, const MwDocumentElementID& id)
{
    static_assert(num_options_v<MwDocumentElementType> == 2);

    if (id.type == MwDocumentElementType::Landmark) {
        auto& lms = doc.landmark_pairs;
        const auto it = rgs::find(lms, id.uid, id_of<MwDocumentLandmarkPair>);
        if (it != doc.landmark_pairs.end()) {
            upd_location(*it, id.input).reset();

            if (!has_source_or_destination_location(*it)) {
                // the landmark now has no data associated with it: garbage collect it
                doc.landmark_pairs.erase(it);
            }
            return true;
        }
    } else if (id.type == MwDocumentElementType::NonParticipatingLandmark) {
        const auto has_id = [id = id.uid](const auto& npl) {
            return npl.uid == id;
        };
        const auto num_els_deleted = std::erase_if(doc.non_participating_landmarks, has_id);
        return num_els_deleted > 0;
    }
    return false;
}

bool osc::delete_element_by_id(MwDocument& doc, UID id)
{
    const auto has_uid = [id](const auto& lp) {
        return lp.uid == id;
    };
    return std::erase_if(doc.landmark_pairs, has_uid) > 0 or
           std::erase_if(doc.non_participating_landmarks, has_uid) > 0;
}

CStringView osc::find_element_name_or(
    const MwDocument& doc, const MwDocumentElementID& id, CStringView alternative)
{
    if (const auto* p = find_element(doc, id)) {
        return p->get_name();
    } else {
        return alternative;
    }
}

std::vector<MwDocumentElementID> osc::get_all_element_ids(const MwDocument& doc)
{
    std::vector<MwDocumentElementID> rv;
    rv.reserve(2 * doc.landmark_pairs.size() + doc.non_participating_landmarks.size());
    for (const auto& lm : doc.landmark_pairs) {
        rv.push_back(
            MwDocumentElementID{
                lm.uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Source});
        rv.push_back(
            MwDocumentElementID{
                lm.uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Destination});
    }
    for (const auto& npl : doc.non_participating_landmarks) {
        rv.push_back(
            MwDocumentElementID{
                npl.uid,
                MwDocumentElementType::NonParticipatingLandmark,
                MiDocumentInputIdentifier::Source});
    }
    return rv;
}
