#include "TPSDocumentHelpers.h"

#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementType.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>

#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utils/algorithms.h>
#include <liboscar/utils/enum_helpers.h>
#include <liboscar/utils/string_name.h>

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
    template<std::convertible_to<TPSDocument> TDocument>
    auto FindLandmarkPairImpl(TDocument& doc, UID id) -> decltype(doc.landmarkPairs.data())
    {
        return find_or_nullptr(doc.landmarkPairs, id, id_of<TPSDocumentLandmarkPair>);
    }

    template<std::convertible_to<TPSDocument> TDocument>
    auto FindNonParticipatingLandmarkImpl(TDocument& doc, UID id) -> decltype(doc.nonParticipatingLandmarks.data())
    {
        return find_or_nullptr(doc.nonParticipatingLandmarks, id, id_of<TPSDocumentNonParticipatingLandmark>);
    }

    size_t CountFullyPaired(const TPSDocument& doc)
    {
        return rgs::count_if(doc.landmarkPairs, IsFullyPaired);
    }
}

std::optional<Vector3> osc::FindLandmarkLocation(const TPSDocument& doc, UID uid, TPSDocumentInputIdentifier input, TPSDocumentElementType elementType)
{
    static_assert(num_options<TPSDocumentElementType>() == 2);
    switch (elementType) {
    case TPSDocumentElementType::Landmark: {
        if (const TPSDocumentLandmarkPair* p = FindLandmarkPair(doc, uid)) {
            static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
            return input == TPSDocumentInputIdentifier::Source ? p->maybeSourceLocation : p->maybeDestinationLocation;
        }
        break;
    }
    case TPSDocumentElementType::NonParticipatingLandmark: {
        if (const auto* npl = FindNonParticipatingLandmark(doc, uid)) {
            return npl->location;
        }
        break;
    default: return std::nullopt;
    }
    }
    return std::nullopt;
}

bool osc::TranslateLandmarkByID(TPSDocument& doc, UID uid, TPSDocumentInputIdentifier input, TPSDocumentElementType elementType, const Vector3& translation)
{
    static_assert(num_options<TPSDocumentElementType>() == 2);
    switch (elementType) {
    case TPSDocumentElementType::Landmark: {
        if (TPSDocumentLandmarkPair* p = FindLandmarkPair(doc, uid)) {
            static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
            if (auto& pos = input == TPSDocumentInputIdentifier::Source ? p->maybeSourceLocation : p->maybeDestinationLocation) {
                *pos += translation;
                return true;
            }
        }
        break;
    }
    case TPSDocumentElementType::NonParticipatingLandmark: {
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

const TPSDocumentLandmarkPair* osc::FindLandmarkPair(const TPSDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

TPSDocumentLandmarkPair* osc::FindLandmarkPair(TPSDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

const TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmark(const TPSDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmark(TPSDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

const TPSDocumentElement* osc::FindElement(const TPSDocument& doc, const TPSDocumentElementID& id)
{
    static_assert(num_options<TPSDocumentElementType>() == 2);

    switch (id.type) {
    case TPSDocumentElementType::Landmark:
        if (const auto* p = FindLandmarkPair(doc, id.uid); p && GetLocation(*p, id.input))
        {
            return p;
        }
        else
        {
            return nullptr;
        }
    case TPSDocumentElementType::NonParticipatingLandmark: return FindNonParticipatingLandmark(doc, id.uid);
    default: return nullptr;
    }
}

const TPSDocumentLandmarkPair* osc::FindLandmarkPairByName(const TPSDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmarkPairs, name, name_of<TPSDocumentLandmarkPair>);
}

TPSDocumentLandmarkPair* osc::FindLandmarkPairByName(TPSDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmarkPairs, name, name_of<TPSDocumentLandmarkPair>);
}

const TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmarkByName(const TPSDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.nonParticipatingLandmarks, name, name_of<TPSDocumentNonParticipatingLandmark>);
}

TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmarkByName(TPSDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.nonParticipatingLandmarks, name, name_of<TPSDocumentNonParticipatingLandmark>);
}

bool osc::ContainsElementWithName(const TPSDocument& doc, const StringName& name)
{
    return
        FindLandmarkPairByName(doc, name) != nullptr ||
        FindNonParticipatingLandmarkByName(doc, name) != nullptr;
}

std::optional<opyn::landmark_pair_3d<float>> osc::TryExtractLandmarkPair(const TPSDocumentLandmarkPair& p)
{
    if (IsFullyPaired(p)) {
        return opyn::landmark_pair_3d<float>{to<SimTK::fVec3>(*p.maybeSourceLocation), to<SimTK::fVec3>(*p.maybeDestinationLocation)};
    }
    else {
        return std::nullopt;
    }
}

std::vector<opyn::landmark_pair_3d<float>> osc::GetLandmarkPairs(const TPSDocument& doc)
{
    std::vector<opyn::landmark_pair_3d<float>> rv;
    rv.reserve(CountFullyPaired(doc));
    for (const auto& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(opyn::landmark_pair_3d<float>{to<SimTK::fVec3>(*p.maybeSourceLocation), to<SimTK::fVec3>(*p.maybeDestinationLocation)});
        }
    }
    return rv;
}

std::vector<NamedLandmarkPair3D> osc::GetNamedLandmarkPairs(const TPSDocument& doc)
{
    std::vector<NamedLandmarkPair3D> rv;
    rv.reserve(CountFullyPaired(doc));
    for (const auto& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(NamedLandmarkPair3D{{to<SimTK::fVec3>(*p.maybeSourceLocation), to<SimTK::fVec3>(*p.maybeDestinationLocation)}, p.name});
        }
    }
    return rv;
}

size_t osc::CountNumLandmarksForInput(const TPSDocument& doc, TPSDocumentInputIdentifier which)
{
    const auto hasLocation = [which](const TPSDocumentLandmarkPair& p) { return HasLocation(p, which); };
    return rgs::count_if(doc.landmarkPairs, hasLocation);
}

// returns the next available unique landmark ID
StringName osc::NextLandmarkName(const TPSDocument& doc)
{
    return NextUniqueID(doc.landmarkPairs, "landmark_");
}

// returns the next available unique non-participating landmark ID
StringName osc::NextNonParticipatingLandmarkName(const TPSDocument& doc)
{
    return NextUniqueID(doc.nonParticipatingLandmarks, "datapoint_");
}

void osc::AddLandmarkToInput(
    TPSDocument& doc,
    TPSDocumentInputIdentifier which,
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
        for (TPSDocumentLandmarkPair& p : doc.landmarkPairs)
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
            TPSDocumentLandmarkPair& p = doc.landmarkPairs.emplace_back(NextLandmarkName(doc));
            UpdLocation(p, which) = position;
        }
    }
}

void osc::AddNonParticipatingLandmark(
    TPSDocument& doc,
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

bool osc::DeleteElementByID(TPSDocument& doc, const TPSDocumentElementID& id)
{
    static_assert(num_options<TPSDocumentElementType>() == 2);

    if (id.type == TPSDocumentElementType::Landmark)
    {
        auto& lms = doc.landmarkPairs;
        const auto it = rgs::find(lms, id.uid, id_of<TPSDocumentLandmarkPair>);
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
    else if (id.type == TPSDocumentElementType::NonParticipatingLandmark)
    {
        const auto numElsDeleted = std::erase_if(doc.nonParticipatingLandmarks, [id = id.uid](const auto& npl) { return npl.uid == id; });
        return numElsDeleted > 0;
    }
    return false;
}

bool osc::DeleteElementByID(TPSDocument& doc, UID id)
{
    return
        std::erase_if(doc.landmarkPairs, [id](const auto& lp) { return lp.uid == id; }) > 0 or
        std::erase_if(doc.nonParticipatingLandmarks, [id](const auto& nlp) { return nlp.uid == id; }) > 0;
}

CStringView osc::FindElementNameOr(
    const TPSDocument& doc,
    const TPSDocumentElementID& id,
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

std::vector<TPSDocumentElementID> osc::GetAllElementIDs(const TPSDocument& doc)
{
    std::vector<TPSDocumentElementID> rv;
    rv.reserve(2*doc.landmarkPairs.size() + doc.nonParticipatingLandmarks.size());
    for (const auto& lm : doc.landmarkPairs)
    {
        rv.push_back(TPSDocumentElementID{lm.uid, TPSDocumentElementType::Landmark, TPSDocumentInputIdentifier::Source});
        rv.push_back(TPSDocumentElementID{lm.uid, TPSDocumentElementType::Landmark, TPSDocumentInputIdentifier::Destination});
    }
    for (const auto& npl : doc.nonParticipatingLandmarks)
    {
        rv.push_back(TPSDocumentElementID{npl.uid, TPSDocumentElementType::NonParticipatingLandmark, TPSDocumentInputIdentifier::Source});
    }
    return rv;
}
