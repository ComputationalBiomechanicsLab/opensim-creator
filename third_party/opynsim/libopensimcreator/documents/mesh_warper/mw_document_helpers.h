#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_named_landmark_pair_3d.h>
#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_element_id.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>

#include <libopynsim/utilities/landmark_pair_3d.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>
#include <liboscar/utilities/string_name.h>

#include <cstddef>
#include <optional>
#include <vector>

// TPS document helper functions
namespace osc
{
    // if it exists in the document, returns the position of the given landmark in its mesh coordinate system.
    std::optional<Vector3> FindLandmarkLocation(const MwDocument&, UID, MiDocumentInputIdentifier, MwDocumentElementType);

    // if it exists in the document, translates the given landmark by the given translation vector in the mesh's coordinate system.
    bool TranslateLandmarkByID(MwDocument&, UID, MiDocumentInputIdentifier, MwDocumentElementType, const Vector3& translation);

    // if it exists in the document, returns a pointer to the identified landmark pair; otherwise, returns `nullptr`
    const MwDocumentLandmarkPair* FindLandmarkPair(const MwDocument&, UID);
    MwDocumentLandmarkPair* FindLandmarkPair(MwDocument&, UID);

    // if it exists in the document, returns a pointer to the identified non-participating landmark; otherwise, returns `nullptr`
    const MwDocumentNonParticipatingLandmark* FindNonParticipatingLandmark(const MwDocument&, UID);
    MwDocumentNonParticipatingLandmark* FindNonParticipatingLandmark(MwDocument&, UID);

    // if it exists in the document, returns a pointer to the identified element; otherwise, returns `nullptr`
    const MwDocumentElement* FindElement(const MwDocument& doc, const MwDocumentElementID&);

    // if it exists in the document, returns a pointer to the landmark that has the given name; otherwise, returns `nullptr`
    const MwDocumentLandmarkPair* FindLandmarkPairByName(const MwDocument&, const StringName&);
    MwDocumentLandmarkPair* FindLandmarkPairByName(MwDocument&, const StringName&);
    const MwDocumentNonParticipatingLandmark* FindNonParticipatingLandmarkByName(const MwDocument&, const StringName&);
    MwDocumentNonParticipatingLandmark* FindNonParticipatingLandmarkByName(MwDocument&, const StringName&);

    // returns `true` if the document contains an element (landmark, non-participating landmark, etc.) with the given name
    bool ContainsElementWithName(const MwDocument& doc, const StringName&);

    // returns the (mutable) source/destination of the given landmark pair, if available
    inline std::optional<Vector3>& UpdLocation(MwDocumentLandmarkPair& landmarkPair, MiDocumentInputIdentifier which)
    {
        static_assert(num_options<MiDocumentInputIdentifier>() == 2);
        return which == MiDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns the source/destination of the given landmark pair, if available
    inline std::optional<Vector3> const& GetLocation(const MwDocumentLandmarkPair& landmarkPair, MiDocumentInputIdentifier which)
    {
        static_assert(num_options<MiDocumentInputIdentifier>() == 2);
        return which == MiDocumentInputIdentifier::Source ? landmarkPair.maybeSourceLocation : landmarkPair.maybeDestinationLocation;
    }

    // returns `true` if the given landmark pair has a location assigned for `which`
    inline bool HasLocation(const MwDocumentLandmarkPair& landmarkPair, MiDocumentInputIdentifier which)
    {
        return GetLocation(landmarkPair, which).has_value();
    }

    // returns the (mutable) source/destination mesh in the given document
    inline Mesh& UpdMesh(MwDocument& doc, MiDocumentInputIdentifier which)
    {
        static_assert(num_options<MiDocumentInputIdentifier>() == 2);
        return which == MiDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns the source/destination mesh in the given document
    inline const Mesh& GetMesh(const MwDocument& doc, MiDocumentInputIdentifier which)
    {
        static_assert(num_options<MiDocumentInputIdentifier>() == 2);
        return which == MiDocumentInputIdentifier::Source ? doc.sourceMesh : doc.destinationMesh;
    }

    // returns `true` if both the source and destination are defined for the given UI landmark
    inline bool IsFullyPaired(const MwDocumentLandmarkPair& p)
    {
        return p.maybeSourceLocation && p.maybeDestinationLocation;
    }

    // returns `true` if the given UI landmark has either a source or a destination defined
    inline bool HasSourceOrDestinationLocation(const MwDocumentLandmarkPair& p)
    {
        return p.maybeSourceLocation || p.maybeDestinationLocation;
    }

    // returns `true` if the document contains at least one "morphing" landmarks (i.e. ignores non-participating landmarks)
    inline bool ContainsLandmarks(const MwDocument& doc)
    {
        return !doc.landmarkPairs.empty();
    }

    // returns `true` if the document contains at least one non-participating landmark
    inline bool ContainsNonParticipatingLandmarks(const MwDocument& doc)
    {
        return !doc.nonParticipatingLandmarks.empty();
    }

    // returns source + destination landmark pair, if both are fully defined; otherwise, returns std::nullopt
    std::optional<opyn::LandmarkPair3D<float>> TryExtractLandmarkPair(const MwDocumentLandmarkPair&);

    // returns all fully paired landmarks in `doc`
    std::vector<opyn::LandmarkPair3D<float>> GetLandmarkPairs(const MwDocument&);

    // returns all fully paired landmarks, incl. their names, in `doc`
    std::vector<MwNamedLandmarkPair3D> GetNamedLandmarkPairs(const MwDocument&);

    // returns the count of landmarks in the document for which `which` is defined
    size_t CountNumLandmarksForInput(const MwDocument&, MiDocumentInputIdentifier);

    // returns the next available unique landmark name
    StringName NextLandmarkName(const MwDocument&);

    // returns the next available unique non-participating landmark name
    StringName NextNonParticipatingLandmarkName(const MwDocument&);

    // adds a source/destination landmark at the given location
    void AddLandmarkToInput(
        MwDocument&,
        MiDocumentInputIdentifier,
        const Vector3&,
        std::optional<std::string_view> suggestedName = std::nullopt
    );

    // adds a non-participating landmark to the document
    void AddNonParticipatingLandmark(
        MwDocument&,
        const Vector3&,
        std::optional<std::string_view> suggestedName = std::nullopt
    );

    // returns `true` if the given element was deleted from the document
    bool DeleteElementByID(MwDocument&, const MwDocumentElementID&);
    bool DeleteElementByID(MwDocument&, UID);

    // returns the name of the element, or an alternative if the element cannot be found
    CStringView FindElementNameOr(const MwDocument&, const MwDocumentElementID&, CStringView alternative = "unknown");

    // returns all element IDs for all elements in the document
    std::vector<MwDocumentElementID> GetAllElementIDs(const MwDocument&);
}
