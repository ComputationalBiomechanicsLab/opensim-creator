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
    concept UIDed = requires(T v)
    {
        { v.uid } -> std::convertible_to<UID>;
    };

    template<UIDed T>
    UID id_of(const T& v)
    {
        return v.uid;
    }

    template<typename T>
    concept Named = requires(T v)
    {
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
    StringName NextUniqueID(const Range& range, std::string_view prefix)
    {
        std::string name;
        for (size_t i = 0; i < std::numeric_limits<decltype(i)>::max()-1; ++i)
        {
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
    auto FindLandmarkPairImpl(TDocument& doc, UID id) -> decltype(doc.landmarkPairs.data())
    {
        return find_or_nullptr(doc.landmarkPairs, id, id_of<MwDocumentLandmarkPair>);
    }

    template<std::convertible_to<MwDocument> TDocument>
    auto FindNonParticipatingLandmarkImpl(TDocument& doc, UID id) -> decltype(doc.nonParticipatingLandmarks.data())
    {
        return find_or_nullptr(doc.nonParticipatingLandmarks, id, id_of<MwDocumentNonParticipatingLandmark>);
    }

    size_t CountFullyPaired(const MwDocument& doc)
    {
        return rgs::count_if(doc.landmarkPairs, IsFullyPaired);
    }
}

std::optional<Vector3> osc::FindLandmarkLocation(const MwDocument& doc, UID uid, MiDocumentInputIdentifier input, MwDocumentElementType elementType)
{
    static_assert(num_options<MwDocumentElementType>() == 2);
    switch (elementType) {
    case MwDocumentElementType::Landmark: {
        if (const MwDocumentLandmarkPair* p = FindLandmarkPair(doc, uid)) {
            static_assert(num_options<MiDocumentInputIdentifier>() == 2);
            return input == MiDocumentInputIdentifier::Source ? p->maybeSourceLocation : p->maybeDestinationLocation;
        }
        break;
    }
    case MwDocumentElementType::NonParticipatingLandmark: {
        if (const auto* npl = FindNonParticipatingLandmark(doc, uid)) {
            return npl->location;
        }
        break;
    default: return std::nullopt;
    }
    }
    return std::nullopt;
}

bool osc::TranslateLandmarkByID(MwDocument& doc, UID uid, MiDocumentInputIdentifier input, MwDocumentElementType elementType, const Vector3& translation)
{
    static_assert(num_options<MwDocumentElementType>() == 2);
    switch (elementType) {
    case MwDocumentElementType::Landmark: {
        if (MwDocumentLandmarkPair* p = FindLandmarkPair(doc, uid)) {
            static_assert(num_options<MiDocumentInputIdentifier>() == 2);
            if (auto& pos = input == MiDocumentInputIdentifier::Source ? p->maybeSourceLocation : p->maybeDestinationLocation) {
                *pos += translation;
                return true;
            }
        }
        break;
    }
    case MwDocumentElementType::NonParticipatingLandmark: {
        if (auto* npl = FindNonParticipatingLandmark(doc, uid)) {
            npl->location += translation;
            return true;
        }
        break;
    }
    default: return false;
    }
    return false;
}

const MwDocumentLandmarkPair* osc::FindLandmarkPair(const MwDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

MwDocumentLandmarkPair* osc::FindLandmarkPair(MwDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

const MwDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmark(const MwDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

MwDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmark(MwDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

const MwDocumentElement* osc::FindElement(const MwDocument& doc, const MwDocumentElementID& id)
{
    static_assert(num_options<MwDocumentElementType>() == 2);

    switch (id.type) {
    case MwDocumentElementType::Landmark:
        if (const auto* p = FindLandmarkPair(doc, id.uid); p && GetLocation(*p, id.input))
        {
            return p;
        }
        else
        {
            return nullptr;
        }
    case MwDocumentElementType::NonParticipatingLandmark: return FindNonParticipatingLandmark(doc, id.uid);
    default: return nullptr;
    }
}

const MwDocumentLandmarkPair* osc::FindLandmarkPairByName(const MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmarkPairs, name, name_of<MwDocumentLandmarkPair>);
}

MwDocumentLandmarkPair* osc::FindLandmarkPairByName(MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmarkPairs, name, name_of<MwDocumentLandmarkPair>);
}

const MwDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmarkByName(const MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.nonParticipatingLandmarks, name, name_of<MwDocumentNonParticipatingLandmark>);
}

MwDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmarkByName(MwDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.nonParticipatingLandmarks, name, name_of<MwDocumentNonParticipatingLandmark>);
}

bool osc::ContainsElementWithName(const MwDocument& doc, const StringName& name)
{
    return
        FindLandmarkPairByName(doc, name) != nullptr ||
        FindNonParticipatingLandmarkByName(doc, name) != nullptr;
}

std::optional<opyn::LandmarkPair3D<float>> osc::TryExtractLandmarkPair(const MwDocumentLandmarkPair& p)
{
    if (IsFullyPaired(p)) {
        return opyn::LandmarkPair3D<float>{to<SimTK::fVec3>(*p.maybeSourceLocation), to<SimTK::fVec3>(*p.maybeDestinationLocation)};
    }
    else {
        return std::nullopt;
    }
}

std::vector<opyn::LandmarkPair3D<float>> osc::GetLandmarkPairs(const MwDocument& doc)
{
    std::vector<opyn::LandmarkPair3D<float>> rv;
    rv.reserve(CountFullyPaired(doc));
    for (const auto& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(opyn::LandmarkPair3D<float>{to<SimTK::fVec3>(*p.maybeSourceLocation), to<SimTK::fVec3>(*p.maybeDestinationLocation)});
        }
    }
    return rv;
}

std::vector<MwNamedLandmarkPair3D> osc::GetNamedLandmarkPairs(const MwDocument& doc)
{
    std::vector<MwNamedLandmarkPair3D> rv;
    rv.reserve(CountFullyPaired(doc));
    for (const auto& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(MwNamedLandmarkPair3D{{to<SimTK::fVec3>(*p.maybeSourceLocation), to<SimTK::fVec3>(*p.maybeDestinationLocation)}, p.name});
        }
    }
    return rv;
}

size_t osc::CountNumLandmarksForInput(const MwDocument& doc, MiDocumentInputIdentifier which)
{
    const auto hasLocation = [which](const MwDocumentLandmarkPair& p) { return HasLocation(p, which); };
    return rgs::count_if(doc.landmarkPairs, hasLocation);
}

// returns the next available unique landmark ID
StringName osc::NextLandmarkName(const MwDocument& doc)
{
    return NextUniqueID(doc.landmarkPairs, "landmark_");
}

// returns the next available unique non-participating landmark ID
StringName osc::NextNonParticipatingLandmarkName(const MwDocument& doc)
{
    return NextUniqueID(doc.nonParticipatingLandmarks, "datapoint_");
}

void osc::AddLandmarkToInput(
    MwDocument& doc,
    MiDocumentInputIdentifier which,
    const Vector3& position,
    std::optional<std::string_view> suggestedName)
{
    if (suggestedName)
    {
        // if a name is suggested, then lookup the landmark by name and
        // overwrite the location; otherwise, create a new landmark with
        // the name (this is _probably_ what the user intended)

        const StringName name{*suggestedName};
        auto* p = FindLandmarkPairByName(doc, name);
        if (!p)
        {
            p = &doc.landmarkPairs.emplace_back(name);
        }
        UpdLocation(*p, which) = position;
    }
    else
    {
        // no name suggested: so assume that the user wants to pair the
        // landmarks in-order with landmarks that have no corresponding
        // location yet; otherwise, create a new (half) landmark with
        // a generated name

        bool wasAssignedToExistingEmptySlot = false;
        for (MwDocumentLandmarkPair& p : doc.landmarkPairs)
        {
            std::optional<Vector3>& maybeLoc = UpdLocation(p, which);
            if (!maybeLoc)
            {
                maybeLoc = position;
                wasAssignedToExistingEmptySlot = true;
                break;
            }
        }

        // if there wasn't an empty slot, then create a new landmark pair and
        // assign the location to the relevant part of the pair
        if (!wasAssignedToExistingEmptySlot)
        {
            MwDocumentLandmarkPair& p = doc.landmarkPairs.emplace_back(NextLandmarkName(doc));
            UpdLocation(p, which) = position;
        }
    }
}

void osc::AddNonParticipatingLandmark(
    MwDocument& doc,
    const Vector3& location,
    std::optional<std::string_view> suggestedName)
{
    if (suggestedName)
    {
        // if a name is suggested, then lookup the non-participating landmark
        // by name and overwrite the location; otherwise, create a new
        // landmark with the name (this is _probably_ what the user intended)

        const StringName name{*suggestedName};
        auto* p = FindNonParticipatingLandmarkByName(doc, name);
        if (p)
        {
            p->location = location;
        }
        else
        {
            doc.nonParticipatingLandmarks.emplace_back(name, location);
        }
    }
    else
    {
        // if no name is suggested, generate one

        doc.nonParticipatingLandmarks.emplace_back(NextNonParticipatingLandmarkName(doc), location);
    }
}

bool osc::DeleteElementByID(MwDocument& doc, const MwDocumentElementID& id)
{
    static_assert(num_options<MwDocumentElementType>() == 2);

    if (id.type == MwDocumentElementType::Landmark)
    {
        auto& lms = doc.landmarkPairs;
        const auto it = rgs::find(lms, id.uid, id_of<MwDocumentLandmarkPair>);
        if (it != doc.landmarkPairs.end())
        {
            UpdLocation(*it, id.input).reset();

            if (!HasSourceOrDestinationLocation(*it))
            {
                // the landmark now has no data associated with it: garbage collect it
                doc.landmarkPairs.erase(it);
            }
            return true;
        }
    }
    else if (id.type == MwDocumentElementType::NonParticipatingLandmark)
    {
        const auto numElsDeleted = std::erase_if(doc.nonParticipatingLandmarks, [id = id.uid](const auto& npl) { return npl.uid == id; });
        return numElsDeleted > 0;
    }
    return false;
}

bool osc::DeleteElementByID(MwDocument& doc, UID id)
{
    return
        std::erase_if(doc.landmarkPairs, [id](const auto& lp) { return lp.uid == id; }) > 0 or
        std::erase_if(doc.nonParticipatingLandmarks, [id](const auto& nlp) { return nlp.uid == id; }) > 0;
}

CStringView osc::FindElementNameOr(
    const MwDocument& doc,
    const MwDocumentElementID& id,
    CStringView alternative)
{
    if (const auto* p = FindElement(doc, id))
    {
        return p->getName();
    }
    else
    {
        return alternative;
    }
}

std::vector<MwDocumentElementID> osc::GetAllElementIDs(const MwDocument& doc)
{
    std::vector<MwDocumentElementID> rv;
    rv.reserve(2*doc.landmarkPairs.size() + doc.nonParticipatingLandmarks.size());
    for (const auto& lm : doc.landmarkPairs)
    {
        rv.push_back(MwDocumentElementID{lm.uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Source});
        rv.push_back(MwDocumentElementID{lm.uid, MwDocumentElementType::Landmark, MiDocumentInputIdentifier::Destination});
    }
    for (const auto& npl : doc.nonParticipatingLandmarks)
    {
        rv.push_back(MwDocumentElementID{npl.uid, MwDocumentElementType::NonParticipatingLandmark, MiDocumentInputIdentifier::Source});
    }
    return rv;
}
