#pragma once

#include <libopensimcreator/Documents/MeshWarper/TPSDocumentElementType.h>
#include <libopensimcreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>

#include <liboscar/Utils/HashHelpers.h>
#include <liboscar/Utils/UID.h>

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
