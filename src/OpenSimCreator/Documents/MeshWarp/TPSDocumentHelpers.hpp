#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputElementType.hpp>
#include <OpenSimCreator/Utils/TPS3D.hpp>

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringName.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

// TPS document helper functions
namespace osc
{
    // returns the (mutable) source/destination of the given landmark pair, if available
    std::optional<Vec3>& UpdLocation(TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns the source/destination of the given landmark pair, if available
    std::optional<Vec3> const& GetLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns `true` if the given landmark pair has a location assigned for `which`
    bool HasLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        return GetLocation(landmarkPair, which).has_value();
    }

    // returns the (mutable) source/destination mesh in the given document
    Mesh& UpdMesh(TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns the source/destination mesh in the given document
    Mesh const& GetMesh(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns `true` if both the source and destination are defined for the given UI landmark
    bool IsFullyPaired(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation && p.maybeDestinationLocation;
    }

    // returns `true` if the given UI landmark has either a source or a destination defined
    bool HasSourceOrDestinationLocation(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation || p.maybeDestinationLocation;
    }

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns std::nullopt
    std::optional<LandmarkPair3D> TryExtractLandmarkPair(TPSDocumentLandmarkPair const& p)
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

    // returns all fully paired landmarks in `doc`
    std::vector<LandmarkPair3D> GetLandmarkPairs(TPSDocument const& doc)
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

    // returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        auto const hasLocation = [which](TPSDocumentLandmarkPair const& p) { return HasLocation(p, which); };
        return std::count_if(doc.landmarkPairs.begin(), doc.landmarkPairs.end(), hasLocation);
    }

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

    // returns the next available unique landmark ID
    StringName NextLandmarkID(TPSDocument const& doc)
    {
        return NextUniqueID(doc.landmarkPairs, "landmark_");
    }

    // returns the next available unique non-participating landmark ID
    StringName NextNonParticipatingLandmarkID(TPSDocument const& doc)
    {
        return NextUniqueID(doc.nonParticipatingLandmarks, "datapoint_");
    }

    // helper: add a source/destination landmark at the given location
    void AddLandmarkToInput(
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

    // returns `true` if the given element was deleted from the document
    bool DeleteElementByID(TPSDocument& doc, TPSDocumentElementID const& id)
    {
        if (id.elementType == TPSDocumentInputElementType::Landmark)
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
        return false;
    }
}
