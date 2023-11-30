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
#include <stdexcept>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;

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
    case TPSDocumentElementType::Landmark: return FindLandmarkPair(doc, id.uid);
    case TPSDocumentElementType::NonParticipatingLandmark: return FindNonParticipatingLandmark(doc, id.uid);
    default: return nullptr;
    }
}

bool osc::ContainsElementWithName(TPSDocument const& doc, StringName const& name)
{
    auto const& lms = doc.landmarkPairs;
    auto const& npls = doc.nonParticipatingLandmarks;
    return
        std::any_of(lms.begin(), lms.end(), std::bind_front(HasName<TPSDocumentLandmarkPair>, name)) ||
        std::any_of(npls.begin(), npls.end(), std::bind_front(HasName<TPSDocumentNonParticipatingLandmark>, name));
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
    rv.reserve(std::count_if(doc.landmarkPairs.begin(), doc.landmarkPairs.end(), IsFullyPaired));

    for (TPSDocumentLandmarkPair const& p : doc.landmarkPairs)
    {
        if (std::optional<LandmarkPair3D> maybePair = TryExtractLandmarkPair(p))
        {
            rv.emplace_back(std::move(maybePair).value());
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
    Vec3 const& pos)
{
    // first, try assigning it to an empty slot in the existing data
    //
    // (e.g. imagine the caller added a few source points and is now
    //       trying to add destination points - they should probably
    //       be paired in-sequence with the unpaired source points)
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
