#include "TPSDocumentHelpers.hpp"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>
#include <OpenSimCreator/Utils/TPS3D.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringName.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using osc::ConvertibleTo;
using osc::Iterable;
using osc::StringName;
using osc::TPSDocument;
using osc::TPSDocumentLandmarkPair;
using osc::TPSDocumentNonParticipatingLandmark;
using osc::UID;

namespace
{
    template<class T>
    concept UIDed = requires(T v)
    {
        { v.uid } -> ConvertibleTo<UID>;
    };

    template<class T>
    concept Named = requires(T v)
    {
        { v.name } -> ConvertibleTo<std::string_view>;
    };

    template<UIDed T>
    bool HasUID(UID id, T const& v)
    {
        return v.uid == id;
    }

    template<Named T>
    bool HasName(std::string_view const& name, T const& v)
    {
        return v.name == name;
    }

    // returns the next available unique ID with the given prefix
    template<Iterable Container>
    StringName NextUniqueID(Container const& c, std::string_view prefix)
    {
        std::string name;
        for (size_t i = 0; i < std::numeric_limits<decltype(i)>::max()-1; ++i)
        {
            name += prefix;
            name += std::to_string(i);

            if (std::none_of(c.begin(), c.end(), std::bind_front(HasName<typename Container::value_type>, name)))
            {
                return StringName{std::move(name)};
            }
            name.clear();
        }
        throw std::runtime_error{"unable to generate a unique name for a scene element"};
    }

    // equivalent of `std::find_if`, but returns a `nullptr` when nothing is found
    template<Iterable Container, class UnaryPredicate>
    auto NullableFindIf(Container& c, UnaryPredicate p) -> decltype(c.data())
    {
        auto const it = std::find_if(c.begin(), c.end(), p);
        return it != c.end() ? &(*it) : nullptr;
    }

    // this template exists because there's a const and non-const version of the function
    template<class TDocument>
    auto FindLandmarkPairImpl(TDocument& doc, UID id) -> decltype(doc.landmarkPairs.data())
    {
        return NullableFindIf(doc.landmarkPairs, std::bind_front(HasUID<TPSDocumentLandmarkPair>, id));
    }

    template<class TDocument>
    auto FindNonParticipatingLandmarkImpl(TDocument& doc, UID id) -> decltype(doc.nonParticipatingLandmarks.data())
    {
        return NullableFindIf(doc.nonParticipatingLandmarks, std::bind_front(HasUID<TPSDocumentNonParticipatingLandmark>, id));
    }

    size_t CountFullyPaired(TPSDocument const& doc)
    {
        return std::count_if(doc.landmarkPairs.begin(), doc.landmarkPairs.end(), osc::IsFullyPaired);
    }
}

osc::TPSDocumentLandmarkPair const* osc::FindLandmarkPair(TPSDocument const& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

osc::TPSDocumentLandmarkPair* osc::FindLandmarkPair(TPSDocument& doc, UID uid)
{
    return FindLandmarkPairImpl(doc, uid);
}

osc::TPSDocumentNonParticipatingLandmark const* osc::FindNonParticipatingLandmark(TPSDocument const& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

osc::TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmark(TPSDocument& doc, UID id)
{
    return FindNonParticipatingLandmarkImpl(doc, id);
}

osc::TPSDocumentElement const* osc::FindElement(TPSDocument const& doc, TPSDocumentElementID const& id)
{
    static_assert(NumOptions<TPSDocumentElementType>() == 2);

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

osc::TPSDocumentLandmarkPair const* osc::FindLandmarkPairByName(TPSDocument const& doc, StringName const& name)
{
    return NullableFindIf(doc.landmarkPairs, std::bind_front(HasName<TPSDocumentLandmarkPair>, name));
}

osc::TPSDocumentLandmarkPair* osc::FindLandmarkPairByName(TPSDocument& doc, StringName const& name)
{
    return NullableFindIf(doc.landmarkPairs, std::bind_front(HasName<TPSDocumentLandmarkPair>, name));
}

osc::TPSDocumentNonParticipatingLandmark const* osc::FindNonParticipatingLandmarkByName(TPSDocument const& doc, StringName const& name)
{
    return NullableFindIf(doc.nonParticipatingLandmarks, std::bind_front(HasName<TPSDocumentNonParticipatingLandmark>, name));
}

osc::TPSDocumentNonParticipatingLandmark* osc::FindNonParticipatingLandmarkByName(TPSDocument& doc, StringName const& name)
{
    return NullableFindIf(doc.nonParticipatingLandmarks, std::bind_front(HasName<TPSDocumentNonParticipatingLandmark>, name));
}

bool osc::ContainsElementWithName(TPSDocument const& doc, StringName const& name)
{
    return FindLandmarkPairByName(doc, name) || FindNonParticipatingLandmarkByName(doc, name);
}

std::optional<osc::LandmarkPair3D> osc::TryExtractLandmarkPair(TPSDocumentLandmarkPair const& p)
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

std::vector<osc::LandmarkPair3D> osc::GetLandmarkPairs(TPSDocument const& doc)
{
    std::vector<LandmarkPair3D> rv;
    rv.reserve(CountFullyPaired(doc));
    for (auto const& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(LandmarkPair3D{*p.maybeSourceLocation, *p.maybeDestinationLocation});
        }
    }
    return rv;
}

std::vector<osc::NamedLandmarkPair3D> osc::GetNamedLandmarkPairs(TPSDocument const& doc)
{
    std::vector<NamedLandmarkPair3D> rv;
    rv.reserve(CountFullyPaired(doc));
    for (auto const& p : doc.landmarkPairs)
    {
        if (IsFullyPaired(p))
        {
            rv.push_back(NamedLandmarkPair3D{{*p.maybeSourceLocation, *p.maybeDestinationLocation}, p.name});
        }
    }
    return rv;
}

size_t osc::CountNumLandmarksForInput(TPSDocument const& doc, TPSDocumentInputIdentifier which)
{
    auto const hasLocation = [which](TPSDocumentLandmarkPair const& p) { return HasLocation(p, which); };
    return std::count_if(doc.landmarkPairs.begin(), doc.landmarkPairs.end(), hasLocation);
}

// returns the next available unique landmark ID
StringName osc::NextLandmarkName(TPSDocument const& doc)
{
    return NextUniqueID(doc.landmarkPairs, "landmark_");
}

// returns the next available unique non-participating landmark ID
StringName osc::NextNonParticipatingLandmarkName(TPSDocument const& doc)
{
    return NextUniqueID(doc.nonParticipatingLandmarks, "datapoint_");
}

void osc::AddLandmarkToInput(
    TPSDocument& doc,
    TPSDocumentInputIdentifier which,
    Vec3 const& pos,
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
    Vec3 const& location,
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

bool osc::DeleteElementByID(TPSDocument& doc, TPSDocumentElementID const& id)
{
    static_assert(NumOptions<TPSDocumentElementType>() == 2);

    if (id.type == TPSDocumentElementType::Landmark)
    {
        auto& lms = doc.landmarkPairs;
        auto const it = std::find_if(lms.begin(), lms.end(), std::bind_front(HasUID<TPSDocumentLandmarkPair>, id.uid));
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
        auto const numElsDeleted = std::erase_if(doc.nonParticipatingLandmarks, std::bind_front(HasUID<TPSDocumentNonParticipatingLandmark>, id.uid));
        return numElsDeleted > 0;
    }
    return false;
}

bool osc::DeleteElementByID(TPSDocument& doc, UID id)
{
    return
        std::erase_if(doc.landmarkPairs, std::bind_front(HasUID<TPSDocumentLandmarkPair>, id)) > 0 ||
        std::erase_if(doc.nonParticipatingLandmarks, std::bind_front(HasUID<TPSDocumentNonParticipatingLandmark>, id)) > 0;
}

osc::CStringView osc::FindElementNameOr(
    TPSDocument const& doc,
    TPSDocumentElementID const& id,
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

std::vector<osc::TPSDocumentElementID> osc::GetAllElementIDs(TPSDocument const& doc)
{
    std::vector<TPSDocumentElementID> rv;
    rv.reserve(2*doc.landmarkPairs.size() + doc.nonParticipatingLandmarks.size());
    for (auto const& lm : doc.landmarkPairs)
    {
        rv.push_back(TPSDocumentElementID{lm.uid, TPSDocumentElementType::Landmark, TPSDocumentInputIdentifier::Source});
        rv.push_back(TPSDocumentElementID{lm.uid, TPSDocumentElementType::Landmark, TPSDocumentInputIdentifier::Destination});
    }
    for (auto const& npl : doc.nonParticipatingLandmarks)
    {
        rv.push_back(TPSDocumentElementID{npl.uid, TPSDocumentElementType::NonParticipatingLandmark, TPSDocumentInputIdentifier::Source});
    }
    return rv;
}
