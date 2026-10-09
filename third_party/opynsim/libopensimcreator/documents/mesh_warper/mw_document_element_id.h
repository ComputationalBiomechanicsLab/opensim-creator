#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document_element_type.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_input_identifier.h>

#include <liboscar/utilities/hash_helpers.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <functional>

namespace osc
{
    /// An associative identifier that can be used to look up a specific
    /// `MwDocumentElement` in a `MwDocument`. This can use useful for
    /// selection logic, tracking changes, etc.
    struct MwDocumentElementID final {

        friend bool operator==(const MwDocumentElementID&, const MwDocumentElementID&) = default;

        UID uid;
        MwDocumentElementType type;
        MiDocumentInputIdentifier input = MiDocumentInputIdentifier::Source;
    };
}

template<>
struct std::hash<osc::MwDocumentElementID> final {
    size_t operator()(const osc::MwDocumentElementID& el) const
    {
        return osc::hash_of(el.uid, el.type, el.input);
    }
};
