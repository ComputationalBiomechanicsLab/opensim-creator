#include "TPSDocumentHelpers.hpp"

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementType.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>
#include <OpenSimCreator/Utils/TPS3D.hpp>

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringName.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string_view>
#include <vector>

using osc::StringName;

namespace
{
    // returns `true` if an element in the container has `.id` == `id`
    template<class Container>
    bool ContainsElementWithID(Container const& c, StringName const& id)
    {
        return std::any_of(c.begin(), c.end(), [&id](auto const& el) { return el.id == id; });
    }

    // returns the next available unique ID with the given prefix
    template<class Container>
    StringName NextUniqueID(Container const& c, std::string_view prefix)
    {
        // keep generating a name until an unused one is found
        for (size_t i = 0; i < std::numeric_limits<size_t>::max()-1; ++i)
        {
            std::stringstream ss;
            ss << prefix << i;
            StringName name{std::move(ss).str()};

            if (!ContainsElementWithID(c, name))
            {
                return name;
            }
        }
        throw std::runtime_error{"unable to generate a unique name for a scene element"};
    }
}

osc::TPSDocumentElement const* osc::FindElement(TPSDocument const& doc, TPSDocumentElementID const& id)
{
    static_assert(NumOptions<TPSDocumentElementType>() == 2);

    if (id.elementType == TPSDocumentElementType::Landmark)
    {
        for (auto const& landmark : doc.landmarkPairs)
        {
            if (landmark.id == id.elementID)
            {
                return &landmark;
            }
        }
        return nullptr;
    }
    else if (id.elementType == TPSDocumentElementType::NonParticipatingLandmark)
    {
        for (auto const& npl : doc.nonParticipatingLandmarks)
        {
            if (npl.id == id.elementID)
            {
                return &npl;
            }
        }
        return nullptr;
    }
    else
    {
        return nullptr;
    }
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
            rv.emplace_back(*maybePair);
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
StringName osc::NextLandmarkID(TPSDocument const& doc)
{
    return NextUniqueID(doc.landmarkPairs, "landmark_");
}

// returns the next available unique non-participating landmark ID
StringName osc::NextNonParticipatingLandmarkID(TPSDocument const& doc)
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
        TPSDocumentLandmarkPair& p = doc.landmarkPairs.emplace_back(NextLandmarkID(doc));
        UpdLocation(p, which) = pos;
    }
}

bool osc::DeleteElementByID(TPSDocument& doc, TPSDocumentElementID const& id)
{
    if (id.elementType == TPSDocumentElementType::Landmark)
    {
        auto const pairHasID = [&id](TPSDocumentLandmarkPair const& p) { return p.id == id.elementID; };
        auto const it = std::find_if(
            doc.landmarkPairs.begin(),
            doc.landmarkPairs.end(),
            pairHasID
        );
        if (it != doc.landmarkPairs.end())
        {
            UpdLocation(*it, id.whichInput).reset();

            if (!HasSourceOrDestinationLocation(*it))
            {
                // the landmark now has no data associated with it: garbage collect it
                doc.landmarkPairs.erase(it);
            }
            return true;
        }
    }
    else if (id.elementType == TPSDocumentElementType::NonParticipatingLandmark)
    {
        auto const landmarkHasID = [&id](TPSDocumentNonParticipatingLandmark const& lm) { return lm.id == id.elementID; };
        auto const numElsDeleted = std::erase_if(doc.nonParticipatingLandmarks, landmarkHasID);
        return numElsDeleted > 0;
    }
    return false;
}
