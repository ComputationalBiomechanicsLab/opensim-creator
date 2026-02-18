#pragma once

#include <libopensimcreator/documents/mesh_warper/tps_document_element_type.h>
#include <libopensimcreator/documents/mesh_warper/tps_document_input_identifier.h>

#include <liboscar/utilities/hash_helpers.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <functional>

namespace osc
{
    // an associative identifier that points to a specific part of a TPS document
    //
    // (handy for selection logic etc.)
    struct TPSDocumentElementID final {

        friend bool operator==(const TPSDocumentElementID&, const TPSDocumentElementID&) = default;

        UID uid;
        TPSDocumentElementType type;
        TPSDocumentInputIdentifier input = TPSDocumentInputIdentifier::Source;
    };
}

template<>
struct std::hash<osc::TPSDocumentElementID> final {
    size_t operator()(const osc::TPSDocumentElementID& el) const
    {
        return osc::hash_of(el.uid, el.type, el.input);
    }
};
