#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Utils/TPS3D.hpp>

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringName.hpp>

#include <cstddef>
#include <optional>
#include <vector>

// TPS document helper functions
namespace osc
{
    // if it exists in `doc`, returns a pointer to the identified element; otherwise, returns `nullptr`
    TPSDocumentElement const* FindElement(TPSDocument const&, TPSDocumentElementID const&);

    // returns the (mutable) source/destination of the given landmark pair, if available
    inline std::optional<Vec3>& UpdLocation(TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns the source/destination of the given landmark pair, if available
    inline std::optional<Vec3> const& GetLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns `true` if the given landmark pair has a location assigned for `which`
    inline bool HasLocation(TPSDocumentLandmarkPair const& landmarkPair, TPSDocumentInputIdentifier which)
    {
        return GetLocation(landmarkPair, which).has_value();
    }

    // returns the (mutable) source/destination mesh in the given document
    inline Mesh& UpdMesh(TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns the source/destination mesh in the given document
    inline Mesh const& GetMesh(TPSDocument const& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(NumOptions<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns `true` if both the source and destination are defined for the given UI landmark
    inline bool IsFullyPaired(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation && p.maybeDestinationLocation;
    }

    // returns `true` if the given UI landmark has either a source or a destination defined
    inline bool HasSourceOrDestinationLocation(TPSDocumentLandmarkPair const& p)
    {
        return p.maybeSourceLocation || p.maybeDestinationLocation;
    }

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns std::nullopt
    std::optional<LandmarkPair3D> TryExtractLandmarkPair(TPSDocumentLandmarkPair const&);

    // returns `true` if the document contains at least one "morphing" landmarks (i.e. ignores non-participating landmarks)
    inline bool ContainsLandmarks(TPSDocument const& doc)
    {
        return !doc.landmarkPairs.empty();
    }

    // returns `true` if the document contains at least one non-participating landmark
    inline bool ContainsNonParticipatingLandmarks(TPSDocument const& doc)
    {
        return !doc.nonParticipatingLandmarks.empty();
    }

    // returns all fully paired landmarks in `doc`
    std::vector<LandmarkPair3D> GetLandmarkPairs(TPSDocument const&);

    // returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(TPSDocument const&, TPSDocumentInputIdentifier);

    // returns the next available unique landmark ID
    StringName NextLandmarkID(TPSDocument const&);

    // returns the next available unique non-participating landmark ID
    StringName NextNonParticipatingLandmarkID(TPSDocument const&);

    // helper: add a source/destination landmark at the given location
    void AddLandmarkToInput(TPSDocument&, TPSDocumentInputIdentifier, Vec3 const&);

    // returns `true` if the given element was deleted from the document
    bool DeleteElementByID(TPSDocument&, TPSDocumentElementID const&);
}
