#include "TPSDocumentHelpers.h"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementType.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>

#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringName.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <sstream>
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
    UID id_of(T const& v)
    {
        return v.uid;
    }

    template<typename T>
    concept Named = requires(T v)
    {
        { v.name } -> std::convertible_to<std::string_view>;
    };

    template<Named T>
    std::string_view name_of(T const& v)
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

TPSDocumentLandmarkPair const* osc::FindLandmarkPair(const TPSDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

TPSDocumentLandmarkPair* osc::FindLandmarkPair(TPSDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

TPSDocumentNonParticipatingLandmark const* osc::FindNonParticipatingLandmark(const TPSDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmark(TPSDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

TPSDocumentElement const* osc::FindElement(const TPSDocument& doc, const TPSDocumentElementID& id)
{
    static_assert(num_options<TPSDocumentElementType>() == 2);

    switch (id.type) {
    case TPSDocumentElementType::Landmark:
        if (auto const* p = FindLandmarkPair(doc, id.uid); p && GetLocation(*p, id.input))
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

TPSDocumentLandmarkPair const* osc::FindLandmarkPairByName(const TPSDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmarkPairs, name, name_of<TPSDocumentLandmarkPair>);
}

TPSDocumentLandmarkPair* osc::FindLandmarkPairByName(TPSDocument& doc, const StringName& name)
{
    return find_or_nullptr(doc.landmarkPairs, name, name_of<TPSDocumentLandmarkPair>);
}

TPSDocumentNonParticipatingLandmark const* osc::FindNonParticipatingLandmarkByName(const TPSDocument& doc, const StringName& name)
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

std::optional<LandmarkPair3D> osc::TryExtractLandmarkPair(const TPSDocumentLandmarkPair& p)
{
    if (IsFullyPaired(p))
    {
        return LandmarkPair3D{*p.maybeSourceLocation, *p.maybeDestinationLocation};
    }
    else
    {
        return std::nullopt;
    }
}

std::vector<LandmarkPair3D> osc::GetLandmarkPairs(const TPSDocument& doc)
{
    std::vector<LandmarkPair3D> rv;
    rv.reserve(CountFullyPaired(doc));
    for (const auto& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(LandmarkPair3D{*p.maybeSourceLocation, *p.maybeDestinationLocation});
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
            rv.push_back(NamedLandmarkPair3D{{*p.maybeSourceLocation, *p.maybeDestinationLocation}, p.name});
        }
    }
    return rv;
}

size_t osc::CountNumLandmarksForInput(const TPSDocument& doc, TPSDocumentInputIdentifier which)
{
    auto const hasLocation = [which](const TPSDocumentLandmarkPair& p) { return HasLocation(p, which); };
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
    const Vec3& pos,
    std::optional<std::string_view> suggestedName)
{
    if (suggestedName)
    {
        // if a name is suggested, then lookup the landmark by name and
        // overwrite the location; otherwise, create a new landmark with
        // the name (this is _probably_ what the user intended)

        StringName const name{*suggestedName};
        auto* p = FindLandmarkPairByName(doc, name);
        if (!p)
        {
            p = &doc.landmarkPairs.emplace_back(name);
        }
        UpdLocation(*p, which) = pos;
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
            std::optional<Vec3>& maybeLoc = UpdLocation(p, which);
            if (!maybeLoc)
            {
                maybeLoc = pos;
                wasAssignedToExistingEmptySlot = true;
                break;
            }
        }

        // if there wasn't an empty slot, then create a new landmark pair and
        // assign the location to the relevant part of the pair
        if (!wasAssignedToExistingEmptySlot)
        {
            TPSDocumentLandmarkPair& p = doc.landmarkPairs.emplace_back(NextLandmarkName(doc));
            UpdLocation(p, which) = pos;
        }
    }
}

void osc::AddNonParticipatingLandmark(
    TPSDocument& doc,
    const Vec3& location,
    std::optional<std::string_view> suggestedName)
{
    if (suggestedName)
    {
        // if a name is suggested, then lookup the non-participating landmark
        // by name and overwrite the location; otherwise, create a new
        // landmark with the name (this is _probably_ what the user intended)

        StringName const name{*suggestedName};
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
        auto const it = rgs::find(lms, id.uid, id_of<TPSDocumentLandmarkPair>);
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
        auto const numElsDeleted = std::erase_if(doc.nonParticipatingLandmarks, [id = id.uid](const auto& npl) { return npl.uid == id; });
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
    if (auto const* p = FindElement(doc, id))
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
