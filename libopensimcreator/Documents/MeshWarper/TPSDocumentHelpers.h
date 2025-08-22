#pragma once

#include <libopensimcreator/Documents/MeshWarper/NamedLandmarkPair3D.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocument.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElement.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementID.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <libopensimcreator/Utils/LandmarkPair3D.h>

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/StringName.h>

#include <cstddef>
#include <optional>
#include <vector>

// TPS document helper functions
namespace osc
{
    // if it exists in the document, returns the position of the given landmark in its mesh coordinate system.
    std::optional<Vector3> FindLandmarkLocation(const TPSDocument&, UID, TPSDocumentInputIdentifier, TPSDocumentElementType);

    // if it exists in the document, translates the given landmark by the given translation vector in the mesh's coordinate system.
    bool TranslateLandmarkByID(TPSDocument&, UID, TPSDocumentInputIdentifier, TPSDocumentElementType, const Vector3& translation);

    // if it exists in the document, returns a pointer to the identified landmark pair; otherwise, returns `nullptr`
    const TPSDocumentLandmarkPair* FindLandmarkPair(const TPSDocument&, UID);
    TPSDocumentLandmarkPair* FindLandmarkPair(TPSDocument&, UID);

    // if it exists in the document, returns a pointer to the identified non-participating landmark; otherwise, returns `nullptr`
    const TPSDocumentNonParticipatingLandmark* FindNonParticipatingLandmark(const TPSDocument&, UID);
    TPSDocumentNonParticipatingLandmark* FindNonParticipatingLandmark(TPSDocument&, UID);

    // if it exists in the document, returns a pointer to the identified element; otherwise, returns `nullptr`
    const TPSDocumentElement* FindElement(const TPSDocument& doc, const TPSDocumentElementID&);

    // if it exists in the document, returns a pointer to the landmark that has the given name; otherwise, returns `nullptr`
    const TPSDocumentLandmarkPair* FindLandmarkPairByName(const TPSDocument&, const StringName&);
    TPSDocumentLandmarkPair* FindLandmarkPairByName(TPSDocument&, const StringName&);
    const TPSDocumentNonParticipatingLandmark* FindNonParticipatingLandmarkByName(const TPSDocument&, const StringName&);
    TPSDocumentNonParticipatingLandmark* FindNonParticipatingLandmarkByName(TPSDocument&, const StringName&);

    // returns `true` if the document contains an element (landmark, non-participating landmark, etc.) with the given name
    bool ContainsElementWithName(const TPSDocument& doc, const StringName&);

    // returns the (mutable) source/destination of the given landmark pair, if available
    inline std::optional<Vector3>& UpdLocation(TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns the source/destination of the given landmark pair, if available
    inline std::optional<Vector3> const& GetLocation(const TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns `true` if the given landmark pair has a location assigned for `which`
    inline bool HasLocation(const TPSDocumentLandmarkPair& landmarkPair, TPSDocumentInputIdentifier which)
    {
        return GetLocation(landmarkPair, which).has_value();
    }

    // returns the (mutable) source/destination mesh in the given document
    inline Mesh& UpdMesh(TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns the source/destination mesh in the given document
    inline const Mesh& GetMesh(const TPSDocument& doc, TPSDocumentInputIdentifier which)
    {
        static_assert(num_options<TPSDocumentInputIdentifier>() == 2);
        return which == TPSDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns `true` if both the source and destination are defined for the given UI landmark
    inline bool IsFullyPaired(const TPSDocumentLandmarkPair& p)
    {
        return p.maybeSourceLocation && p.maybeDestinationLocation;
    }

    // returns `true` if the given UI landmark has either a source or a destination defined
    inline bool HasSourceOrDestinationLocation(const TPSDocumentLandmarkPair& p)
    {
        return p.maybeSourceLocation || p.maybeDestinationLocation;
    }

    // returns `true` if the document contains at least one "morphing" landmarks (i.e. ignores non-participating landmarks)
    inline bool ContainsLandmarks(const TPSDocument& doc)
    {
        return !doc.landmarkPairs.empty();
    }

    // returns `true` if the document contains at least one non-participating landmark
    inline bool ContainsNonParticipatingLandmarks(const TPSDocument& doc)
    {
        return !doc.nonParticipatingLandmarks.empty();
    }

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns std::nullopt
    std::optional<LandmarkPair3D<float>> TryExtractLandmarkPair(const TPSDocumentLandmarkPair&);

    // returns all fully paired landmarks in `doc`
    std::vector<LandmarkPair3D<float>> GetLandmarkPairs(const TPSDocument&);

    // returns all fully paired landmarks, incl. their names, in `doc`
    std::vector<NamedLandmarkPair3D> GetNamedLandmarkPairs(const TPSDocument&);

    // returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(const TPSDocument&, TPSDocumentInputIdentifier);

    // returns the next available unique landmark name
    StringName NextLandmarkName(const TPSDocument&);

    // returns the next available unique non-participating landmark name
    StringName NextNonParticipatingLandmarkName(const TPSDocument&);

    // adds a source/destination landmark at the given location
    void AddLandmarkToInput(
        TPSDocument&,
        TPSDocumentInputIdentifier,
        const Vector3&,
        std::optional<std::string_view> suggestedName = std::nullopt
    );

    // adds a non-participating landmark to the document
    void AddNonParticipatingLandmark(
        TPSDocument&,
        const Vector3&,
        std::optional<std::string_view> suggestedName = std::nullopt
    );

    // returns `true` if the given element was deleted from the document
    bool DeleteElementByID(TPSDocument&, const TPSDocumentElementID&);
    bool DeleteElementByID(TPSDocument&, UID);

    // returns the name of the element, or an alternative if the element cannot be found
    CStringView FindElementNameOr(const TPSDocument&, const TPSDocumentElementID&, CStringView alternative = "unknown");

    // returns all element IDs for all elements in the document
    std::vector<TPSDocumentElementID> GetAllElementIDs(const TPSDocument&);
}
