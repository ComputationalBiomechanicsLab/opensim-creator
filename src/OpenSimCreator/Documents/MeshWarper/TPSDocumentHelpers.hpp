#pragma once

#include <OpenSimCreator/Documents/MeshWarper/NamedLandmarkPair3D.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElement.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentElementID.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Utils/LandmarkPair3D.hpp>

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/StringName.hpp>

#include <cstddef>
#include <optional>
#include <vector>

// TPS document helper functions
namespace osc
{
    // if it exists in the document, returns a pointer to the identified landmark pair; otherwise, returns `nullptr`
    TPSDocumentLandmarkPair const* FindLandmarkPair(TPSDocument const&, UID);
    TPSDocumentLandmarkPair* FindLandmarkPair(TPSDocument&, UID);

    // if it exists in the document, returns a pointer to the identified non-participating landmark; otherwise, returns `nullptr`
    TPSDocumentNonParticipatingLandmark const* FindNonParticipatingLandmark(TPSDocument const&, UID);
    TPSDocumentNonParticipatingLandmark* FindNonParticipatingLandmark(TPSDocument&, UID);

    // if it exists in the document, returns a pointer to the identified element; otherwise, returns `nullptr`
    TPSDocumentElement const* FindElement(TPSDocument const& doc, TPSDocumentElementID const&);

    // if it exists in the document, returns a pointer to the landmark that has the given name; otherwise, returns `nullptr`
    TPSDocumentLandmarkPair const* FindLandmarkPairByName(TPSDocument const&, StringName const&);
    TPSDocumentLandmarkPair* FindLandmarkPairByName(TPSDocument&, StringName const&);
    TPSDocumentNonParticipatingLandmark const* FindNonParticipatingLandmarkByName(TPSDocument const&, StringName const&);
    TPSDocumentNonParticipatingLandmark* FindNonParticipatingLandmarkByName(TPSDocument&, StringName const&);

    // returns `true` if the document contains an element (landmark, non-participating landmark, etc.) with the given name
    bool ContainsElementWithName(TPSDocument const& doc, StringName const&);

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

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns std::nullopt
    std::optional<LandmarkPair3D> TryExtractLandmarkPair(TPSDocumentLandmarkPair const&);

    // returns all fully paired landmarks in `doc`
    std::vector<LandmarkPair3D> GetLandmarkPairs(TPSDocument const&);

    // returns all fully paired landmarks, incl. their names, in `doc`
    std::vector<NamedLandmarkPair3D> GetNamedLandmarkPairs(TPSDocument const&);

    // returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(TPSDocument const&, TPSDocumentInputIdentifier);

    // returns the next available unique landmark name
    StringName NextLandmarkName(TPSDocument const&);

    // returns the next available unique non-participating landmark name
    StringName NextNonParticipatingLandmarkName(TPSDocument const&);

    // adds a source/destination landmark at the given location
    void AddLandmarkToInput(
        TPSDocument&,
        TPSDocumentInputIdentifier,
        Vec3 const&,
        std::optional<std::string_view> suggestedName = std::nullopt
    );

    // adds a non-participating landmark to the document
    void AddNonParticipatingLandmark(
        TPSDocument&,
        Vec3 const&,
        std::optional<std::string_view> suggestedName = std::nullopt
    );

    // returns `true` if the given element was deleted from the document
    bool DeleteElementByID(TPSDocument&, TPSDocumentElementID const&);
    bool DeleteElementByID(TPSDocument&, UID);

    // returns the name of the element, or an alternative if the element cannot be found
    CStringView FindElementNameOr(TPSDocument const&, TPSDocumentElementID const&, CStringView alternative = "unknown");

    // returns all element IDs for all elements in the document
    std::vector<TPSDocumentElementID> GetAllElementIDs(TPSDocument const&);
}
